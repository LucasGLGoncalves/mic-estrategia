/* economy.c - costs, times, production rates, build checks and queues */
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int tech(const Game *g, int id) { return IS_TECH(id) ? g->tech[TI(id)] : 0; }

int officer_active(const Game *g, int off) { return g->officer[off] > g->now; }

double storage_cap(int lvl) { return 5000.0 * floor(2.5 * exp(20.0 * lvl / 33.0)); }

double prod_metal_lvl(int l) { return 30.0 * l * pow(1.1, l); }
double prod_crystal_lvl(int l) { return 20.0 * l * pow(1.1, l); }
double prod_deut_lvl(int l, double tavg) {
    double f = 1.36 - 0.004 * tavg;
    if (f < 0.2) f = 0.2;
    return 10.0 * l * pow(1.1, l) * f;
}

double item_cost1(int id, int level, double out[3]) {
    const ItemDef *d = &ITEMS[id];
    double e = 0;
    if (IS_BLD(id) || IS_TECH(id)) {
        double m = pow(d->factor, level - 1);
        for (int i = 0; i < 3; i++) {
            out[i] = floor(d->cost[i] * m);
            if (id == T_ASTRO) out[i] = 100.0 * floor(out[i] / 100.0 + 0.5);
        }
        if (d->energy > 0) e = floor(d->energy * pow(id == T_GRAVITON ? 3.0 : (id == B_SPACE_DOCK ? 2.5 : 2.0), level - 1));
    } else {
        for (int i = 0; i < 3; i++) out[i] = d->cost[i];
    }
    return e;
}

void item_cost(const Game *g, int id, long long lc, double out[3]) {
    (void)g;
    if (IS_SHIP(id) || IS_DEF(id)) {
        item_cost1(id, 1, out);
        for (int i = 0; i < 3; i++) out[i] *= (double)lc;
    } else {
        item_cost1(id, (int)lc, out);
    }
}

double item_energy_req(int id, int level) {
    double c[3];
    return item_cost1(id, level, c);
}

/* ------------- workers and energy per building ------------- */
static double expo(int l, double k) { return k * l * pow(1.1, l); }

double workers_for(int id, int l) {
    if (l <= 0) return 0;
    switch (id) {
    case B_METAL_MINE: case B_CRYSTAL_MINE: return expo(l, 8);
    case B_DEUT_SYNTH: return expo(l, 10);
    case B_SOLAR_PLANT: return expo(l, 3);
    case B_FUSION: return expo(l, 6);
    case B_FARM: return expo(l, 2.5);
    case B_FOOD_SYNTH: return expo(l, 4);
    case B_ROBOTICS: return 10.0 * l;
    case B_SHIPYARD: return 12.0 * l;
    case B_LAB: return 15.0 * l;
    case B_MISSILE_SILO: return 8.0 * l;
    case B_NANITE: return 60.0 * l;
    case B_TERRAFORMER: return 25.0 * l;
    case B_SPACE_DOCK: return 10.0 * l;
    case B_ORBITAL: return 30.0 * l;
    case B_TRADE_HUB: return 20.0 * l;
    case B_ACADEMY: return 30.0 * l;
    case B_MEDICAL: return 20.0 * l;
    default: return 0;
    }
}

double energy_for(int id, int l) {
    if (l <= 0) return 0;
    switch (id) {
    case B_METAL_MINE: case B_CRYSTAL_MINE: return expo(l, 10);
    case B_DEUT_SYNTH: return expo(l, 20);
    case B_FARM: return expo(l, 6);
    case B_FOOD_SYNTH: return expo(l, 15);
    case B_ARCOLOGY: return expo(l, 50);
    case B_ROBOTICS: return 5.0 * l;
    case B_SHIPYARD: return 6.0 * l;
    case B_LAB: return 8.0 * l;
    case B_MISSILE_SILO: return 5.0 * l;
    case B_NANITE: return 40.0 * l;
    case B_TERRAFORMER: return 30.0 * l;
    case B_SPACE_DOCK: return 10.0 * l;
    case B_ORBITAL: return 25.0 * l;
    case B_TRADE_HUB: return 10.0 * l;
    case B_MEDICAL: return 10.0 * l;
    case B_ACADEMY: return 15.0 * l;
    default: return 0;
    }
}

