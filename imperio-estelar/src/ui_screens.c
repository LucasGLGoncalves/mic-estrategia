/* ui_screens.c - fleet, galaxy, empire, trade, merchant, messages, tech, officers, ranking, simulator, help */
#include "ui.h"
#include "art.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void title_line(const char *t, const char *right) {
    cv_fill(CX, CY, CW, 1, ' ', C_TEXT, C_BG);
    cv_text(CX + 1, CY, t, C_WHITE, C_BG);
    cv_bold_rect(CX + 1, CY, utf8_len(t), 1);
    if (right) cv_text_r(CX + CW - 2, CY, right, C_DIM, C_BG);
}

static void hint(const char *s) {
    cv_fill(CX, CANVAS_H - 1, CW, 1, ' ', C_TEXT, C_BG);
    cv_text_w(CX + 1, CANVAS_H - 1, CW - 2, s, C_DIM, C_BG);
}

static int num_append(char *buf, size_t n, int k) {
    if (k >= '0' && k <= '9' && strlen(buf) < n - 1 && strlen(buf) < 13) {
        if (buf[0] == '0' && buf[1] == 0) buf[0] = 0;
        size_t L = strlen(buf);
        buf[L] = (char)k;
        buf[L + 1] = 0;
        return 1;
    }
    if (k == K_BS) { size_t L = strlen(buf); if (L) buf[L - 1] = 0; return 1; }
    return 0;
}

/* ===================== FLEET ===================== */
static int flyable(int i) { return T_END + i != S_SATELLITE && T_END + i != S_CRAWLER; }

void dispatch_open(UI *u, Game *g, int tg, int ts, int tp, int ttype, int mission) {
    Dispatch *d = &u->dp;
    memset(d, 0, sizeof *d);
    d->active = 1;
    d->step = 0;
    d->origin = g->cur;
    d->tg = tg; d->ts = ts; d->tp = tp; d->ttype = ttype;
    d->mission = mission ? mission : M_TRANSPORT;
    d->speed = 100;
    u->screen = SCR_FLEET;
    Body *b = &g->body[g->cur];
    /* sensible preselection */
    if (mission == M_ESPIONAGE) { long long p = b->ship[SI(S_PROBE)]; d->sel[SI(S_PROBE)] = p < 3 ? p : 3; }
    else if (mission == M_EXPLORE) {
        if (b->ship[SI(S_PATHFINDER)] > 0) d->sel[SI(S_PATHFINDER)] = 1;
        else d->sel[SI(S_PROBE)] = b->ship[SI(S_PROBE)] > 0 ? 1 : 0;
    } else if (mission == M_COLONIZE) d->sel[SI(S_COLONY)] = b->ship[SI(S_COLONY)] > 0 ? 1 : 0;
    else if (mission == M_RECYCLE) d->sel[SI(S_RECYCLER)] = b->ship[SI(S_RECYCLER)];
    int first = 0;
    for (int i = 0; i < N_SHIP; i++) if (flyable(i) && b->ship[i] > 0) { first = i; break; }
    d->row = first;
}

static void draw_dispatch(UI *u, Game *g) {
    Dispatch *d = &u->dp;
    Body *b = &g->body[d->origin];
    char t[220];
    snprintf(t, sizeof t, " Enviar frota de %s [%d:%d:%d] - passo %d/3 ", b->name, b->g, b->s, b->p, d->step + 1);
    title_line(t, NULL);
    double dist = fleet_distance(b->g, b->s, b->p, d->tg, d->ts, d->tp);
    double slow = fleet_slowest(g, d->sel);
    double dur = slow > 0 ? flight_duration(g, dist, slow, d->speed) : 0;
    double fuel = slow > 0 ? fleet_fuel(g, d->sel, dist, d->speed, b) : 0;
    if (d->mission == M_EXPEDITION) fuel = floor(fuel * 1.5);
    double cap = fleet_cargo_cap(g, d->sel);
    double used = d->cargo[0] + d->cargo[1] + d->cargo[2] + d->cargo[3];
    if (d->step == 0) {
        ui_panel(CX, CY + 1, 52, CH - 2, "1. Escolha as naves");
        int y = CY + 2;
        cv_text(CX + 3, y, "Nave", C_TITLE, C_PANEL);
        cv_text(CX + 30, y, "Disponível", C_TITLE, C_PANEL);
        cv_text(CX + 42, y, "Enviar", C_TITLE, C_PANEL);
        y++;
        for (int i = 0; i < N_SHIP; i++) {
            if (!flyable(i)) continue;
            int sel = d->row == i;
            uint32_t bg = sel ? C_SEL : C_PANEL;
            cv_fill(CX + 1, y, 50, 1, ' ', C_TEXT, bg);
            if (sel) cv_put(CX + 1, y, 0x25B6, C_GOLD, bg);
            cv_text(CX + 3, y, ITEMS[T_END + i].name, b->ship[i] > 0 ? C_TEXT : C_DIM, bg);
            char a[24];
            fmt_num(a, sizeof a, (double)b->ship[i]);
            cv_text_r(CX + 39, y, a, C_DIM, bg);
            if (sel && d->num[0]) cv_text_r(CX + 48, y, d->num, C_GOLD, bg);
            else { fmt_num(a, sizeof a, (double)d->sel[i]); cv_text_r(CX + 48, y, a, d->sel[i] > 0 ? C_GREEN : C_DIM, bg); }
            y++;
        }
    } else if (d->step == 1) {
        ui_panel(CX, CY + 1, 52, CH - 2, "2. Destino");
        static const char *FL[4] = {"Galáxia", "Sistema", "Posição", "Tipo"};
        int vals[3] = {d->tg, d->ts, d->tp};
        for (int f = 0; f < 4; f++) {
            int y = CY + 3 + f * 2;
            uint32_t bg = d->field == f ? C_SEL : C_PANEL;
            cv_fill(CX + 2, y, 46, 1, ' ', C_TEXT, bg);
            cv_text(CX + 4, y, FL[f], C_TEXT, bg);
            if (f < 3) {
                char v[40];
                if (d->field == f && d->num[0]) snprintf(v, sizeof v, "%s_", d->num);
                else snprintf(v, sizeof v, "%d", vals[f]);
                cv_text(CX + 20, y, v, C_GOLD, bg);
            } else cv_text(CX + 20, y, d->ttype == TT_PLANET ? "Planeta" : d->ttype == TT_MOON ? "Lua" : "Campo de destroços", C_GOLD, bg);
        }
        int y = CY + 12;
        int npi = d->tp <= 15 ? npc_planet_at(g, d->tg, d->ts, d->tp) : -1;
        int own = d->tp <= 15 ? player_body_at(g, d->tg, d->ts, d->tp, d->ttype == TT_MOON) : -1;
        if (d->tp == 16) cv_text(CX + 3, y, "Espaço profundo: destino de expedições.", C_CYAN, C_PANEL);
        else if (own >= 0) cv_textf(CX + 3, y, C_CYAN, C_PANEL, "Seu: %s", g->body[own].name);
        else if (npi >= 0) {
            const Npc *n = &g->npc[g->npl[npi].npc];
            cv_textf(CX + 3, y, C_ORANGE, C_PANEL, "%s de %s (%s)", g->npl[npi].pname, n->name, npc_type_name(n->type));
            char p[24];
            fmt_num(p, sizeof p, npc_points(g, g->npl[npi].npc));
            cv_textf(CX + 3, y + 1, C_DIM, C_PANEL, "Pontos: %s", p);
        } else cv_text(CX + 3, y, is_explored(g, d->tg, d->ts, d->tp) ? "Posição vazia (explorada)" : "Posição vazia", C_DIM, C_PANEL);
        Debris *db = debris_at(g, d->tg, d->ts, d->tp, 0);
        if (db && db->m + db->c > 0) {
            char a[24], c[24];
            fmt_short(a, sizeof a, db->m); fmt_short(c, sizeof c, db->c);
            cv_textf(CX + 3, y + 2, C_GOLD, C_PANEL, "Destroços: %s M  %s C", a, c);
        }
        cv_text(CX + 3, CY + CH - 6, "P: alternar entre seus planetas", C_DIM, C_PANEL);
        cv_text(CX + 3, CY + CH - 5, "\u2190\u2192 campo  \u2191\u2193 +/-  dígitos digitam", C_DIM, C_PANEL);
    } else {
        ui_panel(CX, CY + 1, 52, CH - 2, "3. Missão e carga");
        static const char *ROWS[7] = {"Missão", "Velocidade", "Metal", "Cristal", "Deutério", "Alimento", ">> LANÇAR FROTA <<"};
        for (int r = 0; r < 7; r++) {
            int y = CY + 3 + r * 2;
            uint32_t bg = d->frow == r ? C_SEL : C_PANEL;
            cv_fill(CX + 2, y, 46, 1, ' ', C_TEXT, bg);
            cv_text(CX + 4, y, ROWS[r], r == 6 ? C_GOLD : C_TEXT, bg);
            char v[48] = "";
            if (r == 0) snprintf(v, sizeof v, "\u25C4 %s \u25BA", mission_name(d->mission));
            else if (r == 1) snprintf(v, sizeof v, "\u25C4 %d%% \u25BA", d->speed);
            else if (r < 6) {
                if (d->frow == r && d->num[0]) snprintf(v, sizeof v, "%s_", d->num);
                else fmt_num(v, sizeof v, d->cargo[r - 2]);
            }
            cv_text(CX + 20, y, v, C_GOLD, bg);
            if (r >= 2 && r < 6) {
                char a[24];
                fmt_short(a, sizeof a, b->res[r - 2]);
                cv_textf(CX + 36, y, C_DIM, bg, "/ %s", a);
            }
        }
        cv_text(CX + 3, CY + CH - 5, "A = máximo do recurso selecionado", C_DIM, C_PANEL);
    }
    /* summary */
    int sx = CX + 53, sw = CW - 53;
    ui_panel(sx, CY + 1, sw, CH - 2, "Resumo");
    int y = CY + 3;
    char a[32];
    fmt_num(a, sizeof a, fleet_ship_count(d->sel));
    cv_textf(sx + 2, y++, C_TEXT, C_PANEL, "Naves: %s", a);
    cv_textf(sx + 2, y++, C_TEXT, C_PANEL, "Destino: [%d:%d:%d]", d->tg, d->ts, d->tp);
    fmt_num(a, sizeof a, dist);
    cv_textf(sx + 2, y++, C_TEXT, C_PANEL, "Distância: %s", a);
    fmt_num(a, sizeof a, slow);
    cv_textf(sx + 2, y++, C_TEXT, C_PANEL, "Velocidade: %s", a);
    char tm[32];
    fmt_time(tm, sizeof tm, dur);
    cv_textf(sx + 2, y++, C_CYAN, C_PANEL, "Duração: %s", tm);
    fmt_clock(tm, sizeof tm, g->now + dur);
    cv_textf(sx + 2, y++, C_TEXT, C_PANEL, "Chegada: %s", tm);
    fmt_clock(tm, sizeof tm, g->now + dur * 2 + (d->mission == M_EXPEDITION ? 3600.0 / g->speed : 0));
    cv_textf(sx + 2, y++, C_TEXT, C_PANEL, "Retorno: %s", tm);
    fmt_num(a, sizeof a, fuel);
    cv_textf(sx + 2, y++, b->res[RES_DEUT] >= fuel + d->cargo[2] ? C_TEXT : C_RED, C_PANEL, "Combustível: %s", a);
    char c2[32];
    fmt_num(a, sizeof a, used);
    fmt_num(c2, sizeof c2, cap);
    cv_textf(sx + 2, y++, used <= cap ? C_TEXT : C_RED, C_PANEL, "Carga: %s/%s", a, c2);
    ui_bar(sx + 2, y++, sw - 4, cap > 0 ? used / cap : 0, used <= cap ? C_GREEN : C_RED);
    y++;
    cv_textf(sx + 2, y++, C_DIM, C_PANEL, "Slots: %d/%d", fleet_slots_used(g), fleet_slots_max(g));
    cv_textf(sx + 2, y++, C_DIM, C_PANEL, "Expedições: %d/%d", exped_slots_used(g), exped_slots_max(g));
    if (d->step == 2) {
        char why[160];
        int ok = mission_valid(g, d->origin, d->mission, d->tg, d->ts, d->tp, d->ttype, d->sel, why, sizeof why);
        y++;
        cv_wrap(sx + 2, y, sw - 4, 4, ok ? "Missão válida." : why, ok ? C_GREEN : C_RED, C_PANEL);
    }
    if (d->err[0]) cv_wrap(sx + 2, CY + CH - 6, sw - 4, 3, d->err, C_RED, C_PANEL);
    hint(d->step == 0 ? "\u2191\u2193 nave  dígitos quantidade  A tudo desta  T todas  N nenhuma  Enter próximo  Esc cancelar"
                      : d->step == 1 ? "\u2190\u2192 campo  \u2191\u2193 ajustar  dígitos  P meus planetas  Enter próximo  Esc voltar"
                                     : "\u2191\u2193 linha  \u2190\u2192 missão/velocidade  dígitos carga  A máximo  Enter na última linha lança");
}

static void commit_num(Dispatch *d, int step) {
    if (!d->num[0]) return;
    long long v = atoll(d->num);
    if (step == 1) {
        if (d->field == 0) d->tg = clampi((int)v, 1, UNI_GALAXIES);
        else if (d->field == 1) d->ts = clampi((int)v, 1, UNI_SYSTEMS);
        else if (d->field == 2) d->tp = clampi((int)v, 1, 16);
    }
    d->num[0] = 0;
}

