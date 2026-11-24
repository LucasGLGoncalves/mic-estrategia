/* ai.c - a simple autopilot used by --simulate / tests to exercise every system */
#include "game.h"
#include <math.h>
#include <string.h>

static int ok(Game *g, int bi, int id) { return can_build(g, bi, id, 1) == BC_OK; }
static int go(Game *g, int bi, int id) { return enqueue(g, bi, id, 1) == BC_OK; }

static void plan_buildings(Game *g, int bi) {
    Body *b = &g->body[bi];
    if (b->bq.n > 0) return;
    Rates r;
    body_rates(g, b, &r);
    if (b->is_moon) {
        if (b->bld[B_LUNAR_BASE] < 2) go(g, bi, B_LUNAR_BASE);
        else if (b->bld[B_HE3] < b->bld[B_LUNAR_BASE] * 2) go(g, bi, B_HE3);
        else if (b->bld[B_PHALANX] < 1) go(g, bi, B_PHALANX);
        else go(g, bi, B_LUNAR_BASE);
        return;
    }
    int M = b->bld[B_METAL_MINE], C = b->bld[B_CRYSTAL_MINE], D = b->bld[B_DEUT_SYNTH];
    for (int k = 0; k < 3; k++)
        if (b->res[k] > 0.8 * r.cap[k] && go(g, bi, B_METAL_STORE + k)) return;
    {
        static const int big[] = {B_SOLAR_PLANT, B_METAL_MINE, B_CRYSTAL_MINE, B_DEUT_SYNTH, B_FUSION};
        for (int i = 0; i < 5; i++) {
            double c[3];
            item_cost1(big[i], b->bld[big[i]] + 1, c);
            for (int k = 0; k < 3; k++) if (c[k] > r.cap[k] * 0.95 && go(g, bi, B_METAL_STORE + k)) return;
        }
    }
    if (M >= 4 && b->bld[B_ROBOTICS] < 2 && go(g, bi, B_ROBOTICS)) return;
    if (M >= 4 && bi == 0 && b->bld[B_LAB] < 1 && go(g, bi, B_LAB)) return;
    if (b->bld[B_ROBOTICS] >= 2 && b->bld[B_SHIPYARD] < 1 && go(g, bi, B_SHIPYARD)) return;
    if (r.e_prod - r.e_cons < 15 || r.e_eff < 0.99) {
        if (b->bld[B_FUSION] < b->bld[B_SOLAR_PLANT] / 3 && ok(g, bi, B_FUSION) && b->res[RES_DEUT] > 500) { go(g, bi, B_FUSION); return; }
        if (go(g, bi, B_SOLAR_PLANT)) return;
        return;
    }
    for (int k = 0; k < 3; k++)
        if (b->res[k] > 0.85 * r.cap[k] && go(g, bi, B_METAL_STORE + k)) return;
    if (body_fields_used(b) + 2 >= body_fields_max(g, b)) { if (go(g, bi, B_TERRAFORMER)) return; }
    static const int pri[] = {B_METAL_MINE, B_CRYSTAL_MINE, B_DEUT_SYNTH, B_ROBOTICS, B_LAB, B_SHIPYARD, B_ORBITAL, B_TRADE_HUB, B_MISSILE_SILO, B_NANITE};
    for (int i = 0; i < 10; i++) {
        int id = pri[i];
        int want = 0;
        switch (id) {
        case B_METAL_MINE: want = M <= C + 2; break;
        case B_CRYSTAL_MINE: want = C <= D + 3; break;
        case B_DEUT_SYNTH: want = D <= M - 3; break;
        case B_ROBOTICS: want = b->bld[B_ROBOTICS] < M / 3 + 1; break;
        case B_LAB: want = b->bld[B_LAB] < M / 3 + 1 && bi == 0; break;
        case B_SHIPYARD: want = b->bld[B_SHIPYARD] < M / 3 + 1; break;
        case B_ORBITAL: want = b->bld[B_ORBITAL] < 1 && bi == 0; break;
        case B_TRADE_HUB: want = b->bld[B_TRADE_HUB] < 1; break;
        case B_MISSILE_SILO: want = b->bld[B_MISSILE_SILO] < 2 && M > 14; break;
        case B_NANITE: want = b->bld[B_NANITE] < 2; break;
        }
        if (want && go(g, bi, id)) return;
    }
    if (!go(g, bi, B_METAL_MINE) && !go(g, bi, B_CRYSTAL_MINE)) go(g, bi, B_DEUT_SYNTH);
}

static void plan_life(Game *g, int bi) {
    Body *b = &g->body[bi];
    if (b->is_moon || b->lq.n > 0) return;
    Rates r;
    body_rates(g, b, &r);
    if (r.prod[RES_FOOD] < r.food_cons * 0.15 || b->res[RES_FOOD] < 300) { if (go(g, bi, B_FARM)) return; if (go(g, bi, B_FOOD_SYNTH)) return; }
    if (b->pop > r.pop_cap * 0.85 || r.wf < 0.99) { if (go(g, bi, B_HABITAT)) return; }
    if (b->bld[B_MEDICAL] < b->bld[B_HABITAT] / 4 && go(g, bi, B_MEDICAL)) return;
    if (bi == 0 && b->bld[B_ACADEMY] < 3 && go(g, bi, B_ACADEMY)) return;
}

