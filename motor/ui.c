/* ui.c - peças de interface para jogos de estratégia */
#include "ui.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term.h"

/* ------------------------------------------------------------------ números */
void fmt_num(char *saida, size_t tamanho, double valor) {
    char digitos[32];
    long long v = llround(valor);
    int negativo = v < 0;
    unsigned long long u =
        negativo ? (unsigned long long)(-(v + 1)) + 1 : (unsigned long long)v;
    snprintf(digitos, sizeof digitos, "%llu", u);
    char buf[48];
    int n = 0, len = (int)strlen(digitos);
    if (negativo) buf[n++] = '-';
    for (int i = 0; i < len; i++) {
        /* ponto a cada três algarismos */
        if (i > 0 && (len - i) % 3 == 0) buf[n++] = '.';
        buf[n++] = digitos[i];
    }
    buf[n] = '\0';
    snprintf(saida, tamanho, "%s", buf);
}

void fmt_curto(char *saida, size_t tamanho, double valor) {
    double a = fabs(valor);
    const char *sufixo = "";
    double v = valor;
    if (a >= 1e9) {
        v = valor / 1e9;
        sufixo = "G";
    } else if (a >= 1e6) {
        v = valor / 1e6;
        sufixo = "M";
    } else if (a >= 1e4) {
        v = valor / 1e3;
        sufixo = "k";
    } else {
        fmt_num(saida, tamanho, valor);
        return;
    }
    char buf[32];
    snprintf(buf, sizeof buf,
             fabs(v) < 10    ? "%.2f%s"
             : fabs(v) < 100 ? "%.1f%s"
                             : "%.0f%s",
             v, sufixo);
    for (char *p = buf; *p; p++)
        if (*p == '.') *p = ','; /* vírgula decimal */
    snprintf(saida, tamanho, "%s", buf);
}

void fmt_tempo(char *saida, size_t tamanho, double segundos) {
    long long s = (long long)ceil(segundos < 0 ? 0 : segundos);
    long long d = s / 86400, h = s % 86400 / 3600, m = s % 3600 / 60,
              ss = s % 60;
    if (d > 0) snprintf(saida, tamanho, "%lldd %02lldh", d, h);
    else if (h > 0) snprintf(saida, tamanho, "%lldh %02lldm", h, m);
    else if (m > 0) snprintf(saida, tamanho, "%lldm %02llds", m, ss);
    else snprintf(saida, tamanho, "%llds", ss);
}

void fmt_ajusta(char *saida, size_t tamanho, const char *texto, int largura) {
    size_t n = 0;
    int col = 0;
    const char *p = texto;
    while (*p && col < largura) {
        const char *ini = p;
        utf8_proximo(&p);
        size_t bytes = (size_t)(p - ini);
        if (n + bytes + 1 > tamanho) break;
        memcpy(saida + n, ini, bytes);
        n += bytes;
        col++;
    }
    while (col < largura && n + 2 <= tamanho) {
        saida[n++] = ' ';
        col++;
    }
    saida[n] = '\0';
}

/* ------------------------------------------------------------------ listas */
void lista_ajusta(Lista *l, int n, int visiveis) {
    if (l->sel >= n) l->sel = n - 1;
    if (l->sel < 0) l->sel = 0;
    if (l->sel < l->topo) l->topo = l->sel;
    if (l->sel >= l->topo + visiveis) l->topo = l->sel - visiveis + 1;
    if (l->topo > n - visiveis) l->topo = n - visiveis;
    if (l->topo < 0) l->topo = 0;
}

int lista_tecla(Lista *l, int tecla, int n, int visiveis) {
    switch (tecla) {
    /* dá a volta */
    case TECLA_CIMA: l->sel = l->sel > 0 ? l->sel - 1 : n - 1; break;
    case TECLA_BAIXO: l->sel = l->sel < n - 1 ? l->sel + 1 : 0; break;
    case TECLA_PGUP: l->sel -= visiveis; break;
    case TECLA_PGDN: l->sel += visiveis; break;
    case TECLA_HOME: l->sel = 0; break;
    case TECLA_END: l->sel = n - 1; break;
    default: return 0;
    }
    lista_ajusta(l, n, visiveis);
    return 1;
}

