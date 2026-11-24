/* term.c - terminal I/O and canvas */
#include "term.h"
#include "util.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef DISABLE_NEWLINE_AUTO_RETURN
#define DISABLE_NEWLINE_AUTO_RETURN 0x0008
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif
#else
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

TermOpts g_term;
volatile int g_term_close_req = 0;
volatile int g_term_close_ack = 0;

static Cell cur[CANVAS_W * CANVAS_H];
static Cell prev[CANVAS_W * CANVAS_H];
static int full_redraw = 1;
static int last_tw = -1, last_th = -1;
static int clip_x0 = 0, clip_y0 = 0, clip_x1 = CANVAS_W, clip_y1 = CANVAS_H;

/* key queue */
static int kq[512];
static int kq_head = 0, kq_tail = 0;
static void kq_push(int k) {
    int n = (kq_tail + 1) % 512;
    if (n == kq_head) return;
    kq[kq_tail] = k;
    kq_tail = n;
}
int term_key(void) {
    if (kq_head == kq_tail) return 0;
    int k = kq[kq_head];
    kq_head = (kq_head + 1) % 512;
    return k;
}

/* output buffer */
static char *ob = NULL;
static size_t ob_len = 0, ob_cap = 0;
static void ob_put(const char *s, size_t n) {
    if (ob_len + n + 1 > ob_cap) {
        size_t nc = ob_cap ? ob_cap * 2 : 65536;
        while (nc < ob_len + n + 1) nc *= 2;
        char *nb = (char *)realloc(ob, nc);
        if (!nb) return;
        ob = nb; ob_cap = nc;
    }
    memcpy(ob + ob_len, s, n);
    ob_len += n;
}
static void ob_str(const char *s) { ob_put(s, strlen(s)); }

#ifdef _WIN32
static HANDLE hIn, hOut;
static DWORD oldInMode, oldOutMode;
static UINT oldCP, oldInCP;
static int out_is_console = 0;

static BOOL WINAPI ctrl_handler(DWORD type) {
    if (type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT) {
        g_term_close_req = 1;
        for (int i = 0; i < 80 && !g_term_close_ack; i++) Sleep(50);
        return TRUE;
    }
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) { kq_push(K_CTRL_C); return TRUE; }
    return FALSE;
}

static void ob_flush(void) {
    if (!ob_len) return;
    if (out_is_console) {
        int wn = MultiByteToWideChar(CP_UTF8, 0, ob, (int)ob_len, NULL, 0);
        if (wn > 0) {
            wchar_t *w = (wchar_t *)malloc(sizeof(wchar_t) * (size_t)wn);
            if (w) {
                MultiByteToWideChar(CP_UTF8, 0, ob, (int)ob_len, w, wn);
                DWORD done = 0, off = 0;
                while (off < (DWORD)wn) {
                    if (!WriteConsoleW(hOut, w + off, (DWORD)wn - off, &done, NULL)) break;
                    off += done;
                }
                free(w);
            }
        }
    } else {
        fwrite(ob, 1, ob_len, stdout);
        fflush(stdout);
    }
    ob_len = 0;
}

void term_size(int *w, int *h) {
    CONSOLE_SCREEN_BUFFER_INFO ci;
    if (!g_term.headless && GetConsoleScreenBufferInfo(hOut, &ci)) {
        *w = ci.srWindow.Right - ci.srWindow.Left + 1;
        *h = ci.srWindow.Bottom - ci.srWindow.Top + 1;
    } else { *w = CANVAS_W; *h = CANVAS_H; }
}