static void plan_research(Game *g) {
    if (g->rq.n > 0) return;
    static const struct { int id, lvl; } T[] = {
        {T_ENERGY, 1}, {T_COMBUSTION, 2}, {T_COMPUTER, 2}, {T_LASER, 3}, {T_ARMOR, 2}, {T_LOGISTICS, 1}, {T_ESPIONAGE, 2},
        {T_ENERGY, 3}, {T_IMPULSE, 3}, {T_ESPIONAGE, 4}, {T_BIOTECH, 2}, {T_GEOPHYSICS, 2}, {T_COMPUTER, 4}, {T_ASTRO, 1},
        {T_WEAPONS, 3}, {T_SHIELDING, 2}, {T_COMBUSTION, 6}, {T_ASTRO, 3}, {T_ION, 2}, {T_MEDICINE, 2}, {T_ORBITAL_ENG, 1}};
    for (size_t i = 0; i < sizeof T / sizeof T[0]; i++)
        if (g->tech[TI(T[i].id)] < T[i].lvl && enqueue(g, 0, T[i].id, 1) == BC_OK) return;
    /* otherwise cheapest available */
    int best = -1;
    double bc = 1e300;
    for (int id = N_BLD; id < T_END; id++) {
        if (can_build(g, 0, id, 1) != BC_OK) continue;
        double c[3];
        item_cost1(id, g->tech[TI(id)] + 1, c);
        double v = c[0] + c[1] + c[2];
        if (v < bc) { bc = v; best = id; }
    }
    if (best >= 0) enqueue(g, 0, best, 1);
}

static double last_def[MAX_BODIES];

static void plan_yard(Game *g, int bi) {
    Body *b = &g->body[bi];
    if (b->nsq > 0 || b->bld[B_SHIPYARD] < 1) return;
    int th = b->is_moon ? 0 : threat_level(g, bi);
    if (th >= 4 && g->now - last_def[bi] > 1800.0) {
        static const int D[] = {D_GAUSS, D_HEAVY_LASER, D_LIGHT_LASER, D_ROCKET};
        for (int i = 0; i < 4; i++) {
            long long mx = max_affordable(g, b, D[i]);
            long long n = (long long)(mx * 0.25);
            if (n >= 1 && enqueue(g, bi, D[i], n) == BC_OK) { last_def[bi] = g->now; return; }
        }
    }
    if (b->ship[SI(S_SMALL_CARGO)] < 4 && enqueue(g, bi, S_SMALL_CARGO, 1) == BC_OK) return;
    if (b->ship[SI(S_PROBE)] < 3 && enqueue(g, bi, S_PROBE, 1) == BC_OK) return;
    if (bi == 0 && colonies_count(g) < max_colonies(g) && b->ship[SI(S_COLONY)] < 1 && enqueue(g, bi, S_COLONY, 1) == BC_OK) return;
    if (b->ship[SI(S_LARGE_CARGO)] < 3 && enqueue(g, bi, S_LARGE_CARGO, 1) == BC_OK) return;
    if (b->ship[SI(S_RECYCLER)] < 2 && enqueue(g, bi, S_RECYCLER, 1) == BC_OK) return;
    long long lf = max_affordable(g, b, S_LIGHT_FIGHTER);
    if (lf > 10 && b->ship[SI(S_LIGHT_FIGHTER)] < 50) enqueue(g, bi, S_LIGHT_FIGHTER, lf / 4);
}

static int fleet_busy(const Game *g, int m) {
    for (int i = 0; i < MAX_FLEETS; i++) if (g->fleet[i].used && g->fleet[i].mission == m) return 1;
    return 0;
}

