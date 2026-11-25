/* jogo.c - as tabelas e as regras do Posto Avançado */
#include "jogo.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../motor/ruido.h"

/* SEU JOGO: as construções. Uma linha nova aqui (e um valor novo no enum
   Construcao) e ela aparece no menu, no mapa e no arquivo salvo. */
/* clang-format off */
const DadosConstrucao CONSTRUCOES[N_CONSTRUCOES] = {
    /* nome, letra, terreno, custo (C M P), produção (C M P), mora, trabalha */
    [NADA]     = {"Vazio",    '.', PLANICIE, {0, 0, 0},   {0, 0, 0},   0, 0, ""},
    [CASA]     = {"Casa",     'C', PLANICIE, {0, 20, 10}, {0, 0, 0},   5, 0,
                  "Abriga 5 pessoas. Sem casas, a população não cresce."},
    [FAZENDA]  = {"Fazenda",  'F', PLANICIE, {0, 15, 0},  {1.2, 0, 0}, 0, 2,
                  "Produz comida. Cada pessoa come 0,1 por segundo."},
    [SERRARIA] = {"Serraria", 'S', FLORESTA, {5, 10, 5},  {0, 0.8, 0}, 0, 2,
                  "Corta madeira. Precisa ficar na floresta."},
    [PEDREIRA] = {"Pedreira", 'P', MONTANHA, {10, 25, 0}, {0, 0, 0.5}, 0, 3,
                  "Extrai pedra. Precisa ficar na montanha."},
};
/* clang-format on */

const char *const NOME_RECURSO[N_RECURSOS] = {"Comida", "Madeira", "Pedra"};
const char *const NOME_TERRENO[N_TERRENOS] = {"Água", "Planície", "Floresta",
                                              "Montanha"};

#define FOME_POR_PESSOA 0.1  /* comida por segundo */
#define CRESCIMENTO     0.02 /* da população, por segundo, com comida */
#define EVENTO_MIN      60.0 /* intervalo entre eventos, em segundos */
#define EVENTO_MAX      180.0
#define CRESCE_CUSTO    1.15 /* cada construção igual custa 15% mais */

void jogo_avisa(Jogo *j, const char *formato, ...) {
    if (j->n_avisos == MAX_AVISOS) { /* cheio: esquece o mais antigo */
        memmove(j->aviso[0], j->aviso[1],
                sizeof j->aviso[0] * (MAX_AVISOS - 1));
        j->n_avisos--;
    }
    va_list ap;
    va_start(ap, formato);
    vsnprintf(j->aviso[j->n_avisos++], sizeof j->aviso[0], formato, ap);
    va_end(ap);
}

/* SEU JOGO: o mapa. Duas camadas de ruído (capítulo 22): altitude e umidade. */
void jogo_gera_mapa(Jogo *j) {
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) {
            /* as quadras são duas vezes mais altas que largas na tela */
            double alt = fbm2(j->semente, x * 0.07, y * 0.14, 4);
            double umi = fbm2(j->semente ^ 0x5EED, x * 0.11, y * 0.22, 3);
            Terreno t = alt < 0.40   ? AGUA
                        : alt > 0.64 ? MONTANHA
                        : umi > 0.52 ? FLORESTA
                                     : PLANICIE;
            j->mapa[y][x].terreno = t;
        }
}

void jogo_novo(Jogo *j, uint64_t semente) {
    memset(j, 0, sizeof *j);
    j->semente = semente;
    rng_semente(&j->rng, semente);
    jogo_gera_mapa(j);
    j->recurso[COMIDA] = 60;
    j->recurso[MADEIRA] = 60;
    j->recurso[PEDRA] = 20;
    j->populacao = 4;
    j->proximo_evento = rng_entre(&j->rng, EVENTO_MIN, EVENTO_MAX);
    /* a primeira casa, de graça, na planície mais perto do centro */
    int melhor = -1, mx = 0, my = 0;
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) {
            int d = abs(x - MAPA_L / 2) + 2 * abs(y - MAPA_A / 2);
            if (j->mapa[y][x].terreno == PLANICIE &&
                (melhor < 0 || d < melhor)) {
                melhor = d;
                mx = x;
                my = y;
            }
        }
    if (melhor >= 0) j->mapa[my][mx].obra = CASA;
    jogo_avisa(j, "O posto avançado foi fundado. Boa sorte!");
}

void jogo_taxas(const Jogo *j, Taxas *t) {
    memset(t, 0, sizeof *t);
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) {
            const DadosConstrucao *d = &CONSTRUCOES[j->mapa[y][x].obra];
            t->moradias += d->moradores;
            t->vagas += d->trabalhadores;
            for (int r = 0; r < N_RECURSOS; r++) t->producao[r] += d->produz[r];
        }
    /* se há menos gente que vagas, todas as construções trabalham menos */
    t->eficiencia = t->vagas == 0 ? 1.0 : fmin(1.0, j->populacao / t->vagas);
    for (int r = 0; r < N_RECURSOS; r++) t->producao[r] *= t->eficiencia;
    t->fome = j->populacao * FOME_POR_PESSOA;
}

/* SEU JOGO: os eventos. Toda opção deveria ter um preço (capítulo 45): o
   desafio 2 do capítulo 46 transforma estes eventos em decisões. */
