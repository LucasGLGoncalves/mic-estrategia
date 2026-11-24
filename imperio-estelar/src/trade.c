/* trade.c - trade routes (internal and with NPC trade worlds) and the merchant */
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const double RES_VALUE[NRES] = {1.0, 1.5, 3.0, 0.6};

double route_capacity(const Game *g, const Route *r) { return fleet_cargo_cap(g, r->ship); }

static void route_target(const Game *g, const Route *r, int *tg, int *ts, int *tp) {
    if (r->kind == RT_INTERNAL && r->dest >= 0 && r->dest < g->nbody) {
        *tg = g->body[r->dest].g; *ts = g->body[r->dest].s; *tp = g->body[r->dest].p;
    } else { *tg = r->tg; *ts = r->ts; *tp = r->tp; }
}

double route_trip_time(const Game *g, const Route *r) {
    const Body *o = &g->body[r->origin];
    int tg, ts, tp;
    route_target(g, r, &tg, &ts, &tp);
    double d = fleet_distance(o->g, o->s, o->p, tg, ts, tp);
    double slow = fleet_slowest(g, r->ship);
    if (slow <= 0) return 3600;
    return flight_duration(g, d, slow, 100);
}

static double route_fuel(const Game *g, const Route *r) {
    const Body *o = &g->body[r->origin];
    int tg, ts, tp;
    route_target(g, r, &tg, &ts, &tp);
    double d = fleet_distance(o->g, o->s, o->p, tg, ts, tp);
    return 2.0 * fleet_fuel(g, r->ship, d, 100, o);
}

double route_risk(const Game *g, const Route *r) {
    const Body *o = &g->body[r->origin];
    int tg, ts, tp;
    route_target(g, r, &tg, &ts, &tp);
    double risk = 0.03;
    if (tg != o->g) risk += 0.06;
    else risk += 0.0008 * abs(ts - o->s);
    if (r->kind == RT_NPC) risk += 0.02;
    risk *= 1.0 - 0.05 * g->tech[TI(T_LOGISTICS)];
    long long esc[N_SHIP];
    memcpy(esc, r->ship, sizeof esc);
    esc[SI(S_SMALL_CARGO)] = esc[SI(S_LARGE_CARGO)] = esc[SI(S_RECYCLER)] = esc[SI(S_PROBE)] = esc[SI(S_COLONY)] = 0;
    double ev = items_value(esc, NULL);
    double cv = route_capacity(g, r) * 0.35 + 5000;
    double deter = 1.0 - ev / (2.0 * cv);
    risk *= clampd(deter, 0.3, 1.0);
    return clampd(risk, 0.005, 0.35);
}

int routes_from(const Game *g, int body) {
    int n = 0;
    for (int i = 0; i < MAX_ROUTES; i++) if (g->route[i].used && g->route[i].origin == body) n++;
    return n;
}

double market_rate(const Game *g, int npc, int from, int to) {
    uint64_t h = hash64((uint64_t)npc * 7919u + g->seed);
    int wants = (int)(h % 4), sells = (int)((h >> 8) % 4);
    double days = (g->now - g->created) * g->speed / 86400.0;
    double fl = 1.0 + 0.15 * noise1(h, days * 0.7) + 0.05 * sin(days * 2.1 + (double)(h & 255));
    double r = RES_VALUE[from] / RES_VALUE[to];
    if (from == wants) r *= 1.25;
    if (to == sells) r *= 1.20;
    return r * 0.9 * fl;
}

double merchant_rate(int from, int to) { return RES_VALUE[from] / RES_VALUE[to]; }

int merchant_trade(Game *g, int bi, int from, int to, double amount, char *err, size_t n) {
    Body *b = &g->body[bi];
    if (from == to) { snprintf(err, n, "Escolha recursos diferentes"); return 0; }
    if (amount <= 0) { snprintf(err, n, "Quantidade inválida"); return 0; }
    if (b->res[from] + 1e-6 < amount) { snprintf(err, n, "Você não tem %s suficiente", res_name(from)); return 0; }
    double tax = g->ev.merchant_until > g->now ? 0.0 : 0.10;
    double get = floor(amount * merchant_rate(from, to) * (1.0 - tax));
    b->res[from] -= amount;
    b->res[to] += get;
    g->flags |= FL_TRADED;
    if (err) snprintf(err, n, "Trocado: %.0f %s -> %.0f %s", amount, res_name(from), get, res_name(to));
    return 1;
}

