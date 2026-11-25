/* tests.c - testes automáticos (./novomundo --selftest) */
#include "game.h"
#include "term.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0, passes = 0, VERB = 1;

#define CHECK(cond, ...)                                                               \
    do {                                                                               \
        if (cond) {                                                                    \
            passes++;                                                                  \
        } else {                                                                       \
            fails++;                                                                   \
            printf("  FALHOU: ");                                                      \
            printf(__VA_ARGS__);                                                       \
            printf("  (%s:%d)\n", __FILE__, __LINE__);                                 \
        }                                                                              \
    } while (0)
#define NEAR(a, b, tol) (fabs((a) - (b)) <= (tol))

static void quiet(Game *g) {
    g->ev.next_event = 1e18;
    g->quest = quest_count();
}

static void t_formulas(void) {
    printf("[fórmulas]\n");
    double c[3];
    item_cost1(B_METAL_MINE, 1, c);
    CHECK(c[0] == 60 && c[1] == 15, "mina de metal nv1 custa 60/15 (deu %.0f/%.0f)",
          c[0], c[1]);
    item_cost1(B_METAL_MINE, 2, c);
    CHECK(c[0] == 90 && c[1] == 22, "mina de metal nv2 custa 90/22 (deu %.0f/%.0f)",
          c[0], c[1]);
    item_cost1(B_CRYSTAL_MINE, 2, c);
    CHECK(NEAR(c[0], 76, 0.5) && NEAR(c[1], 38, 0.5),
          "mina de cristal nv2 76/38 (deu %.0f/%.0f)", c[0], c[1]);
    item_cost1(T_ASTRO, 2, c);
    CHECK(c[0] == 7000 && c[1] == 14000 && c[2] == 7000,
          "astrofísica nv2 7000/14000/7000 (deu %.0f/%.0f/%.0f)", c[0], c[1], c[2]);
    item_cost1(T_ASTRO, 3, c);
    CHECK(c[0] == 12300 && c[1] == 24500, "astrofísica nv3 12300/24500 (deu %.0f/%.0f)",
          c[0], c[1]);
    CHECK(NEAR(energy_for(B_METAL_MINE, 1), 11, 1e-9),
          "energia mina metal nv1 = 11 (deu %f)", energy_for(B_METAL_MINE, 1));
    CHECK(NEAR(energy_for(B_METAL_MINE, 10), 259.37, 0.01),
          "energia mina metal nv10 = 259 (deu %f)", energy_for(B_METAL_MINE, 10));
    CHECK(NEAR(prod_metal_lvl(1), 33, 1e-9), "produção metal nv1 = 33/h (x1)");
    CHECK(storage_cap(0) == 10000 && storage_cap(1) == 20000 && storage_cap(2) == 40000,
          "armazém 10k/20k/40k (deu %.0f %.0f %.0f)", storage_cap(0), storage_cap(1),
          storage_cap(2));
    CHECK(fleet_distance(1, 1, 1, 1, 1, 2) == 1005, "distância mesmo sistema");
    CHECK(fleet_distance(1, 1, 1, 1, 3, 1) == 2700 + 190, "distância sistemas");
    CHECK(fleet_distance(1, 1, 1, 3, 1, 1) == 40000, "distância galáxias");
    CHECK(fleet_distance(1, 1, 1, 9, 1, 1) == 20000, "galáxias circulares (1->9)");
    double loot[3], av[3] = {10000, 10000, 10000};
    loot_take(av, 9000, loot);
    CHECK(NEAR(loot[0] + loot[1] + loot[2], 9000, 1),
          "saque preenche a capacidade (%.0f)", loot[0] + loot[1] + loot[2]);
    loot_take(av, 1e9, loot);
    CHECK(NEAR(loot[0], 5000, 1) && NEAR(loot[1], 5000, 1) && NEAR(loot[2], 5000, 1),
          "saque máximo é 50%%");
    CHECK(rapidfire(item_unit(S_CRUISER), item_unit(S_LIGHT_FIGHTER)) == 6,
          "rapidfire cruzador vs caça = 6");
    CHECK(rapidfire(item_unit(S_DEATHSTAR), item_unit(S_PROBE)) == 1250,
          "disparo rápido: estrela da morte contra sonda = 1250");
    char s[64];
    fmt_num(s, sizeof s, 1234567);
    CHECK(!strcmp(s, "1.234.567"), "formato pt-BR de números (%s)", s);
    fmt_num(s, sizeof s, -980);
    CHECK(!strcmp(s, "-980"), "negativo (%s)", s);
}

