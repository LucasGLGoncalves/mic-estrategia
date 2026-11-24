/* game.h - core game structures and API */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "util.h"

#define GAME_TITLE "IMPÉRIO ESTELAR"
#define GAME_VERSION "1.0"
#define SAVE_FILE "imperio_estelar.sav"
#define SAVE_MAGIC "IMPERIO-ESTELAR-SAVE"
#define SAVE_VERSION 1

#define UNI_GALAXIES 9
#define UNI_SYSTEMS 499
#define MAX_BODIES 40
#define MAX_PLANETS 20
#define MAX_FLEETS 96
#define MAX_MSGS 150
#define MSG_TITLE 80
#define MSG_BODY 3000
#define MAX_ROUTES 24
#define MAX_MODS 10
#define MAX_QUEUE 5
#define MAX_SHIPQ 12
#define MAX_DEBRIS 160
#define MAX_TOUCHED 400
#define MAX_EXPLORED 600
#define N_NPC 552
#define NEWBIE_POINTS 250.0

/* stored resources */
enum { RES_METAL, RES_CRYSTAL, RES_DEUT, RES_FOOD, NRES };

/* ---------------- items ---------------- */
typedef enum {
    B_METAL_MINE, B_CRYSTAL_MINE, B_DEUT_SYNTH, B_SOLAR_PLANT, B_FUSION,
    B_METAL_STORE, B_CRYSTAL_STORE, B_DEUT_TANK,
    B_ROBOTICS, B_SHIPYARD, B_LAB, B_MISSILE_SILO, B_NANITE, B_TERRAFORMER,
    B_SPACE_DOCK, B_ORBITAL, B_TRADE_HUB,
    B_LUNAR_BASE, B_PHALANX, B_JUMP_GATE, B_HE3,
    B_HABITAT, B_FARM, B_MEDICAL, B_ACADEMY, B_FOOD_SYNTH, B_ARCOLOGY,
    N_BLD,
    T_ESPIONAGE = N_BLD, T_COMPUTER, T_WEAPONS, T_SHIELDING, T_ARMOR, T_ENERGY,
    T_HYPERSPACE, T_COMBUSTION, T_IMPULSE, T_HYPERDRIVE, T_LASER, T_ION, T_PLASMA,
    T_IRN, T_ASTRO, T_GRAVITON, T_BIOTECH, T_MEDICINE, T_GEOPHYSICS, T_ORBITAL_ENG,
    T_LOGISTICS,
    T_END,
    S_SMALL_CARGO = T_END, S_LARGE_CARGO, S_LIGHT_FIGHTER, S_HEAVY_FIGHTER, S_CRUISER,
    S_BATTLESHIP, S_BATTLECRUISER, S_BOMBER, S_DESTROYER, S_DEATHSTAR, S_REAPER,
    S_PATHFINDER, S_COLONY, S_RECYCLER, S_PROBE, S_SATELLITE, S_CRAWLER,
    S_END,
    D_ROCKET = S_END, D_LIGHT_LASER, D_HEAVY_LASER, D_GAUSS, D_ION, D_PLASMA,
    D_SMALL_DOME, D_LARGE_DOME, D_ABM, D_IPM,
    D_END,
    ITEM_COUNT = D_END
} ItemId;

#define N_TECH (T_END - N_BLD)
#define N_SHIP (S_END - T_END)
#define N_DEF (D_END - S_END)
#define TI(id) ((id) - N_BLD)
#define SI(id) ((id) - T_END)
#define DI(id) ((id) - S_END)
#define IS_BLD(id) ((id) >= 0 && (id) < N_BLD)
#define IS_TECH(id) ((id) >= N_BLD && (id) < T_END)
#define IS_SHIP(id) ((id) >= T_END && (id) < S_END)
#define IS_DEF(id) ((id) >= S_END && (id) < D_END)
/* combat units: ships then the 8 non-missile defenses */
#define N_CDEF 8
#define N_UNIT (N_SHIP + N_CDEF)

enum { CAT_RESOURCE, CAT_FACILITY, CAT_MOON, CAT_LIFE, CAT_RESEARCH, CAT_SHIP, CAT_DEFENSE };
enum { W_PLANET = 1, W_MOON = 2, W_BOTH = 3 };
enum { DRV_NONE, DRV_COMB, DRV_IMP, DRV_HYP };

