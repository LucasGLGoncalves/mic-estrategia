/* salvar.c - jogo salvo e quadro de honra */
#include "salvar.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSINATURA "HAMURABI-SAVE"
#define VERSAO     1

/* Grava primeiro num arquivo temporário e só no fim troca pelo definitivo:
   se o computador desligar no meio, o jogo salvo anterior continua intacto. */
int salva_reino(const Reino *r, const char *arquivo) {
    char temporario[256];
    snprintf(temporario, sizeof temporario, "%s.tmp", arquivo);
    FILE *f = fopen(temporario, "w");
    if (f == NULL) { return 0; }
    fprintf(f, "%s %d\n", ASSINATURA, VERSAO);
    fprintf(f, "ano %d\n", r->ano);
    fprintf(f, "populacao %d\n", r->populacao);
    fprintf(f, "graos %d\n", r->graos);
    fprintf(f, "terras %d\n", r->terras);
    fprintf(f, "preco_terra %d\n", r->preco_terra);
    fprintf(f, "colheita %d\n", r->colheita);
    fprintf(f, "ratos %d\n", r->ratos);
    fprintf(f, "nascidos %d\n", r->nascidos);
    fprintf(f, "mortos_fome %d\n", r->mortos_fome);
    fprintf(f, "mortos_peste %d\n", r->mortos_peste);
    fprintf(f, "total_mortos %d\n", r->total_mortos);
    fprintf(f, "soma_percent %.6f\n", r->soma_percent);
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        remove(temporario);
        return 0;
    }
    remove(arquivo); /* no Windows, rename falha se o destino já existe */
    return rename(temporario, arquivo) == 0;
}

int carrega_reino(Reino *r, const char *arquivo) {
    FILE *f = fopen(arquivo, "r");
    if (f == NULL) { return 0; }
    char assinatura[32];
    int versao;
    if (fscanf(f, "%31s %d", assinatura, &versao) != 2 ||
        strcmp(assinatura, ASSINATURA) != 0 || versao != VERSAO) {
        fclose(f);
        return 0;
    }
    Reino novo = {0};
    char chave[32];
    double valor;
    int lidos = 0;
    while (fscanf(f, "%31s %lf", chave, &valor) == 2) {
        if (strcmp(chave, "ano") == 0) novo.ano = (int)valor;
        else if (strcmp(chave, "populacao") == 0) novo.populacao = (int)valor;
        else if (strcmp(chave, "graos") == 0) novo.graos = (int)valor;
        else if (strcmp(chave, "terras") == 0) novo.terras = (int)valor;
        else if (strcmp(chave, "preco_terra") == 0)
            novo.preco_terra = (int)valor;
        else if (strcmp(chave, "colheita") == 0) novo.colheita = (int)valor;
        else if (strcmp(chave, "ratos") == 0) novo.ratos = (int)valor;
        else if (strcmp(chave, "nascidos") == 0) novo.nascidos = (int)valor;
        else if (strcmp(chave, "mortos_fome") == 0)
            novo.mortos_fome = (int)valor;
        else if (strcmp(chave, "mortos_peste") == 0)
            novo.mortos_peste = (int)valor;
        else if (strcmp(chave, "total_mortos") == 0)
            novo.total_mortos = (int)valor;
        else if (strcmp(chave, "soma_percent") == 0) novo.soma_percent = valor;
        /* chaves desconhecidas são ignoradas: arquivos futuros não quebram */
        lidos++;
    }
    fclose(f);
    if (lidos == 0 || novo.populacao <= 0 || novo.ano < 1 || novo.ano > ANOS) {
        return 0; /* arquivo corrompido ou incompleto */
    }
    *r = novo;
    return 1;
}

typedef struct {
    char nome[32];
    int pontos;
} Recorde;

static int compara_recordes(const void *a, const void *b) {
    const Recorde *ra = a;
    const Recorde *rb = b;
    return rb->pontos - ra->pontos; /* maior primeiro */
}

void registra_recorde(const char *nome, int pontos, const char *arquivo) {
    FILE *f = fopen(arquivo, "a"); /* "a": acrescenta no fim */
    if (f == NULL) { return; }
    fprintf(f, "%d %s\n", pontos, nome);
    fclose(f);
}

void mostra_recordes(const char *arquivo, int quantos) {
    FILE *f = fopen(arquivo, "r");
    if (f == NULL) {
        printf("Ainda não há reis no quadro de honra.\n");
        return;
    }
    Recorde *lista = NULL;
    int n = 0, cap = 0;
    char linha[128];
    while (fgets(linha, sizeof linha, f) != NULL) {
        Recorde rec;
        if (sscanf(linha, "%d %31[^\n]", &rec.pontos, rec.nome) != 2) continue;
        if (n == cap) {
            cap = cap ? cap * 2 : 8;
            Recorde *maior = realloc(lista, (size_t)cap * sizeof(Recorde));
            if (maior == NULL) break;
            lista = maior;
        }
        lista[n++] = rec;
    }
    fclose(f);
    qsort(lista, (size_t)n, sizeof(Recorde), compara_recordes);
    printf("\n===== QUADRO DE HONRA =====\n");
    for (int i = 0; i < n && i < quantos; i++) {
        printf("%2d. %-20s %5d\n", i + 1, lista[i].nome, lista[i].pontos);
    }
    free(lista);
}