static void t_seed(void) {
    printf("[semente e geração]\n");
    Game *a = calloc(1, sizeof(Game)), *b = calloc(1, sizeof(Game));
    game_new(a, "Lucas", "Casa", 10, "20262411591403");
    game_new(b, "Lucas", "Casa", 10, "20262411591403");
    CHECK(a->seed == b->seed, "mesma data+nome = mesma semente");
    CHECK(a->body[0].g == b->body[0].g && a->body[0].s == b->body[0].s &&
              a->body[0].p == b->body[0].p,
          "mesma posição");
    CHECK(a->body[0].info.vseed == b->body[0].info.vseed &&
              a->body[0].info.biome == b->body[0].info.biome,
          "mesmo planeta");
    game_new(b, "Lucaz", "Casa", 10, "20262411591403");
    CHECK(a->seed != b->seed, "um caractere do nome muda a semente");
    game_new(b, "Lucas", "Casa", 10, "20262411591404");
    CHECK(a->seed != b->seed, "um segundo muda a semente");
    char ss[48];
    make_seedstr(ss, sizeof ss, 1795546648.0);
    CHECK(strlen(ss) == 14, "semente temporal tem 14 dígitos (%s)", ss);
    /* muitos planetas natais são habitáveis */
    int okfert = 1, okfields = 1;
    for (int i = 0; i < 300; i++) {
        PlanetInfo pi;
        planet_generate(0x1234 + (uint64_t)i * 7919, 1 + i % 9, 1 + i, 4 + i % 9, 1,
                        &pi);
        if (pi.fertility < 0.6) okfert = 0;
        if (pi.fields < 140 || pi.fields > 260) okfields = 0;
        if (pi.tmax - pi.tmin < 30) okfields = 0;
    }
    CHECK(okfert, "planeta natal sempre fértil (>=60%%)");
    CHECK(okfields, "planeta natal com 140-260 campos e amplitude térmica");
    CHECK(a->nnpl > 1000, "universo com mais de 1000 planetas NPC (%d)", a->nnpl);
    int collide = 0;
    for (int i = 1; i < a->nnpl; i++)
        if (a->npl[i].g == a->npl[i - 1].g && a->npl[i].s == a->npl[i - 1].s &&
            a->npl[i].p == a->npl[i - 1].p)
            collide++;
    CHECK(collide == 0, "sem colisão de planetas NPC (%d)", collide);
    CHECK(npc_planet_at(a, a->body[0].g, a->body[0].s, a->body[0].p) < 0,
          "nenhum NPC no planeta natal");
    universe_free(a);
    universe_free(b);
    free(a);
    free(b);
}

static void t_economy(void) {
    printf("[economia e cadeia população/alimento]\n");
    Game *g = calloc(1, sizeof(Game));
    game_new(g, "Teste", "Casa", 1, "20262411591403");
    quiet(g);
    Body *b = &g->body[0];
    double m0 = b->res[0];
    Rates r;
    body_rates(g, b, &r);
    CHECK(NEAR(r.prod[RES_METAL],
               30 * (b->p == 8                 ? 1.35
                     : b->p == 7 || b->p == 9  ? 1.23
                     : b->p == 6 || b->p == 10 ? 1.17
                                               : 1.0),
               1e-6),
          "produção base de metal sem minas");
    game_advance(g, g->now + 3600, 0);
    CHECK(b->res[0] > m0 + 25, "metal sobe com o tempo (%.1f -> %.1f)", m0, b->res[0]);
    int rc = enqueue(g, 0, B_METAL_MINE, 1);
    CHECK(rc == BC_OK, "enfileirar mina (rc=%d)", rc);
    CHECK(b->bq.n == 1, "fila com 1 item");
    double before = b->res[0];
    game_advance(g, g->now + 1, 0);
    CHECK(b->bq.it[0].active && b->res[0] < before, "recursos debitados ao iniciar");
    game_advance(g, g->now + 3600, 0);
    CHECK(b->bld[B_METAL_MINE] == 1, "mina concluída (nv %d)", b->bld[B_METAL_MINE]);
    body_rates(g, b, &r);
    CHECK(r.e_eff < 0.01, "sem usina solar a mina não tem energia (ef %.2f)", r.e_eff);
    CHECK(enqueue(g, 0, B_FUSION, 1) == BC_REQ, "reator de fusão exige requisitos");
    CHECK(enqueue(g, 0, T_ENERGY, 1) == BC_REQ, "pesquisa exige laboratório");
    /* população precisa de comida: destrói as fazendas e o alimento, confere a fome */
    b->res[RES_FOOD] = 0;
    b->pop = 5000;
    double p0 = b->pop;
    game_advance(g, g->now + 3600 * 5, 0);
    CHECK(b->pop < p0, "população cai sem comida (%.0f -> %.0f)", p0, b->pop);
    /* mão de obra: muitas minas, ninguém para trabalhar */
    b->bld[B_METAL_MINE] = 20;
    b->bld[B_SOLAR_PLANT] = 25;
    b->pop = 10;
    body_rates(g, b, &r);
    CHECK(r.wf < 0.1, "sem trabalhadores a eficiência cai (wf %.2f)", r.wf);
    CHECK(r.fac_eff >= 0.25, "instalações nunca abaixo de 25%%");
    b->pop = 100000;
    body_rates(g, b, &r);
    CHECK(r.wf > 0.999, "com trabalhadores a eficiência volta (wf %.2f)", r.wf);
    /* limite de armazenamento */
    b->res[0] = r.cap[0] - 1;
    game_advance(g, g->now + 7200, 0);
    CHECK(b->res[0] <= r.cap[0] + 1, "armazém limita a produção");
    /* pontos */
    double p = player_points(g, NULL, NULL, NULL, NULL);
    CHECK(p > 0, "pontos > 0 (%.1f)", p);
    universe_free(g);
    free(g);
}

