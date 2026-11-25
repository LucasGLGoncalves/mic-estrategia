/* combat.c - simulação de combate no estilo do OGame (6 rodadas, fogo rápido, escudos,
 * explosões) */
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int type;
    float hull, shield;
} Unit;

static double u_att[2][N_UNIT], u_shd[2][N_UNIT], u_hull[2][N_UNIT];

static void unit_stats(int side, int wt, int st, int at) {
    for (int u = 0; u < N_UNIT; u++) {
        const ItemDef *d = &ITEMS[unit_item(u)];
        u_att[side][u] = d->att * (1.0 + 0.1 * wt);
        u_shd[side][u] = d->shd * (1.0 + 0.1 * st);
        u_hull[side][u] = d->si / 10.0 * (1.0 + 0.1 * at);
    }
}

static Unit *expand(const long long *n, long long *count) {
    long long tot = 0;
    for (int u = 0; u < N_UNIT; u++) tot += n[u] > 0 ? n[u] : 0;
    *count = tot;
    if (tot == 0) return NULL;
    Unit *a = (Unit *)malloc(sizeof(Unit) * (size_t)tot);
    if (!a) {
        *count = 0;
        return NULL;
    }
    long long k = 0;
    for (int u = 0; u < N_UNIT; u++)
        for (long long i = 0; i < n[u]; i++) {
            a[k].type = u;
            a[k].hull = 0;
            a[k].shield = 0;
            k++;
        }
    return a;
}

void combat_run(Rng *r, const CombatIn *in, CombatOut *out) {
    memset(out, 0, sizeof *out);
    for (int s = 0; s < 2; s++) unit_stats(s, in->wt[s], in->st[s], in->at[s]);
    long long cnt[2];
    Unit *U[2];
    /* limita o número de unidades simuladas para o tempo de execução não explodir:
     * batalhas enormes são reduzidas em escala */
    long long tot = 0;
    for (int s = 0; s < 2; s++)
        for (int u = 0; u < N_UNIT; u++) tot += in->n[s][u];
    double scale = 1.0;
    const long long CAP = 400000;
    long long n[2][N_UNIT];
    if (tot > CAP) scale = (double)CAP / (double)tot;
    for (int s = 0; s < 2; s++)
        for (int u = 0; u < N_UNIT; u++) {
            out->start[s][u] = in->n[s][u];
            n[s][u] = scale < 1.0 ? (long long)ceil(in->n[s][u] * scale) : in->n[s][u];
        }
    for (int s = 0; s < 2; s++) {
        U[s] = expand(n[s], &cnt[s]);
        for (long long i = 0; i < cnt[s]; i++)
            U[s][i].hull = (float)u_hull[s][U[s][i].type];
    }
    int round = 0;
    while (round < 6 && cnt[0] > 0 && cnt[1] > 0) {
        round++;
        for (int s = 0; s < 2; s++)
            for (long long i = 0; i < cnt[s]; i++)
                U[s][i].shield = (float)u_shd[s][U[s][i].type];
        for (int s = 0; s < 2; s++) {
            int o = 1 - s;
            for (long long i = 0; i < cnt[s]; i++) {
                int st = U[s][i].type;
                double dmg = u_att[s][st];
                if (dmg <= 0) continue;
                for (int guard = 0; guard < 5000; guard++) {
                    long long j = (long long)(rng_next(r) % (uint64_t)cnt[o]);
                    Unit *t = &U[o][j];
                    int tt = t->type;
                    if (t->hull > 0) {
                        double maxs = u_shd[o][tt];
                        if (dmg >= 0.01 * maxs) {
                            if (dmg <= t->shield) {
                                t->shield -= (float)dmg;
                            } else {
                                double through = dmg - t->shield;
                                t->shield = 0;
                                t->hull -= (float)through;
                                if (t->hull > 0) {
                                    double mh = u_hull[o][tt];
                                    if (t->hull < 0.7 * mh &&
                                        rng_double(r) < 1.0 - t->hull / mh)
                                        t->hull = 0;
                                }
                            }
                        }
                    }
                    int rf = rapidfire(st, tt);
                    if (rf > 1 && rng_double(r) < (rf - 1.0) / rf) continue;
                    break;
                }
            }
        }
        /* remove os destruídos */
        for (int s = 0; s < 2; s++) {
            long long w = 0;
            for (long long i = 0; i < cnt[s]; i++)
                if (U[s][i].hull > 0) U[s][w++] = U[s][i];
            cnt[s] = w;
        }
    }
    out->rounds = round;
    long long endn[2][N_UNIT];
    memset(endn, 0, sizeof endn);
    for (int s = 0; s < 2; s++)
        for (long long i = 0; i < cnt[s]; i++) endn[s][U[s][i].type]++;
    for (int s = 0; s < 2; s++)
        for (int u = 0; u < N_UNIT; u++) {
            long long e = endn[s][u];
            if (scale < 1.0 && n[s][u] > 0)
                e = (long long)llround((double)e / (double)n[s][u] *
                                       (double)in->n[s][u]);
            if (e > in->n[s][u]) e = in->n[s][u];
            out->end[s][u] = e;
        }
    if (cnt[0] > 0 && cnt[1] == 0) out->winner = 0;
    else if (cnt[1] > 0 && cnt[0] == 0) out->winner = 1;
    else if (cnt[0] == 0 && cnt[1] == 0) out->winner = -1;
    else out->winner = -1;
    /* destroços (30% do metal+cristal das naves destruídas), valor perdido, reparo de
     * defesas (70%) */
    for (int s = 0; s < 2; s++)
        for (int u = 0; u < N_UNIT; u++) {
            long long lost = out->start[s][u] - out->end[s][u];
            if (lost <= 0) continue;
            const ItemDef *d = &ITEMS[unit_item(u)];
            if (u < N_SHIP) {
                out->debris[0] += 0.3 * d->cost[0] * lost;
                out->debris[1] += 0.3 * d->cost[1] * lost;
            } else if (s == 1) {
                long long rep = rng_binomial(r, lost, 0.7);
                out->repaired[u] = rep;
            }
        }
    for (int s = 0; s < 2; s++)
        for (int u = 0; u < N_UNIT; u++) {
            long long lost =
                out->start[s][u] - out->end[s][u] - (s == 1 ? out->repaired[u] : 0);
            if (lost > 0) {
                const ItemDef *d = &ITEMS[unit_item(u)];
                out->lost_value[s] += lost * (d->cost[0] + d->cost[1] + d->cost[2]);
            }
        }
    double deb = out->debris[0] + out->debris[1];
    out->moon_chance = deb / 100000.0;
    if (out->moon_chance > 20) out->moon_chance = 20;
    free(U[0]);
    free(U[1]);
}