static int is_producer(int id) {
    return id == B_METAL_MINE || id == B_CRYSTAL_MINE || id == B_DEUT_SYNTH || id == B_SOLAR_PLANT ||
           id == B_FUSION || id == B_FARM || id == B_FOOD_SYNTH;
}

static double pct(const Body *b, int id) { return is_producer(id) ? b->pct[id] / 100.0 : 1.0; }

void body_add_mod(Body *b, int kind, double factor, double until) {
    int slot = -1;
    for (int i = 0; i < MAX_MODS; i++) {
        if (b->mod[i].until <= 0) { slot = i; break; }
    }
    if (slot < 0) {
        double mn = 1e300;
        for (int i = 0; i < MAX_MODS; i++) if (b->mod[i].until < mn) { mn = b->mod[i].until; slot = i; }
    }
    b->mod[slot].kind = kind;
    b->mod[slot].factor = factor;
    b->mod[slot].until = until;
}

double body_mod(const Body *b, int kind, double now) {
    double f = 1.0;
    for (int i = 0; i < MAX_MODS; i++)
        if (b->mod[i].until > now && b->mod[i].kind == kind) f *= b->mod[i].factor;
    return f;
}

static double pos_metal(int p) {
    switch (p) { case 8: return 1.35; case 7: case 9: return 1.23; case 6: case 10: return 1.17; default: return 1.0; }
}
static double pos_crystal(int p) {
    switch (p) { case 1: return 1.4; case 2: return 1.3; case 3: return 1.2; default: return 1.0; }
}