static void t_queues(void) {
    printf("[filas, estaleiro, cancelamento, matéria negra]\n");
    Game *g = calloc(1, sizeof(Game));
    game_new(g, "Fila", "Casa", 10, "20262411591403");
    quiet(g);
    Body *b = &g->body[0];
    b->res[0] = b->res[1] = b->res[2] = 1e7;
    b->bld[B_ROBOTICS] = 2;
    b->bld[B_SHIPYARD] = 2;
    b->bld[B_SOLAR_PLANT] = 20;
    g->tech[TI(T_COMBUSTION)] = 2;
    CHECK(enqueue(g, 0, B_METAL_MINE, 1) == BC_OK &&
              enqueue(g, 0, B_METAL_MINE, 1) == BC_OK,
          "duas obras na fila");
    CHECK(enqueue(g, 0, B_METAL_MINE, 1) == BC_QUEUE,
          "terceira bloqueada sem comandante");
    CHECK(b->bq.it[1].amount == 2, "segundo item mira o nível 2");
    double r0 = b->res[0];
    game_advance(g, g->now + 0.5, 0);
    cancel_queue(g, 0, 0, 0);
    CHECK(NEAR(b->res[0], r0, 2.0), "cancelar devolve recursos (%.0f vs %.0f)",
          b->res[0], r0);
    CHECK(b->bq.n == 0, "cancelar remove os níveis seguintes");
    CHECK(enqueue(g, 0, S_SMALL_CARGO, 10) == BC_OK, "10 cargueiros na fila");
    game_advance(g, g->now + 24 * 3600, 0);
    CHECK(b->ship[SI(S_SMALL_CARGO)] == 10, "10 cargueiros entregues (%lld)",
          b->ship[SI(S_SMALL_CARGO)]);
    CHECK(enqueue(g, 0, D_SMALL_DOME, 2) == BC_MAX ||
              can_build(g, 0, D_SMALL_DOME, 2) != BC_OK,
          "cúpula: máximo 1");
    enqueue(g, 0, B_METAL_MINE, 1);
    game_advance(g, g->now + 0.5, 0);
    double dm0 = g->dm;
    int r = finish_with_dm(g, 0, 0);
    game_advance(g, g->now + 0.5, 0);
    CHECK(r == 1 && g->dm < dm0 && b->bld[B_METAL_MINE] >= 1,
          "finalizar com matéria negra");
    universe_free(g);
    free(g);
}

static void t_combat(void) {
    printf("[combate]\n");
    Rng r;
    rng_seed(&r, 42);
    CombatIn in;
    CombatOut o;
    memset(&in, 0, sizeof in);
    in.n[0][item_unit(S_LIGHT_FIGHTER)] = 100;
    in.n[1][item_unit(D_ROCKET)] = 1;
    combat_run(&r, &in, &o);
    CHECK(o.winner == 0, "100 caças vencem 1 lançador");
    memset(&in, 0, sizeof in);
    in.n[0][item_unit(S_LIGHT_FIGHTER)] = 1;
    in.n[1][item_unit(D_ROCKET)] = 100;
    combat_run(&r, &in, &o);
    CHECK(o.winner == 1, "1 caça perde para 100 lançadores");
    CHECK(o.debris[0] == 900 && o.debris[1] == 300,
          "destrocos 30%% do caça (%.0f/%.0f)", o.debris[0], o.debris[1]);
    memset(&in, 0, sizeof in);
    in.n[0][item_unit(S_BATTLESHIP)] = 1;
    in.n[1][item_unit(D_SMALL_DOME)] = 1;
    combat_run(&r, &in, &o);
    CHECK(o.rounds == 6 && o.winner == -1,
          "nave de batalha x cúpula = empate em 6 rodadas");
    /* tecnologia faz diferença */
    int w0 = 0, w1 = 0;
    for (int i = 0; i < 20; i++) {
        memset(&in, 0, sizeof in);
        in.n[0][item_unit(S_CRUISER)] = 50;
        in.n[1][item_unit(S_CRUISER)] = 50;
        in.wt[0] = 10;
        in.st[0] = 10;
        in.at[0] = 10;
        combat_run(&r, &in, &o);
        if (o.winner == 0) w0++;
        if (o.winner == 1) w1++;
    }
    CHECK(w0 > w1 * 3, "tecnologia superior vence (%d x %d)", w0, w1);
    /* reparo de defesas */
    memset(&in, 0, sizeof in);
    in.n[0][item_unit(S_BATTLESHIP)] = 200;
    in.n[1][item_unit(D_ROCKET)] = 1000;
    combat_run(&r, &in, &o);
    long long lost = o.start[1][item_unit(D_ROCKET)] - o.end[1][item_unit(D_ROCKET)];
    double frac =
        lost > 0 ? (double)o.repaired[item_unit(D_ROCKET)] / (double)lost : 0.7;
    CHECK(frac > 0.6 && frac < 0.8, "~70%% das defesas reparadas (%.2f)", frac);
}

