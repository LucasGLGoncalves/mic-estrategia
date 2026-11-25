/* producao.c - quanto uma mina produz e quanto custa melhorá-la */
#include <math.h>
#include <stdio.h>

int main(void) {
    int nivel;

    printf("Nível atual da mina de metal: ");
    scanf("%d", &nivel);

    /* produção por hora: cresce um pouco mais rápido que o nível */
    double producao = 30.0 * nivel * pow(1.1, nivel);
    /* custo do próximo nível: cada nível custa 50% a mais que o anterior */
    double custo = 60.0 * pow(1.5, nivel);
    double producao_prox = 30.0 * (nivel + 1) * pow(1.1, nivel + 1);
    double ganho = producao_prox - producao;
    double horas = custo / ganho;

    printf("\nNível %d produz %.0f de metal por hora.\n", nivel, producao);
    printf("O nível %d custa %.0f de metal e produz %.0f por hora.\n",
           nivel + 1, custo, producao_prox);
    printf("O ganho é de %.0f por hora: a melhoria se paga em %.1f horas.\n",
           ganho, horas);
    printf("(isso dá %d dias e %d horas)\n", (int)horas / 24, (int)horas % 24);
    return 0;
}