void body_rates(const Game *g, const Body *b, Rates *r) {
    memset(r, 0, sizeof *r);
    double sp = g->speed;
    double now = g->now;
    const PlanetInfo *pi = &b->info;
    r->cap[RES_METAL] = storage_cap(b->bld[B_METAL_STORE]);
    r->cap[RES_CRYSTAL] = storage_cap(b->bld[B_CRYSTAL_STORE]);
    r->cap[RES_DEUT] = storage_cap(b->bld[B_DEUT_TANK]);
    r->cap[RES_FOOD] = storage_cap(b->bld[B_FARM]);
    r->e_eff = 1; r->wf = 1; r->farm_wf = 1; r->fac_eff = 1;
    if (b->is_moon) {
        double he3 = 30.0 * b->bld[B_HE3] * pow(1.1, b->bld[B_HE3]) * sp * body_mod(b, MOD_DEUT, now);
        r->prod[RES_DEUT] = r->gross[RES_DEUT] = he3;
        r->cap[RES_FOOD] = 0;
        return;
    }
    double tavg = (pi->tmin + pi->tmax) / 2.0;
    /* energy production */
    double eng = officer_active(g, OFF_ENGINEER) ? 1.10 : 1.0;
    double emod = body_mod(b, MOD_ENERGY, now);
    double solar = 20.0 * b->bld[B_SOLAR_PLANT] * pow(1.1, b->bld[B_SOLAR_PLANT]) * pct(b, B_SOLAR_PLANT);
    double sat_each = floor((pi->tmax + 140) / 6.0);
    if (sat_each < 1) sat_each = 1;
    sat_each *= 1.0 + 0.05 * tech(g, T_ORBITAL_ENG);
    r->sat_each = sat_each;
    double satp = body_mod(b, MOD_SAT_OFF, now) < 1.0 ? 0.0 : b->sat_pct / 100.0;
    double sats = (double)b->ship[SI(S_SATELLITE)] * sat_each * satp;
    double fusion_on = body_mod(b, MOD_FUSION_OFF, now) < 1.0 ? 0.0 : 1.0;
    int fl = b->bld[B_FUSION];
    double fusion = 30.0 * fl * pow(1.05 + 0.01 * tech(g, T_ENERGY), fl) * pct(b, B_FUSION) * fusion_on;
    double fusion_deut = 10.0 * fl * pow(1.1, fl) * pct(b, B_FUSION) * fusion_on;
    double synth_deut = 8.0 * b->bld[B_FOOD_SYNTH] * pow(1.1, b->bld[B_FOOD_SYNTH]) * pct(b, B_FOOD_SYNTH);
    /* deuterium availability */
    double deut_gross_h = prod_deut_lvl(b->bld[B_DEUT_SYNTH], tavg) * pct(b, B_DEUT_SYNTH);
    double deut_need = fusion_deut + synth_deut;
    double deut_ok = 1.0;
    if (b->res[RES_DEUT] < 1.0 && deut_need > 0 && deut_gross_h < deut_need) deut_ok = deut_gross_h / deut_need;
    fusion *= deut_ok; fusion_deut *= deut_ok; synth_deut *= deut_ok;

    r->e_prod = (solar + fusion + sats) * eng * emod;
    /* energy consumption */
    double cons = 0;
    for (int id = 0; id < N_BLD; id++) cons += energy_for(id, b->bld[id]) * pct(b, id);
    long long crawlers = b->ship[SI(S_CRAWLER)];
    int minesum = b->bld[B_METAL_MINE] + b->bld[B_CRYSTAL_MINE] + b->bld[B_DEUT_SYNTH];
    double crawl_eff = (double)crawlers;
    if (crawl_eff > 8.0 * minesum) crawl_eff = 8.0 * minesum;
    cons += crawl_eff * 50.0 * b->crawler_pct / 100.0;
    r->e_cons = cons;
    r->e_eff = cons > 0 ? (r->e_prod >= cons ? 1.0 : r->e_prod / cons) : 1.0;

    /* workforce */
    double wfarm = workers_for(B_FARM, b->bld[B_FARM]) * pct(b, B_FARM);
    double wother = 0;
    for (int id = 0; id < N_BLD; id++) if (id != B_FARM) wother += workers_for(id, b->bld[id]) * pct(b, id);
    r->w_farm = wfarm; r->w_req = wother;
    r->farm_wf = wfarm > 0 ? clampd(b->pop / wfarm, 0, 1) : 1.0;
    double avail = b->pop - wfarm * r->farm_wf;
    if (avail < 0) avail = 0;
    r->wf = wother > 0 ? clampd(avail / wother, 0, 1) : 1.0;
    double ee = r->e_eff, wf = r->wf;

    /* mines */
    double geo = (officer_active(g, OFF_GEOLOGIST) ? 1.10 : 1.0) + 0.02 * tech(g, T_GEOPHYSICS);
    double crawl = 1.0 + crawl_eff * 0.0002 * b->crawler_pct / 100.0;
    double pl = tech(g, T_PLASMA);
    double mm = prod_metal_lvl(b->bld[B_METAL_MINE]) * pct(b, B_METAL_MINE) * ee * wf * pos_metal(b->p) * pi->rich[0] *
                (geo + 0.01 * pl) * crawl * body_mod(b, MOD_METAL, now);
    double cm = prod_crystal_lvl(b->bld[B_CRYSTAL_MINE]) * pct(b, B_CRYSTAL_MINE) * ee * wf * pos_crystal(b->p) * pi->rich[1] *
                (geo + 0.0066 * pl) * crawl * body_mod(b, MOD_CRYSTAL, now);
    double dm = deut_gross_h * ee * wf * pi->rich[2] * (geo + 0.0033 * pl) * crawl * body_mod(b, MOD_DEUT, now);
    r->base[RES_METAL] = 30.0 * pos_metal(b->p);
    r->base[RES_CRYSTAL] = 15.0 * pos_crystal(b->p);
    r->mine[0] = mm * sp; r->mine[1] = cm * sp; r->mine[2] = dm * sp;
    r->gross[RES_METAL] = (mm + r->base[RES_METAL]) * sp;
    r->gross[RES_CRYSTAL] = (cm + r->base[RES_CRYSTAL]) * sp;
    r->gross[RES_DEUT] = dm * sp;
    r->fusion_deut = fusion_deut * sp;
    r->synth_deut = synth_deut * ee * sp;
    r->prod[RES_METAL] = r->gross[RES_METAL];
    r->prod[RES_CRYSTAL] = r->gross[RES_CRYSTAL];
    r->prod[RES_DEUT] = r->gross[RES_DEUT] - r->fusion_deut - r->synth_deut;

    /* food */
    double bio = 1.0 + 0.06 * tech(g, T_BIOTECH);
    double farm = 25.0 * b->bld[B_FARM] * pow(1.1, b->bld[B_FARM]) * pct(b, B_FARM) * pi->fertility * bio * ee * r->farm_wf;
    double fsyn = 40.0 * b->bld[B_FOOD_SYNTH] * pow(1.1, b->bld[B_FOOD_SYNTH]) * pct(b, B_FOOD_SYNTH) * ee * wf * deut_ok * bio;
    double arco = 200.0 * b->bld[B_ARCOLOGY] * pow(1.1, b->bld[B_ARCOLOGY]) * ee;
    double food = (30.0 + farm + fsyn + arco) * body_mod(b, MOD_FOOD, now);
    r->food_prod = food * sp;
    r->food_cons = b->pop * 0.1 * sp;
    r->gross[RES_FOOD] = r->food_prod;
    r->prod[RES_FOOD] = r->food_prod - r->food_cons;

    /* population */
    int hl = b->bld[B_HABITAT], al = b->bld[B_ARCOLOGY];
    r->pop_cap = 250.0 + 100.0 * hl * pow(1.12, hl) + 2500.0 * al * pow(1.15, al);
    double rad = pi->radiation == 0 ? 1.1 : pi->radiation == 1 ? 1.0 : pi->radiation == 2 ? 0.8 : 0.6;
    double growth = 0.06 * (1.0 + 0.1 * tech(g, T_MEDICINE)) * (1.0 + 0.08 * b->bld[B_MEDICAL]) * rad * body_mod(b, MOD_GROWTH, now);
    r->growth = growth * sp;

    r->fac_eff = clampd(ee < wf ? ee : wf, 0.25, 1.0);
}