static void t_fleet(void) {
    printf("[frotas, espionagem, colonização, expedição, transporte]\n");
    Game *g = calloc(1, sizeof(Game));
    game_new(g, "Frota", "Casa", 10, "20262411591403");
    quiet(g);
    Body *b = &g->body[0];
    b->res[0] = b->res[1] = b->res[2] = 1e7;
    b->ship[SI(S_SMALL_CARGO)] = 10;
    b->ship[SI(S_PROBE)] = 5;
    b->ship[SI(S_COLONY)] = 1;
    b->ship[SI(S_LIGHT_FIGHTER)] = 50;
    g->tech[TI(T_COMPUTER)] = 10;
    g->tech[TI(T_ASTRO)] = 4;
    g->tech[TI(T_COMBUSTION)] = 6;
    g->tech[TI(T_IMPULSE)] = 4;
    char err[160];
    /* acha um npc */
    int npi = -1;
    for (int i = 0; i < g->nnpl; i++)
        if (g->npl[i].g == b->g) {
            npi = i;
            break;
        }
    CHECK(npi >= 0, "existe NPC na galáxia");
    long long s[N_SHIP] = {0};
    s[SI(S_PROBE)] = 2;
    int ok = fleet_launch(g, 0, M_ESPIONAGE, g->npl[npi].g, g->npl[npi].s,
                          g->npl[npi].p, TT_PLANET, s, NULL, 100, err, sizeof err);
    CHECK(ok, "lançar espionagem (%s)", err);
    CHECK(b->ship[SI(S_PROBE)] == 3, "sondas saem do planeta");
    int nm = g->nmsg;
    game_advance(g, g->now + 3 * 3600, 0);
    CHECK(g->flags & FL_SPIED, "relatório de espionagem gerado");
    CHECK(g->nmsg > nm, "mensagem criada");
    /* coloniza uma posição vazia */
    int cs = -1, cp = -1;
    for (int ds = 1; ds < 50 && cs < 0; ds++)
        for (int p = 4; p <= 12; p++)
            if (npc_planet_at(g, b->g, clampi(b->s + ds, 1, UNI_SYSTEMS), p) < 0) {
                cs = clampi(b->s + ds, 1, UNI_SYSTEMS);
                cp = p;
                break;
            }
    memset(s, 0, sizeof s);
    s[SI(S_COLONY)] = 1;
    double cargo[NRES] = {1000, 500, 0, 0};
    ok = fleet_launch(g, 0, M_COLONIZE, b->g, cs, cp, TT_PLANET, s, cargo, 100, err,
                      sizeof err);
    CHECK(ok, "lançar colonização (%s)", err);
    game_advance(g, g->now + 6 * 3600, 0);
    CHECK(colonies_count(g) == 2, "nova colônia criada (%d)", colonies_count(g));
    int col = player_body_at(g, b->g, cs, cp, 0);
    CHECK(col > 0 && g->body[col].res[0] >= 1000, "colônia recebeu a carga");
    if (col <= 0) {
        for (int i = 0; i < g->nmsg; i++)
            printf("    msg: %s | %.200s\n", g->msg[i].title, g->msg[i].body);
        universe_free(g);
        free(g);
        return;
    }
    /* transporte: planeta natal -> colônia */
    memset(s, 0, sizeof s);
    s[SI(S_SMALL_CARGO)] = 2;
    double cg[NRES] = {5000, 0, 0, 1000};
    double before = g->body[col].res[0];
    ok = fleet_launch(g, 0, M_TRANSPORT, b->g, cs, cp, TT_PLANET, s, cg, 100, err,
                      sizeof err);
    CHECK(ok, "lançar transporte (%s)", err);
    game_advance(g, g->now + 6 * 3600, 0);
    CHECK(g->body[col].res[0] >= before + 5000, "transporte entregue");
    CHECK(b->ship[SI(S_SMALL_CARGO)] == 10, "cargueiros voltaram (%lld)",
          b->ship[SI(S_SMALL_CARGO)]);
    /* expedição */
    memset(s, 0, sizeof s);
    s[SI(S_SMALL_CARGO)] = 3;
    ok = fleet_launch(g, 0, M_EXPEDITION, b->g, b->s, 16, TT_PLANET, s, NULL, 100, err,
                      sizeof err);
    CHECK(ok, "lançar expedição (%s)", err);
    game_advance(g, g->now + 12 * 3600, 0);
    CHECK(g->flags & FL_EXPED, "expedição concluiu");
    CHECK(exped_slots_used(g) == 0, "slot de expedição liberado");
    /* chamar de volta */
    memset(s, 0, sizeof s);
    s[SI(S_LIGHT_FIGHTER)] = 10;
    long long lf0 = b->ship[SI(S_LIGHT_FIGHTER)];
    ok = fleet_launch(g, 0, M_ATTACK, g->npl[npi].g, g->npl[npi].s, g->npl[npi].p,
                      TT_PLANET, s, NULL, 50, err, sizeof err);
    CHECK(ok, "lançar ataque (%s)", err);
    int fi = -1;
    for (int i = 0; i < MAX_FLEETS; i++)
        if (g->fleet[i].used && g->fleet[i].mission == M_ATTACK) fi = i;
    game_advance(g, g->now + 5, 0);
    CHECK(fi >= 0 && fleet_recall(g, fi), "chamar frota de volta");
    game_advance(g, g->now + 60, 0);
    CHECK(b->ship[SI(S_LIGHT_FIGHTER)] == lf0, "frota chamada voltou intacta (%lld)",
          b->ship[SI(S_LIGHT_FIGHTER)]);
    /* mísseis interplanetários */
    b->bld[B_MISSILE_SILO] = 4;
    b->def[DI(D_IPM)] = 10;
    g->tech[TI(T_IMPULSE)] = 4;
    int tgt = -1;
    for (int i = 0; i < g->nnpl; i++)
        if (g->npl[i].g == b->g && abs(g->npl[i].s - b->s) <= ipm_range(g)) {
            tgt = i;
            break;
        }
    if (tgt >= 0) {
        CHECK(ipm_launch(g, 0, tgt, 5, err, sizeof err), "disparar mísseis (%s)", err);
        CHECK(b->def[DI(D_IPM)] == 5, "mísseis consumidos");
    }
    /* portal de salto entre duas luas */
    create_moon(g, 0, 5000);
    int m1 = b->moon;
    PlanetInfo pi2;
    planet_generate(g->seed, b->g, b->s, 15, 0, &pi2);
    int c2 = body_new(g, "Outra", b->g, b->s == 1 ? 2 : b->s - 1, 15, 0, &pi2);
    create_moon(g, c2, 5000);
    int m2 = g->body[c2].moon;
    g->body[m1].bld[B_JUMP_GATE] = 1;
    g->body[m2].bld[B_JUMP_GATE] = 1;
    g->body[m1].ship[SI(S_CRUISER)] = 7;
    CHECK(jump_gate(g, m1, m2, err, sizeof err) && g->body[m2].ship[SI(S_CRUISER)] == 7,
          "portal de salto move a frota (%s)", err);
    CHECK(!jump_gate(g, m2, m1, err, sizeof err), "portal precisa recarregar");
    b = &g->body[0];
    /* missões inválidas */
    CHECK(!mission_valid(g, 0, M_COLONIZE, g->npl[npi].g, g->npl[npi].s, g->npl[npi].p,
                         TT_PLANET, s, err, sizeof err),
          "não coloniza planeta ocupado");
    universe_free(g);
    free(g);
}

