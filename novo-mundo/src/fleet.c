/* fleet.c - movimento de frotas, missões, expedições, colonização */
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void compose_fleet(double value, double pts, long long *ship);

const char *mission_name(int m) {
    switch (m) {
    case M_ATTACK: return "Atacar";
    case M_TRANSPORT: return "Transportar";
    case M_DEPLOY: return "Estacionar";
    case M_ESPIONAGE: return "Espionar";
    case M_COLONIZE: return "Colonizar";
    case M_RECYCLE: return "Reciclar";
    case M_EXPEDITION: return "Expedição";
    case M_EXPLORE: return "Explorar";
    case M_HOSTILE: return "ATAQUE HOSTIL";
    case M_HOSTILE_SPY: return "Espionagem hostil";
    }
    return "-";
}

double fleet_distance(int g1, int s1, int p1, int g2, int s2, int p2) {
    if (g1 != g2) {
        int d = g1 > g2 ? g1 - g2 : g2 - g1;
        if (UNI_GALAXIES - d < d) d = UNI_GALAXIES - d;
        return 20000.0 * d;
    }
    if (s1 != s2) {
        int d = s1 > s2 ? s1 - s2 : s2 - s1;
        if (UNI_SYSTEMS - d < d) d = UNI_SYSTEMS - d;
        return 2700.0 + 95.0 * d;
    }
    if (p1 != p2) return 1000.0 + 5.0 * (p1 > p2 ? p1 - p2 : p2 - p1);
    return 5.0;
}

static int ship_drive(const Game *g, int id, double *base, double *fuel) {
    const ItemDef *d = &ITEMS[id];
    int drv = d->drive;
    *base = d->speed;
    *fuel = d->fuel;
    int imp = g->tech[TI(T_IMPULSE)], hyp = g->tech[TI(T_HYPERDRIVE)];
    if (id == S_SMALL_CARGO && imp >= 5) {
        *base = 10000;
        drv = DRV_IMP;
        *fuel = 20;
    }
    if (id == S_BOMBER && hyp >= 8) {
        *base = 5000;
        drv = DRV_HYP;
    }
    if (id == S_RECYCLER) {
        if (hyp >= 15) {
            *base = 6000;
            drv = DRV_HYP;
        } else if (imp >= 17) {
            *base = 4000;
            drv = DRV_IMP;
        }
    }
    return drv;
}

double ship_speed(const Game *g, int id) {
    double base, fuel;
    int drv = ship_drive(g, id, &base, &fuel);
    switch (drv) {
    case DRV_COMB: return base * (1.0 + 0.1 * g->tech[TI(T_COMBUSTION)]);
    case DRV_IMP: return base * (1.0 + 0.2 * g->tech[TI(T_IMPULSE)]);
    case DRV_HYP: return base * (1.0 + 0.3 * g->tech[TI(T_HYPERDRIVE)]);
    }
    return 0;
}

double fleet_ship_count(const long long *s) {
    double n = 0;
    for (int i = 0; i < N_SHIP; i++) n += (double)s[i];
    return n;
}

double fleet_slowest(const Game *g, const long long *s) {
    double mn = 0;
    for (int i = 0; i < N_SHIP; i++) {
        if (s[i] <= 0) continue;
        double v = ship_speed(g, T_END + i);
        if (mn == 0 || v < mn) mn = v;
    }
    return mn;
}

static double raw_duration(double dist, double slowest, int pct) {
    if (slowest <= 0) return 1e9;
    double p10 = pct / 10.0;
    if (p10 < 1) p10 = 1;
    return 35000.0 / p10 * sqrt(dist * 10.0 / slowest) + 10.0;
}

double flight_duration(const Game *g, double dist, double slowest, int pct) {
    return raw_duration(dist, slowest, pct) / g->speed;
}

double fleet_fuel(const Game *g, const long long *s, double dist, int pct,
                  const Body *origin) {
    double slow = fleet_slowest(g, s);
    double raw = raw_duration(dist, slow, pct);
    double cons = 0;
    for (int i = 0; i < N_SHIP; i++) {
        if (s[i] <= 0) continue;
        double base, fuel;
        ship_drive(g, T_END + i, &base, &fuel);
        double spd = ship_speed(g, T_END + i);
        if (spd <= 0) continue;
        double sv = 35000.0 / (raw - 10.0) * sqrt(dist * 10.0 / spd);
        cons += fuel * (double)s[i] * dist / 35000.0 * pow(sv / 10.0 + 1.0, 2.0);
    }
    double grav = origin ? 0.7 + 0.3 * origin->info.gravity : 1.0;
    if (origin && origin->is_moon) grav = 0.8;
    return floor(cons * grav + 0.5) + 1.0;
}

double fleet_cargo_cap(const Game *g, const long long *s) {
    double c = 0;
    for (int i = 0; i < N_SHIP; i++) c += (double)s[i] * ITEMS[T_END + i].cargo;
    return floor(
        c * (1.0 + 0.05 * g->tech[TI(T_HYPERSPACE)] + 0.05 * g->tech[TI(T_LOGISTICS)]));
}

