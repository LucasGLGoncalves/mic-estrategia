/* salvar.c - grava e lê a partida num arquivo de texto, chave e valor */
#include "salvar.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int salva_jogo(const Jogo *j, const char *arquivo) {
    char tmp[256];
    snprintf(tmp, sizeof tmp, "%s.tmp", arquivo);
    FILE *f = fopen(tmp, "w");
    if (f == NULL) return 0;
    fprintf(f, "MERCADOR-SAVE 1\n");
    fprintf(f, "dia %d\naqui %d\ncreditos %.4f\ndivida %.4f\n", j->dia, j->aqui,
            j->creditos, j->divida);
    fprintf(f, "nave %d %d %d %d %d %d\n", j->carga_max, j->combustivel,
            j->combustivel_max, j->casco, j->canhoes, j->escudo);
    for (int b = 0; b < N_BENS; b++)
        fprintf(f, "carga %d %d %.4f\n", b, j->carga[b], j->pago[b]);
    for (int p = 0; p < N_PLANETAS; p++)
        for (int b = 0; b < N_BENS; b++)
            fprintf(f, "estoque %d %d %.4f\n", p, b, j->planetas[p].estoque[b]);
    /* o estado do gerador: com ele, o acaso continua exatamente de onde
       parou */
    fprintf(f,
            "rng %016" PRIx64 " %016" PRIx64 " %016" PRIx64 " %016" PRIx64 "\n",
            j->rng.s[0], j->rng.s[1], j->rng.s[2], j->rng.s[3]);
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        remove(tmp);
        return 0;
    }
    remove(arquivo);
    return rename(tmp, arquivo) == 0;
}

int carrega_jogo(Jogo *j, const char *arquivo) {
    FILE *f = fopen(arquivo, "r");
    if (f == NULL) return 0;
    char linha[256];
    if (fgets(linha, sizeof linha, f) == NULL ||
        strncmp(linha, "MERCADOR-SAVE 1", 15) != 0) {
        fclose(f);
        return 0;
    }
    Jogo novo;
    mundo_novo(&novo, 0); /* os dados fixos (nomes, tipos) vêm do código */
    while (fgets(linha, sizeof linha, f) != NULL) {
        int a, b, c;
        double v;
        uint64_t s[4];
        if (sscanf(linha, "dia %d", &a) == 1) novo.dia = a;
        else if (sscanf(linha, "aqui %d", &a) == 1 && a >= 0 && a < N_PLANETAS)
            novo.aqui = a;
        else if (sscanf(linha, "creditos %lf", &v) == 1) novo.creditos = v;
        else if (sscanf(linha, "divida %lf", &v) == 1) novo.divida = v;
        else if (sscanf(linha, "nave %d %d %d %d %d %d", &novo.carga_max,
                        &novo.combustivel, &novo.combustivel_max, &novo.casco,
                        &novo.canhoes, &novo.escudo) == 6) {
        } else if (sscanf(linha, "carga %d %d %lf", &a, &b, &v) == 3 &&
                   a >= 0 && a < N_BENS) {
            novo.carga[a] = b;
            novo.pago[a] = v;
        } else if (sscanf(linha, "estoque %d %d %lf", &a, &c, &v) == 3 &&
                   a >= 0 && a < N_PLANETAS && c >= 0 && c < N_BENS) {
            novo.planetas[a].estoque[c] = v;
        } else if (sscanf(linha,
                          "rng %" SCNx64 " %" SCNx64 " %" SCNx64 " %" SCNx64,
                          &s[0], &s[1], &s[2], &s[3]) == 4) {
            memcpy(novo.rng.s, s, sizeof s);
        }
    }
    fclose(f);
    novo.diario.total = 0;
    diario_poe(&novo.diario, "Diário recuperado. Dia %d, em %s.", novo.dia,
               novo.planetas[novo.aqui].nome);
    *j = novo;
    return 1;
}