static void t_events(void) {
    printf("[eventos, ameaça, decisões, ataques hostis]\n");
    Game *g = calloc(1, sizeof(Game));
    game_new(g, "Eventos", "Casa", 10, "20262411591403");
    Body *b = &g->body[0];
    CHECK(newbie_left(g) > 0, "começa sob proteção de novato");
    CHECK(threat_level(g, 0) == 0, "sem ameaça na proteção");
    /* desenvolve artificialmente */
    b->bld[B_METAL_MINE] = 18;
    b->bld[B_CRYSTAL_MINE] = 15;
    b->bld[B_DEUT_SYNTH] = 12;
    b->bld[B_SOLAR_PLANT] = 20;
    b->bld[B_ROBOTICS] = 6;
    b->bld[B_SHIPYARD] = 6;
    b->bld[B_LAB] = 6;
    b->bld[B_FUSION] = 3;
    b->bld[B_HABITAT] = 12;
    b->bld[B_FARM] = 10;
    b->pop = 20000;
    b->res[0] = 200000;
    b->res[1] = 100000;
    b->res[2] = 50000;
    CHECK(newbie_left(g) == 0, "proteção termina com pontos");
    int th = threat_level(g, 0);
    CHECK(th >= 7, "sem defesas a ameaça e alta (%d)", th);
    b->def[DI(D_GAUSS)] = 200;
    b->def[DI(D_HEAVY_LASER)] = 400;
    int th2 = threat_level(g, 0);
    CHECK(th2 < th, "defesas reduzem a ameaça (%d -> %d)", th, th2);
    b->def[DI(D_GAUSS)] = 0;
    b->def[DI(D_HEAVY_LASER)] = 0;
    /* roda muitos eventos */
    int types_seen = 0, hostile_seen = 0, dec_seen = 0;
    long long t0 = g->ev.total;
    for (int i = 0; i < 400; i++) {
        g->ev.next_event = g->now;
        events_process(g);
        if (g->ev.dec.active) {
            dec_seen++;
            decision_choose(g, rng_range(&g->rng, 0, g->ev.dec.nopt - 1));
            if (g->ev.dec.active) decision_close(g);
        }
        for (int f = 0; f < MAX_FLEETS; f++)
            if (g->fleet[f].used && g->fleet[f].mission == M_HOSTILE) hostile_seen++;
        game_advance(g, g->now + 120, 0);
    }
    types_seen = (int)(g->ev.total - t0);
    CHECK(types_seen >= 400, "eventos processados (%d)", types_seen);
    CHECK(dec_seen > 10, "decisões com dados ocorreram (%d)", dec_seen);
    CHECK(hostile_seen > 0, "frotas hostis foram enviadas (%d)", hostile_seen);
    CHECK(g->stat_combats > 0, "houve combates (%.0f)", g->stat_combats);
    int neg = 0;
    for (int k = 0; k < NRES; k++)
        if (b->res[k] < 0) neg = 1;
    CHECK(!neg && b->pop >= 0, "recursos e população não negativos");
    /* semântica dos dados nas decisões */
    g->ev.dec.active = 0;
    g->ev.next_event = g->now + 1e9;
    for (int i = 0; i < 50; i++) {
        memset(&g->ev.dec, 0, sizeof g->ev.dec);
        Decision *d = &g->ev.dec;
        d->active = 1;
        d->type = EV_PLAGUE;
        d->body = 0;
        d->nopt = 1;
        d->dc = 15;
        d->base_mod = 3;
        d->optbonus[0] = 2;
        snprintf(d->opt[0], sizeof d->opt[0], "teste");
        decision_choose(g, 0);
        CHECK(d->total == d->roll + 5, "total = d20 + mod + bonus");
        CHECK((d->roll == 20 && d->outcome == 2) || (d->roll == 1 && d->outcome == 3) ||
                  (d->roll > 1 && d->roll < 20 && d->outcome == (d->total >= 15)),
              "resultado coerente com a CD");
        decision_close(g);
    }
    universe_free(g);
    free(g);
}