static int dispatch_key(UI *u, Game *g, int k) {
    Dispatch *d = &u->dp;
    Body *b = &g->body[d->origin];
    d->err[0] = 0;
    if (d->step == 0) {
        if (k == K_ESC) { d->active = 0; return 1; }
        int prev = d->row;
        if (k == K_UP || k == K_DOWN) {
            if (d->num[0]) { long long v = atoll(d->num); d->sel[d->row] = v > b->ship[d->row] ? b->ship[d->row] : v; d->num[0] = 0; }
            for (int i = 0; i < N_SHIP; i++) {
                d->row = (d->row + (k == K_UP ? N_SHIP - 1 : 1)) % N_SHIP;
                if (flyable(d->row)) break;
            }
            (void)prev;
            return 1;
        }
        if (num_append(d->num, sizeof d->num, k)) {
            long long v = atoll(d->num);
            d->sel[d->row] = v > b->ship[d->row] ? b->ship[d->row] : v;
            return 1;
        }
        if (k == K_RIGHT) { if (d->sel[d->row] < b->ship[d->row]) d->sel[d->row]++; d->num[0] = 0; return 1; }
        if (k == K_LEFT) { if (d->sel[d->row] > 0) d->sel[d->row]--; d->num[0] = 0; return 1; }
        if (k == 'a' || k == 'A') { d->sel[d->row] = b->ship[d->row]; d->num[0] = 0; return 1; }
        if (k == 't' || k == 'T') { for (int i = 0; i < N_SHIP; i++) if (flyable(i)) d->sel[i] = b->ship[i]; d->num[0] = 0; return 1; }
        if (k == 'n' || k == 'N') { memset(d->sel, 0, sizeof d->sel); d->num[0] = 0; return 1; }
        if (k == K_ENTER) {
            d->num[0] = 0;
            if (fleet_ship_count(d->sel) <= 0) { snprintf(d->err, sizeof d->err, "Selecione ao menos uma nave."); return 1; }
            d->step = 1;
            d->field = 2;
            return 1;
        }
        return 1;
    }
    if (d->step == 1) {
        if (k == K_ESC) { d->num[0] = 0; d->step = 0; return 1; }
        if (k == K_LEFT) { commit_num(d, 1); d->field = (d->field + 3) % 4; return 1; }
        if (k == K_RIGHT || k == K_TAB) { commit_num(d, 1); d->field = (d->field + 1) % 4; return 1; }
        if (k == K_UP || k == K_DOWN) {
            commit_num(d, 1);
            int dv = k == K_UP ? 1 : -1;
            if (d->field == 0) d->tg = (d->tg - 1 + dv + UNI_GALAXIES) % UNI_GALAXIES + 1;
            else if (d->field == 1) d->ts = (d->ts - 1 + dv + UNI_SYSTEMS) % UNI_SYSTEMS + 1;
            else if (d->field == 2) d->tp = (d->tp - 1 + dv + 16) % 16 + 1;
            else d->ttype = (d->ttype + (dv > 0 ? 1 : 2)) % 3;
            return 1;
        }
        if (d->field < 3 && num_append(d->num, sizeof d->num, k)) return 1;
        if (k == 'p' || k == 'P') {
            commit_num(d, 1);
            int start = -1;
            for (int i = 0; i < g->nbody; i++) if (g->body[i].used && g->body[i].g == d->tg && g->body[i].s == d->ts && g->body[i].p == d->tp && g->body[i].is_moon == (d->ttype == TT_MOON)) start = i;
            for (int k2 = 1; k2 <= g->nbody; k2++) {
                int i = (start + k2 + g->nbody) % g->nbody;
                if (!g->body[i].used || i == d->origin) continue;
                d->tg = g->body[i].g; d->ts = g->body[i].s; d->tp = g->body[i].p; d->ttype = g->body[i].is_moon ? TT_MOON : TT_PLANET;
                break;
            }
            return 1;
        }
        if (k == K_ENTER) {
            commit_num(d, 1);
            d->step = 2;
            d->frow = 0;
            /* auto choose a valid mission */
            char why[8];
            if (!mission_valid(g, d->origin, d->mission, d->tg, d->ts, d->tp, d->ttype, d->sel, why, sizeof why)) {
                static const int ORDER[] = {M_EXPEDITION, M_COLONIZE, M_RECYCLE, M_ESPIONAGE, M_ATTACK, M_TRANSPORT, M_DEPLOY, M_EXPLORE};
                for (int i = 0; i < 8; i++)
                    if (mission_valid(g, d->origin, ORDER[i], d->tg, d->ts, d->tp, d->ttype, d->sel, why, sizeof why)) { d->mission = ORDER[i]; break; }
            }
            return 1;
        }
        return 1;
    }
    /* step 2 */
    if (k == K_ESC) { d->num[0] = 0; d->step = 1; return 1; }
    if (k == K_UP || k == K_DOWN) {
        if (d->num[0] && d->frow >= 2 && d->frow < 6) { d->cargo[d->frow - 2] = (double)atoll(d->num); d->num[0] = 0; }
        d->frow = (d->frow + (k == K_UP ? 6 : 1)) % 7;
        return 1;
    }
    if (d->frow == 0 && (k == K_LEFT || k == K_RIGHT)) {
        int m = d->mission;
        for (int i = 0; i < M_HOSTILE; i++) {
            m = k == K_RIGHT ? m + 1 : m - 1;
            if (m >= M_HOSTILE) m = M_ATTACK;
            if (m < M_ATTACK) m = M_HOSTILE - 1;
            char why[8];
            if (mission_valid(g, d->origin, m, d->tg, d->ts, d->tp, d->ttype, d->sel, why, sizeof why)) break;
        }
        d->mission = m;
        return 1;
    }
    if (d->frow == 1 && (k == K_LEFT || k == K_RIGHT)) { d->speed = clampi(d->speed + (k == K_RIGHT ? 10 : -10), 10, 100); return 1; }
    if (d->frow >= 2 && d->frow < 6) {
        int r = d->frow - 2;
        if (num_append(d->num, sizeof d->num, k)) {
            double v = (double)atoll(d->num);
            double cap = fleet_cargo_cap(g, d->sel), others = 0;
            for (int i = 0; i < NRES; i++) if (i != r) others += d->cargo[i];
            if (v > b->res[r]) v = floor(b->res[r]);
            if (v > cap - others) v = floor(fmax(0, cap - others));
            d->cargo[r] = v;
            if (v < (double)atoll(d->num)) snprintf(d->num, sizeof d->num, "%.0f", v);
            return 1;
        }
        if (k == 'a' || k == 'A') {
            double cap = fleet_cargo_cap(g, d->sel);
            double others = 0;
            for (int i = 0; i < NRES; i++) if (i != r) others += d->cargo[i];
            double room = cap - others;
            double avail = b->res[r];
            if (r == RES_DEUT) {
                double dist = fleet_distance(b->g, b->s, b->p, d->tg, d->ts, d->tp);
                avail -= fleet_fuel(g, d->sel, dist, d->speed, b) * (d->mission == M_EXPEDITION ? 1.5 : 1.0);
            }
            d->cargo[r] = floor(fmax(0, fmin(room, avail)));
            d->num[0] = 0;
            return 1;
        }
    }
    if (k == K_ENTER) {
        if (d->num[0] && d->frow >= 2 && d->frow < 6) { d->cargo[d->frow - 2] = (double)atoll(d->num); d->num[0] = 0; }
        if (d->frow != 6) { d->frow++; return 1; }
        char err[160];
        if (fleet_launch(g, d->origin, d->mission, d->tg, d->ts, d->tp, d->ttype, d->sel, d->cargo, d->speed, err, sizeof err)) {
            ui_toast(u, C_GREEN, "Frota enviada: %s [%d:%d:%d]", mission_name(d->mission), d->tg, d->ts, d->tp);
            d->active = 0;
        } else snprintf(d->err, sizeof d->err, "%s", err);
        return 1;
    }
    return 1;
}

void scr_fleet_draw(UI *u, Game *g) {
    if (u->dp.active) { draw_dispatch(u, g); return; }
    Body *b = &g->body[g->cur];
    char t[160], r[80];
    snprintf(t, sizeof t, " Frota - %s ", b->name);
    snprintf(r, sizeof r, "Slots %d/%d  Expedições %d/%d", fleet_slots_used(g), fleet_slots_max(g), exped_slots_used(g), exped_slots_max(g));
    title_line(t, r);
    int focus = u->tab[SCR_FLEET];
    ui_panel(CX, CY + 1, 44, 19, "Naves estacionadas");
    int y = CY + 2;
    for (int i = 0; i < N_SHIP; i++) {
        char a[24];
        fmt_num(a, sizeof a, (double)b->ship[i]);
        cv_text(CX + 2, y, ITEMS[T_END + i].name, b->ship[i] > 0 ? C_TEXT : C_DIM, C_PANEL);
        cv_text_r(CX + 41, y, a, b->ship[i] > 0 ? C_GOLD : C_DIM, C_PANEL);
        y++;
    }
    ui_panel(CX + 44, CY + 1, CW - 44, 19, "Poder da frota");
    long long fl[N_SHIP];
    memcpy(fl, b->ship, sizeof fl);
    fl[SI(S_SATELLITE)] = 0; fl[SI(S_CRAWLER)] = 0;
    char a[32];
    y = CY + 3;
    fmt_num(a, sizeof a, fleet_ship_count(fl));
    cv_textf(CX + 46, y++, C_TEXT, C_PANEL, "Naves que voam: %s", a);
    fmt_num(a, sizeof a, fleet_cargo_cap(g, fl));
    cv_textf(CX + 46, y++, C_TEXT, C_PANEL, "Capacidade de carga: %s", a);
    fmt_num(a, sizeof a, body_military_value(b));
    cv_textf(CX + 46, y++, C_TEXT, C_PANEL, "Valor militar: %s", a);
    fmt_num(a, sizeof a, b->res[RES_DEUT]);
    cv_textf(CX + 46, y++, C_TEXT, C_PANEL, "Deutério: %s", a);
    y++;
    cv_textf(CX + 46, y++, C_DIM, C_PANEL, "Armas +%d%% Escudos +%d%%", g->tech[TI(T_WEAPONS)] * 10, g->tech[TI(T_SHIELDING)] * 10);
    cv_textf(CX + 46, y++, C_DIM, C_PANEL, "Blindagem +%d%%", g->tech[TI(T_ARMOR)] * 10);
    cv_textf(CX + 46, y++, C_DIM, C_PANEL, "Combustão %d Impulso %d Hiper %d", g->tech[TI(T_COMBUSTION)], g->tech[TI(T_IMPULSE)], g->tech[TI(T_HYPERDRIVE)]);
    y++;
    cv_wrap(CX + 46, y, CW - 48, 5, "Dica: frotas em voo não podem ser atacadas. Quando uma frota hostil se aproximar, envie suas naves em missão (fleetsave) ou fortaleça as defesas.", C_DIM, C_PANEL);
    /* movements */
    int my = CY + 20;
    ui_panel(CX, my, CW, CANVAS_H - 1 - my, focus ? "Movimentos [foco]" : "Movimentos");
    int idx[MAX_FLEETS], n = 0;
    for (int i = 0; i < MAX_FLEETS; i++) if (g->fleet[i].used) idx[n++] = i;
    if (n == 0) cv_text(CX + 3, my + 2, "Nenhuma frota em movimento.", C_DIM, C_PANEL);
    int sel = clampi(u->sel[SCR_FLEET], 0, n > 0 ? n - 1 : 0);
    u->sel[SCR_FLEET] = sel;
    int maxr = CANVAS_H - 3 - my;
    int top = sel >= maxr ? sel - maxr + 1 : 0;
    for (int k = 0; k < n && k - top < maxr; k++) {
        if (k < top) continue;
        Fleet *f = &g->fleet[idx[k]];
        int yy = my + 1 + k - top;
        int hostile = f->mission == M_HOSTILE || f->mission == M_HOSTILE_SPY;
        uint32_t bg = (focus && k == sel) ? C_SEL : C_PANEL;
        cv_fill(CX + 1, yy, CW - 2, 1, ' ', C_TEXT, bg);
        double tt = f->state == FS_OUT ? f->arrive : f->state == FS_HOLD ? f->hold : f->ret;
        char tm[32], cnt[24];
        fmt_time(tm, sizeof tm, tt - g->now);
        fmt_short(cnt, sizeof cnt, fleet_ship_count(f->ship));
        if (hostile) snprintf(t, sizeof t, "\u26A0 %-14s [%d:%d:%d] -> %s  chega em %s  (%s naves)", mission_name(f->mission), f->og, f->os, f->op, g->body[f->origin].name, tm, cnt);
        else snprintf(t, sizeof t, "%-12s [%d:%d:%d] -> [%d:%d:%d]  %s %s  (%s naves)", mission_name(f->mission), f->og, f->os, f->op, f->tg, f->ts, f->tp,
                      f->state == FS_OUT ? "chega em" : f->state == FS_HOLD ? "explora até" : "retorna em", tm, cnt);
        cv_text_w(CX + 2, yy, CW - 4, t, hostile ? C_RED : f->state == FS_RETURN ? C_DIM : C_CYAN, bg);
    }
    hint(focus ? "\u2191\u2193 selecionar movimento  B chamar de volta  Tab naves  Enter enviar frota"
               : b->is_moon && b->bld[B_JUMP_GATE] ? "Enter enviar frota  J Portal de Salto  Tab movimentos (B chamar de volta)"
                                                   : "Enter enviar frota  Tab focar movimentos (B chamar de volta)  9 Galáxia para escolher alvos");
}

int scr_fleet_key(UI *u, Game *g, int k) {
    if (u->dp.active) return dispatch_key(u, g, k);
    if (k == K_ENTER) {
        /* default target: another own body if any, else the galaxy selection */
        int other = -1;
        for (int i = 0; i < g->nbody; i++) if (g->body[i].used && i != g->cur) { other = i; break; }
        if (other >= 0) dispatch_open(u, g, g->body[other].g, g->body[other].s, g->body[other].p, g->body[other].is_moon ? TT_MOON : TT_PLANET, M_TRANSPORT);
        else dispatch_open(u, g, u->gg, u->gs, u->sel[SCR_GALAXY] + 1, TT_PLANET, 0);
        return 1;
    }
    if (k == K_TAB) { u->tab[SCR_FLEET] ^= 1; return 1; }
    if (k == 'j' || k == 'J') {
        Body *b = &g->body[g->cur];
        if (!b->is_moon || b->bld[B_JUMP_GATE] < 1) { ui_toast(u, C_RED, "Portal de Salto: selecione uma lua com o portal construído."); return 1; }
        const char *items[12];
        static char lab[12][80];
        int n = 0;
        for (int i = 0; i < g->nbody && n < 12; i++)
            if (g->body[i].used && g->body[i].is_moon && i != g->cur && g->body[i].bld[B_JUMP_GATE] > 0) {
                body_label(g, i, lab[n], sizeof lab[n]);
                items[n] = lab[n];
                n++;
            }
        if (!n) { ui_toast(u, C_RED, "Nenhuma outra lua com Portal de Salto."); return 1; }
        ui_menu(u, MP_JUMP, 0, "Saltar toda a frota para...", n, items, NULL);
        return 1;
    }
    if (u->tab[SCR_FLEET]) {
        int idx[MAX_FLEETS], n = 0;
        for (int i = 0; i < MAX_FLEETS; i++) if (g->fleet[i].used) idx[n++] = i;
        if (k == K_UP) { u->sel[SCR_FLEET] = n ? (u->sel[SCR_FLEET] + n - 1) % n : 0; return 1; }
        if (k == K_DOWN) { u->sel[SCR_FLEET] = n ? (u->sel[SCR_FLEET] + 1) % n : 0; return 1; }
        if ((k == 'b' || k == 'B') && n > 0) {
            int fi = idx[clampi(u->sel[SCR_FLEET], 0, n - 1)];
            if (fleet_recall(g, fi)) ui_toast(u, C_CYAN, "Frota chamada de volta.");
            else ui_toast(u, C_RED, "Esta frota não pode ser chamada de volta.");
            return 1;
        }
    }
    return 0;
}

/* ===================== GALAXY ===================== */
static const char *npc_flag(int t) {
    switch (t) {
    case NPC_INACTIVE: return "(i)";
    case NPC_TURTLE: return "(d)";
    case NPC_FLEETER: return "(A)";
    case NPC_TRADER: return "($)";
    case NPC_PIRATE: return "(P)";
    case NPC_XENO: return "(X)";
    default: return "";
    }
}

