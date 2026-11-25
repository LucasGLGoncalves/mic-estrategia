/* palitos.c - o jogo dos palitos para dois jogadores */
#include <stdio.h>

int main(void) {
    int palitos = 21;
    int jogador = 1;
    int tirar;

    printf("=== JOGO DOS PALITOS ===\n");
    printf("Há 21 palitos na mesa. Cada um, na sua vez, tira 1, 2 ou 3.\n");
    printf("Quem tirar o último palito perde.\n");

    while (palitos > 0) {
        /* desenha os palitos, em grupos de cinco */
        printf("\n");
        for (int i = 1; i <= palitos; i++) {
            printf("|");
            if (i % 5 == 0) {
                printf("   ");
            } else {
                printf(" ");
            }
        }
        printf(" (%d)\n", palitos);

        /* pede a jogada até ela ser válida */
        do {
            printf("Jogador %d, quantos palitos você tira? ", jogador);
            scanf("%d", &tirar);
            if (tirar < 1 || tirar > 3 || tirar > palitos) {
                printf("Jogada inválida: tire de 1 a 3, sem passar do que há "
                       "na mesa.\n");
            }
        } while (tirar < 1 || tirar > 3 || tirar > palitos);

        palitos -= tirar;
        if (palitos == 0) {
            printf("\nO jogador %d tirou o último palito e perdeu!\n", jogador);
            printf("Vitória do jogador %d.\n", 3 - jogador);
        }
        jogador = 3 - jogador; /* alterna entre 1 e 2 */
    }
    return 0;
}
