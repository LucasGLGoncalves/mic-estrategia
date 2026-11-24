/* main.c - entry point: title/registration, main loop, CLI tools (selftest, simulate, snapshot) */
#include "art.h"
#include "game.h"
#include "term.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int run_selftest(int verbose);

static Game *G;

static const int SPEEDS[] = {1, 2, 5, 10, 25, 50, 100};
#define NSPEEDS 7

/* ---------------- registration ---------------- */
static void draw_registration(const char *name, const char *home, int field, int sp, double t, const char *err) {
        cv_clear(RGB(0, 0, 0));
        art_starfield(0, 0, CANVAS_W, CANVAS_H, 0x5EED, t, RGB(0, 0, 0));
        art_logo(18, 3, t);
        cv_text_c(60, 8, "um império espacial em gráficos ASCII", C_DIM, COL_KEEP);
        int bx = 30, by = 11, bw = 60, bh = 19;
        cv_fill(bx, by, bw, bh, ' ', C_TEXT, C_PANEL);
        cv_box(bx, by, bw, bh, C_BORDER, C_PANEL, 1);
        cv_text_c(60, by, " Fundar novo império ", C_GOLD, C_PANEL);
        const char *labels[3] = {"Nome do comandante", "Nome do planeta natal", "Velocidade do universo"};
        for (int f = 0; f < 3; f++) {
            int y = by + 2 + f * 3;
            cv_text(bx + 3, y, labels[f], field == f ? C_GOLD : C_TEXT, C_PANEL);
            cv_fill(bx + 3, y + 1, bw - 6, 1, ' ', C_WHITE, field == f ? RGB(30, 45, 75) : RGB(8, 12, 22));
            if (f == 0) cv_text(bx + 4, y + 1, name, C_WHITE, COL_KEEP);
            if (f == 1) cv_text(bx + 4, y + 1, home, C_WHITE, COL_KEEP);
            if (f == 2) {
                char s[64];
                snprintf(s, sizeof s, "◄  x%d  ►", SPEEDS[sp]);
                cv_text(bx + 4, y + 1, s, C_WHITE, COL_KEEP);
                cv_text(bx + 22, y + 1, SPEEDS[sp] == 1 ? "clássico, partidas de meses" : SPEEDS[sp] <= 5 ? "ritmo tranquilo" : SPEEDS[sp] <= 10 ? "recomendado" : SPEEDS[sp] <= 25 ? "acelerado" : "frenético", C_DIM, COL_KEEP);
            }
            if (field == f && f < 2 && (((int)(t * 3)) & 1)) cv_put(bx + 4 + utf8_len(f == 0 ? name : home), y + 1, '_', C_GOLD, COL_KEEP);
        }
        int y = by + 11;
        cv_fill(bx + 18, y, 24, 1, ' ', C_TEXT, field == 3 ? C_GOLD : C_PANEL2);
        cv_text_c(60, y, "[ FUNDAR IMPÉRIO ]", field == 3 ? RGB(20, 20, 30) : C_TEXT, COL_KEEP);
        cv_wrap(bx + 3, by + 13, bw - 6, 4,
                "O universo nasce de uma semente feita da data e hora exatas (ano, dia, mês, minuto, hora, segundo) somadas às letras do seu nome. "
                "Cada império é único.", C_DIM, C_PANEL);
        if (err[0]) cv_text_c(60, by + bh - 1, err, C_RED, C_PANEL);
        cv_text_c(60, CANVAS_H - 2, "↑↓/Tab campos   ←→ velocidade   Enter avançar   Esc sair", C_DIM, COL_KEEP);
}

