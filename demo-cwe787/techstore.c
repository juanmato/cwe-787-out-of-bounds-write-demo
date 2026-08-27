/* techstore.c — Demo CWE-787 autocontenida (servidor + página + lógica en C).
 *
 * Un solo binario: sirve la página del checkout y ejecuta la lógica VULNERABLE o
 * CORREGIDA en su propio proceso. No necesita Python, ni compilador, ni nada más
 * en la máquina que lo corre. Doble clic (Windows) o ./techstore (mac/Linux).
 *
 * El bug (CWE-787) es real: en modo "vuln", coupon[16] y discount_percent son
 * campos contiguos y la copia con strcpy() no controla el límite, así que un cupón
 * de más de 16 bytes se DESBORDA y sobre-escribe el descuento.
 *
 * Compilación portable: usa Winsock en Windows y BSD sockets en POSIX. Toda la
 * lógica HTTP y del exploit es idéntica en ambos; solo cambia el arranque del socket.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "page.h"   /* la página index.html embebida (PAGE / PAGE_LEN) */

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
  #define BADSOCK INVALID_SOCKET
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  typedef int sock_t;
  #define CLOSESOCK close
  #define BADSOCK (-1)
#endif

#define PORT 8000

/* ----------------- Lógica de la tienda (idéntica a vuln.c / secure.c) ----------------- */

static int lookup_discount(const char *code) {
    if (strcmp(code, "SALE10") == 0) return 10;
    if (strcmp(code, "SALE25") == 0) return 25;
    return 0;
}

/* Copia INSEGURA: recibe char* genérico para reproducir el strcpy() del mundo real. */
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void copy_input(char *dst, const char *src) {
    strcpy(dst, src);            /* CWE-787: out-of-bounds write (sin límite) */
}

/* Copia SEGURA: nunca escribe más de dstsize bytes y termina en NUL. */
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void copy_input_safe(char *dst, size_t dstsize, const char *src) {
    strncpy(dst, src, dstsize - 1);
    dst[dstsize - 1] = '\0';
}

/* Corre la lógica y arma el cuerpo JSON de la respuesta en 'out'. */
static void run_logic(const char *mode, const char *coupon, char *out, size_t outsz) {
    const long BASE = 1200;
    int secure = (strcmp(mode, "secure") == 0);

    struct {
        char coupon[16];       /* bytes  0..15 */
        int  discount_percent; /* bytes 16..19  <-- víctima del desborde */
        char _pad[64];         /* colchón: evita que la demo crashee */
    } cart;
    memset(&cart, 0, sizeof(cart));

    int rejected = 0;
    if (secure) {
        if (strlen(coupon) >= sizeof(cart.coupon)) rejected = 1;
        else copy_input_safe(cart.coupon, sizeof(cart.coupon), coupon);
    } else {
        copy_input(cart.coupon, coupon);   /* <--- acá ocurre el desborde */
    }

    int valid = 0; long final = BASE; int discount = 0;
    const char *result;
    if (rejected) {
        result = "REJECTED";
    } else {
        valid = (lookup_discount(cart.coupon) > 0);
        if (valid) cart.discount_percent = lookup_discount(cart.coupon);
        discount = cart.discount_percent;
        long d = discount;
        final = BASE - (BASE * d) / 100;
        if (final < 0) final = 0;
        result = (final == 0) ? "FREE" : (valid ? "DISCOUNTED" : "FULL");
    }

    /* volcado de los 20 bytes */
    unsigned char *raw = (unsigned char *)&cart;
    char hexarr[128] = "";
    for (int i = 0; i < 20; i++) {
        char h[8]; snprintf(h, sizeof(h), "%s\"%02x\"", i ? "," : "", rejected ? 0 : raw[i]);
        strncat(hexarr, h, sizeof(hexarr) - strlen(hexarr) - 1);
    }

    snprintf(out, outsz,
        "{\"bytes\":[%s],\"discount\":%d,\"coupon_valid\":%s,\"base\":%ld,"
        "\"final\":%ld,\"result\":\"%s\",\"raw\":\"DISCOUNT: %d\\nCOUPON_VALID: %d\\nFINAL: %ld\\nRESULT: %s\"}",
        hexarr, discount, valid ? "true" : "false", BASE, final, result,
        discount, valid, final, result);
}