int term_init(const TermOpts *o) {
    g_term = *o;
    if (g_term.headless) return 1;
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD m;
    out_is_console = GetConsoleMode(hOut, &m) ? 1 : 0;
    oldOutMode = m;
    if (out_is_console) {
        if (!SetConsoleMode(hOut, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN)) {
            if (!SetConsoleMode(hOut, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                fprintf(stderr, "Este console nao suporta sequencias ANSI (requer Windows 10 ou superior / Windows Terminal).\n");
                return 0;
            }
        }
    }
    GetConsoleMode(hIn, &oldInMode);
    SetConsoleMode(hIn, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS);
    oldCP = GetConsoleOutputCP();
    oldInCP = GetConsoleCP();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleCtrlHandler(ctrl_handler, TRUE);
    /* try to grow the classic console to the canvas size */
    CONSOLE_SCREEN_BUFFER_INFO ci;
    if (GetConsoleScreenBufferInfo(hOut, &ci)) {
        int ww = ci.srWindow.Right - ci.srWindow.Left + 1, wh = ci.srWindow.Bottom - ci.srWindow.Top + 1;
        if (ww < CANVAS_W || wh < CANVAS_H) {
            COORD bs = ci.dwSize;
            if (bs.X < CANVAS_W) bs.X = CANVAS_W;
            if (bs.Y < CANVAS_H) bs.Y = CANVAS_H;
            SetConsoleScreenBufferSize(hOut, bs);
            SMALL_RECT r = {0, 0, CANVAS_W - 1, CANVAS_H - 1};
            SetConsoleWindowInfo(hOut, TRUE, &r);
        }
    }
    ob_str("\x1b[?1049h\x1b[?25l\x1b[0m\x1b[2J");
    ob_flush();
    full_redraw = 1;
    return 1;
}

void term_shutdown(void) {
    if (g_term.headless) return;
    ob_str("\x1b[0m\x1b[2J\x1b[H\x1b[?25h\x1b[?1049l");
    ob_flush();
    SetConsoleMode(hIn, oldInMode);
    if (out_is_console) SetConsoleMode(hOut, oldOutMode);
    SetConsoleOutputCP(oldCP);
    SetConsoleCP(oldInCP);
}

static int map_vk(WORD vk) {
    switch (vk) {
    case VK_UP: return K_UP; case VK_DOWN: return K_DOWN;
    case VK_LEFT: return K_LEFT; case VK_RIGHT: return K_RIGHT;
    case VK_PRIOR: return K_PGUP; case VK_NEXT: return K_PGDN;
    case VK_HOME: return K_HOME; case VK_END: return K_END;
    case VK_DELETE: return K_DEL; case VK_INSERT: return K_INS;
    case VK_RETURN: return K_ENTER; case VK_ESCAPE: return K_ESC;
    case VK_BACK: return K_BS; case VK_TAB: return K_TAB;
    case VK_F1: return K_F1; case VK_F2: return K_F2; case VK_F3: return K_F3;
    case VK_F4: return K_F4; case VK_F5: return K_F5; case VK_F6: return K_F6;
    case VK_F7: return K_F7; case VK_F8: return K_F8; case VK_F9: return K_F9;
    case VK_F10: return K_F10;
    default: return 0;
    }
}

int term_poll(int ms) {
    if (g_term.headless) return 0;
    if (g_term_close_req) { kq_push(K_CLOSE); return 1; }
    if (WaitForSingleObject(hIn, (DWORD)(ms < 0 ? 0 : ms)) != WAIT_OBJECT_0) return kq_head != kq_tail;
    DWORD n = 0;
    while (GetNumberOfConsoleInputEvents(hIn, &n) && n > 0) {
        INPUT_RECORD recs[64];
        DWORD got = 0;
        if (!ReadConsoleInputW(hIn, recs, 64, &got)) break;
        for (DWORD i = 0; i < got; i++) {
            if (recs[i].EventType == WINDOW_BUFFER_SIZE_EVENT) { kq_push(K_RESIZE); continue; }
            if (recs[i].EventType != KEY_EVENT) continue;
            KEY_EVENT_RECORD *k = &recs[i].Event.KeyEvent;
            if (!k->bKeyDown) continue;
            int rep = k->wRepeatCount ? k->wRepeatCount : 1;
            int key = map_vk(k->wVirtualKeyCode);
            if (!key) {
                wchar_t c = k->uChar.UnicodeChar;
                if (c == 0) continue;
                if (c >= 0xD800 && c <= 0xDFFF) continue;
                key = (int)c;
            }
            for (int r = 0; r < rep && r < 8; r++) kq_push(key);
        }
    }
    return kq_head != kq_tail;
}

#else /* POSIX */
static struct termios orig_tio;
static int tio_saved = 0;
static volatile sig_atomic_t winch = 0;

static void on_winch(int s) { (void)s; winch = 1; }
static void on_term(int s) { (void)s; g_term_close_req = 1; }

static void ob_flush(void) {
    size_t off = 0;
    while (off < ob_len) {
        ssize_t w = write(1, ob + off, ob_len - off);
        if (w <= 0) break;
        off += (size_t)w;
    }
    ob_len = 0;
}

void term_size(int *w, int *h) {
    struct winsize ws;
    if (!g_term.headless && ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        *w = ws.ws_col; *h = ws.ws_row;
    } else { *w = CANVAS_W; *h = CANVAS_H; }
}

int term_init(const TermOpts *o) {
    g_term = *o;
    if (g_term.headless) return 1;
    if (!isatty(0)) { fprintf(stderr, "Entrada nao e um terminal.\n"); return 0; }
    if (tcgetattr(0, &orig_tio) == 0) {
        tio_saved = 1;
        struct termios raw = orig_tio;
        raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON | ISIG | IEXTEN);
        raw.c_iflag &= ~(tcflag_t)(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(0, TCSAFLUSH, &raw);
    }
    signal(SIGWINCH, on_winch);
    signal(SIGTERM, on_term);
    signal(SIGHUP, on_term);
    ob_str("\x1b[?1049h\x1b[?25l\x1b[0m\x1b[2J");
    ob_flush();
    full_redraw = 1;
    return 1;
}

void term_shutdown(void) {
    if (g_term.headless) return;
    ob_str("\x1b[0m\x1b[2J\x1b[H\x1b[?25h\x1b[?1049l");
    ob_flush();
    if (tio_saved) tcsetattr(0, TCSAFLUSH, &orig_tio);
}

static unsigned char ib[1024];
static int ib_len = 0;

static void parse_input(int final) {
    int i = 0;
    while (i < ib_len) {
        unsigned char c = ib[i];
        if (c == 27) {
            if (i + 1 >= ib_len) {
                if (!final) break;
                kq_push(K_ESC); i++; continue;
            }
            unsigned char c1 = ib[i + 1];
            if (c1 == '[' || c1 == 'O') {
                int j = i + 2;
                while (j < ib_len && !((ib[j] >= 0x40 && ib[j] <= 0x7E))) j++;
                if (j >= ib_len) { if (!final) break; kq_push(K_ESC); i++; continue; }
                unsigned char fin = ib[j];
                char params[32] = {0};
                int pl = j - (i + 2);
                if (pl > 30) pl = 30;
                memcpy(params, ib + i + 2, (size_t)pl);
                int key = 0;
                switch (fin) {
                case 'A': key = K_UP; break; case 'B': key = K_DOWN; break;
                case 'C': key = K_RIGHT; break; case 'D': key = K_LEFT; break;
                case 'H': key = K_HOME; break; case 'F': key = K_END; break;
                case 'P': key = K_F1; break; case 'Q': key = K_F2; break;
                case 'R': key = K_F3; break; case 'S': key = K_F4; break;
                case 'Z': key = K_TAB; break;
                case '~': {
                    int n = atoi(params);
                    switch (n) {
                    case 1: case 7: key = K_HOME; break; case 4: case 8: key = K_END; break;
                    case 2: key = K_INS; break; case 3: key = K_DEL; break;
                    case 5: key = K_PGUP; break; case 6: key = K_PGDN; break;
                    case 11: key = K_F1; break; case 12: key = K_F2; break;
                    case 13: key = K_F3; break; case 14: key = K_F4; break;
                    case 15: key = K_F5; break; case 17: key = K_F6; break;
                    case 18: key = K_F7; break; case 19: key = K_F8; break;
                    case 20: key = K_F9; break; case 21: key = K_F10; break;
                    }
                } break;
                }
                if (key) kq_push(key);
                i = j + 1;
                continue;
            }
            /* alt+key: ignore alt */
            kq_push(K_ESC);
            i++;
            continue;
        }
        if (c == 13 || c == 10) { kq_push(K_ENTER); i++; continue; }
        if (c == 127 || c == 8) { kq_push(K_BS); i++; continue; }
        if (c < 0x80) { kq_push(c); i++; continue; }
        /* utf-8 */
        int need = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 1;
        if (i + need > ib_len) { if (!final) break; i++; continue; }
        char tmp[8] = {0};
        memcpy(tmp, ib + i, (size_t)need);
        const char *p = tmp;
        uint32_t cp = utf8_next(&p);
        kq_push((int)cp);
        i += need;
    }
    if (i > 0) { memmove(ib, ib + i, (size_t)(ib_len - i)); ib_len -= i; }
}

int term_poll(int ms) {
    if (g_term.headless) return 0;
    if (winch) { winch = 0; kq_push(K_RESIZE); }
    if (g_term_close_req) { kq_push(K_CLOSE); return 1; }
    fd_set fs;
    FD_ZERO(&fs);
    FD_SET(0, &fs);
    struct timeval tv = {ms / 1000, (ms % 1000) * 1000};
    int r = select(1, &fs, NULL, NULL, &tv);
    if (r > 0) {
        ssize_t n = read(0, ib + ib_len, sizeof(ib) - (size_t)ib_len);
        if (n > 0) ib_len += (int)n;
        parse_input(0);
        if (ib_len > 0) {
            /* incomplete sequence: wait a little for the rest */
            FD_ZERO(&fs); FD_SET(0, &fs);
            struct timeval tv2 = {0, 25000};
            if (select(1, &fs, NULL, NULL, &tv2) > 0) {
                n = read(0, ib + ib_len, sizeof(ib) - (size_t)ib_len);
                if (n > 0) ib_len += (int)n;
            }
            parse_input(1);
        }
    }
    return kq_head != kq_tail;
}
#endif

void term_force_redraw(void) { full_redraw = 1; }

/* ---------------- canvas ---------------- */
Cell *cv_cell(int x, int y) {
    if (x < 0 || y < 0 || x >= CANVAS_W || y >= CANVAS_H) return NULL;
    return &cur[y * CANVAS_W + x];
}

void cv_set_clip(int x, int y, int w, int h) {
    clip_x0 = x < 0 ? 0 : x; clip_y0 = y < 0 ? 0 : y;
    clip_x1 = x + w > CANVAS_W ? CANVAS_W : x + w;
    clip_y1 = y + h > CANVAS_H ? CANVAS_H : y + h;
}
void cv_reset_clip(void) { clip_x0 = 0; clip_y0 = 0; clip_x1 = CANVAS_W; clip_y1 = CANVAS_H; }

void cv_clear(uint32_t bg) {
    for (int i = 0; i < CANVAS_W * CANVAS_H; i++) { cur[i].ch = ' '; cur[i].fg = RGB(200, 200, 200); cur[i].bg = bg; cur[i].attr = 0; }
}

void cv_put_attr(int x, int y, uint32_t ch, uint32_t fg, uint32_t bg, uint8_t attr) {
    if (x < clip_x0 || y < clip_y0 || x >= clip_x1 || y >= clip_y1) return;
    Cell *c = &cur[y * CANVAS_W + x];
    c->ch = ch;
    if (fg != COL_KEEP) c->fg = fg;
    if (bg != COL_KEEP) c->bg = bg;
    c->attr = attr;
}

void cv_put(int x, int y, uint32_t ch, uint32_t fg, uint32_t bg) { cv_put_attr(x, y, ch, fg, bg, 0); }

int cv_text(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    int n = 0;
    uint32_t cp;
    while ((cp = utf8_next(&s)) != 0) { cv_put(x + n, y, cp, fg, bg); n++; }
    return n;
}

int cv_text_w(int x, int y, int maxw, const char *s, uint32_t fg, uint32_t bg) {
    int n = 0;
    uint32_t cp;
    int len = utf8_len(s);
    while ((cp = utf8_next(&s)) != 0 && n < maxw) {
        if (len > maxw && n == maxw - 1) cp = 0x2026; /* … */
        cv_put(x + n, y, cp, fg, bg);
        n++;
    }
    return n;
}

int cv_textf(int x, int y, uint32_t fg, uint32_t bg, const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return cv_text(x, y, buf, fg, bg);
}

int cv_text_r(int xr, int y, const char *s, uint32_t fg, uint32_t bg) {
    int n = utf8_len(s);
    return cv_text(xr - n + 1, y, s, fg, bg);
}

int cv_text_c(int xc, int y, const char *s, uint32_t fg, uint32_t bg) {
    int n = utf8_len(s);
    return cv_text(xc - n / 2, y, s, fg, bg);
}

void cv_fill(int x, int y, int w, int h, uint32_t ch, uint32_t fg, uint32_t bg) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) cv_put(x + i, y + j, ch, fg, bg);
}

