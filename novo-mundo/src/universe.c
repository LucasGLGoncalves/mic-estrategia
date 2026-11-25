/* universe.c - planetas procedurais, impérios npc, consultas à galáxia */
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *BIOME_NAME[BIO_COUNT] = {
    "Temperado", "Oceânico", "Floresta tropical", "Desértico", "Savana",
    "Tundra",    "Glacial",  "Vulcânico",         "Tóxico",    "Rochoso estéril",
    "Lunar"};
const char *ATMO_NAME[ATM_COUNT] = {"Nitrogênio-Oxigênio (respirável)",
                                    "Dióxido de carbono densa",
                                    "Rarefeita CO2/N2",
                                    "Metano-Nitrogênio",
                                    "Amônia-Metano (tóxica)",
                                    "Sulfúrica (SO2/CO2)",
                                    "Nenhuma (vácuo)",
                                    "Nitrogênio frio"};
const char *RAD_NAME[4] = {"Baixa", "Moderada", "Alta", "Extrema"};
const char *MAG_NAME[3] = {"Fraca", "Moderada", "Forte"};

static const char *CLIMATES[BIO_COUNT][3] = {
    {"Estações bem definidas e chuvas regulares", "Brisas oceânicas e verões amenos",
     "Clima ameno com frentes frias"},
    {"Monções intensas e furacões sazonais", "Neblina constante sobre mares sem fim",
     "Marés colossais e tempestades"},
    {"Calor úmido e chuvas torrenciais", "Selva densa sob névoa permanente",
     "Tempestades elétricas sobre a mata"},
    {"Tempestades de areia frequentes", "Dias escaldantes e noites gélidas",
     "Ventos secos e céu sem nuvens"},
    {"Estação seca longa e chuvas curtas", "Planícies quentes com ventos constantes",
     "Incêndios de pradaria na estiagem"},
    {"Verões curtos e permafrost profundo", "Ventos gélidos e neve rasa",
     "Auroras intensas sobre planícies frias"},
    {"Nevascas perpétuas e gelo milenar", "Gêiseres criovulcânicos sob o gelo",
     "Frio extremo e céu cristalino"},
    {"Erupções constantes e chuva de cinzas", "Rios de lava e tremores frequentes",
     "Ar sulfúrico e calor infernal"},
    {"Chuva ácida e névoa corrosiva", "Pântanos venenosos sob céu amarelo",
     "Ventos carregados de esporos"},
    {"Vácuo, crateras e silêncio", "Ventos rarefeitos levantam poeira fina",
     "Variação térmica brutal dia/noite"},
    {"Superfície lunar sem atmosfera", "Regolito cinzento e crateras antigas",
     "Noites lunares de duas semanas"},
};

const char *climate_text(const PlanetInfo *pi) {
    int b = clampi(pi->biome, 0, BIO_COUNT - 1);
    return CLIMATES[b][clampi(pi->climate, 0, 2)];
}

static const int TMAX_BASE[16] = {0,  240, 190, 140, 90, 80,  70,  60,
                                  50, 40,  30,  20,  10, -30, -70, -110};
static const int FIELDS_AVG[16] = {0,   64,  68,  73, 173, 167, 155, 144,
                                   150, 159, 101, 98, 105, 110, 84,  101};

static int pick(Rng *r, const int *w, int n) {
    int tot = 0;
    for (int i = 0; i < n; i++) tot += w[i];
    int x = rng_range(r, 0, tot - 1);
    for (int i = 0; i < n; i++) {
        if (x < w[i]) return i;
        x -= w[i];
    }
    return n - 1;
}

