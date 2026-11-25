/* util.c - gerador aleatório, hash, UTF-8, formatação, ruído */
#ifndef _WIN32
/* localtime_r e nanosleep são do POSIX, não do C11: é preciso pedi-las antes
 * de qualquer #include */
#define _POSIX_C_SOURCE 200809L
#endif
#include "util.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

uint64_t splitmix64(uint64_t *x) {
    uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

uint64_t hash64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

uint64_t hash_combine(uint64_t a, uint64_t b) {
    return hash64(a ^ (b + 0x9E3779B97F4A7C15ULL + (a << 6) + (a >> 2)));
}

uint64_t fnv1a(const void *data, size_t n, uint64_t h) {
    const unsigned char *p = (const unsigned char *)data;
    if (h == 0) h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < n; i++) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

double hash_unit(uint64_t h) {
    return (double)(hash64(h) >> 11) * (1.0 / 9007199254740992.0);
}

static inline uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

void rng_seed(Rng *r, uint64_t seed) {
    uint64_t x = seed;
    for (int i = 0; i < 4; i++) r->s[i] = splitmix64(&x);
}

uint64_t rng_next(Rng *r) {
    uint64_t *s = r->s;
    uint64_t result = rotl(s[1] * 5, 7) * 9;
    uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl(s[3], 45);
    return result;
}

double rng_double(Rng *r) {
    return (double)(rng_next(r) >> 11) * (1.0 / 9007199254740992.0);
}

int rng_range(Rng *r, int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (int)(rng_next(r) % (uint64_t)(hi - lo + 1));
}

double rng_uniform(Rng *r, double lo, double hi) {
    return lo + (hi - lo) * rng_double(r);
}

double rng_normal(Rng *r) {
    double u1 = rng_double(r), u2 = rng_double(r);
    if (u1 < 1e-12) u1 = 1e-12;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

double rng_exp(Rng *r, double mean) {
    double u = rng_double(r);
    if (u < 1e-12) u = 1e-12;
    return -log(u) * mean;
}

int rng_chance(Rng *r, double p) { return rng_double(r) < p; }

int rng_d(Rng *r, int sides) { return rng_range(r, 1, sides); }

long long rng_binomial(Rng *r, long long n, double p) {
    if (n <= 0 || p <= 0) return 0;
    if (p >= 1) return n;
    if (n < 64) {
        long long k = 0;
        for (long long i = 0; i < n; i++)
            if (rng_double(r) < p) k++;
        return k;
    }
    double mean = n * p, sd = sqrt(n * p * (1 - p));
    long long k = (long long)llround(mean + sd * rng_normal(r));
    if (k < 0) k = 0;
    if (k > n) k = n;
    return k;
}

/* ---------------- UTF-8 ---------------- */
uint32_t utf8_next(const char **ps) {
    const unsigned char *s = (const unsigned char *)*ps;
    uint32_t c = s[0];
    if (c == 0) return 0;
    if (c < 0x80) {
        *ps += 1;
        return c;
    }
    if ((c & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        *ps += 2;
        return ((c & 0x1F) << 6) | (s[1] & 0x3F);
    }
    if ((c & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        *ps += 3;
        return ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    }
    if ((c & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 &&
        (s[3] & 0xC0) == 0x80) {
        *ps += 4;
        return ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) |
               (s[3] & 0x3F);
    }
    *ps += 1;
    return '?';
}

int utf8_encode(uint32_t cp, char *o) {
    if (cp < 0x80) {
        o[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        o[0] = (char)(0xC0 | (cp >> 6));
        o[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        o[0] = (char)(0xE0 | (cp >> 12));
        o[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        o[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    o[0] = (char)(0xF0 | (cp >> 18));
    o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    o[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    o[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

int utf8_len(const char *s) {
    int n = 0;
    while (utf8_next(&s)) n++;
    return n;
}

void utf8_pop(char *s) {
    size_t n = strlen(s);
    if (!n) return;
    n--;
    while (n > 0 && (((unsigned char)s[n]) & 0xC0) == 0x80) n--;
    s[n] = 0;
}

void utf8_append(char *s, size_t cap, uint32_t cp) {
    char tmp[5];
    int k = utf8_encode(cp, tmp);
    size_t n = strlen(s);
    if (n + (size_t)k + 1 > cap) return;
    memcpy(s + n, tmp, (size_t)k);
    s[n + k] = 0;
}

/* ---------------- formatação ---------------- */
void fmt_num(char *buf, size_t n, double v) {
    char tmp[64];
    int neg = v < 0;
    if (neg) v = -v;
    if (v > 9.0e18) v = 9.0e18;
    long long x = (long long)floor(v + 1e-9);
    snprintf(tmp, sizeof tmp, "%lld", x);
    int len = (int)strlen(tmp);
    char out[96];
    int o = 0;
    if (neg && x != 0) out[o++] = '-';
    for (int i = 0; i < len; i++) {
        out[o++] = tmp[i];
        int rem = len - i - 1;
        if (rem > 0 && rem % 3 == 0) out[o++] = '.';
    }
    out[o] = 0;
    snprintf(buf, n, "%s", out);
}

void fmt_short(char *buf, size_t n, double v) {
    double a = fabs(v);
    const char *sg = v < 0 ? "-" : "";
    char t[32];
    if (a < 100000.0) {
        fmt_num(buf, n, v);
        return;
    }
    if (a < 1e6) snprintf(t, sizeof t, "%s%.1fk", sg, a / 1e3);
    else if (a < 1e9) snprintf(t, sizeof t, "%s%.2fM", sg, a / 1e6);
    else if (a < 1e12) snprintf(t, sizeof t, "%s%.2fB", sg, a / 1e9);
    else snprintf(t, sizeof t, "%s%.2fT", sg, a / 1e12);
    for (char *p = t; *p; p++)
        if (*p == '.') *p = ',';
    snprintf(buf, n, "%s", t);
}

void fmt_time(char *buf, size_t n, double secs) {
    if (secs < 0) secs = 0;
    long long s = (long long)ceil(secs - 1e-6);
    long long d = s / 86400;
    s %= 86400;
    long long h = s / 3600;
    s %= 3600;
    long long m = s / 60;
    s %= 60;
    if (d > 0) snprintf(buf, n, "%lldd %02lldh %02lldm", d, h, m);
    else if (h > 0) snprintf(buf, n, "%lldh %02lldm %02llds", h, m, s);
    else if (m > 0) snprintf(buf, n, "%lldm %02llds", m, s);
    else snprintf(buf, n, "%llds", s);
}

void fmt_clock(char *buf, size_t n, double epoch) {
    time_t t = (time_t)epoch;
    struct tm tmv;
#ifdef _WIN32
    {
        struct tm *lt = localtime(&t);
        if (lt) tmv = *lt;
        else memset(&tmv, 0, sizeof tmv);
    }
#else
    localtime_r(&t, &tmv);
#endif
    snprintf(buf, n, "%02d/%02d %02d:%02d:%02d", tmv.tm_mday, tmv.tm_mon + 1,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

void fmt_pct(char *buf, size_t n, double v) { snprintf(buf, n, "%+.0f%%", v * 100.0); }

double clampd(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

double now_seconds(void) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long t =
        ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return (double)(t - 116444736000000000ULL) / 1e7;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + tv.tv_usec / 1e6;
#endif
}

void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
#endif
}

double ipow(double b, int e) {
    double r = 1.0;
    int neg = e < 0;
    if (neg) e = -e;
    while (e) {
        if (e & 1) r *= b;
        b *= b;
        e >>= 1;
    }
    return neg ? 1.0 / r : r;
}

/* ---------------- ruído ---------------- */
static double lattice(uint64_t seed, long long x, long long y, long long z) {
    uint64_t h = seed;
    h = hash_combine(h, (uint64_t)x);
    h = hash_combine(h, (uint64_t)y);
    h = hash_combine(h, (uint64_t)z);
    return hash_unit(h) * 2.0 - 1.0;
}

static double smooth(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }

double noise3(uint64_t seed, double x, double y, double z) {
    long long xi = (long long)floor(x), yi = (long long)floor(y),
              zi = (long long)floor(z);
    double xf = x - xi, yf = y - yi, zf = z - zi;
    double u = smooth(xf), v = smooth(yf), w = smooth(zf);
    double c000 = lattice(seed, xi, yi, zi), c100 = lattice(seed, xi + 1, yi, zi);
    double c010 = lattice(seed, xi, yi + 1, zi),
           c110 = lattice(seed, xi + 1, yi + 1, zi);
    double c001 = lattice(seed, xi, yi, zi + 1),
           c101 = lattice(seed, xi + 1, yi, zi + 1);
    double c011 = lattice(seed, xi, yi + 1, zi + 1),
           c111 = lattice(seed, xi + 1, yi + 1, zi + 1);
    double x00 = c000 + u * (c100 - c000), x10 = c010 + u * (c110 - c010);
    double x01 = c001 + u * (c101 - c001), x11 = c011 + u * (c111 - c011);
    double y0 = x00 + v * (x10 - x00), y1 = x01 + v * (x11 - x01);
    return y0 + w * (y1 - y0);
}

double fbm3(uint64_t seed, double x, double y, double z, int oct) {
    double sum = 0, amp = 0.5, f = 1.0, norm = 0;
    for (int i = 0; i < oct; i++) {
        sum += amp * noise3(seed + (uint64_t)i * 7919, x * f, y * f, z * f);
        norm += amp;
        amp *= 0.5;
        f *= 2.03;
    }
    return sum / norm;
}

double noise1(uint64_t seed, double x) {
    long long xi = (long long)floor(x);
    double t = smooth(x - xi);
    double a = lattice(seed, xi, 0, 0), b = lattice(seed, xi + 1, 0, 0);
    return a + t * (b - a);
}
