/* salvar.h - o diário de bordo gravado em disco */
#ifndef SALVAR_H
#define SALVAR_H

#include "mundo.h"

#define ARQUIVO_SAVE "mercador.sav"

int salva_jogo(const Jogo *j, const char *arquivo);
int carrega_jogo(Jogo *j, const char *arquivo);

#endif