void planet_generate(uint64_t seed, int g, int s, int p, int home, PlanetInfo *pi) {
    Rng r;
    uint64_t h = hash_combine(
        hash_combine(hash_combine(seed, (uint64_t)g), (uint64_t)s), (uint64_t)p);
    rng_seed(&r, h ^ (home ? 0x0F0F1234ULL : 0));
    memset(pi, 0, sizeof *pi);
    p = clampi(p, 1, 15);
    pi->tmax = TMAX_BASE[p] + rng_range(&r, -20, 20);
    pi->tmin = pi->tmax - 40 + rng_range(&r, -6, 6);
    int fields;
    if (home) fields = rng_range(&r, 150, 240);
    else {
        fields = (int)(FIELDS_AVG[p] * rng_uniform(&r, 0.7, 1.3));
        if (fields < 40) fields = 40;
    }
    double diam = sqrt((double)fields) * 1000.0 +
                  rng_range(&r, home ? -350 : 0, home ? 350 : 900);
    pi->diameter = (int)diam / 100 * 100;
    pi->fields = (int)floor((pi->diameter / 1000.0) * (pi->diameter / 1000.0));
    if (pi->fields < 30) pi->fields = 30;
    double tavg = (pi->tmin + pi->tmax) / 2.0;
    /* bioma */
    int b;
    static const int B0[] = {BIO_VOLCANIC, BIO_DESERT, BIO_TOXIC}, W0[] = {70, 20, 10};
    static const int B1[] = {BIO_DESERT, BIO_SAVANNA, BIO_TOXIC, BIO_VOLCANIC},
                     W1[] = {50, 20, 15, 15};
    static const int B2[] = {BIO_SAVANNA, BIO_JUNGLE, BIO_TEMPERATE, BIO_DESERT,
                             BIO_OCEANIC},
                     W2[] = {25, 25, 30, 10, 10};
    static const int B3[] = {BIO_TEMPERATE, BIO_OCEANIC, BIO_JUNGLE, BIO_TUNDRA,
                             BIO_BARREN},
                     W3[] = {40, 25, 10, 15, 10};
    static const int B4[] = {BIO_TUNDRA, BIO_ICE, BIO_OCEANIC, BIO_BARREN},
                     W4[] = {45, 35, 10, 10};
    static const int B5[] = {BIO_ICE, BIO_BARREN, BIO_TUNDRA}, W5[] = {70, 20, 10};
    if (tavg > 120) b = B0[pick(&r, W0, 3)];
    else if (tavg > 60) b = B1[pick(&r, W1, 4)];
    else if (tavg > 20) b = B2[pick(&r, W2, 5)];
    else if (tavg > -10) b = B3[pick(&r, W3, 5)];
    else if (tavg > -60) b = B4[pick(&r, W4, 4)];
    else b = B5[pick(&r, W5, 3)];
    if (home && (b == BIO_BARREN || b == BIO_VOLCANIC || b == BIO_TOXIC))
        b = tavg > 40 ? BIO_SAVANNA : (tavg > -20 ? BIO_TEMPERATE : BIO_TUNDRA);
    pi->biome = b;
    /* atmosfera */
    int a;
    switch (b) {
    case BIO_TEMPERATE:
    case BIO_OCEANIC:
    case BIO_JUNGLE: a = rng_chance(&r, 0.85) ? ATM_N2O2 : ATM_CO2_DENSE; break;
    case BIO_SAVANNA: a = rng_chance(&r, 0.7) ? ATM_N2O2 : ATM_THIN; break;
    case BIO_DESERT:
        a = rng_chance(&r, 0.5) ? ATM_THIN
                                : (rng_chance(&r, 0.5) ? ATM_CO2_DENSE : ATM_N2O2);
        break;
    case BIO_TUNDRA: a = rng_chance(&r, 0.6) ? ATM_COLD_N2 : ATM_N2O2; break;
    case BIO_ICE:
        a = rng_chance(&r, 0.5) ? ATM_COLD_N2
                                : (rng_chance(&r, 0.5) ? ATM_METHANE : ATM_THIN);
        break;
    case BIO_VOLCANIC: a = rng_chance(&r, 0.7) ? ATM_SULFUR : ATM_CO2_DENSE; break;
    case BIO_TOXIC: a = rng_chance(&r, 0.5) ? ATM_AMMONIA : ATM_METHANE; break;
    default: a = rng_chance(&r, 0.7) ? ATM_NONE : ATM_THIN; break;
    }
    pi->atmo = a;
    switch (a) {
    case ATM_N2O2: pi->pressure = rng_uniform(&r, 0.6, 1.8); break;
    case ATM_CO2_DENSE: pi->pressure = rng_uniform(&r, 3, 90); break;
    case ATM_THIN: pi->pressure = rng_uniform(&r, 0.01, 0.3); break;
    case ATM_METHANE: pi->pressure = rng_uniform(&r, 0.8, 3); break;
    case ATM_AMMONIA: pi->pressure = rng_uniform(&r, 1.5, 8); break;
    case ATM_SULFUR: pi->pressure = rng_uniform(&r, 2, 40); break;
    case ATM_NONE: pi->pressure = 0; break;
    default: pi->pressure = rng_uniform(&r, 0.3, 1.2); break;
    }
    pi->gravity = (pi->diameter / 12800.0) * rng_uniform(&r, 0.75, 1.25);
    pi->daylen = rng_uniform(&r, 8, 72);
    switch (b) {
    case BIO_OCEANIC: pi->water = rng_uniform(&r, 0.80, 0.95); break;
    case BIO_TEMPERATE: pi->water = rng_uniform(&r, 0.45, 0.72); break;
    case BIO_JUNGLE: pi->water = rng_uniform(&r, 0.30, 0.55); break;
    case BIO_SAVANNA: pi->water = rng_uniform(&r, 0.15, 0.35); break;
    case BIO_TUNDRA: pi->water = rng_uniform(&r, 0.20, 0.45); break;
    case BIO_DESERT: pi->water = rng_uniform(&r, 0.0, 0.08); break;
    case BIO_ICE: pi->water = rng_uniform(&r, 0.0, 0.1); break;
    case BIO_VOLCANIC: pi->water = 0; break;
    case BIO_TOXIC: pi->water = rng_uniform(&r, 0.25, 0.6); break;
    default: pi->water = 0; break;
    }
    static const double FERT[BIO_COUNT] = {1.0, 0.9,  1.3, 0.35, 0.8, 0.5,
                                           0.2, 0.25, 0.3, 0.1,  0.0};
    pi->fertility =
        FERT[b] * rng_uniform(&r, 0.85, 1.15) * (a == ATM_N2O2 ? 1.0 : 0.75);
    if (home && pi->fertility < 0.6) pi->fertility = 0.6 + rng_uniform(&r, 0, 0.1);
    pi->magneto = rng_chance(&r, 0.25 + 0.3 * (pi->diameter > 12000))
                      ? 2
                      : (rng_chance(&r, 0.6) ? 1 : 0);
    int rad = (p <= 3   ? 2
               : p <= 6 ? 1
                        : 0) +
              (pi->magneto == 0   ? 1
               : pi->magneto == 2 ? -1
                                  : 0) +
              (a == ATM_NONE || a == ATM_THIN ? 1 : 0);
    pi->radiation = clampi(rad, 0, 3);
    for (int i = 0; i < 3; i++) pi->rich[i] = rng_uniform(&r, 0.8, 1.25);
    if (b == BIO_VOLCANIC) pi->rich[0] *= 1.15;
    if (b == BIO_TOXIC || b == BIO_DESERT) pi->rich[1] *= 1.12;
    if (b == BIO_ICE || b == BIO_OCEANIC) pi->rich[2] *= 1.15;
    if (b == BIO_BARREN) {
        pi->rich[0] *= 1.1;
        pi->rich[1] *= 1.1;
    }
    pi->has_moon = rng_chance(&r, home ? 0.35 : 0.18);
    pi->moon_diam = rng_range(&r, 2000, 8900);
    pi->vseed = rng_next(&r);
    pi->ring = rng_chance(&r, 0.07);
    pi->climate = rng_range(&r, 0, 2);
}

