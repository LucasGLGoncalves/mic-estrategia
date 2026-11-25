/* salvar.c - o arquivo salvo do Posto Avançado. Só vai para o arquivo o que
   não pode ser recalculado: o terreno sai da semente de novo. */
#include "salvar.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define ASSINATURA "POSTO-SAVE 1"

/* Troca o arquivo antigo pelo novo de uma vez só (capítulo 42) */
static int substitui(const char *tmp, const char *arquivo) {
#ifdef _WIN32
    return MoveFileExA(tmp, arquivo, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(tmp, arquivo) == 0;
#endif
}

int salva_jogo(const Jogo *j, const char *arquivo) {
    char tmp[256];
    snprintf(tmp, sizeof tmp, "%s.tmp", arquivo);
    FILE *f = fopen(tmp, "w");
    if (f == NULL) return 0;
    fprintf(f, "%s\n", ASSINATURA);
    fprintf(f, "tempo %lld\n", (long long)time(NULL));
    fprintf(f, "semente %016" PRIx64 "\n", j->semente);
    fprintf(f,
            "rng %016" PRIx64 " %016" PRIx64 " %016" PRIx64 " %016" PRIx64 "\n",
            j->rng.s[0], j->rng.s[1], j->rng.s[2], j->rng.s[3]);
    fprintf(f, "relogio %.17g\n", j->relogio);
    fprintf(f, "populacao %.17g\n", j->populacao);
    fprintf(f, "evento %.17g\n", j->proximo_evento);
    for (int r = 0; r < N_RECURSOS; r++)
        fprintf(f, "recurso %d %.17g\n", r, j->recurso[r]);
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++)
            if (j->mapa[y][x].obra != NADA)
                fprintf(f, "obra %d %d %d\n", x, y, (int)j->mapa[y][x].obra);
    /* SEU JOGO: cada campo novo do Jogo precisa de uma linha aqui e outra
       em carrega_jogo. O teste de ida e volta (testes.c) avisa se faltar. */
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok || !substitui(tmp, arquivo)) {
        remove(tmp);
        return 0;
    }
    return 1;
}

int carrega_jogo(Jogo *j, const char *arquivo, time_t *quando) {
    FILE *f = fopen(arquivo, "r");
    if (f == NULL) return 0;
    char linha[256];
    if (fgets(linha, sizeof linha, f) == NULL ||
        strncmp(linha, ASSINATURA, strlen(ASSINATURA)) != 0) {
        fclose(f);
        return 0;
    }
    Jogo novo; /* lê numa cópia: se o arquivo estiver ruim, j fica intacto */
    memset(&novo, 0, sizeof novo);
    long long t = 0;
    int tem_semente = 0;
    while (fgets(linha, sizeof linha, f) != NULL) {
        uint64_t s[4];
        int a, b, c;
        double v;
        if (sscanf(linha, "tempo %lld", &t) == 1) continue;
        if (sscanf(linha, "semente %" SCNx64, &s[0]) == 1) {
            novo.semente = s[0];
            tem_semente = 1;
        } else if (sscanf(linha,
                          "rng %" SCNx64 " %" SCNx64 " %" SCNx64 " %" SCNx64,
                          &s[0], &s[1], &s[2], &s[3]) == 4) {
            memcpy(novo.rng.s, s, sizeof s);
        } else if (sscanf(linha, "relogio %lf", &v) == 1) {
            novo.relogio = v;
        } else if (sscanf(linha, "populacao %lf", &v) == 1) {
            novo.populacao = v;
        } else if (sscanf(linha, "evento %lf", &v) == 1) {
            novo.proximo_evento = v;
        } else if (sscanf(linha, "recurso %d %lf", &a, &v) == 2) {
            if (a >= 0 && a < N_RECURSOS) novo.recurso[a] = v;
        } else if (sscanf(linha, "obra %d %d %d", &a, &b, &c) == 3) {
            /* desconfie dos números que vêm de fora (capítulo 42) */
            if (a >= 0 && a < MAPA_L && b >= 0 && b < MAPA_A && c > NADA &&
                c < N_CONSTRUCOES)
                novo.mapa[b][a].obra = (Construcao)c;
        }
    }
    fclose(f);
    if (!tem_semente || !(novo.populacao >= 1)) return 0;
    jogo_gera_mapa(&novo); /* o terreno volta, igual, a partir da semente */
    *j = novo;
    *quando = (time_t)t;
    return 1;
}
