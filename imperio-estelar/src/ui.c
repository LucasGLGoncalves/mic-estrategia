/* ui.c - frame, menu, modals, overview, building/research/shipyard screens */
#include "ui.h"
#include "art.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct { int key; const char *label; int scr; } MENU[] = {
    {'1', "Visão Geral", SCR_OVERVIEW}, {'2', "Recursos", SCR_RESOURCES}, {'3', "Civilização", SCR_LIFE},
    {'4', "Instalações", SCR_FACILITIES}, {'5', "Pesquisa", SCR_RESEARCH}, {'6', "Estaleiro", SCR_SHIPYARD},
    {'7', "Defesa", SCR_DEFENSE}, {'8', "Frota", SCR_FLEET}, {'9', "Galáxia", SCR_GALAXY},
    {'0', "Império", SCR_EMPIRE}, {'C', "Comércio", SCR_TRADE}, {'E', "Mercador", SCR_MERCHANT},
    {'M', "Mensagens", SCR_MESSAGES}, {'T', "Tecnologia", SCR_TECH}, {'O', "Oficiais", SCR_OFFICERS},
    {'R', "Classificação", SCR_RANKING}, {'Z', "Simulador", SCR_SIM}, {'H', "Ajuda", SCR_HELP},
};
#define NMENU ((int)(sizeof MENU / sizeof MENU[0]))

void ui_init(UI *u, Game *g) {
    memset(u, 0, sizeof *u);
    u->screen = SCR_OVERVIEW;
    u->gg = g->body[g->cur].g;
    u->gs = g->body[g->cur].s;
    u->tech_item = B_METAL_MINE;
    u->dp.speed = 100;
    u->mer_from = RES_METAL;
    u->mer_to = RES_CRYSTAL;
}

void ui_toast(UI *u, uint32_t col, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(u->toast, sizeof u->toast, fmt, ap);
    va_end(ap);
    u->toast_col = col;
    u->toast_until = u->t + 4.0;
}

void ui_panel(int x, int y, int w, int h, const char *title) {
    cv_fill(x, y, w, h, ' ', C_TEXT, C_PANEL);
    cv_box(x, y, w, h, C_BORDER, C_PANEL, 2);
    if (title && title[0]) {
        int n = utf8_len(title);
        int tx = x + (w - n - 2) / 2;
        cv_put(tx, y, ' ', C_TITLE, C_PANEL);
        cv_text(tx + 1, y, title, C_TITLE, C_PANEL);
        cv_put(tx + 1 + n, y, ' ', C_TITLE, C_PANEL);
        cv_bold_rect(tx + 1, y, n, 1);
    }
}

void ui_bar(int x, int y, int w, double f, uint32_t col) {
    f = clampd(f, 0, 1);
    int full = (int)(f * w * 2.0 + 0.5);
    for (int i = 0; i < w; i++) {
        uint32_t ch = full >= 2 * (i + 1) ? 0x2588 : full == 2 * i + 1 ? 0x258C : 0x2591;
        cv_put(x + i, y, ch, ch == 0x2591 ? RGB(45, 55, 75) : col, COL_KEEP);
    }
}

void cost_str(char *buf, size_t n, const double c[3]) {
    char a[24], b[24], d[24];
    fmt_short(a, sizeof a, c[0]); fmt_short(b, sizeof b, c[1]); fmt_short(d, sizeof d, c[2]);
    snprintf(buf, n, "%sM %sC %sD", a, b, d);
}

void draw_cost(int x, int y, const Game *g, const Body *b, const double c[3], double energy) {
    char t[24];
    static const uint32_t RC[3] = {RGB(190, 195, 205), RGB(110, 215, 240), RGB(90, 220, 190)};
    for (int i = 0; i < 3; i++) {
        fmt_short(t, sizeof t, c[i]);
        uint32_t col = c[i] <= 0 ? RGB(70, 80, 100) : (b->res[i] + 1e-6 >= c[i] ? RC[i] : C_RED);
        cv_text_r(x + 8 + i * 10, y, c[i] <= 0 ? "-" : t, col, COL_KEEP);
    }
    if (energy > 0) {
        Rates r;
        body_rates(g, b, &r);
        cv_put(x + 30, y, 'E', r.e_prod >= energy ? C_GOLD : C_RED, COL_KEEP);
    }
}

static void cost_header(int x, int y, uint32_t bg) {
    cv_text_r(x + 8, y, "Metal", C_TITLE, bg);
    cv_text_r(x + 18, y, "Cristal", C_TITLE, bg);
    cv_text_r(x + 28, y, "Deutério", C_TITLE, bg);
}

static void fmt_cap(char *buf, size_t n, double v) {
    if (v >= 1e9) snprintf(buf, n, "%.1fB", v / 1e9);
    else if (v >= 1e6) snprintf(buf, n, "%.1fM", v / 1e6);
    else if (v >= 1e4) snprintf(buf, n, "%.0fk", v / 1e3);
    else fmt_num(buf, n, v);
    for (char *p = buf; *p; p++) if (*p == '.') *p = ',';
}

const char *body_label(const Game *g, int bi, char *buf, size_t n) {
    const Body *b = &g->body[bi];
    snprintf(buf, n, "%s [%d:%d:%d]%s", b->name, b->g, b->s, b->p, b->is_moon ? " (Lua)" : "");
    return buf;
}

void ui_prompt(UI *u, int purpose, int arg, const char *title, const char *text, const char *init, int numeric) {
    u->modal = MD_PROMPT;
    u->mpurpose = purpose;
    u->marg = arg;
    snprintf(u->mtitle, sizeof u->mtitle, "%s", title);
    snprintf(u->mtext, sizeof u->mtext, "%s", text ? text : "");
    snprintf(u->minput, sizeof u->minput, "%s", init ? init : "");
    u->mnumeric = numeric;
    u->mmaxlen = numeric ? 15 : 24;
}

void ui_menu(UI *u, int purpose, int arg, const char *title, int n, const char **items, const int *ok) {
    u->modal = MD_MENU;
    u->mpurpose = purpose;
    u->marg = arg;
    snprintf(u->mtitle, sizeof u->mtitle, "%s", title);
    u->mn = n > 12 ? 12 : n;
    u->msel = 0;
    for (int i = 0; i < u->mn; i++) {
        snprintf(u->mitems[i], sizeof u->mitems[i], "%s", items[i]);
        u->mok[i] = ok ? ok[i] : 1;
    }
    for (int i = 0; i < u->mn; i++) if (u->mok[i]) { u->msel = i; break; }
}

void ui_info(UI *u, const char *title, const char *fmt, ...) {
    u->modal = MD_INFO;
    snprintf(u->mtitle, sizeof u->mtitle, "%s", title);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(u->mtext, sizeof u->mtext, fmt, ap);
    va_end(ap);
}

void ui_confirm(UI *u, int purpose, int arg, const char *title, const char *fmt, ...) {
    u->modal = MD_CONFIRM;
    u->mpurpose = purpose;
    u->marg = arg;
    snprintf(u->mtitle, sizeof u->mtitle, "%s", title);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(u->mtext, sizeof u->mtext, fmt, ap);
    va_end(ap);
}

void ui_open_tech(UI *u, int id) {
    u->tech_item = id;
    u->sel[SCR_TECH] = id;
    u->screen = SCR_TECH;
    u->tab[SCR_TECH] = 0;
}

/* ===================== FRAME ===================== */
static void draw_header(UI *u, Game *g) {
    (void)u;
    cv_fill(0, 0, CANVAS_W, 1, ' ', C_TEXT, C_PANEL2);
    cv_text(1, 0, "✦ IMPÉRIO ESTELAR", C_GOLD, C_PANEL2);
    cv_bold_rect(1, 0, 18, 1);
    char b[160];
    snprintf(b, sizeof b, "Comandante %s", g->name);
    int x = 22 + cv_text_w(22, 0, 34, b, C_TEXT, C_PANEL2) + 3;
    snprintf(b, sizeof b, "x%d", g->speed);
    x += cv_text(x, 0, b, C_DIM, C_PANEL2) + 3;
    int tot;
    int rank = player_rank(g, &tot);
    char p[32];
    fmt_num(p, sizeof p, player_points(g, NULL, NULL, NULL, NULL));
    snprintf(b, sizeof b, "Pontos %s  (Lugar %d de %d)", p, rank, tot);
    cv_text_w(x, 0, 104 - x, b, C_TITLE, C_PANEL2);
    char clk[32];
    fmt_clock(clk, sizeof clk, g->now);
    cv_text_r(118, 0, clk, C_DIM, C_PANEL2);
}

static void draw_resbar(UI *u, Game *g) {
    (void)u;
    Body *b = &g->body[g->cur];
    Rates r;
    body_rates(g, b, &r);
    cv_fill(0, 1, CANVAS_W, 3, ' ', C_TEXT, C_BG);
    static const char *L[7] = {"Metal", "Cristal", "Deutério", "Energia", "População", "Alimento", "Matéria Negra"};
    static const uint32_t COL[7] = {RGB(190, 195, 205), RGB(110, 215, 240), RGB(90, 220, 190), RGB(245, 215, 80),
                                    RGB(225, 215, 200), RGB(130, 215, 90), RGB(200, 130, 235)};
    for (int i = 0; i < 7; i++) {
        int x = 1 + i * 17;
        cv_fill(x, 1, 16, 3, ' ', C_TEXT, C_PANEL);
        cv_put(x + 1, 1, 0x2588, COL[i], C_PANEL);
        cv_text(x + 3, 1, L[i], C_DIM, C_PANEL);
        char v[32], s[40];
        uint32_t vc = COL[i];
        if (i < 3) {
            fmt_num(v, sizeof v, b->res[i]);
            if (b->res[i] >= r.cap[i] - 0.5) vc = C_RED;
            char rate[24], cap[24];
            fmt_short(rate, sizeof rate, r.prod[i]);
            fmt_cap(cap, sizeof cap, r.cap[i]);
            snprintf(s, sizeof s, "%s%s/h", r.prod[i] >= 0 ? "+" : "", rate);
            cv_text(x + 1, 3, s, r.prod[i] >= 0 ? C_GREEN : C_RED, C_PANEL);
            cv_text_r(x + 15, 1, cap, b->res[i] >= r.cap[i] - 0.5 ? C_RED : RGB(85, 98, 120), C_PANEL);
        } else if (i == 3) {
            fmt_num(v, sizeof v, r.e_prod - r.e_cons);
            if (r.e_prod - r.e_cons < 0) vc = C_RED;
            char a[24], c[24];
            fmt_short(a, sizeof a, r.e_prod);
            fmt_short(c, sizeof c, r.e_cons);
            snprintf(s, sizeof s, "%s/%s", c, a);
            cv_text(x + 1, 3, s, C_DIM, C_PANEL);
        } else if (i == 4) {
            char a[24], c[24];
            fmt_short(a, sizeof a, b->pop);
            fmt_short(c, sizeof c, r.pop_cap);
            snprintf(v, sizeof v, "%s/%s", a, c);
            int starving = b->res[RES_FOOD] < 1 && r.prod[RES_FOOD] < 0;
            if (starving || r.wf < 0.999) vc = C_RED;
            if (b->is_moon) snprintf(v, sizeof v, "-");
            if (starving) snprintf(s, sizeof s, "FOME!");
            else if (r.wf < 0.999 && !b->is_moon) snprintf(s, sizeof s, "Trab. %.0f%%", r.wf * 100);
            else {
                double gr = b->pop < r.pop_cap ? r.growth * b->pop * (1.0 - b->pop / r.pop_cap) : 0;
                char gg[24];
                fmt_short(gg, sizeof gg, gr);
                snprintf(s, sizeof s, "+%s/h", gg);
            }
            cv_text(x + 1, 3, s, starving || r.wf < 0.999 ? C_RED : C_GREEN, C_PANEL);
        } else if (i == 5) {
            fmt_num(v, sizeof v, b->res[RES_FOOD]);
            char rate[24];
            fmt_short(rate, sizeof rate, r.prod[RES_FOOD]);
            snprintf(s, sizeof s, "%s%s/h", r.prod[RES_FOOD] >= 0 ? "+" : "", rate);
            if (b->is_moon) { snprintf(v, sizeof v, "-"); s[0] = 0; }
            cv_text(x + 1, 3, s, r.prod[RES_FOOD] >= 0 ? C_GREEN : C_RED, C_PANEL);
        } else {
            fmt_num(v, sizeof v, g->dm);
            cv_text(x + 1, 3, "premium", C_DIM, C_PANEL);
        }
        cv_text_r(x + 14, 2, v, vc, C_PANEL);
        cv_bold_rect(x + 1, 2, 14, 1);
    }
}

