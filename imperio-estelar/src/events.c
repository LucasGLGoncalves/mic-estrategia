/* events.c - random events, dice tests, hostile fleets, threat */
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void compose_fleet(double value, double pts, long long *ship);

const char *event_name(int t) {
    static const char *N[EV_COUNT] = {
        "-", "Sondas de espionagem", "Ataque pirata", "Ataque xeno", "Ataque de mísseis", "Terremoto",
        "Chuva de meteoros", "Tempestade solar", "Epidemia", "Tempestade", "Praga agrícola",
        "Vazamento no reator", "Imigrantes", "Veio mineral", "Cometa", "Nave abandonada",
        "Mercador viajante", "Anomalia", "Descoberta científica", "Revolta popular"};
    return (t >= 0 && t < EV_COUNT) ? N[t] : "?";
}

static double points(const Game *g) { return player_points(g, NULL, NULL, NULL, NULL); }

double newbie_left(const Game *g) {
    double p = points(g);
    return p >= NEWBIE_POINTS ? 0 : NEWBIE_POINTS - p;
}

static int tier(const Game *g) {
    double p = points(g);
    int t = (int)floor(log10(p + 1.0) * 2.0) - 3;
    return clampi(t, 0, 10);
}

static double stored(const Game *g, const Body *b) {
    Rates r;
    body_rates(g, b, &r);
    double s = 0;
    for (int i = 0; i < 3; i++) {
        double prot = r.cap[i] * 0.1;
        if (b->res[i] > prot) s += b->res[i] - prot;
    }
    return s;
}

static double expected_raid(const Game *g, const Body *b) {
    double pts = points(g);
    /* strength ramps up after the newbie protection ends */
    double k = 0.04 + 0.07 * clampd((pts - NEWBIE_POINTS) / 2000.0, 0.0, 1.0);
    double base = 1000.0 * pts * k;
    double greed = 0.3 * stored(g, b);
    double gcap = 1000.0 * pts * 0.25 + 20000.0;
    if (greed > gcap) greed = gcap;
    return base + greed;
}

int threat_level(const Game *g, int bi) {
    if (newbie_left(g) > 0) return 0;
    const Body *b = &g->body[bi];
    double v = expected_raid(g, b);
    double mil = body_military_value(b);
    double ratio = v / (mil + 1000.0);
    return clampi((int)lround(ratio * 3.0), 1, 10);
}

void events_schedule(Game *g, int first) {
    double mean = 5.0 * 3600.0 / g->speed;
    if (g->ev.offline) mean *= 3.0;
    double dt = first ? 1800.0 / g->speed + rng_exp(&g->rng, mean * 0.5) : rng_exp(&g->rng, mean);
    double mn = 300.0 / g->speed;
    if (dt < mn) dt = mn;
    g->ev.next_event = g->now + dt;
}

double events_next_time(const Game *g) {
    double t = g->ev.next_event;
    if (g->ev.dec.active && g->ev.dec.deadline < t) t = g->ev.dec.deadline;
    return t;
}

/* pick a random populated planet (not moon), weighted by value */
static int pick_body(Game *g) {
    double w[MAX_BODIES], tot = 0;
    for (int i = 0; i < g->nbody; i++) {
        w[i] = 0;
        if (!g->body[i].used || g->body[i].is_moon) continue;
        w[i] = 1.0 + stored(g, &g->body[i]) / 10000.0 + body_fields_used(&g->body[i]);
        tot += w[i];
    }
    double x = rng_double(&g->rng) * tot;
    for (int i = 0; i < g->nbody; i++) { if (x < w[i]) return i; x -= w[i]; }
    return 0;
}

/* find an attacker npc planet near the target */
static int pick_attacker(Game *g, const Body *b, int want_type) {
    int best = -1;
    double bestd = 1e300;
    for (int tries = 0; tries < 400; tries++) {
        int i = rng_range(&g->rng, 0, g->nnpl - 1);
        const NpcPlanet *pl = &g->npl[i];
        int t = g->npc[pl->npc].type;
        if (want_type == NPC_XENO ? t != NPC_XENO : (t != NPC_PIRATE && t != NPC_FLEETER)) continue;
        double d = fleet_distance(b->g, b->s, b->p, pl->g, pl->s, pl->p) * rng_uniform(&g->rng, 0.5, 1.5);
        if (d < bestd) { bestd = d; best = i; }
    }
    if (best < 0) best = rng_range(&g->rng, 0, g->nnpl - 1);
    return best;
}

static void launch_hostile(Game *g, int bi, int npi, int kind, double value) {
    Body *b = &g->body[bi];
    int slot = -1;
    for (int i = 0; i < MAX_FLEETS; i++) if (!g->fleet[i].used) { slot = i; break; }
    if (slot < 0) return;
    Fleet *f = &g->fleet[slot];
    memset(f, 0, sizeof *f);
    double pts = points(g);
    compose_fleet(value, pts, f->ship);
    f->ship[SI(S_PROBE)] = 0;
    f->ship[SI(S_RECYCLER)] = 0;
    if (fleet_ship_count(f->ship) < 1) { f->ship[SI(S_LIGHT_FIGHTER)] = 2; f->ship[SI(S_SMALL_CARGO)] = 1; }
    if (f->ship[SI(S_SMALL_CARGO)] + f->ship[SI(S_LARGE_CARGO)] == 0) f->ship[SI(S_SMALL_CARGO)] = 1 + (long long)(value / 60000.0);
    const NpcPlanet *pl = &g->npl[npi];
    f->used = 1;
    f->mission = M_HOSTILE;
    f->state = FS_OUT;
    f->origin = bi;
    f->og = pl->g; f->os = pl->s; f->op = pl->p;
    f->tg = b->g; f->ts = b->s; f->tp = b->p;
    f->ttype = b->is_moon ? TT_MOON : TT_PLANET;
    f->npc = pl->npc;
    f->hkind = kind;
    int w = g->tech[TI(T_WEAPONS)], s = g->tech[TI(T_SHIELDING)], a = g->tech[TI(T_ARMOR)];
    int d = kind == HK_XENO ? 2 : -1;
    f->wt = clampi(w + d, 0, 40); f->st = clampi(s + d, 0, 40); f->at = clampi(a + d, 0, 40);
    double dist = fleet_distance(pl->g, pl->s, pl->p, b->g, b->s, b->p);
    /* hostile speed: own drive techs approx = player's */
    double slow = fleet_slowest(g, f->ship);
    double dur = flight_duration(g, dist, slow, 100);
    double warn = 240.0 + 120.0 * b->bld[B_ORBITAL] + (b->moon >= 0 ? 300.0 * g->body[b->moon].bld[B_PHALANX] : 0);
    if (warn > 3600) warn = 3600;
    if (dur < warn) dur = warn;
    if (dur > 4.0 * 3600.0) dur = 4.0 * 3600.0;
    f->depart = g->now;
    f->arrive = g->now + dur;
    f->ret = f->arrive + dur;
    f->speed = 100;
    g->ev.last_hostile = g->now;
    if (g->ev.offline) g->ev.offline_hostiles++;
    char t[32];
    fmt_time(t, sizeof t, dur);
    const Npc *n = &g->npc[pl->npc];
    int detail = g->tech[TI(T_ESPIONAGE)] >= 4 || (b->moon >= 0 && g->body[b->moon].bld[B_PHALANX] > 0);
    char list[900] = "";
    if (detail) {
        for (int i = 0; i < N_SHIP; i++) if (f->ship[i] > 0) {
            size_t L = strlen(list);
            char a2[32];
            fmt_num(a2, sizeof a2, (double)f->ship[i]);
            snprintf(list + L, sizeof list - L, "  %s: %s\n", ITEMS[T_END + i].name, a2);
        }
    } else {
        char a2[32];
        fmt_num(a2, sizeof a2, fleet_ship_count(f->ship));
        snprintf(list, sizeof list, "  ~%s naves (Espionagem 4 ou Falange revelam a composição)\n", a2);
    }
    game_msg(g, MSG_COMBAT, kind == HK_XENO ? "ALERTA: frota xeno a caminho!" : "ALERTA: frota hostil a caminho!",
             "\001rUma frota de %s\001d (%s) partiu de [%d:%d:%d] rumo a %s [%d:%d:%d].\nChegada em %s.\n\n%s\n"
             "Reaja: construa defesas, envie sua frota em missão (fleetsave) ou gaste os recursos para que não sejam saqueados.",
             n->name, kind == HK_XENO ? "Xenos" : npc_type_name(n->type), pl->g, pl->s, pl->p, b->name, b->g, b->s, b->p, t, list);
    game_toast(g, RGB(235, 85, 70), "ALERTA! Frota hostil chega a %s em %s", b->name, t);
}

