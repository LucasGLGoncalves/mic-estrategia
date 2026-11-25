/* term.h - a camada de terminal do motor: modo cru, teclado, tempo e saída.
   Funciona no Windows 10+ (console com sequências VT) e em sistemas POSIX
   (Linux, macOS, WSL). O resto do jogo não precisa saber em qual. */
#ifndef TERM_H
#define TERM_H

#include <stddef.h>

/* Teclas especiais. As teclas comuns chegam como o código Unicode do caractere
   ('a', 'Z', '7', 'ç'...); as especiais usam números acima do Unicode. */
enum {
    TECLA_ENTER = 13,
    TECLA_ESC = 27,
    TECLA_TAB = 9,
    TECLA_APAGA = 8, /* backspace */
    TECLA_CIMA = 0x110000,
    TECLA_BAIXO,
    TECLA_ESQUERDA,
    TECLA_DIREITA,
    TECLA_PGUP,
    TECLA_PGDN,
    TECLA_HOME,
    TECLA_END,
    TECLA_INSERT,
    TECLA_DELETE,
    TECLA_F1,
    TECLA_F2,
    TECLA_F3,
    TECLA_F4,
    TECLA_F5,
    TECLA_F6,
    TECLA_F7,
    TECLA_F8,
    TECLA_F9,
    TECLA_F10,
    TECLA_SAIR /* o jogador fechou a janela ou apertou Ctrl+C */
};

/* Prepara o terminal para o jogo. Devolve 0 se não for possível. */
int term_inicia(void);
/* Devolve o terminal ao normal. É chamada sozinha na saída do programa. */
void term_encerra(void);

/* Devolve a próxima tecla apertada, ou 0 se não houver nenhuma (não espera). */
int term_tecla(void);
/* Espera até 'ms' milissegundos por uma tecla. Devolve a tecla, ou 0. */
int term_espera_tecla(int ms);

/* Tamanho atual da janela do terminal, em colunas e linhas. */
void term_tamanho(int *colunas, int *linhas);

/* Relógio que só anda para a frente, em segundos (com frações). */
double term_agora(void);
/* Dorme 'ms' milissegundos. */
void term_dorme(int ms);

/* Escreve bytes no terminal (guardados num buffer até term_descarrega). */
void term_escreve(const char *s, size_t n);
void term_descarrega(void);

#endif