void cv_tint_rect(int x, int y, int w, int h, uint32_t bg) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            Cell *c = cv_cell(x + i, y + j);
            if (c && x + i >= clip_x0 && x + i < clip_x1 && y + j >= clip_y0 && y + j < clip_y1) c->bg = bg;
        }
}

void cv_bold_rect(int x, int y, int w, int h) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            Cell *c = cv_cell(x + i, y + j);
            if (c) c->attr |= AT_BOLD;
        }
}

void cv_hline(int x, int y, int w, uint32_t ch, uint32_t fg, uint32_t bg) {
    for (int i = 0; i < w; i++) cv_put(x + i, y, ch, fg, bg);
}
void cv_vline(int x, int y, int h, uint32_t ch, uint32_t fg, uint32_t bg) {
    for (int i = 0; i < h; i++) cv_put(x, y + i, ch, fg, bg);
}

void cv_box(int x, int y, int w, int h, uint32_t fg, uint32_t bg, int style) {
    static const uint32_t S[4][6] = {
        {0x250C, 0x2510, 0x2514, 0x2518, 0x2500, 0x2502},
        {0x2554, 0x2557, 0x255A, 0x255D, 0x2550, 0x2551},
        {0x256D, 0x256E, 0x2570, 0x256F, 0x2500, 0x2502},
        {0x250F, 0x2513, 0x2517, 0x251B, 0x2501, 0x2503},
    };
    if (w < 2 || h < 2) return;
    const uint32_t *s = S[style & 3];
    cv_put(x, y, s[0], fg, bg);
    cv_put(x + w - 1, y, s[1], fg, bg);
    cv_put(x, y + h - 1, s[2], fg, bg);
    cv_put(x + w - 1, y + h - 1, s[3], fg, bg);
    cv_hline(x + 1, y, w - 2, s[4], fg, bg);
    cv_hline(x + 1, y + h - 1, w - 2, s[4], fg, bg);
    cv_vline(x, y + 1, h - 2, s[5], fg, bg);
    cv_vline(x + w - 1, y + 1, h - 2, s[5], fg, bg);
}

