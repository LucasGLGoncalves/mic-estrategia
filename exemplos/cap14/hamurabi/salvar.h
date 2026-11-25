/* salvar.h - jogo salvo e quadro de honra */
#ifndef SALVAR_H
#define SALVAR_H

#include "reino.h"

#define ARQUIVO_SAVE "hamurabi.sav"
#define ARQUIVO_RECORDES "recordes.txt"

int salva_reino(const Reino *r, const char *arquivo);
int carrega_reino(Reino *r, const char *arquivo);

void registra_recorde(const char *nome, int pontos, const char *arquivo);
void mostra_recordes(const char *arquivo, int quantos);

#endif
