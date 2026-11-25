/* colonia.c - a economia da colônia, segundo a segundo */
#include "colonia.h"

#include <math.h>
#include <string.h>

const char *NOME_RECURSO[N_RECURSOS] = {"Metal", "Cristal", "Comida",
                                        "Ciência"};

const DadosEstrutura ESTRUTURAS[N_ESTRUTURAS] = {
    /* nome, descrição metal cristal fator tempo trab */
    [MINA_METAL] = {"Mina de metal", "extrai metal do subsolo", 60, 15, 1.5, 10,
                    2},
    [MINA_CRISTAL] = {"Mina de cristal", "corta cristais das cavernas", 48, 24,
                      1.6, 12, 2},
    [FAZENDA] = {"Fazenda hidropônica", "produz comida sob as cúpulas", 40, 10,
                 1.5, 10, 1},
    [USINA] = {"Usina solar", "gera a energia das outras", 75, 30, 1.5, 12, 1},
    [HABITACAO] = {"Habitação", "abriga mais colonos", 50, 20, 1.6, 15, 0},
    [ARMAZEM] = {"Armazém", "guarda mais metal, cristal e comida", 100, 50, 1.7,
                 20, 0},
    [LABORATORIO] = {"Laboratório", "produz ciência", 200, 400, 1.8, 40, 3},
    [PORTAL] = {"Portal Estelar", "o caminho de volta para casa", 80000, 50000,
                1, 600, 0},
};

const DadosPesquisa PESQUISAS[N_PESQUISAS] = {
    [ROBOTICA] = {"Robótica", "obras 30% mais rápidas", 80, -1},
    [MINERACAO] = {"Mineração profunda", "+25% de metal e cristal", 250,
                   ROBOTICA},
    [FOTOSSINTESE] = {"Fotossíntese artificial", "+40% de comida", 150, -1},
    [SUPERCONDUTORES] = {"Supercondutores", "+30% de energia", 400, MINERACAO},
    [ARQUITETURA] = {"Arquitetura orbital", "habitações abrigam o dobro", 600,
                     FOTOSSINTESE},
    [DOBRA] = {"Propulsão de dobra", "libera o Portal Estelar", 2500,
               SUPERCONDUTORES},
};

void colonia_nova(Colonia *c) {
    memset(c, 0, sizeof *c);
    c->recurso[METAL] = 500;
    c->recurso[CRISTAL] = 300;
    c->recurso[COMIDA] = 100;
    c->populacao = 6;
    diario_poe(
        &c->diario,
        "A nave pousou. Seis colonos, alguns caixotes e um planeta inteiro.");
}

/* A fórmula de produção de quase tudo: cresce um pouco mais que o nível. */
static double rende(int n) { return n * pow(1.1, n); }

Taxas colonia_taxas(const Colonia *c) {
    Taxas t;
    memset(&t, 0, sizeof t);
    const int *n = c->nivel;
    /* os trabalhadores: se faltar gente, tudo rende proporcionalmente menos */
    for (int e = 0; e < N_ESTRUTURAS; e++)
        t.trabalhadores += n[e] * ESTRUTURAS[e].trabalhadores;
    t.eficiencia_trabalho =
        t.trabalhadores > 0 ? fmin(1, c->populacao / t.trabalhadores) : 1;
    /* a energia: a usina só depende de gente; as outras dependem dela */
    t.energia_gerada = 20 * rende(n[USINA]) * t.eficiencia_trabalho *
                       (c->pesquisado[SUPERCONDUTORES] ? 1.3 : 1);
    t.energia_gasta = 10 * rende(n[MINA_METAL]) + 10 * rende(n[MINA_CRISTAL]) +
                      6 * rende(n[FAZENDA]) + 12 * rende(n[LABORATORIO]);
    t.eficiencia_energia =
        t.energia_gasta > 0 ? fmin(1, t.energia_gerada / t.energia_gasta) : 1;
    double ef = t.eficiencia_trabalho * t.eficiencia_energia;
    double minerio = c->pesquisado[MINERACAO] ? 1.25 : 1;
    /* uma produção mínima, de mão, garante que a colônia nunca trava */
    t.producao[METAL] = 1.0 + 3 * rende(n[MINA_METAL]) * ef * minerio;
    t.producao[CRISTAL] = 0.5 + 2 * rende(n[MINA_CRISTAL]) * ef * minerio;
    t.producao[COMIDA] =
        1.0 +
        3 * rende(n[FAZENDA]) * ef * (c->pesquisado[FOTOSSINTESE] ? 1.4 : 1) -
        0.06 * c->populacao;
    t.producao[CIENCIA] = 1.0 * rende(n[LABORATORIO]) * ef;
    /* a população cresce numa curva logística, limitada pelas habitações */
    t.capacidade =
        (10 + 12 * n[HABITACAO]) * (c->pesquisado[ARQUITETURA] ? 2 : 1);
    t.estoque_max = 2500 * pow(2, n[ARMAZEM]); /* cada nível dobra o espaço */
    if (c->recurso[COMIDA] > 0 || t.producao[COMIDA] >= 0)
        t.crescimento = 0.02 * c->populacao * (1 - c->populacao / t.capacidade);
    else t.crescimento = -0.01 * c->populacao; /* fome */
    return t;
}

