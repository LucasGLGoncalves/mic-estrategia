/* rng.h - acaso sob controle: gerador xoshiro256**, sementes, hashes e dados */
#ifndef RNG_H
#define RNG_H

#include <stdint.h>

typedef struct {
    uint64_t s[4]; /* o estado inteiro do gerador: dá para gravar e restaurar */
} Rng;

void rng_semente(Rng *r, uint64_t semente);
uint64_t rng_proximo(Rng *r); /* 64 bits aleatórios */
/* de 0 (inclusive) a 1 (exclusive) */
double rng_real(Rng *r);
/* de min a max, inclusive, sem vício */
int rng_faixa(Rng *r, int min, int max);
int rng_dado(Rng *r, int lados);               /* de 1 a lados */
int rng_dados(Rng *r, int quantos, int lados); /* NdL: soma de vários dados */
/* 1 com probabilidade p (de 0 a 1) */
int rng_chance(Rng *r, double p);
double rng_entre(Rng *r, double min, double max);
double rng_normal(Rng *r, double media, double desvio); /* curva de sino */
/* índice sorteado pelos pesos */
int rng_ponderado(Rng *r, const double *pesos, int n);
void rng_embaralha(Rng *r, int *v, int n);

/* Hashes: transformam textos e coordenadas em números bem misturados. */
uint64_t hash_texto(const char *s); /* FNV-1a */
uint64_t hash_mistura(uint64_t x);  /* finalizador do splitmix64 */
uint64_t hash_xy(uint64_t semente, int x, int y);

#endif