static void sorteia_evento(Jogo *j) {
    static const double pesos[] = {3, 2, 2, 1};
    switch (rng_ponderado(&j->rng, pesos, 4)) {
    case 0: {
        double v = 20 + j->populacao * 2;
        j->recurso[COMIDA] += v;
        jogo_avisa(j, "\001gColheita farta\001d: +%.0f de comida.", v);
    } break;
    case 1: {
        double v = floor(j->recurso[MADEIRA] * 0.25);
        j->recurso[MADEIRA] -= v;
        jogo_avisa(j, "\001rTempestade\001d: o vento levou %.0f de madeira.",
                   v);
    } break;
    case 2: {
        Taxas t;
        jogo_taxas(j, &t);
        if (j->populacao + 2 <= t.moradias) {
            j->populacao += 2;
            jogo_avisa(j, "\001gViajantes\001d chegaram: +2 moradores.");
        } else {
            jogo_avisa(j, "Viajantes passaram, mas não havia casas para eles.");
        }
    } break;
    default: {
        double v = floor(j->recurso[PEDRA] * 0.3);
        j->recurso[PEDRA] -= v;
        jogo_avisa(j, "\001rDesabamento\001d na pedreira: -%.0f de pedra.", v);
    } break;
    }
}

/* Um passo pequeno do tempo: produção, fome e crescimento (capítulo 19) */
static void passo(Jogo *j, double dt) {
    Taxas t;
    jogo_taxas(j, &t);
    for (int r = 0; r < N_RECURSOS; r++) j->recurso[r] += t.producao[r] * dt;
    j->recurso[COMIDA] -= t.fome * dt;
    if (j->recurso[COMIDA] < 0) { /* fome: um pouco da população vai embora */
        j->recurso[COMIDA] = 0;
        double antes = j->populacao;
        j->populacao = fmax(1.0, j->populacao - 0.05 * dt);
        if ((int)antes != (int)j->populacao)
            jogo_avisa(j, "\001rFome!\001d Um morador deixou o posto.");
    } else if (t.moradias > 0) { /* crescimento logístico (capítulo 25) */
        double p = j->populacao;
        j->populacao += CRESCIMENTO * p * (1.0 - p / t.moradias) * dt;
        if (j->populacao > t.moradias) j->populacao = fmax(p, t.moradias);
    }
    j->relogio += dt;
    if (j->relogio >= j->proximo_evento) {
        sorteia_evento(j);
        j->proximo_evento += rng_entre(&j->rng, EVENTO_MIN, EVENTO_MAX);
    }
}

void jogo_avanca(Jogo *j, double segundos) {
    while (segundos > 0) { /* passos de no máximo 1 segundo */
        double dt = segundos > 1.0 ? 1.0 : segundos;
        passo(j, dt);
        segundos -= dt;
    }
}

int jogo_quantas(const Jogo *j, Construcao c) {
    int n = 0;
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++)
            if (j->mapa[y][x].obra == c) n++;
    return n;
}

/* Custos que crescem (capítulo 25): sem isso, o mapa enche em minutos e o
   jogo acaba antes de começar. */
static void custo_da(Construcao c, int ja_tem, double custo[N_RECURSOS]) {
    double f = pow(CRESCE_CUSTO, ja_tem);
    for (int r = 0; r < N_RECURSOS; r++)
        custo[r] = floor(CONSTRUCOES[c].custo[r] * f);
}

void custo_atual(const Jogo *j, Construcao c, double custo[N_RECURSOS]) {
    custo_da(c, jogo_quantas(j, c), custo);
}

Resultado pode_construir(const Jogo *j, int x, int y, Construcao c) {
    if (x < 0 || y < 0 || x >= MAPA_L || y >= MAPA_A || c <= NADA ||
        c >= N_CONSTRUCOES)
        return ERRO_FORA;
    if (j->mapa[y][x].obra != NADA) return ERRO_OCUPADA;
    if (j->mapa[y][x].terreno != CONSTRUCOES[c].terreno) return ERRO_TERRENO;
    double custo[N_RECURSOS];
    custo_atual(j, c, custo);
    for (int r = 0; r < N_RECURSOS; r++)
        if (j->recurso[r] + 1e-9 < custo[r]) return ERRO_RECURSOS;
    return OK;
}

Resultado constroi(Jogo *j, int x, int y, Construcao c) {
    Resultado res = pode_construir(j, x, y, c);
    if (res != OK) return res;
    double custo[N_RECURSOS];
    custo_atual(j, c, custo);
    for (int r = 0; r < N_RECURSOS; r++) j->recurso[r] -= custo[r];
    j->mapa[y][x].obra = c;
    return OK;
}

int demole(Jogo *j, int x, int y) {
    if (x < 0 || y < 0 || x >= MAPA_L || y >= MAPA_A) return 0;
    Construcao c = j->mapa[y][x].obra;
    if (c == NADA) return 0;
    j->mapa[y][x].obra = NADA;
    double custo[N_RECURSOS]; /* o quanto custou a última */
    custo_atual(j, c, custo);
    for (int r = 0; r < N_RECURSOS; r++) j->recurso[r] += floor(custo[r] / 2);
    return 1;
}

const char *texto_resultado(Resultado r) {
    switch (r) {
    case OK: return "ok";
    case ERRO_FORA: return "fora do mapa";
    case ERRO_TERRENO: return "terreno errado";
    case ERRO_OCUPADA: return "quadra ocupada";
    case ERRO_RECURSOS: return "recursos insuficientes";
    }
    return "?";
}

/* Os pontos: tudo o que se gastou em construções, mais 10 por morador */
double jogo_pontos(const Jogo *j) {
    double p = j->populacao * 10;
    for (Construcao c = CASA; c < N_CONSTRUCOES; c++) {
        int n = jogo_quantas(j, c);
        for (int k = 0; k < n; k++) {
            double custo[N_RECURSOS];
            custo_da(c, k, custo);
            for (int r = 0; r < N_RECURSOS; r++) p += custo[r];
        }
    }
    return p;
}