int fleet_slots_used(const Game *g) {
    int n = 0;
    for (int i = 0; i < MAX_FLEETS; i++)
        if (g->fleet[i].used && g->fleet[i].mission != M_HOSTILE &&
            g->fleet[i].mission != M_HOSTILE_SPY)
            n++;
    return n;
}
int fleet_slots_max(const Game *g) {
    return 1 + g->tech[TI(T_COMPUTER)] + (officer_active(g, OFF_ADMIRAL) ? 2 : 0);
}
int exped_slots_used(const Game *g) {
    int n = 0;
    for (int i = 0; i < MAX_FLEETS; i++)
        if (g->fleet[i].used && g->fleet[i].mission == M_EXPEDITION) n++;
    return n;
}
int exped_slots_max(const Game *g) {
    int a = g->tech[TI(T_ASTRO)];
    return (int)floor(sqrt((double)a)) +
           (officer_active(g, OFF_ADMIRAL) && a > 0 ? 1 : 0);
}
int intergalactic_ok(const Game *g) { return g->tech[TI(T_HYPERDRIVE)] >= 3; }

static int only_ships(const long long *s, int id1, int id2) {
    for (int i = 0; i < N_SHIP; i++)
        if (s[i] > 0 && T_END + i != id1 && T_END + i != id2) return 0;
    return 1;
}

static int target_empty(const Game *g, int gg, int ss, int pp) {
    return npc_planet_at(g, gg, ss, pp) < 0 && player_body_at(g, gg, ss, pp, 0) < 0;
}

int mission_valid(const Game *g, int bi, int m, int tg, int ts, int tp, int tt,
                  const long long *s, char *why, size_t n) {
    const Body *o = &g->body[bi];
#define FAIL(msg)                                                                      \
    do {                                                                               \
        if (why) snprintf(why, n, "%s", msg);                                          \
        return 0;                                                                      \
    } while (0)
    if (fleet_ship_count(s) <= 0) FAIL("Nenhuma nave selecionada");
    if (s[SI(S_SATELLITE)] > 0 || s[SI(S_CRAWLER)] > 0)
        FAIL("Satélites e rastejadores não voam");
    if (tg < 1 || tg > UNI_GALAXIES || ts < 1 || ts > UNI_SYSTEMS || tp < 1 || tp > 16)
        FAIL("Coordenadas inválidas");
    if (tg != o->g && !intergalactic_ok(g))
        FAIL("Viagem intergaláctica exige Propulsor Hiperespacial 3");
    if (tp == 16 && m != M_EXPEDITION)
        FAIL("Posição 16 é espaço profundo: apenas expedições");
    int npc = tp <= 15 ? npc_planet_at(g, tg, ts, tp) : -1;
    int own = tp <= 15 ? player_body_at(g, tg, ts, tp, tt == TT_MOON) : -1;
    if (fleet_slots_used(g) >= fleet_slots_max(g))
        FAIL("Todos os slots de frota em uso (pesquise Computadores)");
    switch (m) {
    case M_ATTACK:
        if (npc < 0) FAIL("Só é possível atacar planetas de outros impérios");
        if (tt == TT_DEBRIS) FAIL("Alvo inválido");
        if (tt == TT_MOON && !g->npl[npc].has_moon) FAIL("Este planeta não possui lua");
        break;
    case M_TRANSPORT:
    case M_DEPLOY:
        if (own < 0) FAIL("Destino deve ser um planeta ou lua seu");
        if (own == bi) FAIL("Origem e destino são iguais");
        break;
    case M_ESPIONAGE:
        if (!only_ships(s, S_PROBE, -1)) FAIL("Espionagem usa apenas sondas");
        if (npc < 0) FAIL("Escolha um planeta de outro império");
        break;
    case M_COLONIZE:
        if (s[SI(S_COLONY)] <= 0) FAIL("É necessária uma Nave Colonizadora");
        if (tt != TT_PLANET || !target_empty(g, tg, ts, tp))
            FAIL("A posição precisa estar vazia");
        if (colonies_count(g) >= max_colonies(g))
            FAIL("Limite de colônias (pesquise Astrofísica)");
        if (!position_allowed(g, tp))
            FAIL("Posição extrema: exige Astrofísica mais alta");
        break;
    case M_RECYCLE: {
        if (s[SI(S_RECYCLER)] <= 0) FAIL("São necessários Recicladores");
        const Debris *d = NULL;
        for (int i = 0; i < g->ndebris; i++)
            if (g->debris[i].g == tg && g->debris[i].s == ts && g->debris[i].p == tp)
                d = &g->debris[i];
        if (!d || d->m + d->c < 1) FAIL("Não há campo de destroços nesse local");
    } break;
    case M_EXPEDITION:
        if (tp != 16) FAIL("Expedições vão para a posição 16");
        if (g->tech[TI(T_ASTRO)] < 1) FAIL("Exige Astrofísica 1");
        if (exped_slots_used(g) >= exped_slots_max(g))
            FAIL("Slots de expedição esgotados");
        break;
    case M_EXPLORE:
        if (s[SI(S_PROBE)] <= 0 && s[SI(S_PATHFINDER)] <= 0)
            FAIL("Exploração exige Sonda ou Desbravador");
        if (own >= 0) FAIL("Você já conhece seu próprio planeta");
        if (npc < 0 && is_explored(g, tg, ts, tp)) FAIL("Posição já explorada");
        break;
    default: FAIL("Missão inválida");
    }
    if (why) why[0] = 0;
    return 1;
#undef FAIL
}

