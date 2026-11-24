/* save.c - text save format (versioned, atomic write) */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

static void wstr(FILE *f, const char *s) {
    fputc('"', f);
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '"' || *p == '\\' || *p < 32) fprintf(f, "\\x%02x", *p);
        else fputc(*p, f);
    }
    fputc('"', f);
}

static void wq(FILE *f, const Queue *q) {
    fprintf(f, " %d", q->n);
    for (int i = 0; i < q->n; i++) fprintf(f, " %d %lld %.17g %.17g %d", q->it[i].item, q->it[i].amount, q->it[i].start, q->it[i].end, q->it[i].active);
    fputc('\n', f);
}

int game_save(Game *g, const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return 0;
    g->last_save = g->now;
    fprintf(f, "%s %d\n", SAVE_MAGIC, SAVE_VERSION);
    fprintf(f, "name "); wstr(f, g->name); fputc('\n', f);
    fprintf(f, "seedstr "); wstr(f, g->seedstr); fputc('\n', f);
    fprintf(f, "seed %llu speed %d\n", (unsigned long long)g->seed, g->speed);
    fprintf(f, "times %.17g %.17g %.17g %.17g\n", g->created, g->now, g->last_save, g->last_seen);
    fprintf(f, "dm %.17g %.17g cur %d rqb %d quest %d flags %u\n", g->dm, g->honour, g->cur, g->rq_body, g->quest, g->flags);
    fprintf(f, "tech");
    for (int i = 0; i < N_TECH; i++) fprintf(f, " %d", g->tech[i]);
    fprintf(f, "\nofficer");
    for (int i = 0; i < N_OFF; i++) fprintf(f, " %.17g", g->officer[i]);
    fprintf(f, "\nrq");
    wq(f, &g->rq);
    fprintf(f, "rng %llu %llu %llu %llu\n", (unsigned long long)g->rng.s[0], (unsigned long long)g->rng.s[1],
            (unsigned long long)g->rng.s[2], (unsigned long long)g->rng.s[3]);
    fprintf(f, "ev %.17g %.17g %lld %.17g\n", g->ev.next_event, g->ev.last_hostile, g->ev.total, g->ev.merchant_until);
    const Decision *d = &g->ev.dec;
    fprintf(f, "dec %d %d %d ", d->active, d->type, d->body);
    wstr(f, d->title); fputc(' ', f); wstr(f, d->text); fprintf(f, " %d", d->nopt);
    for (int i = 0; i < 3; i++) {
        fputc(' ', f); wstr(f, d->opt[i]);
        for (int k = 0; k < NRES; k++) fprintf(f, " %.17g", d->optcost[i][k]);
        fprintf(f, " %.17g %d", d->optdm[i], d->optbonus[i]);
    }
    fprintf(f, " %d %d ", d->dc, d->base_mod);
    wstr(f, d->mod_text);
    fprintf(f, " %.17g %d %d %d %d %d %d ", d->deadline, d->defopt, d->chosen, d->roll, d->total, d->outcome, d->rerolls);
    wstr(f, d->result);
    fprintf(f, " %d %d\n", d->stage, d->tier);
    fprintf(f, "stats %.17g %.17g %.17g %.17g %.17g %.17g\n", g->stat_produced[0], g->stat_produced[1], g->stat_produced[2],
            g->stat_produced[3], g->stat_combats, g->stat_wins);
    fprintf(f, "bodies %d\n", g->nbody);
    for (int i = 0; i < g->nbody; i++) {
        const Body *b = &g->body[i];
        const PlanetInfo *p = &b->info;
        fprintf(f, "body %d %d %d %d ", b->used, b->is_moon, b->parent, b->moon);
        wstr(f, b->name);
        fprintf(f, " %d %d %d\n", b->g, b->s, b->p);
        fprintf(f, "info %d %d %d %d %d %d %.17g %.17g %.17g %.17g %.17g %d %d %.17g %.17g %.17g %d %d %llu %d %d\n",
                p->tmin, p->tmax, p->diameter, p->fields, p->biome, p->atmo, p->pressure, p->gravity, p->daylen, p->water,
                p->fertility, p->radiation, p->magneto, p->rich[0], p->rich[1], p->rich[2], p->has_moon, p->moon_diam,
                (unsigned long long)p->vseed, p->ring, p->climate);
        fprintf(f, "res %.17g %.17g %.17g %.17g %.17g\n", b->res[0], b->res[1], b->res[2], b->res[3], b->pop);
        fprintf(f, "bld");
        for (int k = 0; k < N_BLD; k++) fprintf(f, " %d", b->bld[k]);
        fprintf(f, "\npct");
        for (int k = 0; k < N_BLD; k++) fprintf(f, " %d", b->pct[k]);
        fprintf(f, " %d %d\nship", b->sat_pct, b->crawler_pct);
        for (int k = 0; k < N_SHIP; k++) fprintf(f, " %lld", b->ship[k]);
        fprintf(f, "\ndef");
        for (int k = 0; k < N_DEF; k++) fprintf(f, " %lld", b->def[k]);
        fprintf(f, "\nbq");
        wq(f, &b->bq);
        fprintf(f, "lq");
        wq(f, &b->lq);
        fprintf(f, "sq %d", b->nsq);
        for (int k = 0; k < b->nsq; k++) fprintf(f, " %d %lld %.17g %.17g", b->sq[k].item, b->sq[k].count, b->sq[k].unit, b->sq[k].next);
        fprintf(f, "\nmod");
        for (int k = 0; k < MAX_MODS; k++) fprintf(f, " %d %.17g %.17g", b->mod[k].kind, b->mod[k].factor, b->mod[k].until);
        fprintf(f, "\nbt %.17g %.17g %.17g\n", b->founded, b->last_attacked, b->gate_ready);
    }
    int nf = 0;
    for (int i = 0; i < MAX_FLEETS; i++) if (g->fleet[i].used) nf++;
    fprintf(f, "fleets %d\n", nf);
    for (int i = 0; i < MAX_FLEETS; i++) {
        const Fleet *fl = &g->fleet[i];
        if (!fl->used) continue;
        fprintf(f, "fleet %d %d %d %d %d %d %d %d %d %d %d", fl->mission, fl->state, fl->origin, fl->og, fl->os, fl->op, fl->otype,
                fl->tg, fl->ts, fl->tp, fl->ttype);
        for (int k = 0; k < N_SHIP; k++) fprintf(f, " %lld", fl->ship[k]);
        for (int k = 0; k < NRES; k++) fprintf(f, " %.17g", fl->cargo[k]);
        fprintf(f, " %.17g %.17g %.17g %.17g %d %d %d %d %d %d\n", fl->depart, fl->arrive, fl->hold, fl->ret, fl->speed, fl->npc, fl->hkind,
                fl->wt, fl->st, fl->at);
    }
    int nr = 0;
    for (int i = 0; i < MAX_ROUTES; i++) if (g->route[i].used) nr++;
    fprintf(f, "routes %d\n", nr);
    for (int i = 0; i < MAX_ROUTES; i++) {
        const Route *r = &g->route[i];
        if (!r->used) continue;
        fprintf(f, "route %d %d %d %d %d %d %d %d %d %d %.17g", i, r->kind, r->origin, r->dest, r->tg, r->ts, r->tp, r->npc, r->res_out, r->res_in, r->amount);
        for (int k = 0; k < N_SHIP; k++) fprintf(f, " %lld", r->ship[k]);
        fprintf(f, " %d %.17g %.17g %.17g", r->state, r->t_next, r->t_arrive, r->t_return);
        for (int k = 0; k < NRES; k++) fprintf(f, " %.17g %.17g", r->load[k], r->back[k]);
        fprintf(f, " %lld %lld %lld %.17g\n", r->trips, r->ambushes, r->lost_ships, r->delivered);
    }
    fprintf(f, "msgs %d\n", g->nmsg);
    for (int i = 0; i < g->nmsg; i++) {
        const Message *m = &g->msg[i];
        fprintf(f, "msg %.17g %d %d ", m->t, m->type, m->read);
        wstr(f, m->title); fputc(' ', f); wstr(f, m->body); fputc('\n', f);
    }
    fprintf(f, "debris %d\n", g->ndebris);
    for (int i = 0; i < g->ndebris; i++) fprintf(f, "d %d %d %d %.17g %.17g\n", g->debris[i].g, g->debris[i].s, g->debris[i].p, g->debris[i].m, g->debris[i].c);
    fprintf(f, "touched %d\n", g->ntouched);
    for (int i = 0; i < g->ntouched; i++) {
        const Touched *t = &g->touched[i];
        fprintf(f, "t %d %d %d %.17g %.17g %.17g %.17g\n", t->g, t->s, t->p, t->t_raid, t->def_dmg, t->fleet_dmg, t->t_dmg);
    }
    fprintf(f, "explored %d", g->nexplored);
    for (int i = 0; i < g->nexplored; i++) fprintf(f, " %d", g->explored[i]);
    fprintf(f, "\nend\n");
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) { remove(tmp); return 0; }
#ifdef _WIN32
    if (!MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING)) { remove(tmp); return 0; }