typedef struct { int id, lvl; } Req;

typedef struct {
    const char *name, *shortn, *desc;
    int cat, where;
    double cost[3];
    double energy;     /* energy required (graviton/terraformer) */
    double factor;
    Req req[6];
    int maxlvl;
    double att, shd, si, cargo, speed, fuel;
    int drive;
} ItemDef;

extern const ItemDef ITEMS[ITEM_COUNT];
int rapidfire(int shooter_unit, int target_unit);
int unit_item(int u);
int item_unit(int id);

/* ---------------- planet info ---------------- */
enum { BIO_TEMPERATE, BIO_OCEANIC, BIO_JUNGLE, BIO_DESERT, BIO_SAVANNA, BIO_TUNDRA,
       BIO_ICE, BIO_VOLCANIC, BIO_TOXIC, BIO_BARREN, BIO_MOON, BIO_COUNT };
enum { ATM_N2O2, ATM_CO2_DENSE, ATM_THIN, ATM_METHANE, ATM_AMMONIA, ATM_SULFUR, ATM_NONE, ATM_COLD_N2, ATM_COUNT };

typedef struct {
    int tmin, tmax, diameter, fields;
    int biome, atmo;
    double pressure, gravity, daylen, water, fertility;
    int radiation, magneto;
    double rich[3];
    int has_moon, moon_diam;
    uint64_t vseed;
    int ring;
    int climate;   /* index into climate phrase table of biome */
} PlanetInfo;

extern const char *BIOME_NAME[BIO_COUNT];
extern const char *ATMO_NAME[ATM_COUNT];
extern const char *RAD_NAME[4];
extern const char *MAG_NAME[3];
const char *climate_text(const PlanetInfo *pi);

/* ---------------- body ---------------- */
typedef struct { int item; long long amount; double start, end; int active; } QItem;
typedef struct { QItem it[MAX_QUEUE]; int n; } Queue;
typedef struct { int item; long long count; double unit; double next; } SQItem;

enum { MOD_METAL, MOD_CRYSTAL, MOD_DEUT, MOD_ENERGY, MOD_FOOD, MOD_GROWTH, MOD_FUSION_OFF,
       MOD_SAT_OFF, MOD_RESEARCH, MOD_TRADE, MOD_COUNT };
typedef struct { int kind; double factor; double until; } Mod;

typedef struct {
    int used, is_moon, parent, moon;
    char name[32];
    int g, s, p;
    PlanetInfo info;
    double res[NRES];
    double pop;
    int bld[N_BLD];
    int pct[N_BLD];
    int sat_pct, crawler_pct;
    long long ship[N_SHIP];
    long long def[N_DEF];
    Queue bq, lq;
    SQItem sq[MAX_SHIPQ];
    int nsq;
    Mod mod[MAX_MODS];
    double founded, last_attacked;
    double gate_ready;
} Body;

/* ---------------- fleets ---------------- */
enum { M_NONE, M_ATTACK, M_TRANSPORT, M_DEPLOY, M_ESPIONAGE, M_COLONIZE, M_RECYCLE,
       M_EXPEDITION, M_EXPLORE, M_HOSTILE, M_HOSTILE_SPY, M_COUNT };
enum { FS_OUT, FS_HOLD, FS_RETURN };
enum { TT_PLANET, TT_MOON, TT_DEBRIS };
enum { HK_PIRATE, HK_XENO, HK_NPC };

typedef struct {
    int used, mission, state;
    int origin;                 /* player body (for hostile: target body) */
    int og, os, op, otype;
    int tg, ts, tp, ttype;
    long long ship[N_SHIP];
    double cargo[NRES];
    double depart, arrive, hold, ret;
    int speed;
    int npc, hkind;
    int wt, st, at;             /* hostile techs */
} Fleet;

/* ---------------- trade routes ---------------- */
enum { RT_INTERNAL, RT_NPC };
enum { RS_IDLE, RS_OUT, RS_BACK };
typedef struct {
    int used, kind;
    int origin, dest;
    int tg, ts, tp, npc;
    int res_out, res_in;
    double amount;
    long long ship[N_SHIP];
    int state;
    double t_next, t_arrive, t_return;
    double load[NRES], back[NRES];
    long long trips, ambushes, lost_ships;
    double delivered;
} Route;