int fleet_launch(Game *g, int bi, int m, int tg, int ts, int tp, int tt,
                 const long long *s, const double *cargo, int pct, char *err,
                 size_t n) {
    Body *o = &g->body[bi];
    if (!mission_valid(g, bi, m, tg, ts, tp, tt, s, err, n)) return 0;
    for (int i = 0; i < N_SHIP; i++)
        if (s[i] < 0 || s[i] > o->ship[i]) {
            snprintf(err, n, "Naves insuficientes");
            return 0;
        }
    double dist = fleet_distance(o->g, o->s, o->p, tg, ts, tp);
    double fuel = fleet_fuel(g, s, dist, pct, o);
    if (m == M_EXPEDITION) fuel *= 1.5;
    fuel = floor(fuel);
    double c[NRES] = {0};
    if (cargo)
        for (int i = 0; i < NRES; i++) c[i] = cargo[i] > 0 ? floor(cargo[i]) : 0;
    double tot = c[0] + c[1] + c[2] + c[3];
    if (tot > fleet_cargo_cap(g, s)) {
        snprintf(err, n, "Carga excede a capacidade");
        return 0;
    }
    for (int i = 0; i < NRES; i++)
        if (o->res[i] + 1e-6 < c[i] + (i == RES_DEUT ? fuel : 0)) {
            snprintf(err, n,
                     i == RES_DEUT ? "Deutério insuficiente (carga + combustível)"
                                   : "Recursos insuficientes para a carga");
            return 0;
        }
    int slot = -1;
    for (int i = 0; i < MAX_FLEETS; i++)
        if (!g->fleet[i].used) {
            slot = i;
            break;
        }
    if (slot < 0) {
        snprintf(err, n, "Limite interno de frotas");
        return 0;
    }
    Fleet *f = &g->fleet[slot];
    memset(f, 0, sizeof *f);
    f->used = 1;
    f->mission = m;
    f->state = FS_OUT;
    f->origin = bi;
    f->og = o->g;
    f->os = o->s;
    f->op = o->p;
    f->otype = o->is_moon ? TT_MOON : TT_PLANET;
    f->tg = tg;
    f->ts = ts;
    f->tp = tp;
    f->ttype = tt;
    f->speed = pct;
    double dur = flight_duration(g, dist, fleet_slowest(g, s), pct);
    f->depart = g->now;
    f->arrive = g->now + dur;
    f->hold = 0;
    f->ret = f->arrive + dur;
    for (int i = 0; i < N_SHIP; i++) {
        f->ship[i] = s[i];
        o->ship[i] -= s[i];
    }
    for (int i = 0; i < NRES; i++) {
        f->cargo[i] = c[i];
        o->res[i] -= c[i];
    }
    o->res[RES_DEUT] -= fuel;
    if (err) err[0] = 0;
    return 1;
}

int fleet_recall(Game *g, int fi) {
    Fleet *f = &g->fleet[fi];
    if (!f->used || f->mission == M_HOSTILE || f->mission == M_HOSTILE_SPY) return 0;
    if (f->state == FS_RETURN) return 0;
    if (f->mission == M_DEPLOY && f->state == FS_OUT) { /* tudo certo */
    }
    double back;
    if (f->state == FS_OUT) back = g->now - f->depart;
    else back = f->arrive - f->depart;
    f->state = FS_RETURN;
    f->ret = g->now + back;
    return 1;
}

static void fleet_home(Game *g, Fleet *f) {
    int bi = f->origin;
    if (bi < 0 || bi >= g->nbody || !g->body[bi].used) bi = 0;
    Body *b = &g->body[bi];
    for (int i = 0; i < N_SHIP; i++) b->ship[i] += f->ship[i];
    for (int i = 0; i < NRES; i++) b->res[i] += f->cargo[i];
    f->used = 0;
}

static void ships_line(char *buf, size_t n, const long long *s) {
    buf[0] = 0;
    for (int i = 0; i < N_SHIP; i++) {
        if (s[i] <= 0) continue;
        size_t len = strlen(buf);
        char a[32];
        fmt_num(a, sizeof a, (double)s[i]);
        snprintf(buf + len, n - len, "  %s: %s\n", ITEMS[T_END + i].name, a);
    }
}

