/* tela.c - a tela virtual do motor */
#include "tela.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term.h"

typedef struct {
    uint32_t ch;
    Cor frente, fundo;
} Celula;

static Celula *atual = NULL;    /* o quadro que estamos desenhando */
static Celula *anterior = NULL; /* o que já está no terminal */
static int larg = 0, alt = 0;
static int redesenhar = 1;
static int cores_24 = 1;
static int ascii = 0;
static int term_l = 0, term_a = 0;

int tela_inicia(int largura, int altura) {
    tela_encerra();
    atual = calloc((size_t)largura * (size_t)altura, sizeof(Celula));
    anterior = calloc((size_t)largura * (size_t)altura, sizeof(Celula));
    if (atual == NULL || anterior == NULL) {
        tela_encerra();
        return 0;
    }
    larg = largura;
    alt = altura;
#ifndef _WIN32
    const char *ct = getenv("COLORTERM");
    cores_24 = ct != NULL &&
               (strstr(ct, "truecolor") != NULL || strstr(ct, "24bit") != NULL);
#endif
    tela_limpa(FUNDO);
    redesenhar = 1;
    return 1;
}

void tela_encerra(void) {
    free(atual);
    free(anterior);
    atual = anterior = NULL;
    larg = alt = 0;
}

int tela_largura(void) { return larg; }
int tela_altura(void) { return alt; }
void tela_modo_cores(int verdadeiras) {
    cores_24 = verdadeiras;
    redesenhar = 1;
}
void tela_modo_ascii(int ligado) {
    ascii = ligado;
    redesenhar = 1;
}
void tela_redesenha(void) { redesenhar = 1; }

/* ------------------------------------------------------------------ UTF-8 */
uint32_t utf8_proximo(const char **s) {
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t c = p[0];
    if (c == 0) return 0;
    int extras = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
    if (extras > 0) c &= 0x3Fu >> extras;
    int i = 1;
    for (; i <= extras && (p[i] & 0xC0) == 0x80; i++)
        c = (c << 6) | (p[i] & 0x3F);
    *s += i;
    return c;
}

