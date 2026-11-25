/* contagem.c - a contagem regressiva do lançamento, com while e com for */
#include <stdio.h>

int main(void) {
    int t = 5;
    while (t > 0) {
        printf("T-%d... ", t);
        t--;
    }
    printf("Ignição!\n");

    for (int estagio = 1; estagio <= 3; estagio++) {
        printf("Estágio %d separado.\n", estagio);
    }
    printf("Órbita alcançada.\n");
    return 0;
}
