/* ui.h - peças de interface para jogos de estratégia: números formatados,
   listas, abas, um diário de mensagens e janelas que esperam a resposta do
   jogador */
#ifndef UI_H
#define UI_H

#include <stddef.h>

#include "tela.h"

/* ---- números e tempo, no jeito brasileiro ---- */
void fmt_num(char *saida, size_t tamanho, double valor);   /* 1.234.567 */
void fmt_curto(char *saida, size_t tamanho, double valor); /* 12,3k  4,56M */
/* 1d 02h 3h 05m 4m 12s */
void fmt_tempo(char *saida, size_t tamanho, double segundos);
/* Copia o texto completando com espaços (ou cortando) até ter exatamente
   'largura' caracteres na tela. Ao contrário do %-20s do printf, conta letras,
   não bytes. */
void fmt_ajusta(char *saida, size_t tamanho, const char *texto, int largura);

/* ---- listas com um item selecionado ---- */
typedef struct {
    int sel;  /* item selecionado */
    int topo; /* primeiro item visível (para rolar listas compridas) */
} Lista;

/* Trata as setas, PgUp, PgDn, Home e End. Devolve 1 se a tecla era de lista. */
int lista_tecla(Lista *l, int tecla, int n, int visiveis);
void lista_ajusta(Lista *l, int n, int visiveis);
void lista_desenha(int x, int y, int l, int a, const char *const *itens, int n,
                   const Lista *ls);

/* ---- abas: "1 Recursos  2 Frota  3 Pesquisa", com a atual destacada ---- */
void ui_abas(int x, int y, const char *const *nomes, int n, int atual);

/* ---- diário: as últimas mensagens do jogo, com marcas de cor ---- */
#define DIARIO_MAX 64
typedef struct {
    char texto[DIARIO_MAX][160];
    int total; /* quantas mensagens já chegaram (as antigas são esquecidas) */
} Diario;
void diario_poe(Diario *d, const char *formato, ...);
void diario_desenha(const Diario *d, int x, int y, int l, int a);

/* ---- janelas que esperam o jogador ---- */
void ui_aviso(const char *titulo, const char *texto);
int ui_confirma(const char *titulo, const char *pergunta); /* 1 = sim */
/* -1 = desistiu */
int ui_escolha(const char *titulo, const char *const *opcoes, int n);
int ui_le_texto(const char *titulo, const char *rotulo, char *buf, int tamanho);
int ui_le_numero(const char *titulo, const char *rotulo, int min, int max,
                 int *valor);

#endif
