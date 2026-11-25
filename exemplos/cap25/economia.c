/* economia.c - a matemática dos jogos de estratégia, em gráficos de
   caracteres */
#include <math.h>
#include <stdio.h>

#include "../../motor/tela.h"
#include "../../motor/term.h"
#include "../../motor/ui.h"

#define N 60 /* pontos em cada série */

typedef struct {
    const char *nome;
    double v[N];
    Cor cor;
} Serie;

/* Desenha várias séries num mesmo gráfico; com 'log', o eixo vertical é
   logarítmico. */
static void grafico(int x, int y, int l, int a, const Serie *s, int ns,
                    int log_y, const char *rotulo_x) {
    double lo = 1e300, hi = -1e300;
    for (int k = 0; k < ns; k++)
        for (int i = 0; i < N; i++) {
            double v = log_y ? log10(fmax(s[k].v[i], 1e-9)) : s[k].v[i];
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }
    if (hi - lo < 1e-9) hi = lo + 1;
    /* eixo │ */
    for (int j = 0; j < a; j++)
        tela_poe(x + 9, y + j, 0x2502, CINZA_ESCURO, FUNDO);
    for (int i = 0; i < l - 10; i++)
        tela_poe(x + 10 + i, y + a, 0x2500, CINZA_ESCURO, FUNDO);
    for (int j = 0; j <= 4; j++) { /* cinco marcas no eixo vertical */
        double v = lo + (hi - lo) * j / 4.0;
        char b[24];
        fmt_curto(b, sizeof b, log_y ? pow(10, v) : v);
        tela_texto_dir(x + 8, y + a - 1 - (a - 1) * j / 4, b, CINZA, FUNDO);
    }
    tela_texto_dir(x + l - 1, y + a + 1, rotulo_x, CINZA, FUNDO);
    int lx = x + 12; /* a legenda, uma série depois da outra */
    for (int k = 0; k < ns; k++) {
        for (int i = 0; i < N; i++) {
            double v = log_y ? log10(fmax(s[k].v[i], 1e-9)) : s[k].v[i];
            int px = x + 10 + i * (l - 11) / (N - 1);
            int py = y + a - 1 - (int)((v - lo) / (hi - lo) * (a - 1) + 0.5);
            tela_poe(px, py, 0x2022, s[k].cor, FUNDO); /* • */
        }
        lx += tela_textof(lx, y - 1, s[k].cor, FUNDO, "• %s", s[k].nome) + 4;
    }
}

static void custos(void) {
    Serie s[3] = {{"custo do nível", {0}, LARANJA},
                  {"produção por hora", {0}, VERDE},
                  {"horas para se pagar", {0}, CIANO}};
    for (int i = 0; i < N; i++) {
        int nivel = 1 + i / 2;
        s[0].v[i] = 60 * pow(1.5, nivel - 1);
        s[1].v[i] = 30 * nivel * pow(1.1, nivel);
        s[2].v[i] =
            s[0].v[i] / (s[1].v[i] - 30 * (nivel - 1) * pow(1.1, nivel - 1));
    }
    grafico(1, 5, 98, 15, s, 3, 1,
            "nível da mina: 1 a 30 (escala logarítmica)");
    tela_quebra(2, 22, 96, 2,
                "O custo (\001olaranja\001d) cresce 50% por nível; a produção "
                "(\001gverde\001d), bem menos. Por isso cada melhoria demora "
                "mais a se pagar (\001cciano\001d).",
                BRANCO, FUNDO);
}

static void populacao(void) {
    Serie s[3] = {{"crescimento exponencial", {0}, VERMELHO},
                  {"logístico (limite: 10 mil)", {0}, VERDE},
                  {"comida disponível", {0}, AMARELO}};
    double pe = 500, pl = 500;
    for (int i = 0; i < N; i++) {
        s[0].v[i] = pe;
        s[1].v[i] = pl;
        s[2].v[i] = 10000;
        pe *= 1.08; /* sempre 8% a mais */
        /* 8% enquanto há espaço; zero no limite */
        pl += 0.08 * pl * (1 - pl / 10000);
    }
    grafico(1, 5, 98, 15, s, 3, 1, "anos: 0 a 59 (escala logarítmica)");
    tela_quebra(2, 22, 96, 2,
                "Sem limite, a população explode (\001rvermelho\001d; na "
                "escala logarítmica, uma reta). Com a comida como teto, ela "
                "desacelera e se acomoda (\001gverde\001d): a curva logística.",
                BRANCO, FUNDO);
}

