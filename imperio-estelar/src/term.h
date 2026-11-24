/* term.h - portable terminal layer (Windows console VT / POSIX termios)
 * Draws into an off-screen canvas of Cells (Unicode + 24-bit colors) and
 * flushes only the changed cells to the terminal. */
#ifndef TERM_H
#define TERM_H

#include <stdint.h>
#include <stdio.h>

#define CANVAS_W 120
#define CANVAS_H 36

#include "util.h"
#define COL_KEEP 0xFE000000u   /* keep existing background/foreground */
#define COL_R(c) (((c) >> 16) & 255)
#define COL_G(c) (((c) >> 8) & 255)
#define COL_B(c) ((c) & 255)

enum { AT_BOLD = 1, AT_UNDER = 2 };

typedef struct { uint32_t ch, fg, bg; uint8_t attr; } Cell;

enum {
    K_UP = 0x110000, K_DOWN, K_LEFT, K_RIGHT, K_PGUP, K_PGDN, K_HOME, K_END,
    K_DEL, K_INS, K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10,
    K_RESIZE, K_CLOSE
};
#define K_ENTER 13
#define K_ESC 27
#define K_BS 8
#define K_TAB 9
#define K_CTRL_C 3

enum { CM_TRUE = 0, CM_256 = 1 };

typedef struct {
    int ascii;      /* map non-ASCII glyphs to ASCII */
    int colormode;  /* CM_TRUE / CM_256 */
    int headless;   /* no terminal I/O (tests, snapshots) */
} TermOpts;

extern TermOpts g_term;
extern volatile int g_term_close_req;   /* window close / SIGTERM requested */
extern volatile int g_term_close_ack;   /* main loop saved & acknowledged */

int  term_init(const TermOpts *o);
void term_shutdown(void);
int  term_poll(int ms);          /* waits up to ms for input; returns 1 if keys queued */
int  term_key(void);             /* pop next key, 0 if none */
void term_size(int *w, int *h);
void term_force_redraw(void);

/* canvas */
Cell *cv_cell(int x, int y);
void cv_clear(uint32_t bg);
void cv_put(int x, int y, uint32_t ch, uint32_t fg, uint32_t bg);
void cv_put_attr(int x, int y, uint32_t ch, uint32_t fg, uint32_t bg, uint8_t attr);
int  cv_text(int x, int y, const char *utf8, uint32_t fg, uint32_t bg);
int  cv_textf(int x, int y, uint32_t fg, uint32_t bg, const char *fmt, ...);
int  cv_text_w(int x, int y, int maxw, const char *utf8, uint32_t fg, uint32_t bg); /* clipped */
int  cv_text_r(int xr, int y, const char *utf8, uint32_t fg, uint32_t bg);          /* right aligned, ends at xr */
int  cv_text_c(int xc, int y, const char *utf8, uint32_t fg, uint32_t bg);          /* centered */
void cv_fill(int x, int y, int w, int h, uint32_t ch, uint32_t fg, uint32_t bg);
void cv_box(int x, int y, int w, int h, uint32_t fg, uint32_t bg, int style);
void cv_hline(int x, int y, int w, uint32_t ch, uint32_t fg, uint32_t bg);
void cv_vline(int x, int y, int h, uint32_t ch, uint32_t fg, uint32_t bg);
void cv_set_clip(int x, int y, int w, int h);
void cv_reset_clip(void);
void cv_bold_rect(int x, int y, int w, int h);
void cv_tint_rect(int x, int y, int w, int h, uint32_t bg);
void cv_present(void);
int  cv_wrap(int x, int y, int w, int maxh, const char *text, uint32_t fg, uint32_t bg); /* returns lines */

/* snapshot helpers for tests */
void cv_dump_text(FILE *f);
void cv_dump_cells(FILE *f);

uint32_t col_lerp(uint32_t a, uint32_t b, double t);
uint32_t col_scale(uint32_t a, double k);
uint32_t ascii_fallback(uint32_t cp);

#endif
