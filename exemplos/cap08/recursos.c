/* recursos.c - arrays: a produção de uma semana e os estoques da colônia */
#include <stdio.h>

#define DIAS 7

int main(void) {
    int producao[DIAS] = {120, 135, 90, 160, 155, 40, 170};
    const char *nomes[DIAS] = {"seg", "ter", "qua", "qui", "sex", "sáb", "dom"};

    int total = 0;
    int melhor = 0; /* índice do melhor dia */
    for (int i = 0; i < DIAS; i++) {
        total += producao[i];
        if (producao[i] > producao[melhor]) { melhor = i; }
    }
    printf("Produção de metal na semana:\n");
    for (int i = 0; i < DIAS; i++) {
        printf("  %s %4d ", nomes[i], producao[i]);
        for (int k = 0; k < producao[i] / 10; k++) { printf("#"); }
        printf("\n");
    }
    printf("Total: %d   Média: %.1f por dia   Melhor dia: %s\n", total,
           (double)total / DIAS, nomes[melhor]);

    /* um array pode ser modificado item a item */
    producao[5] = producao[5] * 3; /* sábado: a tempestade passou */
    printf("Sábado corrigido: %d\n", producao[5]);
    printf("O array ocupa %zu bytes: %zu itens de %zu bytes.\n",
           sizeof producao, sizeof producao / sizeof producao[0],
           sizeof producao[0]);
    return 0;
}
