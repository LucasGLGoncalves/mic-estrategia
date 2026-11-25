/* testes.c - testes automáticos do Posto Avançado (./posto --testa).
   SEU JOGO: cada regra nova merece um CHECK (capítulo 44). */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "jogo.h"
#include "piloto.h"
#include "salvar.h"

static int falhas = 0, acertos = 0;

#define CHECK(cond, ...)                                                       \
    do {                                                                       \
        if (cond) {                                                            \
            acertos++;                                                         \
        } else {                                                               \
            falhas++;                                                          \
            printf("  FALHOU: ");                                              \
            printf(__VA_ARGS__);                                               \
            printf("  (%s:%d)\n", __FILE__, __LINE__);                         \
        }                                                                      \
    } while (0)

static int mapas_iguais(const Jogo *a, const Jogo *b) {
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++)
            if (a->mapa[y][x].terreno != b->mapa[y][x].terreno) return 0;
    return 1;
}

/* a primeira quadra livre de um terreno, ou 0 se não houver */
static int acha(const Jogo *j, Terreno t, int *qx, int *qy) {
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++)
            if (j->mapa[y][x].terreno == t && j->mapa[y][x].obra == NADA) {
                *qx = x;
                *qy = y;
                return 1;
            }
    return 0;
}

static int arquivos_iguais(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int igual = fa && fb;
    while (igual) {
        int ca = fgetc(fa), cb = fgetc(fb);
        if (ca != cb) igual = 0;
        if (ca == EOF || cb == EOF) break;
    }
    if (fa) fclose(fa);
    if (fb) fclose(fb);
    return igual;
}

static void t_semente(void) {
    printf("[semente e mapa]\n");
    Jogo a, b;
    jogo_novo(&a, 42);
    jogo_novo(&b, 42);
    CHECK(mapas_iguais(&a, &b), "mesma semente, mesmo mapa");
    jogo_novo(&b, 43);
    CHECK(!mapas_iguais(&a, &b), "semente diferente, mapa diferente");
    int conta[N_TERRENOS] = {0};
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) conta[a.mapa[y][x].terreno]++;
    for (int t = 0; t < N_TERRENOS; t++)
        CHECK(conta[t] > 0, "o mapa tem %s (%d quadras)", NOME_TERRENO[t],
              conta[t]);
}

static void t_construcao(void) {
    printf("[construção]\n");
    Jogo j;
    jogo_novo(&j, 7);
    int x = 0, y = 0;
    CHECK(acha(&j, PLANICIE, &x, &y), "há planície livre");
    double madeira = j.recurso[MADEIRA];
    CHECK(constroi(&j, x, y, FAZENDA) == OK, "construir uma fazenda");
    CHECK(fabs(j.recurso[MADEIRA] - (madeira - 15)) < 1e-9,
          "a fazenda custa 15 de madeira");
    CHECK(constroi(&j, x, y, CASA) == ERRO_OCUPADA,
          "quadra ocupada é recusada");
    if (acha(&j, AGUA, &x, &y))
        CHECK(constroi(&j, x, y, CASA) == ERRO_TERRENO,
              "casa na água é recusada");
    j.recurso[MADEIRA] = 0;
    acha(&j, PLANICIE, &x, &y);
    CHECK(constroi(&j, x, y, CASA) == ERRO_RECURSOS, "sem madeira não há casa");
    j.mapa[y][x].obra = CASA; /* é a segunda casa: custou 23 de madeira */
    CHECK(demole(&j, x, y) && fabs(j.recurso[MADEIRA] - 11) < 1e-9,
          "demolir devolve metade do custo (%.0f)", j.recurso[MADEIRA]);
    double c1[N_RECURSOS], c2[N_RECURSOS];
    custo_atual(&j, FAZENDA, c1);
    acha(&j, PLANICIE, &x, &y);
    j.mapa[y][x].obra = FAZENDA;
    custo_atual(&j, FAZENDA, c2);
    CHECK(c2[MADEIRA] > c1[MADEIRA],
          "cada fazenda custa mais que a anterior "
          "(%.0f -> %.0f)",
          c1[MADEIRA], c2[MADEIRA]);
}

static void t_fome(void) {
    printf("[fome e crescimento]\n");
    Jogo j;
    jogo_novo(&j, 11);
    j.recurso[COMIDA] = 0;
    j.proximo_evento = 1e18; /* sem eventos: só a regra da fome */
    jogo_avanca(&j, 600);
    CHECK(j.populacao < 4, "sem comida a população diminui (%.2f)",
          j.populacao);
    CHECK(j.populacao >= 1, "mas nunca abaixo de 1 (%.2f)", j.populacao);
    j.recurso[COMIDA] = 1e6;
    jogo_avanca(&j, 3600);
    Taxas t;
    jogo_taxas(&j, &t);
    CHECK(j.populacao <= t.moradias + 1e-9,
          "a população não passa das moradias");
    CHECK(j.populacao > 4.5, "com comida e casa, a população cresce (%.2f)",
          j.populacao);
}

static void t_salvar(void) {
    printf("[salvar e carregar]\n");
    Jogo a, b;
    jogo_novo(&a, 2026);
    for (int i = 0; i < 360; i++) { /* uma hora de jogo com o piloto */
        jogo_avanca(&a, 10);
        piloto_joga(&a);
    }
    time_t quando;
    CHECK(salva_jogo(&a, "teste_a.sav"), "salvar");
    CHECK(carrega_jogo(&b, "teste_a.sav", &quando), "carregar");
    CHECK(salva_jogo(&b, "teste_b.sav"), "salvar de novo");
    CHECK(arquivos_iguais("teste_a.sav", "teste_b.sav"),
          "salvar, carregar e salvar dá o mesmo arquivo");
    CHECK(mapas_iguais(&a, &b), "o terreno volta igual, da semente");
    jogo_avanca(&a, 900);
    jogo_avanca(&b, 900);
    CHECK(a.recurso[MADEIRA] == b.recurso[MADEIRA] &&
              a.populacao == b.populacao,
          "o original e o carregado continuam iguais");
    FILE *f = fopen("teste_c.sav", "w");
    if (f) {
        fputs("lixo\n", f);
        fclose(f);
    }
    CHECK(!carrega_jogo(&b, "teste_c.sav", &quando), "recusa um arquivo ruim");
    remove("teste_a.sav");
    remove("teste_b.sav");
    remove("teste_c.sav");
}

static void t_piloto(void) {
    printf("[piloto automático: 4 horas]\n");
    Jogo j;
    jogo_novo(&j, 99);
    double p0 = jogo_pontos(&j);
    int ruim = 0;
    for (int i = 0; i < 4 * 360; i++) {
        jogo_avanca(&j, 10);
        piloto_joga(&j);
        for (int r = 0; r < N_RECURSOS; r++)
            if (!(j.recurso[r] >= 0)) ruim = 1;
        if (!(j.populacao >= 1)) ruim = 1;
    }
    CHECK(!ruim, "nada negativo, nenhum NaN");
    CHECK(jogo_pontos(&j) > p0 * 5, "o posto cresceu (%.0f -> %.0f pontos)", p0,
          jogo_pontos(&j));
}

int roda_testes(void) {
    t_semente();
    t_construcao();
    t_fome();
    t_salvar();
    t_piloto();
    printf("\n%d verificações OK, %d falhas\n", acertos, falhas);
    return falhas ? 1 : 0;
}
