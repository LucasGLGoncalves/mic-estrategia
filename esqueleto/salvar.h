/* salvar.h - grava e lê o posto num arquivo de texto (capítulos 14 e 42) */
#ifndef SALVAR_H
#define SALVAR_H

#include <time.h>

#include "jogo.h"

#define ARQUIVO_SAVE "posto.sav"

int salva_jogo(const Jogo *j, const char *arquivo);
/* Devolve 1 se leu um jogo válido; 'quando' recebe a hora em que foi salvo. */
int carrega_jogo(Jogo *j, const char *arquivo, time_t *quando);

#endif