static void spy_report(Game *g, Fleet *f, int npi) {
    NpcSnap sn;
    npc_snapshot(g, npi, &sn);
    const NpcPlanet *pl = &g->npl[npi];
    const Npc *nn = &g->npc[pl->npc];
    long long probes = f->ship[SI(S_PROBE)];
    int diff = g->tech[TI(T_ESPIONAGE)] - sn.tech[TI(T_ESPIONAGE)];
    int info =
        diff + (int)floor(sqrt((double)probes)); /* 0 recursos, 1 frota, 2 defesa, 3
                                                    construções, 5 tecnologia */
    char body[MSG_BODY];
    char a[32], b[32], c[32];
    fmt_num(a, sizeof a, sn.res[0]);
    fmt_num(b, sizeof b, sn.res[1]);
    fmt_num(c, sizeof c, sn.res[2]);
    int n = snprintf(body, sizeof body,
                     "\001w%s\001d [%d:%d:%d] de \001y%s\001d %s%s%s (%s)\nPontos: "
                     "~%.0f\n\n\001cRecursos\001d  Metal %s  Cristal %s  Deutério %s\n",
                     pl->pname, pl->g, pl->s, pl->p, nn->name, nn->tag[0] ? "[" : "",
                     nn->tag, nn->tag[0] ? "]" : "", npc_type_name(nn->type), sn.points,
                     a, b, c);
    char tmp[1200];
    if (info >= 1) {
        ships_line(tmp, sizeof tmp, sn.ship);
        n += snprintf(body + n, sizeof body - (size_t)n, "\n\001cFrota\001d\n%s",
                      tmp[0] ? tmp : "  nenhuma\n");
    } else
        n += snprintf(
            body + n, sizeof body - (size_t)n,
            "\n\001aFrota: informação insuficiente (mais sondas/Espionagem)\001d\n");
    if (info >= 2) {
        tmp[0] = 0;
        for (int i = 0; i < N_DEF; i++) {
            if (sn.def[i] <= 0) continue;
            size_t L = strlen(tmp);
            fmt_num(a, sizeof a, (double)sn.def[i]);
            snprintf(tmp + L, sizeof tmp - L, "  %s: %s\n", ITEMS[S_END + i].name, a);
        }
        n += snprintf(body + n, sizeof body - (size_t)n, "\n\001cDefesa\001d\n%s",
                      tmp[0] ? tmp : "  nenhuma\n");
    } else
        n += snprintf(body + n, sizeof body - (size_t)n,
                      "\n\001aDefesa: informação insuficiente\001d\n");
    if (info >= 3 && n < (int)sizeof body - 200)
        n +=
            snprintf(body + n, sizeof body - (size_t)n,
                     "\n\001cConstruções\001d  Metal %d  Cristal %d  Deutério %d  "
                     "Solar %d  Estaleiro %d  Lab %d\n",
                     sn.bld[B_METAL_MINE], sn.bld[B_CRYSTAL_MINE], sn.bld[B_DEUT_SYNTH],
                     sn.bld[B_SOLAR_PLANT], sn.bld[B_SHIPYARD], sn.bld[B_LAB]);
    if (info >= 5 && n < (int)sizeof body - 120)
        n += snprintf(
            body + n, sizeof body - (size_t)n,
            "\n\001cPesquisa\001d  Armas %d  Escudos %d  Blindagem %d  Espionagem %d\n",
            sn.tech[TI(T_WEAPONS)], sn.tech[TI(T_SHIELDING)], sn.tech[TI(T_ARMOR)],
            sn.tech[TI(T_ESPIONAGE)]);
    /* contraespionagem */
    double ships = fleet_ship_count(sn.ship);
    double chance =
        ships * 0.0025 * pow(2.0, -diff) / (probes > 0 ? sqrt((double)probes) : 1.0);
    if (chance > 0.9) chance = 0.9;
    if (n < (int)sizeof body - 80)
        n += snprintf(body + n, sizeof body - (size_t)n,
                      "\nChance de contraespionagem: %.0f%%\n", chance * 100);
    char title[MSG_TITLE];
    snprintf(title, sizeof title, "Relatório de espionagem [%d:%d:%d]", pl->g, pl->s,
             pl->p);
    game_msg(g, MSG_SPY, title, "%s", body);
    g->flags |= FL_SPIED;
    if (rng_double(&g->rng) < chance) {
        Debris *d = debris_at(g, f->tg, f->ts, f->tp, 1);
        d->c += 300.0 * (double)probes;
        f->ship[SI(S_PROBE)] = 0;
        game_msg(g, MSG_SPY, "Sondas destruídas",
                 "Suas %lld sondas foram detectadas e abatidas sobre [%d:%d:%d].",
                 probes, f->tg, f->ts, f->tp);
        if (fleet_ship_count(f->ship) <= 0) f->used = 0;
    }
}

static void explore_report(Game *g, Fleet *f) {
    PlanetInfo pi;
    planet_generate(g->seed, f->tg, f->ts, f->tp, 0, &pi);
    set_explored(g, f->tg, f->ts, f->tp);
    g->flags |= FL_EXPLORED;
    char title[MSG_TITLE];
    snprintf(title, sizeof title, "Exploração de [%d:%d:%d]", f->tg, f->ts, f->tp);
    game_msg(g, MSG_FLEET, title,
             "Bioma: \001y%s\001d\nAtmosfera: %s (%.2f atm)\nTemperatura: %d°C a "
             "%d°C\nDiâmetro: %d km (%d campos)\nGravidade: %.2f g\n"
             "Fertilidade: %.0f%%   Água: %.0f%%\nRiqueza: Metal %+.0f%%  Cristal "
             "%+.0f%%  Deutério %+.0f%%\nLua: %s\nClima: %s",
             BIOME_NAME[pi.biome], ATMO_NAME[pi.atmo], pi.pressure, pi.tmin, pi.tmax,
             pi.diameter, pi.fields, pi.gravity, pi.fertility * 100, pi.water * 100,
             (pi.rich[0] - 1) * 100, (pi.rich[1] - 1) * 100, (pi.rich[2] - 1) * 100,
             pi.has_moon ? "sim" : "não", climate_text(&pi));
}