static int menu_screen_for_key(int k) {
    if (k >= 'a' && k <= 'z') k -= 32;
    for (int i = 0; i < NMENU; i++) if (MENU[i].key == k) return MENU[i].scr;
    return -1;
}

int is_global_key(int k) {
    if (k >= 'a' && k <= 'z') k -= 32;
    if (menu_screen_for_key(k) >= 0) return 1;
    return k == 'Q' || k == 'L' || k == '[' || k == ']' || k == ',' || k == '.' || k == 'V';
}

static void draw_left(UI *u, Game *g) {
    cv_fill(0, 4, CX, CANVAS_H - 4, ' ', C_TEXT, C_BG);
    /* next mission */
    uint32_t qb = RGB(28, 24, 10);
    cv_fill(0, 4, 19, 7, ' ', C_TEXT, qb);
    cv_box(0, 4, 19, 7, C_GOLD, qb, 0);
    cv_text(2, 4, " Próxima missão ", C_GOLD, qb);
    char q[96], rw[96];
    quest_title(g, g->quest, q, sizeof q);
    cv_wrap(1, 5, 17, 3, q, C_WHITE, qb);
    if (g->quest < quest_count()) {
        double have, need;
        quest_progress(g, g->quest, &have, &need);
        char a[24], b[24];
        fmt_short(a, sizeof a, have > need ? need : have);
        fmt_short(b, sizeof b, need);
        ui_bar(1, 8, 9, need > 0 ? have / need : 0, C_GOLD);
        cv_textf(11, 8, C_DIM, qb, "%s/%s", a, b);
        quest_reward_text(g->quest, rw, sizeof rw);
        cv_text_w(1, 9, 17, rw, C_GREEN, qb);
    }
    int y = 12;
    for (int i = 0; i < NMENU; i++, y++) {
        int cur = u->screen == MENU[i].scr;
        uint32_t bg = cur ? C_SEL : C_PANEL;
        cv_fill(0, y, 19, 1, ' ', C_TEXT, bg);
        cv_put(1, y, (uint32_t)MENU[i].key, C_GOLD, bg);
        cv_text(3, y, MENU[i].label, cur ? C_WHITE : C_TEXT, bg);
        if (MENU[i].scr == SCR_MESSAGES) {
            int un = game_unread(g);
            if (un) cv_textf(15, y, C_RED, bg, "(%d)", un > 99 ? 99 : un);
        }
        if (MENU[i].scr == SCR_MERCHANT && g->ev.merchant_until > g->now) cv_text(15, y, "$", C_GOLD, bg);
        if (cur) cv_bold_rect(3, y, 15, 1);
    }
    cv_fill(0, y, 19, 1, ' ', C_TEXT, C_PANEL);
    cv_put(1, y, 'Q', C_GOLD, C_PANEL);
    cv_text(3, y, "Salvar e sair", C_TEXT, C_PANEL);
    y += 2;
    /* status */
    int th = threat_level(g, g->body[g->cur].is_moon ? g->body[g->cur].parent : g->cur);
    double nb = newbie_left(g);
    if (nb > 0) {
        cv_text(1, y, "Proteção novato", C_GREEN, C_BG);
        char a[24];
        fmt_num(a, sizeof a, nb);
        cv_textf(1, y + 1, C_DIM, C_BG, "faltam %s pts", a);
    } else {
        cv_text(1, y, "Ameaça", C_TEXT, C_BG);
        uint32_t tc = th >= 7 ? C_RED : th >= 4 ? C_ORANGE : C_GREEN;
        ui_bar(8, y, 7, th / 10.0, tc);
        cv_textf(16, y, tc, C_BG, "%d", th);
        cv_text(1, y + 1, th >= 7 ? "defesas fracas!" : th >= 4 ? "reforce defesas" : "bem protegido", C_DIM, C_BG);
    }
    static const char *OFF = "CAEGT";
    cv_text(1, y + 2, "Oficiais", C_DIM, C_BG);
    for (int i = 0; i < N_OFF; i++) cv_put(11 + i, y + 2, (uint32_t)OFF[i], officer_active(g, i) ? C_GOLD : RGB(50, 55, 70), C_BG);
}

static void draw_right(UI *u, Game *g) {
    (void)u;
    int x = 102;
    cv_fill(x, 4, CANVAS_W - x, CANVAS_H - 4, ' ', C_TEXT, C_BG);
    cv_fill(x, 4, 18, 1, ' ', C_TEXT, C_PANEL2);
    cv_textf(x + 1, 4, C_TITLE, C_PANEL2, "Planetas %d/%d", colonies_count(g), max_colonies(g));
    int y = 5;
    for (int i = 0; i < g->nbody && y < CANVAS_H - 1; i++) {
        Body *b = &g->body[i];
        if (!b->used || b->is_moon) continue;
        int cur = g->cur == i;
        uint32_t bg = cur ? C_SEL : C_BG;
        cv_fill(x, y, 18, 2, ' ', C_TEXT, bg);
        cv_put(x + 1, y, '(', C_DIM, bg);
        cv_put(x + 2, y, 0x25CF, biome_color(b->info.biome), bg);
        cv_put(x + 3, y, ')', C_DIM, bg);
        cv_text_w(x + 5, y, 13, b->name, cur ? C_WHITE : C_TEXT, bg);
        int hostile = 0;
        for (int k = 0; k < MAX_FLEETS; k++) if (g->fleet[k].used && g->fleet[k].mission == M_HOSTILE && (g->fleet[k].origin == i || (b->moon >= 0 && g->fleet[k].origin == b->moon))) hostile = 1;
        cv_textf(x + 2, y + 1, hostile ? C_RED : C_DIM, bg, "[%d:%d:%d]%s", b->g, b->s, b->p, hostile ? " !" : "");
        int busy = b->bq.n > 0 || b->lq.n > 0 || b->nsq > 0;
        if (busy) cv_put(x + 17, y + 1, 0x2022, C_GREEN, bg);
        y += 2;
        if (b->moon >= 0 && y < CANVAS_H) {
            int mc = g->cur == b->moon;
            uint32_t mb = mc ? C_SEL : C_BG;
            cv_fill(x, y, 18, 1, ' ', C_TEXT, mb);
            cv_text(x + 2, y, "└ (o) Lua", mc ? C_WHITE : RGB(170, 170, 180), mb);
            y++;
        }
    }
    if (y < CANVAS_H - 3) {
        cv_text(x + 1, CANVAS_H - 3, "[ ] trocar", C_DIM, C_BG);
        cv_text(x + 1, CANVAS_H - 2, "L lua", C_DIM, C_BG);
    }
}

static void draw_fleetbar(UI *u, Game *g) {
    int hostile = -1;
    double first = 1e300;
    int own = 0;
    double own_first = 1e300;
    int own_i = -1;
    for (int i = 0; i < MAX_FLEETS; i++) {
        Fleet *f = &g->fleet[i];
        if (!f->used) continue;
        if (f->mission == M_HOSTILE) { if (f->arrive < first) { first = f->arrive; hostile = i; } }
        else {
            own++;
            double t = f->state == FS_OUT ? f->arrive : f->state == FS_HOLD ? f->hold : f->ret;
            if (t < own_first) { own_first = t; own_i = i; }
        }
    }
    char b[200];
    if (g->ev.dec.active) {
        int blink = ((int)(u->t * 2)) & 1;
        cv_fill(CX, 4, CW, 1, ' ', C_TEXT, blink ? RGB(90, 70, 10) : RGB(60, 45, 5));
        snprintf(b, sizeof b, "⚠ EVENTO PENDENTE: %s  -  pressione V para decidir", g->ev.dec.title);
        cv_text_c(CX + CW / 2, 4, b, C_GOLD, COL_KEEP);
        return;
    }
    if (hostile >= 0) {
        Fleet *f = &g->fleet[hostile];
        int blink = ((int)(u->t * 2)) & 1;
        cv_fill(CX, 4, CW, 1, ' ', C_TEXT, blink ? RGB(120, 20, 20) : RGB(80, 10, 10));
        char t[32];
        fmt_time(t, sizeof t, f->arrive - g->now);
        char n[24];
        fmt_num(n, sizeof n, fleet_ship_count(f->ship));
        snprintf(b, sizeof b, "⚠ FROTA HOSTIL -> %s em %s  (%s naves)", g->body[f->origin].name, t, n);
        cv_text_c(CX + CW / 2, 4, b, C_WHITE, COL_KEEP);
        cv_bold_rect(CX, 4, CW, 1);
        return;
    }
    cv_fill(CX, 4, CW, 1, ' ', C_TEXT, C_PANEL2);
    if (own_i >= 0) {
        Fleet *f = &g->fleet[own_i];
        char t[32];
        fmt_time(t, sizeof t, own_first - g->now);
        snprintf(b, sizeof b, "%d frota(s) em missão  •  %s [%d:%d:%d] %s em %s", own, mission_name(f->mission), f->tg, f->ts, f->tp,
                 f->state == FS_RETURN ? "retorna" : f->state == FS_HOLD ? "explora" : "chega", t);
        cv_text_c(CX + CW / 2, 4, b, C_CYAN, C_PANEL2);
    } else {
        cv_text_c(CX + CW / 2, 4, "SEM MOVIMENTO DE FROTA", C_TITLE, C_PANEL2);
    }
}