int body_fields_used(const Body *b) {
    int n = 0;
    for (int id = 0; id < N_BLD; id++) if (id != B_TERRAFORMER) n += b->bld[id];
    return n;
}

int body_fields_max(const Game *g, const Body *b) {
    (void)g;
    if (b->is_moon) return 1 + 3 * b->bld[B_LUNAR_BASE];
    return b->info.fields + 5 * b->bld[B_TERRAFORMER];
}

int req_have(const Game *g, const Body *b, int id) {
    if (IS_TECH(id)) return g->tech[TI(id)];
    if (IS_BLD(id)) return b ? b->bld[id] : 0;
    return 0;
}

int reqs_met(const Game *g, const Body *b, int id) {
    const ItemDef *d = &ITEMS[id];
    for (int i = 0; i < 6 && d->req[i].lvl > 0; i++)
        if (req_have(g, b, d->req[i].id) < d->req[i].lvl) return 0;
    return 1;
}

int queue_limit(const Game *g) { return officer_active(g, OFF_COMMANDER) ? 5 : 2; }

double build_time(const Game *g, const Body *b, int id, int level) {
    double c[3];
    item_cost1(id, level, c);
    double div = 2500.0 * fmax(4.0 - level / 2.0, 1.0) * (1.0 + b->bld[B_ROBOTICS]) * pow(2.0, b->bld[B_NANITE]);
    Rates r;
    body_rates(g, b, &r);
    double t = (c[0] + c[1]) / div * 3600.0 / g->speed / r.fac_eff;
    if (t < 1) t = 1;
    return t;
}

