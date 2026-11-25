/* main.c - Colônia: um jogo de construção em tempo real, que continua com o
   jogo fechado */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../motor/tela.h"
#include "../motor/term.h"
#include "piloto.h"
#include "salvar.h"
#include "telas.h"

static Colonia colonia;

/* Faz a colônia trabalhar pelo tempo em que o jogo ficou fechado. Sem o
   administrador por perto, ela trabalha a meio ritmo. Escreve o relatório em
   'texto' e devolve 1 se houve progresso para contar. */
static int progresso_offline(time_t salvo_em, char *texto, size_t tam) {
    double fora = difftime(time(NULL), salvo_em);
    if (fora < 10) return 0;
    double conta = (fora > MAX_OFFLINE ? MAX_OFFLINE : fora) / 2;
    double antes[N_RECURSOS];
    memcpy(antes, colonia.recurso, sizeof antes);
    int obras = colonia.n_fila;
    colonia_avanca(&colonia, conta);
    obras -= colonia.n_fila;
    char t[24];
    fmt_tempo(t, sizeof t, fora);
    int n = snprintf(texto, tam,
                     "Você esteve fora por %s. Sem você, a colônia trabalhou a "
                     "meio ritmo:\n\n",
                     t);
    for (int r = 0; r < N_RECURSOS; r++) {
        char v[24], nome[24];
        fmt_num(v, sizeof v, colonia.recurso[r] - antes[r]);
        fmt_ajusta(nome, sizeof nome, NOME_RECURSO[r], 9);
        n += snprintf(texto + n, tam - (size_t)n, "%s +%s\n", nome, v);
    }
    if (obras > 0)
        n += snprintf(texto + n, tam - (size_t)n, "\n%d obra(s) concluída(s).",
                      obras);
    if (fora > MAX_OFFLINE)
        snprintf(texto + n, tam - (size_t)n,
                 "\n\n\001y(O tempo fora vale até 8 horas.)\001d");
    diario_poe(&colonia.diario, "De volta depois de %s.", t);
    return 1;
}

static void tecla_lista(Interface *ui, int t) {
    if (ui->aba == ABA_OBRAS) {
        if (t == TECLA_ENTER) {
            const char *motivo;
            Estrutura e = (Estrutura)ui->obras.sel;
            if (encomenda(&colonia, e))
                diario_poe(&colonia.diario, "Encomendada: %s, nível %d.",
                           ESTRUTURAS[e].nome, nivel_planejado(&colonia, e));
            else if (!pode_construir(&colonia, e, &motivo))
                diario_poe(&colonia.diario, "\001rAgora não\001d: %s.", motivo);
        } else {
            lista_tecla(&ui->obras, t, N_ESTRUTURAS, N_ESTRUTURAS);
        }
    } else if (ui->aba == ABA_PESQUISAS) {
        if (t == TECLA_ENTER) {
            Pesquisa p = (Pesquisa)ui->pesquisas.sel;
            if (!pesquisa(&colonia, p) && pode_pesquisar(&colonia, p))
                diario_poe(&colonia.diario,
                           "\001rCiência insuficiente\001d para %s.",
                           PESQUISAS[p].nome);
        } else {
            lista_tecla(&ui->pesquisas, t, N_PESQUISAS, N_PESQUISAS);
        }
    }
}

int main(int argc, char **argv) {
    Interface ui = {ABA_OBRAS, {0, 0}, {0, 0}, 1};
    int novo = 0;
    double automatico = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--novo") == 0) novo = 1;
        else if (strcmp(argv[i], "--auto") == 0 && i + 1 < argc)
            automatico = atof(argv[++i]) * 60;
        else if (strcmp(argv[i], "--velocidade") == 0 && i + 1 < argc)
            ui.velocidade = atoi(argv[++i]);
    }
    if (ui.velocidade < 1) ui.velocidade = 1;
    if (!term_inicia() || !tela_inicia(100, 30)) return 1;
    time_t salvo_em;
    tela_limpa(FUNDO);
    if (!novo && carrega_colonia(&colonia, ARQUIVO_SAVE, &salvo_em)) {
        diario_poe(&colonia.diario, "Colônia recuperada.");
        char relatorio[600];
        if (progresso_offline(salvo_em, relatorio, sizeof relatorio)) {
            /* a colônia já crescida, atrás do aviso */
            desenha(&colonia, &ui, term_agora());
            ui_aviso("Bem-vindo de volta", relatorio);
        }
    } else {
        colonia_nova(&colonia);
    }
    /* o administrador automático joga os primeiros minutos */
    if (automatico > 0) {
        piloto_joga(&colonia, automatico);
        diario_poe(&colonia.diario,
                   "O piloto automático passou o comando para você.");
    }
    int avisou_vitoria = colonia.venceu;
    double antes = term_agora(), ultimo_save = antes;
    for (;;) {
        double agora = term_agora();
        /* o tempo real move a colônia */
        colonia_avanca(&colonia, (agora - antes) * ui.velocidade);
        antes = agora;
        if (colonia.venceu && !avisou_vitoria) {
            avisou_vitoria = 1;
            char t[24], texto[300];
            fmt_tempo(t, sizeof t, colonia.relogio);
            snprintf(texto, sizeof texto,
                     "Depois de %s de trabalho, o Portal Estelar se abre e a "
                     "primeira mensagem chega de casa.\n\n\001gA colônia "
                     "venceu.\001d Ela continua "
                     "aqui, crescendo, enquanto você quiser.",
                     t);
            desenha(&colonia, &ui, agora);
            ui_aviso("O Portal Estelar", texto);
        }
        if (agora - ultimo_save > 30) { /* salva sozinho a cada meio minuto */
            salva_colonia(&colonia, ARQUIVO_SAVE);
            ultimo_save = agora;
        }
        desenha(&colonia, &ui, agora);
        tela_mostra();
        int t = term_espera_tecla(80);
        if (t == 0) continue;
        if (t == TECLA_ESC || t == TECLA_SAIR) {
            if (ui_confirma("Sair", "Salvar e sair? A colônia continua "
                                    "trabalhando enquanto você estiver fora."))
                break;
        } else if (t >= '1' && t <= '3') {
            ui.aba = (Aba)(t - '1');
        } else if (t == TECLA_TAB) {
            ui.aba = (Aba)((ui.aba + 1) % N_ABAS);
        } else if (t == 'x' || t == 'X') {
            if (colonia.n_fila > 0 &&
                ui_confirma(
                    "Cancelar",
                    "Cancelar a última obra da fila? Os recursos voltam."))
                cancela_ultima(&colonia);
        } else if (t == 'v' || t == 'V') {
            ui.velocidade = ui.velocidade == 1    ? 10
                            : ui.velocidade == 10 ? 100
                                                  : 1;
        } else {
            tecla_lista(&ui, t);
        }
    }
    salva_colonia(&colonia, ARQUIVO_SAVE);
    tela_encerra();
    term_encerra();
    return 0;
}
