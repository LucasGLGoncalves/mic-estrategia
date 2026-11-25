/* colonia.h - a colônia: recursos, estruturas, pesquisas e o relógio */
#ifndef COLONIA_H
#define COLONIA_H

#include "../motor/ui.h"

typedef enum { METAL, CRISTAL, COMIDA, CIENCIA, N_RECURSOS } Recurso;

typedef enum {
    MINA_METAL,
    MINA_CRISTAL,
    FAZENDA,
    USINA,
    HABITACAO,
    ARMAZEM,
    LABORATORIO,
    PORTAL,
    N_ESTRUTURAS
} Estrutura;

typedef enum {
    ROBOTICA,
    MINERACAO,
    FOTOSSINTESE,
    SUPERCONDUTORES,
    ARQUITETURA,
    DOBRA,
    N_PESQUISAS
} Pesquisa;

typedef struct {
    const char *nome, *descricao;
    double metal, cristal; /* custo do nível 1 */
    double fator;          /* cada nível custa 'fator' vezes o anterior */
    double tempo;          /* segundos de obra do nível 1 */
    int trabalhadores;     /* por nível */
} DadosEstrutura;

typedef struct {
    const char *nome, *efeito;
    double custo; /* em ciência */
    int requer;   /* pesquisa necessária antes, ou -1 */
} DadosPesquisa;

extern const char *NOME_RECURSO[N_RECURSOS];
extern const DadosEstrutura ESTRUTURAS[N_ESTRUTURAS];
extern const DadosPesquisa PESQUISAS[N_PESQUISAS];

#define MAX_FILA 5
/* o tempo fora vale até 8 horas, a meio ritmo */
#define MAX_OFFLINE (8 * 3600.0)

typedef struct {
    Estrutura estrutura;
    int nivel;
    double duracao; /* segundos, calculados na hora da encomenda */
} Obra;

typedef struct {
    double recurso[N_RECURSOS];
    double populacao;
    int nivel[N_ESTRUTURAS];
    int pesquisado[N_PESQUISAS];
    Obra fila[MAX_FILA];
    int n_fila;
    double trabalhado; /* segundos de obra na primeira encomenda da fila */
    double relogio;    /* segundos de jogo desde a fundação */
    int venceu;
    Diario diario;
} Colonia;

/* Tudo o que a colônia faz por segundo, calculado a partir dos níveis. */
typedef struct {
    double producao[N_RECURSOS]; /* líquida: a comida já desconta o consumo */
    double energia_gerada, energia_gasta, eficiencia_energia;
    int trabalhadores; /* quantos as estruturas pedem */
    double eficiencia_trabalho;
    double capacidade; /* quantas pessoas as habitações abrigam */
    /* quanto metal, cristal e comida o armazém guarda */
    double estoque_max;
    double crescimento; /* pessoas por segundo */
} Taxas;

void colonia_nova(Colonia *c);
Taxas colonia_taxas(const Colonia *c);
void colonia_avanca(Colonia *c, double segundos);

void custo_de(Estrutura e, int nivel, double *metal, double *cristal);
double duracao_de(const Colonia *c, Estrutura e, int nivel);
int nivel_planejado(const Colonia *c, Estrutura e); /* o nível depois da fila */
/* Pode encomendar? Se não, *motivo diz por quê. */
int pode_construir(const Colonia *c, Estrutura e, const char **motivo);
int encomenda(Colonia *c, Estrutura e);
void cancela_ultima(Colonia *c);
int pode_pesquisar(const Colonia *c, Pesquisa p);
int pesquisa(Colonia *c, Pesquisa p);

#endif