void lista_desenha(int x, int y, int l, int a, const char *const *itens, int n,
                   const Lista *ls) {
    for (int i = 0; i < a && ls->topo + i < n; i++) {
        int k = ls->topo + i;
        int sel = k == ls->sel;
        Cor fundo = sel ? RGB(40, 60, 100) : COR_MANTEM;
        tela_preenche(x, y + i, l, 1, ' ', BRANCO, fundo);
        tela_texto(x, y + i, sel ? "▶ " : "  ", AMARELO, COR_MANTEM);
        tela_texto(x + 2, y + i, itens[k], sel ? BRANCO : CINZA, COR_MANTEM);
    }
    /* ▲ */
    if (ls->topo > 0) tela_poe(x + l - 1, y, 0x25B2, CINZA, COR_MANTEM);
    /* ▼ */
    if (ls->topo + a < n)
        tela_poe(x + l - 1, y + a - 1, 0x25BC, CINZA, COR_MANTEM);
}

/* ------------------------------------------------------------------ abas */
void ui_abas(int x, int y, const char *const *nomes, int n, int atual) {
    for (int i = 0; i < n; i++) {
        char buf[64];
        snprintf(buf, sizeof buf, " %s ", nomes[i]);
        Cor fr = i == atual ? PRETO : CINZA;
        Cor fu = i == atual ? AMARELO : PAINEL;
        x += tela_texto(x, y, buf, fr, fu) + 1;
    }
}

/* ------------------------------------------------------------------ diário */
void diario_poe(Diario *d, const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    vsnprintf(d->texto[d->total % DIARIO_MAX], sizeof d->texto[0], formato,
              args);
    va_end(args);
    d->total++;
}

void diario_desenha(const Diario *d, int x, int y, int l, int a) {
    int n = d->total < DIARIO_MAX ? d->total : DIARIO_MAX;
    /* de trás para a frente: quantas mensagens recentes cabem nas 'a'
       linhas? */
    int linhas = 0, quantas = 0;
    while (quantas < n) {
        int k = (d->total - 1 - quantas) % DIARIO_MAX;
        /* só mede */
        int h = tela_quebra(0, 0, l, 0, d->texto[k], BRANCO, COR_MANTEM);
        if (linhas + h > a) break;
        linhas += h;
        quantas++;
    }
    /* agora desenha da mais antiga para a mais nova */
    for (int i = quantas - 1; i >= 0; i--) {
        int k = (d->total - 1 - i) % DIARIO_MAX;
        Cor c = i == 0 ? BRANCO : CINZA;
        y += tela_quebra(x, y, l, a, d->texto[k], c, COR_MANTEM);
    }
}

/* ------------------------------------------------------------------ janelas */
/* Desenha uma caixa centralizada e devolve as coordenadas do seu interior. */
static void janela(const char *titulo, int l, int a, int *x, int *y) {
    *x = (tela_largura() - l) / 2;
    *y = (tela_altura() - a) / 2;
    tela_caixa(*x, *y, l, a, AMARELO, PAINEL, titulo);
}

static int espera(void) {
    for (;;) {
        int t = term_espera_tecla(200);
        if (t) return t;
    }
}

void ui_aviso(const char *titulo, const char *texto) {
    int l = tela_largura() * 3 / 5, x, y;
    /* só mede */
    int linhas = tela_quebra(0, 0, l - 4, 0, texto, BRANCO, COR_MANTEM);
    janela(titulo, l, linhas + 4, &x, &y);
    tela_quebra(x + 2, y + 1, l - 4, linhas, texto, BRANCO, PAINEL);
    tela_texto_dir(x + l - 3, y + linhas + 2, "Enter", CINZA, PAINEL);
    tela_mostra();
    for (;;) {
        int t = espera();
        if (t == TECLA_ENTER || t == TECLA_ESC || t == ' ' || t == TECLA_SAIR)
            return;
    }
}

