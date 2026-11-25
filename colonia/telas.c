/* telas.c - o cabeçalho, a paisagem, os recursos, as listas e a fila */
#include "telas.h"

#include <stdio.h>
#include <string.h>

#include "../motor/tela.h"
#include "paisagem.h"

static void cabecalho(const Colonia *c, const Interface *ui) {
    tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 0, "◆ COLÔNIA", AMARELO, PAINEL);
    /* um segundo real é um minuto de jogo; começa às 7h */
    double minutos = c->relogio + 7 * 60;
    int dia = (int)(minutos / 1440) + 1, hora = (int)(minutos / 60) % 24,
        min = (int)minutos % 60;
    tela_textof(14, 0, CINZA, PAINEL, "Dia %d, %02d:%02d", dia, hora, min);
    if (ui->velocidade > 1)
        tela_textof(28, 0, LARANJA, PAINEL, "vel. %dx", ui->velocidade);
    static const char *abas[N_ABAS] = {"1 Construções", "2 Pesquisas",
                                       "3 Diário"};
    ui_abas(39, 0, abas, N_ABAS, ui->aba);
    if (c->venceu) tela_texto(84, 0, "✓ Portal aberto", VERDE, PAINEL);
}

/* Uma taxa por segundo: com uma casa decimal quando é pequena. */
static void fmt_taxa(char *saida, size_t tam, double v) {
    if (v >= 100) {
        fmt_curto(saida, tam, v);
        return;
    }
    snprintf(saida, tam, "%.1f", v);
    for (char *p = saida; *p; p++)
        if (*p == '.') *p = ','; /* vírgula decimal, como no resto do jogo */
}

static void linha_recurso(int y, const char *nome, double valor, double taxa,
                          int cheio, Cor cor) {
    char v[24], t[24];
    fmt_num(v, sizeof v, valor);
    fmt_taxa(t, sizeof t, taxa < 0 ? -taxa : taxa);
    tela_texto(2, y, nome, cor, FUNDO);
    tela_texto_dir(18, y, v, BRANCO, FUNDO);
    if (cheio) tela_texto(20, y, "cheio!", VERMELHO, FUNDO);
    else
        tela_textof(20, y, taxa < 0 ? VERMELHO : VERDE, FUNDO, "%c%s/s",
                    taxa < 0 ? '-' : '+', t);
}

static void recursos(const Colonia *c, const Taxas *t) {
    tela_caixa(0, 11, 33, 13, CINZA_ESCURO, FUNDO, "Recursos");
    static const Cor COR[N_RECURSOS] = {RGB(190, 170, 150), CIANO, VERDE,
                                        MAGENTA};
    for (int r = 0; r < N_RECURSOS; r++)
        linha_recurso(12 + r, NOME_RECURSO[r], c->recurso[r], t->producao[r],
                      r != CIENCIA && c->recurso[r] >= t->estoque_max, COR[r]);
    char g[16], s[16];
    fmt_curto(g, sizeof g, t->energia_gerada);
    fmt_curto(s, sizeof s, t->energia_gasta);
    tela_texto(2, 17, "Energia", AMARELO, FUNDO);
    tela_textof(11, 17, BRANCO, FUNDO, "%s de %s", s, g);
    tela_barra(2, 18, 20, t->eficiencia_energia,
               t->eficiencia_energia < 1 ? VERMELHO : AMARELO, PAINEL);
    tela_textof(23, 18, t->eficiencia_energia < 1 ? VERMELHO : CINZA, FUNDO,
                "%3.0f%%", 100 * t->eficiencia_energia);
    tela_texto(2, 19, "Colonos", RGB(120, 180, 255), FUNDO);
    tela_textof(11, 19, BRANCO, FUNDO, "%.0f de %.0f", c->populacao,
                t->capacidade);
    tela_textof(2, 20, t->eficiencia_trabalho < 1 ? VERMELHO : CINZA, FUNDO,
                "Trabalho %d vagas %3.0f%%", t->trabalhadores,
                100 * t->eficiencia_trabalho);
    char cap[16];
    fmt_curto(cap, sizeof cap, t->estoque_max);
    tela_textof(2, 22, CINZA, FUNDO, "Armazém: até %s de cada", cap);
}