static int registration(Game *g) {
    char name[32] = "", home[32] = "Planeta Natal";
    int field = 0, sp = 3;
    double t0 = now_seconds();
    char err[80] = "";
    for (;;) {
        double t = now_seconds() - t0;
        draw_registration(name, home, field, sp, t, err);
        cv_present();
        term_poll(60);
        int k;
        while ((k = term_key()) != 0) {
            if (k == K_RESIZE) { term_force_redraw(); continue; }
            if (k == K_ESC || k == K_CLOSE || k == K_CTRL_C) return 0;
            if (k == K_UP) field = (field + 3) % 4;
            else if (k == K_DOWN || k == K_TAB) field = (field + 1) % 4;
            else if (field == 2 && k == K_LEFT) sp = (sp + NSPEEDS - 1) % NSPEEDS;
            else if (field == 2 && k == K_RIGHT) sp = (sp + 1) % NSPEEDS;
            else if (k == K_ENTER) {
                if (field < 3) { field++; continue; }
                char nm[32];
                snprintf(nm, sizeof nm, "%s", name);
                while (nm[0] == ' ') memmove(nm, nm + 1, strlen(nm));
                if (!nm[0]) { snprintf(err, sizeof err, "Digite o nome do comandante"); field = 0; continue; }
                char seedstr[48];
                make_seedstr(seedstr, sizeof seedstr, now_seconds());
                game_new(g, nm, home[0] ? home : "Planeta Natal", SPEEDS[sp], seedstr);
                return 1;
            } else if (k == K_BS) {
                if (field == 0) utf8_pop(name);
                else if (field == 1) utf8_pop(home);
            } else if (k >= 32 && k < 0x110000 && field < 2) {
                char *dst = field == 0 ? name : home;
                if (utf8_len(dst) < 20) utf8_append(dst, 32, (uint32_t)k);
            }
        }
    }
}

static void draw_reveal(Game *g, double t) {
    Body *b = &g->body[0];
        cv_clear(RGB(0, 0, 0));
        art_starfield(0, 0, CANVAS_W, CANVAS_H, g->seed, t, RGB(0, 0, 0));
        cv_text_c(60, 1, "A semente do seu universo", C_GOLD, COL_KEEP);
        const char *s = g->seedstr;
        char line[200];
        snprintf(line, sizeof line, "ano %.4s  dia %.2s  mês %.2s  minuto %.2s  hora %.2s  segundo %.2s   +   \"%s\"", s, s + 4, s + 6, s + 8, s + 10, s + 12, g->name);
        cv_text_c(60, 3, line, C_CYAN, COL_KEEP);
        snprintf(line, sizeof line, "= semente 0x%016llX", (unsigned long long)g->seed);
        cv_text_c(60, 4, line, C_PURPLE, COL_KEEP);
        int reveal_t = (int)(t * 12);
        art_planet(4, 6, 60, 28, &b->info, t, 0.15, b->info.vseed, b->moon >= 0, 0, 0, 0);
        int x = 68, y = 7;
        ui_panel(x - 2, y - 1, 52, 27, "Planeta natal");
        char v[96], a[32];
#define L(lbl, ...) do { if (reveal_t > (y - 7) * 2) { snprintf(v, sizeof v, __VA_ARGS__); cv_text(x, y, lbl, C_DIM, C_PANEL); cv_text(x + 15, y, v, C_WHITE, C_PANEL); } y++; } while (0)
        L("Nome:", "%s", b->name);
        L("Posição:", "[%d:%d:%d]", b->g, b->s, b->p);
        fmt_num(a, sizeof a, b->info.diameter);
        L("Diâmetro:", "%s km (%d campos)", a, b->info.fields);
        L("Temperatura:", "%d°C a %d°C", b->info.tmin, b->info.tmax);
        L("Bioma:", "%s", BIOME_NAME[b->info.biome]);
        L("Atmosfera:", "%s", ATMO_NAME[b->info.atmo]);
        L("Pressão:", "%.2f atm", b->info.pressure);
        L("Clima:", "%s", climate_text(&b->info));
        L("Gravidade:", "%.2f g", b->info.gravity);
        L("Dia:", "%.1f horas", b->info.daylen);
        L("Água:", "%.0f%% da superfície", b->info.water * 100);
        L("Fertilidade:", "%.0f%%", b->info.fertility * 100);
        L("Radiação:", "%s", RAD_NAME[b->info.radiation]);
        L("Magnetosfera:", "%s", MAG_NAME[b->info.magneto]);
        L("Metal:", "%+.0f%% de riqueza", (b->info.rich[0] - 1) * 100);
        L("Cristal:", "%+.0f%% de riqueza", (b->info.rich[1] - 1) * 100);
        L("Deutério:", "%+.0f%% de riqueza", (b->info.rich[2] - 1) * 100);
        L("Lua:", "%s", b->moon >= 0 ? "sim" : "não");
        y++;
        L("Metal:", "1.000");
        L("Cristal:", "1.000");
        L("Deutério:", "200");
        L("População:", "210");
        L("Matéria Negra:", "8.000");
#undef L
        cv_text_c(60, CANVAS_H - 1, "Enter para assumir o comando", ((int)(t * 2)) & 1 ? C_GOLD : C_DIM, COL_KEEP);
}

