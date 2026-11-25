/* caminho.h - encontrar caminhos numa grade: busca em largura, Dijkstra e A* */
#ifndef CAMINHO_H
#define CAMINHO_H

/* Quanto custa entrar na célula (x, y). Um valor negativo significa
   "bloqueada". */
typedef int (*CustoCelula)(void *dados, int x, int y);

typedef enum { BUSCA_LARGURA, BUSCA_DIJKSTRA, BUSCA_ESTRELA } ModoBusca;

typedef struct {
    /* quantas células tem o caminho (sem contar a origem); -1 se não há */
    int passos;
    int custo;      /* a soma dos custos do caminho */
    int explorados; /* quantas células a busca precisou examinar */
} ResultadoBusca;

/* Procura um caminho de (x0, y0) até (x1, y1), andando nas 4 direções.
   Grava em caminho_xy os pares x,y do caminho (do primeiro passo ao destino),
   até 'max' passos. Se distancias != NULL (largura * altura ints), grava a
   distância de cada célula examinada
   (-1 nas não examinadas), para quem quiser ver como a busca se espalhou. */
ResultadoBusca caminho_busca(int largura, int altura, int x0, int y0, int x1,
                             int y1, CustoCelula custo, void *dados,
                             ModoBusca modo, int *caminho_xy, int max,
                             int *distancias);

#endif
