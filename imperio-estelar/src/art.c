/* art.c - procedural ASCII planets, landscapes with building sprites, ships */
#include "art.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t deep, shallow, beach, low, mid, high, peak, snow, cloud, atmo, ground;
    double cap, clouds;
} Pal;

static const Pal PALS[BIO_COUNT] = {
    /* TEMPERATE */ {RGB(16, 40, 110), RGB(30, 95, 175), RGB(200, 185, 130), RGB(70, 150, 60), RGB(40, 110, 45), RGB(115, 105, 70), RGB(150, 150, 150), RGB(240, 245, 250), RGB(245, 245, 255), RGB(100, 160, 255), RGB(50, 95, 40), 0.80, 0.55},
    /* OCEANIC */   {RGB(10, 35, 105), RGB(30, 115, 185), RGB(215, 205, 155), RGB(80, 165, 95), RGB(55, 130, 70), RGB(120, 120, 100), RGB(170, 170, 170), RGB(240, 245, 250), RGB(240, 245, 255), RGB(90, 170, 255), RGB(60, 105, 65), 0.82, 0.62},
    /* JUNGLE */    {RGB(20, 60, 90), RGB(40, 115, 110), RGB(125, 115, 60), RGB(35, 130, 40), RGB(20, 95, 30), RGB(45, 75, 35), RGB(95, 95, 75), RGB(230, 235, 235), RGB(235, 240, 240), RGB(120, 210, 170), RGB(30, 80, 30), 0.92, 0.68},
    /* DESERT */    {RGB(60, 95, 115), RGB(95, 145, 140), RGB(235, 205, 145), RGB(225, 175, 105), RGB(205, 145, 85), RGB(175, 115, 65), RGB(140, 90, 60), RGB(245, 235, 215), RGB(240, 225, 200), RGB(235, 175, 115), RGB(175, 135, 80), 0.96, 0.15},
    /* SAVANNA */   {RGB(30, 60, 115), RGB(60, 115, 155), RGB(205, 185, 125), RGB(175, 165, 75), RGB(145, 145, 60), RGB(135, 105, 65), RGB(125, 115, 105), RGB(235, 235, 230), RGB(240, 240, 235), RGB(190, 205, 150), RGB(140, 125, 60), 0.90, 0.35},
    /* TUNDRA */    {RGB(30, 50, 85), RGB(60, 90, 125), RGB(135, 135, 125), RGB(115, 135, 115), RGB(135, 145, 135), RGB(165, 165, 165), RGB(205, 205, 215), RGB(235, 240, 250), RGB(230, 235, 245), RGB(170, 205, 235), RGB(110, 120, 110), 0.55, 0.45},
    /* ICE */       {RGB(80, 120, 170), RGB(150, 195, 230), RGB(185, 215, 240), RGB(200, 225, 245), RGB(220, 235, 250), RGB(235, 245, 255), RGB(250, 250, 255), RGB(250, 252, 255), RGB(235, 245, 255), RGB(180, 220, 255), RGB(190, 210, 230), 0.0, 0.30},
    /* VOLCANIC */  {RGB(255, 85, 0), RGB(255, 170, 40), RGB(70, 35, 25), RGB(55, 42, 40), RGB(75, 58, 52), RGB(95, 75, 65), RGB(125, 105, 95), RGB(145, 135, 125), RGB(120, 110, 100), RGB(225, 120, 60), RGB(45, 35, 35), 1.10, 0.35},
    /* TOXIC */     {RGB(125, 155, 20), RGB(175, 195, 45), RGB(145, 125, 65), RGB(115, 75, 135), RGB(95, 62, 115), RGB(135, 95, 145), RGB(175, 145, 175), RGB(225, 225, 175), RGB(225, 225, 150), RGB(205, 225, 85), RGB(90, 70, 100), 1.10, 0.55},
    /* BARREN */    {RGB(70, 65, 60), RGB(90, 85, 80), RGB(110, 105, 100), RGB(130, 124, 118), RGB(150, 144, 138), RGB(170, 164, 158), RGB(190, 185, 180), RGB(220, 215, 210), RGB(200, 200, 200), RGB(150, 150, 150), RGB(110, 105, 100), 1.10, 0.0},
    /* MOON */      {RGB(70, 70, 75), RGB(90, 90, 95), RGB(110, 110, 115), RGB(135, 135, 140), RGB(155, 155, 160), RGB(175, 175, 180), RGB(195, 195, 200), RGB(220, 220, 225), RGB(200, 200, 200), RGB(150, 150, 150), RGB(120, 120, 125), 1.10, 0.0},
};

uint32_t biome_color(int b) { return PALS[clampi(b, 0, BIO_COUNT - 1)].low; }
uint32_t sky_color(const PlanetInfo *pi) { return PALS[clampi(pi->biome, 0, BIO_COUNT - 1)].atmo; }

static int in_clip(int x, int y, int cx, int cy, int cw, int ch) { return x >= cx && y >= cy && x < cx + cw && y < cy + ch; }

void art_starfield(int x0, int y0, int w, int h, uint64_t seed, double t, uint32_t bg) {
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++) {
            uint64_t hh = hash_combine(seed, (uint64_t)(x * 131 + y * 7919));
            double q = hash_unit(hh);
            uint32_t ch = ' ', fg = RGB(80, 80, 90);
            if (q < 0.028) {
                double tw = 0.55 + 0.45 * sin(t * (1.0 + hash_unit(hh ^ 77) * 2.5) + q * 900.0);
                int c = (int)(90 + 150 * tw * (q < 0.004 ? 1.0 : 0.6));
                double hue = hash_unit(hh ^ 99);
                fg = hue < 0.15 ? RGB(c, c * 8 / 10, c * 6 / 10) : hue < 0.3 ? RGB(c * 8 / 10, c * 9 / 10, c) : RGB(c, c, c);
                ch = q < 0.002 ? (tw > 0.8 ? '+' : '*') : q < 0.006 ? '*' : '.';
                if (q >= 0.006 && q < 0.012) ch = 0x00B7;
            }
            cv_put(x, y, ch, fg, bg);
        }
}

typedef enum { TR_DEEP, TR_SHALLOW, TR_BEACH, TR_LOW, TR_MID, TR_HIGH, TR_PEAK, TR_SNOW, TR_LAVA, TR_CRACK } Terr;