double research_time(const Game *g, int bi, int id, int level) {
    const Body *b = &g->body[bi];
    double c[3];
    item_cost1(id, level, c);
    /* effective lab level: own lab + best other labs (IRN) */
    int lab = b->bld[B_LAB];
    int irn = g->tech[TI(T_IRN)];
    if (irn > 0) {
        int others[MAX_BODIES], n = 0;
        for (int i = 0; i < g->nbody; i++)
            if (i != bi && g->body[i].used && !g->body[i].is_moon) others[n++] = g->body[i].bld[B_LAB];
        for (int k = 0; k < irn && k < n; k++) {
            int best = -1;
            for (int j = 0; j < n; j++) if (others[j] >= 0 && (best < 0 || others[j] > others[best])) best = j;
            if (best < 0) break;
            lab += others[best];
            others[best] = -1;
        }
    }
    double t = (c[0] + c[1]) / (1000.0 * (1.0 + lab)) * 3600.0 / g->speed;
    double acad = 0.03 * b->bld[B_ACADEMY];
    if (acad > 0.45) acad = 0.45;
    t *= (1.0 - acad);
    if (officer_active(g, OFF_TECHNOCRAT)) t *= 0.75;
    t *= body_mod(b, MOD_RESEARCH, g->now);
    Rates r;
    body_rates(g, b, &r);
    t /= r.fac_eff;
    if (t < 1) t = 1;
    return t;
}

double unit_time(const Game *g, const Body *b, int id) {
    double c[3];
    item_cost1(id, 1, c);
    Rates r;
    body_rates(g, b, &r);
    double t = (c[0] + c[1]) / (2500.0 * (1.0 + b->bld[B_SHIPYARD]) * pow(2.0, b->bld[B_NANITE])) * 3600.0 / g->speed / r.fac_eff;
    if (t < 0.05) t = 0.05;
    return t;
}

int missile_capacity(const Body *b) { return b->bld[B_MISSILE_SILO] * 10; }
int missile_used(const Body *b) {
    long long u = b->def[DI(D_ABM)] + 2 * b->def[DI(D_IPM)];
    for (int i = 0; i < b->nsq; i++) {
        if (b->sq[i].item == D_ABM) u += b->sq[i].count;
        if (b->sq[i].item == D_IPM) u += 2 * b->sq[i].count;
    }
    return (int)u;
}

static int queued_count(const Queue *q, int id) {
    int n = 0;
    for (int i = 0; i < q->n; i++) if (q->it[i].item == id) n++;
    return n;
}

static long long sq_count(const Body *b, int id) {
    long long n = 0;
    for (int i = 0; i < b->nsq; i++) if (b->sq[i].item == id) n += b->sq[i].count;
    return n;
}

static int research_busy_lab(const Game *g) {
    return g->rq.n > 0 && g->rq.it[0].active;
}

const char *bc_text(int c) {
    switch (c) {
    case BC_OK: return "Disponível";
    case BC_REQ: return "Requisitos não atendidos";
    case BC_RES: return "Recursos insuficientes";
    case BC_FIELDS: return "Sem campos livres";
    case BC_QUEUE: return "Fila cheia";
    case BC_MAX: return "Nível máximo";
    case BC_BUSY: return "Instalação ocupada";
    case BC_PLACE: return "Não pode ser construído aqui";
    case BC_ENERGY: return "Energia insuficiente";
    case BC_SILO: return "Silo de mísseis cheio";
    case BC_STORAGE: return "Custo maior que a capacidade do armazém";
    case BC_LABBUSY: return "Já há pesquisa em andamento em outro planeta";
    }
    return "?";
}

int exceeds_storage(const Game *g, const Body *b, const double c[3]) {
    Rates r;
    body_rates(g, b, &r);
    for (int i = 0; i < 3; i++) if (c[i] > r.cap[i] && c[i] > b->res[i]) return 1;
    return 0;
}