void hostile_arrive(Game *g, int fi) {
    Fleet *f = &g->fleet[fi];
    int bi = f->origin;
    if (bi < 0 || bi >= g->nbody || !g->body[bi].used) { f->used = 0; return; }
    Body *b = &g->body[bi];
    CombatIn in;
    memset(&in, 0, sizeof in);
    for (int i = 0; i < N_SHIP; i++) { in.n[0][i] = f->ship[i]; in.n[1][i] = b->ship[i]; }
    for (int i = 0; i < N_CDEF; i++) in.n[1][N_SHIP + i] = b->def[i];
    in.wt[0] = f->wt; in.st[0] = f->st; in.at[0] = f->at;
    in.wt[1] = g->tech[TI(T_WEAPONS)]; in.st[1] = g->tech[TI(T_SHIELDING)]; in.at[1] = g->tech[TI(T_ARMOR)];
    CombatOut out;
    combat_run(&g->rng, &in, &out);
    g->stat_combats++;
    long long dock_rec[N_SHIP] = {0};
    double dockp = 0.05 * b->bld[B_SPACE_DOCK];
    for (int i = 0; i < N_SHIP; i++) {
        long long lost = b->ship[i] - out.end[1][i];
        if (lost > 0 && dockp > 0 && T_END + i != S_SATELLITE && T_END + i != S_CRAWLER) dock_rec[i] = (long long)floor(lost * dockp);
        b->ship[i] = out.end[1][i] + dock_rec[i];
    }
    for (int i = 0; i < N_CDEF; i++) b->def[i] = out.end[1][N_SHIP + i] + out.repaired[N_SHIP + i];
    double loot[3] = {0, 0, 0};
    if (out.winner == 0) {
        Rates r;
        body_rates(g, b, &r);
        double avail[3];
        for (int i = 0; i < 3; i++) { double prot = r.cap[i] * 0.1; avail[i] = b->res[i] > prot ? b->res[i] - prot : 0; }
        long long surv[N_SHIP];
        for (int i = 0; i < N_SHIP; i++) surv[i] = out.end[0][i];
        double cap = 0;
        for (int i = 0; i < N_SHIP; i++) cap += (double)surv[i] * ITEMS[T_END + i].cargo;
        loot_take(avail, cap, loot);
        for (int i = 0; i < 3; i++) b->res[i] -= loot[i];
    } else if (out.winner == 1) {
        g->flags |= FL_DEFENDED;
        g->stat_wins++;
        g->honour += out.lost_value[0] / 5000.0;
    }
    Debris *d = debris_at(g, b->g, b->s, b->p, 1);
    d->m += out.debris[0]; d->c += out.debris[1];
    int moon_formed = 0;
    int planet = b->is_moon ? b->parent : bi;
    if (out.moon_chance > 0 && g->body[planet].moon < 0 && rng_double(&g->rng) * 100.0 < out.moon_chance) {
        create_moon(g, planet, 2000 + (int)(out.moon_chance * 300));
        moon_formed = 1;
    }
    b = &g->body[bi];
    b->last_attacked = g->now;
    const Npc *n = &g->npc[f->npc];
    char body[MSG_BODY], an[64], dn[64];
    snprintf(an, sizeof an, "%s %s", f->hkind == HK_XENO ? "Xeno" : n->type == NPC_PIRATE ? "Pirata" : "Invasor", n->name);
    snprintf(dn, sizeof dn, "Defensor %s (%s)", g->name, b->name);
    combat_report(&in, &out, an, dn, body, sizeof body);
    size_t L = strlen(body);
    char a[32], bb[32], c[32];
    fmt_num(a, sizeof a, loot[0]); fmt_num(bb, sizeof bb, loot[1]); fmt_num(c, sizeof c, loot[2]);
    L += (size_t)snprintf(body + L, sizeof body - L, "Saqueado: %s metal, %s cristal, %s deutério\n", a, bb, c);
    long long dsum = 0;
    for (int i = 0; i < N_SHIP; i++) dsum += dock_rec[i];
    if (dsum > 0 && L < sizeof body - 80) L += (size_t)snprintf(body + L, sizeof body - L, "A Doca Espacial recuperou %lld naves.\n", dsum);
    if (moon_formed && L < sizeof body - 80) snprintf(body + L, sizeof body - L, "\001mOs destroços se aglutinaram e formaram uma LUA!\001d\n");
    char title[MSG_TITLE];
    snprintf(title, sizeof title, "Defesa de %s: %s", b->name, out.winner == 1 ? "vitória" : out.winner == 0 ? "derrota" : "empate");
    game_msg(g, MSG_COMBAT, title, "%s", body);
    game_toast(g, out.winner == 1 ? RGB(110, 215, 110) : RGB(235, 85, 70), "Ataque a %s: %s", b->name,
               out.winner == 1 ? "invasores destruídos!" : out.winner == 0 ? "planeta saqueado" : "empate");
    f->used = 0;
}