void art_planet_disc(double cx, double cy, double R, const PlanetInfo *pi, double t, double lights, int clx, int cly, int clw, int clh) {
    const Pal *pl = &PALS[clampi(pi->biome, 0, BIO_COUNT - 1)];
    double period = 70.0 * clampd(pi->daylen / 24.0, 0.5, 3.0);
    double rot = t * 2.0 * M_PI / period + hash_unit(pi->vseed ^ 5) * 6.28;
    double tilt = 0.12 + hash_unit(pi->vseed ^ 1) * 0.35;
    double Lx = -0.58, Ly = -0.36, Lz = 0.73;
    double ln = sqrt(Lx * Lx + Ly * Ly + Lz * Lz);
    Lx /= ln; Ly /= ln; Lz /= ln;
    double sea;
    switch (pi->biome) {
    case BIO_VOLCANIC: sea = -0.30; break;
    case BIO_ICE: case BIO_BARREN: case BIO_MOON: sea = -9; break;
    case BIO_DESERT: sea = pi->water > 0.02 ? -0.95 : -9; break;
    default: sea = (pi->water - 0.5) * 0.85; break;
    }
    int has_atm = pi->atmo != ATM_NONE && pi->pressure > 0.04;
    double atm = has_atm ? clampd(0.05 + log10(1.0 + pi->pressure) * 0.07, 0.05, 0.16) : 0.0;
    double ringf = pi->ring ? 1.95 : 1.0;
    int xa = (int)floor(cx - 2.0 * R * (1.0 + atm) * ringf) - 1, xb = (int)ceil(cx + 2.0 * R * (1.0 + atm) * ringf) + 1;
    int ya = (int)floor(cy - R * (1.0 + atm) - 1), yb = (int)ceil(cy + R * (1.0 + atm) + 1);
    double ct = cos(tilt), st = sin(tilt), cr = cos(rot), sr = sin(rot);
    double crc = cos(rot * 1.22 + t * 0.01), src = sin(rot * 1.22 + t * 0.01);
    uint64_t sd = pi->vseed;
    for (int y = ya; y <= yb; y++) {
        for (int x = xa; x <= xb; x++) {
            if (!in_clip(x, y, clx, cly, clw, clh)) continue;
            double u = (x + 0.5 - cx) / (2.0 * R), v = (y + 0.5 - cy) / R;
            double r2 = u * u + v * v;
            /* ring behind (upper half) */
            if (pi->ring && r2 > 1.0) {
                double rv = v / 0.22;
                double rho = sqrt(u * u + rv * rv);
                if (rho > 1.35 && rho < 1.95 && (v < 0 || r2 > 1.0)) {
                    double band = 0.6 + 0.4 * sin(rho * 23.0);
                    double lit = clampd(0.35 + 0.65 * (-(u * Lx + v * Ly)), 0.2, 1.0);
                    cv_put(x, y, band > 0.75 ? '=' : '-', col_scale(RGB(205, 185, 145), band * lit), COL_KEEP);
                    continue;
                }
            }
            if (r2 <= 1.0) {
                double z = sqrt(1.0 - r2);
                double ndl = u * Lx + v * Ly + z * Lz;
                double lum = ndl > 0 ? ndl : 0;
                lum = (0.05 + 0.95 * lum) * (0.72 + 0.28 * z);
                /* planet space */
                double yp = v * ct - z * st, zp = v * st + z * ct;
                double px = u * cr + zp * sr, pz = -u * sr + zp * cr, py = yp;
                double e = fbm3(sd, px * 1.7 + 11.0, py * 1.7 + 3.0, pz * 1.7 - 7.0, 5) / 0.42;
                double dtl = noise3(sd + 99, px * 7.0, py * 7.0, pz * 7.0);
                Terr tr;
                if (e < sea - 0.18) tr = TR_DEEP;
                else if (e < sea) tr = TR_SHALLOW;
                else if (e < sea + 0.04) tr = TR_BEACH;
                else {
                    double hgt = sea < -5 ? (e + 1.0) / 2.3 : (e - sea) / (1.2 - sea);
                    tr = hgt < 0.22 ? TR_LOW : hgt < 0.45 ? TR_MID : hgt < 0.68 ? TR_HIGH : TR_PEAK;
                }
                double lat = fabs(py);
                if (pl->cap < 1.0 && lat + dtl * 0.08 > pl->cap) tr = TR_SNOW;
                if (pi->biome == BIO_ICE) {
                    if (fabs(dtl) < 0.025) tr = TR_CRACK;
                    else tr = e < -0.2 ? TR_LOW : e < 0.2 ? TR_MID : e < 0.5 ? TR_HIGH : TR_PEAK;
                }
                if (pi->biome == BIO_VOLCANIC && tr <= TR_SHALLOW) tr = TR_LAVA;
                if (pi->biome == BIO_VOLCANIC && tr >= TR_LOW && fabs(dtl) < 0.03) tr = TR_LAVA;
                uint32_t col;
                switch (tr) {
                case TR_DEEP: col = pl->deep; break;
                case TR_SHALLOW: col = pl->shallow; break;
                case TR_BEACH: col = pl->beach; break;
                case TR_LOW: col = pl->low; break;
                case TR_MID: col = pl->mid; break;
                case TR_HIGH: col = pl->high; break;
                case TR_PEAK: col = pl->peak; break;
                case TR_SNOW: col = pl->snow; break;
                case TR_CRACK: col = pl->deep; break;
                default: col = e < sea - 0.1 ? pl->deep : pl->shallow; break;
                }
                if (pi->biome == BIO_BARREN || pi->biome == BIO_MOON) {
                    double cr2 = noise3(sd + 33, px * 4.0, py * 4.0, pz * 4.0);
                    if (cr2 > 0.33) col = pl->high;
                    else if (cr2 < -0.30) col = pl->deep;
                }
                col = col_scale(col, 0.9 + dtl * 0.35);
                uint32_t ch;
                uint32_t fc;
                int water = (tr == TR_DEEP || tr == TR_SHALLOW) && pi->biome != BIO_VOLCANIC;
                if (tr == TR_LAVA) {
                    double glow = 0.75 + 0.25 * sin(t * 2.0 + dtl * 30.0);
                    fc = col_scale(e < sea - 0.1 ? pl->deep : pl->shallow, clampd(glow * (0.55 + 0.45 * lum), 0.5, 1.1));
                    ch = lum > 0.5 ? '~' : (glow > 0.85 ? '*' : '~');
                } else if (water) {
                    static const char WR[] = " .-~~=";
                    int k = (int)(lum * 5.0 + 0.5);
                    if (k > 5) k = 5;
                    ch = (uint32_t)WR[k];
                    if (ch == ' ' && lum > 0.06) ch = '.';
                    double spec = pow(clampd(ndl, 0, 1), 18.0) * 0.6;
                    fc = col_lerp(col_scale(col, lum * 1.15), RGB(255, 255, 240), spec);
                } else {
                    static const char LR[] = " .:-=+*#%@";
                    int k = (int)(lum * 9.0 + 0.5);
                    if (k > 9) k = 9;
                    if (tr == TR_PEAK && lum > 0.35) ch = '^';
                    else if (tr == TR_CRACK) ch = lum > 0.3 ? '/' : '.';
                    else ch = (uint32_t)LR[k];
                    if (ch == ' ' && lum > 0.05) ch = '.';
                    fc = col_scale(col, lum * 1.1);
                }
                /* city lights on the night side */
                if (lights > 0 && !water && tr != TR_LAVA && ndl < 0.08) {
                    uint64_t ch2 = hash_combine(sd ^ 0x11, (uint64_t)((long)floor(px * 26.0) * 73856093L ^ (long)floor(py * 26.0) * 19349663L ^ (long)floor(pz * 26.0) * 83492791L));
                    double q = hash_unit(ch2);
                    if (q < lights * 0.45) {
                        ch = q < lights * 0.12 ? '*' : 0x00B7;
                        fc = col_scale(RGB(255, 205, 120), 0.6 + 0.4 * (1.0 - clampd(ndl + 0.1, 0, 1)));
                    }
                }
                /* clouds */
                if (pl->clouds > 0) {
                    double cx2 = u * crc + zp * src, cz2 = -u * src + zp * crc;
                    double cvv = fbm3(sd + 777, cx2 * 2.1, py * 2.6, cz2 * 2.1, 4) / 0.42;
                    double thr = 1.0 - pl->clouds * 1.35;
                    if (cvv > thr) {
                        double a = clampd((cvv - thr) / 0.55, 0, 1) * 0.9;
                        uint32_t cc = col_scale(pl->cloud, lum * 1.05);
                        fc = col_lerp(fc, cc, a);
                        if (a > 0.45) ch = lum > 0.62 ? '@' : lum > 0.38 ? 'o' : lum > 0.15 ? ':' : '.';
                    }
                }
                cv_put(x, y, ch, fc, RGB(0, 0, 0));
            } else if (has_atm && r2 <= (1.0 + atm) * (1.0 + atm)) {
                double r = sqrt(r2);
                double a = (1.0 + atm - r) / atm;
                double side = clampd(((u / r) * Lx + (v / r) * Ly) * 0.5 + 0.5, 0, 1);
                double k = a * (0.15 + 0.85 * side);
                if (k > 0.12) cv_put(x, y, k > 0.55 ? ':' : '.', col_scale(pl->atmo, 0.35 + 0.75 * k), COL_KEEP);
            }
            /* ring front (lower half) */
            if (pi->ring) {
                double rv = v / 0.22;
                double rho = sqrt(u * u + rv * rv);
                if (rho > 1.35 && rho < 1.95 && v > 0.0 && r2 <= 1.0) {
                    double band = 0.6 + 0.4 * sin(rho * 23.0);
                    double lit = clampd(0.35 + 0.65 * (-(u * Lx + v * Ly)), 0.2, 1.0);
                    cv_put(x, y, band > 0.75 ? '=' : '-', col_scale(RGB(205, 185, 145), band * lit), COL_KEEP);
                }
            }
        }
    }
}

