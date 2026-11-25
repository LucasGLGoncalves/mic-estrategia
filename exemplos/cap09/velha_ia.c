/* velha_ia.c - o jogo da velha contra um computador que nunca perde
   (minimax) */
#include <stdio.h>
#include <stdlib.h>

/* quantas posições o computador analisou na última jogada */
long long posicoes;

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

void desenha(char tab[3][3]) {
    printf("\n");
    for (int y = 0; y < 3; y++) {
        printf("   ");
        for (int x = 0; x < 3; x++) {
            char c = tab[y][x] == ' ' ? (char)('1' + y * 3 + x) : tab[y][x];
            printf(" %c %s", c, x < 2 ? "|" : "");
        }
        printf("\n%s", y < 2 ? "   ---+---+---\n" : "\n");
    }
}

char vencedor(char tab[3][3]) {
    for (int i = 0; i < 3; i++) {
        if (tab[i][0] != ' ' && tab[i][0] == tab[i][1] &&
            tab[i][1] == tab[i][2])
            return tab[i][0];
        if (tab[0][i] != ' ' && tab[0][i] == tab[1][i] &&
            tab[1][i] == tab[2][i])
            return tab[0][i];
    }
    if (tab[1][1] != ' ' && tab[0][0] == tab[1][1] && tab[1][1] == tab[2][2])
        return tab[1][1];
    if (tab[1][1] != ' ' && tab[0][2] == tab[1][1] && tab[1][1] == tab[2][0])
        return tab[1][1];
    return ' ';
}

int cheio(char tab[3][3]) {
    for (int casa = 0; casa < 9; casa++) {
        if (tab[casa / 3][casa % 3] == ' ') return 0;
    }
    return 1;
}

/* Dá uma nota à posição, supondo que os dois lados jogam perfeitamente daqui em
   diante. Notas positivas são boas para o computador ('O'); negativas, para o
   humano ('X'). Vencer cedo vale mais que vencer tarde, e perder tarde é menos
   ruim que perder cedo. */
int minimax(char tab[3][3], char vez, int profundidade) {
    posicoes++;
    char v = vencedor(tab);
    if (v == 'O') return 10 - profundidade;
    if (v == 'X') return profundidade - 10;
    if (cheio(tab)) return 0;

    int melhor = (vez == 'O') ? -1000 : 1000;
    for (int casa = 0; casa < 9; casa++) {
        int y = casa / 3, x = casa % 3;
        if (tab[y][x] != ' ') continue;
        tab[y][x] = vez; /* experimenta a jogada... */
        int nota = minimax(tab, vez == 'O' ? 'X' : 'O', profundidade + 1);
        tab[y][x] = ' '; /* ...e desfaz */
        /* o computador maximiza */
        if (vez == 'O' && nota > melhor) melhor = nota;
        if (vez == 'X' && nota < melhor) melhor = nota; /* o humano minimiza */
    }
    return melhor;
}

int melhor_jogada(char tab[3][3]) {
    int melhor_nota = -1000, melhor_casa = -1;
    posicoes = 0;
    for (int casa = 0; casa < 9; casa++) {
        int y = casa / 3, x = casa % 3;
        if (tab[y][x] != ' ') continue;
        tab[y][x] = 'O';
        int nota = minimax(tab, 'X', 1);
        tab[y][x] = ' ';
        if (nota > melhor_nota) {
            melhor_nota = nota;
            melhor_casa = casa;
        }
    }
    return melhor_casa;
}

int main(void) {
    char tab[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};
    printf("=== JOGO DA VELHA CONTRA O COMPUTADOR ===\n");
    char vez =
        le_inteiro("Quem começa (1 = você, com X; 2 = o computador, com O)? ",
                   1, 2) == 1
            ? 'X'
            : 'O';

    while (vencedor(tab) == ' ' && !cheio(tab)) {
        int casa;
        if (vez == 'X') {
            desenha(tab);
            for (;;) {
                casa = le_inteiro("Sua jogada (1 a 9): ", 1, 9) - 1;
                if (tab[casa / 3][casa % 3] == ' ') break;
                printf("Essa casa já está ocupada.\n");
            }
        } else {
            casa = melhor_jogada(tab);
            printf(
                "\nO computador analisou %lld posições e jogou na casa %d.\n",
                posicoes, casa + 1);
        }
        tab[casa / 3][casa % 3] = vez;
        vez = (vez == 'X') ? 'O' : 'X';
    }
    desenha(tab);
    char v = vencedor(tab);
    if (v == 'O') printf("O computador venceu. Ele avisou que nunca perde.\n");
    else if (v == 'X') printf("Você venceu?! Isso não deveria acontecer.\n");
    else printf("Deu velha. Contra um jogador perfeito, empatar é vencer.\n");
    return 0;
}
