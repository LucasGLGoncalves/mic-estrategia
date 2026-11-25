/* relatorio.c - o relatório da colônia, guardado em variáveis */
#include <stdio.h>

int main(void) {
    int ano = 2387;
    int colonos = 3000;
    int fazendas = 4;
    double comida = 1250.5;      /* toneladas no silo */
    double consumo = 0.35;       /* toneladas por colono, por ano */
    char setor = 'K';
    const int MAX_FAZENDAS = 12;

    printf("=== Relatório da colônia, ano %d ===\n", ano);
    printf("Setor ............ %c\n", setor);
    printf("Colonos .......... %d\n", colonos);
    printf("Fazendas ......... %d de %d\n", fazendas, MAX_FAZENDAS);
    printf("Comida no silo ... %.1f t\n", comida);
    printf("Consumo anual .... %.2f t por colono\n", consumo);

    ano = ano + 1;
    colonos = colonos + 150;
    printf("\nNo ano %d chegaram mais colonos: agora são %d.\n", ano, colonos);
    return 0;
}
