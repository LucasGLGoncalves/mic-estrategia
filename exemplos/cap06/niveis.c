/* niveis.c - a tabela de níveis da mina de metal, calculada com um laço */
#include <math.h>
#include <stdio.h>

int main(void) {
    double custo_total = 0;

    printf("Nível   Produção/h        Custo   Custo total   Paga em\n");
    printf("-----   ----------   ----------   -----------   -------\n");
    for (int nivel = 1; nivel <= 20; nivel++) {
        double producao = 30.0 * nivel * pow(1.1, nivel);
        double custo = 60.0 * pow(1.5, nivel - 1);
        double anterior = 30.0 * (nivel - 1) * pow(1.1, nivel - 1);
        custo_total += custo;
        printf("%5d   %10.0f   %10.0f   %11.0f   %6.1fh\n", nivel, producao,
               custo, custo_total, custo / (producao - anterior));
    }
    return 0;
}
