/* ia.h - o general do computador */
#ifndef IA_H
#define IA_H

#include "jogo.h"

/* Chamada depois de cada ação que o jogador consegue ver, para a tela
   acompanhar. */
typedef void (*MostraAcao)(const Jogo *j);

/* Joga o turno inteiro do lado 'eu': produção e todas as unidades.
   Ele usa as mesmas regras e a mesma névoa de guerra que o jogador. */
void ia_joga(Jogo *j, Dono eu, MostraAcao mostra);

#endif