int route_create(Game *g, const Route *t, char *err, size_t n) {
    if (t->origin < 0 || t->origin >= g->nbody || !g->body[t->origin].used) { snprintf(err, n, "Origem inválida"); return 0; }
    Body *o = &g->body[t->origin];
    if (g->tech[TI(T_LOGISTICS)] < 1) { snprintf(err, n, "Exige Logística 1"); return 0; }
    if (o->bld[B_TRADE_HUB] <= routes_from(g, t->origin)) { snprintf(err, n, "Porto Comercial insuficiente (1 rota por nível)"); return 0; }
    if (t->kind == RT_INTERNAL) {
        if (t->dest < 0 || t->dest >= g->nbody || !g->body[t->dest].used || t->dest == t->origin) { snprintf(err, n, "Destino inválido"); return 0; }
        if (g->body[t->dest].g != o->g && !intergalactic_ok(g)) { snprintf(err, n, "Rota intergaláctica exige Propulsor Hiperespacial 3"); return 0; }
    } else {
        int npi = npc_planet_at(g, t->tg, t->ts, t->tp);
        if (npi < 0 || g->npc[g->npl[npi].npc].type != NPC_TRADER) { snprintf(err, n, "O destino precisa ser um mundo Mercante"); return 0; }
        if (t->tg != o->g && !intergalactic_ok(g)) { snprintf(err, n, "Rota intergaláctica exige Propulsor Hiperespacial 3"); return 0; }
        if (t->res_in == t->res_out) { snprintf(err, n, "Exportação e importação devem ser diferentes"); return 0; }
    }
    double cap = fleet_cargo_cap(g, t->ship);
    if (cap <= 0) { snprintf(err, n, "Atribua cargueiros à rota"); return 0; }
    if (t->ship[SI(S_SATELLITE)] || t->ship[SI(S_CRAWLER)]) { snprintf(err, n, "Satélites/rastejadores não voam"); return 0; }
    for (int i = 0; i < N_SHIP; i++) if (t->ship[i] > o->ship[i] || t->ship[i] < 0) { snprintf(err, n, "Naves insuficientes na origem"); return 0; }
    int slot = -1;
    for (int i = 0; i < MAX_ROUTES; i++) if (!g->route[i].used) { slot = i; break; }
    if (slot < 0) { snprintf(err, n, "Limite de rotas"); return 0; }
    Route *r = &g->route[slot];
    *r = *t;
    r->used = 1;
    r->state = RS_IDLE;
    r->t_next = g->now;
    memset(r->load, 0, sizeof r->load);
    memset(r->back, 0, sizeof r->back);
    r->trips = r->ambushes = r->lost_ships = 0;
    r->delivered = 0;
    if (r->kind == RT_INTERNAL) { r->tg = g->body[r->dest].g; r->ts = g->body[r->dest].s; r->tp = g->body[r->dest].p; }
    else r->npc = g->npl[npc_planet_at(g, r->tg, r->ts, r->tp)].npc;
    for (int i = 0; i < N_SHIP; i++) o->ship[i] -= r->ship[i];
    g->flags |= FL_ROUTE;
    if (err) snprintf(err, n, "Rota #%d criada", slot + 1);
    return 1;
}

void route_delete(Game *g, int ri) {
    Route *r = &g->route[ri];
    if (!r->used) return;
    Body *o = &g->body[r->origin < g->nbody && g->body[r->origin].used ? r->origin : 0];
    for (int i = 0; i < N_SHIP; i++) o->ship[i] += r->ship[i];
    for (int i = 0; i < NRES; i++) o->res[i] += r->load[i] + r->back[i];
    r->used = 0;
}

void trade_process(Game *g) {
    for (int ri = 0; ri < MAX_ROUTES; ri++) {
        Route *r = &g->route[ri];
        if (!r->used) continue;
        if (r->origin >= g->nbody || !g->body[r->origin].used || (r->kind == RT_INTERNAL && (r->dest >= g->nbody || !g->body[r->dest].used))) {
            route_delete(g, ri);
            continue;
        }
        Body *o = &g->body[r->origin];
        for (int guard = 0; guard < 6; guard++) {
            if (r->state == RS_IDLE && r->t_next <= g->now) {
                double cap = route_capacity(g, r);
                double want = r->amount > 0 ? fmin(r->amount, cap) : cap;
                double fuel = route_fuel(g, r);
                double avail = o->res[r->res_out];
                if (r->res_out == RES_DEUT) avail -= fuel;
                double load = floor(fmin(want, avail));
                if (load < cap * 0.02 || o->res[RES_DEUT] < fuel) {
                    r->t_next = g->now + 900.0 / g->speed;
                    break;
                }
                o->res[RES_DEUT] -= fuel;
                o->res[r->res_out] -= load;
                memset(r->load, 0, sizeof r->load);
                r->load[r->res_out] = load;
                r->state = RS_OUT;
                r->t_arrive = g->now + route_trip_time(g, r);
                continue;
            }
            if (r->state == RS_OUT && r->t_arrive <= g->now) {
                double keep = 1.0;
                if (newbie_left(g) <= 0 && rng_double(&g->rng) < route_risk(g, r)) route_ambush(g, ri, &keep);
                for (int i = 0; i < NRES; i++) r->load[i] = floor(r->load[i] * keep);
                double delivered = r->load[0] + r->load[1] + r->load[2] + r->load[3];
                if (r->kind == RT_INTERNAL) {
                    Body *d = &g->body[r->dest];
                    for (int i = 0; i < NRES; i++) d->res[i] += r->load[i];
                } else {
                    double got = floor(r->load[r->res_out] * market_rate(g, r->npc, r->res_out, r->res_in));
                    memset(r->back, 0, sizeof r->back);
                    r->back[r->res_in] = got;
                }
                memset(r->load, 0, sizeof r->load);
                r->delivered += delivered;
                r->trips++;
                r->state = RS_BACK;
                r->t_return = r->t_arrive + route_trip_time(g, r);
                if (fleet_ship_count(r->ship) <= 0 || route_capacity(g, r) <= 0) {
                    game_msg(g, MSG_TRADE, "Rota encerrada", "Todas as naves da rota #%d foram perdidas. A rota foi desfeita.", ri + 1);
                    r->used = 0;
                    break;
                }
                continue;
            }
            if (r->state == RS_BACK && r->t_return <= g->now) {
                for (int i = 0; i < NRES; i++) o->res[i] += r->back[i];
                memset(r->back, 0, sizeof r->back);
                r->state = RS_IDLE;
                r->t_next = r->t_return;
                continue;
            }
            break;
        }
    }
}

double trade_next_time(const Game *g) {
    double t = 1e300;
    for (int i = 0; i < MAX_ROUTES; i++) {
        const Route *r = &g->route[i];
        if (!r->used) continue;
        double x = r->state == RS_IDLE ? r->t_next : r->state == RS_OUT ? r->t_arrive : r->t_return;
        if (x < t) t = x;
    }
    return t;
}
