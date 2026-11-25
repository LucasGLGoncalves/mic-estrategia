/* salvar.h - a colônia gravada em disco, com a hora em que foi gravada */
#ifndef SALVAR_H
#define SALVAR_H

#include <time.h>

#include "colonia.h"

#define ARQUIVO_SAVE "colonia.sav"

int salva_colonia(const Colonia *c, const char *arquivo);
/* Carrega e devolve, em *quando, a hora do relógio do computador em que o
   jogo foi salvo: com ela se calcula o progresso offline. */
int carrega_colonia(Colonia *c, const char *arquivo, time_t *quando);

#endif
