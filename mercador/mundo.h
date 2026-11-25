/* mundo.h - a galáxia do mercador: planetas, mercadorias e mercados */
#ifndef MUNDO_H
#define MUNDO_H

#include "../motor/rng.h"
#include "../motor/ui.h"

#define N_PLANETAS    8
#define N_BENS        6
#define DIAS_DE_JOGO  60
#define CARGA_INICIAL 20

typedef enum {
    AGRICOLA,
    MINERADOR,
    INDUSTRIAL,
    TECNOLOGICO,
    FRONTEIRA,
    N_TIPOS
} TipoPlaneta;

typedef struct {
    const char *nome;
    double base; /* preço de referência na galáxia */
    int ilegal;  /* proibido fora dos planetas de fronteira */
} Bem;

typedef struct {
    const char *nome;
    int x, y; /* posição no mapa estelar */
    TipoPlaneta tipo;
    int risco;     /* 0 a 3: chance de piratas nas rotas que chegam aqui */
    int estaleiro; /* vende melhorias para a nave */
    double estoque[N_BENS], ideal[N_BENS];
} Planeta;

typedef struct {
    Planeta planetas[N_PLANETAS];
    int dia, aqui;
    double creditos, divida;
    int carga[N_BENS];
    /* preço médio pago, para o jogador saber se tem lucro */
    double pago[N_BENS];
    int carga_max, combustivel, combustivel_max, casco, canhoes, escudo;
    int terminou;
    Rng rng;
    Diario diario;
} Jogo;

extern const Bem BENS[N_BENS];
extern const char *NOME_TIPO[N_TIPOS];

void mundo_novo(Jogo *j, uint64_t semente);
double preco(const Jogo *j, int planeta, int bem);
double preco_compra(const Jogo *j, int planeta, int bem);
double preco_venda(const Jogo *j, int planeta, int bem);
int carga_usada(const Jogo *j);
int compra(Jogo *j, int bem, int quantidade); /* devolve quantas comprou */
int vende(Jogo *j, int bem, int quantidade);
void passa_um_dia(Jogo *j);
double patrimonio(const Jogo *j);
double distancia(const Jogo *j, int a, int b);
int dias_de_viagem(const Jogo *j, int destino);
int combustivel_da_viagem(const Jogo *j, int destino);

#endif
