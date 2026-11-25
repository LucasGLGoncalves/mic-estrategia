/* tabuleiro.c - o mar de cada jogador */
#include "tabuleiro.h"

static const Navio FROTA[N_NAVIOS] = {
    {"Porta-aviões", 5, 0, 0, 0, 0, 0}, {"Encouraçado", 4, 0, 0, 0, 0, 0},
    {"Cruzador", 3, 0, 0, 0, 0, 0},     {"Submarino", 3, 0, 0, 0, 0, 0},
    {"Destróier", 2, 0, 0, 0, 0, 0},
};

void tab_inicia(Tabuleiro *t) {
    for (int i = 0; i < N_NAVIOS; i++) t->navios[i] = FROTA[i];
    for (int y = 0; y < TAM; y++)
        for (int x = 0; x < TAM; x++) {
            t->ocupado[y][x] = -1;
            t->tiros[y][x] = AGUA;
        }
}

/* Um navio cabe se não sai do mar e não encosta em outro (nem pelas
   diagonais). */
int tab_cabe(const Tabuleiro *t, int tamanho, int x, int y, int vertical) {
    int x2 = vertical ? x : x + tamanho - 1,
        y2 = vertical ? y + tamanho - 1 : y;
    if (x < 0 || y < 0 || x2 >= TAM || y2 >= TAM) return 0;
    for (int j = y - 1; j <= y2 + 1; j++)
        for (int i = x - 1; i <= x2 + 1; i++)
            if (i >= 0 && j >= 0 && i < TAM && j < TAM && t->ocupado[j][i] >= 0)
                return 0;
    return 1;
}

int tab_posiciona(Tabuleiro *t, int navio, int x, int y, int vertical) {
    Navio *n = &t->navios[navio];
    if (n->posicionado || !tab_cabe(t, n->tamanho, x, y, vertical)) return 0;
    n->x = x;
    n->y = y;
    n->vertical = vertical;
    n->posicionado = 1;
    for (int k = 0; k < n->tamanho; k++) {
        int cx = vertical ? x : x + k, cy = vertical ? y + k : y;
        t->ocupado[cy][cx] = navio;
    }
    return 1;
}

void tab_posiciona_tudo(Tabuleiro *t, Rng *r) {
    for (int i = 0; i < N_NAVIOS; i++) {
        while (!t->navios[i].posicionado) {
            int v = rng_faixa(r, 0, 1);
            tab_posiciona(t, i, rng_faixa(r, 0, TAM - 1),
                          rng_faixa(r, 0, TAM - 1), v);
        }
    }
}

int tab_afundado(const Tabuleiro *t, int navio) {
    return t->navios[navio].acertos >= t->navios[navio].tamanho;
}

ResultadoTiro tab_atira(Tabuleiro *t, int x, int y, int *navio_atingido) {
    if (x < 0 || y < 0 || x >= TAM || y >= TAM || t->tiros[y][x] != AGUA)
        return TIRO_INVALIDO;
    int n = t->ocupado[y][x];
    *navio_atingido = n;
    if (n < 0) {
        t->tiros[y][x] = ERRO;
        return TIRO_AGUA;
    }
    t->tiros[y][x] = ACERTO;
    t->navios[n].acertos++;
    return tab_afundado(t, n) ? TIRO_AFUNDOU : TIRO_ACERTO;
}

int tab_restantes(const Tabuleiro *t) {
    int n = 0;
    for (int i = 0; i < N_NAVIOS; i++)
        if (!tab_afundado(t, i)) n++;
    return n;
}
