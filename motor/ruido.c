/* ruido.c - ruído de valor e fBm */
#include "ruido.h"

#include <math.h>

#include "rng.h"

/* O valor sorteado num ponto inteiro da grade: sempre o mesmo para a mesma
   semente. */
static double valor(uint64_t semente, int x, int y, int z) {
    uint64_t h = hash_xy(
        semente ^ ((uint64_t)(uint32_t)z * 0xD6E8FEB86659FD93ULL), x, y);
    return (double)(h >> 11) * (1.0 / 9007199254740992.0);
}

/* Curva suave entre 0 e 1: começa e termina devagar, sem "quinas". */
static double suave(double t) { return t * t * (3.0 - 2.0 * t); }

static double mistura(double a, double b, double t) { return a + (b - a) * t; }

double ruido1(uint64_t semente, double x) { return ruido2(semente, x, 0.0); }

double ruido2(uint64_t semente, double x, double y) {
    int x0 = (int)floor(x), y0 = (int)floor(y);
    double tx = suave(x - x0), ty = suave(y - y0);
    double a = valor(semente, x0, y0, 0), b = valor(semente, x0 + 1, y0, 0);
    double c = valor(semente, x0, y0 + 1, 0),
           d = valor(semente, x0 + 1, y0 + 1, 0);
    return mistura(mistura(a, b, tx), mistura(c, d, tx), ty);
}

double ruido3(uint64_t semente, double x, double y, double z) {
    int x0 = (int)floor(x), y0 = (int)floor(y), z0 = (int)floor(z);
    double tx = suave(x - x0), ty = suave(y - y0), tz = suave(z - z0);
    double v[2];
    for (int k = 0; k < 2; k++) {
        double a = valor(semente, x0, y0, z0 + k),
               b = valor(semente, x0 + 1, y0, z0 + k);
        double c = valor(semente, x0, y0 + 1, z0 + k),
               d = valor(semente, x0 + 1, y0 + 1, z0 + k);
        v[k] = mistura(mistura(a, b, tx), mistura(c, d, tx), ty);
    }
    return mistura(v[0], v[1], tz);
}

double fbm2(uint64_t semente, double x, double y, int oitavas) {
    double soma = 0, forca = 1, total = 0, freq = 1;
    for (int i = 0; i < oitavas; i++) {
        soma += forca * ruido2(semente + (uint64_t)i * 101, x * freq, y * freq);
        total += forca;
        forca *= 0.5;
        freq *= 2.0;
    }
    return soma / total;
}

double fbm3(uint64_t semente, double x, double y, double z, int oitavas) {
    double soma = 0, forca = 1, total = 0, freq = 1;
    for (int i = 0; i < oitavas; i++) {
        soma += forca * ruido3(semente + (uint64_t)i * 101, x * freq, y * freq,
                               z * freq);
        total += forca;
        forca *= 0.5;
        freq *= 2.0;
    }
    return soma / total;
}