static void t_trade(void) {
    printf("[rotas comerciais e mercador]\n");
    Game *g = calloc(1, sizeof(Game));
    game_new(g, "Comercio", "Casa", 10, "20262411591403");
    quiet(g);
    Body *b = &g->body[0];
    b->res[0] = b->res[1] = b->res[2] = 1e6;
    b->ship[SI(S_LARGE_CARGO)] = 5;
    b->bld[B_TRADE_HUB] = 2;
    g->tech[TI(T_LOGISTICS)] = 1;
    g->tech[TI(T_COMBUSTION)] = 6;
    PlanetInfo pi;
    planet_generate(g->seed, b->g, b->s, 15, 0, &pi);
    int col = body_new(g, "Colonia", b->g, b->s, b->p == 15 ? 14 : 15, 0, &pi);
    Route t;
    memset(&t, 0, sizeof t);
    t.kind = RT_INTERNAL;
    t.origin = 0;
    t.dest = col;
    t.res_out = RES_METAL;
    t.amount = 10000;
    t.ship[SI(S_LARGE_CARGO)] = 2;
    char err[160];
    CHECK(route_create(g, &t, err, sizeof err), "criar rota interna (%s)", err);
    CHECK(b->ship[SI(S_LARGE_CARGO)] == 3, "naves alocadas à rota");
    double c0 = g->body[col].res[0];
    game_advance(g, g->now + 6 * 3600, 0);
    CHECK(g->body[col].res[0] > c0 + 10000, "rota entregou metal (%.0f)",
          g->body[col].res[0] - c0);
    int ri = -1;
    for (int i = 0; i < MAX_ROUTES; i++)
        if (g->route[i].used) ri = i;
    CHECK(ri >= 0 && g->route[ri].trips >= 2, "várias viagens");
    route_delete(g, ri);
    CHECK(b->ship[SI(S_LARGE_CARGO)] == 5, "naves voltam ao desfazer");
    /* mercador npc */
    int tw = -1;
    for (int i = 0; i < g->nnpl; i++)
        if (g->npc[g->npl[i].npc].type == NPC_TRADER && g->npl[i].g == b->g) {
            tw = i;
            break;
        }
    if (tw >= 0) {
        memset(&t, 0, sizeof t);
        t.kind = RT_NPC;
        t.origin = 0;
        t.tg = g->npl[tw].g;
        t.ts = g->npl[tw].s;
        t.tp = g->npl[tw].p;
        t.res_out = RES_METAL;
        t.res_in = RES_CRYSTAL;
        t.ship[SI(S_LARGE_CARGO)] = 2;
        CHECK(route_create(g, &t, err, sizeof err), "criar rota com Mercante (%s)",
              err);
        double cr0 = b->res[1];
        game_advance(g, g->now + 24 * 3600, 0);
        CHECK(b->res[1] > cr0, "importou cristal (%.0f)", b->res[1] - cr0);
        double rt = market_rate(g, g->npl[tw].npc, RES_METAL, RES_CRYSTAL);
        CHECK(rt > 0.3 && rt < 1.5, "taxa de mercado plausível (%.3f)", rt);
    }
    b->res[0] = b->res[1] = 1e6;
    double m0 = b->res[0], k0 = b->res[1];
    CHECK(merchant_trade(g, 0, RES_METAL, RES_CRYSTAL, 3000, err, sizeof err),
          "mercador troca");
    CHECK(NEAR(b->res[1] - k0, 1800, 1) && NEAR(m0 - b->res[0], 3000, 0.1),
          "3000 metal -> 1800 cristal com taxa 10%%");
    universe_free(g);
    free(g);
}

static int file_equal(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int eq = fa && fb;
    while (eq) {
        int x = fgetc(fa), y = fgetc(fb);
        if (x != y) eq = 0;
        if (x == EOF || y == EOF) break;
    }
    if (fa) fclose(fa);
    if (fb) fclose(fb);
    return eq;
}

