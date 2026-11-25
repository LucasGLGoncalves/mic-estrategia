/* semente.c - transformando um nome num número, e o número num universo */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

/* FNV-1a: um "hash" simples e bom; mistura cada byte do texto no resultado. */
uint64_t hash_texto(const char *s) {
    uint64_t h = 14695981039346656037ULL;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

/* xorshift64: um gerador pseudoaleatório de três linhas. */
uint64_t proximo(uint64_t *estado) {
    uint64_t x = *estado;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return *estado = x;
}

int main(void) {
    const char *nomes[] = {"Lucas", "lucas", "Lucas!", "Ane"};
    for (int i = 0; i < 4; i++) {
        printf("hash(\"%s\") = 0x%016" PRIx64 "\n", nomes[i],
               hash_texto(nomes[i]));
    }

    uint64_t estado = hash_texto("Lucas");
    printf("\nPlanetas do universo de \"Lucas\":");
    for (int i = 0; i < 6; i++) {
        printf(" %" PRIu64, proximo(&estado) % 15 + 1);
    }
    estado = hash_texto("Lucas");
    printf("\nDe novo, com a mesma semente:  ");
    for (int i = 0; i < 6; i++) {
        printf(" %" PRIu64, proximo(&estado) % 15 + 1);
    }
    printf("\n");
    return 0;
}