void scr_galaxy_draw(UI *u, Game *g) {
    int gg = u->gg, gs = u->gs;
    char t[320], r[120];
    snprintf(t, sizeof t, " Galáxia [%d:%d] ", gg, gs);
    snprintf(r, sizeof r, "%s", intergalactic_ok(g) ? "viagem intergaláctica liberada" : "outras galáxias: Prop. Hiperespacial 3");
    title_line(t, r);
    /* system strip */
    int sy = CY + 1;
    cv_fill(CX, sy, CW, 3, ' ', C_TEXT, RGB(0, 0, 0));
    art_starfield(CX, sy, CW, 3, (uint64_t)(gg * 1000 + gs), u->t, RGB(0, 0, 0));
    uint64_t starh = hash64((uint64_t)(gg * 1000 + gs) ^ g->seed);
    uint32_t sc = hash_unit(starh) < 0.3 ? RGB(255, 170, 90) : hash_unit(starh) < 0.7 ? RGB(255, 235, 150) : RGB(190, 215, 255);
    cv_text(CX + 1, sy, " \\|/", sc, COL_KEEP);
    cv_text(CX + 1, sy + 1, "-(@)-", sc, COL_KEEP);
    cv_text(CX + 1, sy + 2, " /|\\", sc, COL_KEEP);
    int sel = clampi(u->sel[SCR_GALAXY], 0, 15);
    u->sel[SCR_GALAXY] = sel;
    for (int p = 1; p <= 15; p++) {
        int x = CX + 6 + p * 5;
        int npi = npc_planet_at(g, gg, gs, p);
        int own = player_body_at(g, gg, gs, p, 0);
        uint32_t ch = 0x00B7, col = RGB(60, 70, 90);
        if (npi >= 0 || own >= 0) {
            PlanetInfo pi;
            if (own >= 0) pi = g->body[own].info; else planet_generate(g->seed, gg, gs, p, 0, &pi);
            ch = 0x25CF;
            col = own >= 0 ? C_CYAN : biome_color(pi.biome);
        }
        cv_put(x, sy + 1, ch, col, COL_KEEP);
        if (p - 1 == sel) { cv_put(x - 1, sy + 1, '[', C_GOLD, COL_KEEP); cv_put(x + 1, sy + 1, ']', C_GOLD, COL_KEEP); }
        cv_textf(x - (p >= 10), sy + 2, C_DIM, COL_KEEP, "%d", p);
    }
    /* table */
    int ty = CY + 4;
    cv_fill(CX, ty, CW, 1, ' ', C_TEXT, C_PANEL2);
    cv_text(CX + 1, ty, "Pos", C_TITLE, C_PANEL2);
    cv_text(CX + 6, ty, "Planeta", C_TITLE, C_PANEL2);
    cv_text(CX + 27, ty, "Lua", C_TITLE, C_PANEL2);
    cv_text(CX + 32, ty, "Destr.", C_TITLE, C_PANEL2);
    cv_text(CX + 40, ty, "Jogador", C_TITLE, C_PANEL2);
    cv_text(CX + 66, ty, "Status", C_TITLE, C_PANEL2);
    cv_text(CX + 73, ty, "Rank", C_TITLE, C_PANEL2);
    int order[N_NPC];
    static double ptsbuf[N_NPC];
    int nr = npc_rank_list(g, order, ptsbuf);
    double me = player_points(g, NULL, NULL, NULL, NULL);
    for (int p = 1; p <= 16; p++) {
        int y = ty + p;
        uint32_t bg = p - 1 == sel ? C_SEL : (p & 1 ? C_PANEL : RGB(16, 23, 39));
        cv_fill(CX, y, CW, 1, ' ', C_TEXT, bg);
        cv_textf(CX + 1, y, C_DIM, bg, "%2d", p);
        if (p == 16) { cv_text(CX + 6, y, "Espaço profundo (expedições)", C_PURPLE, bg); continue; }
        int npi = npc_planet_at(g, gg, gs, p);
        int own = player_body_at(g, gg, gs, p, 0);
        Debris *d = debris_at(g, gg, gs, p, 0);
        if (d && d->m + d->c >= 1) {
            char a[24];
            fmt_short(a, sizeof a, d->m + d->c);
            cv_text(CX + 32, y, a, C_GOLD, bg);
        }
        if (own >= 0) {
            cv_put(CX + 5, y, 0x25CF, C_CYAN, bg);
            cv_text_w(CX + 7, y, 19, g->body[own].name, C_CYAN, bg);
            if (g->body[own].moon >= 0) cv_text(CX + 27, y, "(o)", RGB(190, 190, 200), bg);
            cv_text(CX + 40, y, g->name, C_CYAN, bg);
            cv_text(CX + 66, y, "você", C_CYAN, bg);
            continue;
        }
        if (npi >= 0) {
            const NpcPlanet *pl = &g->npl[npi];
            const Npc *n = &g->npc[pl->npc];
            PlanetInfo pi;
            planet_generate(g->seed, gg, gs, p, 0, &pi);
            cv_put(CX + 5, y, 0x25CF, biome_color(pi.biome), bg);
            cv_text_w(CX + 7, y, 19, pl->pname, C_TEXT, bg);
            if (pl->has_moon) cv_text(CX + 27, y, "(o)", RGB(190, 190, 200), bg);
            uint32_t nc = n->type == NPC_PIRATE || n->type == NPC_XENO ? C_RED : n->type == NPC_INACTIVE ? C_DIM : n->type == NPC_TRADER ? C_GOLD : C_TEXT;
            char nm[48];
            snprintf(nm, sizeof nm, "%s%s%s%s", n->name, n->tag[0] ? " [" : "", n->tag, n->tag[0] ? "]" : "");
            cv_text_w(CX + 40, y, 25, nm, nc, bg);
            cv_text(CX + 66, y, npc_flag(n->type), nc, bg);
            if (n->type != NPC_PIRATE && n->type != NPC_XENO) {
                int rk = 1;
                double pts = npc_points(g, pl->npc);
                for (int i = 0; i < nr; i++) if (ptsbuf[i] > pts) rk++;
                if (me > pts) rk++;
                cv_textf(CX + 73, y, C_DIM, bg, "%d", rk);
            }
            continue;
        }
        if (is_explored(g, gg, gs, p)) {
            PlanetInfo pi;
            planet_generate(g->seed, gg, gs, p, 0, &pi);
            cv_put(CX + 5, y, 0x25CB, biome_color(pi.biome), bg);
            snprintf(t, sizeof t, "%s, %d campos", BIOME_NAME[pi.biome], pi.fields);
            cv_text_w(CX + 7, y, 32, t, C_GREEN, bg);
        } else cv_text(CX + 7, y, "\u2014 livre", RGB(70, 80, 100), bg);
    }
    /* details */
    int dy = ty + 18;
    ui_panel(CX, dy, CW, CANVAS_H - 1 - dy, "Detalhes");
    int p = sel + 1;
    int npi = p <= 15 ? npc_planet_at(g, gg, gs, p) : -1;
    int own = p <= 15 ? player_body_at(g, gg, gs, p, 0) : -1;
    const Body *cb = &g->body[g->cur];
    double dist = fleet_distance(cb->g, cb->s, cb->p, gg, gs, p);
    char ds[32];
    fmt_num(ds, sizeof ds, dist);
    if (p == 16) {
        cv_wrap(CX + 2, dy + 1, CW - 4, 3, "Região inexplorada além da órbita do último planeta. Expedições encontram recursos, naves e Matéria Negra... ou piratas, aliens e buracos negros. Requer Astrofísica.", C_TEXT, C_PANEL);
    } else if (own >= 0) {
        cv_textf(CX + 2, dy + 1, C_CYAN, C_PANEL, "%s - seu %s", g->body[own].name, own == 0 ? "planeta natal" : "planeta");
    } else if (npi >= 0) {
        const NpcPlanet *pl = &g->npl[npi];
        const Npc *n = &g->npc[pl->npc];
        char pts[32];
        fmt_num(pts, sizeof pts, npc_points(g, pl->npc));
        cv_textf(CX + 2, dy + 1, C_TEXT, C_PANEL, "%s [%d:%d:%d] - %s, %s (%d planetas)  Pontos: %s", pl->pname, gg, gs, p, n->name, npc_type_name(n->type), n->nplanets, pts);
        const char *desc = n->type == NPC_INACTIVE ? "Inativo: acumula recursos e quase não tem defesas. Alvo de saque, mas atacar fracos custa honra."
                         : n->type == NPC_TRADER ? "Mundo Mercante: crie rotas comerciais de exportação/importação com ele (Comércio)."
                         : n->type == NPC_PIRATE ? "Covil pirata: origem de muitos ataques. Frota forte, bom saque e honra."
                         : n->type == NPC_XENO ? "Colmeia Xeno: tecnologia superior. Perigo extremo."
                         : n->type == NPC_FLEETER ? "Agressivo: mantém grandes frotas e ataca vizinhos."
                         : n->type == NPC_TURTLE ? "Defensivo: muitas defesas planetárias." : "Minerador: economia forte, defesa moderada.";
        cv_wrap(CX + 2, dy + 2, CW - 4, 2, desc, C_DIM, C_PANEL);
    } else {
        PlanetInfo pi;
        planet_generate(g->seed, gg, gs, p, 0, &pi);
        if (is_explored(g, gg, gs, p))
            snprintf(t, sizeof t, "Explorado: %s, %d campos, %d°C a %d°C, fertilidade %.0f%%, riqueza M%+.0f%% C%+.0f%% D%+.0f%%",
                     BIOME_NAME[pi.biome], pi.fields, pi.tmin, pi.tmax, pi.fertility * 100, (pi.rich[0] - 1) * 100, (pi.rich[1] - 1) * 100, (pi.rich[2] - 1) * 100);
        else snprintf(t, sizeof t, "Posição livre. Envie uma sonda/desbravador (Explorar) para revelar bioma, tamanho e riqueza antes de colonizar.%s",
                      position_allowed(g, p) ? "" : " Posição extrema: exige Astrofísica mais alta.");
        cv_wrap(CX + 2, dy + 1, CW - 4, 3, t, is_explored(g, gg, gs, p) ? C_GREEN : C_TEXT, C_PANEL);
    }
    cv_textf(CX + 2, dy + 4, C_DIM, C_PANEL, "Distância de %s: %s", cb->name, ds);
    hint("\u2190\u2192 sistema  PgUp/PgDn galáxia  \u2191\u2193 posição  Enter ações  G ir para  Home meu sistema");
}

static void galaxy_actions(UI *u, Game *g) {
    int p = u->sel[SCR_GALAXY] + 1;
    int npi = p <= 15 ? npc_planet_at(g, u->gg, u->gs, p) : -1;
    int own = p <= 15 ? player_body_at(g, u->gg, u->gs, p, 0) : -1;
    Debris *d = p <= 15 ? debris_at(g, u->gg, u->gs, p, 0) : NULL;
    const char *items[10];
    int ok[10];
    items[0] = "Espionar (envio rápido de sondas)"; ok[0] = npi >= 0;
    items[1] = "Atacar"; ok[1] = npi >= 0;
    items[2] = "Transportar recursos"; ok[2] = own >= 0 && own != g->cur;
    items[3] = "Estacionar frota"; ok[3] = own >= 0 && own != g->cur;
    items[4] = "Colonizar"; ok[4] = p <= 15 && npi < 0 && own < 0;
    items[5] = "Explorar (sonda/desbravador)"; ok[5] = p <= 15 && own < 0;
    items[6] = "Reciclar destroços"; ok[6] = d && d->m + d->c >= 1;
    items[7] = "Expedição"; ok[7] = p == 16;
    items[8] = "Criar rota comercial (Mercante)"; ok[8] = npi >= 0 && g->npc[g->npl[npi].npc].type == NPC_TRADER;
    items[9] = "Disparar mísseis interplanetários"; ok[9] = npi >= 0 && g->body[g->cur].def[DI(D_IPM)] > 0;
    char title[64];
    snprintf(title, sizeof title, "Ações em [%d:%d:%d]", u->gg, u->gs, p);
    ui_menu(u, MP_GAL_ACTION, p, title, 10, items, ok);
}

int scr_galaxy_key(UI *u, Game *g, int k) {
    switch (k) {
    case K_LEFT: u->gs = u->gs <= 1 ? UNI_SYSTEMS : u->gs - 1; return 1;
    case K_RIGHT: u->gs = u->gs >= UNI_SYSTEMS ? 1 : u->gs + 1; return 1;
    case K_PGUP: u->gg = u->gg <= 1 ? UNI_GALAXIES : u->gg - 1; return 1;
    case K_PGDN: u->gg = u->gg >= UNI_GALAXIES ? 1 : u->gg + 1; return 1;
    case K_UP: u->sel[SCR_GALAXY] = (u->sel[SCR_GALAXY] + 15) % 16; return 1;
    case K_DOWN: u->sel[SCR_GALAXY] = (u->sel[SCR_GALAXY] + 1) % 16; return 1;
    case K_HOME: u->gg = g->body[g->cur].g; u->gs = g->body[g->cur].s; u->sel[SCR_GALAXY] = g->body[g->cur].p - 1; return 1;
    case K_ENTER: case ' ': galaxy_actions(u, g); return 1;
    case 'g': case 'G': {
        char init[24];
        snprintf(init, sizeof init, "%d:%d", u->gg, u->gs);
        ui_prompt(u, MP_GOTO, 0, "Ir para", "Digite galáxia:sistema (ex.: 3:120)", init, 0);
        return 1;
    }
    }
    return 0;
}

/* ===================== EMPIRE ===================== */
void scr_empire_draw(UI *u, Game *g) {
    title_line(" Império - visão de todos os planetas ", NULL);
    int cols[MAX_BODIES], n = 0;
    for (int i = 0; i < g->nbody; i++) {
        if (!g->body[i].used || g->body[i].is_moon) continue;
        cols[n++] = i;
        if (g->body[i].moon >= 0) cols[n++] = g->body[i].moon;
    }
    int sel = clampi(u->sel[SCR_EMPIRE], 0, n - 1);
    u->sel[SCR_EMPIRE] = sel;
    int vis = 4;
    int sc = u->scroll[SCR_EMPIRE];
    if (sel < sc) sc = sel;
    if (sel >= sc + vis) sc = sel - vis + 1;
    u->scroll[SCR_EMPIRE] = sc;
    static const char *ROWS[] = {"Nome", "Coordenadas", "Campos", "Metal", "Cristal", "Deutério", "Alimento", "População",
                                 "Energia", "Metal/h", "Cristal/h", "Deutério/h", "Alimento/h", "Minas M/C/D", "Robôs/Est/Lab",
                                 "Habit/Fazenda", "Frota (valor)", "Defesa (valor)", "Ameaça", "Obras"};
    int nr = (int)(sizeof ROWS / sizeof ROWS[0]);
    int y0 = CY + 2;
    for (int r = 0; r < nr; r++) {
        uint32_t bg = r & 1 ? C_PANEL : RGB(16, 23, 39);
        cv_fill(CX, y0 + r, CW, 1, ' ', C_TEXT, bg);
        cv_text(CX + 1, y0 + r, ROWS[r], C_TITLE, bg);
    }
    double tot[12] = {0};
    for (int c = 0; c < vis && sc + c < n; c++) {
        int bi = cols[sc + c];
        const Body *b = &g->body[bi];
        Rates r;
        body_rates(g, b, &r);
        int x = CX + 16 + c * 16;
        int issel = sc + c == sel;
        if (issel) cv_tint_rect(x - 1, y0, 16, nr, C_SEL);
        char v[48];
        int y = y0;
#define CELL(fmtcol, ...) do { snprintf(v, sizeof v, __VA_ARGS__); cv_text_w(x, y++, 15, v, fmtcol, COL_KEEP); } while (0)
        CELL(b->is_moon ? RGB(190, 190, 200) : C_WHITE, "%s", b->name);
        CELL(C_CYAN, "[%d:%d:%d]", b->g, b->s, b->p);
        CELL(C_TEXT, "%d/%d", body_fields_used(b), body_fields_max(g, b));
        for (int k = 0; k < 4; k++) { fmt_short(v, sizeof v, b->res[k]); cv_text_w(x, y++, 15, v, C_TEXT, COL_KEEP); }
        if (b->is_moon) CELL(C_DIM, "-"); else { fmt_short(v, sizeof v, b->pop); cv_text_w(x, y++, 15, v, C_TEXT, COL_KEEP); }
        fmt_short(v, sizeof v, r.e_prod - r.e_cons);
        cv_text_w(x, y++, 15, v, r.e_prod >= r.e_cons ? C_GREEN : C_RED, COL_KEEP);
        for (int k = 0; k < 4; k++) { fmt_short(v, sizeof v, r.prod[k]); cv_text_w(x, y++, 15, v, r.prod[k] >= 0 ? C_GREEN : C_RED, COL_KEEP); }
        CELL(C_GOLD, "%d/%d/%d", b->bld[B_METAL_MINE], b->bld[B_CRYSTAL_MINE], b->bld[B_DEUT_SYNTH]);
        CELL(C_GOLD, "%d/%d/%d", b->bld[B_ROBOTICS], b->bld[B_SHIPYARD], b->bld[B_LAB]);
        CELL(C_GOLD, "%d/%d", b->bld[B_HABITAT], b->bld[B_FARM]);
        fmt_short(v, sizeof v, items_value(b->ship, NULL)); cv_text_w(x, y++, 15, v, C_TEXT, COL_KEEP);
        fmt_short(v, sizeof v, items_value(NULL, b->def)); cv_text_w(x, y++, 15, v, C_TEXT, COL_KEEP);
        int th = b->is_moon ? 0 : threat_level(g, bi);
        CELL(th >= 7 ? C_RED : th >= 4 ? C_ORANGE : C_GREEN, "%d/10", th);
        CELL(C_DIM, "%d+%d+%d", b->bq.n, b->lq.n, b->nsq);
#undef CELL
    }
    for (int i = 0; i < n; i++) {
        const Body *b = &g->body[cols[i]];
        Rates r;
        body_rates(g, b, &r);
        for (int k = 0; k < 4; k++) { tot[k] += b->res[k]; tot[4 + k] += r.prod[k]; }
        tot[8] += b->pop;
    }
    int shown = n - sc < vis ? n - sc : vis;
    int x = CX + 17 + shown * 16;
    if (n > 1 && x < CX + CW - 8) {
        int y = y0;
        cv_text(x, y, "TOTAL", C_GOLD, COL_KEEP);
        char v[32];
        for (int k = 0; k < 4; k++) { fmt_short(v, sizeof v, tot[k]); cv_text_w(x, y0 + 3 + k, CX + CW - x - 1, v, C_GOLD, COL_KEEP); }
        fmt_short(v, sizeof v, tot[8]); cv_text_w(x, y0 + 7, CX + CW - x - 1, v, C_GOLD, COL_KEEP);
        for (int k = 0; k < 4; k++) { fmt_short(v, sizeof v, tot[4 + k]); cv_text_w(x, y0 + 9 + k, CX + CW - x - 1, v, C_GOLD, COL_KEEP); }
    }
    hint("\u2190\u2192 selecionar planeta  Enter ir para o planeta  (Total de todos à direita)");
}

