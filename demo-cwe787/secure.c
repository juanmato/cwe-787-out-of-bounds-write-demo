// secure.c — Tienda online CORREGIDA (mitiga CWE-787)
//
// Misma tienda, misma struct... pero la copia del cupon AHORA respeta el tamano
// del buffer. Es imposible escribir mas alla de coupon[15], asi que
// 'discount_percent' no puede ser sobre-escrito por el input del usuario.
#include <stdio.h>
#include <string.h>

static int lookup_discount(const char *code) {
    if (strcmp(code, "SALE10") == 0) return 10;
    if (strcmp(code, "SALE25") == 0) return 25;
    return 0;
}

// Copia SEGURA: nunca escribe mas de 'dstsize' bytes y garantiza el '\0' final.
__attribute__((noinline))
void apply_coupon_safe(char *dst, size_t dstsize, const char *src) {
    strncpy(dst, src, dstsize - 1);    // FIX: limite explicito de longitud
    dst[dstsize - 1] = '\0';           // FIX: siempre termina en NUL
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("RESULT: ERROR\n"); return 1; }
    const long BASE = 1200;

    struct {
        char coupon[16];
        int  discount_percent;
        char _pad[64];
    } cart;
    memset(&cart, 0, sizeof(cart));

    // Rechazo temprano de cupones absurdamente largos (ademas de la copia acotada):
    if (strlen(argv[1]) >= sizeof(cart.coupon)) {
        printf("BYTES: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00\n");
        printf("DISCOUNT: 0\nCOUPON_VALID: 0\nBASE: 1200\nFINAL: 1200\n");
        printf("RESULT: REJECTED\n");
        fflush(stdout);
        return 0;
    }

    apply_coupon_safe(cart.coupon, sizeof(cart.coupon), argv[1]);  // <--- copia acotada

    int valid = (lookup_discount(cart.coupon) > 0);
    if (valid) cart.discount_percent = lookup_discount(cart.coupon);

    long d = cart.discount_percent;
    long final = BASE - (BASE * d) / 100;
    if (final < 0) final = 0;

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
