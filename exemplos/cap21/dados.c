/* dados.c - o gerador do motor posto à prova: sementes, sequências e
   histogramas */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "../../motor/rng.h"
#include "../../motor/tela.h"
#include "../../motor/term.h"
#include "../../motor/ui.h"

#define ROLAGENS 20000

/* Desenha um histograma vertical com blocos de oito alturas diferentes. */
static void histograma(int x, int y, int altura, const int *contagem, int min,
                       int max, const char *titulo, Cor cor) {
    static const uint32_t blocos[9] = {' ',    0x2581, 0x2582, 0x2583, 0x2584,
                                       0x2585, 0x2586, 0x2587, 0x2588};
    int maior = 1;
    for (int v = min; v <= max; v++)
        if (contagem[v] > maior) maior = contagem[v];
    tela_texto(x, y - 1, titulo, AMARELO, FUNDO);
    for (int v = min; v <= max; v++) {
        int col = x + (v - min);
        /* altura da barra em oitavos */
        int oitavos = contagem[v] * altura * 8 / maior;
        for (int j = 0; j < altura; j++) {
            int k = oitavos - j * 8;
            uint32_t ch = k >= 8 ? blocos[8] : k > 0 ? blocos[k] : ' ';
            tela_poe(col, y + altura - 1 - j, ch, cor, FUNDO);
        }
        if (v == min || v == max || v % 5 == 0)
            tela_textof(col, y + altura, CINZA, FUNDO, "%d", v);
    }
}

int main(void) {
    if (!term_inicia() || !tela_inicia(100, 25)) return 1;
    char nome[40] = "Lucas";
    ui_le_texto("Semente", "Digite um nome para gerar o universo:", nome,
                sizeof nome);

    for (;;) {
        uint64_t semente = hash_texto(nome);
        Rng r;
        rng_semente(&r, semente);
        int d20[21] = {0}, tres_d6[19] = {0}, dois_d10[21] = {0}, sucessos = 0;
        for (int i = 0; i < ROLAGENS; i++) {
            int dado = rng_dado(&r, 20);
            d20[dado]++;
            tres_d6[rng_dados(&r, 3, 6)]++;
            dois_d10[rng_dados(&r, 2, 10)]++;
            if (dado + 4 >= 13) sucessos++;
        }

        tela_limpa(FUNDO);
        tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
        tela_texto(1, 0, "ACASO SOB CONTROLE", AMARELO, PAINEL);
        tela_textof(2, 2, BRANCO, FUNDO,
                    "Nome: \"%s\"   semente: 0x%016" PRIX64, nome, semente);
        rng_semente(&r, semente);
        char linha[200] = "Primeiros números (1 a 100):";
        for (int i = 0; i < 12; i++) {
            char n[8];
            snprintf(n, sizeof n, " %d", rng_faixa(&r, 1, 100));
            strcat(linha, n);
        }
        tela_texto(2, 3, linha, CIANO, FUNDO);

        histograma(4, 7, 10, d20, 1, 20, "1d20: todos iguais", VERDE);
        histograma(34, 7, 10, tres_d6, 3, 18, "3d6: curva de sino", LARANJA);
        histograma(62, 7, 10, dois_d10, 2, 20, "2d10: um triângulo", MAGENTA);
        tela_textof(2, 20, BRANCO, FUNDO,
                    "Teste d20 + 4 contra CD 13: %.1f%% de sucesso em %d "
                    "rolagens (a conta dá 60%%).",
                    100.0 * sucessos / ROLAGENS, ROLAGENS);
        tela_texto(2, 23, "N outro nome   Esc sai", CINZA, FUNDO);
        tela_mostra();

        int t = term_espera_tecla(1000);
        while (t == 0) t = term_espera_tecla(1000);
        if (t == TECLA_ESC || t == TECLA_SAIR) break;
        if (t == 'n' || t == 'N')
            ui_le_texto("Semente", "Outro nome:", nome, sizeof nome);
    }
    tela_encerra();
    term_encerra();
    return 0;
}
