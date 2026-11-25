/* telas.c - mercado, mapa estelar, estaleiro, banco e diário */
#include "telas.h"

#include <math.h>
#include <stdio.h>

#include "../motor/tela.h"

static const Cor COR_TIPO[N_TIPOS] = {VERDE, LARANJA, CINZA, CIANO, VERMELHO};

int destino_da_linha(const Jogo *j, int linha) {
    return linha < j->aqui ? linha : linha + 1;
}

const char *nome_melhoria(Melhoria m) {
    static const char *nomes[N_MELHORIAS] = {
        "Porão maior (+10 de carga)", "Canhão extra", "Escudo (reduz o dano)",
        "Reparo completo do casco", "Encher o tanque"};
    return nomes[m];
}

double custo_melhoria(const Jogo *j, Melhoria m) {
    switch (m) {
    case M_CARGA: return 1500.0 * (1 + (j->carga_max - CARGA_INICIAL) / 10);
    case M_CANHAO: return 1800.0 * j->canhoes;
    case M_ESCUDO: return j->escudo >= 3 ? 0 : 2500.0 * (j->escudo + 1);
    case M_REPARO: return 8.0 * (100 - j->casco);
    case M_COMBUSTIVEL: return 15.0 * (j->combustivel_max - j->combustivel);
    default: return 0;
    }
}

static void barra_superior(const Jogo *j) {
    char c[24], d[24];
    tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 0, "✦ MERCADOR DAS ESTRELAS", AMARELO, PAINEL);
    fmt_num(c, sizeof c, j->creditos);
    fmt_num(d, sizeof d, j->divida);
    tela_textof(34, 0, CINZA, PAINEL, "Dia %d/%d   Créditos %s   Dívida %s",
                j->dia, DIAS_DE_JOGO, c, d);
}

static void cabecalho(const Jogo *j, const Interface *ui) {
    barra_superior(j);
    static const char *abas[N_ABAS] = {"1 Mercado", "2 Mapa", "3 Nave",
                                       "4 Banco", "5 Diário"};
    ui_abas(1, 2, abas, N_ABAS, ui->aba);
    const Planeta *p = &j->planetas[j->aqui];
    tela_textof(60, 2, COR_TIPO[p->tipo], FUNDO, "● %s, mundo %s", p->nome,
                NOME_TIPO[p->tipo]);
}

static void painel_nave(const Jogo *j, int x, int y) {
    tela_caixa(x, y, 30, 10, CINZA_ESCURO, FUNDO, "Nave");
    tela_textof(x + 2, y + 1, BRANCO, FUNDO, "Carga       %3d/%d",
                carga_usada(j), j->carga_max);
    tela_barra(x + 2, y + 2, 26, (double)carga_usada(j) / j->carga_max, AMARELO,
               PAINEL);
    tela_textof(x + 2, y + 3, BRANCO, FUNDO, "Combustível %3d/%d",
                j->combustivel, j->combustivel_max);
    tela_barra(x + 2, y + 4, 26, (double)j->combustivel / j->combustivel_max,
               CIANO, PAINEL);
    tela_textof(x + 2, y + 5, j->casco < 40 ? VERMELHO : BRANCO, FUNDO,
                "Casco       %3d%%", j->casco);
    tela_barra(x + 2, y + 6, 26, j->casco / 100.0,
               j->casco < 40 ? VERMELHO : VERDE, PAINEL);
    tela_textof(x + 2, y + 7, BRANCO, FUNDO, "Canhões %d   Escudo %d",
                j->canhoes, j->escudo);
    char v[24];
    fmt_num(v, sizeof v, patrimonio(j));
    tela_textof(x + 2, y + 8, patrimonio(j) >= 0 ? VERDE : VERMELHO, FUNDO,
                "Patrimônio %s", v);
}