/* ---------------- nomes ---------------- */
static const char *SYL1[] = {"Kor", "Vel", "Ix",  "Thal", "Zar", "Mor", "Quen", "Dra",
                             "Syl", "Bru", "Or",  "Tav",  "Lum", "Nex", "Ar",   "Cael",
                             "Ves", "Gor", "Hel", "Jun",  "Kai", "Rha", "Sor",  "Ul",
                             "Xen", "Yor", "Fen", "Pax",  "Ner", "Ob",  "Ka",   "Te",
                             "Mi",  "Ra",  "Lu",  "Za",   "Vi",  "No"};
static const char *SYL2[] = {"tha", "mi", "ra", "lo", "ven", "dor", "ix",  "ul",
                             "ae",  "on", "ar", "is", "ek",  "um",  "ith", "os",
                             "an",  "ir", "el", "ka", "ro",  "zi",  "na",  "te"};
static const char *SYL3[] = {"x",  "r",  "n", "s", "th", "ra", "on", "ius", "ax",
                             "or", "is", "",  "",  "",   "ek", "um", "ia",  "os"};
static const char *TITLES[] = {"",
                               "",
                               "",
                               "Lorde ",
                               "Almirante ",
                               "Capitão ",
                               "Comandante ",
                               "Imperador ",
                               "Dama ",
                               "Arconte ",
                               "General "};

