/* sistema.c - um sistema solar descrito com estruturas, enums e uma tabela */
#include <stdio.h>

typedef enum { ROCHOSO, OCEANICO, GASOSO, GELADO, TIPOS } TipoPlaneta;

const char *NOME_TIPO[TIPOS] = {"rochoso", "oceânico", "gasoso", "gelado"};

typedef struct {
    const char *nome;
    int posicao;     /* distância ao sol, em ordem */
    int temperatura; /* média, em graus Celsius */
    int campos;      /* espaço para construir */
    TipoPlaneta tipo;
} Planeta;

/* Habitável: nem quente nem frio demais, e com chão firme. */
int habitavel(Planeta p) {
    return p.tipo != GASOSO && p.temperatura > -50 && p.temperatura < 60;
}

int main(void) {
    Planeta sistema[] = {
        {"Brasa", 1, 310, 95, ROCHOSO},
        {"Aurora", 3, 22, 163, OCEANICO},
        {.nome = "Kepler",
         .posicao = 4,
         .temperatura = -8,
         .campos = 188,
         .tipo = ROCHOSO},
        {"Titã Menor", 7, -140, 120, GELADO},
        {"Colosso", 9, -180, 260, GASOSO},
    };
    int n = sizeof sistema / sizeof sistema[0];

    printf("Pos  Campos   Temp.  Planeta (tipo)\n");
    int melhor = -1; /* nenhum ainda */
    for (int i = 0; i < n; i++) {
        Planeta p = sistema[i];
        printf("%3d  %6d  %5d  %s (%s)%s\n", p.posicao, p.campos, p.temperatura,
               p.nome, NOME_TIPO[p.tipo], habitavel(p) ? " *" : "");
        if (habitavel(p) && (melhor < 0 || p.campos > sistema[melhor].campos)) {
            melhor = i;
        }
    }
    if (melhor >= 0) {
        printf("\nMelhor lugar para uma colônia: %s, com %d campos.\n",
               sistema[melhor].nome, sistema[melhor].campos);
    }

    Planeta copia = sistema[2]; /* estruturas são copiadas inteiras */
    copia.campos = 999;
    printf("A cópia tem %d campos; o original continua com %d.\n", copia.campos,
           sistema[2].campos);
    return 0;
}