static void mercado(const Jogo *j, const Interface *ui) {
    tela_texto(2, 4,
               "Mercadoria       Compra    Venda   Estoque   Na nave   Pago",
               CINZA, FUNDO);
    for (int b = 0; b < N_BENS; b++) {
        int y = 5 + b, sel = b == ui->bens.sel;
        Cor fundo = sel ? RGB(40, 60, 100) : FUNDO;
        tela_preenche(1, y, 66, 1, ' ', BRANCO, fundo);
        char nome[40];
        fmt_ajusta(nome, sizeof nome, BENS[b].nome, 14);
        tela_textof(2, y, BENS[b].ilegal ? LARANJA : BRANCO, fundo, "%s", nome);
        tela_textof(18, y, BRANCO, fundo, "%7.0f  %7.0f   %7.0f",
                    preco_compra(j, j->aqui, b), preco_venda(j, j->aqui, b),
                    j->planetas[j->aqui].estoque[b]);
        if (j->carga[b] > 0) {
            int lucro = preco_venda(j, j->aqui, b) > j->pago[b];
            tela_textof(51, y, BRANCO, fundo, "%5d", j->carga[b]);
            tela_textof(59, y, lucro ? VERDE : VERMELHO, fundo, "%6.0f",
                        j->pago[b]);
        }
    }
    tela_quebra(
        2, 13, 64, 4,
        "\001yC\001d compra   \001yV\001d vende   \001yT\001d vende tudo do "
        "item. "
        "Cada unidade comprada encarece a próxima, e cada unidade vendida "
        "barateia a próxima: venda aos poucos, em planetas diferentes.",
        CINZA, FUNDO);
    if (BENS[ui->bens.sel].ilegal)
        tela_texto(2, 17, "Armas são proibidas fora dos mundos de fronteira!",
                   LARANJA, FUNDO);
}

/* o nome do planeta vai à direita do ponto; perto da borda, vai à esquerda */
static void rotulo(int ox, int oy, const Planeta *pl, Cor c) {
    int n = utf8_largura(pl->nome);
    int x = pl->x + 2 + n <= 61 ? pl->x + 2 : pl->x - 1 - n;
    tela_texto(ox + x, oy + pl->y, pl->nome, c, FUNDO);
}

/* o boletim do destino: preços de lá comparados com os daqui */
static void boletim(const Jogo *j, int dest, int x, int y) {
    tela_textof(x, y, AMARELO, FUNDO, "Boletim de %s", j->planetas[dest].nome);
    tela_texto(x, y + 1, "Mercadoria    Venda lá  Lucro", CINZA, FUNDO);
    for (int b = 0; b < N_BENS; b++) {
        char nome[40];
        fmt_ajusta(nome, sizeof nome, BENS[b].nome, 12);
        double la = preco_venda(j, dest, b),
               lucro = la - preco_compra(j, j->aqui, b);
        tela_textof(x, y + 2 + b, BENS[b].ilegal ? LARANJA : BRANCO, FUNDO,
                    "%s %8.0f", nome, la);
        tela_textof(x + 23, y + 2 + b, lucro > 0 ? VERDE : VERMELHO, FUNDO,
                    "%+6.0f", lucro);
    }
}

