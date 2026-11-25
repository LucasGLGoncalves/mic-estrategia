/* salvar.c - grava e lê a colônia num arquivo de texto, chave e valor */
#include "salvar.h"

#include <stdio.h>
#include <string.h>

int salva_colonia(const Colonia *c, const char *arquivo) {
    char tmp[256];
    snprintf(tmp, sizeof tmp, "%s.tmp", arquivo);
    FILE *f = fopen(tmp, "w");
    if (f == NULL) return 0;
    fprintf(f, "COLONIA-SAVE 1\n");
    /* a hora do mundo real */
    fprintf(f, "tempo %lld\n", (long long)time(NULL));
    fprintf(f, "relogio %.3f\npopulacao %.6f\nvenceu %d\n", c->relogio,
            c->populacao, c->venceu);
    for (int r = 0; r < N_RECURSOS; r++)
        fprintf(f, "recurso %d %.6f\n", r, c->recurso[r]);
    for (int e = 0; e < N_ESTRUTURAS; e++)
        fprintf(f, "nivel %d %d\n", e, c->nivel[e]);
    for (int p = 0; p < N_PESQUISAS; p++)
        fprintf(f, "pesquisado %d %d\n", p, c->pesquisado[p]);
    for (int i = 0; i < c->n_fila; i++)
        fprintf(f, "obra %d %d %.3f\n", c->fila[i].estrutura, c->fila[i].nivel,
                c->fila[i].duracao);
    fprintf(f, "trabalhado %.3f\n", c->trabalhado);
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        remove(tmp);
        return 0;
    }
    remove(arquivo); /* no Windows, rename não substitui um arquivo existente */
    return rename(tmp, arquivo) == 0;
}

int carrega_colonia(Colonia *c, const char *arquivo, time_t *quando) {
    FILE *f = fopen(arquivo, "r");
    if (f == NULL) return 0;
    char linha[256];
    if (fgets(linha, sizeof linha, f) == NULL ||
        strncmp(linha, "COLONIA-SAVE 1", 14) != 0) {
        fclose(f);
        return 0;
    }
    Colonia nova;
    memset(&nova, 0, sizeof nova);
    long long t = 0;
    while (fgets(linha, sizeof linha, f) != NULL) {
        int a, b;
        double v;
        if (sscanf(linha, "tempo %lld", &t) == 1) continue;
        if (sscanf(linha, "relogio %lf", &v) == 1) nova.relogio = v;
        else if (sscanf(linha, "populacao %lf", &v) == 1) nova.populacao = v;
        else if (sscanf(linha, "venceu %d", &a) == 1) nova.venceu = a;
        else if (sscanf(linha, "recurso %d %lf", &a, &v) == 2 && a >= 0 &&
                 a < N_RECURSOS)
            nova.recurso[a] = v;
        else if (sscanf(linha, "nivel %d %d", &a, &b) == 2 && a >= 0 &&
                 a < N_ESTRUTURAS)
            nova.nivel[a] = b;
        else if (sscanf(linha, "pesquisado %d %d", &a, &b) == 2 && a >= 0 &&
                 a < N_PESQUISAS)
            nova.pesquisado[a] = b;
        else if (sscanf(linha, "obra %d %d %lf", &a, &b, &v) == 3 && a >= 0 &&
                 a < N_ESTRUTURAS && nova.n_fila < MAX_FILA)
            nova.fila[nova.n_fila++] = (Obra){(Estrutura)a, b, v};
        else if (sscanf(linha, "trabalhado %lf", &v) == 1) nova.trabalhado = v;
    }
    fclose(f);
    if (nova.populacao < 1) nova.populacao = 1;
    *c = nova;
    *quando = (time_t)t;
    return 1;
}