/* Quanto o próximo nível acrescenta: calcula as taxas de uma colônia
   imaginária. */
static void ganho_do_proximo(const Colonia *c, Estrutura e, char *saida,
                             size_t tam) {
    static Colonia teste;
    teste = *c;
    teste.nivel[e]++;
    Taxas antes = colonia_taxas(c), depois = colonia_taxas(&teste);
    char v[16];
    switch (e) {
    case USINA:
        fmt_curto(v, sizeof v, depois.energia_gerada - antes.energia_gerada);
        snprintf(saida, tam, "+%s de energia", v);
        break;
    case HABITACAO:
        snprintf(saida, tam, "+%.0f vagas para colonos",
                 depois.capacidade - antes.capacidade);
        break;
    case PORTAL: snprintf(saida, tam, "a vitória"); break;
    default: {
        static const char *nomes[N_RECURSOS] = {"metal", "cristal", "comida",
                                                "ciência"};
        Recurso r = e == MINA_METAL     ? METAL
                    : e == MINA_CRISTAL ? CRISTAL
                    : e == FAZENDA      ? COMIDA
                                        : CIENCIA;
        fmt_taxa(v, sizeof v, depois.producao[r] - antes.producao[r]);
        snprintf(saida, tam, "+%s de %s por segundo", v, nomes[r]);
    }
    }
}

static void lista_obras(const Colonia *c, const Interface *ui) {
    tela_texto(36, 12, "Estrutura              Nível    Metal  Cristal   Tempo",
               CINZA, FUNDO);
    for (int e = 0; e < N_ESTRUTURAS; e++) {
        int y = 13 + e, sel = e == ui->obras.sel,
            prox = nivel_planejado(c, e) + 1;
        const char *motivo;
        int ok = pode_construir(c, e, &motivo);
        Cor fundo = sel ? RGB(40, 60, 100) : FUNDO;
        tela_preenche(35, y, 63, 1, ' ', BRANCO, fundo);
        char nome[48], m[16], k[16], tp[16];
        double cm, ck;
        custo_de(e, prox, &cm, &ck);
        fmt_ajusta(nome, sizeof nome, ESTRUTURAS[e].nome, 22);
        fmt_curto(m, sizeof m, cm);
        fmt_curto(k, sizeof k, ck);
        fmt_tempo(tp, sizeof tp, duracao_de(c, e, prox));
        Cor cor = ok ? BRANCO : CINZA_ESCURO;
        if (e == PORTAL && !c->pesquisado[DOBRA]) cor = CINZA_ESCURO;
        tela_textof(36, y, cor, fundo, "%s %3d", nome, c->nivel[e]);
        if (e == PORTAL && c->nivel[PORTAL] > 0) {
            tela_texto(65, y, "concluído", VERDE, fundo);
            continue;
        }
        tela_textof(65, y, c->recurso[METAL] >= cm ? cor : VERMELHO, fundo,
                    "%7s", m);
        tela_textof(74, y, c->recurso[CRISTAL] >= ck ? cor : VERMELHO, fundo,
                    "%7s", k);
        tela_texto_dir(89, y, tp, cor, fundo);
    }
    Estrutura e = (Estrutura)ui->obras.sel;
    char ganho[64];
    const char *motivo;
    ganho_do_proximo(c, e, ganho, sizeof ganho);
    tela_textof(36, 21, CINZA, FUNDO, "%s: %s.", ESTRUTURAS[e].descricao,
                ganho);
    if (!pode_construir(c, e, &motivo))
        tela_textof(36, 22, VERMELHO, FUNDO, "Agora não: %s.", motivo);
    else tela_texto(36, 22, "Enter encomenda.", AMARELO, FUNDO);
}