static int reveal(Game *g) {
    double t0 = now_seconds();
    for (;;) {
        double t = now_seconds() - t0;
        draw_reveal(g, t);
        cv_present();
        term_poll(50);
        int k;
        while ((k = term_key()) != 0) {
            if (k == K_RESIZE) term_force_redraw();
            else if (k == K_CLOSE) return 0;
            else if (k == K_ENTER || k == ' ' || k == K_ESC) return 1;
        }
    }
}

static int title_menu(const char *name) {
    int sel = 0;
    double t0 = now_seconds();
    for (;;) {
        double t = now_seconds() - t0;
        cv_clear(RGB(0, 0, 0));
        art_starfield(0, 0, CANVAS_W, CANVAS_H, 0x5EED, t, RGB(0, 0, 0));
        art_logo(18, 5, t);
        cv_text_c(60, 10, "um império espacial em gráficos ASCII", C_DIM, COL_KEEP);
        char cont[96];
        snprintf(cont, sizeof cont, "Continuar - Comandante %s", name);
        const char *items[3] = {cont, "Novo império (substitui o atual)", "Sair"};
        for (int i = 0; i < 3; i++) {
            uint32_t bg = sel == i ? C_SEL : RGB(0, 0, 0);
            cv_fill(35, 15 + i * 2, 50, 1, ' ', C_TEXT, bg);
            cv_text_c(60, 15 + i * 2, items[i], sel == i ? C_GOLD : C_TEXT, bg);
        }
        cv_text_c(60, CANVAS_H - 2, "versão " GAME_VERSION "  •  ↑↓ Enter", C_DIM, COL_KEEP);
        cv_present();
        term_poll(60);
        int k;
        while ((k = term_key()) != 0) {
            if (k == K_RESIZE) term_force_redraw();
            else if (k == K_UP) sel = (sel + 2) % 3;
            else if (k == K_DOWN) sel = (sel + 1) % 3;
            else if (k == K_ENTER) return sel;
            else if (k == K_ESC || k == K_CLOSE || k == K_CTRL_C) return 2;
        }
    }
}

static void away_report(UI *u, Game *g, double away) {
    char t[48];
    fmt_time(t, sizeof t, away);
    char a[32], b[32], c[32], d[32];
    fmt_num(a, sizeof a, g->away_gain[0]); fmt_num(b, sizeof b, g->away_gain[1]);
    fmt_num(c, sizeof c, g->away_gain[2]); fmt_num(d, sizeof d, g->away_gain[3]);
    u->modal = MD_AWAY;
    snprintf(u->mtitle, sizeof u->mtitle, "Bem-vindo de volta, Comandante");
    snprintf(u->mtext, sizeof u->mtext,
             "Você esteve fora por \001y%s\001d. O império continuou trabalhando:\n\n"
             "  Metal      \001g+%s\001d\n  Cristal    \001g+%s\001d\n  Deutério   \001g+%s\001d\n  Alimento   %s%s\001d\n\n"
             "Eventos durante a ausência: \001y%d\001d\nMensagens não lidas: \001y%d\001d (tecla M)\n\n"
             "Construções, pesquisas, frotas e rotas comerciais foram processadas na ordem em que terminariam.",
             t, a, b, c, g->away_gain[3] >= 0 ? "\001g+" : "\001r", d, g->away_events, game_unread(g));
}

