/* conselho.c - um evento com escolhas e consequências */
#include <stdio.h>

int main(void) {
    int comida = 600;
    int colonos = 3000;
    int moral = 60; /* de 0 a 100 */
    int escolha;

    printf("=== CONSELHO DA COLÔNIA ===\n\n");
    printf("Uma tempestade de poeira destruiu metade da colheita.\n");
    printf("Comida: %d t   Colonos: %d   Moral: %d%%\n\n", comida, colonos,
           moral);
    printf("O que o conselho deve fazer?\n");
    printf("  1) Racionar a comida (a moral cai)\n");
    printf("  2) Comprar comida da estação orbital (custa 400 de metal)\n");
    printf("  3) Não fazer nada e torcer pela próxima colheita\n");
    printf("Escolha: ");
    scanf("%d", &escolha);
    printf("\n");

    switch (escolha) {
    case 1:
        moral = moral - 15;
        printf("O racionamento é anunciado. Ninguém passa fome, mas há "
               "protestos.\n");
        break;
    case 2:
        comida = comida + 500;
        printf("Os cargueiros chegam ao entardecer com 500 t de grãos.\n");
        break;
    case 3: printf("O conselho decide esperar.\n"); break;
    default:
        printf("O conselho não entende a ordem e se dispersa.\n");
        escolha = 3;
        break;
    }

    /* cada colono come 0,3 t até a próxima colheita */
    int necessario = colonos * 3 / 10;
    if (comida >= necessario) {
        printf("Há comida suficiente até a próxima colheita.\n");
    } else if (escolha == 1) {
        printf("Com o racionamento, a comida dura até a próxima colheita.\n");
    } else {
        int famintos = (necessario - comida) * 10 / 3;
        moral = moral - 30;
        printf("Faltam %d t de comida: %d colonos passam fome.\n",
               necessario - comida, famintos);
    }

    if (moral < 0) moral = 0;
    printf("Moral final: %d%% ", moral);
    if (moral >= 50) printf("(a colônia confia em você)\n");
    else if (moral >= 25) printf("(há rumores de revolta)\n");
    else printf("(a colônia está à beira de um motim!)\n");
    return 0;
}