static void mapa(const Jogo *j, const Interface *ui, double agora) {
    int ox = 2, oy = 5;
    tela_caixa(ox - 1, oy - 1, 64, 22, CINZA_ESCURO, FUNDO, "Mapa estelar");
    for (int y = 0; y < 20; y++)
        for (int x = 0; x < 62; x++)
            if (hash_xy(7, x, y) % 37 == 0)
                tela_poe(ox + x, oy + y, '.', CINZA_ESCURO, FUNDO);
    int dest = destino_da_linha(j, ui->destinos.sel);
    /* a rota até o destino, pontilhada; os pontos andam com o relógio */
    const Planeta *a = &j->planetas[j->aqui], *b = &j->planetas[dest];
    for (int k = 1; k < 40; k++) {
        double t = k / 40.0;
        int x = (int)lround(a->x + (b->x - a->x) * t),
            y = (int)lround(a->y + (b->y - a->y) * t);
        if ((k + (int)(agora * 8)) % 4 == 0)
            tela_poe(ox + x, oy + y, 0x00B7, AMARELO, FUNDO);
    }
    for (int p = 0; p < N_PLANETAS; p++) {
        const Planeta *pl = &j->planetas[p];
        /* ◉ onde você está, ● os outros */
        uint32_t marca = p == j->aqui ? 0x25C9 : 0x25CF;
        tela_poe(ox + pl->x, oy + pl->y, marca, COR_TIPO[pl->tipo], FUNDO);
        rotulo(ox, oy, pl, p == j->aqui ? AMARELO : p == dest ? BRANCO : CINZA);
    }
    tela_texto(68, 4, "Destino    Dias Comb.  Risco", CINZA, FUNDO);
    static const char *risco[4] = {"baixo", "médio", "alto", "letal"};
    for (int l = 0; l < N_PLANETAS - 1; l++) {
        int p = destino_da_linha(j, l), sel = l == ui->destinos.sel;
        Cor fundo = sel ? RGB(40, 60, 100) : FUNDO;
        char nome[40];
        fmt_ajusta(nome, sizeof nome, j->planetas[p].nome, 11);
        tela_preenche(67, 5 + l, 32, 1, ' ', BRANCO, fundo);
        int comb = combustivel_da_viagem(j, p);
        tela_textof(68, 5 + l, BRANCO, fundo, "%s %3d %5d", nome,
                    dias_de_viagem(j, p), comb);
        if (comb > j->combustivel) tela_texto(89, 5 + l, "!", VERMELHO, fundo);
        int r = j->planetas[p].risco;
        tela_texto(91, 5 + l, risco[r], r >= 2 ? VERMELHO : CINZA, fundo);
    }
    tela_quebra(68, 13, 30, 2, "\001yEnter\001d parte para o destino.", CINZA,
                FUNDO);
    boletim(j, dest, 68, 16);
}

static void nave(const Jogo *j, const Interface *ui) {
    int estaleiro = j->planetas[j->aqui].estaleiro;
    tela_texto(2, 4,
               estaleiro ? "Estaleiro" : "Sem estaleiro aqui: só combustível.",
               AMARELO, FUNDO);
    for (int m = 0; m < N_MELHORIAS; m++) {
        int disponivel = estaleiro || m == M_COMBUSTIVEL;
        int sel = m == ui->melhorias.sel, y = 6 + m;
        double c = custo_melhoria(j, m);
        Cor fundo = sel ? RGB(40, 60, 100) : FUNDO;
        tela_preenche(1, y, 60, 1, ' ', BRANCO, fundo);
        char nome[60];
        fmt_ajusta(nome, sizeof nome, nome_melhoria(m), 30);
        tela_textof(2, y, disponivel ? BRANCO : CINZA_ESCURO, fundo,
                    "%s %10.0f", nome, c);
    }
    tela_texto(2, 12, "Enter compra a melhoria escolhida.", CINZA, FUNDO);
}

static void banco(const Jogo *j) {
    if (j->aqui != 6) {
        tela_texto(2, 5, "O Banco de Kepler só atende em Kepler.", CINZA,
                   FUNDO);
        return;
    }
    char d[24];
    fmt_num(d, sizeof d, j->divida);
    tela_texto(2, 4, "Banco de Kepler", AMARELO, FUNDO);
    tela_textof(2, 6, BRANCO, FUNDO, "Dívida atual: %s créditos", d);
    fmt_num(d, sizeof d, j->divida * 0.01);
    tela_textof(2, 7, j->divida > 0 ? VERMELHO : VERDE, FUNDO,
                "Juros de amanhã: %s (1%% ao dia)", d);
    tela_quebra(
        2, 9, 64, 6,
        "\001yP\001d paga parte da dívida.   \001yE\001d pede mais dinheiro "
        "emprestado. "
        "O banco empresta até 20.000 no total.\n\n"
        "Juros de 1% ao dia parecem pouco, mas são compostos: em 60 dias "
        "uma dívida intocada cresce 82%. Pague assim que puder.",
        CINZA, FUNDO);
}