#else
    if (rename(tmp, path) != 0) { remove(tmp); return 0; }
#endif
    return 1;
}

/* ---------------- reader ---------------- */
typedef struct { FILE *f; int err; char tok[8192]; } Rd;

static const char *tok(Rd *r) {
    int c;
    r->tok[0] = 0;
    if (r->err) return r->tok;
    do { c = fgetc(r->f); } while (c == ' ' || c == '\n' || c == '\r' || c == '\t');
    if (c == EOF) { r->err = 1; return r->tok; }
    size_t n = 0;
    if (c == '"') {
        for (;;) {
            c = fgetc(r->f);
            if (c == EOF) { r->err = 1; break; }
            if (c == '"') break;
            if (c == '\\') {
                int x = fgetc(r->f);
                if (x == 'x') {
                    char h[3] = {0, 0, 0};
                    h[0] = (char)fgetc(r->f);
                    h[1] = (char)fgetc(r->f);
                    c = (int)strtol(h, NULL, 16);
                } else c = x;
            }
            if (n < sizeof r->tok - 1) r->tok[n++] = (char)c;
        }
    } else {
        while (c != EOF && c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            if (n < sizeof r->tok - 1) r->tok[n++] = (char)c;
            c = fgetc(r->f);
        }
    }
    r->tok[n] = 0;
    return r->tok;
}