static uint32_t markup_color(char c, uint32_t def) {
    switch (c) {
    case 'r': return RGB(235, 85, 70);
    case 'g': return RGB(110, 215, 110);
    case 'y': return RGB(245, 210, 90);
    case 'w': return RGB(240, 240, 245);
    case 'c': return RGB(100, 215, 235);
    case 'o': return RGB(245, 150, 60);
    case 'a': return RGB(140, 150, 165);
    case 'm': return RGB(200, 130, 235);
    case 'b': return RGB(110, 160, 245);
    default: return def;
    }
}

/* word-wrapping text with \001X color markup; returns total line count */
int cv_wrap(int x, int y, int w, int maxh, const char *text, uint32_t fg, uint32_t bg) {
    int line = 0, col = 0;
    uint32_t color = fg;
    const char *p = text;
    while (*p) {
        if (*p == '\n') { line++; col = 0; p++; continue; }
        if (*p == '\001') { if (p[1]) { color = markup_color(p[1], fg); p += 2; } else p++; continue; }
        if (*p == ' ') {
            if (col > 0 && col < w) { if (line < maxh) cv_put(x + col, y + line, ' ', color, bg); col++; }
            p++;
            continue;
        }
        /* measure word */
        const char *q = p;
        int wl = 0;
        while (*q && *q != ' ' && *q != '\n') {
            if (*q == '\001') { q += q[1] ? 2 : 1; continue; }
            utf8_next(&q);
            wl++;
        }
        if (col + wl > w && col > 0) { line++; col = 0; }
        while (p < q) {
            if (*p == '\001') { if (p[1]) { color = markup_color(p[1], fg); p += 2; } else p++; continue; }
            uint32_t cp = utf8_next(&p);
            if (col >= w) { line++; col = 0; }
            if (line < maxh) cv_put(x + col, y + line, cp, color, bg);
            col++;
        }
    }
    return line + 1;
}