static int affordable(const Body *b, const double c[3]) {
    return b->res[0] + 1e-6 >= c[0] && b->res[1] + 1e-6 >= c[1] && b->res[2] + 1e-6 >= c[2];
}

int can_build(const Game *g, int bi, int id, long long count) {
    const Body *b = &g->body[bi];
    const ItemDef *d = &ITEMS[id];
    int where = b->is_moon ? W_MOON : W_PLANET;
    if (!(d->where & where)) return BC_PLACE;
    if (!reqs_met(g, b, id)) return BC_REQ;
    if (IS_BLD(id)) {
        const Queue *q = d->cat == CAT_LIFE ? &b->lq : &b->bq;
        int target = b->bld[id] + queued_count(q, id) + 1;
        if (d->maxlvl && target > d->maxlvl) return BC_MAX;
        if (q->n >= queue_limit(g)) return BC_QUEUE;
        int used = body_fields_used(b) + b->bq.n + b->lq.n;
        if (id != B_TERRAFORMER && id != B_LUNAR_BASE && used >= body_fields_max(g, b)) return BC_FIELDS;
        if (id == B_LAB && research_busy_lab(g) && g->rq_body == bi) return BC_BUSY;
        if ((id == B_SHIPYARD || id == B_NANITE) && b->nsq > 0) return BC_BUSY;
        if (q->n == 0) {
            double c[3];
            double e = item_cost1(id, target, c);
            if (!affordable(b, c)) return exceeds_storage(g, b, c) ? BC_STORAGE : BC_RES;
            if (e > 0) {
                Rates r;
                body_rates(g, b, &r);
                if (r.e_prod < e) return BC_ENERGY;
            }
        }
        return BC_OK;
    }
    if (IS_TECH(id)) {
        if (b->is_moon) return BC_PLACE;
        if (g->rq.n >= queue_limit(g)) return BC_QUEUE;
        for (int i = 0; i < b->bq.n; i++) if (b->bq.it[i].item == B_LAB && b->bq.it[i].active) return BC_BUSY;
        if (g->rq.n > 0 && g->rq_body != bi) return BC_LABBUSY;
        if (g->rq.n == 0) {
            double c[3];
            int target = g->tech[TI(id)] + 1;
            double e = item_cost1(id, target, c);
            if (!affordable(b, c)) return exceeds_storage(g, b, c) ? BC_STORAGE : BC_RES;
            if (e > 0) {
                Rates r;
                body_rates(g, b, &r);
                if (r.e_prod < e) return BC_ENERGY;
            }
        }
        return BC_OK;
    }
    /* ships / defense */
    if (count <= 0) return BC_RES;
    if (b->bld[B_SHIPYARD] <= 0) return BC_REQ;
    for (int i = 0; i < b->bq.n; i++)
        if (b->bq.it[i].active && (b->bq.it[i].item == B_SHIPYARD || b->bq.it[i].item == B_NANITE)) return BC_BUSY;
    if (b->nsq >= MAX_SHIPQ) return BC_QUEUE;
    if (d->maxlvl) {
        long long have = IS_DEF(id) ? b->def[DI(id)] : 0;
        if (have + sq_count(b, id) + count > d->maxlvl) return BC_MAX;
    }
    if (id == D_ABM || id == D_IPM) {
        int need = (id == D_IPM ? 2 : 1) * (int)count;
        if (missile_used(b) + need > missile_capacity(b)) return BC_SILO;
    }
    double c[3];
    item_cost(g, id, count, c);
    if (!affordable(b, c)) return BC_RES;
    return BC_OK;
}

