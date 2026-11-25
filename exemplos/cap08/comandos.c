/* comandos.c - uma colônia governada por comandos de texto */
#include <stdio.h>
#include <string.h>

/* Lê uma linha inteira do teclado, sem o '\n' do fim.
   Devolve 0 se a entrada acabou. */
int le_linha(char *destino, int tamanho) {
    if (fgets(destino, tamanho, stdin) == NULL) { return 0; }
    destino[strcspn(destino, "\n")] = '\0';
    return 1;
}

int main(void) {
    char colonia[32] = "Kepler";
    char linha[100];
    int metal = 500, minas = 1, fazendas = 1, turno = 1;

    printf("Colônia %s pronta. Digite \"ajuda\" para ver os comandos.\n",
           colonia);
    for (;;) {
        printf("\n[turno %d] %s> ", turno, colonia);
        if (!le_linha(linha, sizeof linha)) { break; }
        char comando[32] = "";
        char argumento[64] = "";
        sscanf(linha, "%31s %63[^\n]", comando, argumento);

        if (strcmp(comando, "ajuda") == 0) {
            printf("Comandos: relatorio, construir mina, construir fazenda,\n");
            printf("          renomear <novo nome>, esperar, sair\n");
            continue; /* pedir ajuda não gasta um turno */
        } else if (strcmp(comando, "relatorio") == 0) {
            printf(
                "%s: metal %d, minas %d, fazendas %d (nome com %zu letras)\n",
                colonia, metal, minas, fazendas, strlen(colonia));
            continue;
        } else if (strcmp(comando, "construir") == 0) {
            if (strcmp(argumento, "mina") == 0 && metal >= 200) {
                metal -= 200;
                minas++;
                printf("Uma nova mina começa a funcionar.\n");
            } else if (strcmp(argumento, "fazenda") == 0 && metal >= 120) {
                metal -= 120;
                fazendas++;
                printf("Uma nova fazenda é plantada.\n");
            } else {
                printf("Não dá para construir \"%s\" agora.\n", argumento);
            }
        } else if (strcmp(comando, "renomear") == 0 && strlen(argumento) > 0) {
            strncpy(colonia, argumento, sizeof colonia - 1);
            colonia[sizeof colonia - 1] = '\0';
            printf("A colônia agora se chama %s.\n", colonia);
        } else if (strcmp(comando, "esperar") == 0) {
            printf("O tempo passa.\n");
        } else if (strcmp(comando, "sair") == 0) {
            break;
        } else {
            printf("Comando desconhecido: \"%s\".\n", linha);
            continue;
        }
        metal += minas * 75; /* produção do turno */
        turno++;
    }
    printf("Fim do governo de %s, no turno %d.\n", colonia, turno);
    return 0;
}