void npc_name_gen(Rng *r, char *out, size_t n) {
    char base[32];
    snprintf(base, sizeof base, "%s%s%s",
             SYL1[rng_range(r, 0, (int)(sizeof SYL1 / sizeof *SYL1) - 1)],
             SYL2[rng_range(r, 0, (int)(sizeof SYL2 / sizeof *SYL2) - 1)],
             SYL3[rng_range(r, 0, (int)(sizeof SYL3 / sizeof *SYL3) - 1)]);
    const char *t = TITLES[rng_range(r, 0, (int)(sizeof TITLES / sizeof *TITLES) - 1)];
    snprintf(out, n, "%s%s", t, base);
}

static const char *PNAMES[] = {
    "Colônia",   "Nova Esperança", "Bastião",  "Refúgio", "Forja",
    "Aurora",    "Posto Avançado", "Cidadela", "Éden",    "Vigia",
    "Abismo",    "Horizonte",      "Trono",    "Ninho",   "Fortaleza",
    "Santuário", "Oásis",          "Farol",    "Âncora",  "Domínio"};

const char *npc_type_name(int t) {
    switch (t) {
    case NPC_INACTIVE: return "Inativo";
    case NPC_MINER: return "Minerador";
    case NPC_TURTLE: return "Defensivo";
    case NPC_FLEETER: return "Agressivo";
    case NPC_TRADER: return "Mercante";
    case NPC_PIRATE: return "Pirata";
    case NPC_XENO: return "Xeno";
    }
    return "?";
}

static int npl_cmp(const void *a, const void *b) {
    const NpcPlanet *x = (const NpcPlanet *)a, *y = (const NpcPlanet *)b;
    long kx = x->g * 100000L + x->s * 100L + x->p,
         ky = y->g * 100000L + y->s * 100L + y->p;
    return kx < ky ? -1 : kx > ky;
}

void universe_free(Game *g) {
    free(g->npl);
    g->npl = NULL;
    g->nnpl = 0;
}

