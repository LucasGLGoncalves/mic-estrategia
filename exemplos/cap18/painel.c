/* painel.c - o primeiro painel de verdade: a tela virtual do motor em ação */
#include "../../motor/tela.h"
#include "../../motor/term.h"

typedef struct {
    const char *nome;
    double estoque, capacidade, por_hora;
    Cor cor;
} Recurso;

static void desenha(double t) {
    Recurso rec[4] = {
        {"Metal", 15486, 40000, 11457, CINZA},
        {"Cristal", 5427, 40000, 6258, CIANO},
        {"Deutério", 3332, 10000, 1519, VERDE},
        {"Alimento", 35724, 50000, 2184, AMARELO},
    };
    tela_limpa(FUNDO);

    /* barra de título */
    tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 0, "✦ COLÔNIA KEPLER", AMARELO, PAINEL);
    tela_texto_dir(98, 0, "Ano 2387 · dia 14", CINZA, PAINEL);

    /* recursos, com barras que mostram quanto do armazém está ocupado */
    tela_caixa(1, 2, 48, 11, CINZA_ESCURO, FUNDO, "Recursos");
    for (int i = 0; i < 4; i++) {
        int y = 4 + i * 2;
        double fracao = rec[i].estoque / rec[i].capacidade;
        tela_texto(3, y, rec[i].nome, rec[i].cor, FUNDO);
        tela_textof(13, y, BRANCO, FUNDO, "%8.0f", rec[i].estoque);
        tela_barra(23, y, 14, fracao, rec[i].cor, RGB(30, 34, 48));
        tela_textof(39, y, fracao > 0.8 ? LARANJA : CINZA, FUNDO, "%3.0f%%",
                    fracao * 100);
        tela_textof(13, y + 1, VERDE, FUNDO, "%+7.0f/h", rec[i].por_hora);
    }

    /* um texto longo, com quebra de linha e cores no meio */
    tela_caixa(51, 2, 48, 11, CINZA_ESCURO, FUNDO, "Relatório do conselho");
    tela_quebra(
        53, 4, 44, 8,
        "A produção de \001ometal\001d subiu \001g12%\001d desde a nova mina. "
        "O armazém de \001ycomida\001d está a \001o71%\001d da capacidade, e a "
        "colheita de amanhã pode não caber. Sensores detectaram uma "
        "\001rfrota desconhecida\001d a 3 sistemas daqui. Recomendamos "
        "reforçar "
        "as defesas antes do fim da semana.",
        BRANCO, FUNDO);

    /* o sistema solar, desenhado com caracteres */
    tela_caixa(1, 14, 98, 9, CINZA_ESCURO, FUNDO, "Sistema Kepler");
    tela_poe(4, 18, 0x2600, AMARELO, FUNDO); /* ☀ */
    const Cor cores[6] = {LARANJA, MARROM, AZUL, CIANO, VERMELHO, MAGENTA};
    const char *nomes[6] = {"Brasa", "Duna",     "Kepler",
                            "Gelo",  "Ferrugem", "Colosso"};
    for (int i = 0; i < 6; i++) {
        int x = 12 + i * 14;
        int y = 18 + (i % 2 ? -1 : 1);
        tela_poe(x, y, 0x25CF, cores[i], FUNDO); /* ● */
        tela_texto(x - 2, y + (i % 2 ? -1 : 1), nomes[i],
                   i == 2 ? AMARELO : CINZA, FUNDO);
    }
    /* uma nave que se move com o tempo */
    int nx = 12 + (int)(t * 6) % 70;
    tela_texto(nx, 21, "»═>", BRANCO, FUNDO);

    tela_preenche(0, 24, 100, 1, ' ', BRANCO, PAINEL);
    tela_texto(1, 24, "Qualquer tecla sai", CINZA, PAINEL);
}

int main(void) {
    if (!term_inicia()) return 1;
    if (!tela_inicia(100, 25)) return 1;
    double inicio = term_agora();
    for (;;) {
        desenha(term_agora() - inicio);
        tela_mostra();
        int t = term_espera_tecla(100);
        if (t != 0) break;
    }
    tela_encerra();
    term_encerra();
    return 0;
}
