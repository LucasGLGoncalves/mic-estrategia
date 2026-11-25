/* jogo.c - as regras e o andamento de uma partida */
#include "jogo.h"

#include <stdio.h>

#include "../motor/term.h"
#include "ia.h"

void nome_casa(int x, int y, char *saida) {
    snprintf(saida, 16, "%c%d", (char)('A' + x), y + 1);
}

void jogo_novo(Jogo *j, uint64_t semente) {
    *j = (Jogo){0};
    rng_semente(&j->rng, semente);
    tab_inicia(&j->meu);
    tab_inicia(&j->dele);
    tab_posiciona_tudo(&j->dele, &j->rng);
    j->fase = POSICIONANDO;
    j->ultimo_x = j->ultimo_y = -1;
    diario_poe(&j->diario,
               "Almirante, posicione a sua frota. \001yR\001d gira o navio, "
               "\001yA\001d posiciona o resto ao acaso.");
}

static void move_cursor(Jogo *j, int tecla) {
    if (tecla == TECLA_CIMA && j->cy > 0) j->cy--;
    if (tecla == TECLA_BAIXO && j->cy < TAM - 1) j->cy++;
    if (tecla == TECLA_ESQUERDA && j->cx > 0) j->cx--;
    if (tecla == TECLA_DIREITA && j->cx < TAM - 1) j->cx++;
}

static void comeca_batalha(Jogo *j) {
    j->fase = JOGANDO;
    j->cx = j->cy = 0;
    diario_poe(&j->diario,
               "Frota pronta. \001gVocê atira primeiro.\001d Escolha uma casa "
               "no mar inimigo e aperte Enter.");
}

static void tiro_do_jogador(Jogo *j) {
    int n;
    char casa[16];
    nome_casa(j->cx, j->cy, casa);
    ResultadoTiro r = tab_atira(&j->dele, j->cx, j->cy, &n);
    if (r == TIRO_INVALIDO) return;
    j->tiros_jogador++;
    if (r == TIRO_AGUA)
        diario_poe(&j->diario, "Você atirou em %s: \001bágua\001d.", casa);
    if (r == TIRO_ACERTO)
        diario_poe(&j->diario, "Você atirou em %s: \001gacertou!\001d", casa);
    if (r == TIRO_AFUNDOU)
        diario_poe(&j->diario,
                   "Você atirou em %s: \001gafundou o %s inimigo!\001d", casa,
                   j->dele.navios[n].nome);
    if (tab_restantes(&j->dele) == 0) {
        j->fase = FIM;
        j->venceu = 1;
        diario_poe(&j->diario,
                   "\001gVITÓRIA!\001d A frota inimiga está no fundo do mar.");
        return;
    }
    j->vez_do_computador = 1;
    j->hora_do_tiro = term_agora() + 0.7;
}

static void tiro_do_computador(Jogo *j) {
    int x = 0, y = 0, n;
    char casa[16];
    ia_escolhe_tiro(&j->meu, &j->rng, &x, &y);
    nome_casa(x, y, casa);
    ResultadoTiro r = tab_atira(&j->meu, x, y, &n);
    j->tiros_computador++;
    j->ultimo_x = x;
    j->ultimo_y = y;
    if (r == TIRO_AGUA)
        diario_poe(&j->diario, "O inimigo atirou em %s: água.", casa);
    if (r == TIRO_ACERTO)
        diario_poe(&j->diario,
                   "O inimigo atirou em %s: \001racertou o seu %s!\001d", casa,
                   j->meu.navios[n].nome);
    if (r == TIRO_AFUNDOU)
        diario_poe(&j->diario,
                   "O inimigo atirou em %s: \001rafundou o seu %s!\001d", casa,
                   j->meu.navios[n].nome);
    if (tab_restantes(&j->meu) == 0) {
        j->fase = FIM;
        diario_poe(&j->diario, "\001rDERROTA.\001d A sua frota foi afundada.");
    }
    j->vez_do_computador = 0;
}

void jogo_tecla(Jogo *j, int tecla, double agora) {
    (void)agora;
    if (tecla == 'h' || tecla == 'H') j->mostra_calor = !j->mostra_calor;
    if (j->fase == POSICIONANDO) {
        move_cursor(j, tecla);
        if (tecla == 'r' || tecla == 'R') j->vertical = !j->vertical;
        if (tecla == 'a' || tecla == 'A') {
            tab_posiciona_tudo(&j->meu, &j->rng);
            comeca_batalha(j);
        } else if (tecla == TECLA_ENTER || tecla == ' ') {
            if (tab_posiciona(&j->meu, j->navio, j->cx, j->cy, j->vertical)) {
                if (++j->navio == N_NAVIOS) comeca_batalha(j);
            } else {
                diario_poe(
                    &j->diario,
                    "\001rNão cabe aí.\001d Os navios não podem se encostar.");
            }
        }
    } else if (j->fase == JOGANDO && !j->vez_do_computador) {
        move_cursor(j, tecla);
        if (tecla == TECLA_ENTER || tecla == ' ') tiro_do_jogador(j);
    }
}

void jogo_atualiza(Jogo *j, double agora) {
    if (j->fase == JOGANDO && j->vez_do_computador &&
        agora >= j->hora_do_tiro) {
        tiro_do_computador(j);
    }
}