/* ---------------- decisions ---------------- */
static void dec_init(Decision *d, int type, int body, const char *title) {
    memset(d, 0, sizeof *d);
    d->active = 1;
    d->type = type;
    d->body = body;
    snprintf(d->title, sizeof d->title, "%s", title);
}

static void dec_opt(Decision *d, const char *label, double m, double c, double de, double food, double dm, int bonus) {
    int i = d->nopt++;
    snprintf(d->opt[i], sizeof d->opt[i], "%s", label);
    d->optcost[i][0] = m; d->optcost[i][1] = c; d->optcost[i][2] = de; d->optcost[i][3] = food;
    d->optdm[i] = dm;
    d->optbonus[i] = bonus;
}

static int opt_affordable(const Game *g, const Decision *d, int i) {
    const Body *b = &g->body[d->body];
    for (int k = 0; k < NRES; k++) if (b->res[k] + 1e-6 < d->optcost[i][k]) return 0;
    if (g->dm + 1e-6 < d->optdm[i]) return 0;
    if (d->type == EV_METEOR && i == 0 && b->def[DI(D_ABM)] <= 0) return 0;
    return 1;
}

static void dec_finish_setup(Game *g, Decision *d) {
    d->deadline = g->now + 180.0;
    d->defopt = d->nopt - 1;
    for (int i = 0; i < d->nopt; i++) if (opt_affordable(g, d, i) && d->optbonus[i] > 0) { d->defopt = i; break; }
}

static int make_decision(Game *g, int type, int bi) {
    Body *b = &g->body[bi];
    Decision *d = &g->ev.dec;
    int t = tier(g);
    d->tier = t;
    Rates r;
    body_rates(g, b, &r);
    char title[64];
    switch (type) {
    case EV_EARTHQUAKE: {
        snprintf(title, sizeof title, "Terremoto em %s!", b->name);
        dec_init(d, type, bi, title);
        snprintf(d->text, sizeof d->text, "Sismógrafos registram um tremor de magnitude %.1f sob %s. Estruturas podem desabar e minas podem perder níveis.",
                 rng_uniform(&g->rng, 5.5, 8.9), b->name);
        d->base_mod = g->tech[TI(T_GEOPHYSICS)] + b->bld[B_ROBOTICS] / 2 + 2 * b->bld[B_NANITE];
        snprintf(d->mod_text, sizeof d->mod_text, "Geofísica %d + Robôs/2 %d + Nanitas×2 %d", g->tech[TI(T_GEOPHYSICS)], b->bld[B_ROBOTICS] / 2, 2 * b->bld[B_NANITE]);
        d->dc = 11 + t / 2 + (b->info.biome == BIO_VOLCANIC ? 2 : 0);
        double cost = fmax(500.0, floor(r.prod[RES_METAL] * 0.8));
        char lab[72];
        snprintf(lab, sizeof lab, "Reforçar estruturas (+5)");
        dec_opt(d, lab, cost, 0, 0, 0, 0, 5);
        dec_opt(d, "Evacuar: minas a 30% por 2h (sem teste)", 0, 0, 0, 0, 0, -1000);
        dec_opt(d, "Arriscar (teste puro)", 0, 0, 0, 0, 0, 0);
    } break;
    case EV_METEOR: {
        snprintf(title, sizeof title, "Chuva de meteoros sobre %s!", b->name);
        dec_init(d, type, bi, title);
        snprintf(d->text, sizeof d->text, "Um enxame de meteoros cruza a órbita de %s. %s",
                 b->name, (b->info.atmo == ATM_NONE || b->info.atmo == ATM_THIN) ? "Sem atmosfera densa, poucos vão queimar na entrada!" : "A atmosfera deve queimar boa parte deles.");
        d->base_mod = g->tech[TI(T_SHIELDING)] / 2 + (b->def[DI(D_SMALL_DOME)] ? 3 : 0) + (b->def[DI(D_LARGE_DOME)] ? 5 : 0);
        snprintf(d->mod_text, sizeof d->mod_text, "Escudos/2 %d + Cúpulas %d", g->tech[TI(T_SHIELDING)] / 2,
                 (b->def[DI(D_SMALL_DOME)] ? 3 : 0) + (b->def[DI(D_LARGE_DOME)] ? 5 : 0));
        d->dc = 12 + t / 2 + ((b->info.atmo == ATM_NONE || b->info.atmo == ATM_THIN) ? 3 : -1);
        long long abm = b->def[DI(D_ABM)];
        int ab = (int)(abm > 5 ? 5 : abm);
        char lab[72];
        snprintf(lab, sizeof lab, "Interceptar com %d míssil(eis) antibalístico(s) (+%d)", ab, 2 * ab);
        dec_opt(d, lab, 0, 0, 0, 0, 0, 2 * ab);
        dec_opt(d, "Desviar energia para os escudos (+4)", 0, 0, 300.0 * (t + 1), 0, 0, 4);
        dec_opt(d, "Abrigar-se e torcer", 0, 0, 0, 0, 0, 0);
    } break;
    case EV_FLARE: {
        snprintf(title, sizeof title, "Tempestade solar atinge %s!", b->name);
        dec_init(d, type, bi, title);
        snprintf(d->text, sizeof d->text, "Uma ejeção de massa coronal se aproxima. Satélites e redes elétricas correm perigo. Magnetosfera: %s.", MAG_NAME[b->info.magneto]);
        int mg = b->info.magneto == 0 ? -2 : b->info.magneto == 2 ? 3 : 0;
        d->base_mod = g->tech[TI(T_SHIELDING)] / 2 + mg;
        snprintf(d->mod_text, sizeof d->mod_text, "Escudos/2 %d + Magnetosfera %+d", g->tech[TI(T_SHIELDING)] / 2, mg);
        d->dc = 12 + t / 2 + (b->p <= 3 ? 2 : 0);
        dec_opt(d, "Recolher satélites: desligados por 3h (sem teste)", 0, 0, 0, 0, 0, -1000);
        dec_opt(d, "Blindar a rede elétrica (+5)", 0, fmax(300, floor(r.prod[RES_CRYSTAL] * 0.6)), 0, 0, 0, 5);
        dec_opt(d, "Arriscar (teste puro)", 0, 0, 0, 0, 0, 0);
    } break;
    case EV_PLAGUE: {
        snprintf(title, sizeof title, "Epidemia em %s!", b->name);
        dec_init(d, type, bi, title);
        snprintf(d->text, sizeof d->text, "Um patógeno desconhecido se espalha entre os %.0f habitantes de %s.", b->pop, b->name);
        d->base_mod = 2 * g->tech[TI(T_MEDICINE)] + b->bld[B_MEDICAL];
        snprintf(d->mod_text, sizeof d->mod_text, "Medicina×2 %d + Centro Médico %d", 2 * g->tech[TI(T_MEDICINE)], b->bld[B_MEDICAL]);
        d->dc = 12 + t / 2 + ((b->info.biome == BIO_JUNGLE || b->info.biome == BIO_TOXIC) ? 2 : 0);
        dec_opt(d, "Quarentena rígida (+3)", 0, 0, 0, fmax(200, floor(b->res[RES_FOOD] * 0.2)), 0, 3);
        dec_opt(d, "Vacinação em massa (+7)", 0, 800.0 * (t + 1), 400.0 * (t + 1), 0, 0, 7);
        dec_opt(d, "Confiar no sistema de saúde", 0, 0, 0, 0, 0, 0);
    } break;
    case EV_REACTOR: {
        snprintf(title, sizeof title, "Vazamento no reator de %s!", b->name);
        dec_init(d, type, bi, title);
        snprintf(d->text, sizeof d->text, "Sensores detectam radiação acima do normal no Reator de Fusão nível %d.", b->bld[B_FUSION]);
        d->base_mod = g->tech[TI(T_ENERGY)] / 2;
        snprintf(d->mod_text, sizeof d->mod_text, "Tecnologia de Energia/2 %d", g->tech[TI(T_ENERGY)] / 2);
        d->dc = 13 + t / 2;
        dec_opt(d, "Desligar o reator por 2h (sem teste)", 0, 0, 0, 0, 0, -1000);
        dec_opt(d, "Equipe de contenção (+5)", 0, 0, fmax(500, floor(r.fusion_deut * 3)), 0, 0, 5);
        dec_opt(d, "Manter a operação", 0, 0, 0, 0, 0, 0);
    } break;
    case EV_DERELICT: {
        snprintf(title, sizeof title, "Nave abandonada em órbita de %s", b->name);
        dec_init(d, type, bi, title);
        snprintf(d->text, sizeof d->text, "Um casco de origem desconhecida deriva em órbita. Pode conter tecnologia... ou uma armadilha.");
        d->base_mod = g->tech[TI(T_ESPIONAGE)] / 2 + g->tech[TI(T_COMPUTER)] / 3;
        snprintf(d->mod_text, sizeof d->mod_text, "Espionagem/2 %d + Computadores/3 %d", g->tech[TI(T_ESPIONAGE)] / 2, g->tech[TI(T_COMPUTER)] / 3);
        d->dc = 12 + t / 2;
        dec_opt(d, "Investigar com cuidado (+4)", 0, 0, 500.0 * (t + 1), 0, 0, 4);
        dec_opt(d, "Rebocar sem investigar", 0, 0, 0, 0, 0, 0);
        dec_opt(d, "Ignorar (sem teste)", 0, 0, 0, 0, 0, -1000);
    } break;
    default: return 0;
    }
    dec_finish_setup(g, d);
    return 1;
}

