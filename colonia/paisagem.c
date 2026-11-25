/* paisagem.c - o horizonte: céu que muda com a hora, estrelas, sol e lua,
   montanhas ao longe, chão, e um desenho para cada construção */
#include "paisagem.h"

#include <math.h>
#include <string.h>

#include "../motor/rng.h"
#include "../motor/ruido.h"
#include "../motor/tela.h"

#define LARGURA 100
#define CHAO    8 /* a linha (relativa) onde as construções se apoiam */

/* Um desenho de várias linhas. Espaço é transparente; alguns caracteres são
   especiais: @ roda (animada), * luz piscando, w janela, % redemoinho. */
typedef struct {
    const char *linhas[5];
    int altura;
    Cor cor;
} Sprite;

static const Sprite NAVE = {
    {"  ▄█▄  ", " ▐█*█▌ ", " ▐█▀█▌ ", "▟█▀ ▀█▙"}, 4, RGB(170, 175, 190)};
static const Sprite MINA_M = {
    {"   ▄@  ", "  ▐█▌  ", " ▄███▄ ", "▟█▀▀▀█▙"}, 4, RGB(150, 120, 100)};
static const Sprite MINA_C = {
    {"  ▲ ▲  ", " ▲█▲█▲ ", "▲██▲██▲", "▀▀▀▀▀▀▀"}, 4, RGB(90, 200, 220)};
static const Sprite CUPULA = {{" ▄▄▄▄ ", "▟████▙"}, 2, RGB(90, 190, 110)};
static const Sprite PAINEL_SOLAR = {{"▄▄▄", " ┃ "}, 2, RGB(70, 110, 210)};
static const Sprite GALPAO = {{" ▄▀▀▀▄ ", " █▓▓▓█ "}, 2, RGB(190, 140, 90)};
static const Sprite CASA = {{"▄██▄", "█ww█"}, 2, RGB(200, 190, 170)};
static const Sprite LAB = {
    {"   *   ", "   ┃   ", " ▄███▄ ", " █w█w█ "}, 4, RGB(215, 215, 225)};
static const Sprite PORTAL_S = {
    {"  ▄▀▀▀▀▀▄  ", " █ %%%%% █ ", " █ %%%%% █ ", " █ %%%%% █ ", "▄█▄▄▄▄▄▄▄█▄"},
    5,
    RGB(200, 170, 90)};
static const Sprite OBRA = {{"╔╬╗", "╬╬╬"}, 2, AMARELO};

static int noite(double hora) { return hora < 6 || hora >= 19; }

static void desenha_sprite(const Sprite *s, int x, int y_base, double agora,
                           double hora, Cor fundo_ceu[]) {
    int y0 = y_base - s->altura + 1;
    for (int l = 0; l < s->altura; l++) {
        const char *p = s->linhas[l];
        int col = 0;
        uint32_t ch;
        while ((ch = utf8_proximo(&p)) != 0) {
            int sx = x + col, sy = y0 + l;
            col++;
            if (ch == ' ' || sx < 0 || sx >= LARGURA) continue;
            Cor cor = s->cor;
            if (ch == '@') { /* a roda da mina gira */
                static const uint32_t RODA[4] = {0x25D0, 0x25D3, 0x25D1,
                                                 0x25D2};
                ch = RODA[(int)(agora * 6) % 4];
                cor = AMARELO;
            } else if (ch == '*') { /* luz de sinalização */
                ch = 0x2022;
                cor = fmod(agora, 1.2) < 0.4 ? VERMELHO : RGB(90, 30, 30);
            } else if (ch == 'w') { /* janelas acesas à noite */
                ch = 0x25AA;
                cor = noite(hora) ? RGB(255, 210, 90) : RGB(60, 60, 70);
            } else if (ch == '%') { /* o redemoinho do portal */
                static const uint32_t TONS[4] = {0x2591, 0x2592, 0x2593,
                                                 0x2588};
                int k = (int)(agora * 5 + col + l * 2) % 4;
                ch = TONS[k];
                cor =
                    cor_mistura(RGB(80, 60, 200), RGB(180, 120, 255), k / 3.0);
            } else if (ch == 0x25B2 && s == &MINA_C) {
                /* cristais de duas cores */
                cor = col % 2 ? RGB(90, 210, 230) : RGB(210, 110, 230);
            }
            tela_poe(sx, sy, ch, cor, fundo_ceu[sy]);
        }
    }
}

/* A cor do céu de acordo com a hora: noite, aurora, dia e crepúsculo. */
static Cor cor_do_ceu(double hora, double altura) {
    static const struct {
        double hora;
        Cor cor;
    } CHAVE[] = {
        {0, RGB(6, 8, 22)},      {5, RGB(10, 12, 30)},
        {6.5, RGB(120, 70, 90)}, {8, RGB(60, 110, 170)},
        {17, RGB(70, 120, 180)}, {18.5, RGB(170, 90, 60)},
        {20, RGB(20, 16, 40)},   {24, RGB(6, 8, 22)},
    };
    int i = 0;
    while (hora > CHAVE[i + 1].hora) i++;
    double t = (hora - CHAVE[i].hora) / (CHAVE[i + 1].hora - CHAVE[i].hora);
    Cor c = cor_mistura(CHAVE[i].cor, CHAVE[i + 1].cor, t);
    return cor_escurece(c, 0.55 + 0.45 * altura); /* mais escuro no alto */
}

