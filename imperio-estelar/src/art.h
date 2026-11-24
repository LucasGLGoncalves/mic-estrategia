/* art.h - ASCII art renderers */
#ifndef ART_H
#define ART_H
#include "game.h"
#include "term.h"

enum { LS_RESOURCES, LS_FACILITIES, LS_LIFE, LS_MOON };

void art_starfield(int x, int y, int w, int h, uint64_t seed, double t, uint32_t bg);
void art_planet(int x, int y, int w, int h, const PlanetInfo *pi, double t, double lights, uint64_t extras_seed,
                int has_moon, int sats, int station, int bg_stars);
void art_planet_disc(double cx, double cy, double R, const PlanetInfo *pi, double t, double lights, int clipx, int clipy, int clipw, int cliph);
void art_landscape(int x, int y, int w, int h, const Game *g, const Body *b, int kind, double t, int highlight_id);
void art_ship(int x, int y, int id, double t);
int  art_ship_width(int id);
uint32_t biome_color(int biome);
uint32_t sky_color(const PlanetInfo *pi);
void art_logo(int x, int y, double t);
void art_d20(int x, int y, int value, uint32_t col);
void art_officer(int x, int y, int off);

#endif