static void roll_dice(Game *g) {
    Decision *d = &g->ev.dec;
    int o = d->chosen;
    if (d->optbonus[o] == -1000) { d->roll = 0; d->total = 0; d->outcome = 1; return; }
    d->roll = rng_d(&g->rng, 20);
    d->total = d->roll + d->base_mod + d->optbonus[o];
    if (d->roll == 20) d->outcome = 2;
    else if (d->roll == 1) d->outcome = 3;
    else d->outcome = d->total >= d->dc ? 1 : 0;
}

void decision_choose(Game *g, int opt) {
    Decision *d = &g->ev.dec;
    if (!d->active || d->stage != 0 || opt < 0 || opt >= d->nopt) return;
    if (!opt_affordable(g, d, opt)) opt = d->nopt - 1;
    Body *b = &g->body[d->body];
    for (int k = 0; k < NRES; k++) b->res[k] -= d->optcost[opt][k];
    g->dm -= d->optdm[opt];
    if (d->type == EV_METEOR && opt == 0) {
        long long use = b->def[DI(D_ABM)] > 5 ? 5 : b->def[DI(D_ABM)];
        b->def[DI(D_ABM)] -= use;
    }
    d->chosen = opt;
    roll_dice(g);
    d->stage = 1;
    d->deadline = g->now + 120.0;
}

void decision_reroll(Game *g) {
    Decision *d = &g->ev.dec;
    if (!d->active || d->stage != 1 || d->rerolls >= 2 || g->dm < 300 || d->optbonus[d->chosen] == -1000) return;
    g->dm -= 300;
    d->rerolls++;
    roll_dice(g);
}

static void lose_levels(Game *g, Body *b, int n, char *out, size_t on) {
    static const int cand[] = {B_METAL_MINE, B_CRYSTAL_MINE, B_DEUT_SYNTH, B_SOLAR_PLANT, B_ROBOTICS, B_METAL_STORE, B_FARM, B_HABITAT};
    for (int k = 0; k < n; k++) {
        int opts[8], m = 0;
        for (int i = 0; i < 8; i++) if (b->bld[cand[i]] > 1) opts[m++] = cand[i];
        if (!m) return;
        int id = opts[rng_range(&g->rng, 0, m - 1)];
        b->bld[id]--;
        size_t L = strlen(out);
        snprintf(out + L, on - L, " %s caiu para o nível %d.", ITEMS[id].name, b->bld[id]);
    }
}

static void destroy_defense_frac(Game *g, Body *b, double frac, char *out, size_t on) {
    long long tot = 0;
    for (int i = 0; i < N_CDEF; i++) {
        if (S_END + i == D_SMALL_DOME || S_END + i == D_LARGE_DOME) continue;
        long long k = rng_binomial(&g->rng, b->def[i], frac);
        b->def[i] -= k;
        tot += k;
    }
    long long s = rng_binomial(&g->rng, b->ship[SI(S_SATELLITE)], frac);
    b->ship[SI(S_SATELLITE)] -= s;
    size_t L = strlen(out);
    if (tot + s > 0) snprintf(out + L, on - L, " Perdidos: %lld defesas e %lld satélites.", tot, s);
}

static double hours(const Game *g, double h) { return g->now + h * 3600.0 / g->speed; }

