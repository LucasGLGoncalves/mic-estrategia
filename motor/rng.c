/* rng.c - acaso sob controle */
#include "rng.h"

#include <math.h>

/* splitmix64: espalha uma semente qualquer pelos 256 bits do estado */
static uint64_t splitmix64(uint64_t *x) {
    uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void rng_semente(Rng *r, uint64_t semente) {
    for (int i = 0; i < 4; i++) r->s[i] = splitmix64(&semente);
}

static uint64_t gira(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

/* xoshiro256** (Blackman e Vigna, 2018): rápido, pequeno e de ótima
   qualidade */
uint64_t rng_proximo(Rng *r) {
    uint64_t *s = r->s;
    uint64_t resultado = gira(s[1] * 5, 7) * 9;
    uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = gira(s[3], 45);
    return resultado;
}

double rng_real(Rng *r) {
    /* os 53 bits mais altos viram a parte quebrada de um double */
    return (double)(rng_proximo(r) >> 11) * (1.0 / 9007199254740992.0);
}

int rng_faixa(Rng *r, int min, int max) {
    if (max <= min) return min;
    uint64_t n = (uint64_t)((int64_t)max - min) + 1;
    /* descarta a sobra que viciaria o resto */
    uint64_t limite = UINT64_MAX - UINT64_MAX % n;
    uint64_t x;
    do { x = rng_proximo(r); } while (x >= limite);
    return (int)((int64_t)min + (int64_t)(x % n));
}

int rng_dado(Rng *r, int lados) { return rng_faixa(r, 1, lados); }

int rng_dados(Rng *r, int quantos, int lados) {
    int soma = 0;
    for (int i = 0; i < quantos; i++) soma += rng_dado(r, lados);
    return soma;
}

int rng_chance(Rng *r, double p) { return rng_real(r) < p; }

double rng_entre(Rng *r, double min, double max) {
    return min + (max - min) * rng_real(r);
}

double rng_normal(Rng *r, double media, double desvio) {
    /* transformação de Box-Muller: dois uniformes viram um valor da curva de
       sino */
    double u1 = rng_real(r), u2 = rng_real(r);
    if (u1 < 1e-300) u1 = 1e-300;
    return media + desvio * sqrt(-2.0 * log(u1)) * cos(6.283185307179586 * u2);
}

int rng_ponderado(Rng *r, const double *pesos, int n) {
    double total = 0;
    for (int i = 0; i < n; i++) total += pesos[i];
    double x = rng_real(r) * total;
    for (int i = 0; i < n; i++) {
        if (x < pesos[i]) return i;
        x -= pesos[i];
    }
    return n - 1;
}

void rng_embaralha(Rng *r, int *v, int n) {
    for (int i = n - 1; i > 0; i--) { /* Fisher-Yates */
        int j = rng_faixa(r, 0, i);
        int t = v[i];
        v[i] = v[j];
        v[j] = t;
    }
}

uint64_t hash_texto(const char *s) {
    uint64_t h = 14695981039346656037ULL;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

uint64_t hash_mistura(uint64_t x) {
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

uint64_t hash_xy(uint64_t semente, int x, int y) {
    uint64_t h = semente ^ ((uint64_t)(uint32_t)x * 0x9E3779B97F4A7C15ULL);
    h = hash_mistura(h ^ ((uint64_t)(uint32_t)y * 0xC2B2AE3D27D4EB4FULL));
    return h;
}
