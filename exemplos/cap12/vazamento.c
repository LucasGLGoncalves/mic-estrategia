/* vazamento.c - dois defeitos clássicos de memória, de propósito */
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *producao = malloc(5 * sizeof(int));
    for (int i = 0; i <= 5; i++) { /* <= : escreve uma posição além do fim! */
        producao[i] = i * 100;
    }
    printf("produção do dia 3: %d\n", producao[3]);

    int *relatorio = malloc(100 * sizeof(int));
    relatorio[0] = 1;
    printf("relatório criado\n");
    /* esquecemos o free(relatorio): vazamento */
    free(producao);
    return 0;
}
