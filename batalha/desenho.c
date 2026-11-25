/* desenho.c - os dois mares, a frota e o diário */
#include <stdio.h>

#include "../motor/tela.h"
#include "ia.h"
#include "jogo.h"

#define MAR_FUNDO RGB(14, 34, 70)
#define MAR_ONDA  RGB(50, 90, 150)

/* Desenha um mar com coordenadas. 'revela' mostra os navios (o seu mar). */
static void desenha_mar(const Tabuleiro *t, int ox, int oy, const char *titulo,
                        int revela, const int calor[TAM][TAM], int ux, int uy,
                        double agora) {
    tela_texto(ox + 3, oy - 2, titulo, AMARELO, FUNDO);
    for (int x = 0; x < TAM; x++)
        tela_textof(ox + 4 + x * 3, oy - 1, CINZA, FUNDO, "%c", 'A' + x);
    int maior = 1;
    if (calor)
        for (int y = 0; y < TAM; y++)
            for (int x = 0; x < TAM; x++)
                if (calor[y][x] > maior) maior = calor[y][x];
    for (int y = 0; y < TAM; y++) {
        tela_textof(ox, oy + y, CINZA, FUNDO, "%2d", y + 1);
        for (int x = 0; x < TAM; x++) {
            int px = ox + 3 + x * 3, n = t->ocupado[y][x];
            Cor fundo = MAR_FUNDO;
            if (calor && t->tiros[y][x] == AGUA)
                fundo = cor_mistura(MAR_FUNDO, RGB(200, 60, 40),
                                    (double)calor[y][x] / maior);
            uint32_t ch = ((x + y + (int)(agora * 2)) % 7 == 0) ? '~' : ' ';
            Cor cor = MAR_ONDA;
            if (revela && n >= 0) {
                ch = 0x25A0; /* ■ */
                cor = CINZA;
            }
            if (t->tiros[y][x] == ERRO) {
                ch = 0x00B7; /* · */
                cor = BRANCO;
            }
            if (t->tiros[y][x] == ACERTO) {
                ch = 0x2716; /* ✖ */
                cor = AMARELO;
                fundo =
                    tab_afundado(t, n) ? RGB(120, 30, 30) : RGB(170, 70, 20);
            }
            if (x == ux && y == uy) fundo = cor_mistura(fundo, BRANCO, 0.25);
            tela_poe(px, oy + y, ' ', cor, fundo);
            tela_poe(px + 1, oy + y, ch, cor, fundo);
            tela_poe(px + 2, oy + y, ' ', cor, fundo);
        }
    }
}

static void desenha_frota(const Tabuleiro *t, int ox, int oy, int revela) {
    for (int i = 0; i < N_NAVIOS; i++) {
        const Navio *n = &t->navios[i];
        int afundado = tab_afundado(t, i);
        Cor c = afundado ? VERMELHO : BRANCO;
        tela_texto(ox, oy + i, n->nome, afundado ? CINZA_ESCURO : CINZA, FUNDO);
        for (int k = 0; k < n->tamanho; k++) {
            /* do inimigo, só se sabe se afundou; do seu, também onde foi
               atingido */
            int atingido = revela ? k < n->acertos : afundado;
            tela_poe(ox + 14 + k, oy + i, atingido ? 0x2716 : 0x25A0,
                     atingido ? c : CINZA, FUNDO);
        }
        if (afundado) tela_texto(ox + 20, oy + i, "afundado", VERMELHO, FUNDO);
    }
}

void jogo_desenha(const Jogo *j, double agora) {
    tela_limpa(FUNDO);
    tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 0, "≈ BATALHA NAVAL", AMARELO, PAINEL);
    const char *fase = j->fase == POSICIONANDO ? "posicione a sua frota"
                       : j->fase == FIM ? (j->venceu ? "vitória!" : "derrota")
                       : j->vez_do_computador ? "o inimigo está mirando..."
                                              : "a sua vez de atirar";
    tela_texto_dir(98, 0, fase, CINZA, PAINEL);

    int calor[TAM][TAM];
    if (j->mostra_calor) ia_calcula_mapa(&j->meu, calor);
    int ux = j->fase == JOGANDO ? j->ultimo_x : -1, uy = j->ultimo_y;
    desenha_mar(&j->meu, 3, 4, "Seu mar", 1,
                j->mostra_calor ? (const int(*)[TAM])calor : NULL, ux, uy,
                agora);
    desenha_mar(&j->dele, 53, 4, "Mar inimigo", j->fase == FIM, NULL, -1, -1,
                agora);

    /* o cursor: o navio sendo posicionado, ou a mira */
    if (j->fase == POSICIONANDO && j->navio < N_NAVIOS) {
        int tam = j->meu.navios[j->navio].tamanho;
        int cabe = tab_cabe(&j->meu, tam, j->cx, j->cy, j->vertical);
        for (int k = 0; k < tam; k++) {
            int x = j->vertical ? j->cx : j->cx + k,
                y = j->vertical ? j->cy + k : j->cy;
            if (x < TAM && y < TAM)
                tela_texto(6 + x * 3, 4 + y, "■", cabe ? VERDE : VERMELHO,
                           COR_MANTEM);
        }
        tela_textof(3, 16, BRANCO, FUNDO, "Posicionando: %s (%d casas)",
                    j->meu.navios[j->navio].nome, tam);
    } else if (j->fase == JOGANDO) {
        int px = 56 + j->cx * 3, py = 4 + j->cy;
        tela_poe(px - 1, py, '[', AMARELO, COR_MANTEM);
        tela_poe(px + 1, py, ']', AMARELO, COR_MANTEM);
        char casa[16];
        nome_casa(j->cx, j->cy, casa);
        tela_textof(53, 16, CINZA, FUNDO, "Mira em %s   tiros: %d", casa,
                    j->tiros_jogador);
        tela_textof(3, 16, CINZA, FUNDO, "Tiros do inimigo: %d",
                    j->tiros_computador);
    }
    desenha_frota(&j->meu, 3, 18, 1);
    desenha_frota(&j->dele, 53, 18, 0);

    tela_caixa(1, 23, 98, 6, CINZA_ESCURO, FUNDO, "Diário de bordo");
    diario_desenha(&j->diario, 3, 24, 94, 4);
    tela_preenche(0, 29, 100, 1, ' ', CINZA, PAINEL);
    tela_texto(1, 29,
               j->fase == POSICIONANDO
                   ? "setas movem   R gira   Enter posiciona   A ao acaso   H "
                     "calor   Esc sai"
               : j->fase == JOGANDO ? "setas miram   Enter atira   H mostra o "
                                      "que o inimigo sabe do seu mar   Esc sai"
                                    : "N nova partida   Esc sai",
               CINZA, PAINEL);
}
