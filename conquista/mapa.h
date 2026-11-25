/* mapa.h - o continente: terreno, cidades e as regras do chão */
#ifndef MAPA_H
#define MAPA_H

#include <stdint.h>

#define MAPA_L      34 /* cada casa ocupa duas colunas da tela: 68 colunas */
#define MAPA_A      24
#define MAX_CIDADES 14

typedef enum { MAR, PLANICIE, FLORESTA, MONTANHA, N_TERRENOS } Terreno;

typedef enum { NEUTRO, JOGADOR, COMPUTADOR, N_DONOS } Dono;

typedef enum { EXPLORADOR, EXERCITO, BLINDADO, N_TIPOS } TipoUnidade;

typedef struct {
    char nome[16];
    int x, y;
    Dono dono;
    int capital;
    TipoUnidade produz;
    int progresso; /* turnos de trabalho acumulados na unidade em produção */
} Cidade;

typedef struct {
    Terreno terreno[MAPA_A][MAPA_L];
    Cidade cidades[MAX_CIDADES];
    int n_cidades;
} Mapa;

extern const char *NOME_TERRENO[N_TERRENOS];

/* Gera um continente com cidades; as duas capitais ficam longe uma da outra
   e todas as cidades são alcançáveis por terra. Devolve 0 se não conseguiu. */
int mapa_gera(Mapa *m, uint64_t semente);

int mapa_dentro(int x, int y);
int mapa_passavel(const Mapa *m, int x, int y);
/* pontos de movimento para entrar; -1 = bloqueado */
int custo_terreno(Terreno t);
double defesa_terreno(Terreno t);
int cidade_em(const Mapa *m, int x, int y); /* índice da cidade ou -1 */

#endif