void universe_build(Game *g) {
    Rng r;
    rng_seed(&r, g->seed ^ 0x5EED1234ABCDULL);
    universe_free(g);
    int total = 0;
    int hg = g->nbody > 0 ? g->body[0].g : 1, hs = g->nbody > 0 ? g->body[0].s : 250;
    for (int i = 0; i < N_NPC; i++) {
        Npc *n = &g->npc[i];
        memset(n, 0, sizeof *n);
        int w[] = {30, 28, 14, 16, 6, 4, 2};
        n->type = pick(&r, w, NPC_TYPES);
        npc_name_gen(&r, n->name, sizeof n->name);
        if (n->type == NPC_PIRATE) snprintf(n->tag, sizeof n->tag, "PIRAT");
        else if (n->type == NPC_XENO) snprintf(n->tag, sizeof n->tag, "XENO");
        else if (rng_chance(&r, 0.4)) {
            int L = rng_range(&r, 3, 4);
            for (int k = 0; k < L; k++) n->tag[k] = (char)('A' + rng_range(&r, 0, 25));
            n->tag[L] = 0;
        }
        double p0;
        switch (n->type) {
        case NPC_INACTIVE:
            p0 = exp(rng_normal(&r) * 1.3 + 5.0);
            n->rate = 0;
            break;
        case NPC_XENO:
            p0 = exp(rng_normal(&r) * 0.8 + 10.5);
            n->rate = p0 * 0.03;
            break;
        case NPC_PIRATE:
            p0 = exp(rng_normal(&r) * 1.2 + 7.5);
            n->rate = p0 * 0.06 + 40;
            break;
        default:
            p0 = exp(rng_normal(&r) * 1.6 + 6.3);
            n->rate = p0 * rng_uniform(&r, 0.03, 0.10) + rng_uniform(&r, 10, 120);
            break;
        }
        if (rng_chance(&r, 0.12) && n->type != NPC_XENO)
            p0 = rng_uniform(&r, 0, 30); /* jogadores novos */
        n->p0 = p0;
        n->nplanets = 1 + (int)(log10(p0 + 1.0) * rng_uniform(&r, 0.8, 1.6));
        if (n->nplanets > 9) n->nplanets = 9;
        total += n->nplanets;
    }
    g->npl = (NpcPlanet *)calloc((size_t)total, sizeof(NpcPlanet));
    g->nnpl = 0;
    /* hash simples de ocupação para evitar colisões */
    int hcap = 16384;
    int *occ = (int *)malloc(sizeof(int) * (size_t)hcap);
    for (int i = 0; i < hcap; i++) occ[i] = -1;
#define KEY(G, S, P) ((G) * 100000 + (S) * 100 + (P))
    /* reserva os corpos do jogador */
    int pk[MAX_BODIES], npk = 0;
    /* só o planeta natal é reservado: colônias sempre são fundadas em posições que
       estavam vazias na criação, então regerar o universo depois de carregar produz
       exatamente o mesmo mapa de npcs */
    if (g->nbody > 0) pk[npk++] = KEY(g->body[0].g, g->body[0].s, g->body[0].p);
    for (int i = 0; i < N_NPC; i++) {
        Npc *n = &g->npc[i];
        int hgal, hsys;
        if (rng_chance(&r, 0.45)) {
            hgal = hg;
            hsys = clampi(hs + rng_range(&r, -60, 60), 1, UNI_SYSTEMS);
        } else {
            hgal = rng_range(&r, 1, UNI_GALAXIES);
            hsys = rng_range(&r, 1, UNI_SYSTEMS);
        }
        for (int k = 0; k < n->nplanets; k++) {
            int G = hgal, S = hsys, P;
            int tries = 0;
            for (;;) {
                if (k > 0) S = clampi(hsys + rng_range(&r, -35, 35), 1, UNI_SYSTEMS);
                P = rng_range(&r, k == 0 ? 4 : 1, k == 0 ? 12 : 15);
                int key = KEY(G, S, P);
                int clash = 0;
                for (int q = 0; q < npk; q++)
                    if (pk[q] == key) clash = 1;
                unsigned hh = (unsigned)hash64((uint64_t)key) & (unsigned)(hcap - 1);
                while (occ[hh] != -1) {
                    if (occ[hh] == key) {
                        clash = 1;
                        break;
                    }
                    hh = (hh + 1) & (unsigned)(hcap - 1);
                }
                if (!clash) {
                    occ[hh] = key;
                    break;
                }
                if (++tries > 30) { S = rng_range(&r, 1, UNI_SYSTEMS); }
                if (tries > 200) break;
            }
            NpcPlanet *pl = &g->npl[g->nnpl++];
            pl->g = G;
            pl->s = S;
            pl->p = P;
            pl->npc = i;
            pl->idx = k;
            pl->has_moon =
                rng_chance(&r, 0.1 + 0.1 * (k == 0) + (n->p0 > 50000 ? 0.3 : 0));
            if (k == 0)
                snprintf(pl->pname, sizeof pl->pname, "%s",
                         n->type == NPC_PIRATE ? "Covil Pirata"
                         : n->type == NPC_XENO ? "Colmeia Xeno"
                                               : "Planeta Natal");
            else
                snprintf(pl->pname, sizeof pl->pname, "%s",
                         PNAMES[rng_range(&r, 0,
                                          (int)(sizeof PNAMES / sizeof *PNAMES) - 1)]);
        }
    }
    free(occ);
#undef KEY
    qsort(g->npl, (size_t)g->nnpl, sizeof(NpcPlanet), npl_cmp);
}

