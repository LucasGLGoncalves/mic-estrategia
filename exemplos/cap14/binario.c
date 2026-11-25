/* binario.c - gravando e lendo uma estrutura em formato binário */
#include <stdio.h>

typedef struct {
    int ano;
    int populacao;
    int graos;
    double soma_percent;
} Resumo;

int main(void) {
    Resumo r = {3, 112, 4830, 12.5};
    FILE *f = fopen("resumo.bin", "wb"); /* "b": modo binário */
    if (f == NULL) {
        printf("Não consegui criar o arquivo.\n");
        return 1;
    }
    size_t gravados = fwrite(&r, sizeof r, 1, f);
    fclose(f);
    printf("Gravei %zu estrutura de %zu bytes.\n", gravados, sizeof r);

    Resumo lido = {0};
    f = fopen("resumo.bin", "rb");
    if (f == NULL || fread(&lido, sizeof lido, 1, f) != 1) {
        printf("Não consegui ler o arquivo.\n");
        return 1;
    }
    fclose(f);
    printf("Li de volta: ano %d, população %d, grãos %d, %.1f%%\n", lido.ano,
           lido.populacao, lido.graos, lido.soma_percent);
    remove("resumo.bin");
    return 0;
}
