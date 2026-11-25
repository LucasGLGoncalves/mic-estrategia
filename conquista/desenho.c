/* desenho.c - o mapa com névoa de guerra, o painel e o diário */
#include "desenho.h"

#include <math.h>
#include <stdio.h>

#include "../motor/tela.h"

#define MAPA_X 1 /* onde a casa (0, 0) aparece na tela */
#define MAPA_Y 2

static const Cor COR_DONO[N_DONOS] = {RGB(105, 105, 110), RGB(50, 100, 200),
                                      RGB(185, 50, 50)};

/* O chão de uma casa: dois caracteres, frente e fundo. */
static void terreno(const Jogo *j, int x, int y, double agora, uint32_t c[2],
                    Cor *fr, Cor *fu) {
    uint64_t h = hash_xy(42, x, y);
    c[0] = c[1] = ' ';
    switch (j->mapa.terreno[y][x]) {
    case MAR:
        *fu = RGB(18, 42, 84);
        *fr = RGB(70, 120, 190);
        /* ondas que vêm e vão */
        if ((h + (uint64_t)(agora * 2)) % 9 == 0) c[h % 2] = '~';
        break;
    case PLANICIE:
        *fu = RGB(56, 88, 46);
        *fr = RGB(100, 140, 76);
        if (h % 6 == 0) c[0] = '.';
        break;
    case FLORESTA:
        *fu = RGB(30, 64, 34);
        *fr = RGB(80, 160, 80);
        c[h % 2] = 0x2663; /* ♣ */
        break;
    default:
        *fu = RGB(88, 86, 92);
        *fr = RGB(215, 215, 220);
        c[0] = 0x25B2; /* ▲ */
        break;
    }
}

/* A barrinha de vida, num caractere só (▁▂▃▄▅▆▇), só para quem está ferido. */
static uint32_t pip_de_vida(const Unidade *u, Cor *cor) {
    double f = (double)u->vida / UNIDADES[u->tipo].vida;
    if (f >= 1) return ' ';
    *cor = f > 0.6 ? VERDE : f > 0.3 ? AMARELO : VERMELHO;
    int n = (int)ceil(f * 7);
    if (n < 1) n = 1;
    return 0x2580 + n; /* de U+2581 a U+2587 */
}

static void desenha_casa(const Jogo *j, const Interface *ui, int x, int y,
                         double agora) {
    int sx = MAPA_X + 2 * x, sy = MAPA_Y + y;
    int visto = ui->revela || j->visto[JOGADOR][y][x];
    int visivel = ui->revela || j->visivel[JOGADOR][y][x];
    if (!visto) { /* névoa: nunca visto */
        tela_preenche(sx, sy, 2, 1, ' ', CINZA, RGB(6, 8, 12));
        return;
    }
    uint32_t c[2];
    Cor fr, fu;
    terreno(j, x, y, agora, c, &fr, &fu);
    int ci = cidade_em(&j->mapa, x, y);
    if (ci >= 0) { /* a cidade, com o dono que o jogador lembra */
        const Cidade *cid = &j->mapa.cidades[ci];
        Dono d = visivel ? cid->dono : j->lembra[JOGADOR][ci];
        fu = COR_DONO[d];
        fr = BRANCO;
        c[0] = 0x2302;                      /* ⌂ */
        c[1] = cid->capital ? 0x2605 : ' '; /* ★ */
    }
    int u = visivel ? unidade_em(j, x, y) : -1;
    Cor fr2 = fr;
    if (u >= 0) {
        const Unidade *un = &j->unidades[u];
        char letra = UNIDADES[un->tipo].letra;
        /* entrincheirada */
        if (un->ordem == ORDEM_SENTINELA) letra = (char)(letra - 'A' + 'a');
        fu = un->dono == JOGADOR ? RGB(60, 115, 225) : RGB(205, 65, 60);
        fr = BRANCO;
        c[0] = (uint32_t)letra;
        c[1] = pip_de_vida(un, &fr2);
        if (u == ui->sel && fmod(agora, 0.8) < 0.45) { /* a selecionada pisca */
            fu = AMARELO;
            fr = PRETO;
        }
    }
    if (!visivel) { /* lembrado, mas fora da vista: tudo mais escuro */
        fu = cor_escurece(fu, 0.5);
        fr = cor_escurece(fr, 0.5);
        fr2 = cor_escurece(fr2, 0.5);
    }
    tela_poe(sx, sy, c[0], fr, fu);
    tela_poe(sx + 1, sy, c[1], fr2, fu);
}