static void mercado(void) {
    Serie s[2] = {{"preço do cristal", {0}, CIANO},
                  {"estoque do mercado", {0}, CINZA}};
    double estoque = 1000, ideal = 1000;
    for (int i = 0; i < N; i++) {
        /* o jogador despeja cristal */
        if (i == 10 || i == 30 || i == 31) estoque += 900;
        /* e depois compra um lote */
        if (i == 45) estoque -= 600;
        double preco = 2.0 * pow(ideal / estoque, 0.8); /* oferta e procura */
        s[0].v[i] = preco;
        s[1].v[i] = estoque / 500;
        /* o mercado volta devagar ao normal */
        estoque += (ideal - estoque) * 0.12;
    }
    grafico(1, 5, 98, 15, s, 2, 0, "dias: 0 a 59");
    tela_quebra(2, 22, 96, 2,
                "Vender muito de uma vez derruba o preço; o mercado se "
                "recupera aos poucos. Vender em lotes, esperando, rende mais.",
                BRANCO, FUNDO);
}

/* Duas estratégias jogando a mesma economia: a simulação diz qual é melhor. */
static void simulacao(void) {
    Serie s[2] = {{"compra o mais barato", {0}, LARANJA},
                  {"compra o que se paga antes", {0}, VERDE}};
    for (int estrategia = 0; estrategia < 2; estrategia++) {
        double metal = 50, prod[3] = {1, 0, 0}, custo[3] = {15, 120, 1300},
               ganho[3] = {1, 9, 70};
        int qtd[3] = {1, 0, 0};
        for (int seg = 0; seg < 3600; seg++) {
            metal += prod[0] + prod[1] + prod[2];
            int escolha = 0;
            for (int k = 1; k < 3; k++) {
                if (estrategia == 0 && custo[k] < custo[escolha]) escolha = k;
                if (estrategia == 1 &&
                    custo[k] / ganho[k] < custo[escolha] / ganho[escolha])
                    escolha = k;
            }
            if (metal >= custo[escolha]) {
                metal -= custo[escolha];
                prod[escolha] += ganho[escolha];
                qtd[escolha]++;
                custo[escolha] *= 1.15;
            }
            if (seg % 60 == 0)
                s[estrategia].v[seg / 60] = prod[0] + prod[1] + prod[2];
        }
    }
    grafico(1, 5, 98, 15, s, 2, 1,
            "minutos de jogo: 0 a 59 (produção por segundo, escala log.)");
    tela_quebra(2, 22, 96, 2,
                "Um robô joga uma hora em milissegundos. Comparar estratégias "
                "assim mostra se há uma jogada óbvia demais.",
                BRANCO, FUNDO);
}

int main(void) {
    if (!term_inicia() || !tela_inicia(100, 25)) return 1;
    const char *abas[] = {"1 Custos", "2 População", "3 Mercado",
                          "4 Simulação"};
    int aba = 0;
    for (;;) {
        tela_limpa(FUNDO);
        tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
        tela_texto(1, 0, "ECONOMIA, CRESCIMENTO E EQUILÍBRIO", AMARELO, PAINEL);
        ui_abas(1, 2, abas, 4, aba);
        if (aba == 0) custos();
        if (aba == 1) populacao();
        if (aba == 2) mercado();
        if (aba == 3) simulacao();
        tela_mostra();
        int t = term_espera_tecla(1000);
        if (t == TECLA_ESC || t == TECLA_SAIR) break;
        if (t >= '1' && t <= '4') aba = t - '1';
        if (t == TECLA_TAB) aba = (aba + 1) % 4;
    }
    tela_encerra();
    term_encerra();
    return 0;
}