void art_planet(int x, int y, int w, int h, const PlanetInfo *pi, double t, double lights, uint64_t xs,
                int has_moon, int sats, int station, int bg_stars) {
    if (bg_stars) art_starfield(x, y, w, h, pi->vseed ^ 0xABC, t, RGB(0, 0, 0));
    double R = h / 2.0 - 1.3;
    if (pi->ring) { if (2.0 * R * 1.95 > w / 2.0 - 1) R = (w / 2.0 - 1) / (2.0 * 1.95); }
    else if (2.0 * R > w / 2.0 - 4) R = (w / 2.0 - 4) / 2.0;
    double cx = x + w / 2.0, cy = y + h / 2.0;
    PlanetInfo moon;
    memset(&moon, 0, sizeof moon);
    moon.biome = BIO_MOON;
    moon.vseed = hash64(pi->vseed ^ 0x4D4F4F4EULL);
    moon.daylen = 300;
    double ma = t * 2.0 * M_PI / 55.0 + hash_unit(xs) * 6.28;
    double mR = R * 0.2 < 1.4 ? 1.4 : R * 0.2;
    double mx = cx + cos(ma) * R * 2.0 * 1.42, my = cy + sin(ma) * R * 0.42;
    int behind = sin(ma) < 0;
    if (has_moon && behind) art_planet_disc(mx, my, mR, &moon, t, 0, x, y, w, h);
    art_planet_disc(cx, cy, R, pi, t, lights, x, y, w, h);
    if (has_moon && !behind) art_planet_disc(mx, my, mR, &moon, t, 0, x, y, w, h);
    int ns = sats > 8 ? 8 : sats;
    for (int k = 0; k < ns; k++) {
        double a = t * 2.0 * M_PI / 24.0 + k * 2.0 * M_PI / ns + 0.3;
        double sx = cx + cos(a) * R * 2.0 * 1.18, sy = cy + sin(a) * R * 0.55 - 0.3 * cos(a);
        int ix = (int)floor(sx), iy = (int)floor(sy);
        double uu = (ix + 0.5 - cx) / (2.0 * R), vv = (iy + 0.5 - cy) / R;
        if (sin(a) < 0 && uu * uu + vv * vv < 1.0) continue;
        if (in_clip(ix, iy, x, y, w, h)) cv_put(ix, iy, '+', RGB(140, 200, 255), COL_KEEP);
    }
    if (station) {
        double a = t * 2.0 * M_PI / 33.0 + 2.0;
        double sx = cx + cos(a) * R * 2.0 * 1.28, sy = cy + sin(a) * R * 0.72;
        int ix = (int)floor(sx) - 1, iy = (int)floor(sy);
        double uu = (ix + 1.5 - cx) / (2.0 * R), vv = (iy + 0.5 - cy) / R;
        if (!(sin(a) < 0 && uu * uu + vv * vv < 1.0))
            for (int k = 0; k < 3; k++)
                if (in_clip(ix + k, iy, x, y, w, h)) cv_put(ix + k, iy, (uint32_t)"=O="[k], k == 1 ? RGB(250, 220, 120) : RGB(170, 180, 195), COL_KEEP);
    }
}

/* ======================= SPRITES ======================= */
typedef struct {
    int w, n, m0, m1;
    const char *art[10];
    const char *mask[10];
} Spr;