static void desenha_mapa(const Jogo *j, const Interface *ui, double agora) {
    tela_caixa(0, 1, 70, 26, CINZA_ESCURO, FUNDO, "Continente");
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) desenha_casa(j, ui, x, y, agora);
    if (ui->modo != MODO_MIRA || ui->sel < 0) return;
    /* a mira: o caminho que o A* encontrou, em pontos, e o cursor piscando */
    int xy[2 * MAPA_L * MAPA_A];
    int n =
        caminho_lista(j, ui->sel, ui->cx, ui->cy, xy, MAPA_L * MAPA_A, NULL);
    for (int i = 0; i + 1 < n; i++)
        tela_poe(MAPA_X + 2 * xy[2 * i] + 1, MAPA_Y + xy[2 * i + 1], 0x2022,
                 AMARELO, COR_MANTEM);
    int sx = MAPA_X + 2 * ui->cx, sy = MAPA_Y + ui->cy;
    Cor fundo = fmod(agora, 0.6) < 0.3 ? AMARELO : LARANJA;
    tela_poe(sx, sy, '[', PRETO, fundo);
    tela_poe(sx + 1, sy, ']', PRETO, fundo);
}

static void painel_unidade(const Jogo *j, const Interface *ui) {
    const Unidade *un = &j->unidades[ui->sel];
    const DadosUnidade *d = &UNIDADES[un->tipo];
    int x = 73, y = 2;
    tela_caixa(71, 1, 29, 12, CINZA_ESCURO, FUNDO, d->nome);
    tela_textof(x, y, BRANCO, FUNDO, "Vida %2d/%-2d", un->vida, d->vida);
    tela_barra(x + 11, y, 13, (double)un->vida / d->vida, VERDE, PAINEL);
    tela_textof(x, y + 1, BRANCO, FUNDO, "Força %d   Movimento %d/%d", d->forca,
                un->movimento, d->movimento);
    Terreno t = j->mapa.terreno[un->y][un->x];
    int c = cidade_em(&j->mapa, un->x, un->y);
    double def = defesa_terreno(t) * (c >= 0 ? 2.0 : 1.0) *
                 (un->ordem == ORDEM_SENTINELA ? 1.25 : 1);
    tela_textof(x, y + 2, CINZA, FUNDO, "Terreno: %s",
                c >= 0 ? "cidade" : NOME_TERRENO[t]);
    tela_textof(x, y + 3, CINZA, FUNDO, "Defesa: x%.2f", def);
    const char *ordem = un->ordem == ORDEM_SENTINELA ? "sentinela"
                        : un->ordem == ORDEM_IR      ? "em marcha"
                                                     : "aguardando";
    tela_textof(x, y + 4, CINZA, FUNDO, "Ordem: %s", ordem);
    if (ui->modo == MODO_MIRA) {
        int custo, xy[2 * MAPA_L * MAPA_A];
        int n = caminho_lista(j, ui->sel, ui->cx, ui->cy, xy, MAPA_L * MAPA_A,
                              &custo);
        if (n > 0) {
            int turnos = (custo + d->movimento - 1) / d->movimento;
            tela_textof(x, y + 6, AMARELO, FUNDO, "Vai até: %d passos", n);
            tela_textof(x, y + 7, AMARELO, FUNDO, "cerca de %d turno%s", turnos,
                        turnos == 1 ? "" : "s");
        } else {
            tela_texto(x, y + 6, "Sem caminho até lá.", VERMELHO, FUNDO);
        }
        int alvo = unidade_em(j, ui->cx, ui->cy);
        int vis = ui->revela || j->visivel[JOGADOR][ui->cy][ui->cx];
        if (vis && alvo >= 0 && j->unidades[alvo].dono != JOGADOR)
            tela_textof(x, y + 8, BRANCO, FUNDO, "Chance de vitória: %.0f%%",
                        100 * chance_de_vitoria(j, ui->sel, ui->cx, ui->cy));
        tela_texto(x, y + 9, "Enter confirma  Esc volta", CINZA, FUNDO);
        return;
    }
    if (c >= 0 && j->mapa.cidades[c].dono == JOGADOR) {
        const Cidade *ci = &j->mapa.cidades[c];
        tela_textof(x, y + 6, BRANCO, FUNDO, "Em %s", ci->nome);
        tela_textof(x, y + 7, CINZA, FUNDO, "produz %s %d/%d",
                    UNIDADES[ci->produz].nome, ci->progresso,
                    UNIDADES[ci->produz].custo);
        tela_texto(x, y + 8, "Enter muda a produção", CINZA, FUNDO);
    }
}