static void apply_decision(Game *g) {
    Decision *d = &g->ev.dec;
    Body *b = &g->body[d->body];
    char *R = d->result;
    size_t RN = sizeof d->result;
    R[0] = 0;
    int o = d->outcome;
    int auto_opt = d->optbonus[d->chosen] == -1000;
    Rates r;
    body_rates(g, b, &r);
    switch (d->type) {
    case EV_EARTHQUAKE:
        if (auto_opt) {
            body_add_mod(b, MOD_METAL, 0.3, hours(g, 2)); body_add_mod(b, MOD_CRYSTAL, 0.3, hours(g, 2)); body_add_mod(b, MOD_DEUT, 0.3, hours(g, 2));
            snprintf(R, RN, "Os trabalhadores foram evacuados. Nenhuma estrutura caiu, mas as minas operam a 30%% por 2h.");
        } else if (o == 2) {
            body_add_mod(b, MOD_METAL, 1.25, hours(g, 6));
            snprintf(R, RN, "Acerto crítico! As estruturas resistiram e o tremor expôs um veio de metal: +25%% metal por 6h.");
        } else if (o == 1) {
            body_add_mod(b, MOD_METAL, 0.9, hours(g, 1));
            snprintf(R, RN, "As estruturas resistiram. Apenas pequenos reparos (-10%% metal por 1h).");
        } else if (o == 3) {
            snprintf(R, RN, "Falha crítica! Desabamentos em série.");
            lose_levels(g, b, 1, R, RN);
            body_add_mod(b, MOD_METAL, 0.6, hours(g, 4)); body_add_mod(b, MOD_CRYSTAL, 0.6, hours(g, 4));
        } else {
            body_add_mod(b, MOD_METAL, 0.6, hours(g, 3)); body_add_mod(b, MOD_CRYSTAL, 0.6, hours(g, 3)); body_add_mod(b, MOD_DEUT, 0.7, hours(g, 3));
            snprintf(R, RN, "Galerias desabaram e precisam de reparos: metal e cristal -40%%, deutério -30%% por 3h.");
        }
        break;
    case EV_METEOR:
        if (o == 2 || o == 1) {
            double c = floor(r.prod[RES_CRYSTAL] * (o == 2 ? 4.0 : 1.0) + 300);
            b->res[RES_CRYSTAL] += c;
            snprintf(R, RN, "%s Fragmentos ricos em cristal foram coletados: +%.0f cristal.", o == 2 ? "Acerto crítico! Os meteoros foram pulverizados." : "Os meteoros foram desviados.", c);
        } else {
            snprintf(R, RN, "%s", o == 3 ? "Falha crítica! Impactos devastadores." : "Vários meteoros atingiram a superfície.");
            destroy_defense_frac(g, b, o == 3 ? 0.25 : 0.10, R, RN);
            for (int i = 0; i < 3; i++) b->res[i] *= (o == 3 ? 0.85 : 0.95);
        }
        break;
    case EV_FLARE:
        if (auto_opt) {
            body_add_mod(b, MOD_SAT_OFF, 0.0, hours(g, 3));
            snprintf(R, RN, "Satélites recolhidos com segurança. Ficarão desligados por 3h.");
        } else if (o == 2) {
            body_add_mod(b, MOD_ENERGY, 1.15, hours(g, 4));
            snprintf(R, RN, "Acerto crítico! A aurora carregou os coletores: +15%% energia por 4h.");
        } else if (o == 1) {
            body_add_mod(b, MOD_ENERGY, 0.9, hours(g, 1));
            snprintf(R, RN, "A rede resistiu com pequenas quedas (-10%% energia por 1h).");
        } else {
            double frac = o == 3 ? 0.5 : rng_uniform(&g->rng, 0.2, 0.4);
            long long s = (long long)floor((double)b->ship[SI(S_SATELLITE)] * frac);
            b->ship[SI(S_SATELLITE)] -= s;
            body_add_mod(b, MOD_ENERGY, o == 3 ? 0.5 : 0.7, hours(g, o == 3 ? 4 : 3));
            snprintf(R, RN, "%s %lld satélites queimaram e a energia caiu %d%% por %dh.", o == 3 ? "Falha crítica!" : "A tempestade sobrecarregou a rede.",
                     s, o == 3 ? 50 : 30, o == 3 ? 4 : 3);
        }
        break;
    case EV_PLAGUE:
        if (o == 2) {
            body_add_mod(b, MOD_GROWTH, 1.3, hours(g, 6));
            snprintf(R, RN, "Acerto crítico! A população desenvolveu imunidade: +30%% crescimento por 6h.");
        } else if (o == 1) {
            double l = floor(b->pop * 0.02);
            b->pop -= l;
            snprintf(R, RN, "A epidemia foi contida. Vítimas: %.0f.", l);
        } else {
            double frac = o == 3 ? 0.35 : rng_uniform(&g->rng, 0.15, 0.25);
            double l = floor(b->pop * frac);
            b->pop -= l;
            if (o == 3) body_add_mod(b, MOD_GROWTH, 0.5, hours(g, 6));
            snprintf(R, RN, "%s A epidemia vitimou %.0f habitantes.", o == 3 ? "Falha crítica!" : "O contágio se espalhou.", l);
        }
        break;
    case EV_REACTOR:
        if (auto_opt) {
            body_add_mod(b, MOD_FUSION_OFF, 0.0, hours(g, 2));
            snprintf(R, RN, "O reator foi desligado com segurança por 2h.");
        } else if (o == 2) {
            body_add_mod(b, MOD_ENERGY, 1.1, hours(g, 6));
            snprintf(R, RN, "Acerto crítico! A equipe descobriu uma otimização: +10%% energia por 6h.");
        } else if (o == 1) {
            snprintf(R, RN, "O vazamento foi selado sem consequências.");
        } else {
            if (o == 3 && b->bld[B_FUSION] > 1) b->bld[B_FUSION]--;
            double l = floor(b->pop * (o == 3 ? 0.06 : 0.03));
            b->pop -= l;
            body_add_mod(b, MOD_FUSION_OFF, 0.0, hours(g, o == 3 ? 4 : 2));
            if (o == 3) snprintf(R, RN, "Falha crítica! O reator foi danificado (nível %d), ficará 4h desligado e %.0f pessoas foram contaminadas.", b->bld[B_FUSION], l);
            else snprintf(R, RN, "Contaminação! O reator foi desligado por 2h para descontaminação e %.0f pessoas adoeceram.", l);
        }
        break;
    case EV_DERELICT:
        if (auto_opt) {
            snprintf(R, RN, "A nave foi ignorada e se perdeu no espaço profundo.");
        } else if (o == 2 || o == 1) {
            double v = (1000.0 * points(g) * 0.02 + 8000.0) * (o == 2 ? 1.5 : 1.0);
            long long found[N_SHIP] = {0};
            compose_fleet(v, points(g) < 500 ? 400 : points(g), found);
            found[SI(S_PROBE)] = 0;
            char list[300] = "";
            for (int i = 0; i < N_SHIP; i++) if (found[i] > 0) {
                b->ship[i] += found[i];
                size_t L = strlen(list);
                snprintf(list + L, sizeof list - L, " %lld %s;", found[i], ITEMS[T_END + i].shortn);
            }
            if (!list[0]) { b->res[RES_METAL] += v * 0.6; b->res[RES_CRYSTAL] += v * 0.4; snprintf(list, sizeof list, " sucata valiosa (%.0f recursos)", v); }
            if (o == 2) g->dm += 200;
            snprintf(R, RN, "%s Recuperado:%s", o == 2 ? "Acerto crítico! Uma joia tecnológica (+200 Matéria Negra)." : "A nave era segura.", list);
        } else {
            snprintf(R, RN, "%s", o == 3 ? "Falha crítica! A nave era uma bomba." : "Armadilha! Sistemas hostis dispararam.");
            for (int i = 0; i < 3; i++) b->res[i] *= 0.95;
            if (o == 3) destroy_defense_frac(g, b, 0.10, R, RN);
        }
        break;
    }
}

