/* ia.h - o almirante do computador: caça os navios com probabilidade */
#ifndef IA_H
#define IA_H

#include "tabuleiro.h"

/* Calcula, para cada casa ainda não atirada do mar 'alvo', quantas posições dos
   navios restantes passariam por ela, considerando o que já se sabe. */
void ia_calcula_mapa(const Tabuleiro *alvo, int mapa[TAM][TAM]);
/* Escolhe a casa de maior probabilidade (sorteando entre os empates). */
void ia_escolhe_tiro(const Tabuleiro *alvo, Rng *r, int *x, int *y);

#endif