int scr_empire_key(UI *u, Game *g, int k) {
    int cols[MAX_BODIES], n = 0;
    for (int i = 0; i < g->nbody; i++) {
        if (!g->body[i].used || g->body[i].is_moon) continue;
        cols[n++] = i;
        if (g->body[i].moon >= 0) cols[n++] = g->body[i].moon;
    }
    if (k == K_LEFT) { u->sel[SCR_EMPIRE] = (u->sel[SCR_EMPIRE] + n - 1) % n; return 1; }
    if (k == K_RIGHT) { u->sel[SCR_EMPIRE] = (u->sel[SCR_EMPIRE] + 1) % n; return 1; }
    if (k == K_ENTER) { g->cur = cols[clampi(u->sel[SCR_EMPIRE], 0, n - 1)]; u->screen = SCR_OVERVIEW; return 1; }
    return 0;
}

/* ===================== TRADE ===================== */
static int trade_worlds(Game *g, const Body *o, int *out, int max) {
    int n = 0;
    int idx[512];
    double dist[512];
    for (int i = 0; i < g->nnpl && n < 512; i++) {
        if (g->npc[g->npl[i].npc].type != NPC_TRADER) continue;
        double d = fleet_distance(o->g, o->s, o->p, g->npl[i].g, g->npl[i].s, g->npl[i].p);
        if (g->npl[i].g != o->g && !intergalactic_ok(g)) d += 1e7;
        int k = n++;
        while (k > 0 && dist[k - 1] > d) { dist[k] = dist[k - 1]; idx[k] = idx[k - 1]; k--; }
        dist[k] = d;
        idx[k] = i;
    }
    if (n > max) n = max;
    for (int i = 0; i < n; i++) out[i] = idx[i];
    return n;
}

static void draw_route_wizard(UI *u, Game *g) {
    RouteWiz *w = &u->rw;
    Body *o = &g->body[w->origin];
    char t[200];
    snprintf(t, sizeof t, " Nova rota comercial a partir de %s - passo %d/4 ", o->name, w->step + 1);
    title_line(t, NULL);
    ui_panel(CX, CY + 1, 52, CH - 2, w->step == 0 ? "Tipo de rota" : w->step == 1 ? "Destino" : w->step == 2 ? "Carga" : "Naves da rota");
    int y = CY + 3;
    if (w->step == 0) {
        static const char *K[2] = {"Interna: entre seus planetas/luas", "Exportação/importação com mundo Mercante"};
        for (int i = 0; i < 2; i++) {
            uint32_t bg = w->row == i ? C_SEL : C_PANEL;
            cv_fill(CX + 2, y + i * 2, 48, 1, ' ', C_TEXT, bg);
            cv_text(CX + 4, y + i * 2, K[i], C_WHITE, bg);
        }
        cv_wrap(CX + 3, y + 6, 46, 8, "Rotas internas levam recursos de um planeta a outro continuamente (ex.: alimento de um mundo fértil para um mundo árido). Rotas com Mercantes vendem um recurso e trazem outro, com taxas que flutuam com o mercado.", C_DIM, C_PANEL);
    } else if (w->step == 1) {
        if (w->kind == RT_INTERNAL) {
            int k = 0;
            for (int i = 0; i < g->nbody; i++) {
                if (!g->body[i].used || i == w->origin) continue;
                uint32_t bg = w->row == k ? C_SEL : C_PANEL;
                cv_fill(CX + 2, y, 48, 1, ' ', C_TEXT, bg);
                char lb[80];
                cv_text_w(CX + 4, y, 44, body_label(g, i, lb, sizeof lb), C_WHITE, bg);
                y++; k++;
            }
            if (k == 0) cv_text(CX + 4, y, "Você precisa de outra colônia ou lua.", C_RED, C_PANEL);
        } else {
            for (int k = 0; k < w->nnpl && k < 20; k++) {
                const NpcPlanet *pl = &g->npl[w->npl_list[k]];
                uint32_t bg = w->row == k ? C_SEL : C_PANEL;
                cv_fill(CX + 2, y, 48, 1, ' ', C_TEXT, bg);
                cv_textf(CX + 4, y, C_GOLD, bg, "[%d:%d:%d] %s", pl->g, pl->s, pl->p, g->npc[pl->npc].name);
                y++;
            }
            if (w->nnpl == 0) cv_text(CX + 4, y, "Nenhum mundo Mercante encontrado.", C_RED, C_PANEL);
        }
    } else if (w->step == 2) {
        const char *rows[3] = {"Exportar", "Importar", "Qtd./viagem"};
        int nrow = w->kind == RT_NPC ? 3 : 2;
        for (int r = 0; r < 3; r++) {
            if (w->kind == RT_INTERNAL && r == 1) continue;
            int yy = y + r * 2;
            uint32_t bg = w->row == r ? C_SEL : C_PANEL;
            cv_fill(CX + 2, yy, 48, 1, ' ', C_TEXT, bg);
            cv_text(CX + 4, yy, rows[r], C_TEXT, bg);
            char v[48];
            if (r == 0) snprintf(v, sizeof v, "\u25C4 %s \u25BA", res_name(w->res_out));
            else if (r == 1) snprintf(v, sizeof v, "\u25C4 %s \u25BA", res_name(w->res_in));
            else if (w->row == 2 && w->num[0]) snprintf(v, sizeof v, "%s_", w->num);
            else if (w->amount <= 0) snprintf(v, sizeof v, "máximo da capacidade");
            else fmt_num(v, sizeof v, w->amount);
            cv_text(CX + 20, yy, v, C_GOLD, bg);
        }
        (void)nrow;
        if (w->kind == RT_NPC) {
            double r = market_rate(g, w->npl_list[0] >= 0 ? g->npl[npc_planet_at(g, w->tg, w->ts, w->tp)].npc : 0, w->res_out, w->res_in);
            cv_textf(CX + 4, y + 7, C_CYAN, C_PANEL, "Taxa atual: 1 %s = %.3f %s", res_name(w->res_out), r, res_name(w->res_in));
        }
    } else {
        cv_text(CX + 3, y - 1, "Nave", C_TITLE, C_PANEL);
        cv_text(CX + 30, y - 1, "Disp.", C_TITLE, C_PANEL);
        cv_text(CX + 40, y - 1, "Na rota", C_TITLE, C_PANEL);
        for (int i = 0; i < N_SHIP; i++) {
            if (!flyable(i) || T_END + i == S_COLONY || T_END + i == S_PROBE) continue;
            uint32_t bg = w->row == i ? C_SEL : C_PANEL;
            cv_fill(CX + 2, y, 48, 1, ' ', C_TEXT, bg);
            cv_text(CX + 3, y, ITEMS[T_END + i].name, o->ship[i] > 0 ? C_TEXT : C_DIM, bg);
            char a[24];
            fmt_short(a, sizeof a, (double)o->ship[i]);
            cv_text_r(CX + 35, y, a, C_DIM, bg);
            if (w->row == i && w->num[0]) cv_text_r(CX + 47, y, w->num, C_GOLD, bg);
            else { fmt_short(a, sizeof a, (double)w->ship[i]); cv_text_r(CX + 47, y, a, w->ship[i] ? C_GREEN : C_DIM, bg); }
            y++;
        }
    }
    /* summary */
    int sx = CX + 53, sw = CW - 53;
    ui_panel(sx, CY + 1, sw, CH - 2, "Resumo");
    Route tmp;
    memset(&tmp, 0, sizeof tmp);
    tmp.kind = w->kind; tmp.origin = w->origin; tmp.dest = w->dest; tmp.tg = w->tg; tmp.ts = w->ts; tmp.tp = w->tp;
    memcpy(tmp.ship, w->ship, sizeof tmp.ship);
    int yy = CY + 3;
    cv_textf(sx + 2, yy++, C_TEXT, C_PANEL, "Porto Comercial: nv %d", o->bld[B_TRADE_HUB]);
    cv_textf(sx + 2, yy++, C_TEXT, C_PANEL, "Rotas desta origem: %d", routes_from(g, w->origin));
    cv_textf(sx + 2, yy++, C_TEXT, C_PANEL, "Logística: %d", g->tech[TI(T_LOGISTICS)]);
    yy++;
    if (w->step >= 2) {
        cv_textf(sx + 2, yy++, C_CYAN, C_PANEL, "Destino [%d:%d:%d]", w->tg, w->ts, w->tp);
        char a[32];
        fmt_num(a, sizeof a, route_capacity(g, &tmp));
        cv_textf(sx + 2, yy++, C_TEXT, C_PANEL, "Capacidade: %s", a);
        if (fleet_ship_count(w->ship) > 0) {
            char tm[32];
            fmt_time(tm, sizeof tm, route_trip_time(g, &tmp) * 2);
            cv_textf(sx + 2, yy++, C_TEXT, C_PANEL, "Ciclo: %s", tm);
            double rk = route_risk(g, &tmp);
            cv_textf(sx + 2, yy++, rk > 0.1 ? C_RED : rk > 0.05 ? C_ORANGE : C_GREEN, C_PANEL, "Risco/viagem: %.1f%%", rk * 100);
        }
    }
    if (w->err[0]) cv_wrap(sx + 2, CY + CH - 7, sw - 4, 4, w->err, C_RED, C_PANEL);
    hint(w->step == 3 ? "\u2191\u2193 nave  dígitos quantidade  A todas desta  Enter criar rota  Esc voltar"
                      : "\u2191\u2193 escolher  \u2190\u2192 alterar  Enter próximo  Esc voltar");
}

static int route_wizard_key(UI *u, Game *g, int k) {
    RouteWiz *w = &u->rw;
    Body *o = &g->body[w->origin];
    w->err[0] = 0;
    if (k == K_ESC) { if (w->step == 0) w->active = 0; else { w->step--; w->row = 0; w->num[0] = 0; } return 1; }
    if (w->step == 0) {
        if (k == K_UP || k == K_DOWN) { w->row ^= 1; return 1; }
        if (k == K_ENTER) {
            w->kind = w->row ? RT_NPC : RT_INTERNAL;
            w->step = 1; w->row = 0;
            if (w->kind == RT_NPC) w->nnpl = trade_worlds(g, o, w->npl_list, 20);
            return 1;
        }
        return 1;
    }
    if (w->step == 1) {
        int n = 0, list[MAX_BODIES];
        if (w->kind == RT_INTERNAL) { for (int i = 0; i < g->nbody; i++) if (g->body[i].used && i != w->origin) list[n++] = i; }
        else n = w->nnpl;
        if (n == 0) { if (k == K_ENTER) w->active = 0; return 1; }
        if (k == K_UP) { w->row = (w->row + n - 1) % n; return 1; }
        if (k == K_DOWN) { w->row = (w->row + 1) % n; return 1; }
        if (k == K_ENTER) {
            if (w->kind == RT_INTERNAL) {
                w->dest = list[w->row];
                w->tg = g->body[w->dest].g; w->ts = g->body[w->dest].s; w->tp = g->body[w->dest].p;
            } else {
                const NpcPlanet *pl = &g->npl[w->npl_list[w->row]];
                w->tg = pl->g; w->ts = pl->s; w->tp = pl->p;
                w->dest = -1;
            }
            w->step = 2; w->row = 0;
            return 1;
        }
        return 1;
    }
    if (w->step == 2) {
        if (k == K_UP || k == K_DOWN) {
            if (w->num[0]) { w->amount = (double)atoll(w->num); w->num[0] = 0; }
            do { w->row = (w->row + (k == K_UP ? 2 : 1)) % 3; } while (w->kind == RT_INTERNAL && w->row == 1);
            return 1;
        }
        if (k == K_LEFT || k == K_RIGHT) {
            int d = k == K_RIGHT ? 1 : NRES - 1;
            if (w->row == 0) w->res_out = (w->res_out + d) % NRES;
            else if (w->row == 1) w->res_in = (w->res_in + d) % NRES;
            return 1;
        }
        if (w->row == 2 && num_append(w->num, sizeof w->num, k)) { w->amount = (double)atoll(w->num); return 1; }
        if (k == K_ENTER) {
            if (w->num[0]) { w->amount = (double)atoll(w->num); w->num[0] = 0; }
            if (w->kind == RT_NPC && w->res_in == w->res_out) { snprintf(w->err, sizeof w->err, "Importação deve ser diferente da exportação"); return 1; }
            w->step = 3;
            w->row = SI(S_LARGE_CARGO);
            return 1;
        }
        return 1;
    }
    /* ships */
    if (k == K_UP || k == K_DOWN) {
        if (w->num[0]) { long long v = atoll(w->num); w->ship[w->row] = v > o->ship[w->row] ? o->ship[w->row] : v; w->num[0] = 0; }
        for (int i = 0; i < N_SHIP; i++) {
            w->row = (w->row + (k == K_UP ? N_SHIP - 1 : 1)) % N_SHIP;
            if (flyable(w->row) && T_END + w->row != S_COLONY && T_END + w->row != S_PROBE) break;
        }
        return 1;
    }
    if (num_append(w->num, sizeof w->num, k)) { long long v = atoll(w->num); w->ship[w->row] = v > o->ship[w->row] ? o->ship[w->row] : v; return 1; }
    if (k == 'a' || k == 'A') { w->ship[w->row] = o->ship[w->row]; w->num[0] = 0; return 1; }
    if (k == K_ENTER) {
        if (w->num[0]) { long long v = atoll(w->num); w->ship[w->row] = v > o->ship[w->row] ? o->ship[w->row] : v; w->num[0] = 0; }
        Route t;
        memset(&t, 0, sizeof t);
        t.kind = w->kind; t.origin = w->origin; t.dest = w->dest; t.tg = w->tg; t.ts = w->ts; t.tp = w->tp;
        t.res_out = w->res_out; t.res_in = w->res_in; t.amount = w->amount;
        memcpy(t.ship, w->ship, sizeof t.ship);
        char err[160];
        if (route_create(g, &t, err, sizeof err)) { ui_toast(u, C_GREEN, "%s", err); w->active = 0; }
        else snprintf(w->err, sizeof w->err, "%s", err);
        return 1;
    }
    return 1;
}

