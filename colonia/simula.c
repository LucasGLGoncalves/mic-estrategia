/* simula.c - o administrador automático joga a colônia, sem tela, e mede
   quanto tempo de jogo leva para abrir o Portal Estelar. */
#include <stdio.h>

#include "piloto.h"

int main(void) {
    static Colonia c;
    colonia_nova(&c);
    for (int minuto = 5; !c.venceu && minuto <= 360; minuto += 5) {
        piloto_joga(&c, 300);
        Taxas t = colonia_taxas(&c);
        printf("%3d min: metal %5.1f/s  cristal %4.1f/s  ciência %3.1f/s  "
               "colonos %3.0f/%-3.0f"
               "  minas %d/%d  lab %d\n",
               minuto, t.producao[METAL], t.producao[CRISTAL],
               t.producao[CIENCIA], c.populacao, t.capacidade,
               c.nivel[MINA_METAL], c.nivel[MINA_CRISTAL],
               c.nivel[LABORATORIO]);
    }
    if (c.venceu)
        printf("Portal aberto em %.0f minutos de jogo.\n", c.relogio / 60);
    else printf("Seis horas e nada de portal.\n");
    return 0;
}
