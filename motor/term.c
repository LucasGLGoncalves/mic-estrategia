/* term.c - a camada de terminal do motor (Windows e POSIX) */
#if !defined(_WIN32)
/* libera as funções POSIX mesmo com -std=c11 */
#define _POSIX_C_SOURCE 200809L
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE /* e as do macOS, como TIOCGWINSZ */
#endif
#endif

#include "term.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ fila de
   teclas */
#define FILA_TECLAS 64
static int fila[FILA_TECLAS];
static int fila_ini = 0, fila_fim = 0;

static void fila_poe(int tecla) {
    int prox = (fila_fim + 1) % FILA_TECLAS;
    if (prox != fila_ini) { /* fila cheia: a tecla se perde */
        fila[fila_fim] = tecla;
        fila_fim = prox;
    }
}

static int fila_tira(void) {
    if (fila_ini == fila_fim) return 0;
    int t = fila[fila_ini];
    fila_ini = (fila_ini + 1) % FILA_TECLAS;
    return t;
}

/* ------------------------------------------------------------------ buffer
   de saída */
static char saida[1 << 16];
static size_t saida_n = 0;
/* específica de cada sistema */
static void escreve_agora(const char *s, size_t n);

void term_escreve(const char *s, size_t n) {
    if (saida_n + n > sizeof saida) term_descarrega();
    if (n > sizeof saida) {
        escreve_agora(s, n);
        return;
    }
    memcpy(saida + saida_n, s, n);
    saida_n += n;
}

void term_descarrega(void) {
    if (saida_n > 0) escreve_agora(saida, saida_n);
    saida_n = 0;
}

static int iniciado = 0;

static void prepara_tela(void) {
    /* tela alternativa, esconde o cursor, cores normais, limpa */
    const char *s = "\x1b[?1049h\x1b[?25l\x1b[0m\x1b[2J\x1b[H";
    term_escreve(s, strlen(s));
    term_descarrega();
}

static void restaura_tela(void) {
    const char *s = "\x1b[0m\x1b[2J\x1b[H\x1b[?25h\x1b[?1049l";
    term_escreve(s, strlen(s));
    term_descarrega();
}

#ifdef _WIN32
/* ================================================================== WINDOWS */
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif

static HANDLE entrada, saida_h;
static DWORD modo_entrada_antigo, modo_saida_antigo;
static UINT pagina_antiga, pagina_entrada_antiga;
static int saida_e_console = 0;

static void escreve_agora(const char *s, size_t n) {
    if (!saida_e_console) {
        fwrite(s, 1, n, stdout);
        fflush(stdout);
        return;
    }
    /* o console do Windows prefere texto em UTF-16 */
    int largo = MultiByteToWideChar(CP_UTF8, 0, s, (int)n, NULL, 0);
    if (largo <= 0) return;
    wchar_t *w = malloc(sizeof(wchar_t) * (size_t)largo);
    if (w == NULL) return;
    MultiByteToWideChar(CP_UTF8, 0, s, (int)n, w, largo);
    DWORD feito = 0, pos = 0;
    while (pos < (DWORD)largo) {
        if (!WriteConsoleW(saida_h, w + pos, (DWORD)largo - pos, &feito, NULL))
            break;
        pos += feito;
    }
    free(w);
}

static BOOL WINAPI trata_ctrl(DWORD tipo) {
    if (tipo == CTRL_C_EVENT || tipo == CTRL_BREAK_EVENT) {
        fila_poe(TECLA_SAIR);
        return TRUE;
    }
    if (tipo == CTRL_CLOSE_EVENT) {
        term_encerra();
        return FALSE;
    }
    return FALSE;
}

