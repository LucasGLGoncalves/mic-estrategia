/* simula.c - o general do computador contra ele mesmo, sem tela:
   serve para testar a inteligência e medir o equilíbrio do jogo. */
#include <stdio.h>
#include <stdlib.h>

#include "ia.h"

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 100;
    int vitorias[N_DONOS] = {0}, empates = 0, soma_turnos = 0;
    static Jogo j;
    for (int s = 1; s <= n; s++) {
        jogo_novo(&j, (uint64_t)s);
        while (j.vencedor == NEUTRO && j.turno <= 300) {
            ia_joga(&j, JOGADOR, NULL);
            if (j.vencedor != NEUTRO) break;
            turno_comeca(&j, COMPUTADOR);
            ia_joga(&j, COMPUTADOR, NULL);
            j.turno++;
            turno_comeca(&j, JOGADOR);
        }
        if (j.vencedor == NEUTRO) empates++;
        else {
            vitorias[j.vencedor]++;
            soma_turnos += j.turno;
        }
    }
    int decididas = n - empates;
    printf("%d partidas: oeste venceu %d, leste venceu %d, sem decisão em 300 "
           "turnos: %d\n",
           n, vitorias[JOGADOR], vitorias[COMPUTADOR], empates);
    if (decididas > 0)
        printf("duração média: %d turnos\n", soma_turnos / decididas);
    return 0;
}
