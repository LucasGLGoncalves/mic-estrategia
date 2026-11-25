/* sim.c - integração no tempo: produção por segundo, filas, frotas, eventos, progresso
 * offline */
#include "game.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void game_toast(Game *g, uint32_t col, const char *fmt, ...) {
    if (g->ev.offline) return;
    if (g->ntoast >= 8) {
        memmove(g->toast[0], g->toast[1], sizeof(g->toast[0]) * 7);
        memmove(&g->toast_col[0], &g->toast_col[1], sizeof(g->toast_col[0]) * 7);
        g->ntoast = 7;
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g->toast[g->ntoast], sizeof g->toast[0], fmt, ap);
    va_end(ap);
    g->toast_col[g->ntoast] = col;
    g->ntoast++;
}

void game_msg(Game *g, int type, const char *title, const char *fmt, ...) {
    if (g->nmsg >= MAX_MSGS) {
        memmove(&g->msg[0], &g->msg[1], sizeof(Message) * (MAX_MSGS - 1));
        g->nmsg = MAX_MSGS - 1;
    }
    Message *m = &g->msg[g->nmsg++];
    m->t = g->now;
    m->type = type;
    m->read = 0;
    snprintf(m->title, sizeof m->title, "%s", title);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m->body, sizeof m->body, fmt, ap);
    va_end(ap);
}

int game_unread(const Game *g) {
    int n = 0;
    for (int i = 0; i < g->nmsg; i++)
        if (!g->msg[i].read) n++;
    return n;
}

static int affordable3(const Body *b, const double c[3]) {
    return b->res[0] + 1e-6 >= c[0] && b->res[1] + 1e-6 >= c[1] &&
           b->res[2] + 1e-6 >= c[2];
}

/* qual: 0 construção, 1 civilização, 2 pesquisa */
static void queue_process(Game *g, int bi, Queue *q, int which) {
    double tstart = g->now;
    for (int guard = 0; guard < 16 && q->n > 0; guard++) {
        QItem *it = &q->it[0];
        Body *pb = &g->body[which == 2 ? g->rq_body : bi];
        if (!it->active) {
            if (!reqs_met(g, pb, it->item)) {
                game_msg(g, MSG_INFO, "Item removido da fila",
                         "%s não atende mais aos requisitos e foi removido da fila.",
                         ITEMS[it->item].name);
                for (int i = 0; i < q->n - 1; i++) q->it[i] = q->it[i + 1];
                q->n--;
                continue;
            }
            double c[3];
            double e = item_cost1(it->item, (int)it->amount, c);
            if (!affordable3(pb, c)) return;
            if (e > 0) {
                Rates r;
                body_rates(g, pb, &r);
                if (r.e_prod < e) return;
            }
            for (int i = 0; i < 3; i++) pb->res[i] -= c[i];
            it->active = 1;
            it->start = tstart;
            it->end =
                tstart + (which == 2
                              ? research_time(g, g->rq_body, it->item, (int)it->amount)
                              : build_time(g, pb, it->item, (int)it->amount));
        }
        if (it->end > g->now) return;
        /* concluído */
        int id = it->item;
        if (which == 2) {
            g->tech[TI(id)] = (int)it->amount;
            game_toast(g, RGB(100, 215, 235), "Pesquisa concluída: %s nível %lld",
                       ITEMS[id].name, it->amount);
        } else {
            pb->bld[id] = (int)it->amount;
            game_toast(g, RGB(110, 215, 110), "%s: %s nível %lld concluído", pb->name,
                       ITEMS[id].name, it->amount);
        }
        tstart = it->end;
        for (int i = 0; i < q->n - 1; i++) q->it[i] = q->it[i + 1];
        q->n--;
    }
}

static void shipyard_process(Game *g, Body *b) {
    double tstart = g->now;
    for (int guard = 0; guard < 32 && b->nsq > 0; guard++) {
        SQItem *s = &b->sq[0];
        if (s->next <= 0) s->next = tstart + s->unit;
        if (s->next > g->now) return;
        long long k = 1 + (long long)floor((g->now - s->next) / s->unit);
        if (k > s->count) k = s->count;
        if (k < 1) k = 1;
        if (IS_SHIP(s->item)) b->ship[SI(s->item)] += k;
        else b->def[DI(s->item)] += k;
        s->count -= k;
        double tdone = s->next + (double)(k - 1) * s->unit;
        s->next += (double)k * s->unit;
        if (s->count <= 0) {
            game_toast(g, RGB(110, 215, 110), "%s: lote de %s concluído", b->name,
                       ITEMS[s->item].name);
            for (int i = 0; i < b->nsq - 1; i++) b->sq[i] = b->sq[i + 1];
            b->nsq--;
            if (b->nsq > 0) b->sq[0].next = 0;
            tstart = tdone;
        }
    }
}