void decision_close(Game *g) {
    Decision *d = &g->ev.dec;
    if (!d->active) return;
    if (d->stage == 0) { d->chosen = d->defopt; decision_choose(g, d->defopt); }
    apply_decision(g);
    char body[MSG_BODY];
    if (d->optbonus[d->chosen] == -1000)
        snprintf(body, sizeof body, "%s\n\nDecisão: %s\n\n%s", d->text, d->opt[d->chosen], d->result);
    else
        snprintf(body, sizeof body, "%s\n\nDecisão: %s\nTeste: d20 = %d  + %d (%s) + %d (opção) = \001w%d\001d contra CD %d  ->  %s\n\n%s",
                 d->text, d->opt[d->chosen], d->roll, d->base_mod, d->mod_text, d->optbonus[d->chosen], d->total, d->dc,
                 d->outcome == 2 ? "\001gSUCESSO CRÍTICO\001d" : d->outcome == 1 ? "\001gSUCESSO\001d" : d->outcome == 3 ? "\001rFALHA CRÍTICA\001d" : "\001rFALHA\001d",
                 d->result);
    game_msg(g, MSG_EVENT, d->title, "%s", body);
    d->active = 0;
}

void decision_autoresolve(Game *g) {
    Decision *d = &g->ev.dec;
    if (!d->active) return;
    if (d->stage == 0) decision_choose(g, d->defopt);
    decision_close(g);
}

/* ---------------- simple events ---------------- */
static void simple_event(Game *g, int type, int bi) {
    Body *b = &g->body[bi];
    Rates r;
    body_rates(g, b, &r);
    char title[MSG_TITLE];
    switch (type) {
    case EV_STORM: {
        const char *name = "Tempestade";
        const char *txt = "Ventos fortes reduzem a produção de alimentos.";
        int kind = MOD_FOOD;
        double f = 0.8;
        switch (b->info.biome) {
        case BIO_DESERT: name = "Tempestade de areia"; txt = "Areia cobre painéis e entra nas brocas: -25% energia."; kind = MOD_ENERGY; f = 0.75; break;
        case BIO_ICE: name = "Nevasca"; txt = "Nevasca bloqueia geradores: -30% energia."; kind = MOD_ENERGY; f = 0.7; break;
        case BIO_OCEANIC: name = "Furacão"; txt = "Ventos de 300 km/h devastam as fazendas: -40% alimento."; f = 0.6; break;
        case BIO_JUNGLE: name = "Tempestade elétrica"; txt = "Raios sobrecarregam a rede: -20% energia."; kind = MOD_ENERGY; break;
        case BIO_TOXIC: name = "Chuva ácida"; txt = "Ácido corrói estruturas metálicas: -20% metal."; kind = MOD_METAL; break;
        case BIO_VOLCANIC: name = "Chuva de cinzas"; txt = "Cinzas bloqueiam a luz: -40% energia."; kind = MOD_ENERGY; f = 0.6; break;
        case BIO_TUNDRA: name = "Ventania glacial"; txt = "Frio extremo atinge as estufas: -30% alimento."; f = 0.7; break;
        case BIO_SAVANNA: name = "Incêndio na pradaria"; txt = "Fogo se espalha pelas plantações: -40% alimento."; f = 0.6; break;
        case BIO_BARREN: name = "Tempestade de poeira"; txt = "Poeira fina cobre tudo: -20% energia."; kind = MOD_ENERGY; break;
        default: break;
        }
        double h = rng_uniform(&g->rng, 2, 6);
        body_add_mod(b, kind, f, hours(g, h));
        snprintf(title, sizeof title, "%s em %s", name, b->name);
        game_msg(g, MSG_EVENT, title, "%s\nDuração: %.0fh.", txt, h);
        game_toast(g, RGB(245, 150, 60), "%s em %s", name, b->name);
    } break;
    case EV_BLIGHT: {
        double f = g->tech[TI(T_BIOTECH)] >= 3 ? 0.75 : 0.5;
        body_add_mod(b, MOD_FOOD, f, hours(g, 4));
        snprintf(title, sizeof title, "Praga agrícola em %s", b->name);
        game_msg(g, MSG_EVENT, title, "Fungos atacam as cúpulas de cultivo: alimento -%d%% por 4h.%s", (int)((1 - f) * 100),
                 f > 0.5 ? " (Biotecnologia reduziu o impacto)" : " Biotecnologia 3 reduziria o impacto.");
        game_toast(g, RGB(245, 150, 60), "Praga agrícola em %s", b->name);
    } break;
    case EV_MIGRANTS: {
        double add = floor(r.pop_cap * rng_uniform(&g->rng, 0.05, 0.15));
        if (b->pop + add > r.pop_cap) add = floor(r.pop_cap - b->pop);
        if (add < 1) add = 1;
        b->pop += add;
        snprintf(title, sizeof title, "Imigrantes chegam a %s", b->name);
        game_msg(g, MSG_EVENT, title, "Uma caravana de refugiados pediu abrigo. %.0f novos habitantes!", add);
        game_toast(g, RGB(110, 215, 110), "+%.0f imigrantes em %s", add, b->name);
    } break;
    case EV_VEIN: {
        int k = rng_chance(&g->rng, 0.6) ? MOD_METAL : MOD_CRYSTAL;
        body_add_mod(b, k, 1.25, hours(g, 6));
        snprintf(title, sizeof title, "Veio mineral em %s", b->name);
        game_msg(g, MSG_EVENT, title, "Geólogos encontraram um veio de %s de alta pureza: +25%% por 6h.", k == MOD_METAL ? "metal" : "cristal");
        game_toast(g, RGB(110, 215, 110), "Veio de %s descoberto!", k == MOD_METAL ? "metal" : "cristal");
    } break;
    case EV_COMET: {
        double c = floor(fmax(500, r.prod[RES_CRYSTAL] * rng_uniform(&g->rng, 1.5, 3.0)));
        double d = floor(fmax(250, (r.gross[RES_DEUT] + 50) * rng_uniform(&g->rng, 1.5, 3.0)));
        b->res[RES_CRYSTAL] += c;
        b->res[RES_DEUT] += d;
        snprintf(title, sizeof title, "Cometa capturado por %s", b->name);
        game_msg(g, MSG_EVENT, title, "Um pequeno cometa foi capturado em órbita e minerado: +%.0f cristal, +%.0f deutério.", c, d);
        game_toast(g, RGB(110, 215, 110), "Cometa minerado: +%.0f cristal", c);
    } break;
    case EV_MERCHANT:
        g->ev.merchant_until = hours(g, 2);
        game_msg(g, MSG_EVENT, "Mercador viajante", "Um mercador nômade estacionou no sistema. Trocas no Mercador sem taxa por 2h!");
        game_toast(g, RGB(245, 210, 90), "Mercador viajante: trocas sem taxa por 2h!");
        break;
    case EV_ANOMALY: {
        double dm = floor(rng_uniform(&g->rng, 100, 600));
        g->dm += dm;
        game_msg(g, MSG_EVENT, "Anomalia de matéria negra", "Sensores captaram uma anomalia próxima a %s. Coletores extraíram %.0f Matéria Negra.", b->name, dm);
        game_toast(g, RGB(200, 130, 235), "+%.0f Matéria Negra (anomalia)", dm);
    } break;
    case EV_SCIENCE:
        if (g->rq.n > 0 && g->rq.it[0].active) {
            double rem = g->rq.it[0].end - g->now;
            double cut = rem * rng_uniform(&g->rng, 0.2, 0.4);
            g->rq.it[0].end -= cut;
            char t[32];
            fmt_time(t, sizeof t, cut);
            game_msg(g, MSG_EVENT, "Descoberta científica", "Um insight brilhante acelerou a pesquisa de %s em %s!", ITEMS[g->rq.it[0].item].name, t);
            game_toast(g, RGB(100, 215, 235), "Pesquisa acelerada em %s", t);
        }
        break;
    case EV_UNREST: {
        body_add_mod(b, MOD_METAL, 0.75, hours(g, 3)); body_add_mod(b, MOD_CRYSTAL, 0.75, hours(g, 3)); body_add_mod(b, MOD_DEUT, 0.75, hours(g, 3));
        double l = floor(b->pop * 0.03);
        b->pop -= l;
        snprintf(title, sizeof title, "Revolta popular em %s", b->name);
        game_msg(g, MSG_EVENT, title, "Sem comida, a população se revoltou! Produção -25%% por 3h e %.0f habitantes deixaram o planeta.\nConstrua Fazendas ou importe alimento por rotas comerciais.", l);
        game_toast(g, RGB(235, 85, 70), "Revolta por fome em %s!", b->name);
    } break;
    case EV_MISSILES: {
        int n = rng_range(&g->rng, 2, 4) * (1 + tier(g) / 3);
        long long abm = b->def[DI(D_ABM)];
        int inter = (int)(abm < n ? abm : n);
        b->def[DI(D_ABM)] -= inter;
        int hit = n - inter;
        char res[300] = "";
        for (int k = 0; k < hit; k++) {
            int cand[N_CDEF], m = 0;
            for (int i = 0; i < N_CDEF; i++) if (b->def[i] > 0 && S_END + i != D_SMALL_DOME && S_END + i != D_LARGE_DOME) cand[m++] = i;
            if (!m) break;
            int i = cand[rng_range(&g->rng, 0, m - 1)];
            double hull = ITEMS[S_END + i].si * (1 + 0.1 * g->tech[TI(T_ARMOR)]);
            long long kill = (long long)floor(12000.0 * 10 / hull) + 1;
            if (kill > b->def[i]) kill = b->def[i];
            b->def[i] -= kill;
        }
        snprintf(res, sizeof res, "%d mísseis interplanetários atingiram %s. %d foram interceptados por antibalísticos; %d destruíram defesas.", n, b->name, inter, hit);
        game_msg(g, MSG_COMBAT, "Ataque de mísseis!", "%s", res);
        game_toast(g, RGB(235, 85, 70), "Ataque de mísseis em %s!", b->name);
    } break;
    }
}

