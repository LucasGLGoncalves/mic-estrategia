/* comando.c - abas, listas, diário e janelas: a interface de um jogo de
   estratégia */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../../motor/tela.h"
#include "../../motor/term.h"
#include "../../motor/ui.h"

typedef struct {
    const char *nome;
    double custo_base, fator, segundos;
    int nivel;
} Construcao;

static Construcao obras[] = {
    {"Mina de metal", 60, 1.5, 40, 3},
    {"Mina de cristal", 48, 1.6, 55, 2},
    {"Sintetizador", 225, 1.5, 90, 0},
    {"Usina solar", 75, 1.5, 35, 4},
    {"Fazenda", 90, 1.4, 30, 2},
    {"Armazém de metal", 1000, 2.0, 300, 1},
    {"Laboratório", 200, 2.0, 120, 0},
    {"Estaleiro", 400, 2.0, 180, 0},
    {"Fábrica de robôs", 400, 2.0, 240, 1},
    {"Silo de mísseis", 20000, 2.0, 900, 0},
};
#define N_OBRAS (int)(sizeof obras / sizeof obras[0])

static double custo(const Construcao *c) {
    return c->custo_base * pow(c->fator, c->nivel);
}

int main(void) {
    if (!term_inicia() || !tela_inicia(100, 25)) return 1;
    char colonia[40] = "Kepler";
    double metal = 1234567.0;
    Lista lista = {0, 0};
    Diario diario = {0};
    int aba = 0;
    const char *abas[] = {"1 Construções", "2 Diário", "3 Números"};
    diario_poe(&diario,
               "Bem-vindo, comandante. A colônia \001y%s\001d aguarda ordens.",
               colonia);

    for (;;) {
        tela_limpa(FUNDO);
        tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
        char m[32];
        fmt_num(m, sizeof m, metal);
        tela_textof(1, 0, AMARELO, PAINEL, "✦ %s", colonia);
        tela_textof(60, 0, CINZA, PAINEL, "Metal %s", m);
        ui_abas(1, 2, abas, 3, aba);

        if (aba == 0) {
            const char *itens[N_OBRAS];
            char textos[N_OBRAS][100];
            for (int i = 0; i < N_OBRAS; i++) {
                char nome[48], c[16], t[16];
                /* conta letras, não bytes */
                fmt_ajusta(nome, sizeof nome, obras[i].nome, 18);
                fmt_curto(c, sizeof c, custo(&obras[i]));
                fmt_tempo(t, sizeof t,
                          obras[i].segundos * pow(1.3, obras[i].nivel));
                snprintf(textos[i], sizeof textos[i],
                         "%s nível %2d   %8s   %8s", nome, obras[i].nivel, c,
                         t);
                itens[i] = textos[i];
            }
            tela_texto(3, 4,
                       "Construção           Nível       Custo      Tempo",
                       CINZA, FUNDO);
            lista_desenha(1, 5, 60, 8, itens, N_OBRAS, &lista);
            tela_caixa(63, 4, 36, 9, CINZA_ESCURO, FUNDO, "Diário");
            diario_desenha(&diario, 65, 5, 32, 7);
            tela_texto(1, 14,
                       "↑↓ escolhe   Enter constrói   N renomeia   Tab troca "
                       "de aba   Esc sai",
                       CINZA, FUNDO);
        } else if (aba == 1) {
            diario_desenha(&diario, 2, 4, 96, 18);
        } else {
            double exemplos[] = {7, 1234, 98765, 4560000, 7.3e9};
            for (int i = 0; i < 5; i++) {
                char a[32], b[32], c[32];
                fmt_num(a, sizeof a, exemplos[i]);
                fmt_curto(b, sizeof b, exemplos[i]);
                fmt_tempo(c, sizeof c, exemplos[i]);
                tela_textof(4, 5 + i, BRANCO, FUNDO, "%16s  %10s  %14s", a, b,
                            c);
            }
            tela_texto(4, 4, "        fmt_num   fmt_curto  fmt_tempo (s)",
                       CINZA, FUNDO);
        }
        tela_mostra();

        int t = term_espera_tecla(250);
        if (t == TECLA_ESC || t == TECLA_SAIR) {
            if (ui_confirma("Sair", "Abandonar o comando da colônia?")) break;
        } else if (t == TECLA_TAB) {
            aba = (aba + 1) % 3;
        } else if (t >= '1' && t <= '3') {
            aba = t - '1';
        } else if (t == 'n' || t == 'N') {
            char novo[40];
            snprintf(novo, sizeof novo, "%s", colonia);
            if (ui_le_texto("Renomear", "Novo nome da colônia:", novo,
                            sizeof novo) &&
                novo[0]) {
                diario_poe(&diario, "A colônia agora se chama \001y%s\001d.",
                           novo);
                snprintf(colonia, sizeof colonia, "%s", novo);
            }
        } else if (aba == 0 && t == TECLA_ENTER) {
            Construcao *c = &obras[lista.sel];
            char pergunta[160], v[24];
            fmt_num(v, sizeof v, custo(c));
            snprintf(pergunta, sizeof pergunta,
                     "Construir %s nível %d por %s de metal?", c->nome,
                     c->nivel + 1, v);
            if (custo(c) > metal) {
                ui_aviso("Metal insuficiente",
                         "Faltam recursos para essa obra.");
            } else if (ui_confirma("Construir", pergunta)) {
                metal -= custo(c);
                c->nivel++;
                diario_poe(&diario, "\001g%s\001d chegou ao nível %d.", c->nome,
                           c->nivel);
            }
        } else if (aba == 0) {
            lista_tecla(&lista, t, N_OBRAS, 8);
        }
    }
    tela_encerra();
    term_encerra();
    return 0;
}
