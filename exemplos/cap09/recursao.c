/* recursao.c - funções que chamam a si mesmas */
#include <stdio.h>

void contagem(int n) {
    if (n == 0) {
        printf("Ignição!\n");
        return; /* caso base: para de chamar */
    }
    printf("%d... ", n);
    contagem(n - 1); /* caso recursivo: um problema menor */
}

long long fatorial(int n) {
    if (n <= 1) { return 1; }
    return n * fatorial(n - 1);
}

/* Quantas partidas diferentes de jogo da velha existem? Joga todas e conta. */
char tab[3][3];

int alguem_venceu(void) {
    for (int i = 0; i < 3; i++) {
        if (tab[i][0] != ' ' && tab[i][0] == tab[i][1] &&
            tab[i][1] == tab[i][2])
            return 1;
        if (tab[0][i] != ' ' && tab[0][i] == tab[1][i] &&
            tab[1][i] == tab[2][i])
            return 1;
    }
    if (tab[1][1] != ' ' && tab[0][0] == tab[1][1] && tab[1][1] == tab[2][2])
        return 1;
    if (tab[1][1] != ' ' && tab[0][2] == tab[1][1] && tab[1][1] == tab[2][0])
        return 1;
    return 0;
}

long long conta_partidas(char vez, int jogadas) {
    if (alguem_venceu() || jogadas == 9) {
        return 1; /* a partida acabou: conta uma */
    }
    long long total = 0;
    for (int casa = 0; casa < 9; casa++) {
        if (tab[casa / 3][casa % 3] == ' ') {
            tab[casa / 3][casa % 3] = vez; /* faz a jogada */
            total += conta_partidas(vez == 'X' ? 'O' : 'X', jogadas + 1);
            tab[casa / 3][casa % 3] = ' '; /* desfaz a jogada */
        }
    }
    return total;
}

int main(void) {
    contagem(5);
    for (int n = 1; n <= 20; n += 4) {
        printf("%2d! = %lld\n", n, fatorial(n));
    }
    for (int i = 0; i < 9; i++) { tab[i / 3][i % 3] = ' '; }
    printf("Partidas possíveis de jogo da velha: %lld\n",
           conta_partidas('X', 0));
    return 0;
}