static void probes_event(Game *g, int bi) {
    Body *b = &g->body[bi];
    int npi = pick_attacker(g, b, NPC_PIRATE);
    const NpcPlanet *pl = &g->npl[npi];
    const Npc *n = &g->npc[pl->npc];
    double v = expected_raid(g, b) * rng_uniform(&g->rng, 0.6, 1.4);
    double mil = body_military_value(b);
    int mydef = (int)(b->def[0] + b->def[1] + b->def[2] + b->def[3] + b->def[4] + b->def[5]);
    double kill = clampd(mil / (v + 1.0) * 0.4 + 0.08 * g->tech[TI(T_ESPIONAGE)] - 0.1, 0.0, 0.85);
    if (mydef == 0 && mil < 1) kill = 0;
    char title[MSG_TITLE];
    snprintf(title, sizeof title, "Sondas detectadas sobre %s", b->name);
    if (rng_double(&g->rng) < kill) {
        Debris *d = debris_at(g, b->g, b->s, b->p, 1);
        d->c += 300.0 * rng_range(&g->rng, 1, 5);
        game_msg(g, MSG_SPY, title, "Sondas de \001y%s\001d [%d:%d:%d] tentaram espionar %s, mas foram \001gabatidas\001d pelas suas defesas.",
                 n->name, pl->g, pl->s, pl->p, b->name);
        game_toast(g, RGB(110, 215, 110), "Sondas inimigas abatidas sobre %s", b->name);
        return;
    }
    game_msg(g, MSG_SPY, title, "Sondas de \001y%s\001d [%d:%d:%d] (%s) espionaram %s.\nEles agora conhecem seus recursos e defesas. Espere visitas...",
             n->name, pl->g, pl->s, pl->p, npc_type_name(n->type), b->name);
    game_toast(g, RGB(245, 210, 90), "Sondas inimigas espionaram %s", b->name);
    if (mil > v * 1.6) {
        game_msg(g, MSG_SPY, "Ataque dissuadido", "Os relatórios mostram que %s está bem defendido. %s desistiu de atacar.", b->name, n->name);
        return;
    }
    double st = stored(g, b);
    double p_attack = clampd(0.3 + 0.5 * st / (st + mil + 5000.0), 0.15, 0.8);
    if (g->ev.offline && g->ev.offline_hostiles >= 2) p_attack = 0;
    if (rng_double(&g->rng) < p_attack) launch_hostile(g, bi, npi, HK_PIRATE, v);
}

static void attack_event(Game *g, int bi, int xeno) {
    Body *b = &g->body[bi];
    if (g->ev.offline && g->ev.offline_hostiles >= 2) { simple_event(g, EV_STORM, bi); return; }
    int npi = pick_attacker(g, b, xeno ? NPC_XENO : NPC_PIRATE);
    double v = expected_raid(g, b) * (xeno ? rng_uniform(&g->rng, 1.5, 2.5) : rng_uniform(&g->rng, 0.5, 1.3));
    launch_hostile(g, bi, npi, xeno ? HK_XENO : HK_PIRATE, v);
}