static void route_wizard_open(UI *u, Game *g, int kind, int npi) {
    RouteWiz *w = &u->rw;
    memset(w, 0, sizeof *w);
    w->active = 1;
    w->origin = g->cur;
    w->res_out = RES_METAL;
    w->res_in = RES_CRYSTAL;
    u->screen = SCR_TRADE;
    if (npi >= 0) {
        w->kind = kind;
        w->tg = g->npl[npi].g; w->ts = g->npl[npi].s; w->tp = g->npl[npi].p;
        w->dest = -1;
        w->npl_list[0] = npi;
        w->nnpl = 1;
        w->step = 2;
    }
}

void scr_trade_draw(UI *u, Game *g) {
    if (u->rw.active) { draw_route_wizard(u, g); return; }
    Body *b = &g->body[g->cur];
    int tab = u->tab[SCR_TRADE];
    char t[200];
    snprintf(t, sizeof t, " Comércio - %s ", tab == 0 ? "Rotas comerciais" : "Mundos Mercantes");
    char r[120];
    snprintf(r, sizeof r, "Porto nv %d  Logística %d", b->bld[B_TRADE_HUB], g->tech[TI(T_LOGISTICS)]);
    title_line(t, r);
    cv_fill(CX, CY + 1, CW, 1, ' ', C_TEXT, C_PANEL2);
    cv_text(CX + 2, CY + 1, "[Rotas]", tab == 0 ? C_GOLD : C_DIM, C_PANEL2);
    cv_text(CX + 12, CY + 1, "[Mercados]", tab == 1 ? C_GOLD : C_DIM, C_PANEL2);
    cv_text(CX + 26, CY + 1, "Tab alterna", C_DIM, C_PANEL2);
    if (tab == 0) {
        int idx[MAX_ROUTES], n = 0;
        for (int i = 0; i < MAX_ROUTES; i++) if (g->route[i].used) idx[n++] = i;
        int sel = clampi(u->sel[SCR_TRADE], 0, n > 0 ? n - 1 : 0);
        u->sel[SCR_TRADE] = sel;
        int y = CY + 3;
        cv_fill(CX, y - 1, CW, 1, ' ', C_TEXT, C_BG);
        cv_text(CX + 1, y - 1, "#  Origem -> Destino            Carga           Naves/cap.  Estado        Viagens Risco", C_TITLE, C_BG);
        if (n == 0) {
            cv_wrap(CX + 2, y + 1, CW - 4, 6, "Nenhuma rota ativa. Pressione N para criar uma rota a partir do planeta atual.\n\nRequisitos: Logística 1 e Porto Comercial (cada nível permite uma rota a partir daquele planeta). "
                                                  "Atribua cargueiros (e escoltas militares para reduzir o risco de emboscadas piratas).", C_DIM, C_BG);
        }
        for (int k = 0; k < n; k++) {
            Route *rt = &g->route[idx[k]];
            uint32_t bg = k == sel ? C_SEL : (k & 1 ? C_PANEL : RGB(16, 23, 39));
            cv_fill(CX, y + k, CW, 1, ' ', C_TEXT, bg);
            char dst[40];
            if (rt->kind == RT_INTERNAL) snprintf(dst, sizeof dst, "%s", g->body[rt->dest].name);
            else snprintf(dst, sizeof dst, "$%s", g->npc[rt->npc].name);
            char cg[48];
            if (rt->kind == RT_INTERNAL) snprintf(cg, sizeof cg, "%s", res_name(rt->res_out));
            else snprintf(cg, sizeof cg, "%s>%s", res_name(rt->res_out), res_name(rt->res_in));
            char cap[24];
            fmt_short(cap, sizeof cap, route_capacity(g, rt));
            char st[32];
            double tt = rt->state == RS_IDLE ? rt->t_next : rt->state == RS_OUT ? rt->t_arrive : rt->t_return;
            char tm[24];
            fmt_time(tm, sizeof tm, tt - g->now);
            snprintf(st, sizeof st, "%s %s", rt->state == RS_IDLE ? "carrega" : rt->state == RS_OUT ? "ida" : "volta", tm);
            snprintf(t, sizeof t, "%-2d %-12.12s -> %-14.14s %-15.15s %-11s %-13.13s %-7lld %.0f%%", idx[k] + 1, g->body[rt->origin].name, dst, cg, cap, st, rt->trips, route_risk(g, rt) * 100);
            cv_text_w(CX + 1, y + k, CW - 2, t, C_TEXT, bg);
        }
        if (n > 0) {
            Route *rt = &g->route[idx[sel]];
            int dy = CANVAS_H - 8;
            ui_panel(CX, dy, CW, 7, "Detalhes da rota");
            char a[32], c[32];
            fmt_num(a, sizeof a, rt->delivered);
            fmt_num(c, sizeof c, route_capacity(g, rt));
            cv_textf(CX + 2, dy + 1, C_TEXT, C_PANEL, "Entregue no total: %s   Capacidade: %s   Emboscadas: %lld   Naves perdidas: %lld", a, c, rt->ambushes, rt->lost_ships);
            t[0] = 0;
            for (int i = 0; i < N_SHIP; i++) if (rt->ship[i] > 0) { size_t L = strlen(t); snprintf(t + L, sizeof t - L, "%s%lld %s", L ? ", " : "Naves: ", rt->ship[i], ITEMS[T_END + i].shortn); }
            cv_text_w(CX + 2, dy + 2, CW - 4, t, C_DIM, C_PANEL);
            if (rt->kind == RT_NPC) cv_textf(CX + 2, dy + 3, C_CYAN, C_PANEL, "Taxa atual: 1 %s = %.3f %s", res_name(rt->res_out), market_rate(g, rt->npc, rt->res_out, rt->res_in), res_name(rt->res_in));
            fmt_time(a, sizeof a, route_trip_time(g, rt) * 2);
            cv_textf(CX + 2, dy + 4, C_DIM, C_PANEL, "Ciclo completo: %s   Quantidade/viagem: %s", a, rt->amount > 0 ? "fixa" : "máxima");
        }
        hint("N nova rota (origem = planeta atual)  X desfazer rota  \u2191\u2193 selecionar  Tab mercados");
    } else {
        int list[64];
        int n = trade_worlds(g, b, list, 16);
        int sel = clampi(u->sel[SCR_TRADE + 0], 0, n > 0 ? n - 1 : 0);
        int y = CY + 3;
        cv_fill(CX, y - 1, CW, 1, ' ', C_TEXT, C_BG);
        cv_text(CX + 1, y - 1, "Mundo", C_TITLE, C_BG);
        cv_text(CX + 22, y - 1, "Coord.", C_TITLE, C_BG);
        cv_text_r(CX + 39, y - 1, "Dist.", C_TITLE, C_BG);
        cv_text_r(CX + 50, y - 1, "Met>Cri", C_TITLE, C_BG);
        cv_text_r(CX + 60, y - 1, "Cri>Met", C_TITLE, C_BG);
        cv_text_r(CX + 70, y - 1, "Deu>Met", C_TITLE, C_BG);
        cv_text_r(CX + 80, y - 1, "Ali>Met", C_TITLE, C_BG);
        for (int k = 0; k < n; k++) {
            const NpcPlanet *pl = &g->npl[list[k]];
            uint32_t bg = k == u->scroll[SCR_TRADE] ? C_SEL : (k & 1 ? C_PANEL : RGB(16, 23, 39));
            cv_fill(CX, y + k, CW, 1, ' ', C_TEXT, bg);
            double d = fleet_distance(b->g, b->s, b->p, pl->g, pl->s, pl->p);
            char ds[24], v[24];
            fmt_short(ds, sizeof ds, d);
            cv_text_w(CX + 1, y + k, 20, g->npc[pl->npc].name, C_GOLD, bg);
            cv_textf(CX + 22, y + k, C_CYAN, bg, "[%d:%d:%d]", pl->g, pl->s, pl->p);
            cv_text_r(CX + 39, y + k, ds, C_DIM, bg);
            static const int FR[4] = {RES_METAL, RES_CRYSTAL, RES_DEUT, RES_FOOD}, TO[4] = {RES_CRYSTAL, RES_METAL, RES_METAL, RES_METAL};
            for (int c = 0; c < 4; c++) {
                double rt = market_rate(g, pl->npc, FR[c], TO[c]);
                double base = merchant_rate(FR[c], TO[c]);
                snprintf(v, sizeof v, "%.3f", rt);
                cv_text_r(CX + 50 + c * 10, y + k, v, rt > base * 0.95 ? C_GREEN : rt < base * 0.8 ? C_RED : C_TEXT, bg);
            }
        }
        (void)sel;
        cv_wrap(CX + 2, CANVAS_H - 5, CW - 4, 3, "As taxas mudam com o tempo (oferta e demanda). Cada Mercante paga mais pelo recurso que deseja e vende mais barato o que produz. Enter cria uma rota com o mundo selecionado.", C_DIM, C_BG);
        hint("\u2191\u2193 selecionar mundo  Enter criar rota com ele  Tab rotas");
    }
}

int scr_trade_key(UI *u, Game *g, int k) {
    if (u->rw.active) return route_wizard_key(u, g, k);
    if (k == K_TAB) { u->tab[SCR_TRADE] ^= 1; return 1; }
    if (u->tab[SCR_TRADE] == 0) {
        int idx[MAX_ROUTES], n = 0;
        for (int i = 0; i < MAX_ROUTES; i++) if (g->route[i].used) idx[n++] = i;
        if (k == K_UP && n) { u->sel[SCR_TRADE] = (u->sel[SCR_TRADE] + n - 1) % n; return 1; }
        if (k == K_DOWN && n) { u->sel[SCR_TRADE] = (u->sel[SCR_TRADE] + 1) % n; return 1; }
        if (k == 'n' || k == 'N') { route_wizard_open(u, g, RT_INTERNAL, -1); return 1; }
        if ((k == 'x' || k == 'X') && n) {
            int ri = idx[clampi(u->sel[SCR_TRADE], 0, n - 1)];
            ui_confirm(u, MP_DELETE_ROUTE, ri, "Desfazer rota", "Desfazer a rota #%d? As naves e a carga em trânsito retornam à origem.", ri + 1);
            return 1;
        }
        return 0;
    }
    int list[64];
    int n = trade_worlds(g, &g->body[g->cur], list, 16);
    if (k == K_UP && n) { u->scroll[SCR_TRADE] = (u->scroll[SCR_TRADE] + n - 1) % n; return 1; }
    if (k == K_DOWN && n) { u->scroll[SCR_TRADE] = (u->scroll[SCR_TRADE] + 1) % n; return 1; }
    if (k == K_ENTER && n) { route_wizard_open(u, g, RT_NPC, list[clampi(u->scroll[SCR_TRADE], 0, n - 1)]); return 1; }
    return 0;
}

/* ===================== MERCHANT ===================== */
void scr_merchant_draw(UI *u, Game *g) {
    Body *b = &g->body[g->cur];
    int free_tax = g->ev.merchant_until > g->now;
    title_line(" Mercador - troca de recursos ", free_tax ? "MERCADOR VIAJANTE: sem taxa!" : "taxa 10%");
    ui_panel(CX, CY + 1, CW, 18, "Balcão de trocas");
    static const uint32_t RC[NRES] = {RGB(190, 195, 205), RGB(110, 215, 240), RGB(90, 220, 190), RGB(130, 215, 90)};
    cv_text(CX + 4, CY + 3, "Vender (\u2191\u2193)", C_TITLE, C_PANEL);
    cv_text(CX + 30, CY + 3, "Comprar (\u2190\u2192)", C_TITLE, C_PANEL);
    for (int i = 0; i < NRES; i++) {
        uint32_t bg = u->mer_from == i ? C_SEL : C_PANEL;
        cv_fill(CX + 3, CY + 5 + i * 2, 22, 1, ' ', C_TEXT, bg);
        cv_text(CX + 4, CY + 5 + i * 2, res_name(i), RC[i], bg);
        char a[24];
        fmt_short(a, sizeof a, b->res[i]);
        cv_text_r(CX + 23, CY + 5 + i * 2, a, C_DIM, bg);
        uint32_t bg2 = u->mer_to == i ? C_SEL : C_PANEL;
        cv_fill(CX + 29, CY + 5 + i * 2, 22, 1, ' ', C_TEXT, bg2);
        cv_text(CX + 30, CY + 5 + i * 2, res_name(i), i == u->mer_from ? C_DIM : RC[i], bg2);
    }
    double rate = merchant_rate(u->mer_from, u->mer_to) * (free_tax ? 1.0 : 0.9);
    cv_textf(CX + 55, CY + 5, C_WHITE, C_PANEL, "1 %s =", res_name(u->mer_from));
    cv_textf(CX + 55, CY + 6, C_GOLD, C_PANEL, "%.3f %s", rate, res_name(u->mer_to));
    char a[24], c[24];
    fmt_num(a, sizeof a, 10000);
    fmt_num(c, sizeof c, floor(10000 * rate));
    cv_textf(CX + 55, CY + 8, C_DIM, C_PANEL, "%s -> %s", a, c);
    cv_wrap(CX + 55, CY + 10, 24, 6, "Base de valor 3:2:1 (metal:cristal:deutério); alimento vale 0,6 metal.", C_DIM, C_PANEL);
    cv_wrap(CX + 3, CY + 14, CW - 6, 3, "O mercador é útil para corrigir desequilíbrios (ex.: falta de cristal para pesquisas). Para fluxos constantes, prefira rotas comerciais com mundos Mercantes (melhores taxas, mas com risco de piratas).", C_DIM, C_PANEL);
    hint("\u2191\u2193 recurso a vender  \u2190\u2192 recurso a comprar  Enter informar quantidade e trocar");
}

int scr_merchant_key(UI *u, Game *g, int k) {
    (void)g;
    switch (k) {
    case K_UP: u->mer_from = (u->mer_from + NRES - 1) % NRES; return 1;
    case K_DOWN: u->mer_from = (u->mer_from + 1) % NRES; return 1;
    case K_LEFT: u->mer_to = (u->mer_to + NRES - 1) % NRES; return 1;
    case K_RIGHT: u->mer_to = (u->mer_to + 1) % NRES; return 1;
    case K_ENTER: {
        if (u->mer_from == u->mer_to) { ui_toast(u, C_RED, "Escolha recursos diferentes"); return 1; }
        char txt[160];
        snprintf(txt, sizeof txt, "Quanto %s vender por %s? (M = tudo)", res_name(u->mer_from), res_name(u->mer_to));
        ui_prompt(u, MP_MER_AMOUNT, 0, "Mercador", txt, "1000", 1);
        return 1;
    }
    }
    return 0;
}

/* ===================== MESSAGES ===================== */
static const char *msg_icon(int t) {
    switch (t) {
    case MSG_COMBAT: return "\u2694";
    case MSG_SPY: return "\u25C9";
    case MSG_EVENT: return "!";
    case MSG_FLEET: return "\u27A4";
    case MSG_TRADE: return "$";
    case MSG_EXPED: return "*";
    case MSG_QUEST: return "\u2713";
    default: return "i";
    }
}

