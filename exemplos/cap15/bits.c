/* bits.c - tipos de tamanho exato, operações com bits e sinalizadores */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

/* Características de um planeta, uma por bit. */
enum {
    TEM_LUA = 1u << 0,       /* 0000 0001 */
    TEM_ANEIS = 1u << 1,     /* 0000 0010 */
    TEM_ATMOSFERA = 1u << 2, /* 0000 0100 */
    TEM_AGUA = 1u << 3,      /* 0000 1000 */
    TEM_VIDA = 1u << 4,      /* 0001 0000 */
};

void mostra_binario(uint8_t v) {
    for (int bit = 7; bit >= 0; bit--) {
        putchar((v >> bit) & 1 ? '1' : '0');
        if (bit == 4) putchar(' ');
    }
}

int conta_bits(uint32_t v) {
    int n = 0;
    while (v) {
        n += v & 1;
        v >>= 1;
    }
    return n;
}

int main(void) {
    uint8_t planeta = TEM_ATMOSFERA | TEM_AGUA; /* liga dois bits */
    printf("planeta: ");
    mostra_binario(planeta);
    printf("  (%u)\n", planeta);

    planeta |= TEM_LUA;            /* liga um bit */
    planeta &= (uint8_t)~TEM_AGUA; /* desliga um bit */
    planeta ^= TEM_VIDA;           /* inverte um bit */
    printf("depois:  ");
    mostra_binario(planeta);
    printf("  lua? %s  água? %s  vida? %s  (%d características)\n",
           planeta & TEM_LUA ? "sim" : "não",
           planeta & TEM_AGUA ? "sim" : "não",
           planeta & TEM_VIDA ? "sim" : "não", conta_bits(planeta));

    /* deslocar é multiplicar ou dividir por potências de 2 */
    printf("\n3 << 4 = %d   200 >> 3 = %d\n", 3 << 4, 200 >> 3);

    /* uma cor de 24 bits guardada num único número: 0xRRGGBB */
    uint32_t laranja = (255u << 16) | (140u << 8) | 30u;
    printf("laranja = 0x%06" PRIX32 ": vermelho %u, verde %u, azul %u\n",
           laranja, (unsigned)((laranja >> 16) & 0xFF),
           (unsigned)((laranja >> 8) & 0xFF), (unsigned)(laranja & 0xFF));

    /* inteiros sem sinal dão a volta, sem erro, quando passam do limite */
    uint8_t contador = 250;
    contador += 10;
    printf("uint8_t 250 + 10 = %u\n", contador);
    uint64_t grande = UINT64_MAX;
    printf("UINT64_MAX = %" PRIu64 " = 0x%" PRIX64 "\n", grande, grande);
    printf("UINT64_MAX + 1 = %" PRIu64 "\n", grande + 1);
    return 0;
}