static void painel_cidade(const Jogo *j, const Interface *ui) {
    const Cidade *ci = &j->mapa.cidades[ui->cidade];
    int x = 73, y = 2;
    char titulo[40];
    snprintf(titulo, sizeof titulo, "%s%s", ci->nome, ci->capital ? " ★" : "");
    tela_caixa(71, 1, 29, 12, CINZA_ESCURO, FUNDO, titulo);
    const DadosUnidade *d = &UNIDADES[ci->produz];
    int ritmo = ci->capital ? 2 : 1;
    int faltam = (d->custo - ci->progresso + ritmo - 1) / ritmo;
    tela_textof(x, y, BRANCO, FUNDO, "Produz: %s", d->nome);
    tela_barra(x, y + 1, 24, (double)ci->progresso / d->custo, AZUL, PAINEL);
    tela_textof(x, y + 2, CINZA, FUNDO, "pronto em %d turno%s", faltam,
                faltam == 1 ? "" : "s");
    if (ci->capital)
        tela_texto(x, y + 3, "A capital produz em dobro.", CINZA, FUNDO);
    tela_textof(x, y + 5, CINZA, FUNDO, "Força %d  Vida %d  Mov. %d", d->forca,
                d->vida, d->movimento);
    tela_texto(x, y + 7, "Enter muda a produção", AMARELO, FUNDO);
    tela_texto(x, y + 8, "C passa para a próxima", CINZA, FUNDO);
}

void desenha(const Jogo *j, const Interface *ui, double agora) {
    tela_limpa(FUNDO);
    tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 0, "♜ CONQUISTA", AMARELO, PAINEL);
    tela_textof(15, 0, CINZA, PAINEL, "Turno %d", j->turno);
    tela_textof(28, 0, CINZA, PAINEL, "Cidades: ");
    int x = 37;
    x += tela_textof(x, 0, RGB(110, 160, 255), PAINEL, "você %d",
                     cidades_de(j, JOGADOR)) +
         2;
    x += tela_textof(x, 0, RGB(240, 110, 100), PAINEL, "inimigo %d",
                     cidades_de(j, COMPUTADOR)) +
         2;
    tela_textof(x, 0, CINZA, PAINEL, "independentes %d", cidades_de(j, NEUTRO));
    if (ui->vez_da_ia) tela_texto(83, 0, "vez do inimigo…", VERMELHO, PAINEL);
    else tela_texto(91, 0, "sua vez", VERDE, PAINEL);

    desenha_mapa(j, ui, agora);
    if (ui->sel >= 0 && j->unidades[ui->sel].viva) painel_unidade(j, ui);
    else if (ui->cidade >= 0) painel_cidade(j, ui);
    else {
        tela_caixa(71, 1, 29, 12, CINZA_ESCURO, FUNDO, "Ordens");
        tela_quebra(
            73, 2, 25, 10,
            "Todas as unidades já agiram.\n\n\001yT\001d encerra o turno.\n"
            "\001yC\001d escolhe uma cidade.",
            CINZA, FUNDO);
    }
    tela_caixa(71, 13, 29, 14, CINZA_ESCURO, FUNDO, "Diário");
    diario_desenha(&j->diario, 73, 14, 25, 12);
    tela_texto(
        1, 27,
        "Setas movem  Tab próxima  Espaço espera  F fortifica  G vai até",
        CINZA, FUNDO);
    tela_texto(1, 28, "C cidades  Enter produção  T encerra o turno  Esc sai",
               CINZA, FUNDO);
    tela_preenche(0, 29, 100, 1, ' ', CINZA, PAINEL);
    tela_texto(1, 29,
               "E exército  B blindado  X explorador  letra minúscula: "
               "fortificado  ⌂ cidade  ★ capital",
               CINZA, PAINEL);
}
