/* reino.c - o estado do reino e as regras de cada ano */
#include "reino.h"

#include <stdio.h>

#include "entrada.h"
#include "eventos.h"

void reino_novo(Reino *r) {
    *r = (Reino){0};
    r->ano = 1;
    r->populacao = 95;
    r->graos = 2800;
    r->terras = 1000;
    r->colheita = 3;
    r->ratos = 200;
    r->nascidos = 5;
    r->preco_terra = sorteia(17, 26);
}

void reino_relatorio(const Reino *r) {
    printf("\n====================================================\n");
    printf("Hamurabi, permita-me relatar. No ano %d do seu reinado:\n", r->ano);
    if (r->mortos_fome > 0)
        printf("  %d pessoas morreram de fome.\n", r->mortos_fome);
    if (r->mortos_peste > 0)
        printf("  A peste levou %d pessoas.\n", r->mortos_peste);
    printf("  %d pessoas chegaram à cidade.\n", r->nascidos);
    printf("  A população agora é de %d.\n", r->populacao);
    printf("  A cidade possui %d acres de terra.\n", r->terras);
    printf("  Colhemos %d saca%s por acre.\n", r->colheita,
           r->colheita == 1 ? "" : "s");
    printf("  Os ratos comeram %d sacas.\n", r->ratos);
    printf("  Há %d sacas no celeiro.\n", r->graos);
    printf("  A terra está valendo %d sacas por acre.\n", r->preco_terra);
    printf("====================================================\n");
}

int reino_limite_plantio(const Reino *r) {
    int limite = r->terras;
    if (limite > r->graos * 2) limite = r->graos * 2;
    if (limite > r->populacao * ACRES_POR_PESSOA)
        limite = r->populacao * ACRES_POR_PESSOA;
    return limite;
}

void reino_fim_do_ano(Reino *r, int comida, int plantados) {
    int pop_inicio = r->populacao;
    r->ratos = 0;
    r->mortos_peste = 0;

    r->colheita = sorteia(1, 5);
    if (evento_acontece(EV_FARTURA)) evento_aplica(r, EV_FARTURA);
    r->graos += plantados * r->colheita;
    if (evento_acontece(EV_RATOS)) evento_aplica(r, EV_RATOS);

    int alimentados = comida / SACAS_POR_PESSOA;
    r->mortos_fome =
        r->populacao > alimentados ? r->populacao - alimentados : 0;
    r->revolta = r->mortos_fome * 100 > pop_inicio * 45;
    r->soma_percent += 100.0 * r->mortos_fome / pop_inicio;
    r->total_mortos += r->mortos_fome;
    r->nascidos =
        sorteia(1, 5) * (20 * r->terras + r->graos) / r->populacao / 100 + 1;
    r->populacao += r->nascidos - r->mortos_fome;

    if (evento_acontece(EV_PESTE)) evento_aplica(r, EV_PESTE);
    r->ano++;
    r->preco_terra = sorteia(17, 26);
}

void reino_avaliacao(const Reino *r) {
    double media = r->soma_percent / ANOS;
    double acres = (double)r->terras / r->populacao;
    printf("\nAo fim do seu reinado, a cidade tem %d pessoas, %d acres e %d "
           "sacas.\n",
           r->populacao, r->terras, r->graos);
    printf("Em %d anos, uma média de %.1f%% do povo morreu de fome por ano,\n",
           ANOS, media);
    printf("num total de %d mortos de fome e de peste. Restam %.1f acres por "
           "pessoa.\n\n",
           r->total_mortos, acres);
    if (media > 33 || acres < 7) {
        printf(
            "Seu governo foi tão desastroso que você foi expulso da cidade.\n");
    } else if (media > 10 || acres < 9) {
        printf("Você governou com a mão pesada de um Nero. O povo não sente "
               "saudades.\n");
    } else if (media > 3 || acres < 10) {
        printf("Seu governo foi razoável. Alguns o detestam, mas a maioria o "
               "tolera.\n");
    } else {
        printf("Um governo fantástico! Carlos Magno e Jefferson não teriam "
               "feito melhor.\n");
    }
}

/* Pontuação final: terra por pessoa vale muito, fome custa caro. */
int reino_pontos(const Reino *r) {
    double media = r->soma_percent / ANOS;
    double acres = (double)r->terras / r->populacao;
    int pontos = (int)(acres * 50 + r->populacao * 2 - media * 30);
    return pontos > 0 ? pontos : 0;
}
