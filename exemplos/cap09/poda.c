/* poda.c - quantas posições o minimax visita, com e sem a poda alfa-beta */
#include <stdio.h>

char tab[3][3];
long long visitas;

char vencedor(void) {
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

int acabou(void) {
    if (vencedor() != ' ') return 1;
    for (int c = 0; c < 9; c++)
        if (tab[c / 3][c % 3] == ' ') return 0;
    return 1;
}

int nota_final(int prof) {
    char v = vencedor();
    if (v == 'O') return 10 - prof;
    if (v == 'X') return prof - 10;
    return 0;
}

int minimax(char vez, int prof) {
    visitas++;
    if (acabou()) return nota_final(prof);
    int melhor = vez == 'O' ? -1000 : 1000;
    for (int c = 0; c < 9; c++) {
        if (tab[c / 3][c % 3] != ' ') continue;
        tab[c / 3][c % 3] = vez;
        int nota = minimax(vez == 'O' ? 'X' : 'O', prof + 1);
        tab[c / 3][c % 3] = ' ';
        if (vez == 'O' ? nota > melhor : nota < melhor) melhor = nota;
    }
    return melhor;
}

/* alfa: o melhor que o computador já garantiu; beta: o melhor que o humano
   já garantiu */
int alfabeta(char vez, int prof, int alfa, int beta) {
    visitas++;
    if (acabou()) return nota_final(prof);
    for (int c = 0; c < 9; c++) {
        if (tab[c / 3][c % 3] != ' ') continue;
        tab[c / 3][c % 3] = vez;
        int nota = alfabeta(vez == 'O' ? 'X' : 'O', prof + 1, alfa, beta);
        tab[c / 3][c % 3] = ' ';
        if (vez == 'O' && nota > alfa) alfa = nota;
        if (vez == 'X' && nota < beta) beta = nota;
        /* o adversário nunca deixaria chegar aqui: poda */
        if (alfa >= beta) break;
    }
    return vez == 'O' ? alfa : beta;
}

int main(void) {
    for (int c = 0; c < 9; c++) tab[c / 3][c % 3] = ' ';
    visitas = 0;
    int a = minimax('O', 0);
    printf("Minimax:   nota %d, %lld posições visitadas\n", a, visitas);
    visitas = 0;
    int b = alfabeta('O', 0, -1000, 1000);
    printf("Alfa-beta: nota %d, %lld posições visitadas\n", b, visitas);
    return 0;
}
