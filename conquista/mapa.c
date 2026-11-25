/* mapa.c - gera o continente com ruído e espalha as cidades */
#include "mapa.h"

#include <math.h>
#include <string.h>

#include "../motor/rng.h"
#include "../motor/ruido.h"

const char *NOME_TERRENO[N_TERRENOS] = {"mar", "planície", "floresta",
                                        "montanha"};

static const char *NOMES[] = {
    "Aurora",    "Bastião", "Corvo",   "Dunas", "Esmeralda", "Granito",
    "Horizonte", "Ilhéu",   "Jade",    "Lume",  "Marfim",    "Névoa",
    "Ônix",      "Quartzo", "Rochedo", "Solar", "Tormenta",  "Vigia",
};
#define N_NOMES ((int)(sizeof NOMES / sizeof NOMES[0]))

int mapa_dentro(int x, int y) {
    return x >= 0 && x < MAPA_L && y >= 0 && y < MAPA_A;
}

int custo_terreno(Terreno t) {
    switch (t) {
    case PLANICIE: return 1;
    case FLORESTA: return 2;
    default: return -1; /* mar e montanha: só a pé de gigante */
    }
}

double defesa_terreno(Terreno t) { return t == FLORESTA ? 1.5 : 1.0; }

int mapa_passavel(const Mapa *m, int x, int y) {
    return mapa_dentro(x, y) && custo_terreno(m->terreno[y][x]) > 0;
}

int cidade_em(const Mapa *m, int x, int y) {
    for (int c = 0; c < m->n_cidades; c++)
        if (m->cidades[c].x == x && m->cidades[c].y == y) return c;
    return -1;
}

/* O relevo: ruído fBm menos uma queda em direção às bordas, para formar
   um continente cercado de mar. A umidade, outro ruído, decide as florestas. */
static void gera_terreno(Mapa *m, uint64_t semente) {
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) {
            double nx = (x - MAPA_L / 2.0) / (MAPA_L / 2.0);
            double ny = (y - MAPA_A / 2.0) / (MAPA_A / 2.0);
            double borda = nx * nx * 0.9 + ny * ny * 0.9;
            double altura = fbm2(semente, x * 0.12, y * 0.12, 4) - borda * 0.4;
            double umidade = fbm2(semente + 1, x * 0.25, y * 0.25, 3);
            Terreno t = PLANICIE;
            if (altura < 0.22) t = MAR;
            else if (altura > 0.56) t = MONTANHA;
            else if (umidade > 0.6) t = FLORESTA;
            m->terreno[y][x] = t;
        }
}

/* Preenchimento por inundação (capítulo 24): marca tudo o que se alcança
   por terra a partir de (x, y). Devolve quantas casas foram marcadas. */
static int inunda(const Mapa *m, int x, int y,
                  unsigned char marca[MAPA_A][MAPA_L]) {
    static int fila[MAPA_L * MAPA_A];
    int ini = 0, fim = 0, total = 0;
    memset(marca, 0, MAPA_A * MAPA_L);
    marca[y][x] = 1;
    fila[fim++] = y * MAPA_L + x;
    while (ini < fim) {
        int p = fila[ini++], px = p % MAPA_L, py = p / MAPA_L;
        total++;
        static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
        for (int d = 0; d < 4; d++) {
            int nx = px + DX[d], ny = py + DY[d];
            if (mapa_passavel(m, nx, ny) && !marca[ny][nx]) {
                marca[ny][nx] = 1;
                fila[fim++] = ny * MAPA_L + nx;
            }
        }
    }
    return total;
}

static int distancia2(const Cidade *a, const Cidade *b) {
    int dx = a->x - b->x, dy = a->y - b->y;
    return dx * dx + dy * dy;
}

int mapa_gera(Mapa *m, uint64_t semente) {
    Rng rng;
    rng_semente(&rng, semente);
    memset(m, 0, sizeof *m);
    gera_terreno(m, semente);

    /* acha a maior região de terra: começa do centro e procura uma casa de
       terra */
    static unsigned char terra[MAPA_A][MAPA_L];
    int melhor = 0, mx = -1, my = -1;
    for (int tentativa = 0; tentativa < 40; tentativa++) {
        int x = rng_faixa(&rng, 4, MAPA_L - 5),
            y = rng_faixa(&rng, 3, MAPA_A - 4);
        if (!mapa_passavel(m, x, y)) continue;
        int n = inunda(m, x, y, terra);
        if (n > melhor) {
            melhor = n;
            mx = x;
            my = y;
        }
    }
    if (melhor < 200) return 0; /* continente pequeno demais: outra semente */
    inunda(m, mx, my, terra);

    /* espalha as cidades na região, cada uma a pelo menos 5 casas das outras */
    int nomes[N_NOMES];
    for (int i = 0; i < N_NOMES; i++) nomes[i] = i;
    rng_embaralha(&rng, nomes, N_NOMES);
    for (int tentativa = 0; tentativa < 3000 && m->n_cidades < MAX_CIDADES;
         tentativa++) {
        Cidade c = {0};
        c.x = rng_faixa(&rng, 1, MAPA_L - 2);
        c.y = rng_faixa(&rng, 1, MAPA_A - 2);
        if (!terra[c.y][c.x] || m->terreno[c.y][c.x] != PLANICIE) continue;
        int longe = 1;
        for (int k = 0; k < m->n_cidades; k++)
            if (distancia2(&c, &m->cidades[k]) < 25) longe = 0;
        if (!longe) continue;
        strcpy(c.nome, NOMES[nomes[m->n_cidades]]);
        c.dono = NEUTRO;
        c.produz = EXERCITO;
        m->cidades[m->n_cidades++] = c;
    }
    if (m->n_cidades < 10) return 0;

    /* as capitais: o par de cidades mais distantes entre si */
    int a = 0, b = 1, maior = -1;
    for (int i = 0; i < m->n_cidades; i++)
        for (int k = i + 1; k < m->n_cidades; k++)
            if (distancia2(&m->cidades[i], &m->cidades[k]) > maior) {
                maior = distancia2(&m->cidades[i], &m->cidades[k]);
                a = i;
                b = k;
            }
    if (m->cidades[a].x > m->cidades[b].x) { /* o jogador fica a oeste */
        int t = a;
        a = b;
        b = t;
    }
    m->cidades[a].dono = JOGADOR;
    m->cidades[b].dono = COMPUTADOR;
    m->cidades[a].capital = m->cidades[b].capital = 1;
    return 1;
}
