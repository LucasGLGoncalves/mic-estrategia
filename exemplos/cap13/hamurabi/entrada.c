/* entrada.c - leitura do teclado e sorteios */
#include "entrada.h"

#include <stdio.h>
#include <stdlib.h>

int le_inteiro(const char *pergunta, int min, int max) {
    int valor;
    for (;;) {
        printf("%s", pergunta);
        int lidos = scanf("%d", &valor);
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
        if (lidos == EOF) {
            exit(0);
        }
        if (lidos == 1 && valor >= min && valor <= max) {
            return valor;
        }
        printf("Digite um número de %d a %d.\n", min, max);
    }
}

int sorteia(int min, int max) {
    return min + rand() % (max - min + 1);
}