/* ===================== OVERVIEW ===================== */
static void queue_panel(int x, int y, int w, int h, const char *title, Game *g, const Queue *q, const Body *b, int ship, const char *empty) {
    ui_panel(x, y, w, h, title);
    if (ship) {
        if (b->nsq == 0) { cv_wrap(x + 2, y + 2, w - 4, h - 3, empty, C_DIM, C_PANEL); return; }
        const SQItem *s = &b->sq[0];
        cv_text_w(x + 2, y + 1, w - 4, ITEMS[s->item].name, C_WHITE, C_PANEL);
        cv_textf(x + 2, y + 2, C_TEXT, C_PANEL, "restam %lld", s->count);
        if (s->next > 0) {
            double left = s->next - g->now;
            ui_bar(x + 2, y + 3, w - 4, 1.0 - left / s->unit, C_GREEN);
            double total = left + s->unit * (double)(s->count - 1);
            for (int i = 1; i < b->nsq; i++) total += b->sq[i].unit * (double)b->sq[i].count;
            char t[32];
            fmt_time(t, sizeof t, total);
            cv_textf(x + 2, y + 4, C_DIM, C_PANEL, "total %s", t);
        }
        if (b->nsq > 1) cv_textf(x + 2, y + 5, C_DIM, C_PANEL, "+%d lote(s)", b->nsq - 1);
        return;
    }
    if (q->n == 0) { cv_wrap(x + 2, y + 2, w - 4, h - 3, empty, C_DIM, C_PANEL); return; }
    const QItem *it = &q->it[0];
    cv_text_w(x + 2, y + 1, w - 4, ITEMS[it->item].name, C_WHITE, C_PANEL);
    cv_textf(x + 2, y + 2, C_GOLD, C_PANEL, "nível %lld", it->amount);
    if (it->active) {
        double tot = it->end - it->start, left = it->end - g->now;
        ui_bar(x + 2, y + 3, w - 4, tot > 0 ? 1.0 - left / tot : 1, C_GREEN);
        char t[32];
        fmt_time(t, sizeof t, left);
        cv_text(x + 2, y + 4, t, C_TEXT, C_PANEL);
    } else cv_text(x + 2, y + 3, "aguardando recursos", C_ORANGE, C_PANEL);
    if (q->n > 1) cv_textf(x + 2, y + 5, C_DIM, C_PANEL, "+%d na fila", q->n - 1);
}

static void info_line(int x, int y, const char *label, const char *val, uint32_t vc) {
    cv_text(x, y, label, C_DIM, C_PANEL);
    cv_text(x + 13, y, val, vc, C_PANEL);
}

static void draw_overview(UI *u, Game *g) {
    Body *b = &g->body[g->cur];
    Rates r;
    body_rates(g, b, &r);
    char t[160];
    snprintf(t, sizeof t, " Visão Geral - %s ", b->name);
    cv_fill(CX, CY, CW, 1, ' ', C_TEXT, C_BG);
    cv_text(CX + 1, CY, t, C_WHITE, C_BG);
    cv_bold_rect(CX + 1, CY, utf8_len(t), 1);
    cv_text_r(CX + CW - 2, CY, b->is_moon ? "Lua" : (g->cur == 0 ? "Planeta natal" : "Colônia"), C_DIM, C_BG);
    /* planet art */
    int ax = CX, ay = CY + 1, aw = 46, ah = 21;
    double lights = b->is_moon ? 0 : clampd(b->pop / 25000.0 + (b->pop > 300 ? 0.08 : 0), 0, 0.9);
    long long sats = b->ship[SI(S_SATELLITE)];
    art_planet(ax, ay, aw, ah, &b->info, u->t, lights, b->info.vseed, !b->is_moon && b->moon >= 0, (int)(sats > 0 ? 1 + sats / 15 : 0), b->bld[B_ORBITAL] > 0, 1);
    cv_box(ax, ay, aw, ah, C_BORDER, COL_KEEP, 2);
    /* info */
    int ix = CX + 46, iy = CY + 1, iw = 36, ih = 21;
    ui_panel(ix, iy, iw, ih, "Dados planetários");
    int x = ix + 2, y = iy + 1;
    char a[48], c[48];
    fmt_num(a, sizeof a, b->info.diameter);
    snprintf(t, sizeof t, "%skm (%d/%d)", a, body_fields_used(b), body_fields_max(g, b));
    info_line(x, y++, "Diâmetro:", t, C_TEXT);
    snprintf(t, sizeof t, "%d°C a %d°C", b->info.tmin, b->info.tmax);
    info_line(x, y++, "Temperatura:", t, b->info.tmax > 80 ? C_ORANGE : b->info.tmin < -60 ? C_CYAN : C_TEXT);
    snprintf(t, sizeof t, "[%d:%d:%d]", b->g, b->s, b->p);
    info_line(x, y++, "Posição:", t, C_CYAN);
    int tot;
    int rank = player_rank(g, &tot);
    fmt_num(a, sizeof a, player_points(g, NULL, NULL, NULL, NULL));
    snprintf(t, sizeof t, "%s (%d de %d)", a, rank, tot);
    info_line(x, y++, "Pontos:", t, C_TEXT);
    snprintf(t, sizeof t, "%.0f", g->honour);
    info_line(x, y++, "Honra:", t, g->honour < 0 ? C_RED : C_TEXT);
    info_line(x, y++, "Bioma:", BIOME_NAME[b->info.biome], biome_color(b->info.biome));
    if (b->is_moon) {
        info_line(x, y++, "Atmosfera:", "Nenhuma", C_TEXT);
        info_line(x, y++, "Ligada a:", g->body[b->parent].name, C_TEXT);
    } else {
        snprintf(t, sizeof t, "%.2f atm", b->info.pressure);
        info_line(x, y++, "Atmosfera:", t, C_TEXT);
        cv_text_w(x, y++, iw - 4, ATMO_NAME[b->info.atmo], sky_color(&b->info), C_PANEL);
        cv_text_w(x, y++, iw - 4, climate_text(&b->info), C_DIM, C_PANEL);
        snprintf(t, sizeof t, "%.2f g  Dia %.0fh", b->info.gravity, b->info.daylen);
        info_line(x, y++, "Gravidade:", t, C_TEXT);
        snprintf(t, sizeof t, "%.0f%%  Fértil %.0f%%", b->info.water * 100, b->info.fertility * 100);
        info_line(x, y++, "Água:", t, C_TEXT);
        snprintf(t, sizeof t, "%s / mag. %s", RAD_NAME[b->info.radiation], MAG_NAME[b->info.magneto]);
        info_line(x, y++, "Radiação:", t, b->info.radiation >= 2 ? C_ORANGE : C_TEXT);
        snprintf(t, sizeof t, "M%+.0f%% C%+.0f%% D%+.0f%%", (b->info.rich[0] - 1) * 100, (b->info.rich[1] - 1) * 100, (b->info.rich[2] - 1) * 100);
        info_line(x, y++, "Riqueza:", t, C_GOLD);
        if (b->moon >= 0) { fmt_num(c, sizeof c, g->body[b->moon].info.diameter); snprintf(t, sizeof t, "Sim (%s km)", c); }
        else snprintf(t, sizeof t, "Não");
        info_line(x, y++, "Lua:", t, b->moon >= 0 ? C_TEXT : C_DIM);
        y++;
        fmt_short(a, sizeof a, b->pop);
        fmt_short(c, sizeof c, r.pop_cap);
        snprintf(t, sizeof t, "%s / %s", a, c);
        info_line(x, y++, "População:", t, C_TEXT);
        snprintf(t, sizeof t, "%.0f%%  Energia %.0f%%", r.wf * 100, r.e_eff * 100);
        info_line(x, y++, "Trabalho:", t, (r.wf < 0.999 || r.e_eff < 0.999) ? C_RED : C_GREEN);
        snprintf(t, sizeof t, "%.0f%% (velocidade)", r.fac_eff * 100);
        info_line(x, y++, "Instalações:", t, r.fac_eff < 0.999 ? C_ORANGE : C_GREEN);
    }
    /* active modifiers */
    int my = y;
    for (int k = 0; k < MAX_MODS && my < iy + ih - 1; k++) {
        const Mod *m = &b->mod[k];
        if (m->until <= g->now) continue;
        static const char *MN[MOD_COUNT] = {"Metal", "Cristal", "Deutério", "Energia", "Alimento", "Cresc.", "Fusão off", "Satélites off", "Pesquisa", "Comércio"};
        char tl[32];
        fmt_time(tl, sizeof tl, m->until - g->now);
        if (m->kind == MOD_FUSION_OFF || m->kind == MOD_SAT_OFF) snprintf(t, sizeof t, "%s (%s)", MN[m->kind], tl);
        else snprintf(t, sizeof t, "%s %+.0f%% (%s)", MN[m->kind], (m->factor - 1) * 100, tl);
        cv_text_w(x, my++, iw - 4, t, m->factor >= 1 && m->kind != MOD_FUSION_OFF && m->kind != MOD_SAT_OFF ? C_GREEN : C_ORANGE, C_PANEL);
    }
    /* queues */
    int qy = CY + 22, qh = 8, qw = CW / 4;
    queue_panel(CX, qy, qw, qh, "Construções", g, &b->bq, b, 0, "Nenhuma construção. (2/4)");
    queue_panel(CX + qw, qy, qw, qh, "Civilização", g, &b->lq, b, 0, b->is_moon ? "Sem população na lua." : "Nada em construção. (3)");
    queue_panel(CX + 2 * qw, qy, qw, qh, "Pesquisa", g, &g->rq, &g->body[g->rq_body], 0, "Nenhuma pesquisa em andamento. (5)");
    queue_panel(CX + 3 * qw, qy, CW - 3 * qw, qh, "Estaleiro", g, NULL, b, 1, "Nenhuma nave ou defesa em produção. (6/7)");
    cv_fill(CX, CANVAS_H - 1, CW, 1, ' ', C_TEXT, C_BG);
    cv_text(CX + 1, CANVAS_H - 1, "N renomear  A abandonar colônia  F finalizar c/ Matéria Negra  X cancelar  L lua", C_DIM, C_BG);
}

/* ===================== BUILDING LISTS ===================== */
static int list_ids(const Game *g, int scr, int *ids) {
    const Body *b = &g->body[g->cur];
    int n = 0;
    if (scr == SCR_RESOURCES) {
        if (b->is_moon) { ids[n++] = B_HE3; ids[n++] = B_METAL_STORE; ids[n++] = B_CRYSTAL_STORE; ids[n++] = B_DEUT_TANK; return n; }
        for (int id = B_METAL_MINE; id <= B_DEUT_TANK; id++) ids[n++] = id;
        ids[n++] = S_SATELLITE;
        ids[n++] = S_CRAWLER;
    } else if (scr == SCR_FACILITIES) {
        if (b->is_moon) { ids[n++] = B_LUNAR_BASE; ids[n++] = B_PHALANX; ids[n++] = B_JUMP_GATE; ids[n++] = B_ROBOTICS; ids[n++] = B_SHIPYARD; return n; }
        for (int id = B_ROBOTICS; id <= B_TRADE_HUB; id++) ids[n++] = id;
    } else if (scr == SCR_LIFE) {
        for (int id = B_HABITAT; id <= B_ARCOLOGY; id++) ids[n++] = id;
    }
    return n;
}

