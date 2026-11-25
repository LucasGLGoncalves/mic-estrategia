/* main.c - Batalha Naval: o laço do jogo */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../motor/tela.h"
#include "../motor/term.h"
#include "jogo.h"

int main(int argc, char **argv) {
    uint64_t semente = (uint64_t)time(NULL);
    if (argc > 2 && strcmp(argv[1], "--semente") == 0)
        semente = strtoull(argv[2], NULL, 10);
    if (!term_inicia() || !tela_inicia(100, 30)) return 1;

    static Jogo jogo; /* static: fora da pilha, e começa zerado */
    jogo_novo(&jogo, semente);
    for (;;) {
        double agora = term_agora();
        int t;
        while ((t = term_tecla()) != 0) {
            if (t == TECLA_ESC || t == TECLA_SAIR) goto fim;
            if (jogo.fase == FIM && (t == 'n' || t == 'N'))
                jogo_novo(&jogo, (uint64_t)time(NULL));
            else jogo_tecla(&jogo, t, agora);
        }
        jogo_atualiza(&jogo, agora);
        jogo_desenha(&jogo, agora);
        tela_mostra();
        term_dorme(30);
    }
fim:
    tela_encerra();
    term_encerra();
    printf("Fim de jogo. Bons ventos, almirante!\n");
    return 0;
}