static void lista_pesquisas(const Colonia *c, const Interface *ui) {
    tela_texto(36, 12, "Pesquisa                  Ciência   Efeito", CINZA,
               FUNDO);
    for (int p = 0; p < N_PESQUISAS; p++) {
        int y = 13 + p, sel = p == ui->pesquisas.sel;
        Cor fundo = sel ? RGB(40, 60, 100) : FUNDO;
        tela_preenche(35, y, 63, 1, ' ', BRANCO, fundo);
        const DadosPesquisa *d = &PESQUISAS[p];
        char nome[48];
        fmt_ajusta(nome, sizeof nome, d->nome, 24);
        Cor cor = c->pesquisado[p]       ? VERDE
                  : pode_pesquisar(c, p) ? BRANCO
                                         : CINZA_ESCURO;
        tela_textof(36, y, cor, fundo, "%s %7.0f   %s", nome, d->custo,
                    d->efeito);
        if (c->pesquisado[p]) tela_texto(97, y, "✓", VERDE, fundo);
    }
    const DadosPesquisa *d = &PESQUISAS[ui->pesquisas.sel];
    if (c->pesquisado[ui->pesquisas.sel])
        tela_texto(36, 22, "Já pesquisada.", VERDE, FUNDO);
    else if (!pode_pesquisar(c, ui->pesquisas.sel))
        tela_textof(36, 22, CINZA, FUNDO, "Requer %s.",
                    PESQUISAS[d->requer].nome);
    else if (c->recurso[CIENCIA] < d->custo)
        tela_textof(36, 22, VERMELHO, FUNDO, "Faltam %.0f de ciência.",
                    d->custo - c->recurso[CIENCIA]);
    else tela_texto(36, 22, "Enter pesquisa.", AMARELO, FUNDO);
}

static void fila(const Colonia *c) {
    tela_caixa(0, 24, 100, 5, CINZA_ESCURO, FUNDO, "Fila de obras");
    if (c->n_fila == 0) {
        tela_texto(
            2, 25,
            "Nenhuma obra em andamento. Escolha uma estrutura e aperte Enter.",
            CINZA, FUNDO);
        return;
    }
    double espera = 0;
    for (int i = 0; i < c->n_fila; i++) {
        const Obra *o = &c->fila[i];
        char nome[48], t[16];
        fmt_ajusta(nome, sizeof nome, ESTRUTURAS[o->estrutura].nome, 20);
        espera += o->duracao - (i == 0 ? c->trabalhado : 0);
        fmt_tempo(t, sizeof t, espera);
        if (i == 0) { /* a obra em andamento, com a barra de progresso */
            tela_textof(2, 25, BRANCO, FUNDO, "%s nível %-2d", nome, o->nivel);
            tela_barra(34, 25, 40, c->trabalhado / o->duracao, AZUL, PAINEL);
            tela_textof(76, 25, BRANCO, FUNDO, "falta %s", t);
        } else { /* as outras, em duas colunas */
            int x = i % 2 ? 2 : 52, y = 25 + (i + 1) / 2;
            tela_textof(x, y, CINZA, FUNDO, "%d. %s nível %-2d pronta em %s",
                        i + 1, nome, o->nivel, t);
        }
    }
}

void desenha(const Colonia *c, const Interface *ui, double agora) {
    tela_limpa(FUNDO);
    cabecalho(c, ui);
    paisagem_desenha(c, 1, agora);
    Taxas t = colonia_taxas(c);
    recursos(c, &t);
    static const char *titulos[N_ABAS] = {"Construções", "Pesquisas", "Diário"};
    tela_caixa(34, 11, 66, 13, CINZA_ESCURO, FUNDO, titulos[ui->aba]);
    if (ui->aba == ABA_OBRAS) lista_obras(c, ui);
    else if (ui->aba == ABA_PESQUISAS) lista_pesquisas(c, ui);
    else diario_desenha(&c->diario, 36, 12, 62, 11);
    fila(c);
    tela_preenche(0, 29, 100, 1, ' ', CINZA, PAINEL);
    tela_texto(1, 29,
               "1-3 abas  ↑↓ escolhe  Enter faz  X cancela a última obra  V "
               "velocidade  Esc sai",
               CINZA, PAINEL);
}
