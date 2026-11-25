/* eventos.h - os acontecimentos que o destino reserva ao reino */
#ifndef EVENTOS_H
#define EVENTOS_H

#include "reino.h"

typedef enum { EV_FARTURA, EV_RATOS, EV_PESTE, EV_QUANTOS } TipoEvento;

typedef struct {
    const char *nome;
    int chance; /* em porcentagem, por ano */
    const char *anuncio;
} Evento;

extern const Evento EVENTOS[EV_QUANTOS];

int evento_acontece(TipoEvento t);
void evento_aplica(Reino *r, TipoEvento t);

#endif
