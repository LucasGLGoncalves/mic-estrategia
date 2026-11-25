/* viagem.h - viajar entre planetas, e o que pode acontecer no caminho */
#ifndef VIAGEM_H
#define VIAGEM_H

#include "mundo.h"

/* Quem chama a viagem decide como desenhá-la: viaja() recebe um ponteiro para a
   função que pinta a cena, com o progresso de 0 (partida) a 1 (chegada). */
typedef void (*DesenhoViagem)(const Jogo *j, int origem, int destino,
                              double progresso);

/* Viaja até o destino, dia a dia, resolvendo os eventos (alguns perguntam ao
   jogador). desenha pode ser NULL: a viagem acontece sem animação. */
void viaja(Jogo *j, int destino, DesenhoViagem desenha);

#endif
