/* fila.c - uma fila de construção que cresce conforme a necessidade */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char nome[32];
    double horas; /* quanto falta para terminar */
} Ordem;

typedef struct {
    Ordem *itens;   /* memória pedida ao sistema */
    int quantidade; /* quantas ordens há na fila */
    int capacidade; /* quantas cabem antes de precisar crescer */
} Fila;

void fila_inicia(Fila *f) {
    f->itens = NULL;
    f->quantidade = 0;
    f->capacidade = 0;
}

/* Acrescenta uma ordem no fim. Devolve 0 se faltou memória. */
int fila_poe(Fila *f, const char *nome, double horas) {
    if (f->quantidade == f->capacidade) {
        int nova = f->capacidade == 0 ? 2 : f->capacidade * 2;
        Ordem *maior = realloc(f->itens, (size_t)nova * sizeof(Ordem));
        if (maior == NULL) { return 0; /* o bloco antigo continua válido */ }
        printf("  (a fila cresceu de %d para %d lugares)\n", f->capacidade,
               nova);
        f->itens = maior;
        f->capacidade = nova;
    }
    Ordem *o = &f->itens[f->quantidade];
    snprintf(o->nome, sizeof o->nome, "%s", nome);
    o->horas = horas;
    f->quantidade++;
    return 1;
}

/* Remove a primeira ordem, empurrando as outras uma posição para a frente. */
void fila_tira_primeira(Fila *f) {
    if (f->quantidade == 0) return;
    memmove(&f->itens[0], &f->itens[1],
            (size_t)(f->quantidade - 1) * sizeof(Ordem));
    f->quantidade--;
}

void fila_libera(Fila *f) {
    free(f->itens);
    fila_inicia(f);
}

void fila_mostra(const Fila *f) {
    for (int i = 0; i < f->quantidade; i++) {
        printf("  %d. %-22s %5.1f h\n", i + 1, f->itens[i].nome,
               f->itens[i].horas);
    }
}

int main(void) {
    Fila fila;
    fila_inicia(&fila);

    const char *pedidos[] = {"Mina de metal 2",   "Usina solar 2",
                             "Mina de cristal 1", "Fazenda 2",
                             "Silo de metal 1",   "Estaleiro 1"};
    double horas[] = {1.5, 2.0, 1.0, 3.5, 2.5, 4.0};
    for (int i = 0; i < 6; i++) {
        printf("Pedido: %s\n", pedidos[i]);
        if (!fila_poe(&fila, pedidos[i], horas[i])) {
            printf("Sem memória!\n");
            return 1;
        }
    }
    printf("\nFila de construção:\n");
    fila_mostra(&fila);

    /* o tempo passa, de hora em hora, e a primeira ordem avança */
    for (int hora = 1; hora <= 6 && fila.quantidade > 0; hora++) {
        fila.itens[0].horas -= 1.0;
        while (fila.quantidade > 0 && fila.itens[0].horas <= 0) {
            double sobra = -fila.itens[0].horas; /* tempo que passou do ponto */
            printf("Hora %d: %s concluída.\n", hora, fila.itens[0].nome);
            fila_tira_primeira(&fila);
            if (fila.quantidade > 0) fila.itens[0].horas -= sobra;
        }
    }
    printf("\nAinda na fila:\n");
    fila_mostra(&fila);
    fila_libera(&fila);
    return 0;
}
