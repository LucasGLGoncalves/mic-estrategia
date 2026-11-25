/* caminhos.c - três jeitos de achar um caminho, e o rastro que cada um deixa */
#include <stdio.h>
#include <time.h>

#include "../../motor/caminho.h"
#include "../../motor/rng.h"
#include "../../motor/ruido.h"
#include "../../motor/tela.h"
#include "../../motor/term.h"

#define L 70
#define A 22

typedef enum { CHAO, PANTANO, MATA, PAREDE } Terreno;
static Terreno mapa[A][L];

/* O custo de entrar em cada tipo de terreno: pântano e mata cansam mais. */
static int custo(void *dados, int x, int y) {
    (void)dados;
    switch (mapa[y][x]) {
    case CHAO: return 1;
    case PANTANO: return 5;
    case MATA: return 3;
    default: return -1; /* parede: não passa */
    }
}

/* Gera um mapa com ruído; se o X ficar isolado, tenta a próxima semente. */
static uint64_t gera_mapa(uint64_t semente) {
    for (;;) {
        for (int y = 0; y < A; y++)
            for (int x = 0; x < L; x++) {
                double h = fbm2(semente, x * 0.09, y * 0.18, 4);
                double u = fbm2(semente + 5, x * 0.07, y * 0.14, 3);
                mapa[y][x] = h > 0.66   ? PAREDE
                             : u > 0.6  ? PANTANO
                             : u < 0.36 ? MATA
                                        : CHAO;
            }
        mapa[A / 2][2] = mapa[A / 2][L - 3] = CHAO;
        ResultadoBusca r = caminho_busca(L, A, 2, A / 2, L - 3, A / 2, custo,
                                         NULL, BUSCA_LARGURA, NULL, 0, NULL);
        if (r.passos > 0) return semente;
        semente++;
    }
}

int main(void) {
    if (!term_inicia() || !tela_inicia(100, 25)) return 1;
    uint64_t semente = gera_mapa(7);
    int ox = 2, oy = A / 2, dx = L - 3, dy = A / 2;
    ModoBusca modo = BUSCA_ESTRELA;
    const char *nomes[3] = {"Busca em largura", "Dijkstra", "A*"};
    static int dist[A * L], passos[2 * A * L];

    for (;;) {
        ResultadoBusca res[3];
        for (int m = 0; m < 3; m++)
            res[m] = caminho_busca(L, A, ox, oy, dx, dy, custo, NULL,
                                   (ModoBusca)m, NULL, 0, NULL);
        ResultadoBusca r = caminho_busca(L, A, ox, oy, dx, dy, custo, NULL,
                                         modo, passos, A * L, dist);

        tela_limpa(FUNDO);
        int maior = 1;
        for (int i = 0; i < A * L; i++)
            if (dist[i] > maior) maior = dist[i];
        for (int y = 0; y < A; y++)
            for (int x = 0; x < L; x++) {
                static const uint32_t ch[4] = {0x00B7, ',', 0x2663, 0x2588};
                static const Cor cor[4] = {RGB(150, 150, 130),
                                           RGB(80, 160, 150), RGB(60, 150, 70),
                                           RGB(110, 100, 90)};
                Cor fundo = FUNDO;
                int d = dist[y * L + x];
                if (d >= 0)
                    fundo = cor_mistura(RGB(30, 40, 90), RGB(90, 30, 40),
                                        (double)d / maior);
                tela_poe(x + 1, y + 2, ch[mapa[y][x]], cor[mapa[y][x]], fundo);
            }
        for (int i = 0; i < r.passos; i++)
            tela_poe(passos[2 * i] + 1, passos[2 * i + 1] + 2, 0x25CF, AMARELO,
                     COR_MANTEM);
        tela_poe(ox + 1, oy + 2, '@', BRANCO, VERDE);
        tela_poe(dx + 1, dy + 2, 'X', BRANCO, VERMELHO);

        tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
        tela_textof(1, 0, AMARELO, PAINEL, "Caminhos: %s", nomes[modo]);
        tela_caixa(72, 2, 27, 21, CINZA_ESCURO, FUNDO, "Comparação");
        for (int m = 0; m < 3; m++) {
            int y = 4 + m * 5;
            tela_texto(74, y, nomes[m], m == (int)modo ? AMARELO : BRANCO,
                       FUNDO);
            tela_textof(74, y + 1, CINZA, FUNDO, "passos      %4d",
                        res[m].passos);
            tela_textof(74, y + 2, CINZA, FUNDO, "custo       %4d",
                        res[m].custo);
            tela_textof(74, y + 3, CINZA, FUNDO, "examinadas  %4d",
                        res[m].explorados);
        }
        tela_texto(74, 19, "· chão 1  , pântano 5", CINZA, FUNDO);
        tela_texto(74, 20, "♣ mata 3  █ parede", CINZA, FUNDO);
        tela_preenche(0, 24, 100, 1, ' ', CINZA, PAINEL);
        tela_texto(1, 24,
                   "1 largura  2 Dijkstra  3 A*   setas movem o X   N novo "
                   "mapa   Esc sai",
                   CINZA, PAINEL);
        tela_mostra();

        int t = term_espera_tecla(1000);
        if (t == TECLA_ESC || t == TECLA_SAIR) break;
        if (t >= '1' && t <= '3') modo = (ModoBusca)(t - '1');
        int nx = dx, ny = dy;
        if (t == TECLA_CIMA) ny--;
        if (t == TECLA_BAIXO) ny++;
        if (t == TECLA_ESQUERDA) nx--;
        if (t == TECLA_DIREITA) nx++;
        if (nx >= 0 && ny >= 0 && nx < L && ny < A && mapa[ny][nx] != PAREDE)
            dx = nx, dy = ny;
        if (t == 'n' || t == 'N') {
            semente = gera_mapa(hash_mistura(semente + (uint64_t)time(NULL)));
            dx = L - 3, dy = A / 2;
        }
    }
    tela_encerra();
    term_encerra();
    return 0;
}
