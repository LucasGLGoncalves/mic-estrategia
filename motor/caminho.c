/* caminho.c - busca em largura, Dijkstra e A* numa grade */
#include "caminho.h"

#include <stdlib.h>

/* Uma fila de prioridade (heap binário): tira sempre o item de menor
   prioridade. */
typedef struct {
    int *celula, *prioridade;
    int n;
} Heap;

static void heap_poe(Heap *h, int celula, int prioridade) {
    int i = h->n++;
    h->celula[i] = celula;
    h->prioridade[i] = prioridade;
    while (i > 0) { /* sobe enquanto for menor que o pai */
        int pai = (i - 1) / 2;
        if (h->prioridade[pai] <= h->prioridade[i]) break;
        int tc = h->celula[pai], tp = h->prioridade[pai];
        h->celula[pai] = h->celula[i];
        h->prioridade[pai] = h->prioridade[i];
        h->celula[i] = tc;
        h->prioridade[i] = tp;
        i = pai;
    }
}

static int heap_tira(Heap *h) {
    int topo = h->celula[0];
    h->n--;
    h->celula[0] = h->celula[h->n];
    h->prioridade[0] = h->prioridade[h->n];
    int i = 0;
    for (;;) { /* desce trocando com o menor dos filhos */
        int e = 2 * i + 1, d = e + 1, menor = i;
        if (e < h->n && h->prioridade[e] < h->prioridade[menor]) menor = e;
        if (d < h->n && h->prioridade[d] < h->prioridade[menor]) menor = d;
        if (menor == i) break;
        int tc = h->celula[menor], tp = h->prioridade[menor];
        h->celula[menor] = h->celula[i];
        h->prioridade[menor] = h->prioridade[i];
        h->celula[i] = tc;
        h->prioridade[i] = tp;
        i = menor;
    }
    return topo;
}

ResultadoBusca caminho_busca(int largura, int altura, int x0, int y0, int x1,
                             int y1, CustoCelula custo, void *dados,
                             ModoBusca modo, int *caminho_xy, int max,
                             int *distancias) {
    ResultadoBusca r = {-1, 0, 0};
    int total = largura * altura;
    int *dist = malloc(sizeof(int) * (size_t)total);
    int *veio = malloc(sizeof(int) * (size_t)total);
    /* cada célula entra no heap no máximo 4 vezes (uma por vizinho) */
    Heap h = {malloc(sizeof(int) * (size_t)total * 4 + 4),
              malloc(sizeof(int) * (size_t)total * 4 + 4), 0};
    if (!dist || !veio || !h.celula || !h.prioridade) goto fim;
    for (int i = 0; i < total; i++) dist[i] = -1;

    int origem = y0 * largura + x0, destino = y1 * largura + x1;
    dist[origem] = 0;
    veio[origem] = -1;
    heap_poe(&h, origem, 0);
    /* na busca em largura, a prioridade é a ordem de chegada */
    int contador = 0;
    static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
    while (h.n > 0) {
        int atual = heap_tira(&h);
        r.explorados++;
        if (atual == destino) break;
        int ax = atual % largura, ay = atual / largura;
        for (int k = 0; k < 4; k++) {
            int nx = ax + DX[k], ny = ay + DY[k];
            if (nx < 0 || ny < 0 || nx >= largura || ny >= altura) continue;
            int c = custo(dados, nx, ny);
            if (c < 0) continue;
            /* a busca em largura ignora os custos */
            if (modo == BUSCA_LARGURA) c = 1;
            int vizinho = ny * largura + nx;
            int novo = dist[atual] + c;
            if (dist[vizinho] >= 0 && dist[vizinho] <= novo) continue;
            dist[vizinho] = novo;
            veio[vizinho] = atual;
            int prioridade = novo;
            if (modo == BUSCA_LARGURA) prioridade = ++contador;
            /* heurística */
            if (modo == BUSCA_ESTRELA)
                prioridade += abs(nx - x1) + abs(ny - y1);
            heap_poe(&h, vizinho, prioridade);
        }
    }
    if (dist[destino] >= 0) {
        int n = 0;
        for (int c = destino; c != origem; c = veio[c]) n++;
        r.passos = n;
        /* o custo de verdade, mesmo para a busca em largura, que o ignorou */
        r.custo = 0;
        for (int c = destino; c != origem; c = veio[c])
            r.custo += custo(dados, c % largura, c / largura);
        int i = n;
        /* remonta de trás para a frente */
        for (int c = destino; c != origem; c = veio[c]) {
            i--;
            if (i < max && caminho_xy != NULL) {
                caminho_xy[2 * i] = c % largura;
                caminho_xy[2 * i + 1] = c / largura;
            }
        }
    }
    if (distancias != NULL)
        for (int i = 0; i < total; i++) distancias[i] = dist[i];
fim:
    free(dist);
    free(veio);
    free(h.celula);
    free(h.prioridade);
    return r;
}