uint32_t col_lerp(uint32_t a, uint32_t b, double t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int r = (int)((int)COL_R(a) + ((int)COL_R(b) - (int)COL_R(a)) * t);
    int g = (int)((int)COL_G(a) + ((int)COL_G(b) - (int)COL_G(a)) * t);
    int bb = (int)((int)COL_B(a) + ((int)COL_B(b) - (int)COL_B(a)) * t);
    return RGB(r, g, bb);
}

uint32_t col_scale(uint32_t a, double k) {
    int r = (int)(COL_R(a) * k), g = (int)(COL_G(a) * k), b = (int)(COL_B(a) * k);
    return RGB(clampi(r, 0, 255), clampi(g, 0, 255), clampi(b, 0, 255));
}

uint32_t ascii_fallback(uint32_t cp) {
    if (cp < 0x80) return cp;
    switch (cp) {
    case 0x2500: case 0x2501: case 0x2550: case 0x2504: case 0x2508: return '-';
    case 0x2502: case 0x2503: case 0x2551: case 0x2506: case 0x250A: return '|';
    case 0x250C: case 0x2510: case 0x2514: case 0x2518: case 0x251C: case 0x2524:
    case 0x252C: case 0x2534: case 0x253C: case 0x2554: case 0x2557: case 0x255A:
    case 0x255D: case 0x2560: case 0x2563: case 0x2566: case 0x2569: case 0x256C:
    case 0x256D: case 0x256E: case 0x256F: case 0x2570: case 0x250F: case 0x2513:
    case 0x2517: case 0x251B: return '+';
    case 0x2588: case 0x2593: return '#';
    case 0x2592: return '%';
    case 0x2591: return ':';
    case 0x2580: return '"';
    case 0x2584: return '_';
    case 0x258C: case 0x2590: return '|';
    case 0x2192: case 0x25BA: case 0x25B6: return '>';
    case 0x2190: case 0x25C4: case 0x25C0: return '<';
    case 0x2191: case 0x25B2: return '^';
    case 0x2193: case 0x25BC: return 'v';
    case 0x2022: case 0x25CF: return 'o';
    case 0x00B7: case 0x2219: return '.';
    case 0x00B0: return 'o';
    case 0x00D7: return 'x';
    case 0x2248: return '~';
    case 0x2713: return 'v';
    case 0x2717: return 'x';
    case 0x2026: return '~';
    case 0x2605: case 0x2726: return '*';
    case 0x25CB: case 0x25EF: return 'O';
    case 0x2666: case 0x25C6: return '*';
    case 0x00AB: return '<';
    case 0x00BB: return '>';
    case 0x2014: case 0x2013: return '-';
    case 0x2261: return '=';
    case 0x00B2: return '2';
    case 0x00AA: return 'a';
    case 0x25C9: return 'o';
    case 0x2694: return 'X';
    case 0x26A0: return '!';
    case 0x27A4: return '>';
    case 0x00B3: return '3';
    }
    /* Latin-1 accents */
    if (cp >= 0xC0 && cp <= 0xC5) return 'A';
    if (cp == 0xC7) return 'C';
    if (cp >= 0xC8 && cp <= 0xCB) return 'E';
    if (cp >= 0xCC && cp <= 0xCF) return 'I';
    if (cp == 0xD1) return 'N';
    if (cp >= 0xD2 && cp <= 0xD6) return 'O';
    if (cp >= 0xD9 && cp <= 0xDC) return 'U';
    if (cp >= 0xE0 && cp <= 0xE5) return 'a';
    if (cp == 0xE7) return 'c';
    if (cp >= 0xE8 && cp <= 0xEB) return 'e';
    if (cp >= 0xEC && cp <= 0xEF) return 'i';
    if (cp == 0xF1) return 'n';
    if (cp >= 0xF2 && cp <= 0xF6) return 'o';
    if (cp >= 0xF9 && cp <= 0xFC) return 'u';
    return '?';
}

