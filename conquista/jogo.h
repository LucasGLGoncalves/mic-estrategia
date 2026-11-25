/* jogo.h - unidades, visão, combate, produção e turnos */
#ifndef JOGO_H
#define JOGO_H

#include "../motor/rng.h"
#include "../motor/ui.h"
#include "mapa.h"

#define MAX_UNIDADES 160

typedef struct {
    const char *nome;
    char letra;
    int forca, vida, movimento, visao;
    int custo;   /* turnos de produção */
    int captura; /* pode tomar cidades? */
} DadosUnidade;

extern const DadosUnidade UNIDADES[N_TIPOS];

typedef enum { ORDEM_NENHUMA, ORDEM_SENTINELA, ORDEM_IR } Ordem;

typedef struct {
    int viva;
    TipoUnidade tipo;
    Dono dono;
    int x, y;
    int vida, movimento; /* movimento: pontos que restam neste turno */
    int agiu;            /* andou ou lutou neste turno? quem descansa se cura */
    Ordem ordem;
    int alvo_x, alvo_y; /* destino da ordem IR */
} Unidade;

typedef struct {
    Mapa mapa;
    Unidade unidades[MAX_UNIDADES];
    int turno;
    Dono vez;
    /* o que cada lado já viu (névoa de guerra) e o que vê agora */
    unsigned char visto[N_DONOS][MAPA_A][MAPA_L];
    unsigned char visivel[N_DONOS][MAPA_A][MAPA_L];
    /* dono de cada cidade, da última vez que foi vista */
    Dono lembra[N_DONOS][MAX_CIDADES];
    int perdas[N_DONOS], vitorias[N_DONOS];
    Dono vencedor; /* NEUTRO enquanto a guerra continua */
    Rng rng;
    Diario diario;
} Jogo;

void jogo_novo(Jogo *j, uint64_t semente);
void jogo_atualiza_visao(Jogo *j, Dono d);
int unidade_em(const Jogo *j, int x, int y); /* índice ou -1 */
int unidade_cria(Jogo *j, TipoUnidade tipo, Dono d, int x, int y);
int cidades_de(const Jogo *j, Dono d);
int unidades_de(const Jogo *j, Dono d);

/* Força efetiva de quem defende a casa (x, y): unidade, guarnição ou
   milícia. */
double forca_defesa(const Jogo *j, int x, int y, int *vida);
/* Estimativa da chance de vitória de um ataque, simulando 400 lutas. */
double chance_de_vitoria(const Jogo *j, int u, int x, int y);

/* Tenta mover a unidade u uma casa (dx, dy); ataca se houver inimigo.
   Devolve 1 se algo aconteceu (andou ou lutou). */
int unidade_move(Jogo *j, int u, int dx, int dy);

/* Procura um caminho para a unidade u até (ax, ay), com A*, usando só o que o
   dono dela conhece do mapa. Grava a direção do primeiro passo e devolve o
   custo total, ou -1 se não há caminho. */
int caminho_para(const Jogo *j, int u, int ax, int ay, int *dx, int *dy);

/* O caminho inteiro, em pares x,y (até max passos); devolve o número de passos
   (-1 se não há) e grava o custo em *custo, se custo != NULL. */
int caminho_lista(const Jogo *j, int u, int ax, int ay, int *xy, int max,
                  int *custo);

void turno_comeca(Jogo *j, Dono d); /* produção, cura e movimento novo */
void cidade_produz(Jogo *j, int c, TipoUnidade tipo);

#endif