static void t_save(void) {
    printf("[salvar e carregar]\n");
    Game *g = calloc(1, sizeof(Game)), *h = calloc(1, sizeof(Game));
    game_new(g, "Salvo \"aspas\" e acentuação", "Lar \\ doce lar", 10,
             "20262411591403");
    for (int i = 0; i < 300; i++) {
        game_advance(g, g->now + 600, 0);
        bot_step(g);
    }
    g->ntoast = 0;
    /* funda uma colônia numa posição vazia antes de salvar: o universo precisa ser
     * regerado idêntico */
    for (int p = 4; p <= 12; p++)
        if (npc_planet_at(g, g->body[0].g, g->body[0].s == 499 ? 498 : g->body[0].s + 1,
                          p) < 0) {
            PlanetInfo pi;
            planet_generate(g->seed, g->body[0].g,
                            g->body[0].s == 499 ? 498 : g->body[0].s + 1, p, 0, &pi);
            body_new(g, "Colônia X", g->body[0].g,
                     g->body[0].s == 499 ? 498 : g->body[0].s + 1, p, 0, &pi);
            break;
        }
    CHECK(game_save(g, "teste_a.sav"), "salvar");
    CHECK(game_load(h, "teste_a.sav"), "carregar");
    h->ntoast = 0;
    CHECK(game_save(h, "teste_b.sav"), "salvar de novo");
    CHECK(file_equal("teste_a.sav", "teste_b.sav"),
          "salvar->carregar->salvar idêntico");
    CHECK(!strcmp(g->name, h->name), "nome preservado (%s)", h->name);
    int same = h->nnpl == g->nnpl;
    for (int i = 0; same && i < g->nnpl; i++)
        if (h->npl[i].g != g->npl[i].g || h->npl[i].s != g->npl[i].s ||
            h->npl[i].p != g->npl[i].p || h->npl[i].npc != g->npl[i].npc)
            same = 0;
    CHECK(same, "universo NPC regenerado idêntico após colonizar (%d planetas)",
          g->nnpl);
    /* continua os dois: precisam ficar sincronizados (determinismo) */
    for (int i = 0; i < 50; i++) {
        game_advance(g, g->now + 600, 0);
        game_advance(h, h->now + 600, 0);
    }
    CHECK(NEAR(g->body[0].res[0], h->body[0].res[0], 1e-6) &&
              g->ev.total == h->ev.total,
          "simulações idênticas após carregar");
    char nm[64];
    CHECK(save_exists("teste_a.sav", nm, sizeof nm) && !strcmp(nm, g->name),
          "detectar save existente");
    FILE *f = fopen("teste_c.sav", "w");
    fprintf(f, "lixo\n");
    fclose(f);
    CHECK(!game_load(h, "teste_c.sav"), "rejeita arquivo corrompido");
    remove("teste_a.sav");
    remove("teste_b.sav");
    remove("teste_c.sav");
    universe_free(g);
    universe_free(h);
    free(g);
    free(h);
}

static void t_offline(void) {
    printf("[progresso offline]\n");
    Game *a = calloc(1, sizeof(Game)), *b = calloc(1, sizeof(Game));
    game_new(a, "Off", "Casa", 5, "20262411591403");
    for (int i = 0; i < 120; i++) {
        game_advance(a, a->now + 300, 0);
        bot_step(a);
    }
    a->ntoast = 0;
    game_save(a, "teste_off.sav");
    game_load(b, "teste_off.sav");
    remove("teste_off.sav");
    /* a: online em passos de 1 s por 2 h ; b: um único salto offline */
    double target = a->now + 2 * 3600;
    a->ev.next_event = b->ev.next_event =
        target + 1e9; /* isola a produção dos eventos aleatórios */
    while (a->now < target) game_advance(a, a->now + 1.0, 0);
    game_advance(b, target, 1);
    for (int k = 0; k < 3; k++) {
        double x = a->body[0].res[k], y = b->body[0].res[k];
        CHECK(fabs(x - y) <= fmax(5.0, 0.01 * x),
              "recurso %d igual online/offline (%.1f vs %.1f)", k, x, y);
    }
    CHECK(fabs(a->body[0].pop - b->body[0].pop) <= fmax(2.0, 0.01 * a->body[0].pop),
          "população igual (%.1f vs %.1f)", a->body[0].pop, b->body[0].pop);
    CHECK(a->body[0].bld[B_METAL_MINE] == b->body[0].bld[B_METAL_MINE],
          "construções concluídas igualmente");
    /* 30 dias fora precisa ser rápido */
    double t0 = now_seconds();
    b->ev.next_event = b->now + 600;
    game_advance(b, b->now + 30 * 86400.0, 1);
    double el = now_seconds() - t0;
    CHECK(el < 10.0, "30 dias offline processados em %.2fs", el);
    CHECK(b->away_events > 0 && b->away_events < 400, "eventos offline limitados (%d)",
          b->away_events);
    int hostiles = 0;
    for (int i = 0; i < b->nmsg; i++)
        if (!strncmp(b->msg[i].title, "ALERTA", 6)) hostiles++;
    CHECK(hostiles <= 40, "ataques offline limitados (%d alertas)", hostiles);
    universe_free(a);
    universe_free(b);
    free(a);
    free(b);
}