int term_inicia(void) {
    if (iniciado) return 1;
    saida_h = GetStdHandle(STD_OUTPUT_HANDLE);
    entrada = GetStdHandle(STD_INPUT_HANDLE);
    DWORD m = 0;
    saida_e_console = GetConsoleMode(saida_h, &m) ? 1 : 0;
    modo_saida_antigo = m;
    if (saida_e_console &&
        !SetConsoleMode(saida_h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        fprintf(stderr, "Este console não entende cores ANSI (use o Windows 10 "
                        "ou mais novo).\n");
        return 0;
    }
    GetConsoleMode(entrada, &modo_entrada_antigo);
    SetConsoleMode(entrada, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS);
    pagina_antiga = GetConsoleOutputCP();
    pagina_entrada_antiga = GetConsoleCP();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleCtrlHandler(trata_ctrl, TRUE);
    iniciado = 1;
    atexit(term_encerra);
    prepara_tela();
    return 1;
}

void term_encerra(void) {
    if (!iniciado) return;
    iniciado = 0;
    restaura_tela();
    SetConsoleMode(entrada, modo_entrada_antigo);
    if (saida_e_console) SetConsoleMode(saida_h, modo_saida_antigo);
    SetConsoleOutputCP(pagina_antiga);
    SetConsoleCP(pagina_entrada_antiga);
}

static int tecla_virtual(WORD vk) {
    switch (vk) {
    case VK_UP: return TECLA_CIMA;
    case VK_DOWN: return TECLA_BAIXO;
    case VK_LEFT: return TECLA_ESQUERDA;
    case VK_RIGHT: return TECLA_DIREITA;
    case VK_PRIOR: return TECLA_PGUP;
    case VK_NEXT: return TECLA_PGDN;
    case VK_HOME: return TECLA_HOME;
    case VK_END: return TECLA_END;
    case VK_INSERT: return TECLA_INSERT;
    case VK_DELETE: return TECLA_DELETE;
    case VK_RETURN: return TECLA_ENTER;
    case VK_ESCAPE: return TECLA_ESC;
    case VK_BACK: return TECLA_APAGA;
    case VK_TAB: return TECLA_TAB;
    default:
        if (vk >= VK_F1 && vk <= VK_F10) return TECLA_F1 + (vk - VK_F1);
        return 0;
    }
}

/* Lê os eventos pendentes do console e põe as teclas na fila. */
static void le_eventos(void) {
    DWORD n = 0;
    while (GetNumberOfConsoleInputEvents(entrada, &n) && n > 0) {
        INPUT_RECORD ev;
        DWORD lidos = 0;
        if (!ReadConsoleInputW(entrada, &ev, 1, &lidos) || lidos == 0) break;
        if (ev.EventType != KEY_EVENT || !ev.Event.KeyEvent.bKeyDown) continue;
        int t = tecla_virtual(ev.Event.KeyEvent.wVirtualKeyCode);
        wchar_t c = ev.Event.KeyEvent.uChar.UnicodeChar;
        if (t == 0 && c >= 32) t = c; /* um caractere comum, já em Unicode */
        if (t == 0 && c == 3) t = TECLA_SAIR;
        for (int r = 0; t != 0 && r < ev.Event.KeyEvent.wRepeatCount; r++)
            fila_poe(t);
    }
}

int term_tecla(void) {
    le_eventos();
    return fila_tira();
}

int term_espera_tecla(int ms) {
    int t = term_tecla();
    if (t) return t;
    if (WaitForSingleObject(entrada, (DWORD)(ms < 0 ? 0 : ms)) ==
        WAIT_OBJECT_0) {
        return term_tecla();
    }
    return 0;
}

void term_tamanho(int *colunas, int *linhas) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(saida_h, &info)) {
        *colunas = info.srWindow.Right - info.srWindow.Left + 1;
        *linhas = info.srWindow.Bottom - info.srWindow.Top + 1;
    } else {
        *colunas = 80;
        *linhas = 25;
    }
}

double term_agora(void) {
    static LARGE_INTEGER freq;
    LARGE_INTEGER agora;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&agora);
    return (double)agora.QuadPart / (double)freq.QuadPart;
}

void term_dorme(int ms) { Sleep((DWORD)(ms < 0 ? 0 : ms)); }

#else
/* ================================================================== POSIX */
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios modo_antigo;

static void escreve_agora(const char *s, size_t n) {
    while (n > 0) {
        ssize_t w = write(STDOUT_FILENO, s, n);
        if (w <= 0) return;
        s += w;
        n -= (size_t)w;
    }
}

static void trata_sinal(int sinal) {
    term_encerra();
    signal(sinal, SIG_DFL);
    raise(sinal);
}

