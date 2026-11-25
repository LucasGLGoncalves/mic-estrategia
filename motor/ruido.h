/* ruido.h - ruído de valor e fBm: números "aleatórios" que variam suavemente,
   para gerar continentes, nuvens, temperaturas e a superfície dos planetas */
#ifndef RUIDO_H
#define RUIDO_H

#include <stdint.h>

double ruido1(uint64_t semente, double x);                     /* de 0 a 1 */
double ruido2(uint64_t semente, double x, double y);           /* de 0 a 1 */
double ruido3(uint64_t semente, double x, double y, double z); /* de 0 a 1 */

/* fBm: várias camadas (oitavas) de ruído somadas, cada uma com o dobro da
   frequência e metade da força da anterior. Devolve de 0 a 1. */
double fbm2(uint64_t semente, double x, double y, int oitavas);
double fbm3(uint64_t semente, double x, double y, double z, int oitavas);

#endif