static void t_render(void) {
    printf("[renderização de todas as telas]\n");
    Game *g = calloc(1, sizeof(Game));
    game_new(g, "Render", "Casa", 25, "20262411591403");
    for (int i = 0; i < 400; i++) {
        game_advance(g, g->now + 300, 0);
        bot_step(g);
    }
    UI *u = calloc(1, sizeof(UI));
    ui_init(u, g);
    int frames = 0;
    for (int s = 0; s < SCR_COUNT; s++) {
        u->screen = s;
        for (int k = 0; k < 6; k++) {
            u->t = k * 1.7;
            ui_draw(u, g);
            /* aperta teclas */
            int keys[] = {K_DOWN, K_DOWN, K_RIGHT, K_TAB, K_UP, K_LEFT};
            int kk = keys[k];
            if (!is_global_key(kk)) ui_key(u, g, kk);
            if (u->modal != MD_NONE) ui_key(u, g, K_ESC);
            u->screen = s;
            frames++;
        }
    }
    /* troca de corpo, alternar lua, todos os corpos */
    for (int i = 0; i < g->nbody; i++)
        if (g->body[i].used)
            for (int s = 0; s < SCR_COUNT; s++) {
                g->cur = i;
                u->screen = s;
                ui_draw(u, g);
                frames++;
            }
    g->cur = 0;
    /* fluxo completo do assistente de envio */
    u->screen = SCR_FLEET;
    ui_key(u, g, K_ENTER);
    ui_key(u, g, 'T');
    ui_draw(u, g);
    ui_key(u, g, K_ENTER);
    ui_draw(u, g);
    ui_key(u, g, K_ENTER);
    for (int r = 0; r < 7; r++) {
        ui_draw(u, g);
        ui_key(u, g, K_DOWN);
    }
    ui_key(u, g, K_ESC);
    ui_key(u, g, K_ESC);
    ui_key(u, g, K_ESC);
    /* janela de evento */
    Decision *d = &g->ev.dec;
    memset(d, 0, sizeof *d);
    d->active = 1;
    d->type = EV_EARTHQUAKE;
    d->body = 0;
    d->nopt = 3;
    d->dc = 12;
    for (int i = 0; i < 3; i++)
        snprintf(d->opt[i], sizeof d->opt[i], "Opção %d", i + 1);
    d->optbonus[1] = -1000;
    d->deadline = g->now + 100;
    u->modal = MD_EVENT;
    ui_draw(u, g);
    ui_key(u, g, '1');
    for (int i = 0; i < 5; i++) {
        u->t += 0.5;
        ui_draw(u, g);
    }
    ui_key(u, g, K_ENTER);
    CHECK(!g->ev.dec.active, "modal de evento resolve a decisão");
    /* texto com strings longas / modo ASCII */
    g_term.ascii = 1;
    ui_draw(u, g);
    g_term.ascii = 0;
    CHECK(frames > 100, "telas renderizadas sem falhas (%d quadros)", frames);
    /* confere que nenhum caractere de controle vaza para o canvas */
    int ctrl = 0;
    for (int y = 0; y < CANVAS_H; y++)
        for (int x = 0; x < CANVAS_W; x++) {
            Cell *c = cv_cell(x, y);
            if (c->ch < 32 && c->ch != 0) ctrl++;
        }
    CHECK(ctrl == 0, "sem caracteres de controle no canvas (%d)", ctrl);
    universe_free(g);
    free(u);
    free(g);
}

static void t_long(void) {
    printf("[simulação longa: 5 dias reais em x10 com piloto automático]\n");
    Game *g = calloc(1, sizeof(Game));
    game_new(g, "Longo", "Casa", 10, "20262411591403");
    double p_prev = 0;
    int monotone_ok = 1;
    for (int i = 0; i < 5 * 24 * 12; i++) {
        game_advance(g, g->now + 300, 0);
        bot_step(g);
        g->ntoast = 0;
        if (i % 72 == 0) {
            double p = player_points(g, NULL, NULL, NULL, NULL);
            if (p + 1e-9 < p_prev * 0.5) monotone_ok = 0;
            p_prev = p;
        }
    }
    double pts = player_points(g, NULL, NULL, NULL, NULL);
    CHECK(pts > 1000, "progresso após 5 dias (%.0f pontos)", pts);
    CHECK(monotone_ok, "pontos não despencam");
    printf("  info: piloto chegou a %.0f pontos, %d planeta(s), %lld eventos\n", pts,
           colonies_count(g), g->ev.total);
    CHECK(g->ev.total > 30, "eventos ocorreram (%lld)", g->ev.total);
    int bad = 0;
    for (int i = 0; i < g->nbody; i++) {
        Body *b = &g->body[i];
        if (!b->used) continue;
        for (int k = 0; k < NRES; k++)
            if (!(b->res[k] >= 0) || isnan(b->res[k])) bad = 1;
        for (int k = 0; k < N_SHIP; k++)
            if (b->ship[k] < 0) bad = 1;
        for (int k = 0; k < N_DEF; k++)
            if (b->def[k] < 0) bad = 1;
        if (body_fields_used(b) > body_fields_max(g, b) + 5) bad = 1;
    }
    CHECK(!bad, "estado consistente (sem negativos/NaN, campos ok)");
    universe_free(g);
    free(g);
}

int run_selftest(int verbose) {
    VERB = verbose;
    setvbuf(stdout, NULL, _IONBF, 0);
    TermOpts o;
    memset(&o, 0, sizeof o);
    o.headless = 1;
    term_init(&o);
    t_formulas();
    t_seed();
    t_economy();
    t_queues();
    t_combat();
    t_fleet();
    t_events();
    t_trade();
    t_save();
    t_offline();
    t_render();
    t_long();
    printf("\n%d verificações OK, %d falhas\n", passes, fails);
    return fails ? 1 : 0;
}