void loot_take(const double avail[3], double cap, double out[3]) {
    double a[3] = {avail[0] * 0.5, avail[1] * 0.5, avail[2] * 0.5};
    out[0] = out[1] = out[2] = 0;
    if (cap <= 0) return;
    double t;
    t = fmin(cap / 3.0, a[0]);
    out[0] += t;
    cap -= t;
    t = fmin(cap / 2.0, a[1]);
    out[1] += t;
    cap -= t;
    t = fmin(cap, a[2]);
    out[2] += t;
    cap -= t;
    t = fmin(cap / 2.0, a[0] - out[0]);
    out[0] += t;
    cap -= t;
    t = fmin(cap, a[1] - out[1]);
    out[1] += t;
    cap -= t;
    t = fmin(cap, a[0] - out[0]);
    out[0] += t;
    cap -= t;
    for (int i = 0; i < 3; i++) out[i] = floor(out[i]);
}

static void side_table(char *buf, size_t n, const CombatOut *o, int s, const char *name,
                       int wt, int st, int at) {
    size_t len = strlen(buf);
    snprintf(buf + len, n - len,
             "\001w%s\001d  (Armas +%d%%  Escudos +%d%%  Blindagem +%d%%)\n", name,
             wt * 10, st * 10, at * 10);
    int any = 0;
    for (int u = 0; u < N_UNIT; u++) {
        if (o->start[s][u] == 0) continue;
        any = 1;
        len = strlen(buf);
        char a[32], b[32];
        fmt_num(a, sizeof a, (double)o->start[s][u]);
        long long e = o->end[s][u] + (s == 1 ? o->repaired[u] : 0);
        fmt_num(b, sizeof b, (double)e);
        long long lost = o->start[s][u] - e;
        /* o %-26s conta bytes, não letras: completa à mão, pelo tamanho em UTF-8 */
        const char *nm = ITEMS[unit_item(u)].name;
        int pad = 26 - utf8_len(nm);
        snprintf(buf + len, n - len, "  %s%*s %10s -> %s%s\001d%s\n", nm,
                 pad > 0 ? pad : 0, "", a, lost > 0 ? "\001r" : "\001g", b,
                 (s == 1 && o->repaired[u] > 0) ? " (reparados)" : "");
    }
    if (!any) {
        len = strlen(buf);
        snprintf(buf + len, n - len, "  (sem unidades)\n");
    }
}

void combat_report(const CombatIn *in, const CombatOut *o, const char *an,
                   const char *dn, char *buf, size_t n) {
    buf[0] = 0;
    side_table(buf, n, o, 0, an, in->wt[0], in->st[0], in->at[0]);
    size_t len = strlen(buf);
    snprintf(buf + len, n - len, "\n");
    side_table(buf, n, o, 1, dn, in->wt[1], in->st[1], in->at[1]);
    len = strlen(buf);
    char l0[32], l1[32], d0[32], d1[32];
    fmt_num(l0, sizeof l0, o->lost_value[0]);
    fmt_num(l1, sizeof l1, o->lost_value[1]);
    fmt_num(d0, sizeof d0, o->debris[0]);
    fmt_num(d1, sizeof d1, o->debris[1]);
    snprintf(buf + len, n - len,
             "\nRodadas: %d   Resultado: %s\nPerdas atacante: %s   Perdas defensor: "
             "%s\nCampo de destroços: %s metal, %s cristal\n",
             o->rounds,
             o->winner == 0   ? "\001rVitória do atacante\001d"
             : o->winner == 1 ? "\001gVitória do defensor\001d"
                              : "\001yEmpate\001d",
             l0, l1, d0, d1);
}
