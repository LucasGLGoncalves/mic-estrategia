/* divisao.c - as surpresas da divisão inteira */
#include <stdio.h>

int main(void) {
    int colonos = 3150;
    int naves = 4;

    printf("%d colonos em %d naves: %d por nave\n", colonos, naves,
           colonos / naves);
    printf("e sobram %d colonos no porto\n", colonos % naves);
    printf("Como fração: %.2f por nave\n", (double)colonos / naves);
    printf("Cuidado: 7 / 2 = %d, mas 7.0 / 2 = %.1f\n", 7 / 2, 7.0 / 2);
    printf("Um terço de 100 em inteiro: %d\n", 100 / 3);
    printf("Um terço de 100 em double:  %f\n", 100 / 3.0);
    return 0;
}