void scr_messages_draw(UI *u, Game *g) {
    char r[64];
    snprintf(r, sizeof r, "%d mensagens, %d não lidas", g->nmsg, game_unread(g));
    title_line(" Mensagens ", r);
    int n = g->nmsg;
    int sel = clampi(u->sel[SCR_MESSAGES], 0, n > 0 ? n - 1 : 0);
    u->sel[SCR_MESSAGES] = sel;
    int lw = 36;
    ui_panel(CX, CY + 1, lw, CH - 2, "Caixa de entrada");
    int rows = CH - 4;
    int top = u->scroll[SCR_MESSAGES];
    if (sel < top) top = sel;
    if (sel >= top + rows) top = sel - rows + 1;
    u->scroll[SCR_MESSAGES] = top;
    for (int k = 0; k < rows && top + k < n; k++) {
        int i = n - 1 - (top + k);
        Message *m = &g->msg[i];
        int y = CY + 2 + k;
        uint32_t bg = top + k == sel ? C_SEL : C_PANEL;
        cv_fill(CX + 1, y, lw - 2, 1, ' ', C_TEXT, bg);
        uint32_t ic = m->type == MSG_COMBAT ? C_RED : m->type == MSG_SPY ? C_CYAN : m->type == MSG_EVENT ? C_ORANGE : m->type == MSG_QUEST ? C_GOLD : m->type == MSG_EXPED ? C_PURPLE : C_TEXT;
        cv_text(CX + 2, y, msg_icon(m->type), ic, bg);
        if (!m->read) cv_put(CX + 3, y, 0x2022, C_GOLD, bg);
        cv_text_w(CX + 5, y, lw - 7, m->title, m->read ? C_DIM : C_WHITE, bg);
    }
    if (n == 0) cv_text(CX + 3, CY + 3, "Nenhuma mensagem.", C_DIM, C_PANEL);
    int bx = CX + lw, bw = CW - lw;
    ui_panel(bx, CY + 1, bw, CH - 2, "Leitura");
    if (n > 0) {
        Message *m = &g->msg[n - 1 - sel];
        m->read = 1;
        cv_text_w(bx + 2, CY + 2, bw - 4, m->title, C_GOLD, C_PANEL);
        cv_bold_rect(bx + 2, CY + 2, bw - 4, 1);
        char clk[32];
        fmt_clock(clk, sizeof clk, m->t);
        cv_text(bx + 2, CY + 3, clk, C_DIM, C_PANEL);
        /* scroll body: render into lines, skip u->msg_scroll lines */
        int avail = CH - 7;
        cv_set_clip(bx + 1, CY + 5, bw - 2, avail);
        int total = cv_wrap(bx + 2, CY + 5 - u->msg_scroll, bw - 4, avail + u->msg_scroll, m->body, C_TEXT, C_PANEL);
        cv_set_clip(CX, CY, CW, CANVAS_H - CY);
        if (u->msg_scroll > total - avail) u->msg_scroll = total - avail > 0 ? total - avail : 0;
        if (total > avail) cv_textf(bx + bw - 12, CANVAS_H - 3, C_DIM, C_PANEL, "%d/%d", u->msg_scroll + avail, total);
    }
    hint("\u2191\u2193 mensagem  PgUp/PgDn rolar texto  X apagar  A marcar todas lidas  D apagar todas lidas");
}

int scr_messages_key(UI *u, Game *g, int k) {
    int n = g->nmsg;
    switch (k) {
    case K_UP: if (n) { u->sel[SCR_MESSAGES] = (u->sel[SCR_MESSAGES] + n - 1) % n; u->msg_scroll = 0; } return 1;
    case K_DOWN: if (n) { u->sel[SCR_MESSAGES] = (u->sel[SCR_MESSAGES] + 1) % n; u->msg_scroll = 0; } return 1;
    case K_PGDN: u->msg_scroll += 10; return 1;
    case K_PGUP: u->msg_scroll = u->msg_scroll > 10 ? u->msg_scroll - 10 : 0; return 1;
    case 'x': case 'X': case K_DEL:
        if (n) {
            int i = n - 1 - clampi(u->sel[SCR_MESSAGES], 0, n - 1);
            memmove(&g->msg[i], &g->msg[i + 1], sizeof(Message) * (size_t)(n - 1 - i));
            g->nmsg--;
            u->msg_scroll = 0;
        }
        return 1;
    case 'a': case 'A': for (int i = 0; i < n; i++) g->msg[i].read = 1; return 1;
    case 'd': case 'D': ui_confirm(u, MP_DELETE_MSGS, 0, "Apagar mensagens", "Apagar todas as mensagens já lidas?"); return 1;
    }
    return 0;
}

/* ===================== TECHNOLOGY ===================== */
static const struct { int from, to; const char *head; } TCATS[] = {
    {B_METAL_MINE, B_DEUT_TANK, "Recursos"}, {B_ROBOTICS, B_TRADE_HUB, "Instalações"}, {B_LUNAR_BASE, B_HE3, "Lua"},
    {B_HABITAT, B_ARCOLOGY, "Civilização"}, {T_ESPIONAGE, T_LOGISTICS, "Pesquisa"}, {S_SMALL_CARGO, S_CRAWLER, "Naves"},
    {D_ROCKET, D_IPM, "Defesa"}};

static int tech_rows(int *rows) {
    int n = 0;
    for (int c = 0; c < 7; c++) {
        rows[n++] = -1 - c;
        for (int id = TCATS[c].from; id <= TCATS[c].to; id++) rows[n++] = id;
    }
    return n;
}

static int tree_line;
static void tree_node(const Game *g, const Body *b, int id, int need, int depth, int x, int y0, int maxh, char *prefix, int last, int *seen) {
    if (tree_line >= maxh) return;
    int have = IS_TECH(id) ? g->tech[TI(id)] : IS_BLD(id) ? b->bld[id] : 0;
    char t[200];
    if (depth == 0) snprintf(t, sizeof t, "%s", ITEMS[id].name);
    else snprintf(t, sizeof t, "%s%s%s (%d/%d)", prefix, last ? "\u2514\u2500 " : "\u251C\u2500 ", ITEMS[id].name, have, need);
    int ok = depth == 0 ? reqs_met(g, b, id) : have >= need;
    cv_text_w(x, y0 + tree_line, 52, t, depth == 0 ? C_WHITE : ok ? C_GREEN : C_RED, COL_KEEP);
    if (depth > 0) {
        /* color prefix dim */
        int pl = utf8_len(prefix) + 3;
        for (int i = 0; i < pl && i < 52; i++) { Cell *c = cv_cell(x + i, y0 + tree_line); if (c) c->fg = C_DIM; }
    }
    tree_line++;
    if (depth > 6) return;
    if (seen[id] && depth > 0) return;
    seen[id] = 1;
    const ItemDef *d = &ITEMS[id];
    int nreq = 0;
    while (nreq < 6 && d->req[nreq].lvl > 0) nreq++;
    char np[120];
    snprintf(np, sizeof np, "%s%s", depth == 0 ? "" : prefix, depth == 0 ? "" : (last ? "   " : "\u2502  "));
    for (int i = 0; i < nreq; i++) tree_node(g, b, d->req[i].id, d->req[i].lvl, depth + 1, x, y0, maxh, np, i == nreq - 1, seen);
}

static void techinfo_table(const Game *g, const Body *b, int id, int x, int y, int w, int maxh) {
    (void)w;
    int L = IS_TECH(id) ? g->tech[TI(id)] : IS_BLD(id) ? b->bld[id] : 0;
    double tavg = (b->info.tmin + b->info.tmax) / 2.0;
    double sp = g->speed;
    int start = L > 2 ? L - 2 : 1;
    int prodkind = id == B_METAL_MINE || id == B_CRYSTAL_MINE || id == B_DEUT_SYNTH || id == B_SOLAR_PLANT || id == B_FUSION ||
                   id == B_FARM || id == B_HE3 || id == B_HABITAT || id == B_METAL_STORE || id == B_CRYSTAL_STORE || id == B_DEUT_TANK || id == B_FOOD_SYNTH;
    char a[32], c[32], d[32], e[32], f[32];
    if (prodkind) {
        const char *h1 = (id == B_SOLAR_PLANT || id == B_FUSION) ? "Energia" : (id >= B_METAL_STORE && id <= B_DEUT_TANK) ? "Capacidade" : id == B_HABITAT ? "Pop. máx" : "Produção/h";
        cv_textf(x, y, C_TITLE, COL_KEEP, "%-4s %-11s %-10s %-8s %-8s %-7s", "Nv", h1, "Diferença", "Dif/nv", "Energia", "Dif.");
        double prev = 0, prevd = 0, preve = 0;
        for (int r = 0; r < maxh - 1; r++) {
            int lv = start + r;
            double v, en = 0;
            switch (id) {
            case B_METAL_MINE: v = prod_metal_lvl(lv) * sp * b->info.rich[0]; en = -energy_for(id, lv); break;
            case B_CRYSTAL_MINE: v = prod_crystal_lvl(lv) * sp * b->info.rich[1]; en = -energy_for(id, lv); break;
            case B_DEUT_SYNTH: v = prod_deut_lvl(lv, tavg) * sp * b->info.rich[2]; en = -energy_for(id, lv); break;
            case B_SOLAR_PLANT: v = 20.0 * lv * pow(1.1, lv); break;
            case B_FUSION: v = 30.0 * lv * pow(1.05 + 0.01 * tech(g, T_ENERGY), lv); en = -10.0 * lv * pow(1.1, lv) * sp; break;
            case B_FARM: v = 25.0 * lv * pow(1.1, lv) * b->info.fertility * sp; en = -energy_for(id, lv); break;
            case B_FOOD_SYNTH: v = 40.0 * lv * pow(1.1, lv) * sp; en = -energy_for(id, lv); break;
            case B_HE3: v = 30.0 * lv * pow(1.1, lv) * sp; break;
            case B_HABITAT: v = 100.0 * lv * pow(1.12, lv); break;
            default: v = storage_cap(lv); break;
            }
            double base = 0;
            if (r == 0) {
                int pl2 = lv - 1;
                switch (id) {
                case B_METAL_MINE: base = prod_metal_lvl(pl2) * sp * b->info.rich[0]; preve = -energy_for(id, pl2); break;
                case B_CRYSTAL_MINE: base = prod_crystal_lvl(pl2) * sp * b->info.rich[1]; preve = -energy_for(id, pl2); break;
                case B_DEUT_SYNTH: base = prod_deut_lvl(pl2, tavg) * sp * b->info.rich[2]; preve = -energy_for(id, pl2); break;
                case B_SOLAR_PLANT: base = 20.0 * pl2 * pow(1.1, pl2); break;
                case B_FUSION: base = 30.0 * pl2 * pow(1.05 + 0.01 * tech(g, T_ENERGY), pl2); preve = -10.0 * pl2 * pow(1.1, pl2) * sp; break;
                case B_FARM: base = 25.0 * pl2 * pow(1.1, pl2) * b->info.fertility * sp; preve = -energy_for(id, pl2); break;
                case B_FOOD_SYNTH: base = 40.0 * pl2 * pow(1.1, pl2) * sp; preve = -energy_for(id, pl2); break;
                case B_HE3: base = 30.0 * pl2 * pow(1.1, pl2) * sp; break;
                case B_HABITAT: base = 100.0 * pl2 * pow(1.12, pl2); break;
                default: base = storage_cap(pl2); break;
                }
                prev = base;
                prevd = 0;
            }
            double diff = v - (start == 1 && r == 0 ? 0 : prev);
            if (r == 0) diff = v - prev;
            fmt_num(a, sizeof a, v); fmt_num(c, sizeof c, v - (r == 0 ? prev : prev));
            fmt_num(d, sizeof d, diff - (r == 0 ? diff : prevd));
            fmt_num(e, sizeof e, en); fmt_num(f, sizeof f, en - preve);
            uint32_t col = lv == L ? C_GOLD : lv == L + 1 ? C_GREEN : C_TEXT;
            snprintf(c, sizeof c, "%s", "");
            char diffs[32];
            fmt_num(diffs, sizeof diffs, v - prev);
            char dpl[32];
            fmt_num(dpl, sizeof dpl, (v - prev) - (r == 0 ? 0 : prevd));
            if (r == 0) snprintf(dpl, sizeof dpl, "%s", diffs);
            cv_textf(x, y + 1 + r, col, COL_KEEP, "%-4d %-11s %-10s %-8s %-8s %-7s", lv, a, diffs, dpl, en != 0 ? e : "-", en != 0 ? f : "-");
            prevd = v - prev;
            prev = v;
            preve = en;
        }
        return;
    }
    if (IS_BLD(id) || IS_TECH(id)) {
        cv_textf(x, y, C_TITLE, COL_KEEP, "%-5s %-11s %-11s %-11s %-12s", "Nível", "Metal", "Cristal", "Deutério", "Tempo");
        for (int r = 0; r < maxh - 1; r++) {
            int lv = start + r;
            if (ITEMS[id].maxlvl && lv > ITEMS[id].maxlvl) break;
            double cc[3];
            double en = item_cost1(id, lv, cc);
            fmt_num(a, sizeof a, cc[0]); fmt_num(c, sizeof c, cc[1]); fmt_num(d, sizeof d, cc[2]);
            double tm = IS_TECH(id) ? (b->is_moon ? 0 : research_time(g, g->cur, id, lv)) : build_time(g, b, id, lv);
            fmt_time(e, sizeof e, tm);
            uint32_t col = lv == L ? C_GOLD : lv == L + 1 ? C_GREEN : C_TEXT;
            cv_textf(x, y + 1 + r, col, COL_KEEP, "%-5d %-11s %-11s %-11s %-12s", lv, a, c, d, e);
            if (en > 0) { fmt_short(f, sizeof f, en); cv_textf(x + 52, y + 1 + r, C_GOLD, COL_KEEP, "%sE", f); }
        }
        return;
    }
    /* ships / defense */
    const ItemDef *it = &ITEMS[id];
    int yy = y;
    fmt_num(a, sizeof a, it->att * (1 + 0.1 * g->tech[TI(T_WEAPONS)]));
    fmt_num(c, sizeof c, it->shd * (1 + 0.1 * g->tech[TI(T_SHIELDING)]));
    fmt_num(d, sizeof d, it->si * (1 + 0.1 * g->tech[TI(T_ARMOR)]));
    cv_textf(x, yy++, C_CYAN, COL_KEEP, "Ataque %s   Escudo %s   Integridade %s", a, c, d);
    if (IS_SHIP(id)) {
        long long one[N_SHIP] = {0};
        one[SI(id)] = 1;
        fmt_num(a, sizeof a, fleet_cargo_cap(g, one));
        fmt_num(c, sizeof c, ship_speed(g, id));
        cv_textf(x, yy++, C_TEXT, COL_KEEP, "Carga %s   Velocidade %s   Consumo %.0f", a, c, it->fuel);
        static const char *DRV[4] = {"-", "Combustão", "Impulso", "Hiperespacial"};
        cv_textf(x, yy++, C_DIM, COL_KEEP, "Motor: %s", DRV[it->drive]);
    }
    yy++;
    int u0 = item_unit(id);
    cv_text(x, yy++, "Disparo rápido contra:", C_ORANGE, COL_KEEP);
    int any = 0;
    for (int v = 0; v < N_UNIT && u0 >= 0; v++) {
        int rf = rapidfire(u0, v);
        if (rf > 1 && yy < y + maxh) { cv_textf(x + 2, yy++, C_TEXT, COL_KEEP, "%-26s x%d", ITEMS[unit_item(v)].name, rf); any = 1; }
    }
    if (!any) cv_text(x + 2, yy++, "nenhum", C_DIM, COL_KEEP);
    cv_text(x, yy++, "Sofre disparo rápido de:", C_ORANGE, COL_KEEP);
    any = 0;
    for (int v = 0; v < N_UNIT && u0 >= 0; v++) {
        int rf = rapidfire(v, u0);
        if (rf > 1 && yy < y + maxh) { cv_textf(x + 2, yy++, C_TEXT, COL_KEEP, "%-26s x%d", ITEMS[unit_item(v)].name, rf); any = 1; }
    }
    if (!any && yy < y + maxh) cv_text(x + 2, yy++, "nenhum", C_DIM, COL_KEEP);
}