static void effect_text(const Game *g, const Body *b, int id, char *o, size_t n) {
    int L = b->bld[id], N = L + 1;
    double tavg = (b->info.tmin + b->info.tmax) / 2.0;
    double sp = g->speed;
    char a[32], c[32], d[32];
    switch (id) {
    case B_METAL_MINE: case B_CRYSTAL_MINE: case B_DEUT_SYNTH: {
        double p0, p1;
        if (id == B_METAL_MINE) { p0 = prod_metal_lvl(L); p1 = prod_metal_lvl(N); }
        else if (id == B_CRYSTAL_MINE) { p0 = prod_crystal_lvl(L); p1 = prod_crystal_lvl(N); }
        else { p0 = prod_deut_lvl(L, tavg); p1 = prod_deut_lvl(N, tavg); }
        double rich = b->info.rich[id - B_METAL_MINE];
        fmt_num(a, sizeof a, p0 * sp * rich); fmt_num(c, sizeof c, p1 * sp * rich);
        fmt_num(d, sizeof d, energy_for(id, N) - energy_for(id, L));
        snprintf(o, n, "Produção: %s/h → %s/h  •  Energia -%s  •  Trabalhadores +%.0f", a, c, d, workers_for(id, N) - workers_for(id, L));
    } break;
    case B_SOLAR_PLANT:
        fmt_num(a, sizeof a, 20.0 * N * pow(1.1, N) - 20.0 * L * pow(1.1, L));
        snprintf(o, n, "Energia +%s  •  Trabalhadores +%.0f", a, workers_for(id, N) - workers_for(id, L));
        break;
    case B_FUSION: {
        double e0 = 30.0 * L * pow(1.05 + 0.01 * tech(g, T_ENERGY), L), e1 = 30.0 * N * pow(1.05 + 0.01 * tech(g, T_ENERGY), N);
        fmt_num(a, sizeof a, e1 - e0);
        fmt_num(c, sizeof c, (10.0 * N * pow(1.1, N) - 10.0 * L * pow(1.1, L)) * sp);
        snprintf(o, n, "Energia +%s  •  Consumo de deutério +%s/h", a, c);
    } break;
    case B_METAL_STORE: case B_CRYSTAL_STORE: case B_DEUT_TANK:
        fmt_num(a, sizeof a, storage_cap(L)); fmt_num(c, sizeof c, storage_cap(N));
        snprintf(o, n, "Capacidade: %s → %s", a, c);
        break;
    case B_ROBOTICS: snprintf(o, n, "Tempo de construção: -%.0f%% em relação ao nível atual", (1.0 - (1.0 + L) / (1.0 + N)) * 100); break;
    case B_SHIPYARD: snprintf(o, n, "Produção de naves/defesas %.0f%% mais rápida", ((1.0 + N) / (1.0 + L) - 1.0) * 100); break;
    case B_LAB: snprintf(o, n, "Pesquisas %.0f%% mais rápidas  •  libera tecnologias", ((1.0 + N) / (1.0 + L) - 1.0) * 100); break;
    case B_NANITE: snprintf(o, n, "Divide pela metade o tempo de construções, naves e defesas"); break;
    case B_TERRAFORMER: snprintf(o, n, "+5 campos (total %d)  •  exige %.0f de energia", body_fields_max(g, b) + 5, item_energy_req(id, N)); break;
    case B_MISSILE_SILO: snprintf(o, n, "Capacidade de mísseis: %d → %d", L * 10, N * 10); break;
    case B_SPACE_DOCK: snprintf(o, n, "Recupera %d%% das naves perdidas em defesa", N * 5); break;
    case B_ORBITAL: snprintf(o, n, "+2 min de aviso contra ataques  •  libera Astrofísica e Eng. Orbital"); break;
    case B_TRADE_HUB: snprintf(o, n, "Rotas comerciais a partir deste planeta: %d → %d", L, N); break;
    case B_LUNAR_BASE: snprintf(o, n, "+3 campos na lua (total %d)", body_fields_max(g, b) + 3); break;
    case B_PHALANX: snprintf(o, n, "+5 min de aviso e revela a composição de frotas hostis"); break;
    case B_JUMP_GATE: snprintf(o, n, "Portal de salto entre luas (prestígio e logística avançada)"); break;
    case B_HE3:
        fmt_num(a, sizeof a, 30.0 * L * pow(1.1, L) * sp); fmt_num(c, sizeof c, 30.0 * N * pow(1.1, N) * sp);
        snprintf(o, n, "Deutério: %s/h → %s/h (sem energia)", a, c);
        break;
    case B_HABITAT:
        fmt_num(a, sizeof a, 100.0 * L * pow(1.12, L)); fmt_num(c, sizeof c, 100.0 * N * pow(1.12, N));
        snprintf(o, n, "Capacidade populacional do setor: %s → %s", a, c);
        break;
    case B_FARM:
        fmt_num(a, sizeof a, 25.0 * L * pow(1.1, L) * b->info.fertility * sp); fmt_num(c, sizeof c, 25.0 * N * pow(1.1, N) * b->info.fertility * sp);
        fmt_num(d, sizeof d, storage_cap(N));
        snprintf(o, n, "Alimento: %s/h → %s/h  •  Silo %s  •  Energia -%.0f", a, c, d, energy_for(id, N) - energy_for(id, L));
        break;
    case B_MEDICAL: snprintf(o, n, "Crescimento populacional +%d%%  •  +1 no teste contra epidemias", N * 8); break;
    case B_ACADEMY: snprintf(o, n, "Pesquisas iniciadas aqui -%d%% de tempo", N * 3 > 45 ? 45 : N * 3); break;
    case B_FOOD_SYNTH:
        fmt_num(a, sizeof a, (40.0 * N * pow(1.1, N) - 40.0 * L * pow(1.1, L)) * sp);
        fmt_num(c, sizeof c, (8.0 * N * pow(1.1, N) - 8.0 * L * pow(1.1, L)) * sp);
        snprintf(o, n, "Alimento +%s/h  •  consome +%s deutério/h  •  Energia -%.0f", a, c, energy_for(id, N) - energy_for(id, L));
        break;
    case B_ARCOLOGY:
        fmt_num(a, sizeof a, 2500.0 * N * pow(1.15, N) - 2500.0 * L * pow(1.15, L));
        snprintf(o, n, "Capacidade populacional +%s  •  produz alimento", a);
        break;
    default: o[0] = 0;
    }
}

static void req_line(const Game *g, const Body *b, int id, char *o, size_t n) {
    const ItemDef *d = &ITEMS[id];
    o[0] = 0;
    for (int i = 0; i < 6 && d->req[i].lvl > 0; i++) {
        int have = req_have(g, b, d->req[i].id);
        size_t L = strlen(o);
        snprintf(o + L, n - L, "%s%s%s %d/%d\001d", L ? "  " : "", have >= d->req[i].lvl ? "\001g" : "\001r", ITEMS[d->req[i].id].shortn, have, d->req[i].lvl);
    }
}

static int queued_target(const Body *b, int id) {
    const Queue *q = ITEMS[id].cat == CAT_LIFE ? &b->lq : &b->bq;
    int t = 0;
    for (int i = 0; i < q->n; i++) if (q->it[i].item == id && q->it[i].amount > t) t = (int)q->it[i].amount;
    return t;
}

static void draw_hint(const char *s) {
    cv_fill(CX, CANVAS_H - 1, CW, 1, ' ', C_TEXT, C_BG);
    cv_text_w(CX + 1, CANVAS_H - 1, CW - 2, s, C_DIM, C_BG);
}

static void draw_building_screen(UI *u, Game *g, int scr) {
    Body *b = &g->body[g->cur];
    int ids[24];
    int n = list_ids(g, scr, ids);
    if (scr == SCR_LIFE && b->is_moon) {
        ui_panel(CX, CY, CW, CH - 1, "Civilização");
        cv_wrap(CX + 3, CY + 3, CW - 6, 5, "Luas não possuem população. Selecione um planeta (tecla L ou [ ]).", C_DIM, C_PANEL);
        return;
    }
    if (n == 0) return;
    int sel = clampi(u->sel[scr], 0, n - 1);
    u->sel[scr] = sel;
    const char *title = scr == SCR_RESOURCES ? (b->is_moon ? "Recursos Lunares" : "Recursos") : scr == SCR_FACILITIES ? (b->is_moon ? "Instalações Lunares" : "Instalações") : "Civilização";
    char t[160];
    snprintf(t, sizeof t, " %s - %s ", title, b->name);
    cv_fill(CX, CY, CW, 1, ' ', C_TEXT, C_BG);
    cv_text(CX + 1, CY, t, C_WHITE, C_BG);
    cv_bold_rect(CX + 1, CY, utf8_len(t), 1);
    int kind = b->is_moon ? LS_MOON : scr == SCR_RESOURCES ? LS_RESOURCES : scr == SCR_FACILITIES ? LS_FACILITIES : LS_LIFE;
    int lh = n >= 10 ? 12 : 13;
    art_landscape(CX, CY + 1, CW, lh, g, b, kind, u->t, ids[sel]);
    Rates r;
    body_rates(g, b, &r);
    if (scr == SCR_LIFE) {
        char a[24], c[24], d[24], e[24];
        fmt_short(a, sizeof a, b->pop); fmt_short(c, sizeof c, r.pop_cap);
        fmt_short(d, sizeof d, r.food_prod); fmt_short(e, sizeof e, r.food_cons);
        snprintf(t, sizeof t, " Pop %s/%s  Alimento +%s -%s/h  Trab. %.0f+%.0f  Força %.0f%% ", a, c, d, e, r.w_farm, r.w_req, r.wf * 100);
        cv_text_w(CX + 1, CY + 1, CW - 2, t, C_WHITE, RGB(0, 0, 0));
    }
    int ly = CY + 1 + lh;
    cv_fill(CX, ly, CW, 1, ' ', C_TEXT, C_PANEL2);
    cv_text(CX + 3, ly, "Construção", C_TITLE, C_PANEL2);
    cv_text(CX + 27, ly, "Nv", C_TITLE, C_PANEL2);
    cost_header(CX + 32, ly, C_PANEL2);
    cv_text(CX + 63, ly, "Tempo", C_TITLE, C_PANEL2);
    cv_text(CX + 73, ly, "Estado", C_TITLE, C_PANEL2);
    for (int i = 0; i < n; i++) {
        int y = ly + 1 + i;
        int id = ids[i];
        uint32_t bg = i == sel ? C_SEL : (i & 1 ? C_PANEL : RGB(16, 23, 39));
        cv_fill(CX, y, CW, 1, ' ', C_TEXT, bg);
        if (i == sel) cv_put(CX + 1, y, 0x25B6, C_GOLD, bg);
        if (IS_SHIP(id)) {
            long long cnt = b->ship[SI(id)];
            int pct = id == S_SATELLITE ? b->sat_pct : b->crawler_pct;
            snprintf(t, sizeof t, "%s (%d%%)", ITEMS[id].name, pct);
            cv_text_w(CX + 3, y, 24, t, C_TEXT, bg);
            char a[24];
            fmt_short(a, sizeof a, (double)cnt);
            cv_text_r(CX + 30, y, a, C_GOLD, bg);
            if (id == S_SATELLITE) { fmt_num(a, sizeof a, r.sat_each); snprintf(t, sizeof t, "%s energia cada", a); }
            else snprintf(t, sizeof t, "+0,02%% minas cada (máx %d)", 8 * (b->bld[B_METAL_MINE] + b->bld[B_CRYSTAL_MINE] + b->bld[B_DEUT_SYNTH]));
            cv_text(CX + 33, y, t, C_DIM, bg);
            cv_text(CX + 73, y, "Estal.", C_DIM, bg);
            continue;
        }
        int lvl = b->bld[id];
        int qt = queued_target(b, id);
        int pctv = b->pct[id];
        int producer = id == B_METAL_MINE || id == B_CRYSTAL_MINE || id == B_DEUT_SYNTH || id == B_SOLAR_PLANT || id == B_FUSION || id == B_FARM || id == B_FOOD_SYNTH;
        if (producer && pctv < 100) snprintf(t, sizeof t, "%s (%d%%)", ITEMS[id].name, pctv);
        else snprintf(t, sizeof t, "%s", ITEMS[id].name);
        cv_text_w(CX + 3, y, 24, t, lvl > 0 ? C_TEXT : C_DIM, bg);
        cv_textf(CX + 27, y, C_GOLD, bg, "%3d", lvl);
        int next = (qt > lvl ? qt : lvl) + 1;
        double c[3];
        double e = item_cost1(id, next, c);
        draw_cost(CX + 32, y, g, b, c, e);
        char tm[32];
        fmt_time(tm, sizeof tm, build_time(g, b, id, next));
        cv_text(CX + 63, y, tm, C_TEXT, bg);
        int rc = can_build(g, g->cur, id, 1);
        const char *st;
        uint32_t sc;
        if (qt > lvl) { st = b->bq.n && b->bq.it[0].item == id && b->bq.it[0].active ? "Em obra" : "Na fila"; sc = C_GOLD; }
        else if (rc == BC_OK) { st = "Construir"; sc = C_GREEN; }
        else if (rc == BC_RES) { st = "Recursos"; sc = C_RED; }
        else if (rc == BC_REQ) { st = "Requisit."; sc = C_RED; }
        else if (rc == BC_FIELDS) { st = "Campos"; sc = C_RED; }
        else if (rc == BC_QUEUE) { st = "Fila"; sc = C_ORANGE; }
        else if (rc == BC_BUSY) { st = "Ocupado"; sc = C_ORANGE; }
        else if (rc == BC_MAX) { st = "Máximo"; sc = C_DIM; }
        else if (rc == BC_ENERGY) { st = "Energia"; sc = C_RED; }
        else if (rc == BC_STORAGE) { st = "Armazém"; sc = C_RED; }
        else { st = "-"; sc = C_DIM; }
        cv_text(CX + 73, y, st, sc, bg);
        if (qt > lvl) {
            /* also mark that this row is in queue */
        }
    }
    /* details */
    int dy = ly + 1 + n;
    if (dy > CANVAS_H - 6) dy = CANVAS_H - 6;
    int dh = CANVAS_H - 1 - dy;
    ui_panel(CX, dy, CW, dh, ITEMS[ids[sel]].name);
    int id = ids[sel];
    char e[400];
    if (IS_SHIP(id)) {
        snprintf(e, sizeof e, "%s", ITEMS[id].desc);
        cv_wrap(CX + 2, dy + 1, CW - 4, 2, e, C_TEXT, C_PANEL);
        cv_text(CX + 2, dy + 3, "P: alterar produção  •  Enter: abrir o Estaleiro para construir", C_DIM, C_PANEL);
    } else {
        effect_text(g, b, id, e, sizeof e);
        cv_text_w(CX + 2, dy + 1, CW - 4, e, C_CYAN, C_PANEL);
        if (can_build(g, g->cur, id, 1) == BC_STORAGE) {
            cv_text_w(CX + 2, dy + 1, CW - 4, "\u26A0 O custo excede a capacidade dos armazéns: amplie o Armazém de Metal/Cristal/Tanque.", C_RED, C_PANEL);
        }
        req_line(g, b, id, e, sizeof e);
        if (e[0]) { cv_text(CX + 2, dy + 2, "Requisitos:", C_DIM, C_PANEL); cv_wrap(CX + 14, dy + 2, CW - 16, 1, e, C_TEXT, C_PANEL); }
        else cv_wrap(CX + 2, dy + 2, CW - 4, 1, ITEMS[id].desc, C_DIM, C_PANEL);
        const Queue *q = ITEMS[id].cat == CAT_LIFE ? &b->lq : &b->bq;
        if (q->n > 0) {
            const QItem *it = &q->it[0];
            char tm[32];
            fmt_time(tm, sizeof tm, it->active ? it->end - g->now : 0);
            snprintf(e, sizeof e, "Fila %d/%d: %s → %lld %s%s", q->n, queue_limit(g), ITEMS[it->item].name, it->amount,
                     it->active ? tm : "(aguardando recursos)", q->n > 1 ? "  •  X cancela o último" : "  •  X cancela");
            cv_text_w(CX + 2, dy + 3, CW - 4, e, C_GOLD, C_PANEL);
        } else {
            snprintf(e, sizeof e, "Campos %d/%d  •  Fila vazia (%d vagas)", body_fields_used(b), body_fields_max(g, b), queue_limit(g));
            cv_text(CX + 2, dy + 3, e, C_DIM, C_PANEL);
        }
    }
    draw_hint(scr == SCR_RESOURCES ? "↑↓ selecionar  Enter construir  P produção %  I info técnica  X cancelar  F finalizar c/ MN"
                                   : "↑↓ selecionar  Enter construir  I info técnica  X cancelar  F finalizar com Matéria Negra");
}

