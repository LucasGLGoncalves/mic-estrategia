/* mapa.c - um arquipélago gerado por ruído: relevo, umidade, biomas e nomes */
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../motor/rng.h"
#include "../../motor/ruido.h"
#include "../../motor/tela.h"
#include "../../motor/term.h"

#define L 100
#define A 22

typedef struct {
    uint32_t ch;
    Cor frente, fundo;
} Bioma;

/* Escolhe o bioma pela altura (h) e pela umidade (u), ambas de 0 a 1. */
static Bioma bioma(double h, double u, double mar) {
    /* ≈ fundo */
    if (h < mar - 0.12)
        return (Bioma){0x2248, RGB(40, 70, 140), RGB(12, 28, 70)};
    /* ~ raso */
    if (h < mar) return (Bioma){'~', RGB(90, 140, 220), RGB(24, 52, 110)};
    /* praia */
    if (h < mar + 0.03)
        return (Bioma){'.', RGB(200, 180, 120), RGB(150, 130, 80)};
    /* ▲ neve */
    if (h > 0.80) return (Bioma){0x25B2, BRANCO, RGB(120, 120, 130)};
    /* ▲ pico */
    if (h > 0.73) return (Bioma){0x25B2, RGB(150, 140, 130), RGB(80, 74, 66)};
    /* morros */
    if (h > 0.66) return (Bioma){'n', RGB(150, 120, 80), RGB(84, 70, 44)};
    /* deserto */
    if (u < 0.38) return (Bioma){':', RGB(220, 190, 110), RGB(150, 120, 60)};
    /* ♣ mata */
    if (u > 0.62) return (Bioma){0x2663, RGB(40, 130, 60), RGB(18, 64, 30)};
    return (Bioma){'"', RGB(120, 190, 90), RGB(46, 96, 40)}; /* campo */
}

/* Gera um nome juntando sílabas sorteadas: sempre o mesmo para a mesma
   semente. */
static void gera_nome(Rng *r, char *saida, size_t tam) {
    static const char *inicio[] = {"Ka",  "Mo", "Tar", "Ve", "Lu",
                                   "Sha", "Or", "Bri", "Ze", "Ny"};
    static const char *meio[] = {"ra",  "li", "dor", "ve", "mu",
                                 "the", "sa", "ko",  "ne", "la"};
    static const char *fim[] = {"nia", "ris", "dar", "ta", "lon",
                                "mar", "via", "dun", "ra", "lis"};
    snprintf(saida, tam, "%s%s%s", inicio[rng_faixa(r, 0, 9)],
             rng_chance(r, 0.6) ? meio[rng_faixa(r, 0, 9)] : "",
             fim[rng_faixa(r, 0, 9)]);
}

int main(void) {
    if (!term_inicia() || !tela_inicia(L, 25)) return 1;
    uint64_t semente = 2026;
    double mar = 0.45;
    int oitavas = 5;
    for (;;) {
        tela_limpa(FUNDO);
        int terra = 0;
        for (int y = 0; y < A; y++) {
            for (int x = 0; x < L; x++) {
                /* as células do terminal são duas vezes mais altas que
                   largas */
                double nx = x * 0.045, ny = y * 0.09;
                double h = fbm2(semente, nx, ny, oitavas);
                double u = fbm2(semente + 77, nx * 0.8, ny * 0.8, 3);
                /* afunda as bordas para o mapa virar um arquipélago */
                double dx = (x - L / 2.0) / (L / 2.0),
                       dy = (y - A / 2.0) / (A / 2.0);
                h = h - 0.30 * (dx * dx + dy * dy) + 0.10;
                Bioma b = bioma(h, u, mar);
                tela_poe(x, y + 1, b.ch, b.frente, b.fundo);
                if (h >= mar) terra++;
            }
        }
        Rng r;
        rng_semente(&r, semente);
        char nome[32];
        gera_nome(&r, nome, sizeof nome);
        tela_preenche(0, 0, L, 1, ' ', BRANCO, PAINEL);
        tela_textof(1, 0, AMARELO, PAINEL, "Arquipélago de %s", nome);
        tela_textof(50, 0, CINZA, PAINEL, "semente %" PRIu64 "   terra %d%%",
                    semente, terra * 100 / (L * A));
        tela_preenche(0, 23, L, 2, ' ', CINZA, PAINEL);
        tela_textof(
            1, 23, CINZA, PAINEL,
            "N novo mapa   + - nível do mar (%.2f)   O oitavas (%d)   Esc sai",
            mar, oitavas);
        tela_mostra();

        int t = term_espera_tecla(1000);
        if (t == TECLA_ESC || t == TECLA_SAIR) break;
        if (t == 'n' || t == 'N')
            semente = hash_mistura(semente + (uint64_t)time(NULL));
        if (t == '+' && mar < 0.8) mar += 0.03;
        if (t == '-' && mar > 0.1) mar -= 0.03;
        if (t == 'o' || t == 'O') oitavas = oitavas % 6 + 1;
    }
    tela_encerra();
    term_encerra();
    return 0;
}
