/* preproc.c - macros, compilação condicional e uma tabela gerada por macros */
#include <stdio.h>

#define MAX_PLANETAS      9
#define MIN(a, b)         ((a) < (b) ? (a) : (b))
#define LIMITA(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define QUADRADO_RUIM(x)  x *x
#define QUADRADO(x)       ((x) * (x))

#if defined(_WIN32)
#define SISTEMA "Windows"
#elif defined(__APPLE__)
#define SISTEMA "macOS"
#elif defined(__linux__)
#define SISTEMA "Linux"
#else
#define SISTEMA "desconhecido"
#endif

/* Uma lista de recursos escrita uma única vez... */
#define RECURSOS(X)                                                            \
    X(METAL, "Metal", 60)                                                      \
    X(CRISTAL, "Cristal", 48)                                                  \
    X(DEUTERIO, "Deutério", 225)

/* ...que vira um enum... */
#define COMO_ENUM(id, nome, custo) R_##id,
enum { RECURSOS(COMO_ENUM) N_RECURSOS };

/* ...e uma tabela de nomes e custos, sempre na mesma ordem. */
#define COMO_LINHA(id, nome, custo) {#id, nome, custo},
struct {
    const char *codigo;
    const char *nome;
    int custo;
} TABELA[N_RECURSOS] = {RECURSOS(COMO_LINHA)};

int main(void) {
    printf("Compilado para %s, em %s, na linha %d de %s\n", SISTEMA, __DATE__,
           __LINE__, __FILE__);
    printf("MIN(4, 7) = %d, LIMITA(15, 0, MAX_PLANETAS) = %d\n", MIN(4, 7),
           LIMITA(15, 0, MAX_PLANETAS));
    printf("QUADRADO_RUIM(2 + 1) = %d, mas QUADRADO(2 + 1) = %d\n",
           QUADRADO_RUIM(2 + 1), QUADRADO(2 + 1));
    for (int i = 0; i < N_RECURSOS; i++) {
        printf("%d: %-9s %-8s custo base %d\n", i, TABELA[i].codigo,
               TABELA[i].nome, TABELA[i].custo);
    }
#ifndef NDEBUG
    printf("(versão de depuração: compile com -DNDEBUG para a versão final)\n");
#endif
    return 0;
}