static void build_action(UI *u, Game *g, int id, long long count) {
    int rc = enqueue(g, g->cur, id, count);
    if (rc == BC_OK) ui_toast(u, C_GREEN, "%s adicionado à fila", ITEMS[id].name);
    else ui_toast(u, C_RED, "%s: %s", ITEMS[id].name, bc_text(rc));
}

static int building_key(UI *u, Game *g, int scr, int k) {
    Body *b = &g->body[g->cur];
    int ids[24];
    int n = list_ids(g, scr, ids);
    if (n == 0) return 0;
    int *sel = &u->sel[scr];
    switch (k) {
    case K_UP: *sel = (*sel + n - 1) % n; return 1;
    case K_DOWN: *sel = (*sel + 1) % n; return 1;
    case K_ENTER: case ' ': {
        int id = ids[clampi(*sel, 0, n - 1)];
        if (IS_SHIP(id)) { u->screen = SCR_SHIPYARD; u->sel[SCR_SHIPYARD] = SI(id); return 1; }
        build_action(u, g, id, 1);
        return 1;
    }
    case 'i': case 'I': ui_open_tech(u, ids[clampi(*sel, 0, n - 1)]); return 1;
    case 'p': case 'P': {
        int id = ids[clampi(*sel, 0, n - 1)];
        int *pp = NULL;
        if (id == S_SATELLITE) pp = &b->sat_pct;
        else if (id == S_CRAWLER) pp = &b->crawler_pct;
        else if (id == B_METAL_MINE || id == B_CRYSTAL_MINE || id == B_DEUT_SYNTH || id == B_SOLAR_PLANT || id == B_FUSION || id == B_FARM || id == B_FOOD_SYNTH) pp = &b->pct[id];
        if (pp) { *pp = *pp <= 0 ? 100 : *pp - 10; ui_toast(u, C_CYAN, "%s: produção %d%%", ITEMS[id].name, *pp); }
        return 1;
    }
    case 'x': case 'X': {
        int which = scr == SCR_LIFE ? 1 : 0;
        Queue *q = which ? &b->lq : &b->bq;
        if (q->n > 0) {
            ui_confirm(u, MP_CANCEL_Q, which, "Cancelar construção",
                       "Cancelar %s nível %lld?\nRecursos de uma obra em andamento são devolvidos integralmente.",
                       ITEMS[q->it[q->n - 1].item].name, q->it[q->n - 1].amount);
        }
        return 1;
    }
    case 'f': case 'F': {
        int which = scr == SCR_LIFE ? 1 : 0;
        Queue *q = which ? &b->lq : &b->bq;
        if (q->n > 0 && q->it[0].active) {
            double c = dm_finish_cost(g, q->it[0].end - g->now);
            ui_confirm(u, MP_FINISH_DM, which, "Finalizar com Matéria Negra", "Concluir %s agora por %.0f Matéria Negra? (você tem %.0f)",
                       ITEMS[q->it[0].item].name, c, g->dm);
        }
        return 1;
    }
    }
    return 0;
}

/* ===================== RESEARCH ===================== */
static const int RESEARCH_ORDER[] = {
    -1, T_ENERGY, T_LASER, T_ION, T_HYPERSPACE, T_PLASMA,
    -2, T_COMBUSTION, T_IMPULSE, T_HYPERDRIVE,
    -3, T_ESPIONAGE, T_COMPUTER, T_ASTRO, T_IRN, T_GRAVITON,
    -4, T_WEAPONS, T_SHIELDING, T_ARMOR,
    -5, T_BIOTECH, T_MEDICINE, T_GEOPHYSICS, T_ORBITAL_ENG, T_LOGISTICS};
#define NRO ((int)(sizeof RESEARCH_ORDER / sizeof RESEARCH_ORDER[0]))
static const char *RHEAD[] = {"", "Pesquisa básica", "Propulsão", "Pesquisa avançada", "Pesquisa de combate", "Civilização e logística"};

static void draw_research(UI *u, Game *g) {
    Body *b = &g->body[g->cur];
    int items[32], ni = 0;
    for (int i = 0; i < NRO; i++) if (RESEARCH_ORDER[i] >= 0) items[ni++] = RESEARCH_ORDER[i];
    int sel = clampi(u->sel[SCR_RESEARCH], 0, ni - 1);
    u->sel[SCR_RESEARCH] = sel;
    char t[200];
    snprintf(t, sizeof t, " Pesquisa - laboratório de %s (nível %d) ", b->name, b->bld[B_LAB]);
    cv_fill(CX, CY, CW, 1, ' ', C_TEXT, C_BG);
    cv_text(CX + 1, CY, t, C_WHITE, C_BG);
    cv_bold_rect(CX + 1, CY, utf8_len(t), 1);
    /* research status */
    cv_fill(CX, CY + 1, CW, 1, ' ', C_TEXT, C_PANEL2);
    if (g->rq.n > 0) {
        QItem *it = &g->rq.it[0];
        char tm[32];
        fmt_time(tm, sizeof tm, it->active ? it->end - g->now : 0);
        snprintf(t, sizeof t, "Pesquisando %s nível %lld em %s %s  (fila %d/%d)", ITEMS[it->item].name, it->amount, g->body[g->rq_body].name,
                 it->active ? tm : "aguardando recursos", g->rq.n, queue_limit(g));
        cv_text_w(CX + 1, CY + 1, 60, t, C_GOLD, C_PANEL2);
        if (it->active) ui_bar(CX + 62, CY + 1, 18, 1.0 - (it->end - g->now) / (it->end - it->start + 1e-9), C_CYAN);
    } else cv_text(CX + 1, CY + 1, "Nenhuma pesquisa em andamento.", C_DIM, C_PANEL2);
    /* list with headers, scrolling */
    int rows = 20;
    int top = CY + 2;
    /* map selection index to row index */
    int rowsel = 0, k = 0;
    for (int i = 0; i < NRO; i++) { if (RESEARCH_ORDER[i] >= 0) { if (k == sel) { rowsel = i; break; } k++; } }
    int sc = u->scroll[SCR_RESEARCH];
    if (rowsel < sc + 1) sc = rowsel - 1;
    if (rowsel >= sc + rows) sc = rowsel - rows + 1;
    if (sc < 0) sc = 0;
    if (sc > NRO - rows) sc = NRO - rows;
    u->scroll[SCR_RESEARCH] = sc;
    for (int rr = 0; rr < rows; rr++) {
        int i = sc + rr;
        int y = top + rr;
        if (i >= NRO) { cv_fill(CX, y, CW, 1, ' ', C_TEXT, C_BG); continue; }
        int id = RESEARCH_ORDER[i];
        if (id < 0) {
            cv_fill(CX, y, CW, 1, ' ', C_TEXT, C_BG);
            cv_textf(CX + 1, y, C_TITLE, C_BG, "── %s ", RHEAD[-id]);
            cv_bold_rect(CX + 4, y, 30, 1);
            continue;
        }
        int is_sel = i == rowsel;
        uint32_t bg = is_sel ? C_SEL : ((rr & 1) ? C_PANEL : RGB(16, 23, 39));
        cv_fill(CX, y, CW, 1, ' ', C_TEXT, bg);
        if (is_sel) cv_put(CX + 1, y, 0x25B6, C_GOLD, bg);
        int lvl = g->tech[TI(id)];
        int qt = 0;
        for (int q = 0; q < g->rq.n; q++) if (g->rq.it[q].item == id && g->rq.it[q].amount > qt) qt = (int)g->rq.it[q].amount;
        cv_text_w(CX + 3, y, 24, ITEMS[id].name, reqs_met(g, b, id) ? C_TEXT : C_DIM, bg);
        cv_textf(CX + 27, y, C_GOLD, bg, "%3d", lvl);
        int next = (qt > lvl ? qt : lvl) + 1;
        double c[3];
        double e = item_cost1(id, next, c);
        draw_cost(CX + 32, y, g, b, c, e);
        char tm[32];
        fmt_time(tm, sizeof tm, b->is_moon ? 0 : research_time(g, g->cur, id, next));
        cv_text(CX + 63, y, b->is_moon ? "-" : tm, C_TEXT, bg);
        int rc = can_build(g, g->cur, id, 1);
        const char *st = qt > lvl ? "Na fila" : rc == BC_OK ? "Pesquisar" : rc == BC_LABBUSY ? "Outro pl." : rc == BC_STORAGE ? "Armazém" : rc == BC_RES ? "Recursos" : rc == BC_REQ ? "Requisit." : rc == BC_BUSY ? "Ocupado" : rc == BC_QUEUE ? "Fila" : rc == BC_ENERGY ? "Energia" : rc == BC_PLACE ? "Planeta" : "-";
        uint32_t scc = qt > lvl ? C_GOLD : rc == BC_OK ? C_GREEN : (rc == BC_BUSY || rc == BC_QUEUE || rc == BC_LABBUSY) ? C_ORANGE : C_RED;
        cv_text(CX + 73, y, st, scc, bg);
    }
    int id = items[sel];
    int dy = top + rows;
    ui_panel(CX, dy, CW, CANVAS_H - 1 - dy, ITEMS[id].name);
    cv_wrap(CX + 2, dy + 1, CW - 4, 2, ITEMS[id].desc, C_TEXT, C_PANEL);
    char e[400];
    req_line(g, b, id, e, sizeof e);
    if (e[0]) { cv_text(CX + 2, dy + 3, "Requisitos:", C_DIM, C_PANEL); cv_wrap(CX + 14, dy + 3, CW - 16, 1, e, C_TEXT, C_PANEL); }
    else cv_text(CX + 2, dy + 3, "Sem requisitos.", C_DIM, C_PANEL);
    draw_hint("↑↓ selecionar  Enter pesquisar  I árvore/info técnica  X cancelar pesquisa  F finalizar com Matéria Negra");
}

