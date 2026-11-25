/* velha.c - o jogo da velha para dois jogadores */
#include <stdio.h>
#include <stdlib.h>

int le_inteiro(const char *pergunta, int min, int max) {
    int valor;
    for (;;) {
        printf("%s", pergunta);
        int lidos = scanf("%d", &valor);
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        if (lidos == EOF) { exit(0); }
        if (lidos == 1 && valor >= min && valor <= max) { return valor; }
        printf("Digite um número de %d a %d.\n", min, max);
    }
}

/* As casas são numeradas de 1 a 9, como no teclado de um telefone:
   1 2 3
   4 5 6
   7 8 9  */
void desenha(char tab[3][3]) {
    printf("\n");
    for (int y = 0; y < 3; y++) {
        printf("   ");
        for (int x = 0; x < 3; x++) {
            char c = tab[y][x];
            if (c == ' ') {
                c = (char)('1' + y * 3 + x); /* casa vazia: mostra o número */
            }
            printf(" %c ", c);
            if (x < 2) printf("|");
        }
        printf("\n");
        if (y < 2) printf("   ---+---+---\n");
    }
    printf("\n");
}

/* Devolve 'X' ou 'O' se alguém completou uma linha, ou ' ' se ninguém. */
char vencedor(char tab[3][3]) {
    for (int i = 0; i < 3; i++) {
        if (tab[i][0] != ' ' && tab[i][0] == tab[i][1] &&
            tab[i][1] == tab[i][2])
            return tab[i][0]; /* linha */
        if (tab[0][i] != ' ' && tab[0][i] == tab[1][i] &&
            tab[1][i] == tab[2][i])
            return tab[0][i]; /* coluna */
    }
    if (tab[1][1] != ' ' && tab[0][0] == tab[1][1] && tab[1][1] == tab[2][2])
        return tab[1][1]; /* diagonal \ */
    if (tab[1][1] != ' ' && tab[0][2] == tab[1][1] && tab[1][1] == tab[2][0])
        return tab[1][1]; /* diagonal / */
    return ' ';
}

int main(void) {
    char tab[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};
    char vez = 'X';

    printf("=== JOGO DA VELHA ===\n");
    for (int jogada = 0; jogada < 9; jogada++) {
        desenha(tab);
        int casa;
        for (;;) {
            printf("Vez do %c. ", vez);
            casa = le_inteiro("Escolha uma casa (1 a 9): ", 1, 9) - 1;
            if (tab[casa / 3][casa % 3] == ' ') break;
            printf("Essa casa já está ocupada.\n");
        }
        tab[casa / 3][casa % 3] = vez;
        if (vencedor(tab) != ' ') {
            desenha(tab);
            printf("O %c venceu!\n", vez);
            return 0;
        }
        vez = (vez == 'X') ? 'O' : 'X';
    }
    desenha(tab);
    printf("Deu velha: ninguém venceu.\n");
    return 0;
}