/* ---------------- headless tools ---------------- */
static void sim_print(Game *g, double hours) {
    double pb, pr, pf, pd;
    double p = player_points(g, &pb, &pr, &pf, &pd);
    int tot;
    int rk = player_rank(g, &tot);
    Body *b = &g->body[0];
    Rates r;
    body_rates(g, b, &r);
    int cols = colonies_count(g);
    printf("[campos %d/%d M%.0f C%.0f D%.0f fila%d %s%s] ", body_fields_used(b), body_fields_max(g, b), b->res[0], b->res[1], b->res[2], b->bq.n,
           b->bq.n ? ITEMS[b->bq.it[0].item].shortn : "", b->bq.n && !b->bq.it[0].active ? "(esperando)" : "");
    printf("%7.1fh pts=%9.0f (b%.0f r%.0f f%.0f d%.0f) rank %4d/%d  M%2d C%2d D%2d S%2d F%2d Lab%2d Hab%2d Farm%2d pop %6.0f/%6.0f wf%.2f ee%.2f food %+8.0f/h  col %d ev %lld cmb %.0f/%.0f msgs %d dm %.0f\n",
           hours, p, pb, pr, pf, pd, rk, tot, b->bld[B_METAL_MINE], b->bld[B_CRYSTAL_MINE], b->bld[B_DEUT_SYNTH], b->bld[B_SOLAR_PLANT],
           b->bld[B_FUSION], b->bld[B_LAB], b->bld[B_HABITAT], b->bld[B_FARM], b->pop, r.pop_cap, r.wf, r.e_eff, r.prod[RES_FOOD], cols,
           g->ev.total, g->stat_wins, g->stat_combats, g->nmsg, g->dm);
    fflush(stdout);
}

static int simulate(double hours, int speed, const char *name, int verbose) {
    Game *g = (Game *)calloc(1, sizeof(Game));
    game_new(g, name, "Planeta Natal", speed, "20262411591403");
    double start = g->now;
    double end = start + hours * 3600.0;
    double next_print = start;
    while (g->now < end) {
        double step = 60.0;
        game_advance(g, g->now + step, 0);
        bot_step(g);
        g->ntoast = 0;
        if (verbose && g->now >= next_print) { sim_print(g, (g->now - start) / 3600.0); next_print += hours * 3600.0 / 24.0; }
    }
    sim_print(g, (g->now - start) / 3600.0);
    /* sanity */
    int bad = 0;
    for (int i = 0; i < g->nbody; i++) {
        Body *b = &g->body[i];
        if (!b->used) continue;
        for (int k = 0; k < NRES; k++) if (!(b->res[k] >= 0) || isnan(b->res[k]) || isinf(b->res[k])) { printf("BAD res body %d k %d = %f\n", i, k, b->res[k]); bad = 1; }
        if (!(b->pop >= 0) || isnan(b->pop)) { printf("BAD pop %d\n", i); bad = 1; }
        for (int k = 0; k < N_SHIP; k++) if (b->ship[k] < 0) { printf("BAD ship %d %d\n", i, k); bad = 1; }
        for (int k = 0; k < N_DEF; k++) if (b->def[k] < 0) { printf("BAD def %d %d\n", i, k); bad = 1; }
    }
    if (verbose) {
        int def_w = 0, def_l = 0, exp_n = 0, dec_n = 0, amb = 0;
        for (int i = 0; i < g->nmsg; i++) {
            if (!strncmp(g->msg[i].title, "Defesa de", 9)) { if (strstr(g->msg[i].title, "vitória")) def_w++; else def_l++; }
            if (!strncmp(g->msg[i].title, "Expedição", 10)) exp_n++;
            if (g->msg[i].type == MSG_EVENT) dec_n++;
            if (!strncmp(g->msg[i].title, "Emboscada", 9)) amb++;
        }
        printf("(últimas %d msgs) defesas: %d vitórias / %d derrotas, expedições %d, eventos %d, emboscadas %d\n", g->nmsg, def_w, def_l, exp_n, dec_n, amb);
        printf("--- últimas mensagens ---\n");
        for (int i = g->nmsg - 12 < 0 ? 0 : g->nmsg - 12; i < g->nmsg; i++) printf("  [%d] %s\n", g->msg[i].type, g->msg[i].title);
    }
    universe_free(g);
    free(g);
    return bad;
}