static void termina_obra(Colonia *c) {
    Obra o = c->fila[0];
    c->trabalhado -= o.duracao;
    memmove(&c->fila[0], &c->fila[1], (size_t)(c->n_fila - 1) * sizeof(Obra));
    c->n_fila--;
    c->nivel[o.estrutura] = o.nivel;
    if (o.estrutura == PORTAL) {
        c->venceu = 1;
        diario_poe(&c->diario, "\001gO Portal Estelar se abre!\001d");
    } else {
        diario_poe(&c->diario, "Pronto: %s, nível %d.",
                   ESTRUTURAS[o.estrutura].nome, o.nivel);
    }
}

/* Avança o tempo em passos de no máximo um segundo: as taxas mudam quando
   uma obra termina ou a população cresce, e passos pequenos acompanham a
   mudança. */
void colonia_avanca(Colonia *c, double segundos) {
    while (segundos > 0) {
        double dt = segundos > 1 ? 1 : segundos;
        segundos -= dt;
        Taxas t = colonia_taxas(c);
        for (int r = 0; r < N_RECURSOS; r++) {
            c->recurso[r] += t.producao[r] * dt;
            if (c->recurso[r] < 0) c->recurso[r] = 0;
            /* o que não cabe no armazém se perde (a ciência não ocupa
               espaço) */
            if (r != CIENCIA && c->recurso[r] > t.estoque_max)
                c->recurso[r] = t.estoque_max;
        }
        c->populacao += t.crescimento * dt;
        if (c->populacao < 1) c->populacao = 1; /* alguém sempre fica */
        c->relogio += dt;
        if (c->n_fila > 0) {
            c->trabalhado += dt;
            while (c->n_fila > 0 && c->trabalhado >= c->fila[0].duracao)
                termina_obra(c);
            if (c->n_fila == 0) c->trabalhado = 0;
        }
    }
}

void custo_de(Estrutura e, int nivel, double *metal, double *cristal) {
    double f = pow(ESTRUTURAS[e].fator, nivel - 1);
    *metal = ESTRUTURAS[e].metal * f;
    *cristal = ESTRUTURAS[e].cristal * f;
}

double duracao_de(const Colonia *c, Estrutura e, int nivel) {
    double d = ESTRUTURAS[e].tempo * pow(1.4, nivel - 1);
    return c->pesquisado[ROBOTICA] ? d * 0.7 : d;
}

int nivel_planejado(const Colonia *c, Estrutura e) {
    int n = c->nivel[e];
    for (int i = 0; i < c->n_fila; i++)
        if (c->fila[i].estrutura == e) n = c->fila[i].nivel;
    return n;
}

int pode_construir(const Colonia *c, Estrutura e, const char **motivo) {
    int nivel = nivel_planejado(c, e) + 1;
    double m, k;
    custo_de(e, nivel, &m, &k);
    *motivo = NULL;
    if (e == PORTAL && !c->pesquisado[DOBRA])
        *motivo = "requer Propulsão de dobra";
    else if (e == PORTAL && c->nivel[PORTAL] > 0)
        *motivo = "o portal já está aberto";
    else if (e == PORTAL && nivel > 1) *motivo = "já encomendado";
    else if (c->n_fila >= MAX_FILA) *motivo = "a fila está cheia";
    else if (c->recurso[METAL] < m) *motivo = "falta metal";
    else if (c->recurso[CRISTAL] < k) *motivo = "falta cristal";
    return *motivo == NULL;
}

int encomenda(Colonia *c, Estrutura e) {
    const char *motivo;
    if (!pode_construir(c, e, &motivo)) return 0;
    int nivel = nivel_planejado(c, e) + 1;
    double m, k;
    custo_de(e, nivel, &m, &k);
    c->recurso[METAL] -= m; /* paga na hora da encomenda */
    c->recurso[CRISTAL] -= k;
    c->fila[c->n_fila++] = (Obra){e, nivel, duracao_de(c, e, nivel)};
    return 1;
}

void cancela_ultima(Colonia *c) {
    if (c->n_fila == 0) return;
    Obra o = c->fila[--c->n_fila];
    double m, k;
    custo_de(o.estrutura, o.nivel, &m, &k);
    c->recurso[METAL] += m; /* devolve tudo */
    c->recurso[CRISTAL] += k;
    if (c->n_fila == 0) c->trabalhado = 0;
}

int pode_pesquisar(const Colonia *c, Pesquisa p) {
    const DadosPesquisa *d = &PESQUISAS[p];
    return !c->pesquisado[p] && (d->requer < 0 || c->pesquisado[d->requer]);
}

int pesquisa(Colonia *c, Pesquisa p) {
    if (!pode_pesquisar(c, p) || c->recurso[CIENCIA] < PESQUISAS[p].custo)
        return 0;
    c->recurso[CIENCIA] -= PESQUISAS[p].custo;
    c->pesquisado[p] = 1;
    diario_poe(&c->diario, "Pesquisa concluída: \001c%s\001d (%s).",
               PESQUISAS[p].nome, PESQUISAS[p].efeito);
    return 1;
}