void scr_tech_draw(UI *u, Game *g) {
    Body *b = &g->body[g->cur];
    int rows[128];
    int n = tech_rows(rows);
    int id = u->tech_item;
    if (id < 0 || id >= ITEM_COUNT) id = u->tech_item = B_METAL_MINE;
    char t[160];
    snprintf(t, sizeof t, " Tecnologia - %s ", ITEMS[id].name);
    title_line(t, NULL);
    /* list */
    int lw = 25;
    ui_panel(CX, CY + 1, lw, CH - 2, NULL);
    int pos = 0;
    for (int i = 0; i < n; i++) if (rows[i] == id) pos = i;
    int vis = CH - 4;
    int top = u->scroll[SCR_TECH];
    if (pos < top + 1) top = pos - 1;
    if (pos >= top + vis) top = pos - vis + 1;
    if (top < 0) top = 0;
    u->scroll[SCR_TECH] = top;
    for (int k = 0; k < vis && top + k < n; k++) {
        int r = rows[top + k];
        int y = CY + 2 + k;
        if (r < 0) { cv_textf(CX + 2, y, C_TITLE, C_PANEL, "\u2500 %s", TCATS[-r - 1].head); continue; }
        uint32_t bg = r == id ? C_SEL : C_PANEL;
        cv_fill(CX + 1, y, lw - 2, 1, ' ', C_TEXT, bg);
        int ok = reqs_met(g, b, r);
        cv_text_w(CX + 3, y, lw - 5, ITEMS[r].name, ok ? C_TEXT : C_DIM, bg);
    }
    /* tabs */
    int px = CX + lw, pw = CW - lw;
    ui_panel(px, CY + 1, pw, CH - 2, NULL);
    static const char *TABS[4] = {"Árvore", "Aplicações", "Info técnica", "Requisitos"};
    int tx = px + 2;
    for (int i = 0; i < 4; i++) {
        int sel = u->tab[SCR_TECH] == i;
        char lb[32];
        snprintf(lb, sizeof lb, " %s ", TABS[i]);
        cv_text(tx, CY + 1, lb, sel ? RGB(20, 20, 30) : C_DIM, sel ? C_GOLD : C_PANEL);
        tx += utf8_len(lb) + 1;
    }
    int cx = px + 2, cy = CY + 3, ch = CH - 5;
    switch (u->tab[SCR_TECH]) {
    case 0: {
        int seen[ITEM_COUNT];
        memset(seen, 0, sizeof seen);
        tree_line = 0;
        char pre[4] = "";
        tree_node(g, b, id, 0, 0, cx, cy, ch, pre, 1, seen);
        if (tree_line == 1) cv_text(cx, cy + 2, "Sem requisitos: pode ser construído desde o início.", C_GREEN, COL_KEEP);
    } break;
    case 1: {
        int y = cy;
        cv_text(cx, y++, "Este item é requisito para:", C_TITLE, COL_KEEP);
        int any = 0;
        for (int o = 0; o < ITEM_COUNT && y < cy + ch; o++) {
            const ItemDef *d = &ITEMS[o];
            for (int i = 0; i < 6 && d->req[i].lvl > 0; i++) {
                if (d->req[i].id != id) continue;
                int have = req_have(g, b, id);
                cv_textf(cx + 2, y++, have >= d->req[i].lvl ? C_GREEN : C_TEXT, COL_KEEP, "%-30s (precisa nível %d)%s", d->name, d->req[i].lvl, reqs_met(g, b, o) ? "  \u2713 liberado" : "");
                any = 1;
            }
        }
        if (!any) cv_text(cx + 2, y, "Nada depende deste item.", C_DIM, COL_KEEP);
    } break;
    case 2: {
        int lines = cv_wrap(cx, cy, pw - 4, 3, ITEMS[id].desc, C_TEXT, COL_KEEP);
        int used = lines > 3 ? 3 : lines;
        techinfo_table(g, b, id, cx, cy + used + 1, pw - 4, ch - used - 1);
    } break;
    case 3: {
        int y = cy;
        const ItemDef *d = &ITEMS[id];
        cv_text(cx, y++, "Requisitos diretos:", C_TITLE, COL_KEEP);
        int any = 0;
        for (int i = 0; i < 6 && d->req[i].lvl > 0; i++) {
            int have = req_have(g, b, d->req[i].id);
            cv_textf(cx + 2, y++, have >= d->req[i].lvl ? C_GREEN : C_RED, COL_KEEP, "%-30s nível %d (você: %d)", ITEMS[d->req[i].id].name, d->req[i].lvl, have);
            any = 1;
        }
        if (!any) cv_text(cx + 2, y++, "nenhum", C_GREEN, COL_KEEP);
        y++;
        static const char *WH[4] = {"", "Planetas", "Luas", "Planetas e luas"};
        cv_textf(cx, y++, C_DIM, COL_KEEP, "Construível em: %s", WH[d->where & 3]);
        int L = IS_TECH(id) ? g->tech[TI(id)] : IS_BLD(id) ? b->bld[id] : 0;
        if (IS_BLD(id) || IS_TECH(id)) {
            double c[3];
            double en = item_cost1(id, L + 1, c);
            cv_textf(cx, y++, C_TEXT, COL_KEEP, "Nível atual %d. Custo do nível %d:", L, L + 1);
            draw_cost(cx + 2, y++, g, b, c, en);
        } else {
            double c[3];
            item_cost1(id, 1, c);
            cv_text(cx, y++, "Custo por unidade:", C_TEXT, COL_KEEP);
            draw_cost(cx + 2, y++, g, b, c, 0);
        }
        y++;
        cv_wrap(cx, y, pw - 4, 6, d->desc, C_DIM, COL_KEEP);
    } break;
    }
    hint("\u2191\u2193 item  \u2190\u2192/Tab abas  Enter ir para onde se constrói este item");
}

int scr_tech_key(UI *u, Game *g, int k) {
    int rows[128];
    int n = tech_rows(rows);
    int pos = 0;
    for (int i = 0; i < n; i++) if (rows[i] == u->tech_item) pos = i;
    if (k == K_UP || k == K_DOWN || k == K_PGUP || k == K_PGDN) {
        int step = (k == K_UP || k == K_PGUP) ? -1 : 1;
        int times = (k == K_PGUP || k == K_PGDN) ? 10 : 1;
        for (int t = 0; t < times; t++) {
            do { pos = (pos + step + n) % n; } while (rows[pos] < 0);
        }
        u->tech_item = rows[pos];
        return 1;
    }
    if (k == K_LEFT) { u->tab[SCR_TECH] = (u->tab[SCR_TECH] + 3) % 4; return 1; }
    if (k == K_RIGHT || k == K_TAB) { u->tab[SCR_TECH] = (u->tab[SCR_TECH] + 1) % 4; return 1; }
    if (k == K_ENTER) {
        int id = u->tech_item;
        const ItemDef *d = &ITEMS[id];
        if (d->cat == CAT_RESOURCE) { u->screen = SCR_RESOURCES; u->sel[SCR_RESOURCES] = id - B_METAL_MINE; }
        else if (d->cat == CAT_FACILITY) { u->screen = SCR_FACILITIES; u->sel[SCR_FACILITIES] = id - B_ROBOTICS; }
        else if (d->cat == CAT_MOON) { u->screen = SCR_FACILITIES; }
        else if (d->cat == CAT_LIFE) { u->screen = SCR_LIFE; u->sel[SCR_LIFE] = id - B_HABITAT; }
        else if (d->cat == CAT_RESEARCH) {
            static const int RO[] = {T_ENERGY, T_LASER, T_ION, T_HYPERSPACE, T_PLASMA, T_COMBUSTION, T_IMPULSE, T_HYPERDRIVE, T_ESPIONAGE, T_COMPUTER,
                                     T_ASTRO, T_IRN, T_GRAVITON, T_WEAPONS, T_SHIELDING, T_ARMOR, T_BIOTECH, T_MEDICINE, T_GEOPHYSICS, T_ORBITAL_ENG, T_LOGISTICS};
            for (int i = 0; i < 21; i++) if (RO[i] == id) u->sel[SCR_RESEARCH] = i;
            u->screen = SCR_RESEARCH;
        } else if (d->cat == CAT_SHIP) { u->screen = SCR_SHIPYARD; u->sel[SCR_SHIPYARD] = SI(id); }
        else { u->screen = SCR_DEFENSE; u->sel[SCR_DEFENSE] = DI(id); }
        (void)g;
        return 1;
    }
    return 0;
}

/* ===================== OFFICERS ===================== */
static const struct { const char *name; const char *desc; double cost; } OFFS[N_OFF] = {
    {"Comandante", "Fila de construção e pesquisa com 5 vagas (em vez de 2).", 2500},
    {"Almirante", "+2 slots de frota e +1 slot de expedição.", 5000},
    {"Engenheiro", "+10% de produção de energia em todos os planetas.", 5000},
    {"Geólogo", "+10% de produção das minas em todos os planetas.", 5000},
    {"Tecnocrata", "-25% no tempo de todas as pesquisas.", 5000},
};

void scr_officers_draw(UI *u, Game *g) {
    char r[64];
    snprintf(r, sizeof r, "Matéria Negra: %.0f", g->dm);
    title_line(" Recrutar Oficiais (7 dias reais) ", r);
    int sel = clampi(u->sel[SCR_OFFICERS], 0, N_OFF - 1);
    for (int i = 0; i < N_OFF; i++) {
        int y = CY + 1 + i * 6;
        uint32_t bg = i == sel ? C_SEL : C_PANEL;
        cv_fill(CX, y, CW, 5, ' ', C_TEXT, bg);
        cv_box(CX, y, CW, 5, i == sel ? C_GOLD : C_BORDER, bg, 2);
        art_officer(CX + 2, y + 1, i);
        cv_text(CX + 12, y + 1, OFFS[i].name, C_GOLD, bg);
        cv_bold_rect(CX + 12, y + 1, 12, 1);
        cv_text(CX + 12, y + 2, OFFS[i].desc, C_TEXT, bg);
        if (officer_active(g, i)) {
            char tm[32];
            fmt_time(tm, sizeof tm, g->officer[i] - g->now);
            cv_textf(CX + 12, y + 3, C_GREEN, bg, "ATIVO - restam %s", tm);
        } else cv_text(CX + 12, y + 3, "inativo", C_DIM, bg);
        cv_textf(CX + 58, y + 3, g->dm >= OFFS[i].cost ? C_PURPLE : C_RED, bg, "%.0f Matéria Negra", OFFS[i].cost);
    }
    hint("\u2191\u2193 escolher oficial  Enter contratar/estender (+7 dias)  Matéria Negra vem de expedições, eventos e missões");
}

int scr_officers_key(UI *u, Game *g, int k) {
    if (k == K_UP) { u->sel[SCR_OFFICERS] = (u->sel[SCR_OFFICERS] + N_OFF - 1) % N_OFF; return 1; }
    if (k == K_DOWN) { u->sel[SCR_OFFICERS] = (u->sel[SCR_OFFICERS] + 1) % N_OFF; return 1; }
    if (k == K_ENTER) {
        int i = clampi(u->sel[SCR_OFFICERS], 0, N_OFF - 1);
        ui_confirm(u, MP_OFFICER, i, "Contratar oficial", "Contratar %s por 7 dias por %.0f Matéria Negra? (você tem %.0f)", OFFS[i].name, OFFS[i].cost, g->dm);
        return 1;
    }
    return 0;
}

/* ===================== RANKING ===================== */
void scr_ranking_draw(UI *u, Game *g) {
    static int order[N_NPC];
    static double pts[N_NPC];
    int n = npc_rank_list(g, order, pts);
    double me = player_points(g, NULL, NULL, NULL, NULL);
    int total = n + 1;
    int myrank = 1;
    for (int i = 0; i < n; i++) if (pts[i] > me) myrank++;
    char r[80];
    snprintf(r, sizeof r, "Você: %d de %d", myrank, total);
    title_line(" Classificação ", r);
    int rows = CH - 3;
    int top = u->scroll[SCR_RANKING];
    if (top < 0) top = 0;
    if (top > total - rows) top = total - rows > 0 ? total - rows : 0;
    u->scroll[SCR_RANKING] = top;
    cv_fill(CX, CY + 1, CW, 1, ' ', C_TEXT, C_PANEL2);
    cv_text(CX + 1, CY + 1, "Lugar  Jogador                      Aliança   Perfil        Pontos", C_TITLE, C_PANEL2);
    for (int k = 0; k < rows && top + k < total; k++) {
        int rank = top + k + 1;
        int y = CY + 2 + k;
        int isme = rank == myrank;
        int ni = rank < myrank ? rank - 1 : rank - 2;
        uint32_t bg = isme ? RGB(30, 60, 40) : (k & 1 ? C_PANEL : RGB(16, 23, 39));
        cv_fill(CX, y, CW, 1, ' ', C_TEXT, bg);
        char p[32], t[160];
        if (isme) {
            fmt_num(p, sizeof p, me);
            snprintf(t, sizeof t, "%5d  %-28.28s %-9s %-13s %s", rank, g->name, "-", "Você", p);
            cv_text(CX + 1, y, t, C_GREEN, bg);
        } else {
            const Npc *nn = &g->npc[order[ni]];
            fmt_num(p, sizeof p, pts[ni]);
            snprintf(t, sizeof t, "%5d  %-28.28s %-9s %-13s %s", rank, nn->name, nn->tag, npc_type_name(nn->type), p);
            cv_text(CX + 1, y, t, rank <= 3 ? C_GOLD : nn->type == NPC_INACTIVE ? C_DIM : C_TEXT, bg);
        }
    }
    hint("\u2191\u2193 PgUp/PgDn rolar  Home sua posição  (Piratas e Xenos não entram no ranking)");
}

int scr_ranking_key(UI *u, Game *g, int k) {
    switch (k) {
    case K_UP: u->scroll[SCR_RANKING]--; return 1;
    case K_DOWN: u->scroll[SCR_RANKING]++; return 1;
    case K_PGUP: u->scroll[SCR_RANKING] -= CH - 4; return 1;
    case K_PGDN: u->scroll[SCR_RANKING] += CH - 4; return 1;
    case K_HOME: {
        int tot;
        int r = player_rank(g, &tot);
        u->scroll[SCR_RANKING] = r - (CH - 3) / 2;
        return 1;
    }
    }
    return 0;
}

/* ===================== SIMULATOR ===================== */
void scr_sim_draw(UI *u, Game *g) {
    (void)g;
    title_line(" Simulador de Combate ", "média de 20 batalhas");
    int sel = clampi(u->sel[SCR_SIM], 0, N_UNIT + 2);
    u->sel[SCR_SIM] = sel;
    cv_fill(CX, CY + 1, CW, 1, ' ', C_TEXT, C_PANEL2);
    cv_text(CX + 2, CY + 1, "Unidade", C_TITLE, C_PANEL2);
    cv_text(CX + 30, CY + 1, "Atacante", u->sim_side == 0 ? C_GOLD : C_TITLE, C_PANEL2);
    cv_text(CX + 42, CY + 1, "Defensor", u->sim_side == 1 ? C_GOLD : C_TITLE, C_PANEL2);
    static const char *TN[3] = {"Tec. Armas", "Tec. Escudos", "Blindagem"};
    int y = CY + 2;
    for (int r = 0; r < N_UNIT + 3; r++, y++) {
        uint32_t bg = r == sel ? C_SEL : (r & 1 ? C_PANEL : RGB(16, 23, 39));
        cv_fill(CX, y, 52, 1, ' ', C_TEXT, bg);
        const char *nm = r < 3 ? TN[r] : ITEMS[unit_item(r - 3)].name;
        cv_text_w(CX + 2, y, 27, nm, r < 3 ? C_CYAN : C_TEXT, bg);
        for (int s = 0; s < 2; s++) {
            char a[24];
            if (r < 3) snprintf(a, sizeof a, "%d", u->sim_tech[s][r]);
            else if (s == 0 && r - 3 >= N_SHIP) snprintf(a, sizeof a, "-");
            else fmt_short(a, sizeof a, (double)u->sim_n[s][r - 3]);
            uint32_t col = (r == sel && u->sim_side == s) ? C_GOLD : C_TEXT;
            cv_text_r(CX + 37 + s * 12, y, a, col, bg);
        }
    }
    ui_panel(CX + 53, CY + 1, CW - 53, CH - 2, "Resultado");
    if (u->sim_result[0]) cv_wrap(CX + 55, CY + 3, CW - 57, CH - 6, u->sim_result, C_TEXT, C_PANEL);
    else cv_wrap(CX + 55, CY + 3, CW - 57, CH - 6, "Configure as forças e pressione S para simular.\n\nP: carrega naves e defesas do planeta atual como Defensor.\nF: carrega suas naves como Atacante.\nX: limpa tudo.", C_DIM, C_PANEL);
    hint("\u2191\u2193 unidade  \u2190\u2192 lado  Enter quantidade  S simular  P/F carregar planeta  X limpar");
}

