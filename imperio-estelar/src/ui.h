/* ui.h - user interface state and helpers */
#ifndef UI_H
#define UI_H
#include "game.h"
#include "term.h"

/* theme */
#define C_BG     RGB(8, 11, 20)
#define C_PANEL  RGB(13, 19, 33)
#define C_PANEL2 RGB(20, 29, 48)
#define C_BORDER RGB(55, 82, 125)
#define C_TITLE  RGB(150, 195, 240)
#define C_TEXT   RGB(205, 212, 226)
#define C_DIM    RGB(110, 125, 150)
#define C_GOLD   RGB(245, 205, 90)
#define C_GREEN  RGB(110, 215, 110)
#define C_RED    RGB(235, 85, 70)
#define C_CYAN   RGB(100, 215, 235)
#define C_ORANGE RGB(245, 150, 60)
#define C_PURPLE RGB(200, 130, 235)
#define C_SEL    RGB(42, 66, 106)
#define C_WHITE  RGB(240, 242, 248)

/* layout */
#define CX 20
#define CY 5
#define CW 82
#define CH 31

enum {
    SCR_OVERVIEW, SCR_RESOURCES, SCR_LIFE, SCR_FACILITIES, SCR_RESEARCH, SCR_SHIPYARD, SCR_DEFENSE,
    SCR_FLEET, SCR_GALAXY, SCR_EMPIRE, SCR_TRADE, SCR_MERCHANT, SCR_MESSAGES, SCR_TECH, SCR_OFFICERS,
    SCR_RANKING, SCR_SIM, SCR_HELP, SCR_COUNT
};

enum { MD_NONE, MD_PROMPT, MD_MENU, MD_INFO, MD_CONFIRM, MD_EVENT, MD_AWAY };

/* modal purposes */
enum { MP_NONE, MP_BUILD_QTY, MP_RENAME, MP_ABANDON, MP_GAL_ACTION, MP_GOTO, MP_DELETE_ROUTE, MP_QUIT,
       MP_FINISH_DM, MP_OFFICER, MP_SIM_QTY, MP_MER_AMOUNT, MP_ROUTE_AMOUNT, MP_RECALL, MP_DELETE_MSGS, MP_CANCEL_Q, MP_IPM_QTY, MP_JUMP };

typedef struct {
    int active, step, origin;
    long long sel[N_SHIP];
    int row;
    char num[24];
    int tg, ts, tp, ttype, field;
    int mission, speed;
    double cargo[NRES];
    int frow;
    char err[160];
} Dispatch;

typedef struct {
    int active, step;
    int kind, origin, dest;
    int tg, ts, tp;
    int res_out, res_in;
    double amount;
    long long ship[N_SHIP];
    int row;
    int npl_list[64];
    int nnpl;
    char num[24];
    char err[160];
} RouteWiz;

typedef struct {
    int screen;
    int sel[SCR_COUNT], scroll[SCR_COUNT], tab[SCR_COUNT];
    int modal, mpurpose, marg, marg2;
    char mtitle[96];
    char mtext[1600];
    char minput[64];
    int mnumeric, mmaxlen;
    int mn, msel;
    char mitems[12][96];
    int mok[12];
    int gg, gs;
    int tech_item;
    int msg_scroll;
    Dispatch dp;
    RouteWiz rw;
    long long sim_n[2][N_UNIT];
    int sim_tech[2][3];
    int sim_side;
    char sim_result[900];
    int mer_from, mer_to;
    char toast[160];
    uint32_t toast_col;
    double toast_until;
    double t;
    int quit;
    double dice_t0;
} UI;

void ui_init(UI *u, Game *g);
void ui_draw(UI *u, Game *g);
void ui_key(UI *u, Game *g, int key);
void ui_toast(UI *u, uint32_t col, const char *fmt, ...);

/* shared helpers (ui.c) */
void ui_panel(int x, int y, int w, int h, const char *title);
void ui_bar(int x, int y, int w, double frac, uint32_t col);
void ui_prompt(UI *u, int purpose, int arg, const char *title, const char *text, const char *init, int numeric);
void ui_menu(UI *u, int purpose, int arg, const char *title, int n, const char **items, const int *ok);
void ui_info(UI *u, const char *title, const char *fmt, ...);
void ui_confirm(UI *u, int purpose, int arg, const char *title, const char *fmt, ...);
void cost_str(char *buf, size_t n, const double c[3]);
void draw_cost(int x, int y, const Game *g, const Body *b, const double c[3], double energy);
const char *body_label(const Game *g, int bi, char *buf, size_t n);
int is_global_key(int key);
void ui_open_tech(UI *u, int id);
void ui_goto_screen(UI *u, Game *g, int scr);

/* screens (ui_screens.c) */
void scr_fleet_draw(UI *u, Game *g);
int  scr_fleet_key(UI *u, Game *g, int k);
void scr_galaxy_draw(UI *u, Game *g);
int  scr_galaxy_key(UI *u, Game *g, int k);
void scr_empire_draw(UI *u, Game *g);
int  scr_empire_key(UI *u, Game *g, int k);
void scr_trade_draw(UI *u, Game *g);
int  scr_trade_key(UI *u, Game *g, int k);
void scr_merchant_draw(UI *u, Game *g);
int  scr_merchant_key(UI *u, Game *g, int k);
void scr_messages_draw(UI *u, Game *g);
int  scr_messages_key(UI *u, Game *g, int k);
void scr_tech_draw(UI *u, Game *g);
int  scr_tech_key(UI *u, Game *g, int k);
void scr_officers_draw(UI *u, Game *g);
int  scr_officers_key(UI *u, Game *g, int k);
void scr_ranking_draw(UI *u, Game *g);
int  scr_ranking_key(UI *u, Game *g, int k);
void scr_sim_draw(UI *u, Game *g);
int  scr_sim_key(UI *u, Game *g, int k);
void scr_help_draw(UI *u, Game *g);
int  scr_help_key(UI *u, Game *g, int k);
void dispatch_open(UI *u, Game *g, int tg, int ts, int tp, int ttype, int mission);
void modal_result_screens(UI *u, Game *g, int purpose, int arg, int ok, const char *input, int choice);

#endif