long long max_affordable(const Game *g, const Body *b, int id) {
    double c[3];
    item_cost1(id, 1, c);
    double mx = 1e18;
    for (int i = 0; i < 3; i++) if (c[i] > 0) { double k = floor((b->res[i] + 1e-6) / c[i]); if (k < mx) mx = k; }
    if (mx > 1e12) mx = 1e12;
    const ItemDef *d = &ITEMS[id];
    if (d->maxlvl) {
        long long have = IS_DEF(id) ? b->def[DI(id)] : 0;
        long long left = d->maxlvl - have - sq_count(b, id);
        if (mx > left) mx = (double)left;
    }
    if (id == D_ABM || id == D_IPM) {
        int per = id == D_IPM ? 2 : 1;
        long long left = (missile_capacity(b) - missile_used(b)) / per;
        if (mx > left) mx = (double)left;
    }
    (void)g;
    return mx < 0 ? 0 : (long long)mx;
}

static void pay(Body *b, const double c[3]) { for (int i = 0; i < 3; i++) b->res[i] -= c[i]; }
static void refund(Body *b, const double c[3]) { for (int i = 0; i < 3; i++) b->res[i] += c[i]; }

int enqueue(Game *g, int bi, int id, long long count) {
    int rc = can_build(g, bi, id, count);
    if (rc != BC_OK) return rc;
    Body *b = &g->body[bi];
    const ItemDef *d = &ITEMS[id];
    if (IS_BLD(id)) {
        Queue *q = d->cat == CAT_LIFE ? &b->lq : &b->bq;
        QItem *it = &q->it[q->n++];
        memset(it, 0, sizeof *it);
        it->item = id;
        it->amount = b->bld[id] + queued_count(q, id); /* includes itself now */
        return BC_OK;
    }
    if (IS_TECH(id)) {
        if (g->rq.n == 0) g->rq_body = bi;
        QItem *it = &g->rq.it[g->rq.n++];
        memset(it, 0, sizeof *it);
        it->item = id;
        it->amount = g->tech[TI(id)] + queued_count(&g->rq, id);
        return BC_OK;
    }
    double c[3];
    item_cost(g, id, count, c);
    pay(b, c);
    SQItem *s = &b->sq[b->nsq++];
    s->item = id;
    s->count = count;
    s->unit = unit_time(g, b, id);
    double start = g->now;
    if (b->nsq > 1) start = -1; /* waits for previous */
    s->next = start < 0 ? 0 : start + s->unit;
    return BC_OK;
}

int cancel_queue(Game *g, int bi, int which, int idx) {
    /* which: 0 building, 1 lifeform, 2 research, 3 shipyard */
    Body *b = &g->body[bi];
    Queue *q = which == 0 ? &b->bq : which == 1 ? &b->lq : which == 2 ? &g->rq : NULL;
    if (q) {
        if (idx < 0 || idx >= q->n) return 0;
        QItem *it = &q->it[idx];
        if (it->active) {
            double c[3];
            item_cost1(it->item, (int)it->amount, c);
            Body *pb = which == 2 ? &g->body[g->rq_body] : b;
            refund(pb, c);
        }
        int id = it->item;
        /* remove item and any later items of same id (levels depend on it) */
        int w = 0;
        for (int i = 0; i < q->n; i++) {
            if (i == idx || (i > idx && q->it[i].item == id)) continue;
            q->it[w++] = q->it[i];
        }
        q->n = w;
        return 1;
    }
    if (idx < 0 || idx >= b->nsq) return 0;
    double c[3];
    item_cost(g, b->sq[idx].item, b->sq[idx].count, c);
    refund(b, c);
    for (int i = idx; i < b->nsq - 1; i++) b->sq[i] = b->sq[i + 1];
    b->nsq--;
    if (idx == 0 && b->nsq > 0) b->sq[0].next = g->now + b->sq[0].unit;
    return 1;
}

double dm_finish_cost(const Game *g, double remaining) {
    (void)g;
    double c = ceil(remaining / 60.0) * 5.0;
    if (c < 5) c = 5;
    return c;
}

