/* util.h - gerador aleatório, hash, UTF-8, formatação de números e tempo */
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#ifndef RGB
#define RGB(r, g, b)                                                                   \
    ((uint32_t)((((uint32_t)(r) & 255u) << 16) | (((uint32_t)(g) & 255u) << 8) |       \
                ((uint32_t)(b) & 255u)))
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    uint64_t s[4];
} Rng;

uint64_t splitmix64(uint64_t *x);
uint64_t hash64(uint64_t x);
uint64_t hash_combine(uint64_t a, uint64_t b);
uint64_t fnv1a(const void *data, size_t n, uint64_t h);
double hash_unit(uint64_t h); /* [0,1) */

void rng_seed(Rng *r, uint64_t seed);
uint64_t rng_next(Rng *r);
double rng_double(Rng *r);             /* [0,1) */
int rng_range(Rng *r, int lo, int hi); /* inclusivo */
double rng_uniform(Rng *r, double lo, double hi);
double rng_normal(Rng *r);
double rng_exp(Rng *r, double mean);
int rng_chance(Rng *r, double p);
int rng_d(Rng *r, int sides); /* 1..lados */
long long rng_binomial(Rng *r, long long n, double p);

/* UTF-8 */
uint32_t utf8_next(const char **s); /* 0 no fim */
int utf8_encode(uint32_t cp, char *out);
int utf8_len(const char *s);
void utf8_pop(char *s);
void utf8_append(char *s, size_t cap, uint32_t cp);

/* formatação (pt-BR) */
void fmt_num(char *buf, size_t n, double v);       /* 1.234.567 */
void fmt_short(char *buf, size_t n, double v);     /* 12,3k  4,56M */
void fmt_time(char *buf, size_t n, double secs);   /* 1d 02h 03m */
void fmt_clock(char *buf, size_t n, double epoch); /* 24/11 14:59:02 */
void fmt_pct(char *buf, size_t n, double v);       /* +12% */

double clampd(double v, double lo, double hi);
int clampi(int v, int lo, int hi);
double now_seconds(void);
void sleep_ms(int ms);
double ipow(double b, int e);

/* ruído de valor */
double noise3(uint64_t seed, double x, double y, double z);
double fbm3(uint64_t seed, double x, double y, double z, int oct);
double noise1(uint64_t seed, double x);

#endif