void paisagem_desenha(const Colonia *c, int y0, double agora) {
    /* um minuto real = uma hora de jogo; começa às 7h */
    double hora = fmod(c->relogio / 60.0 + 7, 24);
    Cor fundo[64];
    for (int l = 0; l < PAISAGEM_ALTURA; l++) {
        Cor f = l < CHAO ? cor_do_ceu(hora, (double)l / CHAO) : RGB(96, 52, 36);
        fundo[y0 + l] = f;
        tela_preenche(0, y0 + l, LARGURA, 1, ' ', BRANCO, f);
    }
    /* estrelas, só à noite, cintilando */
    if (noite(hora))
        for (int s = 0; s < 40; s++) {
            int x = (int)(hash_xy(3, s, 0) % LARGURA),
                y = (int)(hash_xy(3, s, 1) % 5);
            double brilho = 0.5 + 0.5 * sin(agora * 2 + s);
            tela_poe(x, y0 + y, brilho > 0.8 ? '+' : '.',
                     cor_mistura(CINZA_ESCURO, BRANCO, brilho), fundo[y0 + y]);
        }
    /* o sol de dia, a lua de noite, num arco de leste a oeste */
    double arco = noite(hora) ? fmod(hora + 5, 24) / 11.0 : (hora - 6) / 13.0;
    int ax = 3 + (int)(arco * 92),
        ay = y0 + 5 - (int)(sin(arco * 3.14159265) * 4.5);
    /* ☾ */
    if (noite(hora)) tela_poe(ax, ay, 0x263E, RGB(230, 230, 200), fundo[ay]);
    else tela_poe(ax, ay, 0x25CF, RGB(255, 220, 120), fundo[ay]);
    /* montanhas ao longe: ruído de uma dimensão dá a silhueta */
    for (int x = 0; x < LARGURA; x++) {
        int h = 1 + (int)(fbm2(11, x * 0.06, 0, 3) * 4);
        Cor m = cor_mistura(fundo[y0 + CHAO - 1], RGB(60, 40, 50), 0.6);
        for (int l = CHAO - h; l < CHAO; l++)
            tela_poe(x, y0 + l, ' ', BRANCO, m);
    }
    /* o chão, com pedrinhas */
    for (int x = 0; x < LARGURA; x++)
        for (int l = CHAO; l < PAISAGEM_ALTURA; l++)
            if (hash_xy(5, x, l) % 7 == 0)
                tela_poe(x, y0 + l, hash_xy(6, x, l) % 2 ? '.' : ',',
                         RGB(150, 95, 70), fundo[y0 + l]);
    for (int l = 0; l < PAISAGEM_ALTURA; l++) fundo[y0 + l] = COR_MANTEM;

    /* as construções, cada uma no seu lugar; o que está em obra ganha
       andaimes */
    int base = y0 + CHAO;
    const int *n = c->nivel;
    Estrutura em_obra = c->n_fila > 0 ? c->fila[0].estrutura : N_ESTRUTURAS;
    desenha_sprite(&NAVE, 1, base, agora, hora, fundo);
    if (n[MINA_METAL]) desenha_sprite(&MINA_M, 11, base, agora, hora, fundo);
    if (n[MINA_CRISTAL]) desenha_sprite(&MINA_C, 20, base, agora, hora, fundo);
    for (int k = 0; k < (n[FAZENDA] + 1) / 2 && k < 2; k++)
        desenha_sprite(&CUPULA, 29 + 7 * k, base, agora, hora, fundo);
    if (n[ARMAZEM]) desenha_sprite(&GALPAO, 43, base, agora, hora, fundo);
    for (int k = 0; k < n[USINA] && k < 4; k++)
        desenha_sprite(&PAINEL_SOLAR, 51 + 4 * k, base, agora, hora, fundo);
    for (int k = 0; k < (n[HABITACAO] + 1) / 2 && k < 4; k++)
        desenha_sprite(&CASA, 68 + 5 * k, base, agora, hora, fundo);
    if (n[LABORATORIO]) desenha_sprite(&LAB, 88, base, agora, hora, fundo);
    /* flutua no céu */
    if (n[PORTAL]) desenha_sprite(&PORTAL_S, 38, base - 4, agora, hora, fundo);
    if (em_obra != N_ESTRUTURAS) {
        static const int ONDE[N_ESTRUTURAS] = {13, 22, 31, 52, 69, 45, 90, 43};
        int x = ONDE[em_obra];
        int y = n[em_obra] ? base - 4 - (em_obra == PORTAL ? 4 : 0) : base;
        if (n[em_obra] == 0 || fmod(agora, 1) < 0.6)
            desenha_sprite(&OBRA, x, y, agora, hora, fundo);
    }
}
