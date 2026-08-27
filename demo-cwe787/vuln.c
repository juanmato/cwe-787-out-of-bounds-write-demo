// vuln.c — Tienda online VULNERABLE (CWE-787: Out-of-bounds Write)
//
// El codigo de cupon (coupon[16]) y el porcentaje de descuento (discount_percent)
// son campos CONTIGUOS en memoria. apply_coupon() usa strcpy() SIN control de
// longitud: si el cupon supera 16 bytes, la escritura se DESBORDA del buffer y
// sobre-escribe 'discount_percent' con basura enorme -> el precio final queda en $0.
#include <stdio.h>
#include <string.h>

// Cupones legitimos de la tienda:  SALE10 -> 10%   SALE25 -> 25%
static int lookup_discount(const char *code) {
    if (strcmp(code, "SALE10") == 0) return 10;
    if (strcmp(code, "SALE25") == 0) return 25;
    return 0;
}

// Copia insegura: recibe 'char *' generico para reproducir el strcpy() del mundo real.
__attribute__((noinline))
void apply_coupon(char *dst, const char *src) {
    strcpy(dst, src);            // CWE-787: out-of-bounds write (sin limite)
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("RESULT: ERROR\n"); return 1; }
    const long BASE = 1200;      // precio del producto (USD)

    struct {
        char coupon[16];         // bytes  0..15
        int  discount_percent;   // bytes 16..19  <-- victima del desborde
        char _pad[64];           // colchon: evita que la demo crashee
    } cart;
    memset(&cart, 0, sizeof(cart));

    apply_coupon(cart.coupon, argv[1]);            // <--- aca ocurre el desborde

    int valid = (lookup_discount(cart.coupon) > 0);
    if (valid) cart.discount_percent = lookup_discount(cart.coupon);

    // Precio final (con 'long' para no romper al recibir un descuento gigante)
    long d = cart.discount_percent;
    long final = BASE - (BASE * d) / 100;
    if (final < 0) final = 0;                      // clamp: nunca negativo

    unsigned char *raw = (unsigned char *)&cart;
    printf("BYTES:");
    for (int i = 0; i < 20; i++) printf(" %02x", raw[i]);
    printf("\n");
    printf("DISCOUNT: %d\n", cart.discount_percent);
    printf("COUPON_VALID: %d\n", valid);
    printf("BASE: %ld\n", BASE);
    printf("FINAL: %ld\n", final);
    printf("RESULT: %s\n", final == 0 ? "FREE" : (valid ? "DISCOUNTED" : "FULL"));
    fflush(stdout);
    return 0;
}
