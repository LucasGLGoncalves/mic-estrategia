/* viagem.c - viagens, piratas, tempestades, destroços e a patrulha */
#include "viagem.h"

#include <stdio.h>

#include "../motor/tela.h"
#include "../motor/term.h"
#include "../motor/ui.h"

typedef enum { EV_PIRATAS, EV_TEMPESTADE, EV_DESTROCOS, N_EVENTOS } TipoEvento;

/* Um teste de d20, mostrado ao jogador; devolve 1 se passou. */
static int teste(Jogo *j, const char *o_que, int bonus, int dificuldade) {
    int dado = rng_dado(&j->rng, 20);
    int ok = dado == 20 || (dado != 1 && dado + bonus >= dificuldade);
    diario_poe(&j->diario, "%s: d20 = %d %+d contra %d: %s", o_que, dado, bonus,
               dificuldade, ok ? "\001gsucesso\001d" : "\001rfalha\001d");
    return ok;
}

static void perde_carga(Jogo *j, double fracao) {
    for (int b = 0; b < N_BENS; b++)
        j->carga[b] -= (int)(j->carga[b] * fracao + 0.5);
}

static void dano(Jogo *j, int min, int max) {
    int d = rng_faixa(&j->rng, min, max) - j->escudo * 5;
    if (d < 1) d = 1;
    j->casco -= d;
    diario_poe(&j->diario,
               "O casco sofreu \001r%d\001d de dano (agora em %d%%).", d,
               j->casco < 0 ? 0 : j->casco);
}

static void piratas(Jogo *j, int risco) {
    ui_aviso("Piratas!",
             "Uma corveta pirata sai de trás de um asteroide e manda você "
             "parar as máquinas e entregar a carga.");
    const char *opcoes[3] = {"Lutar (d20 + 2 por canhão + escudo)",
                             "Fugir (d20 + 1 a cada 10 lugares vazios)",
                             "Pagar o pedágio (15% dos créditos)"};
    int escolha = ui_escolha("O que fazer?", opcoes, 3);
    if (escolha < 0) escolha = 2;
    if (escolha == 0) {
        if (teste(j, "Combate", j->canhoes * 2 + j->escudo, 11 + risco * 2)) {
            int premio = rng_faixa(&j->rng, 200, 500) * (risco + 1);
            j->creditos += premio;
            diario_poe(&j->diario,
                       "Os piratas fogem em chamas. Recompensa: \001g%d\001d "
                       "créditos.",
                       premio);
        } else {
            dano(j, 15, 35);
            perde_carga(j, 0.3);
            diario_poe(&j->diario,
                       "Os piratas levaram \001r30%% da carga\001d.");
        }
    } else if (escolha == 1) {
        int vazio = j->carga_max - carga_usada(j);
        if (teste(j, "Fuga", vazio / 10, 12 + risco)) {
            diario_poe(&j->diario, "Você some entre os asteroides.");
        } else {
            dano(j, 10, 25);
        }
    } else {
        double pedagio = j->creditos * 0.15;
        if (pedagio < 200) pedagio = j->creditos < 200 ? j->creditos : 200;
        j->creditos -= pedagio;
        diario_poe(&j->diario, "Você pagou \001r%.0f\001d créditos de pedágio.",
                   pedagio);
    }
}

static void evento(Jogo *j, int risco) {
    double pesos[N_EVENTOS] = {2.0 + risco * 3, 2.0, 2.0};
    switch ((TipoEvento)rng_ponderado(&j->rng, pesos, N_EVENTOS)) {
    case EV_PIRATAS: piratas(j, risco); break;
    case EV_TEMPESTADE:
        diario_poe(&j->diario,
                   "Uma \001ytempestade de íons\001d sacode a nave.");
        dano(j, 5, 15);
        break;
    case EV_DESTROCOS: {
        int b = rng_faixa(&j->rng, 0, N_BENS - 1);
        int q = rng_faixa(&j->rng, 1, 5);
        int cabe = j->carga_max - carga_usada(j);
        if (q > cabe) q = cabe;
        j->carga[b] += q;
        diario_poe(&j->diario,
                   "Destroços à deriva! Você recolhe \001g%d de %s\001d.", q,
                   BENS[b].nome);
        break;
    }
    case N_EVENTOS: break;
    }
}

/* Na chegada a um planeta que não é de fronteira, a patrulha pode revistar a
   carga. */
static void patrulha(Jogo *j) {
    if (j->planetas[j->aqui].tipo == FRONTEIRA) return;
    for (int b = 0; b < N_BENS; b++) {
        if (!BENS[b].ilegal || j->carga[b] == 0 || !rng_chance(&j->rng, 0.5))
            continue;
        double multa = 300.0 + j->carga[b] * 40.0;
        diario_poe(
            &j->diario,
            "A \001rpatrulha\001d confiscou %d de %s e cobrou %.0f de multa.",
            j->carga[b], BENS[b].nome, multa);
        j->carga[b] = 0;
        j->creditos -= multa;
    }
}

/* Anima um dia de viagem: oito quadros, do progresso inicial ao final. */
static void anima(const Jogo *j, int origem, int destino, DesenhoViagem desenha,
                  double de, double ate) {
    if (desenha == NULL) return;
    for (int q = 1; q <= 8; q++) {
        desenha(j, origem, destino, de + (ate - de) * q / 8.0);
        tela_mostra();
        term_dorme(45);
    }
}

void viaja(Jogo *j, int destino, DesenhoViagem desenha) {
    int origem = j->aqui;
    int dias = dias_de_viagem(j, destino);
    int risco = j->planetas[destino].risco;
    j->combustivel -= combustivel_da_viagem(j, destino);
    diario_poe(&j->diario, "Partindo de %s para %s: %d dia%s de viagem.",
               j->planetas[j->aqui].nome, j->planetas[destino].nome, dias,
               dias == 1 ? "" : "s");
    for (int d = 0; d < dias && j->casco > 0; d++) {
        anima(j, origem, destino, desenha, (double)d / dias, (d + 1.0) / dias);
        passa_um_dia(j);
        /* o novo dia */
        if (desenha != NULL) desenha(j, origem, destino, (d + 1.0) / dias);
        if (rng_chance(&j->rng, 0.06 + 0.08 * risco)) evento(j, risco);
    }
    if (j->casco <= 0) {
        j->terminou = 1;
        diario_poe(
            &j->diario,
            "\001rA nave se desfaz no vácuo.\001d Fim da linha, capitão.");
        return;
    }
    j->aqui = destino;
    diario_poe(&j->diario, "Chegada a \001y%s\001d, um mundo %s. Dia %d.",
               j->planetas[destino].nome, NOME_TIPO[j->planetas[destino].tipo],
               j->dia);
    patrulha(j);
    if (j->dia > DIAS_DE_JOGO) j->terminou = 1;
}
