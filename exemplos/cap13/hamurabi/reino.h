/* reino.h - o estado do reino e as regras de cada ano */
#ifndef REINO_H
#define REINO_H

#define ANOS 10
#define SACAS_POR_PESSOA 20
#define ACRES_POR_PESSOA 10

typedef struct {
    int ano;
    int populacao;
    int graos;
    int terras;
    int preco_terra;
    int colheita;
    int ratos;
    int nascidos;
    int mortos_fome;
    int mortos_peste;
    int total_mortos;
    double soma_percent;
    int revolta;
} Reino;

void reino_novo(Reino *r);
void reino_relatorio(const Reino *r);
int reino_limite_plantio(const Reino *r);
void reino_fim_do_ano(Reino *r, int comida, int plantados);
void reino_avaliacao(const Reino *r);

#endif