int scr_sim_key(UI *u, Game *g, int k) {
    Body *b = &g->body[g->cur];
    int n = N_UNIT + 3;
    switch (k) {
    case K_UP: u->sel[SCR_SIM] = (u->sel[SCR_SIM] + n - 1) % n; return 1;
    case K_DOWN: u->sel[SCR_SIM] = (u->sel[SCR_SIM] + 1) % n; return 1;
    case K_PGUP: u->sel[SCR_SIM] = clampi(u->sel[SCR_SIM] - 8, 0, n - 1); return 1;
    case K_PGDN: u->sel[SCR_SIM] = clampi(u->sel[SCR_SIM] + 8, 0, n - 1); return 1;
    case K_LEFT: case K_RIGHT: u->sim_side ^= 1; return 1;
    case K_ENTER: {
        int r = u->sel[SCR_SIM];
        if (u->sim_side == 0 && r - 3 >= N_SHIP) return 1;
        char init[24];
        snprintf(init, sizeof init, "%lld", r < 3 ? (long long)u->sim_tech[u->sim_side][r] : u->sim_n[u->sim_side][r - 3]);
        ui_prompt(u, MP_SIM_QTY, r, "Simulador", "Quantidade / nível:", init, 1);
        return 1;
    }
    case 'p': case 'P':
        for (int i = 0; i < N_SHIP; i++) u->sim_n[1][i] = b->ship[i];
        for (int i = 0; i < N_CDEF; i++) u->sim_n[1][N_SHIP + i] = b->def[i];
        u->sim_tech[1][0] = g->tech[TI(T_WEAPONS)]; u->sim_tech[1][1] = g->tech[TI(T_SHIELDING)]; u->sim_tech[1][2] = g->tech[TI(T_ARMOR)];
        return 1;
    case 'f': case 'F':
        for (int i = 0; i < N_SHIP; i++) u->sim_n[0][i] = (T_END + i == S_SATELLITE || T_END + i == S_CRAWLER) ? 0 : b->ship[i];
        u->sim_tech[0][0] = g->tech[TI(T_WEAPONS)]; u->sim_tech[0][1] = g->tech[TI(T_SHIELDING)]; u->sim_tech[0][2] = g->tech[TI(T_ARMOR)];
        return 1;
    case 'x': case 'X': memset(u->sim_n, 0, sizeof u->sim_n); memset(u->sim_tech, 0, sizeof u->sim_tech); u->sim_result[0] = 0; return 1;
    case 's': case 'S': {
        CombatIn in;
        memset(&in, 0, sizeof in);
        for (int s = 0; s < 2; s++) {
            for (int i = 0; i < N_UNIT; i++) in.n[s][i] = u->sim_n[s][i];
            in.wt[s] = u->sim_tech[s][0]; in.st[s] = u->sim_tech[s][1]; in.at[s] = u->sim_tech[s][2];
        }
        for (int i = N_SHIP; i < N_UNIT; i++) in.n[0][i] = 0;
        Rng r;
        rng_seed(&r, (uint64_t)(g->now * 1000));
        int wins[3] = {0, 0, 0};
        double lost[2] = {0, 0}, deb = 0, rounds = 0;
        const int RUNS = 20;
        for (int k2 = 0; k2 < RUNS; k2++) {
            CombatOut o;
            combat_run(&r, &in, &o);
            wins[o.winner == 0 ? 0 : o.winner == 1 ? 1 : 2]++;
            lost[0] += o.lost_value[0]; lost[1] += o.lost_value[1];
            deb += o.debris[0] + o.debris[1];
            rounds += o.rounds;
        }
        char a[32], c[32], d[32];
        fmt_num(a, sizeof a, lost[0] / RUNS); fmt_num(c, sizeof c, lost[1] / RUNS); fmt_num(d, sizeof d, deb / RUNS);
        snprintf(u->sim_result, sizeof u->sim_result,
                 "\001wVitória do atacante:\001d %d%%\n\001wVitória do defensor:\001d %d%%\n\001wEmpate:\001d %d%%\n\nRodadas médias: %.1f\n\n"
                 "Perdas médias:\n  atacante %s\n  defensor %s\n\nDestroços médios: %s\n(30%% de metal e cristal das naves destruídas; defesas são 70%% reparadas)",
                 wins[0] * 100 / RUNS, wins[1] * 100 / RUNS, wins[2] * 100 / RUNS, rounds / RUNS, a, c, d);
        return 1;
    }
    }
    return 0;
}

/* ===================== HELP ===================== */
static const char *HELP[] = {
    "\001yCOMO TUDO SE CONECTA\001d\n\n"
    "Minas produzem \001wmetal, cristal e deutério\001d por segundo, em tempo real (mesmo com o jogo fechado). "
    "Elas precisam de \001yenergia\001d (Usina Solar, Reator de Fusão, Satélites) e de \001wtrabalhadores\001d.\n\n"
    "Trabalhadores vêm da \001wpopulação\001d, que precisa de \001gmoradia\001d (Setor Residencial) e de \001galimento\001d "
    "(Fazendas, que consomem energia e dependem da fertilidade do planeta). Sem comida a população diminui e pode se revoltar.\n\n"
    "Se faltar energia ou trabalhadores, as minas e as instalações perdem eficiência: obras e pesquisas também ficam mais lentas.\n\n"
    "Pesquisas liberam construções, naves e defesas. Veja a árvore completa em \001cT - Tecnologia\001d.\n\n"
    "A Estação Orbital abre a era espacial: Astrofísica (colônias e expedições) e Engenharia Orbital (megaestruturas).",
    "\001yTECLAS\001d\n\n"
    "1 Visão Geral   2 Recursos   3 Civilização   4 Instalações   5 Pesquisa\n"
    "6 Estaleiro     7 Defesa     8 Frota         9 Galáxia       0 Império\n"
    "C Comércio   E Mercador   M Mensagens   T Tecnologia   O Oficiais\n"
    "R Classificação   Z Simulador   H Ajuda   Q salvar e sair\n\n"
    "[ ] ou , . trocar de planeta     L alternar planeta/lua     V evento pendente\n"
    "Esc volta à Visão Geral.\n\n"
    "Nas listas: \u2191\u2193 escolhe, Enter constrói, I abre a info técnica, X cancela, F finaliza com Matéria Negra.\n"
    "Em Recursos, P muda a porcentagem de produção (ex.: desligar uma mina para sobrar energia).",
    "\001yFROTAS E COMBATE\001d\n\n"
    "Missões: Atacar, Transportar, Estacionar, Espionar, Colonizar, Reciclar, Expedição (posição 16) e Explorar.\n\n"
    "O combate segue as regras clássicas: até 6 rodadas, cada unidade atira em um alvo aleatório, escudos absorvem dano, "
    "unidades danificadas podem explodir, e disparo rápido permite tiros extras. 30% do metal/cristal das naves destruídas "
    "vira campo de destroços (recicle-o!). Defesas destruídas têm 70% de chance de reparo.\n\n"
    "Frotas em voo não podem ser atacadas: quando um ALERTA de frota hostil aparecer, você pode enviar suas naves em missão (fleetsave), "
    "construir defesas ou gastar recursos antes da chegada.\n\nUse o Simulador (Z) para testar estratégias.",
    "\001yEVENTOS, DADOS E AMEAÇA\001d\n\n"
    "Após a proteção de novato (250 pontos), o universo reage ao seu crescimento: sondas espionam, piratas e xenos atacam, "
    "rotas são emboscadas.\n\nDesastres naturais dependem do planeta: vulcânicos tremem, mundos sem atmosfera sofrem com meteoros, "
    "planetas quentes e de magnetosfera fraca sofrem tempestades solares, selvas propagam epidemias.\n\n"
    "Muitos eventos pedem uma \001wdecisão\001d: gastar recursos para ganhar bônus, aceitar um efeito garantido ou arriscar. "
    "O teste é \001yd20 + modificadores\001d (tecnologias e construções) contra uma Classe de Dificuldade. 20 natural é crítico, 1 é desastre. "
    "Você pode rolar de novo com Matéria Negra.\n\nO indicador de Ameaça (esquerda) compara seus recursos e pontos com suas defesas.",
    "\001yCOLÔNIAS, LUAS E COMÉRCIO\001d\n\n"
    "Explore posições vazias (sonda/desbravador) para ver bioma, temperatura, fertilidade e riqueza. Colonize com a Nave Colonizadora. "
    "Posições 1-3 são quentes (mais cristal e energia solar), 6-10 rendem mais metal, 13-15 são frias (mais deutério).\n\n"
    "Especialize colônias e ligue-as com \001wrotas comerciais\001d internas (ex.: alimento de um mundo fértil para um árido). "
    "Mundos \001yMercantes ($)\001d compram e vendem recursos com taxas que flutuam. Piratas emboscam comboios: escolte-os com naves militares.\n\n"
    "Luas nascem de grandes batalhas (ou já existem). Nelas: Base Lunar, Falange de Sensores (aviso antecipado), Portal de Salto, Extrator de Hélio-3.\n\n"
    "Outras galáxias exigem Propulsor Hiperespacial 3.",
};
#define NHELP ((int)(sizeof HELP / sizeof HELP[0]))

void scr_help_draw(UI *u, Game *g) {
    (void)g;
    int p = clampi(u->sel[SCR_HELP], 0, NHELP - 1);
    char r[32];
    snprintf(r, sizeof r, "página %d/%d", p + 1, NHELP);
    title_line(" Ajuda ", r);
    ui_panel(CX, CY + 1, CW, CH - 2, NULL);
    cv_wrap(CX + 3, CY + 3, CW - 6, CH - 6, HELP[p], C_TEXT, C_PANEL);
    hint("\u2190\u2192 páginas   Esc voltar");
}

int scr_help_key(UI *u, Game *g, int k) {
    (void)g;
    if (k == K_LEFT || k == K_UP) { u->sel[SCR_HELP] = (u->sel[SCR_HELP] + NHELP - 1) % NHELP; return 1; }
    if (k == K_RIGHT || k == K_DOWN || k == ' ' || k == K_ENTER) { u->sel[SCR_HELP] = (u->sel[SCR_HELP] + 1) % NHELP; return 1; }
    return 0;
}

/* ===================== MODAL RESULTS ===================== */
void modal_result_screens(UI *u, Game *g, int p, int arg, int ok, const char *input, int choice) {
    (void)ok;
    Body *b = &g->body[g->cur];
    switch (p) {
    case MP_GAL_ACTION: {
        int pos = arg;
        int tg = u->gg, ts = u->gs;
        switch (choice) {
        case 0: {
            long long probes = b->ship[SI(S_PROBE)];
            if (probes <= 0) { ui_toast(u, C_RED, "Nenhuma sonda neste planeta (Estaleiro)."); return; }
            long long s[N_SHIP] = {0};
            s[SI(S_PROBE)] = probes < 3 ? probes : 3;
            char err[160];
            if (fleet_launch(g, g->cur, M_ESPIONAGE, tg, ts, pos, TT_PLANET, s, NULL, 100, err, sizeof err)) ui_toast(u, C_CYAN, "%lld sonda(s) enviada(s) para [%d:%d:%d]", s[SI(S_PROBE)], tg, ts, pos);
            else ui_toast(u, C_RED, "%s", err);
        } break;
        case 1: dispatch_open(u, g, tg, ts, pos, TT_PLANET, M_ATTACK); break;
        case 2: dispatch_open(u, g, tg, ts, pos, TT_PLANET, M_TRANSPORT); break;
        case 3: dispatch_open(u, g, tg, ts, pos, TT_PLANET, M_DEPLOY); break;
        case 4: dispatch_open(u, g, tg, ts, pos, TT_PLANET, M_COLONIZE); break;
        case 5: dispatch_open(u, g, tg, ts, pos, TT_PLANET, M_EXPLORE); break;
        case 6: dispatch_open(u, g, tg, ts, pos, TT_DEBRIS, M_RECYCLE); break;
        case 7: dispatch_open(u, g, tg, ts, 16, TT_PLANET, M_EXPEDITION); break;
        case 8: route_wizard_open(u, g, RT_NPC, npc_planet_at(g, tg, ts, pos)); break;
        case 9: {
            char txt[200];
            snprintf(txt, sizeof txt, "Quantos mísseis interplanetários disparar contra [%d:%d:%d]?\nVocê tem %lld. Alcance: %d sistemas.", tg, ts, pos, b->def[DI(D_IPM)], ipm_range(g));
            ui_prompt(u, MP_IPM_QTY, npc_planet_at(g, tg, ts, pos), "Mísseis interplanetários", txt, "1", 1);
        } break;
        }
    } break;
    case MP_GOTO: {
        int a = 0, c = 0;
        if (sscanf(input, "%d:%d", &a, &c) == 2 || sscanf(input, "%d %d", &a, &c) == 2) {
            u->gg = clampi(a, 1, UNI_GALAXIES);
            u->gs = clampi(c, 1, UNI_SYSTEMS);
        }
    } break;
    case MP_DELETE_ROUTE: route_delete(g, arg); ui_toast(u, C_ORANGE, "Rota desfeita."); break;
    case MP_IPM_QTY: {
        long long n = (input[0] == 'M' || input[0] == 'm') ? b->def[DI(D_IPM)] : atoll(input);
        char err[200];
        ui_toast(u, ipm_launch(g, g->cur, arg, n, err, sizeof err) ? C_GREEN : C_RED, "%s", err);
    } break;
    case MP_JUMP: {
        int k = 0, to = -1;
        for (int i = 0; i < g->nbody; i++)
            if (g->body[i].used && g->body[i].is_moon && i != g->cur && g->body[i].bld[B_JUMP_GATE] > 0) { if (k == choice) to = i; k++; }
        char err[160];
        ui_toast(u, jump_gate(g, g->cur, to, err, sizeof err) ? C_GREEN : C_RED, "%s", err);
    } break;
    case MP_OFFICER:
        if (g->dm < OFFS[arg].cost) { ui_toast(u, C_RED, "Matéria Negra insuficiente"); return; }
        g->dm -= OFFS[arg].cost;
        g->officer[arg] = (g->officer[arg] > g->now ? g->officer[arg] : g->now) + 7 * 86400.0;
        ui_toast(u, C_GOLD, "%s contratado!", OFFS[arg].name);
        break;
    case MP_SIM_QTY: {
        long long v = atoll(input);
        if (v < 0) v = 0;
        if (arg < 3) u->sim_tech[u->sim_side][arg] = (int)(v > 40 ? 40 : v);
        else u->sim_n[u->sim_side][arg - 3] = v;
    } break;
    case MP_MER_AMOUNT: {
        double amt = (input[0] == 'M' || input[0] == 'm') ? floor(b->res[u->mer_from]) : (double)atoll(input);
        char err[160];
        if (merchant_trade(g, g->cur, u->mer_from, u->mer_to, amt, err, sizeof err)) ui_toast(u, C_GREEN, "%s", err);
        else ui_toast(u, C_RED, "%s", err);
    } break;
    case MP_DELETE_MSGS: {
        int w = 0;
        for (int i = 0; i < g->nmsg; i++) if (!g->msg[i].read) g->msg[w++] = g->msg[i];
        g->nmsg = w;
        u->sel[SCR_MESSAGES] = 0;
    } break;
    }
}
