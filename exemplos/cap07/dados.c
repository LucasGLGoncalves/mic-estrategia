/* dados.c - rolando dados e fazendo testes de habilidade */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Rola um dado de 'lados' faces: devolve de 1 a lados. */
int rola(int lados) { return rand() % lados + 1; }

/* Rola 'quantos' dados de 'lados' faces e soma: rola_varios(3, 6) é o famoso
   3d6. */
int rola_varios(int quantos, int lados) {
    int soma = 0;
    for (int i = 0; i < quantos; i++) { soma += rola(lados); }
    return soma;
}

/* Um teste de d20: rola, soma o modificador e compara com a classe de
   dificuldade. */
int teste(int modificador, int dificuldade) {
    int dado = rola(20);
    int total = dado + modificador;
    printf("  d20 = %2d %+d = %2d contra CD %d: ", dado, modificador, total,
           dificuldade);
    if (dado == 20) {
        printf("SUCESSO CRÍTICO!\n");
        return 1;
    }
    if (dado == 1) {
        printf("falha crítica...\n");
        return 0;
    }
    printf(total >= dificuldade ? "sucesso\n" : "falha\n");
    return total >= dificuldade;
}

int main(void) {
    srand((unsigned)time(NULL));

    printf("Cinco rolagens de 1d6: ");
    for (int i = 0; i < 5; i++) { printf("%d ", rola(6)); }
    printf("\nCinco rolagens de 3d6: ");
    for (int i = 0; i < 5; i++) { printf("%d ", rola_varios(3, 6)); }

    printf("\n\nUm terremoto atinge a colônia. Os engenheiros (+4) tentam\n");
    printf("salvar a usina (dificuldade 13):\n");
    int sucessos = 0;
    for (int i = 0; i < 6; i++) { sucessos += teste(4, 13); }
    printf("Em 6 tentativas, %d sucesso%s.\n", sucessos,
           sucessos == 1 ? "" : "s");
    return 0;
}