static uint32_t mask_col(char m, double night) {
    switch (m) {
    case 'a': return RGB(155, 165, 180);
    case 'd': return RGB(95, 100, 115);
    case 'w': return RGB(235, 235, 240);
    case 'r': return RGB(225, 70, 55);
    case 'o': return RGB(245, 145, 45);
    case 'y': return RGB(250, 220, 90);
    case 'g': return RGB(90, 205, 95);
    case 'c': return RGB(90, 225, 235);
    case 'b': return RGB(80, 135, 235);
    case 'm': return RGB(205, 95, 225);
    case 'n': return RGB(150, 105, 65);
    case 't': return RGB(45, 175, 155);
    case 'l': return RGB(160, 210, 255);
    case 's': return RGB(215, 185, 125);
    case 'k': return RGB(55, 55, 65);
    case 'L': return night > 0.5 ? RGB(255, 215, 110) : RGB(70, 80, 95);
    case 'G': return RGB(140, 255, 140);
    }
    return RGB(200, 200, 200);
}

/* resource buildings */
static const Spr SP_METAL = {10, 6, 2, 3,
    {"    /\\    ", "   /##\\   ", "   |##|   ", "  _|##|_  ", " /=|##|=\\ ", "[o======o]"},
    {"    aa    ", "   adda   ", "   adda   ", "  aaddaa  ", " oaaddaao ", "dddddddddd"}};
static const Spr SP_CRYSTAL = {10, 6, 2, 3,
    {"   ^   ^  ", "  /c\\ /c\\ ", "  |c| |c| ", " _|c|_|c|_", " \\=======/", "[########]"},
    {"   c   c  ", "  ccc ccc ", "  lcl lcl ", " alcalalca", " aaaaaaaaa", "dddddddddd"}};
static const Spr SP_DEUT = {10, 6, 2, 3,
    {"  _    _  ", " (_)  (_) ", " |T|==|T| ", " |T|  |T| ", " |_|==|_| ", "=========="},
    {"  t    t  ", " ttt  ttt ", " tct  tct ", " tct  tct ", " ttt  ttt ", "dddddddddd"}};
static const Spr SP_SOLAR = {10, 5, 2, 3,
    {" //////// ", " ######## ", "   |  |   ", "   |  |   ", "_[]|__|[]_"},
    {" bbbbbbbb ", " llllllll ", "   a  a   ", "   a  a   ", "aLaaaaaaLa"}};
static const Spr SP_FUSION = {10, 6, 2, 3,
    {"   _..._  ", "  /(@@)\\  ", " |  ()  | ", " |  ()  | ", " |______| ", "[========]"},
    {"   aaaaa  ", "  ayyyya  ", " a  oo  a ", " a  oo  a ", " aaaaaaaa ", "dddddddddd"}};
static const Spr SP_MSTORE = {7, 4, 1, 2,
    {" _____ ", "|[][]|", "|[][]|", "|_____|"},
    {" aaaaa ", "aaaaaa", "aaaaaa", "aaaaaaa"}};
static const Spr SP_CSTORE = {7, 4, 1, 2,
    {" _____ ", "|<><>|", "|<><>|", "|_____|"},
    {" lllll ", "lccccl", "lccccl", "lllllll"}};
static const Spr SP_DTANK = {7, 4, 1, 2,
    {" .---. ", "(     )", "|     |", "'-----'"},
    {" ttttt ", "t     t", "t     t", "ttttttt"}};
/* facilities */
static const Spr SP_ROBOT = {10, 5, 2, 3,
    {"   __/\\_  ", "  |o   |\\ ", "  |[]=L|  ", "  |[]=L|  ", "__|____|__"},
    {"   aaoaa  ", "  ay   aa ", "  aaaaLa  ", "  aaaaLa  ", "dddddddddd"}};
static const Spr SP_SHIPYARD = {14, 6, 2, 3,
    {" |=========- ", " |    |      ", " |  _/=\\__   ", " | <=====##> ", " |  |   |    ", "_|__[____]___"},
    {" aaaaaaaaaay ", " a    a      ", " a  wwwwww   ", " a wwwwwwoo  ", " a  a   a    ", "ddddddddddddd"}};
static const Spr SP_LAB = {10, 6, 3, 4,
    {"    |     ", "   _|_    ", "  /   \\   ", " | L o L| ", " | L   L| ", "_|_____|__"},
    {"    r     ", "   aaa    ", "  wwwww   ", " wLwcwLww ", " wLwwwLww ", "dddddddddd"}};
static const Spr SP_SILO = {10, 4, 1, 2,
    {"  /\\  /\\  ", "  ||  ||  ", "  ||  ||  ", " [||==||] "},
    {"  rr  rr  ", "  aa  aa  ", "  aa  aa  ", " dddddddd "}};
static const Spr SP_NANITE = {10, 4, 1, 2,
    {"  .::::.  ", " |:L::L:| ", " |::::::| ", "_|______|_"},
    {"  gggggg  ", " agLggLga ", " agggggga ", "dddddddddd"}};
static const Spr SP_TERRA = {10, 5, 2, 3,
    {"    !     ", "   /T\\    ", "  | T |   ", "  | T |   ", " /_____\\  "},
    {"    y     ", "   aya    ", "  a y a   ", "  a y a   ", " aaaaaaa  "}};
static const Spr SP_HUB = {11, 5, 2, 3,
    {"  _$_      ", " |   |##   ", " | L |##== ", " | L |##== ", "[_________]"},
    {"  aya      ", " a   aon   ", " a L aon b ", " a L aon b ", "ddddddddddd"}};
static const Spr SP_STATION = {9, 2, 0, 0,
    {" .-o-.   ", "=[###]=  "},
    {" aaya    ", "llaaall  "}};
static const Spr SP_DOCK = {11, 1, 0, 0, {"<|=====|>  "}, {"aaaaaaaaa  "}};
/* life */
static const Spr SP_TOWER = {4, 4, 1, 2, {" _  ", "|L| ", "|L| ", "|_| "}, {" a  ", "aLa ", "aLa ", "aaa "}};
static const Spr SP_FARM = {10, 4, 1, 2,
    {"   ____   ", "  /::::\\  ", " /::::::\\ ", "=========="},
    {"   wwww   ", "  wggggw  ", " wggggggw ", "nnnnnnnnnn"}};
