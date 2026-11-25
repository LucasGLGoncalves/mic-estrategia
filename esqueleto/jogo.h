/* jogo.h - o estado do jogo e as suas regras, sem nenhuma tela.

   SEU JOGO: este é o arquivo para mudar primeiro. Troque os recursos, os
   terrenos e as construções pelos do seu jogo; as tabelas ficam em jogo.c. */
#ifndef JOGO_H
#define JOGO_H

#include <stdint.h>

#include "../motor/rng.h"

#define MAPA_L     48 /* largura do mapa, em quadras */
#define MAPA_A     22 /* altura do mapa */
#define MAX_AVISOS 8

typedef enum { COMIDA, MADEIRA, PEDRA, N_RECURSOS } Recurso;
typedef enum { AGUA, PLANICIE, FLORESTA, MONTANHA, N_TERRENOS } Terreno;
typedef enum {
    NADA,
    CASA,
    FAZENDA,
    SERRARIA,
    PEDREIRA,
    N_CONSTRUCOES
} Construcao;

/* Uma linha da tabela de construções: tudo o que o jogo sabe sobre cada uma */
typedef struct {
    const char *nome;
    char letra;      /* como aparece no mapa */
    Terreno terreno; /* onde pode ser construída */
    double custo[N_RECURSOS];
    double produz[N_RECURSOS]; /* por segundo, com todos os trabalhadores */
    int moradores;             /* quantas pessoas abriga */
    int trabalhadores;         /* quantas pessoas trabalham nela */
    const char *descricao;
} DadosConstrucao;

extern const DadosConstrucao CONSTRUCOES[N_CONSTRUCOES];
extern const char *const NOME_RECURSO[N_RECURSOS];
extern const char *const NOME_TERRENO[N_TERRENOS];

typedef struct {
    Terreno terreno; /* vem da semente: não precisa ser salvo */
    Construcao obra; /* vem do jogador: precisa ser salvo */
} Quadra;

typedef struct {
    uint64_t semente;
    Rng rng;
    double relogio; /* segundos de jogo desde a fundação */
    double recurso[N_RECURSOS];
    double populacao;
    double proximo_evento; /* em segundos de jogo */
    Quadra mapa[MAPA_A][MAPA_L];
    /* recados para a tela, como os toasts do Novo Mundo: o jogo deixa, a
       interface busca. O jogo não sabe se alguém vai ler. */
    char aviso[MAX_AVISOS][120];
    int n_avisos;
} Jogo;

/* Tudo o que se calcula a partir do estado, sem mudá-lo */
typedef struct {
    double producao[N_RECURSOS]; /* por segundo, já com a eficiência */
    double fome;                 /* comida consumida por segundo */
    int moradias, vagas;         /* lugares para morar e para trabalhar */
    double eficiencia;           /* de 0 a 1: há gente para todas as vagas? */
} Taxas;

typedef enum {
    OK,
    ERRO_FORA,
    ERRO_TERRENO,
    ERRO_OCUPADA,
    ERRO_RECURSOS
} Resultado;

void jogo_novo(Jogo *j, uint64_t semente);
void jogo_gera_mapa(Jogo *j); /* o terreno, a partir da semente */
void jogo_taxas(const Jogo *j, Taxas *t);
void jogo_avanca(Jogo *j, double segundos);
int jogo_quantas(const Jogo *j, Construcao c);
/* O preço da próxima: cada construção do mesmo tipo custa 15% mais. */
void custo_atual(const Jogo *j, Construcao c, double custo[N_RECURSOS]);
Resultado pode_construir(const Jogo *j, int x, int y, Construcao c);
Resultado constroi(Jogo *j, int x, int y, Construcao c);
int demole(Jogo *j, int x, int y); /* devolve metade do último custo */
const char *texto_resultado(Resultado r);
double jogo_pontos(const Jogo *j);
void jogo_avisa(Jogo *j, const char *formato, ...);

#endif
