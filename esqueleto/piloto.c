/* piloto.c - um piloto automático de regras simples (capítulo 44). Ele só
   usa as funções públicas do jogo, como a interface: se ele consegue jogar,
   as regras estão no jogo, e não na tela. */
#include "piloto.h"

#include <stdlib.h>

/* A quadra livre do terreno certo mais perto do centro do mapa */
static int acha_quadra(const Jogo *j, Construcao c, int *qx, int *qy) {
    int melhor = -1;
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) {
            if (j->mapa[y][x].obra != NADA) continue;
            if (j->mapa[y][x].terreno != CONSTRUCOES[c].terreno) continue;
            int d = abs(x - MAPA_L / 2) + 2 * abs(y - MAPA_A / 2);
            if (melhor < 0 || d < melhor) {
                melhor = d;
                *qx = x;
                *qy = y;
            }
        }
    return melhor >= 0;
}

void piloto_joga(Jogo *j) {
    Taxas t;
    jogo_taxas(j, &t);
    Construcao quer;
    if (t.producao[COMIDA] < t.fome * 1.3) quer = FAZENDA; /* comer primeiro */
    else if (j->populacao > t.moradias - 1 || t.vagas > t.moradias)
        quer = CASA; /* gente para as vagas */
    else if (t.producao[MADEIRA] < 2 * t.producao[PEDRA] + 0.5) quer = SERRARIA;
    else quer = PEDREIRA;
    int x, y;
    if (acha_quadra(j, quer, &x, &y)) constroi(j, x, y, quer);
}
