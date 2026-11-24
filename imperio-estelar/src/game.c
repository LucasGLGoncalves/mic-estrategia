/* game.c - new game, bodies, quests (guided missions) */
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *res_name(int r) {
    static const char *N[NRES] = {"Metal", "Cristal", "Deutério", "Alimento"};
    return (r >= 0 && r < NRES) ? N[r] : "?";
}

void make_seedstr(char *out, size_t n, double t) {
    time_t tt = (time_t)t;
    struct tm tmv;
#ifdef _WIN32
    { struct tm *lt = localtime(&tt); if (lt) tmv = *lt; else memset(&tmv, 0, sizeof tmv); }
#else
    localtime_r(&tt, &tmv);
#endif
    /* ano dia mes minuto hora segundo */
    snprintf(out, n, "%04d%02d%02d%02d%02d%02d", tmv.tm_year + 1900, tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_min, tmv.tm_hour, tmv.tm_sec);
}

int body_new(Game *g, const char *name, int gg, int ss, int pp, int is_moon, const PlanetInfo *pi) {
    int slot = -1;
    for (int i = 0; i < g->nbody; i++) if (!g->body[i].used) { slot = i; break; }
    if (slot < 0) {
        if (g->nbody >= MAX_BODIES) return -1;
        slot = g->nbody++;
    }
    Body *b = &g->body[slot];
    memset(b, 0, sizeof *b);
    b->used = 1;
    b->is_moon = is_moon;
    b->parent = -1;
    b->moon = -1;
    snprintf(b->name, sizeof b->name, "%s", name);
    b->g = gg; b->s = ss; b->p = pp;
    b->info = *pi;
    for (int i = 0; i < N_BLD; i++) b->pct[i] = 100;
    b->sat_pct = 100;
    b->crawler_pct = 100;
    b->founded = g->now;
    return slot;
}

void create_moon(Game *g, int planet, int diam) {
    Body *p = &g->body[planet];
    if (p->moon >= 0) return;
    PlanetInfo mi;
    memset(&mi, 0, sizeof mi);
    mi.biome = BIO_MOON;
    mi.atmo = ATM_NONE;
    mi.diameter = diam;
    mi.fields = 1;
    mi.tmax = p->info.tmax - 40;
    mi.tmin = p->info.tmin - 40;
    mi.gravity = diam / 12800.0 * 0.6;
    mi.daylen = p->info.daylen * 28.0;
    mi.vseed = hash64(p->info.vseed ^ 0x4D4F4F4EULL);
    mi.rich[0] = mi.rich[1] = mi.rich[2] = 1.0;
    char name[32];
    snprintf(name, sizeof name, "Lua de %.20s", p->name);
    int mi_idx = body_new(g, name, p->g, p->s, p->p, 1, &mi);
    if (mi_idx < 0) return;
    p = &g->body[planet];
    p->moon = mi_idx;
    p->info.has_moon = 1;
    p->info.moon_diam = diam;
    g->body[mi_idx].parent = planet;
}

void game_new(Game *g, const char *name, const char *home, int speed, const char *seedstr) {
    universe_free(g);
    memset(g, 0, sizeof *g);
    snprintf(g->name, sizeof g->name, "%s", name);
    snprintf(g->seedstr, sizeof g->seedstr, "%s", seedstr);
    uint64_t h = fnv1a(seedstr, strlen(seedstr), 0);
    h = fnv1a(name, strlen(name), h);
    g->seed = hash64(h);
    g->speed = speed < 1 ? 1 : speed;
    rng_seed(&g->rng, g->seed);
    g->now = now_seconds();
    g->created = g->now;
    g->last_save = g->now;
    g->last_seen = g->now;
    int gg = rng_range(&g->rng, 1, UNI_GALAXIES);
    int ss = rng_range(&g->rng, 1, UNI_SYSTEMS);
    int pp = rng_range(&g->rng, 4, 12);
    PlanetInfo pi;
    planet_generate(g->seed, gg, ss, pp, 1, &pi);
    int bi = body_new(g, home && home[0] ? home : "Planeta Natal", gg, ss, pp, 0, &pi);
    Body *b = &g->body[bi];
    b->res[RES_METAL] = 1000;
    b->res[RES_CRYSTAL] = 1000;
    b->res[RES_DEUT] = 200;
    b->res[RES_FOOD] = 500;
    b->pop = 210;
    g->dm = 8000;
    g->cur = 0;
    g->rq_body = 0;
    if (pi.has_moon) create_moon(g, bi, pi.moon_diam);
    universe_build(g);
    events_schedule(g, 1);
    game_msg(g, MSG_INFO, "Bem-vindo, Comandante!",
             "Comandante \001y%s\001d, seu império começa em \001w%s\001d [%d:%d:%d], um mundo %s.\n\n"
             "Tudo está interligado: minas precisam de \001yenergia\001d e \001wtrabalhadores\001d; trabalhadores precisam de "
             "\001gmoradia\001d e \001galimento\001d; alimento vem das fazendas, que também consomem energia.\n\n"
             "Siga a \001cPróxima missão\001d (canto superior esquerdo) para aprender o caminho. "
             "Você está sob \001gproteção de novato\001d até %0.f pontos. Depois disso, piratas, desastres e invasores "
             "vão testar seu império constantemente.\n\nTecla H abre a ajuda. Boa sorte!",
             g->name, b->name, gg, ss, pp, BIOME_NAME[pi.biome], NEWBIE_POINTS);
}

