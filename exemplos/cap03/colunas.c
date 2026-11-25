/* colunas.c - alinhando números e textos com a largura do printf */
#include <stdio.h>

int main(void) {
    printf("%-10s %8s %8s\n", "Recurso", "Estoque", "Por hora");
    printf("%-10s %8d %+8d\n", "Metal", 15486, 11457);
    printf("%-10s %8d %+8d\n", "Cristal", 5427, 6258);
    printf("%-10s %8d %+8d\n", "Deutério", 3332, 1519);
    printf("%-10s %8.1f %+8.1f\n", "Energia", 14.0, -62.5);
    printf("\nEm hexadecimal, 255 é %x e 4096 é %X.\n", 255, 4096);
    printf("Com zeros à esquerda: setor %03d, sistema %05d.\n", 7, 479);
    printf("Em notação científica: %e km\n", 149600000.0);
    return 0;
}
