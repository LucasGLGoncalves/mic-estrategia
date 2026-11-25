/* comandos2.c - o governo por comandos, agora com uma tabela de funções */
#include <stdio.h>
#include <string.h>

typedef struct {
    char nome[32];
    int metal;
    int minas;
    int fazendas;
    int turno;
} Colonia;

/* Cada comando é uma função que recebe a colônia (por ponteiro) e o argumento.
   Devolve 1 se o comando gastou um turno. */
typedef int (*FuncaoComando)(Colonia *c, const char *arg);

typedef struct {
    const char *nome;
    FuncaoComando executa;
    const char *ajuda;
} Comando;

int cmd_relatorio(Colonia *c, const char *arg) {
    (void)arg; /* este comando não usa o argumento */
    printf("%s: metal %d, minas %d, fazendas %d\n", c->nome, c->metal, c->minas,
           c->fazendas);
    return 0;
}

int cmd_construir(Colonia *c, const char *arg) {
    if (strcmp(arg, "mina") == 0 && c->metal >= 200) {
        c->metal -= 200;
        c->minas++;
        printf("Uma nova mina começa a funcionar.\n");
    } else if (strcmp(arg, "fazenda") == 0 && c->metal >= 120) {
        c->metal -= 120;
        c->fazendas++;
        printf("Uma nova fazenda é plantada.\n");
    } else {
        printf("Não dá para construir \"%s\" agora.\n", arg);
    }
    return 1;
}

int cmd_renomear(Colonia *c, const char *arg) {
    if (strlen(arg) == 0) {
        printf("Renomear para quê?\n");
        return 0;
    }
    snprintf(c->nome, sizeof c->nome, "%s", arg);
    printf("A colônia agora se chama %s.\n", c->nome);
    return 1;
}

int cmd_esperar(Colonia *c, const char *arg) {
    (void)c;
    (void)arg;
    printf("O tempo passa.\n");
    return 1;
}

int cmd_ajuda(Colonia *c, const char *arg);

const Comando COMANDOS[] = {
    {"ajuda", cmd_ajuda, "mostra esta lista"},
    {"relatorio", cmd_relatorio, "mostra o estado da colônia"},
    {"construir", cmd_construir, "construir mina|fazenda"},
    {"renomear", cmd_renomear, "renomear <novo nome>"},
    {"esperar", cmd_esperar, "deixa um turno passar"},
};
const int N_COMANDOS = sizeof COMANDOS / sizeof COMANDOS[0];

int cmd_ajuda(Colonia *c, const char *arg) {
    (void)c;
    (void)arg;
    for (int i = 0; i < N_COMANDOS; i++) {
        printf("  %-10s %s\n", COMANDOS[i].nome, COMANDOS[i].ajuda);
    }
    printf("  %-10s %s\n", "sair", "encerra o governo");
    return 0;
}

const Comando *procura(const char *nome) {
    for (int i = 0; i < N_COMANDOS; i++) {
        if (strcmp(COMANDOS[i].nome, nome) == 0) { return &COMANDOS[i]; }
    }
    return NULL;
}

int main(void) {
    Colonia col = {"Kepler", 500, 1, 1, 1};
    char linha[100];

    printf("Colônia %s pronta. Digite \"ajuda\" para ver os comandos.\n",
           col.nome);
    for (;;) {
        printf("\n[turno %d] %s> ", col.turno, col.nome);
        if (fgets(linha, sizeof linha, stdin) == NULL) break;
        linha[strcspn(linha, "\n")] = '\0';

        char nome[32] = "", arg[64] = "";
        sscanf(linha, "%31s %63[^\n]", nome, arg);
        if (strcmp(nome, "sair") == 0) break;

        const Comando *cmd = procura(nome);
        if (cmd == NULL) {
            printf("Comando desconhecido: \"%s\".\n", nome);
        } else if (cmd->executa(&col, arg)) {
            col.metal += col.minas * 75;
            col.turno++;
        }
    }
    printf("Fim do governo de %s, no turno %d.\n", col.nome, col.turno);
    return 0;
}
