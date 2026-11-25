/* eventos.c - os acontecimentos que o destino reserva ao reino */
#include "eventos.h"

#include <stdio.h>

#include "entrada.h"

const Evento EVENTOS[EV_QUANTOS] = {
    [EV_FARTURA] = {"fartura", 10,
                    "Os deuses abençoaram os campos: a colheita foi farta."},
    [EV_RATOS] = {"ratos", 40, "Os ratos invadiram o celeiro."},
    [EV_PESTE] = {"peste", 15,
                  "Uma peste terrível varreu a cidade: metade do povo morreu."},
};

int evento_acontece(TipoEvento t) {
    return sorteia(1, 100) <= EVENTOS[t].chance;
}

void evento_aplica(Reino *r, TipoEvento t) {
    printf("  >> %s\n", EVENTOS[t].anuncio);
    switch (t) {
    case EV_PESTE:
        r->mortos_peste = r->populacao / 2;
        r->populacao -= r->mortos_peste;
        r->total_mortos += r->mortos_peste;
        break;
    case EV_RATOS:
        r->ratos = r->graos / sorteia(3, 6);
        r->graos -= r->ratos;
        break;
    case EV_FARTURA: r->colheita += 2; break;
    case EV_QUANTOS: break;
    }
}