/* A nave, desenhada da esquerda para a direita; o fogo do motor é desenhado
   à parte. */
static const char *NAVE[3] = {"   ▄▄▄▄", " ▐█▓▓████▙▄", "   ▀▀▀▀"};

static void cena_viagem(double progresso) {
    tela_caixa(1, 4, 66, 14, CINZA_ESCURO, FUNDO, "Espaço profundo");
    /* três camadas de estrelas: as mais distantes andam mais devagar
       (paralaxe) */
    double andou = progresso * 400;
    for (int s = 0; s < 70; s++) {
        int camada = s % 3;
        int y = 5 + (int)(hash_xy(11, s, 0) % 10);
        double vel = 0.25 + camada * 0.35;
        int x = 63 - (int)(hash_xy(11, s, 1) % 64 + andou * vel) % 64;
        uint32_t c = camada == 2 ? 0x2022 : '.'; /* • perto, . longe */
        tela_poe(2 + x, y, c, camada == 2 ? BRANCO : CINZA_ESCURO, FUNDO);
    }
    int nx = 24, ny = 9;
    for (int l = 0; l < 3; l++) tela_texto(nx, ny + l, NAVE[l], CINZA, FUNDO);
    int quadro = (int)(progresso * 64);
    tela_texto(nx - 2, ny + 1, quadro % 2 ? "≈≈" : "~≈",
               quadro % 3 ? LARANJA : AMARELO, FUNDO);
}

void desenha_viagem(const Jogo *j, int origem, int destino, double progresso) {
    tela_limpa(FUNDO);
    barra_superior(j);
    const Planeta *a = &j->planetas[origem], *b = &j->planetas[destino];
    int dias = dias_de_viagem(j, destino);
    int hoje = (int)ceil(progresso * dias);
    if (hoje < 1) hoje = 1;
    tela_textof(2, 2, AMARELO, FUNDO, "Em viagem: %s → %s", a->nome, b->nome);
    tela_textof(46, 2, CINZA, FUNDO, "dia %d de %d", hoje, dias);
    cena_viagem(progresso);
    /* a barra de progresso, com os nomes nas pontas */
    tela_texto(3, 16, a->nome, COR_TIPO[a->tipo], FUNDO);
    tela_texto_dir(65, 16, b->nome, COR_TIPO[b->tipo], FUNDO);
    tela_barra(3, 15, 62, progresso, AZUL, PAINEL);
    painel_nave(j, 69, 4);
    tela_caixa(1, 19, 98, 9, CINZA_ESCURO, FUNDO, "Diário de bordo");
    diario_desenha(&j->diario, 3, 20, 94, 7);
    tela_preenche(0, 29, 100, 1, ' ', CINZA, PAINEL);
}

void desenha(const Jogo *j, const Interface *ui, double agora) {
    tela_limpa(FUNDO);
    cabecalho(j, ui);
    switch (ui->aba) {
    case ABA_MERCADO: mercado(j, ui); break;
    case ABA_MAPA: mapa(j, ui, agora); break;
    case ABA_NAVE: nave(j, ui); break;
    case ABA_BANCO: banco(j); break;
    case ABA_DIARIO: diario_desenha(&j->diario, 2, 4, 96, 23); break;
    default: break;
    }
    if (ui->aba != ABA_MAPA && ui->aba != ABA_DIARIO) {
        painel_nave(j, 69, 4);
        tela_caixa(1, 19, 98, 9, CINZA_ESCURO, FUNDO, "Diário de bordo");
        diario_desenha(&j->diario, 3, 20, 94, 7);
    }
    tela_preenche(0, 29, 100, 1, ' ', CINZA, PAINEL);
    tela_texto(1, 29, "1-5 telas   ↑↓ escolhe   S salva   Esc sai", CINZA,
               PAINEL);
}
