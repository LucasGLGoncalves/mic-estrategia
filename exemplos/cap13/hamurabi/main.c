/* main.c - O Reino de Hamurabi, versão em vários arquivos */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "entrada.h"
#include "reino.h"

int main(void) {
    printf("=== O REINO DE HAMURABI ===\n");
    printf("Governe a antiga Suméria por %d anos.\n", ANOS);
    int semente = le_inteiro("Número do destino (0 para sortear): ", 0, 999999);
    srand(semente != 0 ? (unsigned)semente : (unsigned)time(NULL));

    Reino r;
    reino_novo(&r);
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
            return 0;
        }
    }
    reino_avaliacao(&r);
    return 0;
}
