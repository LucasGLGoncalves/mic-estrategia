/* desenho.h - a tela da Conquista */
#ifndef DESENHO_H
#define DESENHO_H

#include "jogo.h"

typedef enum { MODO_NORMAL, MODO_MIRA } Modo;

typedef struct {
    int sel;    /* unidade selecionada, ou -1 */
    int cidade; /* cidade selecionada, ou -1 */
    Modo modo;  /* na mira, as setas movem um cursor para a ordem "vai até" */
    int cx, cy; /* o cursor da mira */
    int revela; /* mostra o mapa inteiro, sem névoa (para testes) */
    int vez_da_ia;
} Interface;

void desenha(const Jogo *j, const Interface *ui, double agora);

#endif