int ui_confirma(const char *titulo, const char *pergunta) {
    int l = tela_largura() / 2, x, y;
    int linhas = tela_quebra(0, 0, l - 4, 0, pergunta, BRANCO, COR_MANTEM);
    janela(titulo, l, linhas + 4, &x, &y);
    tela_quebra(x + 2, y + 1, l - 4, linhas, pergunta, BRANCO, PAINEL);
    tela_texto_dir(x + l - 3, y + linhas + 2, "S = sim   N = não", CINZA,
                   PAINEL);
    tela_mostra();
    for (;;) {
        int t = espera();
        if (t == 's' || t == 'S' || t == TECLA_ENTER) return 1;
        if (t == 'n' || t == 'N' || t == TECLA_ESC || t == TECLA_SAIR) return 0;
    }
}

int ui_escolha(const char *titulo, const char *const *opcoes, int n) {
    int l = 10, x, y;
    for (int i = 0; i < n; i++) {
        int w = utf8_largura(opcoes[i]) + 6;
        if (w > l) l = w;
    }
    int a = n < 15 ? n : 15;
    Lista ls = {0, 0};
    for (;;) {
        janela(titulo, l, a + 2, &x, &y);
        lista_desenha(x + 1, y + 1, l - 2, a, opcoes, n, &ls);
        tela_mostra();
        int t = espera();
        if (t == TECLA_ENTER) return ls.sel;
        if (t == TECLA_ESC || t == TECLA_SAIR) return -1;
        lista_tecla(&ls, t, n, a);
    }
}

int ui_le_texto(const char *titulo, const char *rotulo, char *buf,
                int tamanho) {
    int l = tela_largura() / 2, x, y;
    int len = (int)strlen(buf);
    for (;;) {
        janela(titulo, l, 5, &x, &y);
        tela_texto(x + 2, y + 1, rotulo, CINZA, PAINEL);
        tela_preenche(x + 2, y + 2, l - 4, 1, ' ', BRANCO, FUNDO);
        int w = tela_texto(x + 2, y + 2, buf, BRANCO, FUNDO);
        tela_poe(x + 2 + w, y + 2, '_', AMARELO, FUNDO);
        tela_texto_dir(x + l - 3, y + 3, "Enter confirma, Esc desiste", CINZA,
                       PAINEL);
        tela_mostra();
        int t = espera();
        if (t == TECLA_ENTER) return 1;
        if (t == TECLA_ESC || t == TECLA_SAIR) return 0;
        if (t == TECLA_APAGA && len > 0) {
            /* apaga um caractere inteiro, mesmo que ele tenha vários bytes */
            do {
                len--;
            } while (len > 0 && ((unsigned char)buf[len] & 0xC0) == 0x80);
            buf[len] = '\0';
        } else if (t >= 32 && t < 0x110000 && utf8_largura(buf) < l - 6) {
            char c[4];
            int n = utf8_codifica((uint32_t)t, c);
            if (len + n < tamanho) {
                memcpy(buf + len, c, (size_t)n);
                len += n;
                buf[len] = '\0';
            }
        }
    }
}

int ui_le_numero(const char *titulo, const char *rotulo, int min, int max,
                 int *valor) {
    char buf[16] = "";
    for (;;) {
        char r[128];
        snprintf(r, sizeof r, "%s (%d a %d)", rotulo, min, max);
        if (!ui_le_texto(titulo, r, buf, sizeof buf)) return 0;
        char *fim;
        long v = strtol(buf, &fim, 10);
        if (buf[0] != '\0' && *fim == '\0' && v >= min && v <= max) {
            *valor = (int)v;
            return 1;
        }
        buf[0] = '\0';
    }
}