void game_rename(Game *g, int bi, const char *name) {
    if (bi < 0 || bi >= g->nbody || !name[0]) return;
    snprintf(g->body[bi].name, sizeof g->body[bi].name, "%s", name);
}

int game_abandon(Game *g, int bi) {
    if (bi <= 0 || bi >= g->nbody || !g->body[bi].used) return 0;
    Body *b = &g->body[bi];
    if (!b->is_moon && b->moon >= 0) game_abandon(g, b->moon);
    if (b->is_moon && b->parent >= 0) g->body[b->parent].moon = -1;
    for (int i = 0; i < MAX_ROUTES; i++)
        if (g->route[i].used && (g->route[i].origin == bi || (g->route[i].kind == RT_INTERNAL && g->route[i].dest == bi))) {
            if (g->route[i].origin == bi) g->route[i].origin = 0;
            route_delete(g, i);
        }
    for (int i = 0; i < MAX_FLEETS; i++) {
        Fleet *f = &g->fleet[i];
        if (!f->used) continue;
        if ((f->mission == M_HOSTILE || f->mission == M_HOSTILE_SPY) && f->origin == bi) f->used = 0;
        else if (f->origin == bi) f->origin = 0;
    }
    if (g->rq.n > 0 && g->rq_body == bi) g->rq.n = 0;
    b->used = 0;
    if (g->cur == bi) g->cur = 0;
    return 1;
}

/* ---------------- quests ---------------- */
enum { Q_BLD, Q_TECH, Q_SHIP, Q_DEF, Q_FLAG, Q_POINTS };
typedef struct { const char *title; int kind, id; double target; double rw[4]; double dm; } Quest;

static const Quest QUESTS[] = {
    {"Construa a Usina Solar", Q_BLD, B_SOLAR_PLANT, 1, {150, 75, 0, 0}, 0},
    {"Mina de Metal nível 2", Q_BLD, B_METAL_MINE, 2, {200, 100, 0, 0}, 0},
    {"Mina de Cristal nível 1", Q_BLD, B_CRYSTAL_MINE, 1, {150, 150, 0, 0}, 0},
    {"Mina de Metal nível 4", Q_BLD, B_METAL_MINE, 4, {400, 200, 0, 0}, 0},
    {"Setor Residencial nível 1", Q_BLD, B_HABITAT, 1, {300, 150, 0, 0}, 0},
    {"Fazenda Biosfera nível 1", Q_BLD, B_FARM, 1, {300, 200, 0, 200}, 0},
    {"Usina Solar nível 4", Q_BLD, B_SOLAR_PLANT, 4, {500, 250, 0, 0}, 0},
    {"Sintetizador de Deutério nível 2", Q_BLD, B_DEUT_SYNTH, 2, {500, 300, 100, 0}, 0},
    {"Fábrica de Robôs nível 2", Q_BLD, B_ROBOTICS, 2, {800, 400, 200, 0}, 0},
    {"Laboratório de Pesquisa nível 1", Q_BLD, B_LAB, 1, {500, 800, 300, 0}, 0},
    {"Pesquise Tecnologia de Energia", Q_TECH, T_ENERGY, 1, {0, 600, 400, 0}, 0},
    {"Construa o Estaleiro", Q_BLD, B_SHIPYARD, 1, {1000, 500, 0, 0}, 0},
    {"Construa 5 Lançadores de Mísseis", Q_DEF, D_ROCKET, 5, {2000, 500, 0, 0}, 0},
    {"Motor de Combustão nível 2", Q_TECH, T_COMBUSTION, 2, {1000, 0, 600, 0}, 0},
    {"Construa um Cargueiro Pequeno", Q_SHIP, S_SMALL_CARGO, 1, {2000, 2000, 0, 0}, 0},
    {"Atinja 250 pontos (fim da proteção)", Q_POINTS, 0, 250, {0, 0, 0, 0}, 500},
    {"Construa uma Sonda de Espionagem", Q_SHIP, S_PROBE, 1, {0, 1000, 500, 0}, 0},
    {"Espione outro império (Galáxia)", Q_FLAG, FL_SPIED, 1, {0, 0, 0, 0}, 300},
    {"Construa a Estação Orbital", Q_BLD, B_ORBITAL, 1, {3000, 2000, 1000, 0}, 0},
    {"Pesquise Astrofísica", Q_TECH, T_ASTRO, 1, {5000, 5000, 2000, 0}, 0},
    {"Envie uma expedição (posição 16)", Q_FLAG, FL_EXPED, 1, {0, 0, 0, 0}, 500},
    {"Colonize um novo planeta", Q_FLAG, FL_COLONIZED, 1, {10000, 10000, 5000, 2000}, 0},
    {"Pesquise Logística", Q_TECH, T_LOGISTICS, 1, {2000, 2000, 0, 0}, 0},
    {"Construa o Porto Comercial", Q_BLD, B_TRADE_HUB, 1, {3000, 2000, 0, 0}, 0},
    {"Crie uma rota comercial", Q_FLAG, FL_ROUTE, 1, {0, 0, 0, 0}, 1000},
    {"Vença uma batalha defensiva", Q_FLAG, FL_DEFENDED, 1, {0, 0, 0, 0}, 1000},
    {"Recicle um campo de destroços", Q_FLAG, FL_RECYCLED, 1, {0, 0, 0, 0}, 500},
    {"Atinja 5.000 pontos", Q_POINTS, 0, 5000, {0, 0, 0, 0}, 2000},
    {"Construa uma Base Lunar", Q_BLD, B_LUNAR_BASE, 1, {20000, 20000, 10000, 0}, 0},
    {"Atinja 50.000 pontos", Q_POINTS, 0, 50000, {0, 0, 0, 0}, 5000},
};
#define NQ ((int)(sizeof QUESTS / sizeof QUESTS[0]))

