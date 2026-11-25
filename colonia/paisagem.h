/* paisagem.h - o horizonte da colônia, com o céu, o relevo e as construções */
#ifndef PAISAGEM_H
#define PAISAGEM_H

#include "colonia.h"

#define PAISAGEM_ALTURA 10

/* Desenha a paisagem a partir da linha y0. 'agora' é o relógio real, para as
   animações; a hora do dia vem do relógio da colônia. */
void paisagem_desenha(const Colonia *c, int y0, double agora);

#endif