static const Spr SP_MED = {9, 4, 1, 2, {"  _____  ", " | +++ | ", " | L L | ", "_|_____|_"}, {"  wwwww  ", " w rrr w ", " w L L w ", "ddddddddd"}};
static const Spr SP_ACAD = {9, 4, 1, 2, {"  /^^^\\  ", " | | | | ", " | | | | ", "_|_|_|_|_"}, {"  sssss  ", " s w w s ", " s w w s ", "ddddddddd"}};
static const Spr SP_FSYN = {10, 4, 1, 2, {"  _   _   ", " (G)=(G)  ", " |G| |G|  ", "_|_|=|_|__"}, {"  g   g   ", " ggaggg   ", " gGg gGg  ", "dddddddddd"}};
static const Spr SP_ARCO = {14, 5, 2, 3,
    {"      /\\      ", "     /LL\\     ", "    /L::L\\    ", "   /::LL::\\   ", "__/________\\__"},
    {"      cc      ", "     cLLc     ", "    cLggLc    ", "   cggLLggc   ", "dddddddddddddd"}};
/* moon */
static const Spr SP_LBASE = {11, 3, 1, 2, {"  _   ___  ", " (O)=(L L) ", "_|_|=|___|_"}, {"  l   lll  ", " lyldlLlLl ", "ddddddddddd"}};
static const Spr SP_PHALANX = {9, 4, 2, 3, {" \\___/   ", "   |     ", "   |     ", "  /_\\    "}, {" lllll   ", "   a     ", "   a     ", "  aaa    "}};
static const Spr SP_GATE = {9, 5, 2, 3, {"  .---.  ", " / ::: \\ ", "|  :m:  |", " \\ ::: / ", "__'---'__"}, {"  aaaaa  ", " a mmm a ", "a  mmm  a", " a mmm a ", "ddaaaaadd"}};
static const Spr SP_HE3 = {9, 4, 1, 2, {"   /\\    ", "  |He|   ", "  |##|=o ", "_[____]__"}, {"   aa    ", "  aooa   ", "  addaao ", "ddddddddd"}};
static const Spr SP_SCAF = {10, 4, 1, 2, {"  _|__    ", "  |\\/|    ", "  |/\\|    ", "__|__|____"}, {"  yyyy    ", "  yyyy    ", "  yyyy    ", "dddddddddd"}};

static int spr_height(const Spr *s, int tier) { return s->n + (s->m1 - s->m0) * (tier - 1); }

static void spr_draw(const Spr *s, int x, int ybot, int tier, double night, int cx, int cy, int cw, int ch) {
    int total = spr_height(s, tier);
    int y = ybot - total + 1;
    for (int r = 0; r < s->n; r++) {
        int reps = (r >= s->m0 && r < s->m1) ? tier : 1;
        for (int k = 0; k < reps; k++, y++) {
            const char *a = s->art[r], *m = s->mask[r];
            const char *pa = a;
            const char *pm = m;
            for (int i = 0; *pa; i++) {
                uint32_t c = utf8_next(&pa);
                uint32_t mc = *pm ? utf8_next(&pm) : 'a';
                if (c == ' ') continue;
                if (in_clip(x + i, y, cx, cy, cw, ch)) {
                    Cell *ce = cv_cell(x + i, y);
                    uint32_t bgc = ce ? col_scale(ce->bg, 0.5) : RGB(0, 0, 0);
                    cv_put(x + i, y, c, mask_col((char)mc, night), bgc);
                }
            }
        }
    }
}

static int tier_of(int lvl) { return lvl <= 0 ? 1 : lvl < 5 ? 1 : lvl < 10 ? 2 : lvl < 15 ? 3 : 4; }

typedef struct { int id; int dx; const Spr *spr; } Slot;

static const Slot L_RES[] = {{B_METAL_MINE, 0, &SP_METAL}, {B_CRYSTAL_MINE, 11, &SP_CRYSTAL}, {B_DEUT_SYNTH, 22, &SP_DEUT},
                             {B_SOLAR_PLANT, 33, &SP_SOLAR}, {B_FUSION, 44, &SP_FUSION}, {B_METAL_STORE, 56, &SP_MSTORE},
                             {B_CRYSTAL_STORE, 64, &SP_CSTORE}, {B_DEUT_TANK, 72, &SP_DTANK}};
static const Slot L_FAC[] = {{B_ROBOTICS, 0, &SP_ROBOT}, {B_SHIPYARD, 11, &SP_SHIPYARD}, {B_LAB, 25, &SP_LAB},
                             {B_MISSILE_SILO, 36, &SP_SILO}, {B_NANITE, 47, &SP_NANITE}, {B_TERRAFORMER, 58, &SP_TERRA},
                             {B_TRADE_HUB, 69, &SP_HUB}};
static const Slot L_LIFE[] = {{B_HABITAT, 0, &SP_TOWER}, {B_FARM, 20, &SP_FARM}, {B_MEDICAL, 31, &SP_MED},
                              {B_ACADEMY, 41, &SP_ACAD}, {B_FOOD_SYNTH, 51, &SP_FSYN}, {B_ARCOLOGY, 63, &SP_ARCO}};
static const Slot L_MOON[] = {{B_LUNAR_BASE, 0, &SP_LBASE}, {B_PHALANX, 12, &SP_PHALANX}, {B_JUMP_GATE, 22, &SP_GATE},
                              {B_HE3, 33, &SP_HE3}, {B_ROBOTICS, 43, &SP_ROBOT}, {B_SHIPYARD, 54, &SP_SHIPYARD}};

static int building_active(const Body *b, int id) {
    if (b->bq.n && b->bq.it[0].item == id && b->bq.it[0].active) return 1;
    if (b->lq.n && b->lq.it[0].item == id && b->lq.it[0].active) return 1;
    return 0;
}