static int research_key(UI *u, Game *g, int k) {
    int items[32], ni = 0;
    for (int i = 0; i < NRO; i++) if (RESEARCH_ORDER[i] >= 0) items[ni++] = RESEARCH_ORDER[i];
    int *sel = &u->sel[SCR_RESEARCH];
    switch (k) {
    case K_UP: *sel = (*sel + ni - 1) % ni; return 1;
    case K_DOWN: *sel = (*sel + 1) % ni; return 1;
    case K_PGUP: *sel = clampi(*sel - 8, 0, ni - 1); return 1;
    case K_PGDN: *sel = clampi(*sel + 8, 0, ni - 1); return 1;
    case K_ENTER: case ' ': build_action(u, g, items[clampi(*sel, 0, ni - 1)], 1); return 1;
    case 'i': case 'I': ui_open_tech(u, items[clampi(*sel, 0, ni - 1)]); return 1;
    case 'x': case 'X':
        if (g->rq.n > 0) ui_confirm(u, MP_CANCEL_Q, 2, "Cancelar pesquisa", "Cancelar %s nível %lld?", ITEMS[g->rq.it[g->rq.n - 1].item].name, g->rq.it[g->rq.n - 1].amount);
        return 1;
    case 'f': case 'F':
        if (g->rq.n > 0 && g->rq.it[0].active) {
            double c = dm_finish_cost(g, g->rq.it[0].end - g->now);
            ui_confirm(u, MP_FINISH_DM, 2, "Finalizar com Matéria Negra", "Concluir %s agora por %.0f Matéria Negra? (você tem %.0f)", ITEMS[g->rq.it[0].item].name, c, g->dm);
        }
        return 1;
    }
    return 0;
}

/* ===================== SHIPYARD / DEFENSE ===================== */
static int yard_ids(int scr, int *ids) {
    int n = 0;
    if (scr == SCR_SHIPYARD) for (int id = T_END; id < S_END; id++) ids[n++] = id;
    else for (int id = S_END; id < D_END; id++) ids[n++] = id;
    return n;
}

static void draw_yard(UI *u, Game *g, int scr) {
    Body *b = &g->body[g->cur];
    int ids[32];
    int n = yard_ids(scr, ids);
    int sel = clampi(u->sel[scr], 0, n - 1);
    u->sel[scr] = sel;
    char t[200];
    snprintf(t, sizeof t, " %s - %s (Estaleiro nível %d) ", scr == SCR_SHIPYARD ? "Estaleiro" : "Defesa", b->name, b->bld[B_SHIPYARD]);
    cv_fill(CX, CY, CW, 1, ' ', C_TEXT, C_BG);
    cv_text(CX + 1, CY, t, C_WHITE, C_BG);
    cv_bold_rect(CX + 1, CY, utf8_len(t), 1);
    /* queue line */
    cv_fill(CX, CY + 1, CW, 1, ' ', C_TEXT, C_PANEL2);
    if (b->nsq > 0) {
        t[0] = 0;
        for (int i = 0; i < b->nsq && strlen(t) < 150; i++) {
            size_t L = strlen(t);
            snprintf(t + L, sizeof t - L, "%s%lldx %s", i ? ", " : "Produzindo: ", b->sq[i].count, ITEMS[b->sq[i].item].shortn);
        }
        cv_text_w(CX + 1, CY + 1, 58, t, C_GOLD, C_PANEL2);
        if (b->sq[0].next > 0) {
            char tm[32];
            fmt_time(tm, sizeof tm, b->sq[0].next - g->now);
            cv_textf(CX + 60, CY + 1, C_TEXT, C_PANEL2, "próx. %s", tm);
        }
    } else cv_text(CX + 1, CY + 1, "Fila do estaleiro vazia.", C_DIM, C_PANEL2);
    if (scr == SCR_DEFENSE) {
        char a[48];
        snprintf(a, sizeof a, "Silo %d/%d", missile_used(b), missile_capacity(b));
        cv_text_r(CX + CW - 2, CY + 1, a, C_DIM, C_PANEL2);
    }
    int ly = CY + 2;
    cv_fill(CX, ly, CW, 1, ' ', C_TEXT, C_PANEL2);
    cv_text(CX + 3, ly, scr == SCR_SHIPYARD ? "Nave" : "Defesa", C_TITLE, C_PANEL2);
    cv_text(CX + 25, ly, "Possui", C_TITLE, C_PANEL2);
    cost_header(CX + 32, ly, C_PANEL2);
    cv_text(CX + 63, ly, "Tempo/un", C_TITLE, C_PANEL2);
    cv_text(CX + 73, ly, "Máx", C_TITLE, C_PANEL2);
    for (int i = 0; i < n; i++) {
        int y = ly + 1 + i;
        int id = ids[i];
        uint32_t bg = i == sel ? C_SEL : (i & 1 ? C_PANEL : RGB(16, 23, 39));
        cv_fill(CX, y, CW, 1, ' ', C_TEXT, bg);
        if (i == sel) cv_put(CX + 1, y, 0x25B6, C_GOLD, bg);
        int ok = reqs_met(g, b, id) && (ITEMS[id].where & (b->is_moon ? W_MOON : W_PLANET));
        cv_text_w(CX + 3, y, 22, ITEMS[id].name, ok ? C_TEXT : C_DIM, bg);
        long long have = IS_SHIP(id) ? b->ship[SI(id)] : b->def[DI(id)];
        char a[24];
        fmt_short(a, sizeof a, (double)have);
        cv_text_r(CX + 30, y, a, have > 0 ? C_GOLD : C_DIM, bg);
        double c[3];
        item_cost1(id, 1, c);
        draw_cost(CX + 32, y, g, b, c, 0);
        char tm[32];
        fmt_time(tm, sizeof tm, unit_time(g, b, id));
        cv_text(CX + 63, y, b->bld[B_SHIPYARD] > 0 ? tm : "-", C_TEXT, bg);
        if (ok) {
            long long mx = max_affordable(g, b, id);
            fmt_short(a, sizeof a, (double)mx);
            cv_text(CX + 73, y, a, mx > 0 ? C_GREEN : C_RED, bg);
        } else cv_text(CX + 73, y, "bloq.", C_RED, bg);
    }
    int id = ids[sel];
    int dy = ly + 1 + n;
    int dh = CANVAS_H - 1 - dy;
    ui_panel(CX, dy, CW, dh, ITEMS[id].name);
    const ItemDef *d = &ITEMS[id];
    art_ship(CX + 2, dy + 1, id, u->t);
    int sx = CX + 18;
    if (id == D_ABM || id == D_IPM) {
        cv_wrap(sx, dy + 1, CW - 20, 3, d->desc, C_TEXT, C_PANEL);
    } else {
        char a[24], s2[24], h[24];
        double wt = 1 + 0.1 * g->tech[TI(T_WEAPONS)], st = 1 + 0.1 * g->tech[TI(T_SHIELDING)], at = 1 + 0.1 * g->tech[TI(T_ARMOR)];
        fmt_num(a, sizeof a, d->att * wt); fmt_num(s2, sizeof s2, d->shd * st); fmt_num(h, sizeof h, d->si * at);
        snprintf(t, sizeof t, "Ataque %s  Escudo %s  Estrutura %s", a, s2, h);
        cv_text(sx, dy + 1, t, C_CYAN, C_PANEL);
        if (IS_SHIP(id)) {
            char cg[24], sp[24];
            long long one[N_SHIP] = {0};
            one[SI(id)] = 1;
            fmt_num(cg, sizeof cg, fleet_cargo_cap(g, one));
            fmt_num(sp, sizeof sp, ship_speed(g, id));
            snprintf(t, sizeof t, "Carga %s  Velocidade %s  Combustível %.0f", cg, sp, d->fuel);
            cv_text(sx, dy + 2, t, C_TEXT, C_PANEL);
        }
        /* rapidfire */
        int u0 = item_unit(id);
        t[0] = 0;
        for (int v = 0; v < N_UNIT && u0 >= 0; v++) {
            int rf = rapidfire(u0, v);
            if (rf > 1 && strlen(t) < 120) {
                size_t L = strlen(t);
                snprintf(t + L, sizeof t - L, "%s%s x%d", L ? ", " : "Disparo rápido: ", ITEMS[unit_item(v)].shortn, rf);
            }
        }
        if (t[0]) cv_text_w(sx, dy + 3, CW - 20, t, C_ORANGE, C_PANEL);
        else cv_text_w(sx, dy + 3, CW - 20, d->desc, C_DIM, C_PANEL);
    }
    char e[300];
    req_line(g, b, id, e, sizeof e);
    if (e[0] && !reqs_met(g, b, id)) cv_wrap(sx, dy + 4, CW - 20, 1, e, C_TEXT, C_PANEL);
    draw_hint("↑↓ selecionar  Enter construir (quantidade)  I info técnica  X cancelar último lote  F finalizar c/ MN");
}

