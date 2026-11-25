/* simula.c - um comerciante guloso joga centenas de partidas, sem tela,
   para medir o equilíbrio da economia. Os eventos de viagem ficam de fora. */
#include <stdio.h>
#include <stdlib.h>

#include "mundo.h"
#include "telas.h"

/* Escolhe a melhor carga e o melhor destino: o maior lucro por dia de
   viagem. */
static int planeja(const Jogo *j, int *bem) {
    int melhor_destino = -1;
    double melhor = 0;
    for (int d = 0; d < N_PLANETAS; d++) {
        if (d == j->aqui) continue;
        for (int b = 0; b < N_BENS; b++) {
            if (BENS[b].ilegal && j->planetas[d].tipo != FRONTEIRA) continue;
            double custo = preco_compra(j, j->aqui, b);
            int q = (int)(j->creditos / custo);
            if (q > j->carga_max) q = j->carga_max;
            /* 0,85: vender muitas unidades derruba o preço, então desconta */
            double lucro = (preco_venda(j, d, b) * 0.85 - custo) * q;
            double por_dia = lucro / dias_de_viagem(j, d);
            if (por_dia > melhor) {
                melhor = por_dia;
                melhor_destino = d;
                *bem = b;
            }
        }
    }
    return melhor_destino;
}

static double joga(uint64_t semente) {
    Jogo j;
    mundo_novo(&j, semente);
    while (j.dia <= DIAS_DE_JOGO) {
        for (int b = 0; b < N_BENS; b++) vende(&j, b, j.carga[b]);
        if (j.aqui == 6) { /* em Kepler, paga o que puder ao banco */
            double p =
                j.creditos - 500 < j.divida ? j.creditos - 500 : j.divida;
            if (p > 0) {
                j.creditos -= p;
                j.divida -= p;
            }
        }
        double porao = custo_melhoria(&j, M_CARGA);
        if (j.planetas[j.aqui].estaleiro && j.creditos > 3 * porao &&
            j.dia < 40) {
            j.creditos -= porao;
            j.carga_max += 10;
        }
        j.creditos -= custo_melhoria(&j, M_COMBUSTIVEL);
        j.combustivel = j.combustivel_max;
        int bem = 0, destino = planeja(&j, &bem);
        if (destino < 0) destino = 6; /* nada dá lucro: volta para Kepler */
        else compra(&j, bem, j.carga_max);
        int dias = dias_de_viagem(&j, destino);
        for (int d = 0; d < dias; d++) passa_um_dia(&j);
        j.aqui = destino;
    }
    for (int b = 0; b < N_BENS; b++) vende(&j, b, j.carga[b]);
    return patrimonio(&j);
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 200;
    double soma = 0, menor = 1e18, maior = -1e18;
    int ricos = 0;
    for (int s = 1; s <= n; s++) {
        double p = joga((uint64_t)s);
        soma += p;
        if (p < menor) menor = p;
        if (p > maior) maior = p;
        if (p >= 15000) ricos++;
    }
    printf("%d partidas: média %.0f, pior %.0f, melhor %.0f\n", n, soma / n,
           menor, maior);
    printf("terminaram com 15.000 ou mais: %d%%\n", 100 * ricos / n);
    return 0;
}