/* ---------------- messages ---------------- */
enum { MSG_INFO, MSG_COMBAT, MSG_SPY, MSG_EVENT, MSG_FLEET, MSG_TRADE, MSG_EXPED, MSG_QUEST, MSG_COUNT };
typedef struct { double t; int type, read; char title[MSG_TITLE]; char body[MSG_BODY]; } Message;

/* ---------------- universe ---------------- */
enum { NPC_INACTIVE, NPC_MINER, NPC_TURTLE, NPC_FLEETER, NPC_TRADER, NPC_PIRATE, NPC_XENO, NPC_TYPES };
typedef struct {
    char name[24], tag[8];
    int type;
    double p0, rate;      /* points at start, points per game-day */
    int nplanets;
} Npc;
typedef struct { int g, s, p, npc, idx, has_moon; char pname[24]; } NpcPlanet;
typedef struct { int g, s, p; double t_raid, def_dmg, fleet_dmg; double t_dmg; } Touched;
typedef struct { int g, s, p; double m, c; } Debris;

typedef struct {
    double res[3];
    long long ship[N_SHIP];
    long long def[N_DEF];
    int bld[N_BLD];
    int tech[N_TECH];
    double points;
    int npc;
} NpcSnap;

/* ---------------- events ---------------- */
enum { EV_NONE, EV_PROBES, EV_ATTACK, EV_XENO, EV_MISSILES, EV_EARTHQUAKE, EV_METEOR,
       EV_FLARE, EV_PLAGUE, EV_STORM, EV_BLIGHT, EV_REACTOR, EV_MIGRANTS, EV_VEIN,
       EV_COMET, EV_DERELICT, EV_MERCHANT, EV_ANOMALY, EV_SCIENCE, EV_UNREST, EV_COUNT };

typedef struct {
    int active, type, body;
    char title[64];
    char text[600];
    int nopt;
    char opt[3][72];
    double optcost[3][NRES];
    double optdm[3];
    int optbonus[3];      /* added to roll; -1000 = no roll (auto outcome) */
    int dc, base_mod;
    char mod_text[96];
    double deadline;
    int defopt;
    /* resolution */
    int chosen, roll, total, outcome, rerolls;   /* outcome: 0 fail 1 success 2 crit 3 critfail */
    char result[600];
    int stage;            /* 0 choosing, 1 rolled/showing */
    int tier;
} Decision;

typedef struct {
    double next_event;
    double last_hostile;
    int offline, offline_hostiles;
    long long total;
    Decision dec;
    double merchant_until;
} EventState;

/* ---------------- officers ---------------- */
enum { OFF_COMMANDER, OFF_ADMIRAL, OFF_ENGINEER, OFF_GEOLOGIST, OFF_TECHNOCRAT, N_OFF };

/* ---------------- game ---------------- */
typedef struct {
    char name[32];
    char seedstr[48];
    uint64_t seed;
    int speed;
    int tech[N_TECH];
    Queue rq;
    int rq_body;
    double dm, honour;
    double officer[N_OFF];
    double created, now, last_save, last_seen;
    int cur;
    Body body[MAX_BODIES];
    int nbody;
    Fleet fleet[MAX_FLEETS];
    Route route[MAX_ROUTES];
    Message msg[MAX_MSGS];
    int nmsg;
    Debris debris[MAX_DEBRIS];
    int ndebris;
    Touched touched[MAX_TOUCHED];
    int ntouched;
    int explored[MAX_EXPLORED];
    int nexplored;
    Npc npc[N_NPC];
    NpcPlanet *npl;
    int nnpl;
    EventState ev;
    Rng rng;
    int quest;
    unsigned flags;
    double stat_produced[NRES];
    double stat_combats, stat_wins;
    /* notifications for UI (toasts) */
    char toast[8][120];
    uint32_t toast_col[8];
    int ntoast;
    /* offline report */
    double away_secs;
    double away_gain[NRES];
    int away_events;
} Game;

