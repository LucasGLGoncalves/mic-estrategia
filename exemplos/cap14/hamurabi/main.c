/* main.c - O Reino de Hamurabi, com jogo salvo e quadro de honra */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "entrada.h"
#include "reino.h"
#include "salvar.h"

int main(void) {
    printf("=== O REINO DE HAMURABI ===\n");
    printf("Governe a antiga Suméria por %d anos.\n", ANOS);
    srand((unsigned)time(NULL));

    Reino r;
    if (carrega_reino(&r, ARQUIVO_SAVE) &&
        le_inteiro("Há um reinado salvo. Continuar? (1 = sim, 2 = não) ", 1,
                   2) == 1) {
        printf("Bem-vindo de volta, Hamurabi. Estamos no ano %d.\n", r.ano);
    } else {
        reino_novo(&r);
    }

    while (r.ano <= ANOS) {
        reino_relatorio(&r);

        int compra = le_inteiro("Quantos acres deseja comprar? ", 0,
                                r.graos / r.preco_terra);
        if (compra == 0) {
            int venda =
                le_inteiro("Quantos acres deseja vender? ", 0, r.terras);
            r.terras -= venda;
            r.graos += venda * r.preco_terra;
        } else {
            r.terras += compra;
            r.graos -= compra * r.preco_terra;
        }

        int comida =
            le_inteiro("Quantas sacas para alimentar o povo? ", 0, r.graos);
        r.graos -= comida;

        int limite = reino_limite_plantio(&r);
        printf("(você pode plantar até %d acres)\n", limite);
        int plantar = le_inteiro("Quantos acres deseja plantar? ", 0, limite);
        r.graos -= (plantar + 1) / 2;

        reino_fim_do_ano(&r, comida, plantar);
        if (r.revolta) {
            printf("\nVocê deixou %d pessoas morrerem de fome em um ano!\n",
                   r.mortos_fome);
            printf("O povo se revoltou e você foi deposto.\n");
            remove(ARQUIVO_SAVE);
            return 0;
        }
        if (r.ano <= ANOS) {
            if (salva_reino(&r, ARQUIVO_SAVE)) {
                printf(
                    "(reinado salvo: pode sair com Ctrl+C e voltar depois)\n");
            } else {
                printf("(atenção: não foi possível salvar o reinado)\n");
            }
        }
    }
    reino_avaliacao(&r);
    remove(ARQUIVO_SAVE); /* o reinado acabou: o jogo salvo não serve mais */

    int pontos = reino_pontos(&r);
    printf("\nSua pontuação: %d\n", pontos);
    printf("Seu nome para o quadro de honra: ");
    char nome[32] = "";
    if (fgets(nome, sizeof nome, stdin) != NULL) {
        nome[strcspn(nome, "\n")] = '\0';
    }
    registra_recorde(nome[0] ? nome : "Anônimo", pontos, ARQUIVO_RECORDES);
    mostra_recordes(ARQUIVO_RECORDES, 5);
    return 0;
}