static int snapshot(const char *dir, double hours, int speed) {
    Game *g = (Game *)calloc(1, sizeof(Game));
    game_new(g, "Lucas", "Homeworld", speed, "20262411591403");
    double start = g->now;
    while (g->now < start + hours * 3600.0) { game_advance(g, g->now + 60.0, 0); bot_step(g); }
    g->ntoast = 0;
    UI *u = (UI *)calloc(1, sizeof(UI));
    ui_init(u, g);
    u->t = 12.3;
    static const char *names[SCR_COUNT] = {"overview", "resources", "life", "facilities", "research", "shipyard", "defense", "fleet",
                                           "galaxy", "empire", "trade", "merchant", "messages", "tech", "officers", "ranking", "sim", "help"};
    char path[512];
    for (int s = 0; s < SCR_COUNT; s++) {
        u->screen = s;
        if (s == SCR_TECH) { u->tech_item = S_CRUISER; u->tab[SCR_TECH] = 0; }
        ui_draw(u, g);
        snprintf(path, sizeof path, "%s/%s.cells", dir, names[s]);
        FILE *f = fopen(path, "w");
        if (f) { cv_dump_cells(f); fclose(f); }
    }
    /* extra screens */
#define DUMP(nm) do { snprintf(path, sizeof path, "%s/%s.cells", dir, nm); FILE *ff = fopen(path, "w"); if (ff) { cv_dump_cells(ff); fclose(ff); } } while (0)
    draw_registration("Lucas", "Homeworld", 0, 3, 1.0, "");
    DUMP("registration");
    draw_reveal(g, 3.0);
    DUMP("reveal");
    /* moon */
    if (g->body[0].moon < 0) create_moon(g, 0, 6000);
    int mi = g->body[0].moon;
    g->body[mi].bld[B_LUNAR_BASE] = 3; g->body[mi].bld[B_PHALANX] = 1; g->body[mi].bld[B_HE3] = 4; g->body[mi].bld[B_ROBOTICS] = 2;
    g->cur = mi;
    u->screen = SCR_OVERVIEW; ui_draw(u, g); DUMP("moon_overview");
    u->screen = SCR_FACILITIES; ui_draw(u, g); DUMP("moon_facilities");
    g->cur = 0;
    /* event modal: choose then rolled */
    {
        Decision *d = &g->ev.dec;
        memset(d, 0, sizeof *d);
        d->active = 1; d->type = EV_EARTHQUAKE; d->body = 0; d->nopt = 3; d->dc = 13; d->base_mod = 4;
        snprintf(d->title, sizeof d->title, "Terremoto em Homeworld!");
        snprintf(d->text, sizeof d->text, "Sismógrafos registram um tremor de magnitude 7.4 sob Homeworld. Estruturas podem desabar e minas podem perder níveis.");
        snprintf(d->mod_text, sizeof d->mod_text, "Geofísica 2 + Robôs/2 2 + Nanitas×2 0");
        snprintf(d->opt[0], sizeof d->opt[0], "Reforçar estruturas (+5)"); d->optcost[0][0] = 9000; d->optbonus[0] = 5;
        snprintf(d->opt[1], sizeof d->opt[1], "Evacuar: minas a 30%% por 2h (sem teste)"); d->optbonus[1] = -1000;
        snprintf(d->opt[2], sizeof d->opt[2], "Arriscar (teste puro)");
        d->deadline = g->now + 150;
        u->screen = SCR_OVERVIEW; u->modal = MD_EVENT;
        ui_draw(u, g); DUMP("event_choose");
        ui_key(u, g, '3');
        u->t += 3.0;
        ui_draw(u, g); DUMP("event_rolled");
        ui_key(u, g, K_ENTER);
        u->modal = MD_NONE;
        g->ev.dec.active = 0;
    }
    /* dispatch wizard */
    g->body[0].ship[SI(S_LARGE_CARGO)] += 5;
    u->screen = SCR_FLEET;
    ui_key(u, g, K_ENTER); ui_key(u, g, K_DOWN); ui_key(u, g, '3');
    ui_draw(u, g); DUMP("dispatch1");
    ui_key(u, g, K_ENTER); ui_draw(u, g); DUMP("dispatch2");
    ui_key(u, g, K_ENTER); ui_key(u, g, K_DOWN); ui_key(u, g, K_DOWN); ui_key(u, g, '5'); ui_key(u, g, '0'); ui_key(u, g, '0'); ui_key(u, g, '0');
    ui_draw(u, g); DUMP("dispatch3");
    ui_key(u, g, K_ESC); ui_key(u, g, K_ESC); ui_key(u, g, K_ESC);
    u->screen = SCR_TRADE; u->tab[SCR_TRADE] = 1; ui_draw(u, g); DUMP("markets");
    u->screen = SCR_OVERVIEW;
    ui_info(u, "Relatório", "Teste de janela de informação com \001gcor\001d e texto longo para quebrar linhas automaticamente dentro da caixa.");
    ui_draw(u, g); DUMP("modal_info");
    u->modal = MD_NONE;
#undef DUMP
    /* extra: techinfo tab and an event modal */
    u->screen = SCR_TECH; u->tech_item = B_METAL_MINE; u->tab[SCR_TECH] = 2;
    ui_draw(u, g);
    snprintf(path, sizeof path, "%s/techinfo.cells", dir);
    FILE *f = fopen(path, "w");
    if (f) { cv_dump_cells(f); fclose(f); }
    universe_free(g);
    free(u);
    free(g);
    return 0;
}

