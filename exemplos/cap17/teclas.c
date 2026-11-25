/* teclas.c - o teclado em tempo real: cada tecla aparece no instante em que
   é apertada */
#include <stdio.h>
#include <string.h>

#include "../../motor/term.h"

static const char *nome_especial(int t) {
    switch (t) {
    case TECLA_CIMA: return "seta para cima";
    case TECLA_BAIXO: return "seta para baixo";
    case TECLA_ESQUERDA: return "seta para a esquerda";
    case TECLA_DIREITA: return "seta para a direita";
    case TECLA_ENTER: return "Enter";
    case TECLA_TAB: return "Tab";
    case TECLA_APAGA: return "Backspace";
    case TECLA_PGUP: return "Page Up";
    case TECLA_PGDN: return "Page Down";
    case TECLA_HOME: return "Home";
    case TECLA_END: return "End";
    case TECLA_DELETE: return "Delete";
    case TECLA_ESC: return "Esc";
    default: return NULL;
    }
}

/* Escreve um texto na linha e coluna pedidas, apagando o resto da linha. */
static void escreve_em(int linha, int coluna, const char *texto) {
    char buf[256];
    int n =
        snprintf(buf, sizeof buf, "\x1b[%d;%dH%s\x1b[K", linha, coluna, texto);
    term_escreve(buf, (size_t)n);
}

int main(void) {
    if (!term_inicia()) return 1;
    double inicio = term_agora();
    int teclas = 0;
    char linha[200];

    escreve_em(2, 3, "\x1b[1;33mO TECLADO EM TEMPO REAL\x1b[0m");
    escreve_em(4, 3,
               "Aperte qualquer tecla (setas, letras, acentos...). Esc sai.");
    for (;;) {
        /* espera no máximo 50 ms por uma tecla */
        int t = term_espera_tecla(50);
        if (t == TECLA_ESC || t == TECLA_SAIR) break;
        if (t != 0) {
            teclas++;
            const char *nome = nome_especial(t);
            if (nome != NULL) {
                snprintf(linha, sizeof linha,
                         "Última tecla: \x1b[36m%s\x1b[0m (código %d)", nome,
                         t);
            } else {
                char letra[8] = "";
                if (t < 0x80) {
                    letra[0] = (char)t;
                } else {
                    /* acentos: o código Unicode volta a ser UTF-8 para a
                       tela */
                    if (t < 0x800) {
                        letra[0] = (char)(0xC0 | (t >> 6));
                        letra[1] = (char)(0x80 | (t & 0x3F));
                    } else {
                        letra[0] = (char)(0xE0 | (t >> 12));
                        letra[1] = (char)(0x80 | ((t >> 6) & 0x3F));
                        letra[2] = (char)(0x80 | (t & 0x3F));
                    }
                }
                snprintf(linha, sizeof linha,
                         "Última tecla: \x1b[36m'%s'\x1b[0m (código %d)", letra,
                         t);
            }
            escreve_em(7, 3, linha);
        }
        /* o relógio continua andando mesmo sem nenhuma tecla: isso é tempo
           real */
        int col, lin;
        term_tamanho(&col, &lin);
        snprintf(linha, sizeof linha,
                 "Tempo: %6.1f s    Teclas: %d    Janela: %dx%d",
                 term_agora() - inicio, teclas, col, lin);
        escreve_em(9, 3, linha);
        term_descarrega();
    }
    term_encerra();
    printf("Você apertou %d teclas em %.1f segundos.\n", teclas,
           term_agora() - inicio);
    return 0;
}
