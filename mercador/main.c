/* main.c - Mercador das Estrelas: o laço do jogo e as ordens do capitão */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../motor/tela.h"
#include "../motor/term.h"
#include "salvar.h"
#include "telas.h"
#include "viagem.h"

static Jogo jogo;

static void tecla_mercado(Interface *ui, int t) {
    int b = ui->bens.sel, q;
    if (t == 'c' || t == 'C') {
        if (ui_le_numero("Comprar", BENS[b].nome, 1, 999, &q)) {
            int feitas = compra(&jogo, b, q);
            diario_poe(&jogo.diario, "Comprou %d de %s.", feitas, BENS[b].nome);
        }
    } else if ((t == 'v' || t == 'V') && jogo.carga[b] > 0) {
        if (ui_le_numero("Vender", BENS[b].nome, 1, jogo.carga[b], &q)) {
            int feitas = vende(&jogo, b, q);
            diario_poe(&jogo.diario, "Vendeu %d de %s.", feitas, BENS[b].nome);
        }
    } else if ((t == 't' || t == 'T') && jogo.carga[b] > 0) {
        int feitas = vende(&jogo, b, jogo.carga[b]);
        diario_poe(&jogo.diario, "Vendeu todas as %d unidades de %s.", feitas,
                   BENS[b].nome);
    } else {
        lista_tecla(&ui->bens, t, N_BENS, N_BENS);
    }
}

static void tecla_mapa(Interface *ui, int t) {
    if (t == TECLA_ENTER) {
        int dest = destino_da_linha(&jogo, ui->destinos.sel);
        if (combustivel_da_viagem(&jogo, dest) > jogo.combustivel) {
            ui_aviso("Sem combustível", "Não há combustível para essa viagem. "
                                        "Abasteça na tela da nave.");
            return;
        }
        char pergunta[160];
        snprintf(pergunta, sizeof pergunta,
                 "Partir para %s? A viagem leva %d dia(s).",
                 jogo.planetas[dest].nome, dias_de_viagem(&jogo, dest));
        if (ui_confirma("Viagem", pergunta)) {
            viaja(&jogo, dest, desenha_viagem);
            ui->aba = ABA_MERCADO;
            ui->destinos.sel = 0;
        }
    } else {
        lista_tecla(&ui->destinos, t, N_PLANETAS - 1, N_PLANETAS - 1);
    }
}

static void tecla_nave(Interface *ui, int t) {
    if (t != TECLA_ENTER) {
        lista_tecla(&ui->melhorias, t, N_MELHORIAS, N_MELHORIAS);
        return;
    }
    Melhoria m = (Melhoria)ui->melhorias.sel;
    double custo = custo_melhoria(&jogo, m);
    if (!jogo.planetas[jogo.aqui].estaleiro && m != M_COMBUSTIVEL) return;
    if (custo <= 0 || custo > jogo.creditos) {
        ui_aviso("Estaleiro", custo <= 0 ? "Não há nada a fazer."
                                         : "Créditos insuficientes.");
        return;
    }
    jogo.creditos -= custo;
    switch (m) {
    case M_CARGA: jogo.carga_max += 10; break;
    case M_CANHAO: jogo.canhoes++; break;
    case M_ESCUDO: jogo.escudo++; break;
    case M_REPARO: jogo.casco = 100; break;
    case M_COMBUSTIVEL: jogo.combustivel = jogo.combustivel_max; break;
    default: break;
    }
    diario_poe(&jogo.diario, "Estaleiro: %s.", nome_melhoria(m));
}

static void tecla_banco(int t) {
    if (jogo.aqui != 6) return;
    int q;
    if ((t == 'p' || t == 'P') && jogo.divida > 0) {
        int max = (int)(jogo.creditos < jogo.divida ? jogo.creditos
                                                    : jogo.divida + 1);
        if (max > 0 && ui_le_numero("Pagar", "Quanto pagar", 1, max, &q)) {
            jogo.creditos -= q;
            jogo.divida -= q;
            if (jogo.divida < 1) jogo.divida = 0;
            diario_poe(&jogo.diario, "Você pagou %d ao banco.", q);
        }
    } else if (t == 'e' || t == 'E') {
        int max = (int)(20000 - jogo.divida);
        if (max > 0 && ui_le_numero("Empréstimo", "Quanto pedir", 1, max, &q)) {
            jogo.creditos += q;
            jogo.divida += q;
            diario_poe(&jogo.diario, "O banco emprestou %d. Os juros correm.",
                       q);
        }
    }
}

static void tela_final(void) {
    double p = patrimonio(&jogo);
    char v[24], texto[400];
    fmt_num(v, sizeof v, p);
    const char *veredito =
        jogo.casco <= 0
            ? "A nave foi destruída. Os credores ficaram com os destroços."
        : p >= 50000
            ? "Uma fortuna! O seu nome vira lenda nos portos da galáxia."
        : p >= 15000
            ? "Um comerciante respeitado, com uma nave paga e algum dinheiro."
        : p >= 0 ? "Sobreviveu e pagou as contas. Já é alguma coisa."
                 : "As dívidas venceram. O banco tomou a nave.";
    snprintf(texto, sizeof texto,
             "Fim da jornada, no dia %d.\n\nPatrimônio final: \001y%s\001d "
             "créditos.\n\n%s",
             jogo.dia > DIAS_DE_JOGO ? DIAS_DE_JOGO : jogo.dia, v, veredito);
    ui_aviso("Fim da jornada", texto);
}

int main(int argc, char **argv) {
    uint64_t semente = (uint64_t)time(NULL);
    if (argc > 2 && strcmp(argv[1], "--semente") == 0)
        semente = strtoull(argv[2], NULL, 10);
    if (!term_inicia() || !tela_inicia(100, 30)) return 1;
    mundo_novo(&jogo, semente);
    Interface ui = {ABA_MERCADO, {0, 0}, {0, 0}, {0, 0}};
    tela_limpa(FUNDO);
    if (carrega_jogo(&jogo, ARQUIVO_SAVE) &&
        !ui_confirma("Mercador das Estrelas", "Continuar a viagem salva?"))
        mundo_novo(&jogo, semente);

    while (!jogo.terminou) {
        desenha(&jogo, &ui, term_agora());
        tela_mostra();
        int t = term_espera_tecla(120);
        if (t == 0) continue;
        if (t == TECLA_ESC || t == TECLA_SAIR) {
            if (ui_confirma("Sair", "Salvar e sair?")) {
                salva_jogo(&jogo, ARQUIVO_SAVE);
                break;
            }
            continue;
        }
        if (t >= '1' && t <= '5') ui.aba = (Aba)(t - '1');
        else if (t == 's' || t == 'S') {
            diario_poe(&jogo.diario, salva_jogo(&jogo, ARQUIVO_SAVE)
                                         ? "Jogo salvo."
                                         : "\001rFalha ao salvar.\001d");
        } else if (ui.aba == ABA_MERCADO) tecla_mercado(&ui, t);
        else if (ui.aba == ABA_MAPA) tecla_mapa(&ui, t);
        else if (ui.aba == ABA_NAVE) tecla_nave(&ui, t);
        else if (ui.aba == ABA_BANCO) tecla_banco(t);
    }
    if (jogo.terminou) {
        desenha(&jogo, &ui, term_agora());
        tela_final();
        remove(ARQUIVO_SAVE);
    }
    tela_encerra();
    term_encerra();
    return 0;
}