int quest_count(void) { return NQ; }

const char *quest_title(const Game *g, int q, char *buf, size_t n) {
    (void)g;
    if (q < 0 || q >= NQ) { snprintf(buf, n, "Todas as missões concluídas!"); return buf; }
    snprintf(buf, n, "%s", QUESTS[q].title);
    return buf;
}

int quest_progress(const Game *g, int q, double *have, double *need) {
    if (q < 0 || q >= NQ) { *have = *need = 1; return 1; }
    const Quest *Q = &QUESTS[q];
    double h = 0;
    switch (Q->kind) {
    case Q_BLD:
        for (int i = 0; i < g->nbody; i++) if (g->body[i].used && g->body[i].bld[Q->id] > h) h = g->body[i].bld[Q->id];
        break;
    case Q_TECH: h = g->tech[TI(Q->id)]; break;
    case Q_SHIP: for (int i = 0; i < g->nbody; i++) if (g->body[i].used) h += (double)g->body[i].ship[SI(Q->id)]; break;
    case Q_DEF: for (int i = 0; i < g->nbody; i++) if (g->body[i].used) h += (double)g->body[i].def[DI(Q->id)]; break;
    case Q_FLAG: h = (g->flags & (unsigned)Q->id) ? 1 : 0; break;
    case Q_POINTS: h = floor(player_points(g, NULL, NULL, NULL, NULL)); break;
    }
    *have = h;
    *need = Q->target;
    return h >= Q->target;
}

void quest_reward_text(int q, char *buf, size_t n) {
    buf[0] = 0;
    if (q < 0 || q >= NQ) return;
    const Quest *Q = &QUESTS[q];
    for (int i = 0; i < 4; i++) {
        if (Q->rw[i] <= 0) continue;
        size_t L = strlen(buf);
        char a[32];
        fmt_short(a, sizeof a, Q->rw[i]);
        snprintf(buf + L, n - L, "%s%s %s", L ? ", " : "", a, i == 0 ? "M" : i == 1 ? "C" : i == 2 ? "D" : "Alim");
    }
    if (Q->dm > 0) {
        size_t L = strlen(buf);
        snprintf(buf + L, n - L, "%s%.0f MN", L ? ", " : "", Q->dm);
    }
}

void quest_check(Game *g) {
    for (int k = 0; k < 4 && g->quest < NQ; k++) {
        double h, n;
        if (!quest_progress(g, g->quest, &h, &n)) return;
        const Quest *Q = &QUESTS[g->quest];
        Body *b = &g->body[0];
        for (int i = 0; i < NRES; i++) b->res[i] += Q->rw[i];
        g->dm += Q->dm;
        char rw[96];
        quest_reward_text(g->quest, rw, sizeof rw);
        game_msg(g, MSG_QUEST, "Missão concluída", "\001g%s\001d\nRecompensa: %s", Q->title, rw);
        game_toast(g, RGB(245, 210, 90), "Missão concluída: %s (+%s)", Q->title, rw);
        g->quest++;
    }
}