static void attack_npc(Game *g, Fleet *f, int npi) {
    NpcSnap sn;
    npc_snapshot(g, npi, &sn);
    const NpcPlanet *pl = &g->npl[npi];
    const Npc *nn = &g->npc[pl->npc];
    CombatIn in;
    memset(&in, 0, sizeof in);
    for (int i = 0; i < N_SHIP; i++) {
        in.n[0][i] = f->ship[i];
        in.n[1][i] = sn.ship[i];
    }
    for (int i = 0; i < N_CDEF; i++) in.n[1][N_SHIP + i] = sn.def[i];
    in.wt[0] = g->tech[TI(T_WEAPONS)];
    in.st[0] = g->tech[TI(T_SHIELDING)];
    in.at[0] = g->tech[TI(T_ARMOR)];
    in.wt[1] = sn.tech[TI(T_WEAPONS)];
    in.st[1] = sn.tech[TI(T_SHIELDING)];
    in.at[1] = sn.tech[TI(T_ARMOR)];
    CombatOut out;
    combat_run(&g->rng, &in, &out);
    g->stat_combats++;
    for (int i = 0; i < N_SHIP; i++) f->ship[i] = out.end[0][i];
    /* dano ao npc */
    Touched *t = touched_get(g, pl->g, pl->s, pl->p, 1);
    double dv = items_value(NULL, sn.def), fv = items_value(sn.ship, NULL);
    double dl = 0, fl = 0;
    for (int u = 0; u < N_UNIT; u++) {
        long long lost = out.start[1][u] - out.end[1][u] - out.repaired[u];
        if (lost <= 0) continue;
        const ItemDef *d = &ITEMS[unit_item(u)];
        double v = (double)lost * (d->cost[0] + d->cost[1] + d->cost[2]);
        if (u < N_SHIP) fl += v;
        else dl += v;
    }
    double hrs = (g->now - t->t_dmg) * g->speed / 3600.0;
    double cd = t->def_dmg * exp(-hrs / 18.0), cf = t->fleet_dmg * exp(-hrs / 30.0);
    t->def_dmg = clampd(cd + (dv > 0 ? dl / dv * (1 - cd) : 0), 0, 0.98);
    t->fleet_dmg = clampd(cf + (fv > 0 ? fl / fv * (1 - cf) : 0), 0, 0.98);
    t->t_dmg = g->now;
    Debris *d = debris_at(g, pl->g, pl->s, pl->p, 1);
    d->m += out.debris[0];
    d->c += out.debris[1];
    double loot[3] = {0, 0, 0};
    if (out.winner == 0) {
        double cap = fleet_cargo_cap(g, f->ship) -
                     (f->cargo[0] + f->cargo[1] + f->cargo[2] + f->cargo[3]);
        loot_take(sn.res, cap, loot);
        for (int i = 0; i < 3; i++) f->cargo[i] += loot[i];
        t->t_raid = g->now;
        g->stat_wins++;
        g->flags |= FL_ATTACKED;
    }
    /* honra */
    double me = player_points(g, NULL, NULL, NULL, NULL);
    double hon = 0;
    if (nn->type == NPC_INACTIVE || nn->type == NPC_TRADER)
        hon = out.winner == 0 ? -(out.lost_value[1] / 5000.0 + 2) : 0;
    else if (sn.points > me * 0.5 || nn->type == NPC_PIRATE || nn->type == NPC_XENO)
        hon = (out.lost_value[1] + out.lost_value[0] * 0.5) / 4000.0;
    else hon = -(out.lost_value[1] / 8000.0);
    g->honour += hon;
    char body[MSG_BODY];
    char aname[64], dname[64];
    snprintf(aname, sizeof aname, "Atacante %s", g->name);
    snprintf(dname, sizeof dname, "Defensor %s", nn->name);
    combat_report(&in, &out, aname, dname, body, sizeof body);
    size_t len = strlen(body);
    char a[32], b[32], c[32];
    fmt_num(a, sizeof a, loot[0]);
    fmt_num(b, sizeof b, loot[1]);
    fmt_num(c, sizeof c, loot[2]);
    snprintf(body + len, sizeof body - len,
             "Saque: %s metal, %s cristal, %s deutério\nHonra: %+.0f\n", a, b, c, hon);
    char title[MSG_TITLE];
    snprintf(title, sizeof title, "Combate em [%d:%d:%d] %s - %s", pl->g, pl->s, pl->p,
             pl->pname,
             out.winner == 0   ? "vitória"
             : out.winner == 1 ? "derrota"
                               : "empate");
    game_msg(g, MSG_COMBAT, title, "%s", body);
    game_toast(g, out.winner == 0 ? RGB(110, 215, 110) : RGB(235, 85, 70),
               "Combate em [%d:%d:%d]: %s", pl->g, pl->s, pl->p,
               out.winner == 0   ? "vitória!"
               : out.winner == 1 ? "derrota"
                                 : "empate");
    if (fleet_ship_count(f->ship) <= 0) f->used = 0;
}

static void colonize(Game *g, Fleet *f) {
    char why[128];
    if (!target_empty(g, f->tg, f->ts, f->tp) || colonies_count(g) >= max_colonies(g) ||
        !position_allowed(g, f->tp)) {
        game_msg(g, MSG_FLEET, "Colonização falhou",
                 "A posição [%d:%d:%d] não pôde ser colonizada. A frota retorna.",
                 f->tg, f->ts, f->tp);
        return;
    }
    (void)why;
    PlanetInfo pi;
    planet_generate(g->seed, f->tg, f->ts, f->tp, 0, &pi);
    char name[32];
    snprintf(name, sizeof name, "Colônia %d", colonies_count(g));
    int bi = body_new(g, name, f->tg, f->ts, f->tp, 0, &pi);
    if (bi < 0) {
        game_msg(g, MSG_FLEET, "Colonização falhou",
                 "Limite de corpos celestes atingido.");
        return;
    }
    Body *b = &g->body[bi];
    f->ship[SI(S_COLONY)]--;
    for (int i = 0; i < NRES; i++) {
        b->res[i] += f->cargo[i];
        f->cargo[i] = 0;
    }
    b->pop = 120;
    b->res[RES_FOOD] += 500;
    set_explored(g, f->tg, f->ts, f->tp);
    g->flags |= FL_COLONIZED;
    if (pi.has_moon) create_moon(g, bi, pi.moon_diam);
    game_msg(g, MSG_FLEET, "Nova colônia fundada!",
             "Seus colonos chegaram a [%d:%d:%d].\nBioma %s, %d campos, %d°C a "
             "%d°C.\nFertilidade %.0f%%. Riqueza M%+.0f%% C%+.0f%% D%+.0f%%.%s",
             f->tg, f->ts, f->tp, BIOME_NAME[pi.biome], pi.fields, pi.tmin, pi.tmax,
             pi.fertility * 100, (pi.rich[0] - 1) * 100, (pi.rich[1] - 1) * 100,
             (pi.rich[2] - 1) * 100, pi.has_moon ? "\nO planeta possui uma lua!" : "");
    game_toast(g, RGB(110, 215, 110), "Nova colônia em [%d:%d:%d]!", f->tg, f->ts,
               f->tp);
    if (fleet_ship_count(f->ship) <= 0) f->used = 0;
}