/* ----------------- Utilidades HTTP ----------------- */

/* Extrae el valor string de "key":"..." dentro de un cuerpo JSON simple. */
static void json_get(const char *body, const char *key, char *out, size_t outsz) {
    out[0] = '\0';
    char pat[64]; snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(body, pat);
    if (!p) return;
    p = strchr(p + strlen(pat), ':'); if (!p) return;
    p = strchr(p, '"'); if (!p) return; p++;   /* apertura de comillas del valor */
    size_t i = 0;
    while (*p && *p != '"' && i < outsz - 1) out[i++] = *p++;
    out[i] = '\0';
}

static void send_all(sock_t c, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int n = send(c, buf + sent, (int)(len - sent), 0);
        if (n <= 0) break;
        sent += (size_t)n;
    }
}

static void handle(sock_t c) {
    char req[16384]; int total = 0, n;
    /* leer hasta el fin de headers */
    while (total < (int)sizeof(req) - 1) {
        n = recv(c, req + total, (int)sizeof(req) - 1 - total, 0);
        if (n <= 0) { req[total] = 0; return; }
        total += n; req[total] = 0;
        if (strstr(req, "\r\n\r\n")) break;
    }
    char *body = strstr(req, "\r\n\r\n");
    body = body ? body + 4 : req + total;

    if (strncmp(req, "POST /api/check", 15) == 0) {
        /* asegurar cuerpo completo segun Content-Length */
        int clen = 0; char *cl = strstr(req, "Content-Length:");
        if (cl) clen = atoi(cl + 15);
        int have = total - (int)(body - req);
        while (have < clen && total < (int)sizeof(req) - 1) {
            n = recv(c, req + total, (int)sizeof(req) - 1 - total, 0);
            if (n <= 0) break;
            total += n; req[total] = 0;
            body = strstr(req, "\r\n\r\n") + 4;
            have = total - (int)(body - req);
        }
        char mode[32], coupon[4096];
        json_get(body, "mode", mode, sizeof(mode));
        json_get(body, "coupon", coupon, sizeof(coupon));
        if (!mode[0]) strcpy(mode, "vuln");
        char json[8192];
        run_logic(mode, coupon, json, sizeof(json));
        char head[256];
        int hl = snprintf(head, sizeof(head),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
            "Content-Length: %d\r\nConnection: close\r\n\r\n", (int)strlen(json));
        send_all(c, head, hl);
        send_all(c, json, strlen(json));
    } else {
        /* servir la página embebida */
        char head[256];
        int hl = snprintf(head, sizeof(head),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %u\r\nConnection: close\r\n\r\n", PAGE_LEN);
        send_all(c, head, hl);
        send_all(c, (const char *)PAGE, PAGE_LEN);
    }
}

int main(void) {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == BADSOCK) { fprintf(stderr, "No se pudo crear el socket.\n"); return 1; }
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
#ifdef BIND_ANY
    addr.sin_addr.s_addr = htonl(INADDR_ANY);        /* 0.0.0.0 (necesario dentro de Docker) */
#else
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   /* solo localhost (evita el Firewall en Windows) */
#endif

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "No se pudo abrir el puerto %d (¿ya hay algo corriendo?).\n", PORT);
        return 1;
    }
    listen(s, 8);
    printf("\n  Demo CWE-787 (TechStore) corriendo en:  http://localhost:%d\n\n", PORT);
    printf("  Abri esa direccion en el navegador. Ctrl+C para detener.\n\n");
    fflush(stdout);

    for (;;) {
        sock_t c = accept(s, NULL, NULL);
        if (c == BADSOCK) continue;
        handle(c);
        CLOSESOCK(c);
    }
    return 0;
}
