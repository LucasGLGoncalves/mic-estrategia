/* entrada.h - leitura do teclado e sorteios */
#ifndef ENTRADA_H
#define ENTRADA_H

/* Lê um número inteiro entre min e max, repetindo a pergunta até dar certo. */
int le_inteiro(const char *pergunta, int min, int max);

/* Devolve um número aleatório entre min e max, inclusive. */
int sorteia(int min, int max);

#endif
