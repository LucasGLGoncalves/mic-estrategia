/* telas.c - o mapa, os painéis e as teclas do Posto Avançado */
#include "telas.h"

#include <stdio.h>

#include "../motor/tela.h"
#include "../motor/term.h"

#define MAPA_X   1 /* onde o mapa começa na tela */
#define MAPA_Y   2
#define PAINEL_X 51

void interface_inicia(Interface *ui, const Jogo *j) {
    ui->cx = MAPA_L / 2;
    ui->cy = MAPA_A / 2;
    ui->escolha = FAZENDA;
    ui->velocidade = 1;
    ui->diario.total = 0;
    for (int y = 0; y < MAPA_A; y++) /* começa o cursor sobre a primeira casa */
        for (int x = 0; x < MAPA_L; x++)
            if (j->mapa[y][x].obra == CASA) {
                ui->cx = x;
                ui->cy = y;
            }
}

void busca_avisos(Jogo *j, Interface *ui) {
    for (int i = 0; i < j->n_avisos; i++)
        diario_poe(&ui->diario, "%s", j->aviso[i]);
    j->n_avisos = 0;
}

/* SEU JOGO: a aparência de cada terreno */
static const struct {
    uint32_t ch;
    Cor frente, fundo;
} VISUAL[N_TERRENOS] = {
    [AGUA] = {'~', RGB(70, 130, 220), RGB(15, 35, 80)},
    [PLANICIE] = {'.', RGB(120, 170, 80), RGB(30, 55, 25)},
    [FLORESTA] = {0x2663, RGB(70, 170, 80), RGB(18, 45, 20)}, /* ♣ */
    [MONTANHA] = {'^', RGB(185, 180, 170), RGB(62, 57, 52)},
};

static void desenha_mapa(const Jogo *j, const Interface *ui) {
    tela_caixa(MAPA_X - 1, MAPA_Y - 1, MAPA_L + 2, MAPA_A + 2, CINZA_ESCURO,
               FUNDO, "Mapa");
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) {
            const Quadra *q = &j->mapa[y][x];
            uint32_t ch = VISUAL[q->terreno].ch;
            Cor frente = VISUAL[q->terreno].frente,
                fundo = VISUAL[q->terreno].fundo;
            if (q->obra != NADA) {
                ch = (uint32_t)CONSTRUCOES[q->obra].letra;
                frente = AMARELO;
            }
            if (x == ui->cx && y == ui->cy) { /* o cursor: cores invertidas */
                Cor t = frente;
                frente = PRETO;
                fundo = t;
            }
            tela_poe(MAPA_X + x, MAPA_Y + y, ch, frente, fundo);
        }
}

static void desenha_recursos(const Jogo *j, const Taxas *t) {
    int x = PAINEL_X, y = 1;
    tela_caixa(x, y, TELA_L - x, 7, CINZA_ESCURO, FUNDO, "Recursos");
    for (int r = 0; r < N_RECURSOS; r++) {
        double taxa = t->producao[r] - (r == COMIDA ? t->fome : 0);
        char v[24];
        fmt_num(v, sizeof v, j->recurso[r]);
        tela_texto(x + 2, y + 1 + r, NOME_RECURSO[r], CINZA, FUNDO);
        tela_texto_dir(x + 20, y + 1 + r, v, BRANCO, FUNDO);
        char tx[24];
        snprintf(tx, sizeof tx, "%+.1f/s", taxa);
        for (char *c = tx; *c; c++)
            if (*c == '.') *c = ','; /* vírgula decimal */
        tela_texto(x + 23, y + 1 + r, tx, taxa < -1e-9 ? VERMELHO : VERDE,
                   FUNDO);
    }
    tela_textof(x + 2, y + 4, CINZA, FUNDO, "População  %.0f de %d",
                j->populacao, t->moradias);
    tela_textof(x + 2, y + 5, t->eficiencia < 0.999 ? LARANJA : CINZA, FUNDO,
                "Trabalho   %d vagas, eficiência %.0f%%", t->vagas,
                t->eficiencia * 100);
}

static void desenha_quadra(const Jogo *j, const Interface *ui) {
    int x = PAINEL_X, y = 8;
    const Quadra *q = &j->mapa[ui->cy][ui->cx];
    tela_caixa(x, y, TELA_L - x, 4, CINZA_ESCURO, FUNDO, "Quadra");
    tela_textof(x + 2, y + 1, BRANCO, FUNDO, "(%d, %d)  %s", ui->cx, ui->cy,
                NOME_TERRENO[q->terreno]);
    if (q->obra != NADA)
        tela_textof(x + 2, y + 2, AMARELO, FUNDO, "%s  (D demole)",
                    CONSTRUCOES[q->obra].nome);
    else tela_texto(x + 2, y + 2, "livre", CINZA_ESCURO, FUNDO);
}

