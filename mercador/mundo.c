/* mundo.c - planetas, mercadorias e a lei da oferta e da procura */
#include "mundo.h"

#include <math.h>
#include <string.h>

const Bem BENS[N_BENS] = {
    {"Comida", 20, 0},       {"Minério", 40, 0}, {"Remédios", 120, 0},
    {"Eletrônicos", 250, 0}, {"Armas", 420, 1},  {"Artefatos", 900, 0},
};

const char *NOME_TIPO[N_TIPOS] = {"agrícola", "minerador", "industrial",
                                  "tecnológico", "fronteira"};

/* Quanto cada tipo de planeta paga por cada bem, em relação ao preço de
   referência. Abaixo de 1, o planeta produz e vende barato; acima, precisa e
   paga caro. */
/* clang-format off */
static const double MOD[N_TIPOS][N_BENS] = {
    /*             comida minério remédios eletrôn. armas artefatos */
    [AGRICOLA]    = {0.45,  1.30,   1.25,    1.20,   1.10,  1.00},
    [MINERADOR]   = {1.50,  0.45,   1.30,    1.10,   1.20,  0.90},
    [INDUSTRIAL]  = {1.25,  1.45,   0.90,    0.60,   0.80,  1.10},
    [TECNOLOGICO] = {1.30,  1.20,   0.55,    0.80,   1.40,  1.50},
    [FRONTEIRA]   = {1.40,  0.90,   1.60,    1.40,   1.70,  0.60},
};
/* clang-format on */

static const Planeta GALAXIA[N_PLANETAS] = {
    {"Ceres", 8, 4, AGRICOLA, 0, 1, {0}, {0}},
    {"Vulcano", 22, 15, MINERADOR, 1, 0, {0}, {0}},
    {"Tannhäuser", 37, 6, INDUSTRIAL, 0, 1, {0}, {0}},
    {"Nova Lisboa", 50, 16, TECNOLOGICO, 0, 1, {0}, {0}},
    {"Orfeu", 14, 19, AGRICOLA, 1, 0, {0}, {0}},
    {"Hades", 58, 4, FRONTEIRA, 3, 0, {0}, {0}},
    {"Kepler", 30, 11, MINERADOR, 1, 1, {0}, {0}},
    {"Abismo", 6, 12, FRONTEIRA, 2, 0, {0}, {0}},
};

void mundo_novo(Jogo *j, uint64_t semente) {
    memset(j, 0, sizeof *j);
    rng_semente(&j->rng, semente);
    for (int p = 0; p < N_PLANETAS; p++) {
        j->planetas[p] = GALAXIA[p];
        for (int b = 0; b < N_BENS; b++) {
            /* quem produz um bem tem estoque grande dele; quem precisa,
               pequeno */
            double ideal =
                200.0 / MOD[j->planetas[p].tipo][b] * (b == 5 ? 0.2 : 1.0);
            j->planetas[p].ideal[b] = ideal;
            j->planetas[p].estoque[b] = ideal * rng_entre(&j->rng, 0.7, 1.3);
        }
    }
    j->dia = 1;
    j->aqui = 6; /* começa em Kepler */
    j->creditos = 2000;
    j->divida = 10000;
    j->carga_max = CARGA_INICIAL;
    j->combustivel = j->combustivel_max = 40;
    j->casco = 100;
    j->canhoes = 1;
    diario_poe(&j->diario,
               "Você herdou uma nave velha e uma \001rdívida de 10.000\001d "
               "com o Banco de Kepler. Tem %d dias para fazer fortuna.",
               DIAS_DE_JOGO);
}

/* Oferta e procura: pouco estoque, preço alto; muito estoque, preço baixo. */
double preco(const Jogo *j, int planeta, int bem) {
    const Planeta *p = &j->planetas[planeta];
    double fator = pow(p->ideal[bem] / fmax(p->estoque[bem], 1.0), 0.8);
    if (fator < 0.35) fator = 0.35;
    if (fator > 3.0) fator = 3.0;
    return BENS[bem].base * MOD[p->tipo][bem] * fator;
}

/* O mercado compra um pouco abaixo e vende um pouco acima: é assim que ele
   lucra. */
double preco_compra(const Jogo *j, int planeta, int bem) {
    return preco(j, planeta, bem) * 1.06;
}
double preco_venda(const Jogo *j, int planeta, int bem) {
    return preco(j, planeta, bem) * 0.94;
}

int carga_usada(const Jogo *j) {
    int n = 0;
    for (int b = 0; b < N_BENS; b++) n += j->carga[b];
    return n;
}

/* Compra uma unidade de cada vez: cada compra encarece a próxima. */
int compra(Jogo *j, int bem, int quantidade) {
    int feitas = 0;
    Planeta *p = &j->planetas[j->aqui];
    while (feitas < quantidade && carga_usada(j) < j->carga_max &&
           p->estoque[bem] >= 1) {
        double v = preco_compra(j, j->aqui, bem);
        if (v > j->creditos) break;
        j->creditos -= v;
        j->pago[bem] = (j->pago[bem] * j->carga[bem] + v) / (j->carga[bem] + 1);
        j->carga[bem]++;
        p->estoque[bem] -= 1;
        feitas++;
    }
    return feitas;
}

int vende(Jogo *j, int bem, int quantidade) {
    int feitas = 0;
    Planeta *p = &j->planetas[j->aqui];
    while (feitas < quantidade && j->carga[bem] > 0) {
        j->creditos += preco_venda(j, j->aqui, bem);
        j->carga[bem]--;
        p->estoque[bem] += 1;
        feitas++;
    }
    if (j->carga[bem] == 0) j->pago[bem] = 0;
    return feitas;
}

/* Um dia passa: juros da dívida, e cada mercado volta devagar ao seu
   equilíbrio. */
void passa_um_dia(Jogo *j) {
    j->dia++;
    j->divida *= 1.01;
    for (int p = 0; p < N_PLANETAS; p++)
        for (int b = 0; b < N_BENS; b++) {
            Planeta *pl = &j->planetas[p];
            pl->estoque[b] += (pl->ideal[b] - pl->estoque[b]) * 0.10;
            /* o vaivém do comércio */
            pl->estoque[b] *= rng_entre(&j->rng, 0.94, 1.06);
        }
}

double patrimonio(const Jogo *j) {
    double v = j->creditos - j->divida;
    for (int b = 0; b < N_BENS; b++)
        v += j->carga[b] * preco_venda(j, j->aqui, b);
    return v;
}

double distancia(const Jogo *j, int a, int b) {
    double dx = j->planetas[a].x - j->planetas[b].x,
           dy = (j->planetas[a].y - j->planetas[b].y) * 2.0;
    return sqrt(dx * dx + dy * dy);
}

int dias_de_viagem(const Jogo *j, int destino) {
    int d = (int)ceil(distancia(j, j->aqui, destino) / 12.0);
    return d < 1 ? 1 : d;
}

int combustivel_da_viagem(const Jogo *j, int destino) {
    return (int)ceil(distancia(j, j->aqui, destino) / 3.0);
}