int finish_with_dm(Game *g, int bi, int which) {
    Body *b = &g->body[bi];
    double rem = 0;
    if (which == 0 || which == 1 || which == 2) {
        Queue *q = which == 0 ? &b->bq : which == 1 ? &b->lq : &g->rq;
        if (q->n == 0 || !q->it[0].active) return 0;
        rem = q->it[0].end - g->now;
        double cost = dm_finish_cost(g, rem);
        if (g->dm < cost) return -1;
        g->dm -= cost;
        q->it[0].end = g->now;
        return 1;
    }
    if (b->nsq == 0) return 0;
    for (int i = 0; i < b->nsq; i++) rem += b->sq[i].unit * (double)b->sq[i].count;
    if (b->sq[0].next > g->now) rem -= b->sq[0].unit - (b->sq[0].next - g->now);
    double cost = dm_finish_cost(g, rem);
    if (g->dm < cost) return -1;
    g->dm -= cost;
    for (int i = 0; i < b->nsq; i++) {
        if (IS_SHIP(b->sq[i].item)) b->ship[SI(b->sq[i].item)] += b->sq[i].count;
        else b->def[DI(b->sq[i].item)] += b->sq[i].count;
    }
    b->nsq = 0;
    return 1;
}

/* ------------- points ------------- */
static double cum_cost(int id, int L) {
    if (L <= 0) return 0;
    const ItemDef *d = &ITEMS[id];
    double base = d->cost[0] + d->cost[1] + d->cost[2];
    double f = d->factor;
    if (fabs(f - 1.0) < 1e-9) return base * L;
    return base * (pow(f, L) - 1.0) / (f - 1.0);
}

double items_value(const long long *ships, const long long *defs) {
    double v = 0;
    if (ships)
        for (int i = 0; i < N_SHIP; i++) {
            const ItemDef *d = &ITEMS[T_END + i];
            v += (double)ships[i] * (d->cost[0] + d->cost[1] + d->cost[2]);
        }
    if (defs)
        for (int i = 0; i < N_DEF; i++) {
            const ItemDef *d = &ITEMS[S_END + i];
            v += (double)defs[i] * (d->cost[0] + d->cost[1] + d->cost[2]);
        }
    return v;
}

double body_military_value(const Body *b) {
    long long s[N_SHIP];
    memcpy(s, b->ship, sizeof s);
    s[SI(S_SATELLITE)] = 0; s[SI(S_CRAWLER)] = 0; s[SI(S_SMALL_CARGO)] = 0; s[SI(S_LARGE_CARGO)] = 0;
    s[SI(S_COLONY)] = 0; s[SI(S_RECYCLER)] = 0; s[SI(S_PROBE)] = 0;
    long long d[N_DEF];
    memcpy(d, b->def, sizeof d);
    d[DI(D_ABM)] = 0; d[DI(D_IPM)] = 0;
    return items_value(s, d);
}

double player_points(const Game *g, double *pb, double *pr, double *pf, double *pd) {
    double b = 0, r = 0, f = 0, d = 0;
    for (int i = 0; i < g->nbody; i++) {
        const Body *bd = &g->body[i];
        if (!bd->used) continue;
        for (int id = 0; id < N_BLD; id++) b += cum_cost(id, bd->bld[id]);
        f += items_value(bd->ship, NULL);
        d += items_value(NULL, bd->def);
    }
    for (int t = 0; t < N_TECH; t++) r += cum_cost(N_BLD + t, g->tech[t]);
    for (int i = 0; i < MAX_FLEETS; i++)
        if (g->fleet[i].used && g->fleet[i].mission != M_HOSTILE && g->fleet[i].mission != M_HOSTILE_SPY) f += items_value(g->fleet[i].ship, NULL);
    for (int i = 0; i < MAX_ROUTES; i++) if (g->route[i].used) f += items_value(g->route[i].ship, NULL);
    if (pb) *pb = b / 1000.0;
    if (pr) *pr = r / 1000.0;
    if (pf) *pf = f / 1000.0;
    if (pd) *pd = d / 1000.0;
    return (b + r + f + d) / 1000.0;
}