static void recycle(Game *g, Fleet *f) {
    Debris *d = debris_at(g, f->tg, f->ts, f->tp, 0);
    if (!d) {
        game_msg(g, MSG_FLEET, "Reciclagem",
                 "Nenhum destroço encontrado em [%d:%d:%d].", f->tg, f->ts, f->tp);
        return;
    }
    long long rec[N_SHIP] = {0};
    rec[SI(S_RECYCLER)] = f->ship[SI(S_RECYCLER)];
    double cap = fleet_cargo_cap(g, rec);
    double used = f->cargo[0] + f->cargo[1] + f->cargo[2] + f->cargo[3];
    double fcap = fleet_cargo_cap(g, f->ship) - used;
    if (cap > fcap) cap = fcap;
    double m = fmin(d->m, cap / 2), c = fmin(d->c, cap - m);
    if (m < cap - c) m = fmin(d->m, cap - c);
    d->m -= m;
    d->c -= c;
    f->cargo[0] += floor(m);
    f->cargo[1] += floor(c);
    char a[32], b[32];
    fmt_num(a, sizeof a, m);
    fmt_num(b, sizeof b, c);
    game_msg(g, MSG_FLEET, "Reciclagem concluída",
             "Recicladores coletaram %s metal e %s cristal em [%d:%d:%d].", a, b, f->tg,
             f->ts, f->tp);
    g->flags |= FL_RECYCLED;
}

static double exped_cap(const Game *g, const long long *ships) {
    double top = 0;
    for (int i = 0; i < N_NPC; i++)
        if (g->npc[i].type != NPC_PIRATE && g->npc[i].type != NPC_XENO) {
            double p = npc_points(g, i);
            if (p > top) top = p;
        }
    double cap;
    if (top < 10000) cap = 40000;
    else if (top < 100000) cap = 500000;
    else if (top < 1000000) cap = 1200000;
    else cap = 2500000;
    /* pontos de expedição: o que a frota consegue achar depende do tamanho dela
     * (integridade estrutural) */
    double si = 0;
    for (int i = 0; i < N_SHIP; i++) si += (double)ships[i] * ITEMS[T_END + i].si;
    double fleetcap = si * 2.5 + 5000.0;
    return fmin(cap, fleetcap);
}