void art_landscape(int x, int y, int w, int h, const Game *g, const Body *b, int kind, double t, int hl) {
    const PlanetInfo *pi = &b->info;
    const Pal *pl = &PALS[clampi(pi->biome, 0, BIO_COUNT - 1)];
    uint64_t sd = pi->vseed;
    double period = 300.0 * clampd(pi->daylen / 24.0, 0.5, 2.0);
    double ph = fmod(t / period + hash_unit(sd ^ 3), 1.0);
    double sunh = sin(2.0 * M_PI * ph);
    double d = clampd(0.5 + sunh * 0.9, 0.0, 1.0);
    double night = 1.0 - d;
    int has_atm = pi->atmo != ATM_NONE && pi->pressure > 0.04;
    uint32_t top, hor;
    if (has_atm) {
        uint32_t dt = col_scale(pl->atmo, 0.42), dh = col_scale(col_lerp(pl->atmo, RGB(255, 255, 255), 0.28), 0.92);
        uint32_t nt = RGB(4, 6, 16), nh = col_scale(pl->atmo, 0.16);
        if (sunh > -0.25 && sunh < 0.25) dh = col_lerp(dh, RGB(250, 140, 80), 0.45 * (1.0 - fabs(sunh) * 4.0));
        top = col_lerp(nt, dt, d);
        hor = col_lerp(nh, dh, d);
    } else { top = RGB(2, 2, 8); hor = RGB(14, 14, 24); }
    int gy = y + h - 4;
    cv_set_clip(x, y, w, h);
    for (int r = y; r < gy; r++) {
        double k = (double)(r - y) / (double)(gy - y > 1 ? gy - y - 1 : 1);
        uint32_t bg = col_lerp(top, hor, k);
        for (int c = x; c < x + w; c++) {
            uint32_t ch = ' ', fg = RGB(200, 200, 200);
            if (!has_atm || d < 0.45) {
                double q = hash_unit(hash_combine(sd ^ 0x51, (uint64_t)(c * 977 + r * 31)));
                double vis = has_atm ? (0.45 - d) / 0.45 : 1.0;
                if (q < 0.035 * vis) {
                    double tw = 0.6 + 0.4 * sin(t * 1.7 + q * 1000);
                    ch = q < 0.006 ? '*' : '.';
                    fg = col_lerp(bg, RGB(230, 230, 255), clampd(tw * vis, 0, 1));
                }
            }
            cv_put(c, r, ch, fg, bg);
        }
    }
    /* sun */
    if (sunh > -0.1) {
        int sx = x + (int)((fmod(ph + 0.75, 1.0)) * (w + 6)) - 3;
        int sy = gy - 1 - (int)(clampd(sunh, 0, 1) * (gy - y - 2));
        uint32_t sc = (hash_unit(sd ^ 9) < 0.3) ? RGB(255, 180, 100) : (hash_unit(sd ^ 9) < 0.6 ? RGB(255, 240, 170) : RGB(220, 235, 255));
        cv_put(sx - 1, sy, '(', col_scale(sc, 0.8), COL_KEEP);
        cv_put(sx, sy, '@', sc, COL_KEEP);
        cv_put(sx + 1, sy, ')', col_scale(sc, 0.8), COL_KEEP);
    }
    /* parent planet in the moon sky */
    if (kind == LS_MOON && b->parent >= 0) {
        const Body *p = &g->body[b->parent];
        art_planet_disc(x + w * 0.80, y + 3.5, 4.5, &p->info, t, clampd(p->pop / 20000.0, 0.1, 1.0), x, y, w, gy - y);
    }
    /* far & near hills */
    uint32_t far = col_lerp(hor, col_scale(pl->peak, 0.55 + 0.4 * d), 0.55);
    uint32_t nearc = col_scale(pl->ground, 0.45 + 0.35 * d);
    int maxfar = (gy - y) / 2;
    for (int c = x; c < x + w; c++) {
        double n1 = noise1(sd ^ 0xF1, (c - x) * 0.09) * 0.5 + 0.5;
        int hf = 1 + (int)(n1 * maxfar);
        double n0 = noise1(sd ^ 0xF1, (c - x - 1) * 0.09) * 0.5 + 0.5, n2 = noise1(sd ^ 0xF1, (c - x + 1) * 0.09) * 0.5 + 0.5;
        int h0 = 1 + (int)(n0 * maxfar), h2 = 1 + (int)(n2 * maxfar);
        for (int k = 0; k < hf; k++) {
            int r = gy - 1 - k;
            uint32_t ch = ' ';
            if (k == hf - 1) ch = (h0 < hf && h2 < hf) ? '^' : (h0 < hf ? '/' : (h2 < hf ? '\\' : ' '));
            cv_put(c, r, ch, col_scale(far, 1.3), far);
        }
        double m1 = noise1(sd ^ 0xE2, (c - x) * 0.17) * 0.5 + 0.5;
        int hn = (int)(m1 * 2.5);
        for (int k = 0; k < hn; k++) cv_put(c, gy - 1 - k, k == hn - 1 ? '.' : ' ', col_scale(nearc, 1.4), nearc);
    }
    /* ground */
    for (int r = gy; r < y + h; r++)
        for (int c = x; c < x + w; c++) {
            double q = hash_unit(hash_combine(sd ^ 0xA7, (uint64_t)(c * 4099 + r * 17)));
            uint32_t bg = col_scale(pl->ground, (0.45 + 0.55 * d) * (0.85 + 0.25 * q) * (r == gy ? 1.08 : 1.0 - 0.06 * (r - gy)));
            uint32_t ch = ' ', fg = col_scale(bg, 1.35);
            switch (pi->biome) {
            case BIO_DESERT: ch = q < 0.25 ? '.' : q < 0.32 ? '~' : ' '; break;
            case BIO_TEMPERATE: case BIO_JUNGLE: case BIO_OCEANIC: ch = q < 0.3 ? '"' : q < 0.4 ? ',' : ' '; fg = col_scale(bg, 1.6); break;
            case BIO_SAVANNA: ch = q < 0.3 ? '\'' : q < 0.4 ? ',' : ' '; break;
            case BIO_ICE: case BIO_TUNDRA: ch = q < 0.2 ? '*' : q < 0.3 ? '.' : ' '; fg = RGB(240, 245, 255); break;
            case BIO_VOLCANIC: if (q < 0.12) { ch = '~'; fg = col_scale(RGB(255, 110, 20), 0.7 + 0.3 * sin(t * 3 + c)); } else if (q < 0.25) ch = '.'; break;
            case BIO_TOXIC: ch = q < 0.2 ? 'o' : q < 0.3 ? '.' : ' '; fg = RGB(190, 220, 60); break;
            default: ch = q < 0.2 ? '.' : q < 0.25 ? 'o' : ' '; break;
            }
            cv_put(c, r, ch, fg, bg);
        }
    /* buildings */
    const Slot *L;
    int nL;
    switch (kind) {
    case LS_FACILITIES: L = L_FAC; nL = (int)(sizeof L_FAC / sizeof *L_FAC); break;
    case LS_LIFE: L = L_LIFE; nL = (int)(sizeof L_LIFE / sizeof *L_LIFE); break;
    case LS_MOON: L = L_MOON; nL = (int)(sizeof L_MOON / sizeof *L_MOON); break;
    default: L = L_RES; nL = (int)(sizeof L_RES / sizeof *L_RES); break;
    }
    int skyh = gy - y;
    if (kind == LS_RESOURCES) {
        long long sats = b->ship[SI(S_SATELLITE)];
        int ns = sats > 6 ? 6 : (int)sats;
        for (int k = 0; k < ns; k++) {
            int sx = x + (int)fmod(k * 13.0 + t * (0.6 + 0.1 * k), (double)w);
            int sy = y + 1 + (k * 3) % (skyh > 4 ? skyh - 4 : 1);
            for (int i = 0; i < 5; i++) cv_put(sx + i - 2 >= x + w ? sx + i - 2 - w : sx + i - 2, sy, (uint32_t)"=[o]="[i], i == 2 ? RGB(250, 220, 90) : RGB(120, 170, 255), COL_KEEP);
        }
        long long cr = b->ship[SI(S_CRAWLER)];
        int nc = cr > 0 ? (int)(cr / 20 + 1) : 0;
        if (nc > 4) nc = 4;
        for (int k = 0; k < nc; k++) {
            int cx2 = x + 2 + (int)fmod(k * 7 + t * 0.8, 30.0);
            cv_text(cx2, gy + 1, "o=o", RGB(240, 170, 60), COL_KEEP);
        }
    }
    if (kind == LS_FACILITIES) {
        if (b->bld[B_ORBITAL] > 0 || building_active(b, B_ORBITAL)) {
            int ox = x + w - 22 + (int)(3 * sin(t * 0.3));
            spr_draw(&SP_STATION, ox, y + 3, 1, night, x, y, w, h);
            cv_textf(ox + 2, y + 4, RGB(245, 210, 90), COL_KEEP, "%d", b->bld[B_ORBITAL]);
        }
        if (b->bld[B_SPACE_DOCK] > 0 || building_active(b, B_SPACE_DOCK)) {
            int ox = x + w - 40 + (int)(2 * cos(t * 0.25));
            spr_draw(&SP_DOCK, ox, y + 2, 1, night, x, y, w, h);
            cv_textf(ox + 4, y + 3, RGB(245, 210, 90), COL_KEEP, "%d", b->bld[B_SPACE_DOCK]);
        }
    }
    for (int i = 0; i < nL; i++) {
        int id = L[i].id;
        int lvl = b->bld[id];
        int act = building_active(b, id);
        int bx = x + L[i].dx;
        if (lvl <= 0 && !act) {
            if (hl == id) cv_text(bx + 1, gy + 2, "vazio", RGB(120, 130, 150), COL_KEEP);
            continue;
        }
        int tier = tier_of(lvl);
        if (id == B_HABITAT) {
            int nt = lvl <= 0 ? 0 : 1 + lvl / 3;
            if (nt > 5) nt = 5;
            for (int k = 0; k < nt; k++) {
                int th = 1 + (int)(hash_unit(sd + (uint64_t)k) * 3) + lvl / 6;
                if (th > skyh - 2) th = skyh - 2;
                spr_draw(&SP_TOWER, bx + k * 4, gy, th, night, x, y, w, h);
            }
        } else if (lvl > 0) {
            if (skyh < spr_height(L[i].spr, tier) + 1) tier = 1;
            spr_draw(L[i].spr, bx, gy, tier, night, x, y, w, h);
        }
        if (act) {
            if (lvl <= 0) spr_draw(&SP_SCAF, bx, gy, 1, night, x, y, w, h);
            int crane_y = gy - (lvl > 0 ? spr_height(L[i].spr, tier) : 4) - 1;
            if (crane_y < y) crane_y = y;
            int blink = ((int)(t * 2)) & 1;
            cv_text(bx + 1, crane_y, "_|", RGB(250, 220, 90), COL_KEEP);
            cv_put(bx + 3, crane_y, blink ? '*' : '.', blink ? RGB(255, 80, 60) : RGB(120, 40, 30), COL_KEEP);
        }
        char tag[16];
        snprintf(tag, sizeof tag, act ? "%d>%d" : "%d", lvl, lvl + 1);
        uint32_t fg = hl == id ? RGB(20, 20, 30) : RGB(250, 215, 90);
        uint32_t bg = hl == id ? RGB(250, 215, 90) : COL_KEEP;
        cv_text(bx + 1, gy + 2, tag, fg, bg);
    }
    cv_reset_clip();
}

