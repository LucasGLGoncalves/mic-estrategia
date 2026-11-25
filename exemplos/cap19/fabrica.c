/* fabrica.c - um pequeno jogo em tempo real: o metal brota a cada instante */
#include <math.h>
#include <stdio.h>

#include "../../motor/tela.h"
#include "../../motor/term.h"

#define PASSO 0.1 /* a simulação avança sempre de 0,1 em 0,1 segundo */

typedef enum { TITULO, JOGANDO, PAUSADO } Estado;

typedef struct {
    double metal;
    int minas, refinarias;
    double tempo; /* segundos de jogo */
    int velocidade;
} Fabrica;

static double custo_mina(const Fabrica *f) {
    return 15.0 * pow(1.15, f->minas);
}
static double custo_refinaria(const Fabrica *f) {
    return 200.0 * pow(1.8, f->refinarias);
}
static double producao(const Fabrica *f) {
    return f->minas * 1.0 * (1.0 + 0.5 * f->refinarias);
}

/* Um passo fixo de simulação: o mundo só muda aqui. */
static void simula(Fabrica *f, double dt) {
    f->metal += producao(f) * dt;
    f->tempo += dt;
}

static void desenha_fabricas(const Fabrica *f, double t) {
    int mostrar = f->minas < 12 ? f->minas : 12;
    for (int i = 0; i < mostrar; i++) {
        int x = 4 + i * 8;
        tela_texto(x, 12, " ▟██▙ ", MARROM, FUNDO);
        tela_texto(x, 13, " █▒▒█ ", MARROM, FUNDO);
        tela_texto(x, 14, "▀▀▀▀▀▀", CINZA_ESCURO, FUNDO);
        /* fumaça subindo, cada chaminé num ritmo */
        for (int k = 0; k < 3; k++) {
            double fase = fmod(t * 0.8 + i * 0.37 + k / 3.0, 1.0);
            int y = 11 - (int)(fase * 6);
            int dx = (int)(sin(t * 2 + i + k) * 1.5);
            tela_poe(x + 3 + dx, y, fase < 0.5 ? 0x00B0 : '.',
                     cor_mistura(BRANCO, FUNDO, fase), FUNDO);
        }
    }
    if (f->minas > 12)
        tela_textof(4 + 12 * 8, 13, CINZA, FUNDO, "+%d", f->minas - 12);
}

static void desenha(const Fabrica *f, Estado e, double t, double fps) {
    tela_limpa(FUNDO);
    tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 0, "A FÁBRICA", AMARELO, PAINEL);
    tela_textof(60, 0, CINZA, PAINEL, "tempo %6.1f s   x%d   %4.0f quadros/s",
                f->tempo, f->velocidade, fps);
    if (e == TITULO) {
        tela_texto_centro(50, 10, "A FÁBRICA", AMARELO, FUNDO);
        tela_texto_centro(50, 12,
                          "O metal brota sozinho. Construa minas e refinarias.",
                          BRANCO, FUNDO);
        tela_texto_centro(50, 15, "Enter para começar", CINZA, FUNDO);
        return;
    }
    tela_textof(4, 3, BRANCO, FUNDO, "Metal: %.0f", floor(f->metal));
    tela_textof(4, 4, VERDE, FUNDO, "+%.1f por segundo", producao(f));
    desenha_fabricas(f, t);

    int pode1 = f->metal >= custo_mina(f),
        pode2 = f->metal >= custo_refinaria(f);
    tela_textof(4, 17, pode1 ? BRANCO : CINZA_ESCURO, FUNDO,
                "[1] Mina       %8.0f  (tem %d)", custo_mina(f), f->minas);
    tela_textof(4, 18, pode2 ? BRANCO : CINZA_ESCURO, FUNDO,
                "[2] Refinaria  %8.0f  (tem %d, +50%% cada)",
                custo_refinaria(f), f->refinarias);
    tela_barra(4, 20, 40, f->metal / custo_refinaria(f), LARANJA,
               RGB(30, 34, 48));
    tela_texto(46, 20, "até a próxima refinaria", CINZA, FUNDO);
    tela_texto(4, 23, "P pausa   + e - mudam a velocidade   Esc sai", CINZA,
               FUNDO);
    if (e == PAUSADO) {
        tela_caixa(38, 8, 24, 3, AMARELO, PAINEL, NULL);
        tela_texto_centro(50, 9, "PAUSADO", AMARELO, PAINEL);
    }
}

int main(void) {
    if (!term_inicia() || !tela_inicia(100, 25)) return 1;
    Fabrica f = {0, 1, 0, 0, 1};
    Estado estado = TITULO;
    double anterior = term_agora(), acumulado = 0, inicio = anterior;
    double fps = 0, conta_tempo = 0;
    int quadros = 0;

    for (;;) {
        /* 1. entrada: trata todas as teclas que chegaram */
        int t;
        while ((t = term_tecla()) != 0) {
            if (t == TECLA_ESC || t == TECLA_SAIR) goto fim;
            if (estado == TITULO) {
                if (t == TECLA_ENTER) estado = JOGANDO;
            } else if (t == 'p' || t == 'P') {
                estado = estado == PAUSADO ? JOGANDO : PAUSADO;
            } else if (estado == JOGANDO) {
                if (t == '1' && f.metal >= custo_mina(&f)) {
                    f.metal -= custo_mina(&f);
                    f.minas++;
                } else if (t == '2' && f.metal >= custo_refinaria(&f)) {
                    f.metal -= custo_refinaria(&f);
                    f.refinarias++;
                } else if (t == '+' && f.velocidade < 64) {
                    f.velocidade *= 2;
                } else if (t == '-' && f.velocidade > 1) {
                    f.velocidade /= 2;
                }
            }
        }
        /* 2. tempo: quanto passou desde o último quadro? */
        double agora = term_agora();
        double dt = agora - anterior;
        anterior = agora;
        if (dt > 0.25) dt = 0.25; /* o computador travou? não pula demais */
        if (estado == JOGANDO) {
            acumulado += dt * f.velocidade;
            while (acumulado >= PASSO) { /* passos fixos, quantos couberem */
                simula(&f, PASSO);
                acumulado -= PASSO;
            }
        }
        /* 3. desenho, e a medida de quadros por segundo */
        quadros++;
        conta_tempo += dt;
        if (conta_tempo >= 1.0) {
            fps = quadros / conta_tempo;
            quadros = 0;
            conta_tempo = 0;
        }
        desenha(&f, estado, agora - inicio, fps);
        tela_mostra();
        /* uns 60 quadros por segundo, sem esquentar o processador */
        term_dorme(16);
    }
fim:
    tela_encerra();
    term_encerra();
    printf("Você fechou a fábrica com %.0f de metal e %d minas.\n", f.metal,
           f.minas);
    return 0;
}
