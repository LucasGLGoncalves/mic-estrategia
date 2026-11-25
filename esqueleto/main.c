/* main.c - Posto Avançado: o esqueleto de um jogo de estratégia em tempo
   real, para você transformar no seu próprio jogo (capítulo 46).

   Opções:
     --novo         começa um posto novo, mesmo que haja um salvo
     --semente N    o número que gera o mapa (com --novo ou --simula)
     --simula H     o piloto automático joga H horas, sem tela
     --auto H       o piloto joga as primeiras H horas e passa o comando
     --testa        roda os testes automáticos */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../motor/tela.h"
#include "../motor/term.h"
#include "piloto.h"
#include "salvar.h"
#include "telas.h"

#define MAX_FORA 3600.0 /* o tempo fora conta até uma hora */

int roda_testes(void); /* testes.c */

static Jogo jogo;

/* Sem tela: o piloto joga e a evolução vai para a saída padrão. */
static int simula(uint64_t semente, double horas) {
    jogo_novo(&jogo, semente);
    printf("  hora   pontos  população  comida  madeira  pedra\n");
    int passos = (int)(horas * 360); /* passos de 10 segundos */
    for (int i = 0; i <= passos; i++) {
        if (i % (passos / 12 > 0 ? passos / 12 : 1) == 0) {
            Taxas t;
            jogo_taxas(&jogo, &t);
            printf("%6.1f %8.0f %5.0f/%-4d %7.0f %8.0f %6.0f\n",
                   jogo.relogio / 3600, jogo_pontos(&jogo), jogo.populacao,
                   t.moradias, jogo.recurso[COMIDA], jogo.recurso[MADEIRA],
                   jogo.recurso[PEDRA]);
        }
        jogo.n_avisos = 0; /* ninguém vai ler os recados */
        jogo_avanca(&jogo, 10);
        piloto_joga(&jogo);
    }
    return 0;
}

/* Faz o posto trabalhar pelo tempo em que o jogo ficou fechado. */
static void tempo_fora(Interface *ui, time_t salvo_em) {
    double fora = difftime(time(NULL), salvo_em);
    if (fora < 10) return;
    double antes = jogo_pontos(&jogo);
    jogo_avanca(&jogo, fora > MAX_FORA ? MAX_FORA : fora);
    busca_avisos(&jogo, ui);
    char t[24], texto[300];
    fmt_tempo(t, sizeof t, fora);
    snprintf(texto, sizeof texto,
             "Você esteve fora por %s.\nO posto continuou trabalhando: "
             "\001g%+.0f pontos\001d.%s",
             t, jogo_pontos(&jogo) - antes,
             fora > MAX_FORA ? "\n\n(O tempo fora vale até uma hora.)" : "");
    desenha(&jogo, ui); /* o posto já crescido, atrás do aviso */
    ui_aviso("Bem-vindo de volta", texto);
}

int main(int argc, char **argv) {
    int novo = 0;
    uint64_t semente = (uint64_t)time(NULL);
    double horas = 0, automatico = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--novo") == 0) novo = 1;
        else if (strcmp(argv[i], "--semente") == 0 && i + 1 < argc)
            semente = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--simula") == 0 && i + 1 < argc)
            horas = atof(argv[++i]);
        else if (strcmp(argv[i], "--auto") == 0 && i + 1 < argc)
            automatico = atof(argv[++i]);
        else if (strcmp(argv[i], "--testa") == 0) return roda_testes();
    }
    if (horas > 0)
        return simula(semente, horas); /* antes de mexer no terminal */

    if (!term_inicia() || !tela_inicia(TELA_L, TELA_A)) return 1;
    Interface ui;
    time_t salvo_em;
    int carregou = !novo && carrega_jogo(&jogo, ARQUIVO_SAVE, &salvo_em);
    if (!carregou) jogo_novo(&jogo, semente);
    for (double t = 0; t < automatico * 3600; t += 10) {
        jogo_avanca(&jogo, 10);
        piloto_joga(&jogo);
    }
    interface_inicia(&ui, &jogo);
    if (carregou) tempo_fora(&ui, salvo_em);

    /* o laço do jogo: ler, atualizar, desenhar (capítulos 19 e 44) */
    double antes = term_agora(), ultimo_save = antes;
    for (;;) {
        double agora = term_agora();
        jogo_avanca(&jogo, (agora - antes) * ui.velocidade);
        antes = agora;
        busca_avisos(&jogo, &ui);
        desenha(&jogo, &ui);
        tela_mostra();
        if (agora - ultimo_save > 30) { /* salva sozinho a cada meio minuto */
            salva_jogo(&jogo, ARQUIVO_SAVE);
            ultimo_save = agora;
        }
        int t = term_espera_tecla(100);
        if (t != 0 && !trata_tecla(&jogo, &ui, t)) break;
    }
    int ok = salva_jogo(&jogo, ARQUIVO_SAVE);
    tela_encerra();
    term_encerra();
    puts(ok ? "Posto salvo. Até a próxima!"
            : "ATENÇÃO: não foi possível salvar.");
    return 0;
}
