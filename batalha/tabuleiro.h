/* tabuleiro.h - o mar de cada jogador: navios, tiros e o que cada lado sabe */
#ifndef TABULEIRO_H
#define TABULEIRO_H

#include "../motor/rng.h"

#define TAM      10
#define N_NAVIOS 5

typedef enum { AGUA, ERRO, ACERTO } Marca; /* o que se sabe de cada casa */

typedef struct {
    const char *nome;
    int tamanho;
    int x, y;     /* a casa da ponta */
    int vertical; /* 0 = deitado, 1 = em pé */
    int acertos;
    int posicionado;
} Navio;

typedef struct {
    Navio navios[N_NAVIOS];
    int ocupado[TAM][TAM]; /* -1 = água; senão, o índice do navio */
    Marca tiros[TAM][TAM]; /* os tiros que este mar já recebeu */
} Tabuleiro;

typedef enum {
    TIRO_INVALIDO,
    TIRO_AGUA,
    TIRO_ACERTO,
    TIRO_AFUNDOU
} ResultadoTiro;

void tab_inicia(Tabuleiro *t);
int tab_cabe(const Tabuleiro *t, int tamanho, int x, int y, int vertical);
int tab_posiciona(Tabuleiro *t, int navio, int x, int y, int vertical);
void tab_posiciona_tudo(Tabuleiro *t, Rng *r);
ResultadoTiro tab_atira(Tabuleiro *t, int x, int y, int *navio_atingido);
int tab_afundado(const Tabuleiro *t, int navio);
int tab_restantes(const Tabuleiro *t);

#endif