static int yard_key(UI *u, Game *g, int scr, int k) {
    int ids[32];
    int n = yard_ids(scr, ids);
    int *sel = &u->sel[scr];
    Body *b = &g->body[g->cur];
    switch (k) {
    case K_UP: *sel = (*sel + n - 1) % n; return 1;
    case K_DOWN: *sel = (*sel + 1) % n; return 1;
    case K_ENTER: case ' ': {
        int id = ids[clampi(*sel, 0, n - 1)];
        long long mx = max_affordable(g, b, id);
        char txt[200];
        snprintf(txt, sizeof txt, "Quantas unidades de %s?\nMáximo com seus recursos: %lld  (digite o número; Enter confirma)", ITEMS[id].name, mx);
        ui_prompt(u, MP_BUILD_QTY, id, "Construir", txt, mx > 0 ? "1" : "0", 1);
        return 1;
    }
    case 'i': case 'I': ui_open_tech(u, ids[clampi(*sel, 0, n - 1)]); return 1;
    case 'x': case 'X':
        if (b->nsq > 0) ui_confirm(u, MP_CANCEL_Q, 3, "Cancelar lote", "Cancelar o lote de %lldx %s? Os recursos restantes são devolvidos.", b->sq[b->nsq - 1].count, ITEMS[b->sq[b->nsq - 1].item].name);
        return 1;
    case 'f': case 'F':
        if (b->nsq > 0) {
            double rem = 0;
            for (int i = 0; i < b->nsq; i++) rem += b->sq[i].unit * (double)b->sq[i].count;
            ui_confirm(u, MP_FINISH_DM, 3, "Finalizar com Matéria Negra", "Concluir toda a fila do estaleiro por %.0f Matéria Negra? (você tem %.0f)", dm_finish_cost(g, rem), g->dm);
        }
        return 1;
    }
    return 0;
}

/* ===================== OVERVIEW KEYS ===================== */
static int overview_key(UI *u, Game *g, int k) {
    Body *b = &g->body[g->cur];
    switch (k) {
    case 'n': case 'N': ui_prompt(u, MP_RENAME, g->cur, "Renomear", "Novo nome para este corpo celeste:", b->name, 0); return 1;
    case 'a': case 'A':
        if (g->cur == 0) { ui_toast(u, C_RED, "O planeta natal não pode ser abandonado."); return 1; }
        ui_confirm(u, MP_ABANDON, g->cur, "Abandonar", "Abandonar %s [%d:%d:%d]? Construções, naves e recursos estacionados serão perdidos.", b->name, b->g, b->s, b->p);
        return 1;
    case 'f': case 'F':
        if (b->bq.n > 0 && b->bq.it[0].active) {
            double c = dm_finish_cost(g, b->bq.it[0].end - g->now);
            ui_confirm(u, MP_FINISH_DM, 0, "Finalizar com Matéria Negra", "Concluir %s agora por %.0f Matéria Negra? (você tem %.0f)", ITEMS[b->bq.it[0].item].name, c, g->dm);
        }
        return 1;
    case 'x': case 'X':
        if (b->bq.n > 0) ui_confirm(u, MP_CANCEL_Q, 0, "Cancelar construção", "Cancelar %s nível %lld?", ITEMS[b->bq.it[b->bq.n - 1].item].name, b->bq.it[b->bq.n - 1].amount);
        return 1;
    }
    return 0;
}

/* ===================== MODALS ===================== */
static void modal_box(int w, int h, const char *title, int *ox, int *oy, uint32_t border) {
    int x = (CANVAS_W - w) / 2, y = (CANVAS_H - h) / 2;
    cv_fill(x + 1, y + 1, w, h, ' ', C_TEXT, RGB(0, 0, 0));
    cv_fill(x, y, w, h, ' ', C_TEXT, C_PANEL2);
    cv_box(x, y, w, h, border, C_PANEL2, 1);
    int n = utf8_len(title);
    cv_text(x + (w - n) / 2, y, title, C_GOLD, C_PANEL2);
    cv_bold_rect(x + (w - n) / 2, y, n, 1);
    *ox = x; *oy = y;
}

static void draw_event_modal(UI *u, Game *g) {
    Decision *d = &g->ev.dec;
    int x, y, w = 78, h = 26;
    modal_box(w, h, d->title, &x, &y, C_ORANGE);
    int lines = cv_wrap(x + 3, y + 2, w - 6, 4, d->text, C_TEXT, C_PANEL2);
    int yy = y + 2 + (lines > 4 ? 4 : lines) + 1;
    char t[200];
    snprintf(t, sizeof t, "Teste: d20 + %d (%s) contra CD %d", d->base_mod, d->mod_text, d->dc);
    cv_text_w(x + 3, yy, w - 6, t, C_CYAN, C_PANEL2);
    yy += 2;
    Body *b = &g->body[d->body];
    if (d->stage == 0) {
        for (int i = 0; i < d->nopt; i++) {
            int ok = 1;
            for (int k = 0; k < NRES; k++) if (b->res[k] + 1e-6 < d->optcost[i][k]) ok = 0;
            if (d->type == EV_METEOR && i == 0 && b->def[DI(D_ABM)] <= 0) ok = 0;
            cv_textf(x + 3, yy, C_GOLD, C_PANEL2, "[%d]", i + 1);
            cv_text_w(x + 7, yy, 50, d->opt[i], ok ? C_WHITE : C_DIM, C_PANEL2);
            char c[96] = "";
            for (int k = 0; k < NRES; k++) if (d->optcost[i][k] > 0) {
                char a[24];
                fmt_short(a, sizeof a, d->optcost[i][k]);
                size_t L = strlen(c);
                snprintf(c + L, sizeof c - L, "%s%s %s", L ? " " : "", a, k == 0 ? "M" : k == 1 ? "C" : k == 2 ? "D" : "Alim");
            }
            if (c[0]) cv_text_r(x + w - 4, yy, c, ok ? C_TEXT : C_RED, C_PANEL2);
            if (!ok) cv_text_r(x + w - 4, yy + 1, "(indisponível)", C_RED, C_PANEL2);
            yy += 2;
        }
        char tm[32];
        fmt_time(tm, sizeof tm, d->deadline - g->now);
        snprintf(t, sizeof t, "Escolha 1-%d.  Esc adia (decisão automática em %s: opção %d).", d->nopt, tm, d->defopt + 1);
        cv_text_w(x + 3, y + h - 2, w - 6, t, C_DIM, C_PANEL2);
        return;
    }
    /* rolled */
    cv_textf(x + 3, yy, C_TEXT, C_PANEL2, "Decisão: %s", d->opt[d->chosen]);
    yy += 1;
    double since = u->t - u->dice_t0;
    int auto_opt = d->optbonus[d->chosen] == -1000;
    if (!auto_opt) {
        int shown = d->roll;
        uint32_t col = C_WHITE;
        if (since < 1.3) { shown = 1 + (int)(fmod(u->t * 23.0, 20.0)); col = C_GOLD; }
        else col = d->outcome >= 1 && d->outcome <= 2 ? C_GREEN : C_RED;
        art_d20(x + 4, yy + 1, shown, col);
        if (since >= 1.3) {
            snprintf(t, sizeof t, "d20 = %d  + %d  + %d (opção)  =  %d   vs CD %d", d->roll, d->base_mod, d->optbonus[d->chosen], d->total, d->dc);
            cv_text(x + 20, yy + 2, t, C_WHITE, C_PANEL2);
            const char *oc = d->outcome == 2 ? "SUCESSO CRÍTICO!" : d->outcome == 1 ? "SUCESSO" : d->outcome == 3 ? "FALHA CRÍTICA!" : "FALHA";
            cv_text(x + 20, yy + 4, oc, d->outcome == 1 || d->outcome == 2 ? C_GREEN : C_RED, C_PANEL2);
            cv_bold_rect(x + 20, yy + 4, 20, 1);
            if (d->rerolls < 2 && g->dm >= 300) cv_text(x + 20, yy + 6, "R: rolar novamente (300 Matéria Negra)", C_PURPLE, C_PANEL2);
        }
        yy += 9;
    } else yy += 1;
    if (since >= 1.3 || auto_opt) {
        const char *preview = d->outcome == 2 ? "Resultado excelente." : d->outcome == 1 ? "Resultado favorável." : "Resultado desfavorável.";
        if (auto_opt) preview = "Ação sem teste: o efeito será aplicado.";
        cv_text(x + 3, yy, preview, C_TEXT, C_PANEL2);
        cv_text(x + 3, y + h - 2, "Enter: aceitar o resultado", C_DIM, C_PANEL2);
    }
}

static void draw_modal(UI *u, Game *g) {
    int x, y;
    switch (u->modal) {
    case MD_PROMPT: {
        int w = 64, h = 10;
        modal_box(w, h, u->mtitle, &x, &y, C_BORDER);
        cv_wrap(x + 3, y + 2, w - 6, 3, u->mtext, C_TEXT, C_PANEL2);
        cv_fill(x + 3, y + 6, w - 6, 1, ' ', C_WHITE, RGB(5, 8, 14));
        cv_text(x + 4, y + 6, u->minput, C_WHITE, RGB(5, 8, 14));
        if (((int)(u->t * 3)) & 1) cv_put(x + 4 + utf8_len(u->minput), y + 6, '_', C_GOLD, RGB(5, 8, 14));
        cv_text(x + 3, y + 8, u->mnumeric ? "Enter confirma  Esc cancela  M = máximo" : "Enter confirma  Esc cancela", C_DIM, C_PANEL2);
    } break;
    case MD_MENU: {
        int w = 60, h = u->mn + 5;
        modal_box(w, h, u->mtitle, &x, &y, C_BORDER);
        for (int i = 0; i < u->mn; i++) {
            uint32_t bg = i == u->msel ? C_SEL : C_PANEL2;
            cv_fill(x + 2, y + 2 + i, w - 4, 1, ' ', C_TEXT, bg);
            cv_textf(x + 3, y + 2 + i, C_GOLD, bg, "%d", i + 1);
            cv_text_w(x + 6, y + 2 + i, w - 9, u->mitems[i], u->mok[i] ? C_WHITE : C_DIM, bg);
        }
        cv_text(x + 3, y + h - 2, "↑↓ Enter escolher  Esc cancelar", C_DIM, C_PANEL2);
    } break;
    case MD_INFO: case MD_AWAY: {
        int w = 72, h = 22;
        modal_box(w, h, u->mtitle, &x, &y, u->modal == MD_AWAY ? C_GOLD : C_BORDER);
        cv_wrap(x + 3, y + 2, w - 6, h - 5, u->mtext, C_TEXT, C_PANEL2);
        cv_text(x + 3, y + h - 2, "Pressione qualquer tecla", C_DIM, C_PANEL2);
    } break;
    case MD_CONFIRM: {
        int w = 64, h = 10;
        modal_box(w, h, u->mtitle, &x, &y, C_ORANGE);
        cv_wrap(x + 3, y + 2, w - 6, 5, u->mtext, C_TEXT, C_PANEL2);
        cv_text(x + 3, y + h - 2, "S/Enter confirma   N/Esc cancela", C_DIM, C_PANEL2);
    } break;
    case MD_EVENT:
        if (g->ev.dec.active) draw_event_modal(u, g);
        else u->modal = MD_NONE;
        break;
    }
}