static void expedition_outcome(Game *g, Fleet *f) {
    Rng *r = &g->rng;
    double roll = rng_double(r) * 100.0;
    int pf = f->ship[SI(S_PATHFINDER)] > 0;
    double cap = exped_cap(g, f->ship);
    double fcap = fleet_cargo_cap(g, f->ship);
    char title[MSG_TITLE];
    snprintf(title, sizeof title, "Expedição em [%d:%d:16]", f->tg, f->ts);
    g->flags |= FL_EXPED;
    if (roll < 32.5) {
        int type = rng_double(r) < 0.68 ? 0 : (rng_double(r) < 0.75 ? 1 : 2);
        double amount = cap * rng_uniform(r, 0.1, 1.0) * (pf ? 2.0 : 1.0) /
                        (type == 0   ? 1
                         : type == 1 ? 2
                                     : 3);
        double room = fcap - (f->cargo[0] + f->cargo[1] + f->cargo[2] + f->cargo[3]);
        if (amount > room) amount = room;
        if (amount < 0) amount = 0;
        f->cargo[type] += floor(amount);
        char a[32];
        fmt_num(a, sizeof a, amount);
        game_msg(g, MSG_EXPED, title,
                 "Sua frota encontrou um asteroide rico! Foram coletados %s de %s.%s",
                 a, res_name(type),
                 amount >= room - 1
                     ? "\nOs porões estavam cheios: leve mais cargueiros na próxima."
                     : "");
    } else if (roll < 54.5) {
        long long found[N_SHIP] = {0};
        double pts = player_points(g, NULL, NULL, NULL, NULL);
        compose_fleet(cap * rng_uniform(r, 0.15, 0.6), pts < 500 ? 400 : pts, found);
        found[SI(S_PROBE)] /= 2;
        char list[800];
        ships_line(list, sizeof list, found);
        for (int i = 0; i < N_SHIP; i++) f->ship[i] += found[i];
        game_msg(
            g, MSG_EXPED, title,
            "Um cemitério de naves abandonadas! Algumas puderam ser reativadas:\n%s",
            list[0] ? list : "  (nada aproveitável)\n");
    } else if (roll < 63.5) {
        double dm = floor(rng_uniform(r, 300, 1800) * (pf ? 1.5 : 1.0));
        g->dm += dm;
        game_msg(g, MSG_EXPED, title,
                 "A frota encontrou uma anomalia estelar e coletou \001m%.0f Matéria "
                 "Negra\001d!",
                 dm);
    } else if (roll < 81.5) {
        static const char *NADA[] = {
            "Apenas o vazio. A tripulação jura ter ouvido sussurros no rádio.",
            "Um belo campo de nebulosa colorida, mas nada de valor.",
            "Um sinal promissor revelou-se um satélite meteorológico antigo.",
            "A expedição catalogou três novas espécies de micróbios. Os cientistas "
            "estão felizes; o tesouro, não."};
        game_msg(g, MSG_EXPED, title, "%s", NADA[rng_range(r, 0, 3)]);
    } else if (roll < 87.3 || roll < 89.9) {
        int xeno = roll >= 87.3;
        double fv = items_value(f->ship, NULL);
        long long en[N_SHIP] = {0};
        double pts = player_points(g, NULL, NULL, NULL, NULL);
        compose_fleet(fv * (xeno ? rng_uniform(r, 0.4, 1.0) : rng_uniform(r, 0.3, 0.6)),
                      pts < 500 ? 400 : pts, en);
        CombatIn in;
        memset(&in, 0, sizeof in);
        for (int i = 0; i < N_SHIP; i++) {
            in.n[0][i] = en[i];
            in.n[1][i] = f->ship[i];
        }
        int base = g->tech[TI(T_WEAPONS)];
        in.wt[0] = clampi(base + (xeno ? 3 : -3), 0, 40);
        in.st[0] = clampi(g->tech[TI(T_SHIELDING)] + (xeno ? 3 : -3), 0, 40);
        in.at[0] = clampi(g->tech[TI(T_ARMOR)] + (xeno ? 3 : -3), 0, 40);
        in.wt[1] = g->tech[TI(T_WEAPONS)];
        in.st[1] = g->tech[TI(T_SHIELDING)];
        in.at[1] = g->tech[TI(T_ARMOR)];
        CombatOut out;
        combat_run(r, &in, &out);
        for (int i = 0; i < N_SHIP; i++) f->ship[i] = out.end[1][i];
        char body[MSG_BODY];
        combat_report(&in, &out, xeno ? "Xenos desconhecidos" : "Piratas espaciais",
                      "Sua expedição", body, sizeof body);
        game_msg(g, MSG_COMBAT, title, "%s\n%s",
                 xeno ? "Naves alienígenas atacaram sem aviso!"
                      : "Piratas emboscaram sua expedição!",
                 body);
    } else if (roll < 96.9) {
        double extra = (f->ret - f->hold) * rng_uniform(r, 0.5, 1.5);
        f->ret += extra;
        char t[32];
        fmt_time(t, sizeof t, extra);
        game_msg(g, MSG_EXPED, title,
                 "Uma falha no hiperpropulsor atrasou o retorno da frota em %s.", t);
    } else if (roll < 99.4) {
        f->ret = f->hold + (f->ret - f->hold) * 0.5;
        game_msg(g, MSG_EXPED, title,
                 "Uma corrente gravitacional favorável acelera o retorno da frota.");
    } else if (roll < 99.7) {
        g->ev.merchant_until = g->now + 3600.0 * 6 / g->speed;
        game_msg(g, MSG_EXPED, title,
                 "A frota fez contato com um mercador nômade. Ele visitará seu "
                 "império: taxas zero no Mercador por algumas horas!");
    } else {
        game_msg(g, MSG_EXPED, title,
                 "\001rA frota foi engolida por um buraco negro. Não há "
                 "sobreviventes.\001d");
        f->used = 0;
        return;
    }
    if (fleet_ship_count(f->ship) <= 0) f->used = 0;
}

static void arrive(Game *g, int fi) {
    Fleet *f = &g->fleet[fi];
    if (f->mission == M_HOSTILE || f->mission == M_HOSTILE_SPY) {
        hostile_arrive(g, fi);
        return;
    }
    int npi = f->tp <= 15 ? npc_planet_at(g, f->tg, f->ts, f->tp) : -1;
    f->state = FS_RETURN;
    switch (f->mission) {
    case M_TRANSPORT: {
        int bi = player_body_at(g, f->tg, f->ts, f->tp, f->ttype == TT_MOON);
        if (bi >= 0) {
            for (int i = 0; i < NRES; i++) {
                g->body[bi].res[i] += f->cargo[i];
                f->cargo[i] = 0;
            }
            game_msg(g, MSG_FLEET, "Transporte entregue",
                     "Sua frota entregou a carga em %s [%d:%d:%d].", g->body[bi].name,
                     f->tg, f->ts, f->tp);
        }
    } break;
    case M_DEPLOY: {
        int bi = player_body_at(g, f->tg, f->ts, f->tp, f->ttype == TT_MOON);
        if (bi >= 0) {
            f->origin = bi;
            fleet_home(g, f);
            game_msg(g, MSG_FLEET, "Frota estacionada",
                     "Sua frota foi estacionada em %s [%d:%d:%d].", g->body[bi].name,
                     f->tg, f->ts, f->tp);
            return;
        }
    } break;
    case M_ESPIONAGE:
        if (npi >= 0) spy_report(g, f, npi);
        break;
    case M_EXPLORE:
        if (npi >= 0) spy_report(g, f, npi);
        else explore_report(g, f);
        break;
    case M_ATTACK:
        if (npi >= 0) attack_npc(g, f, npi);
        break;
    case M_COLONIZE: colonize(g, f); break;
    case M_RECYCLE: recycle(g, f); break;
    case M_EXPEDITION:
        f->state = FS_HOLD;
        f->hold = f->arrive + 3600.0 / g->speed;
        f->ret = f->hold + (f->arrive - f->depart);
        return;
    }
}

