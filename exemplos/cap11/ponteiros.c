/* ponteiros.c - endereços, ponteiros e o que eles permitem fazer */
#include <stdio.h>

typedef struct {
    const char *nome;
    int naves;
    int combustivel;
} Frota;

/* Troca dois inteiros de lugar: só funciona porque recebe os endereços. */
void troca(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* "Devolve" dois resultados, escrevendo nos endereços que recebeu. */
void divide_colonos(int colonos, int lugares, int *naves, int *sobra) {
    *naves = colonos / lugares;
    *sobra = colonos % lugares;
}

/* Recebe a frota por ponteiro: muda o original, sem copiar a estrutura. */
void reabastece(Frota *f, int quanto) {
    f->combustivel += quanto; /* o mesmo que (*f).combustivel += quanto */
}

int main(void) {
    int metal = 1500;
    int *p = &metal;
    printf("metal vale %d e mora no endereço %p\n", metal, (void *)&metal);
    printf("p guarda o endereço %p, e *p vale %d\n", (void *)p, *p);
    *p = 2000;
    printf("depois de *p = 2000, metal vale %d\n\n", metal);

    int ataque = 7, defesa = 3;
    troca(&ataque, &defesa);
    printf("trocados: ataque = %d, defesa = %d\n", ataque, defesa);

    int naves, sobra;
    divide_colonos(3150, 412, &naves, &sobra);
    printf("3150 colonos em naves de 412: %d naves cheias, %d ficam\n\n", naves,
           sobra);

    Frota alfa = {"Alfa", 12, 300};
    reabastece(&alfa, 450);
    printf("A frota %s tem %d naves e %d de combustível\n", alfa.nome,
           alfa.naves, alfa.combustivel);

    int producao[5] = {10, 20, 30, 40, 50};
    int *q = producao; /* o nome do array já é o endereço do primeiro item */
    printf("producao[2] = %d, *(q + 2) = %d, q[2] = %d\n", producao[2],
           *(q + 2), q[2]);
    printf("endereços: q = %p, q + 1 = %p (4 bytes adiante)\n", (void *)q,
           (void *)(q + 1));

    Frota *nenhuma = NULL;
    if (nenhuma == NULL) { printf("nenhuma frota selecionada\n"); }
    return 0;
}