static void modal_result(UI *u, Game *g, int ok, const char *input, int choice) {
    int p = u->mpurpose, arg = u->marg;
    u->modal = MD_NONE;
    Body *b = &g->body[g->cur];
    if (!ok) return;
    switch (p) {
    case MP_BUILD_QTY: {
        long long n = 0;
        if (input[0] == 'M' || input[0] == 'm') n = max_affordable(g, b, arg);
        else n = atoll(input);
        if (n <= 0) return;
        build_action(u, g, arg, n);
    } break;
    case MP_RENAME: game_rename(g, arg, input); break;
    case MP_ABANDON:
        if (game_abandon(g, arg)) ui_toast(u, C_ORANGE, "Corpo celeste abandonado.");
        break;
    case MP_FINISH_DM: {
        int r = finish_with_dm(g, g->cur, arg);
        if (r < 0) ui_toast(u, C_RED, "Matéria Negra insuficiente");
        else if (r > 0) ui_toast(u, C_PURPLE, "Concluído com Matéria Negra!");
    } break;
    case MP_CANCEL_Q: {
        Queue *q = arg == 0 ? &b->bq : arg == 1 ? &b->lq : arg == 2 ? &g->rq : NULL;
        int idx = q ? q->n - 1 : b->nsq - 1;
        if (cancel_queue(g, g->cur, arg, idx)) ui_toast(u, C_ORANGE, "Cancelado.");
    } break;
    case MP_QUIT: u->quit = 1; break;
    default: modal_result_screens(u, g, p, arg, ok, input, choice); break;
    }
}

static void modal_key(UI *u, Game *g, int k) {
    switch (u->modal) {
    case MD_PROMPT:
        if (k == K_ESC) { modal_result(u, g, 0, "", 0); return; }
        if (k == K_ENTER) { char in[64]; snprintf(in, sizeof in, "%s", u->minput); modal_result(u, g, 1, in, 0); return; }
        if (k == K_BS) { utf8_pop(u->minput); return; }
        if (u->mnumeric) {
            if (k >= '0' && k <= '9' && (int)strlen(u->minput) < u->mmaxlen) {
                if (u->minput[0] == 'M' || (u->minput[0] == '0' && u->minput[1] == 0)) u->minput[0] = 0;
                utf8_append(u->minput, sizeof u->minput, (uint32_t)k);
            } else if (k == 'm' || k == 'M') snprintf(u->minput, sizeof u->minput, "M");
            return;
        }
        if (k >= 32 && k < 0x110000 && utf8_len(u->minput) < u->mmaxlen) utf8_append(u->minput, sizeof u->minput, (uint32_t)k);
        return;
    case MD_MENU:
        if (k == K_ESC) { modal_result(u, g, 0, "", -1); return; }
        if (k == K_UP) { for (int i = 0; i < u->mn; i++) { u->msel = (u->msel + u->mn - 1) % u->mn; if (u->mok[u->msel]) break; } return; }
        if (k == K_DOWN) { for (int i = 0; i < u->mn; i++) { u->msel = (u->msel + 1) % u->mn; if (u->mok[u->msel]) break; } return; }
        if (k >= '1' && k <= '9' && k - '1' < u->mn) { u->msel = k - '1'; k = K_ENTER; }
        if (k == K_ENTER) {
            if (!u->mok[u->msel]) { ui_toast(u, C_RED, "Opção indisponível"); return; }
            modal_result(u, g, 1, "", u->msel);
        }
        return;
    case MD_INFO: case MD_AWAY: u->modal = MD_NONE; return;
    case MD_CONFIRM:
        if (k == 's' || k == 'S' || k == 'y' || k == 'Y' || k == K_ENTER) modal_result(u, g, 1, "", 0);
        else if (k == 'n' || k == 'N' || k == K_ESC) modal_result(u, g, 0, "", 0);
        return;
    case MD_EVENT: {
        Decision *d = &g->ev.dec;
        if (!d->active) { u->modal = MD_NONE; return; }
        if (d->stage == 0) {
            if (k == K_ESC) { u->modal = MD_NONE; return; }
            if (k >= '1' && k <= '0' + d->nopt) {
                int o = k - '1';
                const Body *b = &g->body[d->body];
                int ok = 1;
                for (int r = 0; r < NRES; r++) if (b->res[r] + 1e-6 < d->optcost[o][r]) ok = 0;
                if (d->type == EV_METEOR && o == 0 && b->def[DI(D_ABM)] <= 0) ok = 0;
                if (!ok) { ui_toast(u, C_RED, "Opção indisponível"); return; }
                decision_choose(g, o);
                u->dice_t0 = u->t;
            }
            return;
        }
        if (u->t - u->dice_t0 < 1.3 && d->optbonus[d->chosen] != -1000) return;
        if ((k == 'r' || k == 'R') && d->rerolls < 2 && g->dm >= 300) { decision_reroll(g); u->dice_t0 = u->t; return; }
        if (k == K_ENTER || k == ' ') {
            decision_close(g);
            u->modal = MD_NONE;
            ui_toast(u, C_GOLD, "Evento resolvido - veja os detalhes em Mensagens");
        }
        return;
    }
    }
}

/* ===================== DISPATCH ===================== */
void ui_goto_screen(UI *u, Game *g, int scr) {
    (void)g;
    if (scr >= 0 && scr < SCR_COUNT) u->screen = scr;
}

static void switch_body(UI *u, Game *g, int dir) {
    int order[MAX_BODIES], n = 0;
    for (int i = 0; i < g->nbody; i++) {
        if (!g->body[i].used || g->body[i].is_moon) continue;
        order[n++] = i;
        if (g->body[i].moon >= 0) order[n++] = g->body[i].moon;
    }
    int cur = 0;
    for (int i = 0; i < n; i++) if (order[i] == g->cur) cur = i;
    g->cur = order[(cur + dir + n) % n];
    u->gg = g->body[g->cur].g;
    u->gs = g->body[g->cur].s;
}

void ui_key(UI *u, Game *g, int k) {
    if (k == 0) return;
    if (k == K_CLOSE) { u->quit = 1; return; }
    if (u->modal != MD_NONE) { modal_key(u, g, k); return; }
    int used = 0;
    switch (u->screen) {
    case SCR_OVERVIEW: used = overview_key(u, g, k); break;
    case SCR_RESOURCES: case SCR_FACILITIES: case SCR_LIFE: used = building_key(u, g, u->screen, k); break;
    case SCR_RESEARCH: used = research_key(u, g, k); break;
    case SCR_SHIPYARD: case SCR_DEFENSE: used = yard_key(u, g, u->screen, k); break;
    case SCR_FLEET: used = scr_fleet_key(u, g, k); break;
    case SCR_GALAXY: used = scr_galaxy_key(u, g, k); break;
    case SCR_EMPIRE: used = scr_empire_key(u, g, k); break;
    case SCR_TRADE: used = scr_trade_key(u, g, k); break;
    case SCR_MERCHANT: used = scr_merchant_key(u, g, k); break;
    case SCR_MESSAGES: used = scr_messages_key(u, g, k); break;
    case SCR_TECH: used = scr_tech_key(u, g, k); break;
    case SCR_OFFICERS: used = scr_officers_key(u, g, k); break;
    case SCR_RANKING: used = scr_ranking_key(u, g, k); break;
    case SCR_SIM: used = scr_sim_key(u, g, k); break;
    case SCR_HELP: used = scr_help_key(u, g, k); break;
    }
    if (used) return;
    int K = (k >= 'a' && k <= 'z') ? k - 32 : k;
    int scr = menu_screen_for_key(K);
    if (scr >= 0) { u->screen = scr; return; }
    switch (K) {
    case 'Q': case K_CTRL_C: ui_confirm(u, MP_QUIT, 0, "Sair", "Salvar o império e sair do jogo?\nA produção continua enquanto você estiver fora."); return;
    case K_ESC:
        if (u->screen != SCR_OVERVIEW) u->screen = SCR_OVERVIEW;
        else ui_confirm(u, MP_QUIT, 0, "Sair", "Salvar o império e sair do jogo?");
        return;
    case 'L': {
        Body *b = &g->body[g->cur];
        if (b->is_moon) g->cur = b->parent;
        else if (b->moon >= 0) g->cur = b->moon;
        else ui_toast(u, C_DIM, "Este planeta não possui lua.");
        return;
    }
    case '[': case ',': case K_PGUP: switch_body(u, g, -1); return;
    case ']': case '.': case K_PGDN: switch_body(u, g, 1); return;
    case 'V': if (g->ev.dec.active) u->modal = MD_EVENT; return;
    case K_TAB: switch_body(u, g, 1); return;
    }
}

void ui_draw(UI *u, Game *g) {
    if (g->cur < 0 || g->cur >= g->nbody || !g->body[g->cur].used) g->cur = 0;
    /* pull toasts from game */
    if (g->ntoast > 0 && u->t > u->toast_until - 2.6) {
        ui_toast(u, g->toast_col[0], "%s", g->toast[0]);
        memmove(g->toast[0], g->toast[1], sizeof(g->toast[0]) * 7);
        memmove(&g->toast_col[0], &g->toast_col[1], sizeof(g->toast_col[0]) * 7);
        g->ntoast--;
    }
    if (g->ev.dec.active && u->modal == MD_NONE && g->ev.dec.stage == 0 && g->ev.dec.chosen == 0 && g->ev.dec.rerolls == 0) {
        static double last_open = -1;
        static int last_type = -1;
        static double last_deadline = -1;
        if (last_deadline != g->ev.dec.deadline || last_type != g->ev.dec.type) {
            u->modal = MD_EVENT;
            last_deadline = g->ev.dec.deadline;
            last_type = g->ev.dec.type;
            last_open = u->t;
        }
        (void)last_open;
    }
    cv_clear(C_BG);
    draw_header(u, g);
    draw_resbar(u, g);
    draw_left(u, g);
    draw_right(u, g);
    draw_fleetbar(u, g);
    cv_set_clip(CX, CY, CW, CANVAS_H - CY);
    switch (u->screen) {
    case SCR_OVERVIEW: draw_overview(u, g); break;
    case SCR_RESOURCES: case SCR_FACILITIES: case SCR_LIFE: draw_building_screen(u, g, u->screen); break;
    case SCR_RESEARCH: draw_research(u, g); break;
    case SCR_SHIPYARD: case SCR_DEFENSE: draw_yard(u, g, u->screen); break;
    case SCR_FLEET: scr_fleet_draw(u, g); break;
    case SCR_GALAXY: scr_galaxy_draw(u, g); break;
    case SCR_EMPIRE: scr_empire_draw(u, g); break;
    case SCR_TRADE: scr_trade_draw(u, g); break;
    case SCR_MERCHANT: scr_merchant_draw(u, g); break;
    case SCR_MESSAGES: scr_messages_draw(u, g); break;
    case SCR_TECH: scr_tech_draw(u, g); break;
    case SCR_OFFICERS: scr_officers_draw(u, g); break;
    case SCR_RANKING: scr_ranking_draw(u, g); break;
    case SCR_SIM: scr_sim_draw(u, g); break;
    case SCR_HELP: scr_help_draw(u, g); break;
    }
    cv_reset_clip();
    /* toast */
    if (u->t < u->toast_until && u->toast[0]) {
        int n = utf8_len(u->toast);
        int w = n + 4 > CW - 4 ? CW - 4 : n + 4;
        int x = CX + CW - w - 1, y = CY + 1;
        cv_fill(x, y, w, 1, ' ', C_TEXT, RGB(10, 14, 24));
        cv_put(x, y, 0x258C, u->toast_col, RGB(10, 14, 24));
        cv_text_w(x + 2, y, w - 3, u->toast, u->toast_col, RGB(10, 14, 24));
    }
    if (u->modal != MD_NONE) draw_modal(u, g);
}
