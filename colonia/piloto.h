/* piloto.h - um administrador automático para a colônia */
#ifndef PILOTO_H
#define PILOTO_H

#include "colonia.h"

/* Uma decisão do administrador: pesquisa o que puder e mantém a fila com
   obras. */
void piloto_administra(Colonia *c);

/* Joga sozinho por 'segundos' de jogo, decidindo a cada segundo. */
void piloto_joga(Colonia *c, double segundos);

#endif