static void desenha_menu(const Jogo *j, const Interface *ui) {
    int x = PAINEL_X, y = 12;
    tela_caixa(x, y, TELA_L - x, 10, CINZA_ESCURO, FUNDO, "Construir");
    for (Construcao c = CASA; c < N_CONSTRUCOES; c++) {
        int yy = y + c;
        const DadosConstrucao *d = &CONSTRUCOES[c];
        double custo[N_RECURSOS];
        custo_atual(j, c, custo); /* o preço cresce com a quantidade */
        int pode = 1;
        for (int r = 0; r < N_RECURSOS; r++)
            if (j->recurso[r] < custo[r]) pode = 0;
        Cor fundo = c == ui->escolha ? PAINEL : FUNDO;
        tela_preenche(x + 1, yy, TELA_L - x - 2, 1, ' ', BRANCO, fundo);
        tela_textof(x + 2, yy, AMARELO, fundo, "%d", (int)c);
        tela_textof(x + 4, yy, c == ui->escolha ? BRANCO : CINZA, fundo,
                    "%-9s %c %3d", d->nome, d->letra, jogo_quantas(j, c));
        char txt[64] = "";
        int n = 0;
        for (int r = 0; r < N_RECURSOS; r++)
            if (custo[r] > 0)
                n += snprintf(txt + n, sizeof txt - (size_t)n, "%s%.0f %c",
                              n ? "  " : "", custo[r], NOME_RECURSO[r][0]);
        tela_texto(x + 22, yy, txt, pode ? CINZA : VERMELHO, fundo);
    }
    tela_texto(x + 2, y + 5, "custos: C comida  M madeira  P pedra",
               CINZA_ESCURO, FUNDO);
    const DadosConstrucao *d = &CONSTRUCOES[ui->escolha];
    tela_textof(x + 2, y + 6, CINZA, FUNDO, "Fica em: %s",
                NOME_TERRENO[d->terreno]);
    tela_quebra(x + 2, y + 7, TELA_L - x - 4, 2, d->descricao, CINZA, FUNDO);
}

void desenha(const Jogo *j, const Interface *ui) {
    Taxas t;
    jogo_taxas(j, &t);
    tela_limpa(FUNDO);
    /* o cabeçalho */
    tela_preenche(0, 0, TELA_L, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 0, "◆ POSTO AVANÇADO", AMARELO, PAINEL);
    double minutos = j->relogio + 7 * 60; /* um segundo é um minuto do posto */
    tela_textof(20, 0, CINZA, PAINEL, "Dia %d, %02d:%02d",
                (int)(minutos / 1440) + 1, (int)(minutos / 60) % 24,
                (int)minutos % 60);
    if (ui->velocidade > 1)
        tela_textof(36, 0, LARANJA, PAINEL, "vel. %dx", ui->velocidade);
    char p[24];
    fmt_num(p, sizeof p, jogo_pontos(j));
    tela_texto_dir(TELA_L - 2, 0, p, BRANCO, PAINEL);
    tela_texto_dir(TELA_L - 3 - utf8_largura(p), 0, "Pontos", CINZA, PAINEL);

    desenha_mapa(j, ui);
    desenha_recursos(j, &t);
    desenha_quadra(j, ui);
    desenha_menu(j, ui);
    tela_caixa(PAINEL_X, 22, TELA_L - PAINEL_X, 7, CINZA_ESCURO, FUNDO,
               "Diário");
    diario_desenha(&ui->diario, PAINEL_X + 2, 23, TELA_L - PAINEL_X - 4, 5);
    /* a legenda e as teclas */
    tela_texto(1, 26, "~ água  . planície  ♣ floresta  ^ montanha", CINZA,
               FUNDO);
    tela_texto(1, 27, "C casa  F fazenda  S serraria  P pedreira", CINZA,
               FUNDO);
    tela_texto(1, 29,
               "setas: mover   1-4: escolher   Enter: construir   D: demolir   "
               "V: velocidade   Esc: sair",
               CINZA_ESCURO, FUNDO);
}

int trata_tecla(Jogo *j, Interface *ui, int tecla) {
    switch (tecla) {
    case TECLA_CIMA: ui->cy = (ui->cy + MAPA_A - 1) % MAPA_A; break;
    case TECLA_BAIXO: ui->cy = (ui->cy + 1) % MAPA_A; break;
    case TECLA_ESQUERDA: ui->cx = (ui->cx + MAPA_L - 1) % MAPA_L; break;
    case TECLA_DIREITA: ui->cx = (ui->cx + 1) % MAPA_L; break;
    case TECLA_ENTER:
    case ' ': {
        Resultado r = constroi(j, ui->cx, ui->cy, ui->escolha);
        if (r == OK)
            diario_poe(&ui->diario, "%s construída.",
                       CONSTRUCOES[ui->escolha].nome);
        else
            diario_poe(&ui->diario, "\001rNão dá\001d: %s.",
                       texto_resultado(r));
    } break;
    case 'd':
    case 'D':
        if (j->mapa[ui->cy][ui->cx].obra != NADA &&
            ui_confirma("Demolir",
                        "Demolir esta construção? Metade do custo volta."))
            demole(j, ui->cx, ui->cy);
        break;
    case 'v':
    case 'V':
        ui->velocidade = ui->velocidade == 1    ? 10
                         : ui->velocidade == 10 ? 100
                                                : 1;
        break;
    case TECLA_ESC:
    case TECLA_SAIR:
        if (tecla == TECLA_SAIR ||
            ui_confirma("Sair", "Salvar e sair do jogo?"))
            return 0;
        break;
    default:
        if (tecla >= '1' && tecla < '1' + N_CONSTRUCOES - 1)
            ui->escolha = (Construcao)(tecla - '0');
        /* SEU JOGO: novas teclas entram aqui */
        break;
    }
    return 1;
}
