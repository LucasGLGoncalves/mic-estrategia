/* ia.c - o almirante do computador */
#include "ia.h"

/* Um acerto que ainda não pertence a um navio afundado é uma pista quente. */
static int acerto_vivo(const Tabuleiro *t, int x, int y) {
    return t->tiros[y][x] == ACERTO && !tab_afundado(t, t->ocupado[y][x]);
}

void ia_calcula_mapa(const Tabuleiro *alvo, int mapa[TAM][TAM]) {
    for (int y = 0; y < TAM; y++)
        for (int x = 0; x < TAM; x++) mapa[y][x] = 0;

    for (int n = 0; n < N_NAVIOS; n++) {
        /* navios afundados já não estão em lugar nenhum */
        if (tab_afundado(alvo, n)) continue;
        int tam = alvo->navios[n].tamanho;
        for (int v = 0; v < 2; v++)
            for (int y = 0; y < TAM; y++)
                for (int x = 0; x < TAM; x++) {
                    /* o navio caberia aqui? só se não passar por água conhecida
                       nem por pedaços de navios já afundados */
                    int x2 = v ? x : x + tam - 1, y2 = v ? y + tam - 1 : y;
                    if (x2 >= TAM || y2 >= TAM) continue;
                    int possivel = 1, pistas = 0;
                    for (int k = 0; k < tam && possivel; k++) {
                        int cx = v ? x : x + k, cy = v ? y + k : y;
                        Marca m = alvo->tiros[cy][cx];
                        if (m == ERRO) possivel = 0;
                        else if (m == ACERTO) {
                            if (acerto_vivo(alvo, cx, cy)) pistas++;
                            else possivel = 0;
                        }
                    }
                    if (!possivel) continue;
                    /* posições que explicam acertos já feitos valem muito
                       mais */
                    int peso = 1 + pistas * 20;
                    for (int k = 0; k < tam; k++) {
                        int cx = v ? x : x + k, cy = v ? y + k : y;
                        if (alvo->tiros[cy][cx] == AGUA) mapa[cy][cx] += peso;
                    }
                }
    }
}

void ia_escolhe_tiro(const Tabuleiro *alvo, Rng *r, int *x, int *y) {
    int mapa[TAM][TAM];
    ia_calcula_mapa(alvo, mapa);
    int melhor = -1, empates = 0;
    for (int j = 0; j < TAM; j++)
        for (int i = 0; i < TAM; i++) {
            if (alvo->tiros[j][i] != AGUA) continue;
            if (mapa[j][i] > melhor) {
                melhor = mapa[j][i];
                empates = 1;
                *x = i;
                *y = j;
            } else if (mapa[j][i] == melhor &&
                       rng_faixa(r, 1, ++empates) == 1) {
                *x = i; /* sorteio justo entre as casas empatadas */
                *y = j;
            }
        }
}
