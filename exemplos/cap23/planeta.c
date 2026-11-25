/* planeta.c - um planeta ASCII que gira: esfera, luz, relevo, nuvens e luzes
   das cidades */
#include <math.h>
#include <stdio.h>

#include "../../motor/rng.h"
#include "../../motor/ruido.h"
#include "../../motor/tela.h"
#include "../../motor/term.h"

typedef struct {
    const char *nome;
    Cor mar, praia, terra, alto, gelo;
    double nivel_mar, gelo_lat;
} Estilo;

static const Estilo ESTILOS[4] = {
    {"temperado", RGB(30, 80, 190), RGB(200, 190, 120), RGB(60, 150, 70),
     RGB(120, 110, 90), RGB(235, 240, 250), 0.50, 0.78},
    {"desértico", RGB(150, 100, 50), RGB(220, 170, 90), RGB(210, 140, 70),
     RGB(160, 90, 50), RGB(240, 220, 190), 0.30, 0.95},
    {"gelado", RGB(70, 110, 150), RGB(170, 200, 220), RGB(200, 220, 235),
     RGB(150, 170, 190), RGB(250, 252, 255), 0.45, 0.35},
    {"vulcânico", RGB(200, 60, 20), RGB(250, 150, 40), RGB(60, 50, 50),
     RGB(30, 28, 30), RGB(90, 80, 80), 0.42, 2.0},
};

/* A rampa de brilho: do mais escuro ao mais claro. */
static const char RAMPA[] = " .:-=+*#%@";

static void desenha_planeta(int cx, int cy, double raio, double giro,
                            uint64_t semente, const Estilo *e) {
    /* direção da luz (já normalizada) */
    const double lx = -0.62, ly = -0.38, lz = 0.69;
    for (int y = (int)(cy - raio / 2) - 1; y <= cy + raio / 2 + 1; y++) {
        for (int x = (int)(cx - raio) - 1; x <= cx + raio + 1; x++) {
            /* as células têm o dobro da altura da largura: o y conta em
               dobro */
            double nx = (x - cx) / raio, ny = (y - cy) / (raio / 2.0);
            double d2 = nx * nx + ny * ny;
            if (d2 > 1.0) {
                /* atmosfera */
                if (d2 < 1.18)
                    tela_poe(x, y, '.', RGB(60, 110, 200), COR_MANTEM);
                continue;
            }
            double nz = sqrt(1.0 - d2);
            /* gira o ponto da superfície em volta do eixo vertical */
            double c = cos(giro), s = sin(giro);
            double px = nx * c + nz * s, pz = -nx * s + nz * c, py = ny;
            double h =
                fbm3(semente, px * 1.8 + 5, py * 1.8 + 5, pz * 1.8 + 5, 5);
            /* quanto a superfície encara o sol */
            double luz = nx * lx + ny * ly + nz * lz;
            Cor base;
            if (fabs(py) > e->gelo_lat - (h - 0.5) * 0.3) base = e->gelo;
            else if (h < e->nivel_mar)
                base = cor_mistura(e->mar, RGB(10, 20, 60),
                                   (e->nivel_mar - h) * 3);
            else if (h < e->nivel_mar + 0.03) base = e->praia;
            else if (h > 0.68) base = e->alto;
            else base = e->terra;
            /* nuvens: outra camada de ruído, girando um pouco mais rápido */
            double gn = giro * 1.15;
            double qx = nx * cos(gn) + nz * sin(gn),
                   qz = -nx * sin(gn) + nz * cos(gn);
            double nuvem = fbm3(semente + 9, qx * 2.5, py * 3.5, qz * 2.5, 4);
            int tem_nuvem = nuvem > 0.58;
            if (tem_nuvem)
                base = cor_mistura(base, BRANCO, (nuvem - 0.58) * 5 + 0.45);
            double brilho = luz < 0 ? 0 : luz;
            if (luz < 0.05 && !tem_nuvem && h >= e->nivel_mar + 0.03 &&
                fabs(py) < e->gelo_lat &&
                hash_xy(semente, (int)(px * 40), (int)(py * 20 + pz * 40)) %
                        7 ==
                    0) {
                /* luzes de cidades à noite */
                tela_poe(x, y, '.', RGB(255, 210, 120), COR_MANTEM);
                continue;
            }
            int k = (int)(brilho * 9.0 + 0.5);
            if (k < 1) k = 1;
            Cor cor = cor_escurece(base, 0.35 + 0.65 * brilho);
            tela_poe(x, y, (uint32_t)RAMPA[k], cor, COR_MANTEM);
        }
    }
}

static void desenha_estrelas(uint64_t semente, double t) {
    for (int y = 1; y < 24; y++)
        for (int x = 0; x < 100; x++) {
            uint64_t h = hash_xy(semente, x, y);
            if (h % 61 != 0) continue;
            int pisca = (int)(t * 2 + (double)(h >> 40 & 0xFF)) % 9 == 0;
            tela_poe(x, y, h % 3 ? '.' : '*', pisca ? BRANCO : CINZA_ESCURO,
                     COR_MANTEM);
        }
}

int main(void) {
    if (!term_inicia() || !tela_inicia(100, 25)) return 1;
    uint64_t semente = hash_texto("Kepler");
    int estilo = 0, pausado = 0;
    double giro = 0, velocidade = 0.35, anterior = term_agora(), t = 0;
    for (;;) {
        double agora = term_agora();
        double dt = agora - anterior;
        anterior = agora;
        t += dt;
        if (!pausado) giro += velocidade * dt;

        tela_limpa(PRETO);
        desenha_estrelas(7, t);
        desenha_planeta(50, 12, 21, giro, semente, &ESTILOS[estilo]);
        tela_preenche(0, 0, 100, 1, ' ', BRANCO, PAINEL);
        tela_textof(1, 0, AMARELO, PAINEL, "Planeta %s", ESTILOS[estilo].nome);
        tela_preenche(0, 24, 100, 1, ' ', CINZA, PAINEL);
        tela_texto(1, 24,
                   "1-4 estilo   N outro planeta   + - velocidade   Espaço "
                   "pausa   Esc sai",
                   CINZA, PAINEL);
        tela_mostra();

        int k;
        while ((k = term_tecla()) != 0) {
            if (k == TECLA_ESC || k == TECLA_SAIR) goto fim;
            if (k >= '1' && k <= '4') estilo = k - '1';
            if (k == 'n' || k == 'N') semente = hash_mistura(semente);
            if (k == '+') velocidade *= 1.5;
            if (k == '-') velocidade /= 1.5;
            if (k == ' ') pausado = !pausado;
        }
        term_dorme(33);
    }
fim:
    tela_encerra();
    term_encerra();
    return 0;
}