static void expect(Rd *r, const char *label) {
    const char *t = tok(r);
    if (strcmp(t, label) != 0) r->err = 1;
}
static long long rl(Rd *r) { return strtoll(tok(r), NULL, 10); }
static int ri(Rd *r) { return (int)strtol(tok(r), NULL, 10); }
static double rd(Rd *r) { return strtod(tok(r), NULL); }
static unsigned long long ru(Rd *r) { return strtoull(tok(r), NULL, 10); }
static void rs(Rd *r, char *out, size_t n) { snprintf(out, n, "%s", tok(r)); }

static void rq_(Rd *r, Queue *q) {
    q->n = clampi(ri(r), 0, MAX_QUEUE);
    for (int i = 0; i < q->n; i++) {
        q->it[i].item = clampi(ri(r), 0, ITEM_COUNT - 1);
        q->it[i].amount = rl(r);
        q->it[i].start = rd(r);
        q->it[i].end = rd(r);
        q->it[i].active = ri(r);
    }
}

int game_load(Game *g, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    static Rd r;
    memset(&r, 0, sizeof r);
    r.f = f;
    universe_free(g);
    memset(g, 0, sizeof *g);
    expect(&r, SAVE_MAGIC);
    if (ri(&r) != SAVE_VERSION) r.err = 1;
    expect(&r, "name"); rs(&r, g->name, sizeof g->name);
    expect(&r, "seedstr"); rs(&r, g->seedstr, sizeof g->seedstr);
    expect(&r, "seed"); g->seed = ru(&r);
    expect(&r, "speed"); g->speed = ri(&r);
    if (g->speed < 1) g->speed = 1;
    expect(&r, "times"); g->created = rd(&r); g->now = rd(&r); g->last_save = rd(&r); g->last_seen = rd(&r);
    expect(&r, "dm"); g->dm = rd(&r); g->honour = rd(&r);
    expect(&r, "cur"); g->cur = ri(&r);
    expect(&r, "rqb"); g->rq_body = ri(&r);
    expect(&r, "quest"); g->quest = ri(&r);
    expect(&r, "flags"); g->flags = (unsigned)rl(&r);
    expect(&r, "tech");
    for (int i = 0; i < N_TECH; i++) g->tech[i] = ri(&r);
    expect(&r, "officer");
    for (int i = 0; i < N_OFF; i++) g->officer[i] = rd(&r);
    expect(&r, "rq"); rq_(&r, &g->rq);
    expect(&r, "rng");
    for (int i = 0; i < 4; i++) g->rng.s[i] = ru(&r);
    expect(&r, "ev"); g->ev.next_event = rd(&r); g->ev.last_hostile = rd(&r); g->ev.total = rl(&r); g->ev.merchant_until = rd(&r);
    Decision *d = &g->ev.dec;
    expect(&r, "dec");
    d->active = ri(&r); d->type = ri(&r); d->body = ri(&r);
    rs(&r, d->title, sizeof d->title); rs(&r, d->text, sizeof d->text);
    d->nopt = clampi(ri(&r), 0, 3);
    for (int i = 0; i < 3; i++) {
        rs(&r, d->opt[i], sizeof d->opt[i]);
        for (int k = 0; k < NRES; k++) d->optcost[i][k] = rd(&r);
        d->optdm[i] = rd(&r); d->optbonus[i] = ri(&r);
    }
    d->dc = ri(&r); d->base_mod = ri(&r);
    rs(&r, d->mod_text, sizeof d->mod_text);
    d->deadline = rd(&r); d->defopt = ri(&r); d->chosen = ri(&r); d->roll = ri(&r); d->total = ri(&r); d->outcome = ri(&r); d->rerolls = ri(&r);
    rs(&r, d->result, sizeof d->result);
    d->stage = ri(&r); d->tier = ri(&r);
    expect(&r, "stats");
    for (int i = 0; i < 4; i++) g->stat_produced[i] = rd(&r);
    g->stat_combats = rd(&r); g->stat_wins = rd(&r);
    expect(&r, "bodies");
    g->nbody = clampi(ri(&r), 0, MAX_BODIES);
    for (int i = 0; i < g->nbody && !r.err; i++) {
        Body *b = &g->body[i];
        PlanetInfo *p = &b->info;
        expect(&r, "body");
        b->used = ri(&r); b->is_moon = ri(&r); b->parent = ri(&r); b->moon = ri(&r);
        rs(&r, b->name, sizeof b->name);
        b->g = ri(&r); b->s = ri(&r); b->p = ri(&r);
        expect(&r, "info");
        p->tmin = ri(&r); p->tmax = ri(&r); p->diameter = ri(&r); p->fields = ri(&r); p->biome = clampi(ri(&r), 0, BIO_COUNT - 1);
        p->atmo = clampi(ri(&r), 0, ATM_COUNT - 1); p->pressure = rd(&r); p->gravity = rd(&r); p->daylen = rd(&r); p->water = rd(&r);
        p->fertility = rd(&r); p->radiation = clampi(ri(&r), 0, 3); p->magneto = clampi(ri(&r), 0, 2);
        p->rich[0] = rd(&r); p->rich[1] = rd(&r); p->rich[2] = rd(&r); p->has_moon = ri(&r); p->moon_diam = ri(&r);
        p->vseed = ru(&r); p->ring = ri(&r); p->climate = clampi(ri(&r), 0, 2);
        expect(&r, "res");
        for (int k = 0; k < 4; k++) b->res[k] = rd(&r);
        b->pop = rd(&r);
        expect(&r, "bld");
        for (int k = 0; k < N_BLD; k++) b->bld[k] = ri(&r);
        expect(&r, "pct");
        for (int k = 0; k < N_BLD; k++) b->pct[k] = clampi(ri(&r), 0, 100);
        b->sat_pct = clampi(ri(&r), 0, 100); b->crawler_pct = clampi(ri(&r), 0, 100);
        expect(&r, "ship");
        for (int k = 0; k < N_SHIP; k++) b->ship[k] = rl(&r);
        expect(&r, "def");
        for (int k = 0; k < N_DEF; k++) b->def[k] = rl(&r);
        expect(&r, "bq"); rq_(&r, &b->bq);
        expect(&r, "lq"); rq_(&r, &b->lq);
        expect(&r, "sq");
        b->nsq = clampi(ri(&r), 0, MAX_SHIPQ);
        for (int k = 0; k < b->nsq; k++) {
            b->sq[k].item = clampi(ri(&r), T_END, D_END - 1);
            b->sq[k].count = rl(&r); b->sq[k].unit = rd(&r); b->sq[k].next = rd(&r);
        }
        expect(&r, "mod");
        for (int k = 0; k < MAX_MODS; k++) { b->mod[k].kind = clampi(ri(&r), 0, MOD_COUNT - 1); b->mod[k].factor = rd(&r); b->mod[k].until = rd(&r); }
        expect(&r, "bt"); b->founded = rd(&r); b->last_attacked = rd(&r); b->gate_ready = rd(&r);
    }
    expect(&r, "fleets");
    int nf = ri(&r);
    for (int i = 0; i < nf && i < MAX_FLEETS && !r.err; i++) {
        Fleet *fl = &g->fleet[i];
        expect(&r, "fleet");
        fl->used = 1;
        fl->mission = ri(&r); fl->state = ri(&r); fl->origin = ri(&r); fl->og = ri(&r); fl->os = ri(&r); fl->op = ri(&r); fl->otype = ri(&r);
        fl->tg = ri(&r); fl->ts = ri(&r); fl->tp = ri(&r); fl->ttype = ri(&r);
        for (int k = 0; k < N_SHIP; k++) fl->ship[k] = rl(&r);
        for (int k = 0; k < NRES; k++) fl->cargo[k] = rd(&r);
        fl->depart = rd(&r); fl->arrive = rd(&r); fl->hold = rd(&r); fl->ret = rd(&r);
        fl->speed = ri(&r); fl->npc = clampi(ri(&r), 0, N_NPC - 1); fl->hkind = ri(&r); fl->wt = ri(&r); fl->st = ri(&r); fl->at = ri(&r);
    }
    expect(&r, "routes");
    int nr = ri(&r);
    for (int i = 0; i < nr && !r.err; i++) {
        expect(&r, "route");
        int slot = clampi(ri(&r), 0, MAX_ROUTES - 1);
        Route *rt = &g->route[slot];
        rt->used = 1;
        rt->kind = ri(&r); rt->origin = ri(&r); rt->dest = ri(&r); rt->tg = ri(&r); rt->ts = ri(&r); rt->tp = ri(&r);
        rt->npc = clampi(ri(&r), 0, N_NPC - 1); rt->res_out = clampi(ri(&r), 0, NRES - 1); rt->res_in = clampi(ri(&r), 0, NRES - 1); rt->amount = rd(&r);
        for (int k = 0; k < N_SHIP; k++) rt->ship[k] = rl(&r);
        rt->state = ri(&r); rt->t_next = rd(&r); rt->t_arrive = rd(&r); rt->t_return = rd(&r);
        for (int k = 0; k < NRES; k++) { rt->load[k] = rd(&r); rt->back[k] = rd(&r); }
        rt->trips = rl(&r); rt->ambushes = rl(&r); rt->lost_ships = rl(&r); rt->delivered = rd(&r);
    }
    expect(&r, "msgs");
    g->nmsg = clampi(ri(&r), 0, MAX_MSGS);
    for (int i = 0; i < g->nmsg && !r.err; i++) {
        Message *m = &g->msg[i];
        expect(&r, "msg");
        m->t = rd(&r); m->type = ri(&r); m->read = ri(&r);
        rs(&r, m->title, sizeof m->title);
        rs(&r, m->body, sizeof m->body);
    }
    expect(&r, "debris");
    g->ndebris = clampi(ri(&r), 0, MAX_DEBRIS);
    for (int i = 0; i < g->ndebris && !r.err; i++) {
        expect(&r, "d");
        g->debris[i].g = ri(&r); g->debris[i].s = ri(&r); g->debris[i].p = ri(&r); g->debris[i].m = rd(&r); g->debris[i].c = rd(&r);
    }
    expect(&r, "touched");
    g->ntouched = clampi(ri(&r), 0, MAX_TOUCHED);
    for (int i = 0; i < g->ntouched && !r.err; i++) {
        Touched *t = &g->touched[i];
        expect(&r, "t");
        t->g = ri(&r); t->s = ri(&r); t->p = ri(&r); t->t_raid = rd(&r); t->def_dmg = rd(&r); t->fleet_dmg = rd(&r); t->t_dmg = rd(&r);
    }
    expect(&r, "explored");
    g->nexplored = clampi(ri(&r), 0, MAX_EXPLORED);
    for (int i = 0; i < g->nexplored; i++) g->explored[i] = ri(&r);
    expect(&r, "end");
    fclose(f);
    if (r.err || g->nbody < 1 || !g->body[0].used) { universe_free(g); memset(g, 0, sizeof *g); return 0; }
    if (g->cur < 0 || g->cur >= g->nbody || !g->body[g->cur].used) g->cur = 0;
    if (g->rq_body < 0 || g->rq_body >= g->nbody || !g->body[g->rq_body].used) { g->rq_body = 0; g->rq.n = 0; }
    for (int i = 0; i < MAX_FLEETS; i++)
        if (g->fleet[i].used && (g->fleet[i].origin < 0 || g->fleet[i].origin >= g->nbody)) g->fleet[i].origin = 0;
    for (int i = 0; i < MAX_ROUTES; i++)
        if (g->route[i].used && (g->route[i].origin < 0 || g->route[i].origin >= g->nbody)) g->route[i].used = 0;
    universe_build(g);
    return 1;
}

int save_exists(const char *path, char *name, size_t n) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    static Rd r;
    memset(&r, 0, sizeof r);
    r.f = f;
    expect(&r, SAVE_MAGIC);
    int v = ri(&r);
    expect(&r, "name");
    if (name) rs(&r, name, n);
    fclose(f);
    return !r.err && v == SAVE_VERSION;
}