static void integrate(Game *g, double dt) {
    if (dt <= 0) return;
    double h = dt / 3600.0;
    for (int i = 0; i < g->nbody; i++) {
        Body *b = &g->body[i];
        if (!b->used) continue;
        Rates r;
        body_rates(g, b, &r);
        for (int k = 0; k < 3; k++) {
            double inc = r.prod[k] * h;
            double before = b->res[k];
            if (inc > 0) {
                if (b->res[k] < r.cap[k]) b->res[k] = fmin(r.cap[k], b->res[k] + inc);
            } else {
                b->res[k] += inc;
                if (b->res[k] < 0) b->res[k] = 0;
            }
            double got = b->res[k] - before;
            if (got > 0) g->stat_produced[k] += got;
            if (g->ev.offline) g->away_gain[k] += got;
        }
        if (b->is_moon) continue;
        /* alimento e população */
        double food = b->res[RES_FOOD] + r.prod[RES_FOOD] * h;
        double fb = b->res[RES_FOOD];
        if (r.prod[RES_FOOD] > 0) {
            if (fb < r.cap[RES_FOOD]) food = fmin(r.cap[RES_FOOD], food);
            else food = fb;
        }
        int starving = food <= 0 && r.prod[RES_FOOD] < 0;
        if (food < 0) food = 0;
        b->res[RES_FOOD] = food;
        if (g->ev.offline) g->away_gain[RES_FOOD] += food - fb;
        double P = b->pop, K = r.pop_cap, sp = g->speed;
        if (starving) {
            double target = r.food_prod / (0.1 * sp);
            if (target < P) P = target + (P - target) * exp(-0.15 * sp * h);
        } else if (P > K) {
            P = K + (P - K) * exp(-0.10 * sp * h);
        } else if (P > 0) {
            P = K / (1.0 + (K / P - 1.0) * exp(-r.growth * h));
        } else {
            P = 0;
        }
        if (P < 0) P = 0;
        b->pop = P;
    }
}

static double next_time(const Game *g) {
    double t = 1e300;
    for (int i = 0; i < g->nbody; i++) {
        const Body *b = &g->body[i];
        if (!b->used) continue;
        if (b->bq.n && b->bq.it[0].active && b->bq.it[0].end < t) t = b->bq.it[0].end;
        if (b->lq.n && b->lq.it[0].active && b->lq.it[0].end < t) t = b->lq.it[0].end;
        if (b->nsq && b->sq[0].next > 0 && b->sq[0].next < t) t = b->sq[0].next;
        for (int k = 0; k < MAX_MODS; k++)
            if (b->mod[k].until > g->now && b->mod[k].until < t) t = b->mod[k].until;
    }
    if (g->rq.n && g->rq.it[0].active && g->rq.it[0].end < t) t = g->rq.it[0].end;
    double x = fleet_next_time(g);
    if (x < t) t = x;
    x = trade_next_time(g);
    if (x < t) t = x;
    x = events_next_time(g);
    if (x < t) t = x;
    return t;
}

static void process_all(Game *g) {
    for (int i = 0; i < g->nbody; i++) {
        Body *b = &g->body[i];
        if (!b->used) continue;
        queue_process(g, i, &b->bq, 0);
        queue_process(g, i, &b->lq, 1);
        shipyard_process(g, b);
    }
    if (g->rq.n > 0) queue_process(g, g->rq_body, &g->rq, 2);
    fleet_process(g);
    trade_process(g);
    events_process(g);
    quest_check(g);
}

void game_advance(Game *g, double target, int offline) {
    g->ev.offline = offline;
    if (target <= g->now) {
        process_all(g);
        g->ev.offline = 0;
        return;
    }
    double maxstep = offline ? 600.0 : 60.0;
    for (long guard = 0; g->now < target && guard < 5000000; guard++) {
        double tn = target;
        if (tn - g->now > maxstep) tn = g->now + maxstep;
        double nt = next_time(g);
        if (nt > g->now && nt < tn) tn = nt;
        integrate(g, tn - g->now);
        g->now = tn;
        process_all(g);
    }
    g->ev.offline = 0;
}