void fleet_process(Game *g) {
    for (int i = 0; i < MAX_FLEETS; i++) {
        Fleet *f = &g->fleet[i];
        if (!f->used) continue;
        for (int guard = 0; guard < 4 && f->used; guard++) {
            if (f->state == FS_OUT && f->arrive <= g->now) arrive(g, i);
            else if (f->state == FS_HOLD && f->hold <= g->now) {
                f->state = FS_RETURN;
                expedition_outcome(g, f);
            } else if (f->state == FS_RETURN && f->ret <= g->now &&
                       f->mission != M_HOSTILE && f->mission != M_HOSTILE_SPY) {
                fleet_home(g, f);
            } else break;
        }
    }
}

double fleet_next_time(const Game *g) {
    double t = 1e300;
    for (int i = 0; i < MAX_FLEETS; i++) {
        const Fleet *f = &g->fleet[i];
        if (!f->used) continue;
        double x = f->state == FS_OUT    ? f->arrive
                   : f->state == FS_HOLD ? f->hold
                                         : f->ret;
        if (x < t) t = x;
    }
    return t;
}

/* ---------------- portal de salto ---------------- */
int jump_gate(Game *g, int from, int to, char *err, size_t n) {
    if (from < 0 || to < 0 || from >= g->nbody || to >= g->nbody || from == to) {
        snprintf(err, n, "Destino inválido");
        return 0;
    }
    Body *a = &g->body[from], *b = &g->body[to];
    if (!a->is_moon || !b->is_moon || a->bld[B_JUMP_GATE] < 1 ||
        b->bld[B_JUMP_GATE] < 1) {
        snprintf(err, n, "Ambas as luas precisam de Portal de Salto");
        return 0;
    }
    if (a->gate_ready > g->now || b->gate_ready > g->now) {
        char t[32];
        fmt_time(t, sizeof t, fmax(a->gate_ready, b->gate_ready) - g->now);
        snprintf(err, n, "Portais recarregando: %s", t);
        return 0;
    }
    long long moved = 0;
    for (int i = 0; i < N_SHIP; i++) {
        if (T_END + i == S_SATELLITE || T_END + i == S_CRAWLER) continue;
        b->ship[i] += a->ship[i];
        moved += a->ship[i];
        a->ship[i] = 0;
    }
    if (moved == 0) {
        snprintf(err, n, "Nenhuma nave para saltar");
        return 0;
    }
    a->gate_ready = b->gate_ready = g->now + 3600.0 / g->speed;
    snprintf(err, n, "%lld naves saltaram para %s", moved, b->name);
    return 1;
}

/* ---------------- mísseis interplanetários ---------------- */
int ipm_range(const Game *g) {
    int imp = g->tech[TI(T_IMPULSE)];
    return imp > 0 ? 5 * imp - 1 : 0;
}

int ipm_launch(Game *g, int bi, int npi, long long n, char *err, size_t en) {
    Body *b = &g->body[bi];
    if (npi < 0 || npi >= g->nnpl) {
        snprintf(err, en, "Alvo inválido");
        return 0;
    }
    const NpcPlanet *pl = &g->npl[npi];
    if (n <= 0 || n > b->def[DI(D_IPM)]) {
        snprintf(err, en, "Mísseis insuficientes");
        return 0;
    }
    int dist = abs(pl->s - b->s);
    if (pl->g != b->g || dist > ipm_range(g)) {
        snprintf(err, en, "Fora do alcance (%d sistemas: Motor de Impulso x5-1)",
                 ipm_range(g));
        return 0;
    }
    NpcSnap sn;
    npc_snapshot(g, npi, &sn);
    b->def[DI(D_IPM)] -= n;
    long long abm = sn.def[DI(D_ABM)];
    long long hit = n - (abm < n ? abm : n);
    double dv = items_value(NULL, sn.def);
    double dmg = hit * 12000.0 * (1.0 + 0.1 * g->tech[TI(T_WEAPONS)]) * 4.0;
    double frac = dv > 0 ? fmin(0.95, dmg / dv) : 0;
    Touched *t = touched_get(g, pl->g, pl->s, pl->p, 1);
    double hrs = (g->now - t->t_dmg) * g->speed / 3600.0;
    double cd = t->def_dmg * exp(-hrs / 18.0);
    t->def_dmg = clampd(cd + frac * (1 - cd), 0, 0.98);
    t->t_dmg = g->now;
    game_msg(g, MSG_COMBAT, "Ataque de mísseis interplanetários",
             "%lld mísseis lançados contra %s [%d:%d:%d].\n%lld interceptados por "
             "antibalísticos.\n%lld atingiram o alvo: cerca de %.0f%% das defesas "
             "inimigas foram destruídas.\n"
             "As defesas se reconstroem com o tempo: ataque logo com sua frota.",
             n, pl->pname, pl->g, pl->s, pl->p, n - hit, hit, frac * 100);
    snprintf(err, en, "%lld mísseis disparados (%.0f%% das defesas destruídas)", n,
             frac * 100);
    return 1;
}