/* ======================= SHIPS ======================= */
typedef struct { const char *a[3]; const char *m[3]; } ShipArt;
static const ShipArt SHIPS[N_SHIP] = {
    /* SC */ {{"  __     ", "<[==]>   ", "  ''     "}, {"  aa     ", "oaaaao   ", "  aa     "}},
    /* LC */ {{"  ______    ", "<[=|==|=]>  ", "  ''''''    "}, {"  aaaaaa    ", "oaabaabaao  ", "  aaaaaa    "}},
    /* LF */ {{" \\      ", "==>-    ", " /      "}, {" a      ", "ooaa    ", " a      "}},
    /* HF */ {{" \\\\     ", "=#=>-   ", " //     "}, {" aa     ", "oaaaa   ", " aa     "}},
    /* CR */ {{"  _/\\_     ", "-=<##>==>  ", "   \\/      "}, {"  aaaa     ", "ooaddaaaa  ", "   aa      "}},
    /* BS */ {{"   ___/\\__    ", "<==[####]==>  ", "   '''\\/''    "}, {"   aaaaaaa    ", "ooaaddddaaaa  ", "   aaaaaaa    "}},
    /* BC */ {{" __/\\__      ", ">>=[##]=>>   ", "   \\/        "}, {" aaaaaa      ", "ooaaddaaaa   ", "   aa        "}},
    /* BO */ {{"  _____     ", "[=#OO#=]    ", "  '''''     "}, {"  aaaaa     ", "aarrrraa    ", "  aaaaa     "}},
    /* DE */ {{"  __/^^\\__    ", "<<=[####]=>>  ", "   '\\vv/'     "}, {"  aaaaaaaa    ", "ooaaddddaaaa  ", "   aaaaaa     "}},
    /* DS */ {{"  .----.  ", " ( (@)  ) ", "  '----'  "}, {"  dddddd  ", " d dmd  d ", "  dddddd  "}},
    /* RE */ {{"  _}-\\     ", "}=[##]=>   ", "  ~}-/     "}, {"  aaaa     ", "aaaddaa    ", "  aaaa     "}},
    /* PF */ {{"   _      ", "~=[o]>    ", "   '      "}, {"   a      ", "ooaya     ", "   a      "}},
    /* CO */ {{"  _____    ", "(=[(@)]=)  ", "  '''''    "}, {"  aaaaa    ", "aaagggaaa  ", "  aaaaa    "}},
    /* RC */ {{"   __      ", "<[##]>-o   ", "   ''      "}, {"   aa      ", "aaddaaay   ", "   aa      "}},
    /* PR */ {{"     ", "-o-  ", "     "}, {"     ", "aca  ", "     "}},
    /* SA */ {{"     ", "=[]= ", "     "}, {"     ", "bllb ", "     "}},
    /* CW */ {{" ___ ", "[###]", "o-o-o"}, {" ooo ", "aoooa", "dddddd"}},
};

