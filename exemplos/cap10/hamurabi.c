/* hamurabi.c - O Reino de Hamurabi: governe a Suméria por dez anos */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ANOS             10
#define SACAS_POR_PESSOA 20 /* comida de uma pessoa por ano */
#define ACRES_POR_PESSOA 10 /* quanto uma pessoa consegue cultivar */

typedef struct {
    int ano;
    int populacao;
    int graos;       /* sacas no celeiro */
    int terras;      /* acres */
    int preco_terra; /* sacas por acre, neste ano */
    int colheita;    /* sacas colhidas por acre, no ano passado */
    int ratos;       /* sacas comidas pelos ratos no ano passado */
    int nascidos;    /* chegaram ou nasceram no ano passado */
    int mortos_fome; /* morreram de fome no ano passado */
    int mortos_peste;
    int total_mortos;    /* em todo o reinado */
    double soma_percent; /* soma dos percentuais de mortos de cada ano */
    int revolta;         /* 1 se o povo se revoltou */
} Reino;

typedef enum { EV_FARTURA, EV_RATOS, EV_PESTE, EV_QUANTOS } TipoEvento;

typedef struct {
    const char *nome;
    int chance; /* em porcentagem, por ano */
    const char *anuncio;
} Evento;

/* Os acontecimentos do reino são dados, não código: mudar a chance
   da peste ou reescrever um anúncio não exige mexer na lógica. */
const Evento EVENTOS[EV_QUANTOS] = {
    [EV_FARTURA] = {"fartura", 10,
                    "Os deuses abençoaram os campos: a colheita foi farta."},
    [EV_RATOS] = {"ratos", 40, "Os ratos invadiram o celeiro."},
    [EV_PESTE] = {"peste", 15,
                  "Uma peste terrível varreu a cidade: metade do povo morreu."},
};

int le_inteiro(const char *pergunta, int min, int max) {
    int valor;
    for (;;) {
        printf("%s", pergunta);
        int lidos = scanf("%d", &valor);
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        if (lidos == EOF) { exit(0); }
        if (lidos == 1 && valor >= min && valor <= max) { return valor; }
        printf("Digite um número de %d a %d.\n", min, max);
    }
}

int sorteia(int min, int max) { return min + rand() % (max - min + 1); }

Reino novo_reino(void) {
    Reino r = {0};
    r.ano = 1;
    r.populacao = 95;
    r.graos = 2800;
    r.terras = 1000;
    r.colheita = 3;
    r.ratos = 200;
    r.nascidos = 5;
    r.preco_terra = sorteia(17, 26);
    return r;
}

void relatorio(Reino r) {
    printf("\n====================================================\n");
    printf("Hamurabi, permita-me relatar. No ano %d do seu reinado:\n", r.ano);
    if (r.mortos_fome > 0)
        printf("  %d pessoas morreram de fome.\n", r.mortos_fome);
    if (r.mortos_peste > 0)
        printf("  A peste levou %d pessoas.\n", r.mortos_peste);
    printf("  %d pessoas chegaram à cidade.\n", r.nascidos);
    printf("  A população agora é de %d.\n", r.populacao);
    printf("  A cidade possui %d acres de terra.\n", r.terras);
    printf("  Colhemos %d saca%s por acre.\n", r.colheita,
           r.colheita == 1 ? "" : "s");
    printf("  Os ratos comeram %d sacas.\n", r.ratos);
    printf("  Há %d sacas no celeiro.\n", r.graos);
    printf("  A terra está valendo %d sacas por acre.\n", r.preco_terra);
    printf("====================================================\n");
}

/* Sorteia se um evento acontece este ano, segundo a chance da tabela. */
int acontece(TipoEvento t) { return sorteia(1, 100) <= EVENTOS[t].chance; }

/* Aplica um evento ao reino e devolve o reino modificado. */
Reino aplica_evento(Reino r, TipoEvento t) {
    printf("  >> %s\n", EVENTOS[t].anuncio);
    switch (t) {
    case EV_PESTE:
        r.mortos_peste = r.populacao / 2;
        r.populacao -= r.mortos_peste;
        break;
    case EV_RATOS:
        r.ratos = r.graos / sorteia(3, 6);
        r.graos -= r.ratos;
        break;
    case EV_FARTURA: r.colheita += 2; break;
    case EV_QUANTOS: break;
    }
    return r;
}

/* O ano passa: colheita, ratos, fome, nascimentos, peste. */
Reino fim_do_ano(Reino r, int comida, int plantados) {
    int pop_inicio = r.populacao;
    r.ratos = 0;
    r.mortos_peste = 0;

    r.colheita = sorteia(1, 5);
    if (acontece(EV_FARTURA)) r = aplica_evento(r, EV_FARTURA);
    r.graos += plantados * r.colheita;
    if (acontece(EV_RATOS)) r = aplica_evento(r, EV_RATOS);

    int alimentados = comida / SACAS_POR_PESSOA;
    r.mortos_fome = r.populacao > alimentados ? r.populacao - alimentados : 0;
    r.revolta = r.mortos_fome * 100 > pop_inicio * 45;
    r.soma_percent += 100.0 * r.mortos_fome / pop_inicio;
    r.total_mortos += r.mortos_fome;
    r.nascidos =
        sorteia(1, 5) * (20 * r.terras + r.graos) / r.populacao / 100 + 1;
    r.populacao += r.nascidos - r.mortos_fome;

    if (acontece(EV_PESTE)) {
        r = aplica_evento(r, EV_PESTE);
        r.total_mortos += r.mortos_peste;
    }
    r.ano++;
    r.preco_terra = sorteia(17, 26);
    return r;
}

void avaliacao(Reino r) {
    double media = r.soma_percent / ANOS;
    double acres = (double)r.terras / r.populacao;
    printf("\nAo fim do seu reinado, a cidade tem %d pessoas, %d acres e %d "
           "sacas.\n",
           r.populacao, r.terras, r.graos);
    printf("Em %d anos, uma média de %.1f%% do povo morreu de fome por ano,\n",
           ANOS, media);
    printf("num total de %d mortos de fome e de peste. Restam %.1f acres por "
           "pessoa.\n\n",
           r.total_mortos, acres);
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

int main(void) {
    printf("=== O REINO DE HAMURABI ===\n");
    printf("Governe a antiga Suméria por %d anos.\n", ANOS);
    int semente = le_inteiro("Número do destino (0 para sortear): ", 0, 999999);
    srand(semente != 0 ? (unsigned)semente : (unsigned)time(NULL));

    Reino r = novo_reino();
    while (r.ano <= ANOS) {
        relatorio(r);

        /* 1. comprar ou vender terras */
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

        /* 2. alimentar o povo */
        int comida =
            le_inteiro("Quantas sacas para alimentar o povo? ", 0, r.graos);
        r.graos -= comida;

        /* 3. plantar: meia saca por acre, até 10 acres por pessoa */
        int limite = r.terras;
        if (limite > r.graos * 2) limite = r.graos * 2;
        if (limite > r.populacao * ACRES_POR_PESSOA)
            limite = r.populacao * ACRES_POR_PESSOA;
        printf("(você pode plantar até %d acres)\n", limite);
        int plantar = le_inteiro("Quantos acres deseja plantar? ", 0, limite);
        r.graos -= (plantar + 1) / 2;

        r = fim_do_ano(r, comida, plantar);
        if (r.revolta) {
            printf("\nVocê deixou %d pessoas morrerem de fome em um ano!\n",
                   r.mortos_fome);
            printf("O povo se revoltou e você foi deposto.\n");
            return 0;
        }
    }
    avaliacao(r);
    return 0;
}