/* flags */
#define FL_SPIED     0x0001u
#define FL_COLONIZED 0x0002u
#define FL_EXPED     0x0004u
#define FL_ROUTE     0x0008u
#define FL_ATTACKED  0x0010u
#define FL_DEFENDED  0x0020u
#define FL_RECYCLED  0x0040u
#define FL_TRADED    0x0080u
#define FL_EXPLORED  0x0100u

/* ---------------- rates ---------------- */
typedef struct {
    double prod[NRES];       /* net per hour (speed applied) */
    double gross[NRES];
    double base[NRES];
    double e_prod, e_cons, e_eff;
    double fusion_deut, synth_deut;
    double w_req, w_farm, wf, farm_wf;
    double pop_cap, growth;  /* per hour */
    double food_prod, food_cons;
    double cap[NRES];
    double fac_eff;
    double sat_each;
    double mine[3];          /* mine-only production per hour */
} Rates;

/* ---------------- API ---------------- */
/* economy.c */
double item_cost1(int id, int level, double out[3]);
void   item_cost(const Game *g, int id, long long level_or_count, double out[3]);
double item_energy_req(int id, int level);
double build_time(const Game *g, const Body *b, int id, int level);
double research_time(const Game *g, int body, int id, int level);
double unit_time(const Game *g, const Body *b, int id);
void   body_rates(const Game *g, const Body *b, Rates *r);
int    exceeds_storage(const Game *g, const Body *b, const double c[3]);
void   compose_fleet(double value, double pts, long long *ship);
void   compose_defense(double value, double pts, long long *def);
double storage_cap(int lvl);
int    body_fields_used(const Body *b);
int    body_fields_max(const Game *g, const Body *b);
int    tech(const Game *g, int id);
int    reqs_met(const Game *g, const Body *b, int id);
int    req_have(const Game *g, const Body *b, int id);
int    officer_active(const Game *g, int off);
double player_points(const Game *g, double *pb, double *pr, double *pf, double *pd);
double body_military_value(const Body *b);
double items_value(const long long *ships, const long long *defs);
long long max_affordable(const Game *g, const Body *b, int id);
int    queue_limit(const Game *g);
double prod_metal_lvl(int lvl), prod_crystal_lvl(int lvl), prod_deut_lvl(int lvl, double tavg);
double workers_for(int id, int lvl);
double energy_for(int id, int lvl);
int    missile_capacity(const Body *b);
int    missile_used(const Body *b);

enum { BC_OK, BC_REQ, BC_RES, BC_FIELDS, BC_QUEUE, BC_MAX, BC_BUSY, BC_PLACE, BC_ENERGY, BC_SILO, BC_STORAGE, BC_LABBUSY };
int    can_build(const Game *g, int body, int id, long long count);
const char *bc_text(int code);
int    enqueue(Game *g, int body, int id, long long count);
int    cancel_queue(Game *g, int body, int which, int idx);
double dm_finish_cost(const Game *g, double remaining);
int    finish_with_dm(Game *g, int body, int which);

/* sim.c */
void game_advance(Game *g, double target, int offline);
void game_toast(Game *g, uint32_t col, const char *fmt, ...);
void game_msg(Game *g, int type, const char *title, const char *fmt, ...);
int  game_unread(const Game *g);
void body_add_mod(Body *b, int kind, double factor, double until);
double body_mod(const Body *b, int kind, double now);

/* universe.c */
void planet_generate(uint64_t seed, int g, int s, int p, int homeworld, PlanetInfo *pi);
void universe_build(Game *g);
void universe_free(Game *g);
int  npc_planet_at(const Game *g, int gg, int ss, int pp);
double npc_points(const Game *g, int npc);
void npc_snapshot(Game *g, int npl_index, NpcSnap *s);
Touched *touched_get(Game *g, int gg, int ss, int pp, int create);
int  player_body_at(const Game *g, int gg, int ss, int pp, int moon);
int  is_explored(const Game *g, int gg, int ss, int pp);
void set_explored(Game *g, int gg, int ss, int pp);
Debris *debris_at(Game *g, int gg, int ss, int pp, int create);
int  player_rank(const Game *g, int *total);
int  npc_rank_list(const Game *g, int *order, double *pts);
void npc_name_gen(Rng *r, char *out, size_t n);
const char *npc_type_name(int t);
int  max_colonies(const Game *g);
int  colonies_count(const Game *g);
int  position_allowed(const Game *g, int p);