int art_ship_width(int id) {
    if (!IS_SHIP(id)) return 6;
    return utf8_len(SHIPS[SI(id)].a[1]);
}

void art_ship(int x, int y, int id, double t) {
    if (IS_SHIP(id)) {
        const ShipArt *s = &SHIPS[SI(id)];
        for (int r = 0; r < 3; r++) {
            const char *a = s->a[r], *m = s->m[r];
            for (int i = 0; *a; i++) {
                uint32_t c = utf8_next(&a);
                uint32_t mc = *m ? utf8_next(&m) : 'a';
                if (c == ' ') continue;
                uint32_t col = mask_col((char)mc, 0);
                if (mc == 'o') col = col_scale(col, 0.7 + 0.3 * sin(t * 8 + i));
                cv_put(x + i, y + r, c, col, COL_KEEP);
            }
        }
        return;
    }
    /* defenses */
    static const char *DA[N_DEF][3] = {
        {"  /|  ", " /_|  ", "[___] "}, {"  ==- ", " [o]  ", "[___] "}, {" ===- ", " [O]  ", "[___] "},
        {" ====>", "[#O#] ", "[___] "}, {"  ~~  ", " (@)  ", "[___] "}, {" >==* ", "[@@@] ", "[___] "},
        {" .--. ", "(    )", "      "}, {".----.", "(    )", "'----'"}, {"  ^   ", "  |   ", " [=]  "}, {"  ^   ", " /#\\  ", " [=]  "}};
    static const char *DM[N_DEF][3] = {
        {"  ar  ", " aar  ", "ddddd "}, {"  ccc ", " aya  ", "ddddd "}, {" cccc ", " aya  ", "ddddd "},
        {" bbbbb", "aaoaa ", "ddddd "}, {"  cc  ", " aca  ", "ddddd "}, {" mmmmy", "ammma ", "ddddd "},
        {" bbbb ", "b    b", "      "}, {"bbbbbb", "b    b", "bbbbbb"}, {"  w   ", "  a   ", " ddd  "}, {"  r   ", " ara  ", " ddd  "}};
    if (!IS_DEF(id)) return;
    int k = DI(id);
    for (int r = 0; r < 3; r++) {
        const char *a = DA[k][r], *m = DM[k][r];
        for (int i = 0; *a; i++) {
            uint32_t c = utf8_next(&a);
            uint32_t mc = *m ? utf8_next(&m) : 'a';
            if (c == ' ') continue;
            cv_put(x + i, y + r, c, mask_col((char)mc, 0), COL_KEEP);
        }
    }
}

/* ======================= LOGO ======================= */
static const char *GLYPH(char c, int row) {
    static const char *I[4] = {" ___ ", "|_ _|", " | | ", "|___|"};
    static const char *M[4] = {" __  __ ", "|  \\/  |", "| |\\/| |", "|_|  |_|"};
    static const char *P[4] = {" ___ ", "| _ \\", "|  _/", "|_|  "};
    static const char *E[4] = {" ___ ", "| __|", "| _| ", "|___|"};
    static const char *R[4] = {" ___ ", "| _ \\", "|   /", "|_|_\\"};
    static const char *O[4] = {"  ___  ", " / _ \\ ", "| (_) |", " \\___/ "};
    static const char *S[4] = {" ___ ", "/ __|", "\\__ \\", "|___/"};
    static const char *T[4] = {" _____ ", "|_   _|", "  | |  ", "  |_|  "};
    static const char *L[4] = {" _    ", "| |   ", "| |__ ", "|____|"};
    static const char *A[4] = {"   _   ", "  /_\\  ", " / _ \\ ", "/_/ \\_\\"};
    static const char *SP[4] = {"   ", "   ", "   ", "   "};
    switch (c) {
    case 'I': return I[row]; case 'M': return M[row]; case 'P': return P[row]; case 'E': return E[row];
    case 'R': return R[row]; case 'O': return O[row]; case 'S': return S[row]; case 'T': return T[row];
    case 'L': return L[row]; case 'A': return A[row]; default: return SP[row];
    }
}

void art_logo(int x, int y, double t) {
    const char *word = "IMPERIO ESTELAR";
    cv_fill(x - 1, y - 1, 86, 6, ' ', RGB(0, 0, 0), RGB(0, 0, 0));
    for (int row = 0; row < 4; row++) {
        int cx = x;
        for (const char *p = word; *p; p++) {
            const char *gl = GLYPH(*p, row);
            int n = (int)strlen(gl);
            for (int i = 0; i < n; i++) {
                if (gl[i] == ' ') continue;
                double k = 0.5 + 0.5 * sin(t * 1.5 - (cx + i) * 0.08 + row * 0.3);
                uint32_t col = col_lerp(RGB(110, 165, 245), RGB(255, 215, 100), k);
                cv_put(cx + i, y + row, (uint32_t)(unsigned char)gl[i], col, COL_KEEP);
            }
            cx += n;
        }
    }
    /* accent over the E of IMPERIO */
    cv_put(x + 5 + 8 + 5 + 2, y - 1, '/', RGB(250, 220, 120), COL_KEEP);
}

void art_d20(int x, int y, int v, uint32_t col) {
    static const char *D[7] = {"     /\\     ", "    /  \\    ", "   /    \\   ", "  /      \\  ", "  \\------/  ", "   \\    /   ", "    \\__/    "};
    for (int r = 0; r < 7; r++) cv_text(x, y + r, D[r], col, COL_KEEP);
    char b[8];
    snprintf(b, sizeof b, "%d", v);
    cv_text(x + 6 - (int)strlen(b) / 2, y + 3, b, RGB(255, 255, 255), COL_KEEP);
}

void art_officer(int x, int y, int off) {
    static const char *HAT[N_OFF] = {" _===_ ", " /###\\ ", " [###] ", " (~~~) ", " <***> "};
    static const char *F[2] = {" |o o| ", " |\\_/| "};
    static const uint32_t C[N_OFF] = {RGB(250, 215, 90), RGB(120, 170, 255), RGB(245, 150, 60), RGB(150, 110, 70), RGB(200, 130, 235)};
    cv_text(x, y, HAT[off], C[off], COL_KEEP);
    for (int r = 0; r < 2; r++) cv_text(x, y + 1 + r, F[r], RGB(230, 200, 170), COL_KEEP);
}
