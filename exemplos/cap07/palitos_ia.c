/* palitos_ia.c - o jogo dos palitos contra o computador */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Lê um número inteiro entre min e max, repetindo a pergunta até dar certo. */
int le_inteiro(const char *pergunta, int min, int max) {
    int valor;
    for (;;) {
        printf("%s", pergunta);
        int lidos = scanf("%d", &valor);
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            /* descarta o resto da linha, inclusive letras digitadas por
               engano */
        }
        if (lidos == EOF) {
            exit(0); /* a entrada acabou (Ctrl+D ou Ctrl+Z): encerra */
        }
        if (lidos == 1 && valor >= min && valor <= max) { return valor; }
        printf("Digite um número de %d a %d.\n", min, max);
    }
}

/* Devolve um número aleatório entre min e max, inclusive. */
int sorteia(int min, int max) { return min + rand() % (max - min + 1); }

int menor(int a, int b) { return a < b ? a : b; }

void desenha_palitos(int n) {
    printf("\n");
    for (int i = 1; i <= n; i++) { printf(i % 5 == 0 ? "|   " : "| "); }
    printf(" (%d)\n", n);
}

/* A jogada do computador. No modo difícil, ele deixa sempre 1, 5, 9, 13...
   palitos na mesa; quando não consegue, joga ao acaso e torce por um erro. */
int jogada_computador(int palitos, int dificil) {
    if (dificil) {
        int ideal = (palitos - 1) % 4;
        if (ideal != 0) { return ideal; }
    }
    return sorteia(1, menor(3, palitos));
}

int main(void) {
    srand((unsigned)time(NULL));

    printf("=== JOGO DOS PALITOS ===\n");
    printf("Tire 1, 2 ou 3 palitos por vez. Quem tirar o último perde.\n\n");
    int dificil =
        le_inteiro("Dificuldade (1 = fácil, 2 = difícil): ", 1, 2) == 2;
    int vez = le_inteiro("Quem começa (1 = você, 2 = o computador): ", 1, 2);
    int palitos = le_inteiro("Quantos palitos na mesa (10 a 40)? ", 10, 40);

    while (palitos > 0) {
        desenha_palitos(palitos);
        int tirar;
        if (vez == 1) {
            tirar = le_inteiro("Quantos você tira? ", 1, menor(3, palitos));
        } else {
            tirar = jogada_computador(palitos, dificil);
            printf("O computador tira %d.\n", tirar);
        }
        palitos -= tirar;
        if (palitos == 0) {
            if (vez == 1) {
                printf("\nVocê tirou o último palito. O computador venceu!\n");
            } else {
                printf("\nO computador tirou o último palito. Você venceu!\n");
            }
        }
        vez = 3 - vez;
    }
    return 0;
}
