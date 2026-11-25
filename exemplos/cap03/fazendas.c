/* fazendas.c - lendo números do teclado com scanf */
#include <stdio.h>

int main(void) {
    int fazendas;
    double colheita;

    printf("Quantas fazendas a colônia tem? ");
    scanf("%d", &fazendas);
    printf("Quantas toneladas cada fazenda colhe por ano? ");
    scanf("%lf", &colheita);

    printf("\nCom %d fazendas colhendo %.1f t cada,\n", fazendas, colheita);
    printf("a colônia terá %.1f toneladas de comida por ano.\n",
           fazendas * colheita);
    return 0;
}