static int rgb256(uint32_t c) {
    int r = COL_R(c), g = COL_G(c), b = COL_B(c);
    int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    int mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    if (mx - mn < 12) {
        int avg = (r + g + b) / 3;
        if (avg < 5) return 16;
        if (avg > 246) return 231;
        int k = (avg - 8) / 10;
        if (k < 0) k = 0;
        if (k > 23) k = 23;
        return 232 + k;
    }
#define Q6(v) ((v) < 48 ? 0 : (v) < 115 ? 1 : ((v) - 35) / 40)
    return 16 + 36 * Q6(r) + 6 * Q6(g) + Q6(b);
#undef Q6
}

static void emit_color(int fgbg, uint32_t c) {
    char b[40];
    if (g_term.colormode == CM_TRUE)
        snprintf(b, sizeof b, "\x1b[%d;2;%u;%u;%um", fgbg ? 48 : 38, COL_R(c), COL_G(c), COL_B(c));
    else
        snprintf(b, sizeof b, "\x1b[%d;5;%dm", fgbg ? 48 : 38, rgb256(c));
    ob_str(b);
}

void cv_present(void) {
    if (g_term.headless) return;
    int tw, th;
    term_size(&tw, &th);
    if (tw != last_tw || th != last_th) { full_redraw = 1; last_tw = tw; last_th = th; }
    int ox = tw > CANVAS_W ? (tw - CANVAS_W) / 2 : 0;
    int oy = th > CANVAS_H ? (th - CANVAS_H) / 2 : 0;
    if (full_redraw) ob_str("\x1b[0m\x1b[2J");
    uint32_t lf = 0xFFFFFFFFu, lb = 0xFFFFFFFFu;
    int la = -1;
    int cx = -1, cy = -1;
    int maxw = tw < CANVAS_W ? tw : CANVAS_W;
    int maxh = th < CANVAS_H ? th : CANVAS_H;
    char tmp[32];
    for (int y = 0; y < maxh; y++) {
        for (int x = 0; x < maxw; x++) {
            if (x == tw - 1 && y == th - 1) continue; /* avoid scrolling */
            Cell *c = &cur[y * CANVAS_W + x];
            Cell *p = &prev[y * CANVAS_W + x];
            if (!full_redraw && c->ch == p->ch && c->fg == p->fg && c->bg == p->bg && c->attr == p->attr) continue;
            if (cy != y || cx != x) {
                snprintf(tmp, sizeof tmp, "\x1b[%d;%dH", oy + y + 1, ox + x + 1);
                ob_str(tmp);
            }
            if (c->attr != la) {
                ob_str(c->attr & AT_BOLD ? "\x1b[0;1m" : "\x1b[0m");
                if (c->attr & AT_UNDER) ob_str("\x1b[4m");
                la = c->attr; lf = 0xFFFFFFFFu; lb = 0xFFFFFFFFu;
            }
            if (c->fg != lf) { emit_color(0, c->fg); lf = c->fg; }
            if (c->bg != lb) { emit_color(1, c->bg); lb = c->bg; }
            uint32_t ch = c->ch;
            if (ch < 32) ch = ' ';
            if (g_term.ascii) ch = ascii_fallback(ch);
            char u[5];
            int k = utf8_encode(ch, u);
            ob_put(u, (size_t)k);
            *p = *c;
            cx = x + 1; cy = y;
        }
    }
    if (tw < CANVAS_W || th < CANVAS_H) {
        snprintf(tmp, sizeof tmp, "\x1b[1;1H");
        ob_str(tmp);
        char msg[160];
        snprintf(msg, sizeof msg, "\x1b[0;1;37;41m Janela pequena: %dx%d (ideal %dx%d) - aumente o terminal ou reduza a fonte \x1b[0m", tw, th, CANVAS_W, CANVAS_H);
        ob_str(msg);
        full_redraw = 1;
    } else {
        full_redraw = 0;
    }
    ob_str("\x1b[0m");
    ob_flush();
}

void cv_dump_text(FILE *f) {
    for (int y = 0; y < CANVAS_H; y++) {
        for (int x = 0; x < CANVAS_W; x++) {
            char u[5];
            uint32_t ch = cur[y * CANVAS_W + x].ch;
            if (ch < 32) ch = ' ';
            int k = utf8_encode(ch, u);
            fwrite(u, 1, (size_t)k, f);
        }
        fputc('\n', f);
    }
}

void cv_dump_cells(FILE *f) {
    /* one line per row: ch:fg:bg tokens */
    for (int y = 0; y < CANVAS_H; y++) {
        for (int x = 0; x < CANVAS_W; x++) {
            Cell *c = &cur[y * CANVAS_W + x];
            fprintf(f, "%x,%06x,%06x,%d ", c->ch < 32 ? 32u : c->ch, c->fg & 0xFFFFFFu, c->bg & 0xFFFFFFu, c->attr);
        }
        fputc('\n', f);
    }
}
