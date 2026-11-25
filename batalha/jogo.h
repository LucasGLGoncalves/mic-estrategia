/* jogo.h - o estado de uma partida de batalha naval */
#ifndef JOGO_H
#define JOGO_H

#include "../motor/rng.h"
#include "../motor/ui.h"
#include "tabuleiro.h"

typedef enum { POSICIONANDO, JOGANDO, FIM } Fase;

typedef struct {
    Tabuleiro meu, dele; /* o mar do jogador e o do computador */
    Fase fase;
    int cx, cy;   /* o cursor */
    int vertical; /* orientação do navio sendo posicionado */
    int navio;    /* qual navio está sendo posicionado */
    int vez_do_computador;
    /* quando o computador vai atirar (para dar um suspense) */
    double hora_do_tiro;
    int ultimo_x, ultimo_y;
    /* tecla H: mostra o que o computador "pensa" do seu mar */
    int mostra_calor;
    int tiros_jogador, tiros_computador;
    int venceu;
    Rng rng;
    Diario diario;
} Jogo;

void jogo_novo(Jogo *j, uint64_t semente);
void jogo_tecla(Jogo *j, int tecla, double agora);
void jogo_atualiza(Jogo *j, double agora);
void jogo_desenha(const Jogo *j, double agora);
void nome_casa(int x, int y,
               char *saida); /* 3, 6 vira "D7"; saida com 16 lugares */

#endif