static void plan_fleets(Game *g) {
    Body *h = &g->body[0];
    char err[160];
    /* colonize */
    if (h->ship[SI(S_COLONY)] > 0 && !fleet_busy(g, M_COLONIZE) && colonies_count(g) < max_colonies(g)) {
        for (int ds = 0; ds < 40; ds++) {
            int s = clampi(h->s + (ds % 2 ? ds / 2 + 1 : -(ds / 2)), 1, UNI_SYSTEMS);
            for (int p = 4; p <= 12; p++) {
                if (npc_planet_at(g, h->g, s, p) >= 0 || player_body_at(g, h->g, s, p, 0) >= 0) continue;
                long long sh[N_SHIP] = {0};
                sh[SI(S_COLONY)] = 1;
                double cargo[NRES] = {2000, 1000, 0, 500};
                if (h->res[0] < 2500 || h->res[1] < 1500 || h->res[3] < 600) memset(cargo, 0, sizeof cargo);
                if (fleet_launch(g, 0, M_COLONIZE, h->g, s, p, TT_PLANET, sh, cargo, 100, err, sizeof err)) return;
            }
        }
    }
    /* expedition */
    if (exped_slots_used(g) < exped_slots_max(g) && h->ship[SI(S_SMALL_CARGO)] >= 2) {
        long long sh[N_SHIP] = {0};
        sh[SI(S_SMALL_CARGO)] = 2;
        if (h->ship[SI(S_LIGHT_FIGHTER)] >= 5) sh[SI(S_LIGHT_FIGHTER)] = 5;
        fleet_launch(g, 0, M_EXPEDITION, h->g, h->s, 16, TT_PLANET, sh, NULL, 100, err, sizeof err);
    }
    /* spy a neighbour */
    if (h->ship[SI(S_PROBE)] >= 1 && !fleet_busy(g, M_ESPIONAGE) && rng_chance(&g->rng, 0.2)) {
        for (int k = 0; k < 30; k++) {
            int s = clampi(h->s + rng_range(&g->rng, -5, 5), 1, UNI_SYSTEMS), p = rng_range(&g->rng, 1, 15);
            if (npc_planet_at(g, h->g, s, p) >= 0) {
                long long sh[N_SHIP] = {0};
                sh[SI(S_PROBE)] = 1;
                if (fleet_launch(g, 0, M_ESPIONAGE, h->g, s, p, TT_PLANET, sh, NULL, 100, err, sizeof err)) break;
            }
        }
    }
    /* recycle own debris */
    Debris *d = debris_at(g, h->g, h->s, h->p, 0);
    if (d && d->m + d->c > 2000 && h->ship[SI(S_RECYCLER)] > 0 && !fleet_busy(g, M_RECYCLE)) {
        long long sh[N_SHIP] = {0};
        sh[SI(S_RECYCLER)] = h->ship[SI(S_RECYCLER)];
        fleet_launch(g, 0, M_RECYCLE, h->g, h->s, h->p, TT_DEBRIS, sh, NULL, 100, err, sizeof err);
    }
    /* raid an inactive with light fighters */
    if (h->ship[SI(S_LIGHT_FIGHTER)] >= 20 && h->ship[SI(S_SMALL_CARGO)] >= 2 && !fleet_busy(g, M_ATTACK) && rng_chance(&g->rng, 0.3)) {
        for (int k = 0; k < 60; k++) {
            int s = clampi(h->s + rng_range(&g->rng, -15, 15), 1, UNI_SYSTEMS), p = rng_range(&g->rng, 1, 15);
            int npi = npc_planet_at(g, h->g, s, p);
            if (npi >= 0 && g->npc[g->npl[npi].npc].type == NPC_INACTIVE) {
                long long sh[N_SHIP] = {0};
                sh[SI(S_LIGHT_FIGHTER)] = h->ship[SI(S_LIGHT_FIGHTER)];
                sh[SI(S_SMALL_CARGO)] = 2;
                if (fleet_launch(g, 0, M_ATTACK, h->g, s, p, TT_PLANET, sh, NULL, 100, err, sizeof err)) break;
            }
        }
    }
    /* trade route: home -> first colony (food or metal) */
    int has_route = 0;
    for (int i = 0; i < MAX_ROUTES; i++) if (g->route[i].used) has_route = 1;
    if (!has_route && h->bld[B_TRADE_HUB] > 0 && g->tech[TI(T_LOGISTICS)] > 0 && h->ship[SI(S_SMALL_CARGO)] >= 2) {
        for (int i = 1; i < g->nbody; i++) {
            if (!g->body[i].used || g->body[i].is_moon) continue;
            Route t;
            memset(&t, 0, sizeof t);
            t.kind = RT_INTERNAL; t.origin = 0; t.dest = i; t.res_out = RES_METAL; t.amount = 0;
            t.ship[SI(S_SMALL_CARGO)] = 1;
            route_create(g, &t, err, sizeof err);
            break;
        }
    }
}

static void balance(Game *g, int bi) {
    Body *b = &g->body[bi];
    char err[160];
    /* keep roughly 3:2:1 value; sell surplus crystal/deut for metal */
    if (b->res[RES_CRYSTAL] > b->res[RES_METAL] * 1.5 + 5000) merchant_trade(g, bi, RES_CRYSTAL, RES_METAL, (b->res[RES_CRYSTAL] - b->res[RES_METAL]) * 0.3, err, sizeof err);
    if (b->res[RES_DEUT] > b->res[RES_METAL] + 10000) merchant_trade(g, bi, RES_DEUT, RES_METAL, (b->res[RES_DEUT] - b->res[RES_METAL]) * 0.3, err, sizeof err);
}

void bot_step(Game *g) {
    if (g->ev.dec.active) decision_autoresolve(g);
    for (int i = 0; i < g->nbody; i++) if (g->body[i].used && !g->body[i].is_moon) balance(g, i);
    for (int i = 0; i < g->nbody; i++) {
        if (!g->body[i].used) continue;
        plan_buildings(g, i);
        plan_life(g, i);
        plan_yard(g, i);
    }
    plan_research(g);
    plan_fleets(g);
    /* hire an officer when dark matter piles up */
    if (g->dm > 12000 && !officer_active(g, OFF_GEOLOGIST)) { g->dm -= 5000; g->officer[OFF_GEOLOGIST] = g->now + 7 * 86400.0; }
}