static int gallery(const char *out, double t) {
    cv_clear(RGB(0, 0, 0));
    for (int b = 0; b < BIO_COUNT; b++) {
        PlanetInfo pi;
        planet_generate(0xC0FFEE + (uint64_t)b * 31, 1, 100 + b, 7, 0, &pi);
        pi.biome = b;
        pi.water = b == BIO_OCEANIC ? 0.88 : b == BIO_TEMPERATE ? 0.6 : b == BIO_JUNGLE ? 0.4 : b == BIO_TOXIC ? 0.45 : b == BIO_SAVANNA ? 0.25 : b == BIO_TUNDRA ? 0.3 : 0.0;
        pi.atmo = b == BIO_BARREN || b == BIO_MOON ? ATM_NONE : b == BIO_VOLCANIC ? ATM_SULFUR : b == BIO_TOXIC ? ATM_AMMONIA : ATM_N2O2;
        pi.pressure = pi.atmo == ATM_NONE ? 0 : 1.2;
        pi.ring = b == BIO_TOXIC;
        int x = (b % 4) * 30, y = (b / 4) * 12;
        art_planet(x, y, 30, 12, &pi, t, b == BIO_TEMPERATE ? 0.5 : 0.0, pi.vseed, b == BIO_OCEANIC, b == BIO_ICE ? 3 : 0, b == BIO_DESERT, 1);
        cv_text(x + 1, y, BIOME_NAME[b], RGB(250, 220, 90), COL_KEEP);
    }
    FILE *f = fopen(out, "w");
    if (!f) return 1;
    cv_dump_cells(f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv) {
    TermOpts o;
    memset(&o, 0, sizeof o);
    o.colormode = CM_256;
#ifdef _WIN32
    o.colormode = CM_TRUE;
#else
    const char *ct = getenv("COLORTERM");
    if (ct && (strstr(ct, "truecolor") || strstr(ct, "24bit"))) o.colormode = CM_TRUE;
#endif
    double sim_hours = 0, snap_hours = 6;
    int speed = 10, selftest = 0;
    const char *snapdir = NULL, *simname = "Bot";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--ascii")) o.ascii = 1;
        else if (!strcmp(argv[i], "--256")) o.colormode = CM_256;
        else if (!strcmp(argv[i], "--truecolor")) o.colormode = CM_TRUE;
        else if (!strcmp(argv[i], "--selftest")) selftest = 1;
        else if (!strcmp(argv[i], "--simulate") && i + 1 < argc) sim_hours = atof(argv[++i]);
        else if (!strcmp(argv[i], "--speed") && i + 1 < argc) speed = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--name") && i + 1 < argc) simname = argv[++i];
        else if (!strcmp(argv[i], "--snapshot") && i + 1 < argc) snapdir = argv[++i];
        else if (!strcmp(argv[i], "--hours") && i + 1 < argc) snap_hours = atof(argv[++i]);
        else if (!strcmp(argv[i], "--gallery") && i + 1 < argc) { o.headless = 1; term_init(&o); const char *out = argv[++i]; return gallery(out, 10.0); }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("Império Estelar %s\n  --ascii       só caracteres ASCII (terminais antigos)\n  --256         256 cores\n  --truecolor   cores de 24 bits\n"
                   "  --selftest    executa os testes automáticos\n  --simulate H  simula H horas reais com o piloto automático\n  --speed N     velocidade (com --simulate)\n",
                   GAME_VERSION);
            return 0;
        }
    }
    if (selftest) return run_selftest(1);
    if (sim_hours > 0) return simulate(sim_hours, speed, simname, 1);
    if (snapdir) {
        o.headless = 1;
        term_init(&o);
        return snapshot(snapdir, snap_hours, speed);
    }
    if (!term_init(&o)) return 1;
    G = (Game *)calloc(1, sizeof(Game));
    UI *u = (UI *)calloc(1, sizeof(UI));
    char sname[64];
    int have = save_exists(SAVE_FILE, sname, sizeof sname);
    int started = 0;
    double away = 0;
    if (have) {
        int c = title_menu(sname);
        if (c == 2) { term_shutdown(); return 0; }
        if (c == 0) {
            if (game_load(G, SAVE_FILE)) {
                started = 1;
                double now = now_seconds();
                away = now - G->now;
                memset(G->away_gain, 0, sizeof G->away_gain);
                G->away_events = 0;
                if (away > 5) {
                    cv_clear(RGB(0, 0, 0));
                    cv_text_c(60, 17, "Calculando o que aconteceu enquanto você esteve fora...", C_GOLD, RGB(0, 0, 0));
                    cv_present();
                    game_advance(G, now, 1);
                }
                G->ntoast = 0;
            } else {
                cv_clear(RGB(0, 0, 0));
                cv_text_c(60, 17, "Não foi possível ler o arquivo salvo. Um novo império será criado.", C_RED, RGB(0, 0, 0));
                cv_present();
                sleep_ms(2500);
            }
        }
    }
    if (!started) {
        if (!registration(G)) { term_shutdown(); return 0; }
        if (!reveal(G)) { game_save(G, SAVE_FILE); term_shutdown(); return 0; }
        game_save(G, SAVE_FILE);
    }
    ui_init(u, G);
    if (away > 60) away_report(u, G, away);
    double t0 = now_seconds();
    double last_save = t0;
    while (!u->quit) {
        term_poll(45);
        double now = now_seconds();
        game_advance(G, now, 0);
        u->t = now - t0;
        int k;
        while ((k = term_key()) != 0) {
            if (k == K_RESIZE) { term_force_redraw(); continue; }
            ui_key(u, G, k);
        }
        if (g_term_close_req) {
            G->last_seen = G->now;
            game_save(G, SAVE_FILE);
            g_term_close_ack = 1;
            break;
        }
        ui_draw(u, G);
        cv_present();
        if (now - last_save > 30) { G->last_seen = G->now; game_save(G, SAVE_FILE); last_save = now; }
    }
    G->last_seen = G->now;
    int ok = game_save(G, SAVE_FILE);
    term_shutdown();
    printf("%s\n", ok ? "Império salvo. Até a próxima, Comandante!" : "ATENÇÃO: não foi possível salvar o jogo.");
    universe_free(G);
    free(G);
    free(u);
    return 0;
}
