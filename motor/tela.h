/* tela.h - a tela virtual do motor: uma grade de células desenhada fora do
   terminal e enviada de uma vez, só com o que mudou desde o último quadro. */
#ifndef TELA_H
#define TELA_H

#include <stdint.h>

/* Uma cor de 24 bits no formato 0xRRGGBB. */
typedef uint32_t Cor;
#define RGB(r, g, b)                                                           \
    ((Cor)((((uint32_t)(r) & 0xFF) << 16) | (((uint32_t)(g) & 0xFF) << 8) |    \
           ((uint32_t)(b) & 0xFF)))
#define COR_MANTEM 0xFF000000u /* não muda a cor que já está na célula */

/* A paleta do motor */
#define PRETO        RGB(0, 0, 0)
#define FUNDO        RGB(12, 14, 22)
#define PAINEL       RGB(22, 27, 40)
#define BRANCO       RGB(235, 236, 240)
#define CINZA        RGB(150, 156, 168)
#define CINZA_ESCURO RGB(72, 78, 94)
#define VERMELHO     RGB(232, 76, 72)
#define VERDE        RGB(96, 204, 112)
#define AZUL         RGB(84, 140, 240)
#define AMARELO      RGB(242, 208, 84)
#define LARANJA      RGB(246, 150, 60)
#define CIANO        RGB(84, 214, 224)
#define MAGENTA      RGB(212, 112, 222)
#define MARROM       RGB(150, 102, 62)

/* Cria a tela com o tamanho do jogo (em colunas e linhas). Devolve 0 se
   faltar memória. */
int tela_inicia(int largura, int altura);
void tela_encerra(void);
int tela_largura(void);
int tela_altura(void);

/* Modos de cor: 1 = 24 bits (padrão no Windows e em terminais modernos), 0 =
   256 cores. */
void tela_modo_cores(int verdadeiras);
/* Modo ASCII: troca molduras e blocos por + - | # para terminais sem
   Unicode. */
void tela_modo_ascii(int ligado);

/* ---- desenho na tela virtual (nada aparece até tela_mostra) ---- */
void tela_limpa(Cor fundo);
void tela_poe(int x, int y, uint32_t ch, Cor frente, Cor fundo);
uint32_t tela_caractere(int x, int y);
int tela_texto(int x, int y, const char *s, Cor frente, Cor fundo);
int tela_textof(int x, int y, Cor frente, Cor fundo, const char *formato, ...);
int tela_texto_dir(int x_fim, int y, const char *s, Cor frente, Cor fundo);
int tela_texto_centro(int x_centro, int y, const char *s, Cor frente,
                      Cor fundo);
void tela_preenche(int x, int y, int l, int a, uint32_t ch, Cor frente,
                   Cor fundo);
void tela_caixa(int x, int y, int l, int a, Cor cor, Cor fundo,
                const char *titulo);
void tela_barra(int x, int y, int l, double fracao, Cor cheia, Cor vazia);
/* Texto com quebra de linha automática e cores no meio do texto:
   "\001g" verde, "\001r" vermelho, "\001y" amarelo, "\001b" azul, "\001c"
   ciano,
   "\001m" magenta, "\001o" laranja, "\001w" branco, "\001k" cinza, "\001d" a
   cor padrão. Desenha no máximo 'a' linhas e devolve quantas linhas o texto
   precisa. */
int tela_quebra(int x, int y, int l, int a, const char *texto, Cor frente,
                Cor fundo);

/* Envia ao terminal o que mudou desde o último quadro. */
void tela_mostra(void);
/* Faz o próximo tela_mostra redesenhar tudo (depois de mudar o tamanho da
   janela, por exemplo). */
void tela_redesenha(void);
/* Grava a tela num arquivo de texto (uma célula por token
   "cp,frente,fundo,0"). */
int tela_salva(const char *arquivo);

/* ---- UTF-8 ---- */
/* lê um caractere e avança; 0 no fim */
uint32_t utf8_proximo(const char **s);
/* escreve 1 a 4 bytes; devolve quantos */
int utf8_codifica(uint32_t cp, char *saida);
/* quantos caracteres (não bytes) */
int utf8_largura(const char *s);

/* ---- cores ---- */
Cor cor_mistura(Cor a, Cor b, double t); /* t = 0 dá a, t = 1 dá b */
/* fator 0 = preto, 1 = a própria cor */
Cor cor_escurece(Cor c, double fator);

#endif
