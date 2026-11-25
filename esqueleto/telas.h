/* telas.h - a interface do Posto Avançado */
#ifndef TELAS_H
#define TELAS_H

#include "../motor/ui.h"
#include "jogo.h"

#define TELA_L 100
#define TELA_A 30

/* O estado da interface: separado do jogo, e não é salvo */
typedef struct {
    int cx, cy;         /* o cursor no mapa */
    Construcao escolha; /* o que a tecla Enter constrói */
    int velocidade;     /* 1, 10 ou 100 */
    Diario diario;      /* os recados do jogo, já recolhidos */
} Interface;

void interface_inicia(Interface *ui, const Jogo *j);
/* Recolhe os recados que o jogo deixou e os passa para o diário. */
void busca_avisos(Jogo *j, Interface *ui);
void desenha(const Jogo *j, const Interface *ui);
/* Trata uma tecla. Devolve 0 se o jogador quer sair. */
int trata_tecla(Jogo *j, Interface *ui, int tecla);

#endif
