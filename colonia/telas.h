/* telas.h - os painéis da colônia */
#ifndef TELAS_H
#define TELAS_H

#include "colonia.h"

typedef enum { ABA_OBRAS, ABA_PESQUISAS, ABA_DIARIO, N_ABAS } Aba;

typedef struct {
    Aba aba;
    Lista obras, pesquisas;
    int velocidade; /* 1, 10 ou 100 */
} Interface;

void desenha(const Colonia *c, const Interface *ui, double agora);

#endif