void events_process(Game *g) {
    EventState *e = &g->ev;
    if (e->dec.active && g->now >= e->dec.deadline) decision_autoresolve(g);
    if (g->now < e->next_event) return;
    e->total++;
    g->away_events++;
    int bi = pick_body(g);
    Body *b = &g->body[bi];
    Rates r;
    body_rates(g, b, &r);
    double pts = points(g);
    int newbie = newbie_left(g) > 0;
    double w[EV_COUNT] = {0};
    int bio = b->info.biome;
    if (pts >= 50) {
        w[EV_EARTHQUAKE] = 5 + (bio == BIO_VOLCANIC ? 6 : 0) + (bio == BIO_BARREN ? 2 : 0);
        w[EV_METEOR] = 5 + ((b->info.atmo == ATM_NONE || b->info.atmo == ATM_THIN) ? 4 : 0);
        if (b->ship[SI(S_SATELLITE)] > 0 || b->bld[B_SOLAR_PLANT] > 0) w[EV_FLARE] = 4 + (b->p <= 5 ? 3 : 0) + (b->info.magneto == 0 ? 3 : 0);
        if (b->pop > 400) w[EV_PLAGUE] = 3 + (bio == BIO_JUNGLE ? 3 : 0) + (bio == BIO_TOXIC ? 2 : 0) + (bio == BIO_OCEANIC ? 1 : 0);
        w[EV_STORM] = 5;
        if (b->bld[B_FARM] > 0) w[EV_BLIGHT] = 3 + ((bio == BIO_JUNGLE || bio == BIO_SAVANNA) ? 1 : 0);
        if (b->bld[B_FUSION] > 0) w[EV_REACTOR] = 3;
        w[EV_DERELICT] = 3;
    }
    if (b->res[RES_FOOD] < 1 && r.prod[RES_FOOD] < 0) w[EV_UNREST] = 10;
    if (b->pop < r.pop_cap * 0.9) w[EV_MIGRANTS] = 5;
    w[EV_VEIN] = 4;
    w[EV_COMET] = 3;
    w[EV_MERCHANT] = 2;
    w[EV_ANOMALY] = 2;
    if (g->rq.n > 0 && g->rq.it[0].active) w[EV_SCIENCE] = 3;
    if (!newbie) {
        double ramp = clampd((pts - NEWBIE_POINTS) / 500.0, 0.3, 1.0);
        w[EV_PROBES] = 12 * ramp;
        w[EV_ATTACK] = 7 * ramp;
        if (pts > 20000) w[EV_XENO] = 3;
        if (pts > 30000) w[EV_MISSILES] = 2;
    }
    /* avoid stacking: no new decision while one is pending */
    if (e->dec.active) { w[EV_EARTHQUAKE] = w[EV_METEOR] = w[EV_FLARE] = w[EV_PLAGUE] = w[EV_REACTOR] = w[EV_DERELICT] = 0; }
    /* avoid two hostile fleets at once on the same body */
    for (int i = 0; i < MAX_FLEETS; i++)
        if (g->fleet[i].used && g->fleet[i].mission == M_HOSTILE) { w[EV_ATTACK] *= 0.2; w[EV_XENO] *= 0.2; w[EV_PROBES] *= 0.5; }
    double tot = 0;
    for (int i = 0; i < EV_COUNT; i++) tot += w[i];
    int type = EV_VEIN;
    double x = rng_double(&g->rng) * tot;
    for (int i = 0; i < EV_COUNT; i++) { if (x < w[i]) { type = i; break; } x -= w[i]; }
    switch (type) {
    case EV_PROBES: probes_event(g, bi); break;
    case EV_ATTACK: attack_event(g, bi, 0); break;
    case EV_XENO: attack_event(g, bi, 1); break;
    case EV_EARTHQUAKE: case EV_METEOR: case EV_FLARE: case EV_PLAGUE: case EV_REACTOR: case EV_DERELICT:
        if (make_decision(g, type, bi)) {
            if (e->offline) decision_autoresolve(g);
            else game_toast(g, RGB(245, 210, 90), "EVENTO: %s", e->dec.title);
        }
        break;
    default: simple_event(g, type, bi); break;
    }
    events_schedule(g, 0);
}

void route_ambush(Game *g, int ri, double *keep) {
    Route *rt = &g->route[ri];
    *keep = 1.0;
    double cargo = rt->load[0] + rt->load[1] + rt->load[2] + rt->load[3];
    double v = 0.35 * cargo + 1000.0 * points(g) * rng_uniform(&g->rng, 0.02, 0.06) + 5000;
    long long en[N_SHIP] = {0};
    compose_fleet(v, points(g) < 500 ? 400 : points(g), en);
    en[SI(S_PROBE)] = 0;
    CombatIn in;
    memset(&in, 0, sizeof in);
    for (int i = 0; i < N_SHIP; i++) { in.n[0][i] = en[i]; in.n[1][i] = rt->ship[i]; }
    in.wt[0] = clampi(g->tech[TI(T_WEAPONS)] - 1, 0, 40); in.st[0] = clampi(g->tech[TI(T_SHIELDING)] - 1, 0, 40); in.at[0] = clampi(g->tech[TI(T_ARMOR)] - 1, 0, 40);
    in.wt[1] = g->tech[TI(T_WEAPONS)]; in.st[1] = g->tech[TI(T_SHIELDING)]; in.at[1] = g->tech[TI(T_ARMOR)];
    CombatOut out;
    combat_run(&g->rng, &in, &out);
    double cap0 = route_capacity(g, rt);
    long long lost = 0;
    for (int i = 0; i < N_SHIP; i++) { lost += rt->ship[i] - out.end[1][i]; rt->ship[i] = out.end[1][i]; }
    rt->lost_ships += lost;
    rt->ambushes++;
    double cap1 = route_capacity(g, rt);
    if (out.winner == 0) *keep = 0.0;
    else *keep = cap0 > 0 ? cap1 / cap0 : 0;
    Debris *d = debris_at(g, rt->tg, rt->ts, rt->tp, 1);
    d->m += out.debris[0]; d->c += out.debris[1];
    char body[MSG_BODY];
    combat_report(&in, &out, "Piratas saqueadores", "Comboio comercial", body, sizeof body);
    char title[MSG_TITLE];
    snprintf(title, sizeof title, "Emboscada na rota #%d: %s", ri + 1, out.winner == 1 ? "comboio salvo" : "carga perdida");
    game_msg(g, MSG_TRADE, title, "Piratas atacaram seu comboio rumo a [%d:%d:%d].\nCarga preservada: %.0f%%\n\n%s\nDica: escoltas militares na rota reduzem as perdas.",
             rt->tg, rt->ts, rt->tp, *keep * 100.0, body);
    game_toast(g, out.winner == 1 ? RGB(245, 210, 90) : RGB(235, 85, 70), "Emboscada pirata na rota #%d!", ri + 1);
}