int utf8_codifica(uint32_t cp, char *o) {
    if (cp < 0x80) {
        o[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        o[0] = (char)(0xC0 | (cp >> 6));
        o[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        o[0] = (char)(0xE0 | (cp >> 12));
        o[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        o[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    o[0] = (char)(0xF0 | (cp >> 18));
    o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    o[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    o[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

int utf8_largura(const char *s) {
    int n = 0;
    while (utf8_proximo(&s)) n++;
    return n;
}

/* ------------------------------------------------------------------ cores */
Cor cor_mistura(Cor a, Cor b, double t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int ra = (int)(a >> 16 & 0xFF), ga = (int)(a >> 8 & 0xFF),
        ba = (int)(a & 0xFF);
    int rb = (int)(b >> 16 & 0xFF), gb = (int)(b >> 8 & 0xFF),
        bb = (int)(b & 0xFF);
    return RGB(ra + (int)((rb - ra) * t), ga + (int)((gb - ga) * t),
               ba + (int)((bb - ba) * t));
}

Cor cor_escurece(Cor c, double fator) { return cor_mistura(PRETO, c, fator); }

/* ------------------------------------------------------------------ desenho */
void tela_limpa(Cor fundo) {
    for (int i = 0; i < larg * alt; i++)
        atual[i] = (Celula){' ', BRANCO, fundo};
}

void tela_poe(int x, int y, uint32_t ch, Cor frente, Cor fundo) {
    /* fora da tela: ignora */
    if (x < 0 || y < 0 || x >= larg || y >= alt) return;
    Celula *c = &atual[y * larg + x];
    if (ch < 32 || ch == 127) ch = ' ';
    c->ch = ch;
    if (frente != COR_MANTEM) c->frente = frente;
    if (fundo != COR_MANTEM) c->fundo = fundo;
}

uint32_t tela_caractere(int x, int y) {
    if (x < 0 || y < 0 || x >= larg || y >= alt) return 0;
    return atual[y * larg + x].ch;
}

int tela_texto(int x, int y, const char *s, Cor frente, Cor fundo) {
    int n = 0;
    uint32_t c;
    while ((c = utf8_proximo(&s)) != 0) {
        tela_poe(x + n, y, c, frente, fundo);
        n++;
    }
    return n;
}

int tela_textof(int x, int y, Cor frente, Cor fundo, const char *formato, ...) {
    char buf[512];
    va_list args;
    va_start(args, formato);
    vsnprintf(buf, sizeof buf, formato, args);
    va_end(args);
    return tela_texto(x, y, buf, frente, fundo);
}

int tela_texto_dir(int x_fim, int y, const char *s, Cor frente, Cor fundo) {
    return tela_texto(x_fim - utf8_largura(s) + 1, y, s, frente, fundo);
}

int tela_texto_centro(int x_centro, int y, const char *s, Cor frente,
                      Cor fundo) {
    return tela_texto(x_centro - utf8_largura(s) / 2, y, s, frente, fundo);
}

void tela_preenche(int x, int y, int l, int a, uint32_t ch, Cor frente,
                   Cor fundo) {
    for (int j = y; j < y + a; j++)
        for (int i = x; i < x + l; i++) tela_poe(i, j, ch, frente, fundo);
}

void tela_caixa(int x, int y, int l, int a, Cor cor, Cor fundo,
                const char *titulo) {
    if (l < 2 || a < 2) return;
    tela_preenche(x + 1, y + 1, l - 2, a - 2, ' ', cor, fundo);
    for (int i = x + 1; i < x + l - 1; i++) {
        tela_poe(i, y, 0x2500, cor, fundo); /* ─ */
        tela_poe(i, y + a - 1, 0x2500, cor, fundo);
    }
    for (int j = y + 1; j < y + a - 1; j++) {
        tela_poe(x, j, 0x2502, cor, fundo); /* │ */
        tela_poe(x + l - 1, j, 0x2502, cor, fundo);
    }
    tela_poe(x, y, 0x250C, cor, fundo);                 /* ┌ */
    tela_poe(x + l - 1, y, 0x2510, cor, fundo);         /* ┐ */
    tela_poe(x, y + a - 1, 0x2514, cor, fundo);         /* └ */
    tela_poe(x + l - 1, y + a - 1, 0x2518, cor, fundo); /* ┘ */
    if (titulo != NULL && titulo[0] != '\0') {
        char buf[128];
        snprintf(buf, sizeof buf, " %s ", titulo);
        tela_texto(x + 2, y, buf, AMARELO, fundo);
    }
}

void tela_barra(int x, int y, int l, double fracao, Cor cheia, Cor vazia) {
    static const uint32_t partes[8] = {' ',    0x258F, 0x258E, 0x258D,
                                       0x258C, 0x258B, 0x258A, 0x2589};
    if (fracao < 0) fracao = 0;
    if (fracao > 1) fracao = 1;
    /* a barra cresce de oitavo em oitavo de célula */
    int oitavos = (int)(fracao * l * 8 + 0.5);
    for (int i = 0; i < l; i++) {
        int k = oitavos - i * 8;
        if (k >= 8) tela_poe(x + i, y, 0x2588, cheia, vazia); /* █ */
        /* ▏▎▍▌▋▊▉ */
        else if (k > 0) tela_poe(x + i, y, partes[k], cheia, vazia);
        else tela_poe(x + i, y, ' ', cheia, vazia);
    }
}

static Cor cor_da_marca(char m, Cor padrao) {
    switch (m) {
    case 'g': return VERDE;
    case 'r': return VERMELHO;
    case 'y': return AMARELO;
    case 'b': return AZUL;
    case 'c': return CIANO;
    case 'm': return MAGENTA;
    case 'o': return LARANJA;
    case 'w': return BRANCO;
    case 'k': return CINZA;
    default: return padrao;
    }
}

int tela_quebra(int x, int y, int l, int a, const char *texto, Cor frente,
                Cor fundo) {
    int linha = 0, col = 0;
    Cor cor = frente;
    const char *p = texto;
    while (*p) {
        if (*p == '\n') {
            linha++;
            col = 0;
            p++;
            continue;
        }
        if (*p == '\001' && p[1]) {
            cor = cor_da_marca(p[1], frente);
            p += 2;
            continue;
        }
        if (*p == ' ') {
            if (col > 0 && col < l) col++;
            p++;
            continue;
        }
        /* mede a próxima palavra, ignorando as marcas de cor */
        const char *q = p;
        int tam = 0;
        while (*q && *q != ' ' && *q != '\n') {
            if (*q == '\001' && q[1]) {
                q += 2;
                continue;
            }
            utf8_proximo(&q);
            tam++;
        }
        if (col > 0 && col + tam > l) {
            linha++;
            col = 0;
        }
        while (p < q) {
            if (*p == '\001' && p[1]) {
                cor = cor_da_marca(p[1], frente);
                p += 2;
                continue;
            }
            uint32_t c = utf8_proximo(&p);
            if (col >= l) { /* palavra maior que a linha: corta */
                linha++;
                col = 0;
            }
            if (linha < a) tela_poe(x + col, y + linha, c, cor, fundo);
            col++;
        }
    }
    return linha + 1;
}

/* ------------------------------------------------------------------ saída */
static uint32_t para_ascii(uint32_t c) {
    switch (c) {
    case 0x2500:
    case 0x2550: return '-';
    case 0x2502:
    case 0x2551: return '|';
    case 0x250C:
    case 0x2510:
    case 0x2514:
    case 0x2518:
    case 0x251C:
    case 0x2524:
    case 0x252C:
    case 0x2534:
    case 0x253C:
    case 0x2554:
    case 0x2557:
    case 0x255A:
    case 0x255D: return '+';
    case 0x2588:
    case 0x2589:
    case 0x258A:
    case 0x258B:
    case 0x2593: return '#';
    case 0x258C:
    case 0x258D:
    case 0x2592: return '=';
    case 0x258E:
    case 0x258F:
    case 0x2591: return ':';
    case 0x2580:
    case 0x2584: return '#';
    case 0x00B7:
    case 0x2022: return '.';
    default: return c < 128 ? c : '?';
    }
}

/* Converte uma cor de 24 bits na mais próxima das 256 cores do xterm. */
static int cor_256(Cor c) {
    int r = (int)(c >> 16 & 0xFF), g = (int)(c >> 8 & 0xFF),
        b = (int)(c & 0xFF);
    /* cinza: usa a rampa de 24 tons */
    if (abs(r - g) < 10 && abs(g - b) < 10) {
        if (r < 8) return 16;
        if (r > 238) return 231;
        return 232 + (r - 8) / 10;
    }
    int ri = (r * 5 + 127) / 255, gi = (g * 5 + 127) / 255,
        bi = (b * 5 + 127) / 255;
    return 16 + 36 * ri + 6 * gi + bi;
}

static void emite_cor(Cor c, int de_fundo) {
    char buf[32];
    int n;
    if (cores_24) {
        n = snprintf(buf, sizeof buf, "\x1b[%d;2;%u;%u;%um", de_fundo ? 48 : 38,
                     (unsigned)(c >> 16 & 0xFF), (unsigned)(c >> 8 & 0xFF),
                     (unsigned)(c & 0xFF));
    } else {
        n = snprintf(buf, sizeof buf, "\x1b[%d;5;%dm", de_fundo ? 48 : 38,
                     cor_256(c));
    }
    term_escreve(buf, (size_t)n);
}

void tela_mostra(void) {
    if (atual == NULL) return;
    int tl, ta;
    term_tamanho(&tl, &ta);
    /* a janela mudou de tamanho: limpa e redesenha */
    if (tl != term_l || ta != term_a) {
        term_l = tl;
        term_a = ta;
        term_escreve("\x1b[0m\x1b[2J", 8);
        redesenhar = 1;
    }
    if (tl < larg || ta < alt) {
        char aviso[160];
        int n = snprintf(
            aviso, sizeof aviso,
            "\x1b[H\x1b[0m\x1b[2JAumente a janela para %dx%d (agora: %dx%d)",
            larg, alt, tl, ta);
        term_escreve(aviso, (size_t)n);
        term_descarrega();
        redesenhar = 1;
        return;
    }
    int cx = -1, cy = -1;
    /* cores atuais do terminal: desconhecidas */
    Cor fr = 0xFFFFFFFFu, fu = 0xFFFFFFFFu;
    char buf[32];
    for (int y = 0; y < alt; y++) {
        for (int x = 0; x < larg; x++) {
            Celula *c = &atual[y * larg + x];
            Celula *a = &anterior[y * larg + x];
            if (!redesenhar && c->ch == a->ch && c->frente == a->frente &&
                c->fundo == a->fundo)
                continue;
            if (cx != x || cy != y) {
                int n = snprintf(buf, sizeof buf, "\x1b[%d;%dH", y + 1, x + 1);
                term_escreve(buf, (size_t)n);
            }
            if (c->fundo != fu) {
                emite_cor(c->fundo, 1);
                fu = c->fundo;
            }
            if (c->frente != fr) {
                emite_cor(c->frente, 0);
                fr = c->frente;
            }
            uint32_t ch = ascii ? para_ascii(c->ch) : c->ch;
            int n = utf8_codifica(ch, buf);
            term_escreve(buf, (size_t)n);
            cx = x + 1;
            cy = y;
            *a = *c;
        }
    }
    redesenhar = 0;
    term_descarrega();
}

int tela_salva(const char *arquivo) {
    FILE *f = fopen(arquivo, "w");
    if (f == NULL) return 0;
    for (int y = 0; y < alt; y++) {
        for (int x = 0; x < larg; x++) {
            Celula *c = &atual[y * larg + x];
            fprintf(f, "%s%x,%06x,%06x,0", x ? " " : "", (unsigned)c->ch,
                    (unsigned)(c->frente & 0xFFFFFF),
                    (unsigned)(c->fundo & 0xFFFFFF));
        }
        fputc('\n', f);
    }
    return fclose(f) == 0;
}
