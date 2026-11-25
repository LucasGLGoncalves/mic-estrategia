/* telas.h - as telas do mercador */
#ifndef TELAS_H
#define TELAS_H

#include "../motor/ui.h"
#include "mundo.h"

typedef enum {
    ABA_MERCADO,
    ABA_MAPA,
    ABA_NAVE,
    ABA_BANCO,
    ABA_DIARIO,
    N_ABAS
} Aba;

typedef enum {
    M_CARGA,
    M_CANHAO,
    M_ESCUDO,
    M_REPARO,
    M_COMBUSTIVEL,
    N_MELHORIAS
} Melhoria;

typedef struct {
    Aba aba;
    Lista bens, destinos, melhorias;
} Interface;

/* a lista de destinos pula o planeta atual */
int destino_da_linha(const Jogo *j, int linha);
double custo_melhoria(const Jogo *j, Melhoria m);
const char *nome_melhoria(Melhoria m);
void desenha(const Jogo *j, const Interface *ui, double agora);
void desenha_viagem(const Jogo *j, int origem, int destino, double progresso);

#endif