int term_inicia(void) {
    if (iniciado) return 1;
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &modo_antigo) != 0) {
        fprintf(stderr, "Este programa precisa rodar num terminal.\n");
        return 0;
    }
    struct termios cru = modo_antigo;
    cru.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ISIG | IEXTEN);
    cru.c_iflag &= ~(tcflag_t)(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    cru.c_cc[VMIN] = 0;
    cru.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cru);
    iniciado = 1;
    atexit(term_encerra);
    signal(SIGTERM, trata_sinal);
    signal(SIGHUP, trata_sinal);
    prepara_tela();
    return 1;
}

void term_encerra(void) {
    if (!iniciado) return;
    iniciado = 0;
    restaura_tela();
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &modo_antigo);
}

/* Lê um byte, esperando no máximo 'ms' milissegundos. Devolve -1 se não
   chegou nada. */
static int le_byte(int ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = {ms / 1000, (ms % 1000) * 1000};
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0) return -1;
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    return c;
}

/* Traduz uma sequência de escape (ESC [ ...) numa tecla especial. */
static int le_escape(void) {
    int c = le_byte(30);
    if (c < 0) return TECLA_ESC; /* ESC sozinho */
    if (c != '[' && c != 'O') return 0;
    char seq[8];
    int n = 0;
    for (;;) {
        int d = le_byte(30);
        if (d < 0 || n >= 7) return 0;
        seq[n++] = (char)d;
        if ((d >= 'A' && d <= 'Z') || d == '~') break;
    }
    seq[n] = '\0';
    switch (seq[n - 1]) {
    case 'A': return TECLA_CIMA;
    case 'B': return TECLA_BAIXO;
    case 'C': return TECLA_DIREITA;
    case 'D': return TECLA_ESQUERDA;
    case 'H': return TECLA_HOME;
    case 'F': return TECLA_END;
    case 'P': return TECLA_F1;
    case 'Q': return TECLA_F2;
    case 'R': return TECLA_F3;
    case 'S': return TECLA_F4;
    case '~': {
        int num = atoi(seq);
        switch (num) {
        case 1:
        case 7: return TECLA_HOME;
        case 2: return TECLA_INSERT;
        case 3: return TECLA_DELETE;
        case 4:
        case 8: return TECLA_END;
        case 5: return TECLA_PGUP;
        case 6: return TECLA_PGDN;
        case 15: return TECLA_F5;
        case 17:
        case 18:
        case 19:
        case 20:
        case 21: return TECLA_F6 + (num - 17);
        default: return 0;
        }
    }
    default: return 0;
    }
}

/* Junta os bytes de um caractere UTF-8 num código Unicode. */
static int le_utf8(int primeiro) {
    int extras = primeiro >= 0xF0   ? 3
                 : primeiro >= 0xE0 ? 2
                 : primeiro >= 0xC0 ? 1
                                    : 0;
    int cp = primeiro & (0x3F >> extras);
    for (int i = 0; i < extras; i++) {
        int c = le_byte(30);
        if (c < 0) return 0;
        cp = (cp << 6) | (c & 0x3F);
    }
    return cp;
}

static int traduz(int c) {
    if (c == 27) return le_escape();
    if (c == 3) return TECLA_SAIR; /* Ctrl+C */
    if (c == 127 || c == 8) return TECLA_APAGA;
    if (c == '\r' || c == '\n') return TECLA_ENTER;
    if (c >= 0x80) return le_utf8(c);
    return c;
}

/* Lê tudo o que chegou (esperando até 'ms' pelo primeiro byte) e põe na
   fila. */
static void le_pendentes(int ms) {
    int c = le_byte(ms);
    while (c >= 0) {
        int t = traduz(c);
        if (t) fila_poe(t);
        c = le_byte(0);
    }
}

int term_tecla(void) {
    le_pendentes(0);
    return fila_tira();
}

int term_espera_tecla(int ms) {
    if (fila_ini == fila_fim) le_pendentes(ms < 0 ? 0 : ms);
    return fila_tira();
}

void term_tamanho(int *colunas, int *linhas) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        *colunas = ws.ws_col;
        *linhas = ws.ws_row;
    } else {
        *colunas = 80;
        *linhas = 25;
    }
}

double term_agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void term_dorme(int ms) {
    if (ms <= 0) return;
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}
#endif