/* game.c */
void game_new(Game *g, const char *name, const char *home, int speed, const char *seedstr);
void make_seedstr(char *out, size_t n, double t);
int  body_new(Game *g, const char *name, int gg, int ss, int pp, int is_moon, const PlanetInfo *pi);
void create_moon(Game *g, int planet, int diam);
const char *res_name(int r);
const char *quest_title(const Game *g, int q, char *buf, size_t n);
int  quest_progress(const Game *g, int q, double *have, double *need);
void quest_reward_text(int q, char *buf, size_t n);
int  quest_count(void);
void quest_check(Game *g);
void game_rename(Game *g, int body, const char *name);
int  game_abandon(Game *g, int body);

/* fleet.c */
double fleet_distance(int g1, int s1, int p1, int g2, int s2, int p2);
double ship_speed(const Game *g, int ship);
double fleet_slowest(const Game *g, const long long *ships);
double flight_duration(const Game *g, double dist, double slowest, int speed_pct);
double fleet_fuel(const Game *g, const long long *ships, double dist, int speed_pct, const Body *origin);
double fleet_cargo_cap(const Game *g, const long long *ships);
int    fleet_slots_used(const Game *g);
int    fleet_slots_max(const Game *g);
int    exped_slots_used(const Game *g);
int    exped_slots_max(const Game *g);
int    mission_valid(const Game *g, int body, int mission, int tg, int ts, int tp, int ttype, const long long *ships, char *why, size_t n);
int    fleet_launch(Game *g, int body, int mission, int tg, int ts, int tp, int ttype, const long long *ships, const double *cargo, int speed_pct, char *err, size_t n);
int    fleet_recall(Game *g, int fi);
void   fleet_process(Game *g);
double fleet_next_time(const Game *g);
const char *mission_name(int m);
double fleet_ship_count(const long long *ships);
int    intergalactic_ok(const Game *g);
int    jump_gate(Game *g, int from, int to, char *err, size_t n);
int    ipm_range(const Game *g);
int    ipm_launch(Game *g, int body, int npl_index, long long n, char *err, size_t en);

/* combat.c */
typedef struct {
    long long n[2][N_UNIT];
    int wt[2], st[2], at[2];
} CombatIn;
typedef struct {
    long long start[2][N_UNIT], end[2][N_UNIT], repaired[N_UNIT];
    int rounds, winner;   /* 0 attacker, 1 defender, -1 draw */
    double debris[2];
    double lost_value[2];
    double moon_chance;
} CombatOut;
void combat_run(Rng *r, const CombatIn *in, CombatOut *out);
void combat_report(const CombatIn *in, const CombatOut *out, const char *aname, const char *dname, char *buf, size_t n);
void loot_take(const double avail[3], double capacity, double out[3]);

/* events.c */
void events_schedule(Game *g, int first);
void events_process(Game *g);
double events_next_time(const Game *g);
void decision_choose(Game *g, int opt);
void decision_reroll(Game *g);
void decision_close(Game *g);
void decision_autoresolve(Game *g);
int  threat_level(const Game *g, int body);
double newbie_left(const Game *g);
void hostile_arrive(Game *g, int fi);
void route_ambush(Game *g, int ri, double *keep_frac);
const char *event_name(int t);

/* trade.c */
double route_capacity(const Game *g, const Route *r);
double route_trip_time(const Game *g, const Route *r);
double route_risk(const Game *g, const Route *r);
void   trade_process(Game *g);
double trade_next_time(const Game *g);
int    route_create(Game *g, const Route *tmpl, char *err, size_t n);
void   route_delete(Game *g, int ri);
int    routes_from(const Game *g, int body);
double market_rate(const Game *g, int npc, int res_from, int res_to);
double merchant_rate(int from, int to);
int    merchant_trade(Game *g, int body, int from, int to, double amount, char *err, size_t n);

/* save.c */
int game_save(Game *g, const char *path);
int game_load(Game *g, const char *path);
int save_exists(const char *path, char *name, size_t n);

/* ai.c (bot used for simulations/tests) */
void bot_step(Game *g);

#endif