int npc_planet_at(const Game *g, int gg, int ss, int pp) {
    int lo = 0, hi = g->nnpl - 1;
    long key = gg * 100000L + ss * 100L + pp;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const NpcPlanet *x = &g->npl[mid];
        long k = x->g * 100000L + x->s * 100L + x->p;
        if (k == key) return mid;
        if (k < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

double npc_points(const Game *g, int npc) {
    const Npc *n = &g->npc[npc];
    double days = (g->now - g->created) * g->speed / 86400.0;
    if (days < 0) days = 0;
    return n->p0 + n->rate * days;
}

Touched *touched_get(Game *g, int gg, int ss, int pp, int create) {
    for (int i = 0; i < g->ntouched; i++)
        if (g->touched[i].g == gg && g->touched[i].s == ss && g->touched[i].p == pp)
            return &g->touched[i];
    if (!create) return NULL;
    int slot = g->ntouched;
    if (slot >= MAX_TOUCHED) {
        double old = 1e300;
        slot = 0;
        for (int i = 0; i < MAX_TOUCHED; i++)
            if (g->touched[i].t_raid < old) {
                old = g->touched[i].t_raid;
                slot = i;
            }
    } else g->ntouched++;
    Touched *t = &g->touched[slot];
    memset(t, 0, sizeof *t);
    t->g = gg;
    t->s = ss;
    t->p = pp;
    t->t_raid = g->created;
    t->t_dmg = g->now;
    return t;
}

/* divisão do valor por tipo de npc: economia, pesquisa, frota, defesa */
static const double SPLIT[NPC_TYPES][4] = {
    {0.75, 0.15, 0.03, 0.07}, {0.70, 0.18, 0.05, 0.07}, {0.50, 0.15, 0.05, 0.30},
    {0.45, 0.20, 0.32, 0.03}, {0.65, 0.15, 0.12, 0.08}, {0.35, 0.10, 0.45, 0.10},
    {0.40, 0.20, 0.30, 0.10}};

static void compose(double value, const int *ids, const double *share, int n,
                    long long *ship, long long *def) {
    for (int i = 0; i < n; i++) {
        const ItemDef *d = &ITEMS[ids[i]];
        double unit = d->cost[0] + d->cost[1] + d->cost[2];
        long long k = (long long)floor(value * share[i] / unit);
        if (k < 0) k = 0;
        if (d->maxlvl && k > d->maxlvl) k = d->maxlvl;
        if (IS_SHIP(ids[i])) ship[SI(ids[i])] += k;
        else def[DI(ids[i])] += k;
    }
}

void compose_fleet(double value, double pts, long long *ship) {
    static const int T0[] = {S_LIGHT_FIGHTER, S_SMALL_CARGO, S_PROBE};
    static const double W0[] = {0.65, 0.30, 0.05};
    static const int T1[] = {S_LIGHT_FIGHTER, S_HEAVY_FIGHTER, S_CRUISER,
                             S_SMALL_CARGO,   S_LARGE_CARGO,   S_PROBE};
    static const double W1[] = {0.30, 0.20, 0.25, 0.10, 0.10, 0.05};
    static const int T2[] = {S_CRUISER,       S_BATTLESHIP,  S_LIGHT_FIGHTER,
                             S_HEAVY_FIGHTER, S_LARGE_CARGO, S_BOMBER,
                             S_RECYCLER,      S_PROBE};
    static const double W2[] = {0.25, 0.35, 0.10, 0.05, 0.10, 0.05, 0.05, 0.05};
    static const int T3[] = {S_BATTLESHIP, S_BATTLECRUISER, S_CRUISER,  S_DESTROYER,
                             S_BOMBER,     S_LARGE_CARGO,   S_RECYCLER, S_PROBE};
    static const double W3[] = {0.25, 0.20, 0.10, 0.15, 0.10, 0.10, 0.05, 0.05};
    static const int T4[] = {S_BATTLESHIP, S_BATTLECRUISER, S_DESTROYER, S_REAPER,
                             S_DEATHSTAR,  S_LARGE_CARGO,   S_RECYCLER,  S_PROBE};
    static const double W4[] = {0.20, 0.20, 0.20, 0.15, 0.05, 0.10, 0.05, 0.05};
    long long dummy[N_DEF] = {0};
    if (pts < 500) compose(value, T0, W0, 3, ship, dummy);
    else if (pts < 5000) compose(value, T1, W1, 6, ship, dummy);
    else if (pts < 50000) compose(value, T2, W2, 8, ship, dummy);
    else if (pts < 500000) compose(value, T3, W3, 8, ship, dummy);
    else compose(value, T4, W4, 8, ship, dummy);
}

void compose_defense(double value, double pts, long long *def) {
    static const int T0[] = {D_ROCKET, D_LIGHT_LASER};
    static const double W0[] = {0.7, 0.3};
    static const int T1[] = {D_ROCKET, D_LIGHT_LASER, D_HEAVY_LASER, D_SMALL_DOME};
    static const double W1[] = {0.35, 0.30, 0.20, 0.15};
    static const int T2[] = {D_ROCKET, D_LIGHT_LASER, D_HEAVY_LASER,
                             D_GAUSS,  D_ION,         D_SMALL_DOME};
    static const double W2[] = {0.25, 0.20, 0.15, 0.25, 0.10, 0.05};
    static const int T3[] = {D_ROCKET, D_LIGHT_LASER, D_HEAVY_LASER, D_GAUSS,
                             D_ION,    D_PLASMA,      D_SMALL_DOME,  D_LARGE_DOME};
    static const double W3[] = {0.20, 0.15, 0.10, 0.25, 0.10, 0.15, 0.02, 0.03};
    long long dummy[N_SHIP] = {0};
    if (pts < 500) compose(value, T0, W0, 2, dummy, def);
    else if (pts < 5000) compose(value, T1, W1, 4, dummy, def);
    else if (pts < 50000) compose(value, T2, W2, 6, dummy, def);
    else compose(value, T3, W3, 8, dummy, def);
}

static int lvl_from_cum(double value, double base, double f) {
    /* custo acumulado base*(f^L - 1)/(f - 1) = valor */
    if (value <= 0) return 0;
    double L = log(value * (f - 1.0) / base + 1.0) / log(f);
    return L < 0 ? 0 : (int)L;
}

void npc_snapshot(Game *g, int idx, NpcSnap *s) {
    memset(s, 0, sizeof *s);
    const NpcPlanet *pl = &g->npl[idx];
    const Npc *n = &g->npc[pl->npc];
    s->npc = pl->npc;
    double pts = npc_points(g, pl->npc);
    s->points = pts;
    double share =
        n->nplanets == 1 ? 1.0 : (pl->idx == 0 ? 0.4 : 0.6 / (n->nplanets - 1));
    double pv = pts * 1000.0 * share;
    const double *sp = SPLIT[n->type];
    double econ = pv * sp[0];
    s->bld[B_METAL_MINE] = lvl_from_cum(econ * 0.35, 75, 1.5);
    s->bld[B_CRYSTAL_MINE] = lvl_from_cum(econ * 0.25, 72, 1.6);
    s->bld[B_DEUT_SYNTH] = lvl_from_cum(econ * 0.15, 300, 1.5);
    s->bld[B_SOLAR_PLANT] = lvl_from_cum(econ * 0.10, 105, 1.5);
    s->bld[B_ROBOTICS] = lvl_from_cum(econ * 0.05, 720, 2.0);
    s->bld[B_SHIPYARD] = lvl_from_cum(econ * 0.04, 700, 2.0);
    s->bld[B_LAB] = lvl_from_cum(econ * 0.03, 800, 2.0);
    s->bld[B_METAL_STORE] = lvl_from_cum(econ * 0.02, 1000, 2.0);
    s->bld[B_CRYSTAL_STORE] = lvl_from_cum(econ * 0.01, 1500, 2.0);
    s->bld[B_DEUT_TANK] = lvl_from_cum(econ * 0.01, 2000, 2.0);
    double rp = pts * 1000.0 * sp[1];
    int combat = lvl_from_cum(rp * 0.12, 1000, 2.0);
    s->tech[TI(T_WEAPONS)] = combat;
    s->tech[TI(T_SHIELDING)] = combat > 0 ? combat - 1 : 0;
    s->tech[TI(T_ARMOR)] = combat;
    s->tech[TI(T_ESPIONAGE)] = lvl_from_cum(rp * 0.05, 1400, 2.0);
    s->tech[TI(T_COMPUTER)] = lvl_from_cum(rp * 0.05, 1000, 2.0);
    s->tech[TI(T_ENERGY)] = lvl_from_cum(rp * 0.08, 1200, 2.0);
    if (n->type == NPC_XENO) {
        s->tech[TI(T_WEAPONS)] += 3;
        s->tech[TI(T_SHIELDING)] += 3;
        s->tech[TI(T_ARMOR)] += 3;
    }
    /* estado de dano */
    Touched *t = touched_get(g, pl->g, pl->s, pl->p, 0);
    double ddmg = 0, fdmg = 0, since = (g->now - g->created) * g->speed / 3600.0 + 24.0;
    if (t) {
        double hrs = (g->now - t->t_dmg) * g->speed / 3600.0;
        ddmg = t->def_dmg * exp(-hrs / 18.0);
        fdmg = t->fleet_dmg * exp(-hrs / 30.0);
        since = (g->now - t->t_raid) * g->speed / 3600.0;
    }
    double fv = pv * sp[2] * (1.0 - fdmg);
    double dv = pv * sp[3] * (1.0 - ddmg);
    compose_fleet(fv, pts, s->ship);
    compose_defense(dv, pts, s->def);
    /* recursos */
    PlanetInfo pi;
    planet_generate(g->seed, pl->g, pl->s, pl->p, 0, &pi);
    double tavg = (pi.tmin + pi.tmax) / 2.0;
    double ph[3] = {prod_metal_lvl(s->bld[B_METAL_MINE]) + 30,
                    prod_crystal_lvl(s->bld[B_CRYSTAL_MINE]) + 15,
                    prod_deut_lvl(s->bld[B_DEUT_SYNTH], tavg)};
    double hours = since;
    double maxh = n->type == NPC_INACTIVE ? 96.0 : (n->type == NPC_TRADER ? 24.0 : 8.0);
    if (hours > maxh) hours = maxh;
    if (hours < 0) hours = 0;
    double caps[3] = {storage_cap(s->bld[B_METAL_STORE]),
                      storage_cap(s->bld[B_CRYSTAL_STORE]),
                      storage_cap(s->bld[B_DEUT_TANK])};
    for (int i = 0; i < 3; i++) {
        double v =
            ph[i] * hours * g->speed / g->speed; /* as horas já são horas de jogo */
        v += pv * 0.002 * (i == 0 ? 1.0 : i == 1 ? 0.6 : 0.3);
        if (v > caps[i]) v = caps[i];
        s->res[i] = floor(v);
    }
}

int player_body_at(const Game *g, int gg, int ss, int pp, int moon) {
    for (int i = 0; i < g->nbody; i++) {
        const Body *b = &g->body[i];
        if (b->used && b->g == gg && b->s == ss && b->p == pp &&
            b->is_moon == (moon ? 1 : 0))
            return i;
    }
    return -1;
}

int is_explored(const Game *g, int gg, int ss, int pp) {
    int k = gg * 100000 + ss * 100 + pp;
    for (int i = 0; i < g->nexplored; i++)
        if (g->explored[i] == k) return 1;
    return 0;
}

void set_explored(Game *g, int gg, int ss, int pp) {
    if (is_explored(g, gg, ss, pp)) return;
    int k = gg * 100000 + ss * 100 + pp;
    if (g->nexplored < MAX_EXPLORED) g->explored[g->nexplored++] = k;
    else {
        memmove(g->explored, g->explored + 1, sizeof(int) * (MAX_EXPLORED - 1));
        g->explored[MAX_EXPLORED - 1] = k;
    }
}

Debris *debris_at(Game *g, int gg, int ss, int pp, int create) {
    for (int i = 0; i < g->ndebris; i++)
        if (g->debris[i].g == gg && g->debris[i].s == ss && g->debris[i].p == pp)
            return &g->debris[i];
    if (!create) return NULL;
    int slot = g->ndebris;
    if (slot >= MAX_DEBRIS) {
        double mn = 1e300;
        slot = 0;
        for (int i = 0; i < MAX_DEBRIS; i++)
            if (g->debris[i].m + g->debris[i].c < mn) {
                mn = g->debris[i].m + g->debris[i].c;
                slot = i;
            }
    } else g->ndebris++;
    Debris *d = &g->debris[slot];
    memset(d, 0, sizeof *d);
    d->g = gg;
    d->s = ss;
    d->p = pp;
    return d;
}

static const Game *sort_g;
static int rank_cmp(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    double px = npc_points(sort_g, x), py = npc_points(sort_g, y);
    return px > py ? -1 : px < py;
}

int npc_rank_list(const Game *g, int *order, double *pts) {
    int n = 0;
    for (int i = 0; i < N_NPC; i++) {
        if (g->npc[i].type == NPC_PIRATE || g->npc[i].type == NPC_XENO) continue;
        order[n++] = i;
    }
    sort_g = g;
    qsort(order, (size_t)n, sizeof(int), rank_cmp);
    if (pts)
        for (int i = 0; i < n; i++) pts[i] = npc_points(g, order[i]);
    return n;
}

int player_rank(const Game *g, int *total) {
    double me = player_points(g, NULL, NULL, NULL, NULL);
    int rank = 1, tot = 1;
    for (int i = 0; i < N_NPC; i++) {
        if (g->npc[i].type == NPC_PIRATE || g->npc[i].type == NPC_XENO) continue;
        tot++;
        if (npc_points(g, i) > me) rank++;
    }
    if (total) *total = tot;
    return rank;
}

int max_colonies(const Game *g) { return (g->tech[TI(T_ASTRO)] + 1) / 2 + 1; }

int colonies_count(const Game *g) {
    int n = 0;
    for (int i = 0; i < g->nbody; i++)
        if (g->body[i].used && !g->body[i].is_moon) n++;
    return n;
}

int position_allowed(const Game *g, int p) {
    int a = g->tech[TI(T_ASTRO)];
    if (p >= 4 && p <= 12) return 1;
    if (p == 3 || p == 13) return a >= 4;
    if (p == 2 || p == 14) return a >= 6;
    if (p == 1 || p == 15) return a >= 8;
    return 0;
}
