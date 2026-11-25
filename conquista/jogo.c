/* jogo.c - as regras da guerra: movimento, visão, combate, produção */
#include "jogo.h"

#include <stdio.h>
#include <string.h>

#include "../motor/caminho.h"

const DadosUnidade UNIDADES[N_TIPOS] = {
    /*            nome         letra força vida mov visão custo captura */
    [EXPLORADOR] = {"Explorador", 'X', 1, 6, 4, 4, 3, 0},
    [EXERCITO] = {"Exército", 'E', 3, 10, 2, 2, 4, 1},
    [BLINDADO] = {"Blindado", 'B', 5, 14, 3, 2, 8, 1},
};

int unidade_em(const Jogo *j, int x, int y) {
    for (int u = 0; u < MAX_UNIDADES; u++)
        if (j->unidades[u].viva && j->unidades[u].x == x &&
            j->unidades[u].y == y)
            return u;
    return -1;
}

int unidade_cria(Jogo *j, TipoUnidade tipo, Dono d, int x, int y) {
    for (int u = 0; u < MAX_UNIDADES; u++) {
        if (j->unidades[u].viva) continue;
        Unidade *n = &j->unidades[u];
        memset(n, 0, sizeof *n);
        n->viva = 1;
        n->tipo = tipo;
        n->dono = d;
        n->x = x;
        n->y = y;
        n->vida = UNIDADES[tipo].vida;
        n->movimento = UNIDADES[tipo].movimento;
        return u;
    }
    return -1; /* exército cheio */
}

int cidades_de(const Jogo *j, Dono d) {
    int n = 0;
    for (int c = 0; c < j->mapa.n_cidades; c++)
        n += j->mapa.cidades[c].dono == d;
    return n;
}

int unidades_de(const Jogo *j, Dono d) {
    int n = 0;
    for (int u = 0; u < MAX_UNIDADES; u++)
        n += j->unidades[u].viva && j->unidades[u].dono == d;
    return n;
}

/* ------------------------------------------------------------------ visão */

/* Marca como visível tudo o que está a até r casas de (x, y), num círculo. */
static void enxerga(Jogo *j, Dono d, int x, int y, int r) {
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy > r * r + r || !mapa_dentro(x + dx, y + dy))
                continue;
            j->visivel[d][y + dy][x + dx] = 1;
            j->visto[d][y + dy][x + dx] = 1;
        }
}

void jogo_atualiza_visao(Jogo *j, Dono d) {
    memset(j->visivel[d], 0, sizeof j->visivel[d]);
    for (int u = 0; u < MAX_UNIDADES; u++) {
        const Unidade *un = &j->unidades[u];
        if (un->viva && un->dono == d)
            enxerga(j, d, un->x, un->y, UNIDADES[un->tipo].visao);
    }
    for (int c = 0; c < j->mapa.n_cidades; c++) {
        const Cidade *ci = &j->mapa.cidades[c];
        if (ci->dono == d) enxerga(j, d, ci->x, ci->y, 2);
    }
    /* as cidades à vista: a memória do lado é atualizada */
    for (int c = 0; c < j->mapa.n_cidades; c++) {
        const Cidade *ci = &j->mapa.cidades[c];
        if (j->visivel[d][ci->y][ci->x]) j->lembra[d][c] = ci->dono;
    }
}

/* ------------------------------------------------------------------ partida */

void jogo_novo(Jogo *j, uint64_t semente) {
    memset(j, 0, sizeof *j);
    rng_semente(&j->rng, semente);
    /* nem toda semente dá um bom continente: tenta as seguintes */
    for (uint64_t k = 0; !mapa_gera(&j->mapa, semente + k * 7919); k++) {}
    for (int c = 0; c < j->mapa.n_cidades; c++) {
        Cidade *ci = &j->mapa.cidades[c];
        if (ci->dono == NEUTRO) continue;
        unidade_cria(j, EXERCITO, ci->dono, ci->x, ci->y);
        /* o explorador nasce numa casa vizinha livre */
        static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
        for (int d = 0; d < 4; d++) {
            int x = ci->x + DX[d], y = ci->y + DY[d];
            if (mapa_passavel(&j->mapa, x, y) && unidade_em(j, x, y) < 0) {
                unidade_cria(j, EXPLORADOR, ci->dono, x, y);
                break;
            }
        }
        ci->produz = EXERCITO;
    }
    j->turno = 1;
    j->vez = JOGADOR;
    j->vencedor = NEUTRO;
    jogo_atualiza_visao(j, JOGADOR);
    jogo_atualiza_visao(j, COMPUTADOR);
    diario_poe(&j->diario,
               "Turno 1. Conquiste as cidades do inimigo, \001rvermelho\001d, "
               "antes que ele conquiste as suas.");
}

/* ------------------------------------------------------------------ combate */

double forca_defesa(const Jogo *j, int x, int y, int *vida) {
    int u = unidade_em(j, x, y), c = cidade_em(&j->mapa, x, y);
    if (u >= 0) {
        const Unidade *d = &j->unidades[u];
        double f =
            UNIDADES[d->tipo].forca * defesa_terreno(j->mapa.terreno[y][x]);
        if (c >= 0) f *= 2.0;                       /* muralhas */
        if (d->ordem == ORDEM_SENTINELA) f *= 1.25; /* entrincheirado */
        *vida = d->vida;
        return f;
    }
    if (c >= 0 && j->mapa.cidades[c].dono == NEUTRO) {
        *vida = 8; /* a guarnição de uma cidade independente */
        return 3.0;
    }
    *vida = 4; /* cidade sem defensores: só os moradores resistem */
    return 1.5;
}

/* Uma luta: rodadas até alguém cair. A cada rodada, a chance de o atacante
   acertar é a força dele dividida pela soma das forças. */
static int luta(Rng *r, double fa, double fd, int *va, int *vd) {
    while (*va > 0 && *vd > 0) {
        if (rng_real(r) < fa / (fa + fd)) *vd -= rng_faixa(r, 2, 4);
        else *va -= rng_faixa(r, 2, 4);
    }
    return *vd <= 0;
}

double chance_de_vitoria(const Jogo *j, int u, int x, int y) {
    const Unidade *a = &j->unidades[u];
    int vd0;
    double fd = forca_defesa(j, x, y, &vd0);
    /* um gerador próprio: estimar não pode mudar o acaso do jogo */
    Rng r;
    rng_semente(&r, hash_xy((uint64_t)j->turno * 1000 + (uint64_t)u, x, y));
    int vitorias = 0;
    for (int k = 0; k < 400; k++) {
        int va = a->vida, vd = vd0;
        vitorias += luta(&r, UNIDADES[a->tipo].forca, fd, &va, &vd);
    }
    return vitorias / 400.0;
}

static void captura(Jogo *j, int u, int c) {
    Unidade *a = &j->unidades[u];
    Cidade *ci = &j->mapa.cidades[c];
    Dono antes = ci->dono;
    ci->dono = a->dono;
    ci->progresso = 0;
    ci->produz = EXERCITO;
    if (a->dono == JOGADOR)
        diario_poe(&j->diario, "\001g%s foi conquistada!\001d", ci->nome);
    else if (antes == JOGADOR)
        diario_poe(&j->diario, "\001r%s caiu nas mãos do inimigo!\001d",
                   ci->nome);
    else if (j->visivel[JOGADOR][ci->y][ci->x])
        diario_poe(&j->diario, "O inimigo tomou a cidade independente de %s.",
                   ci->nome);
    if (antes != NEUTRO && cidades_de(j, antes) == 0) j->vencedor = a->dono;
}

static void ataque(Jogo *j, int u, int x, int y) {
    Unidade *a = &j->unidades[u];
    int d = unidade_em(j, x, y), c = cidade_em(&j->mapa, x, y);
    int vd;
    double fd = forca_defesa(j, x, y, &vd);
    int va = a->vida;
    int venceu = luta(&j->rng, UNIDADES[a->tipo].forca, fd, &va, &vd);
    int jogador_ve = a->dono == JOGADOR ||
                     (d >= 0 && j->unidades[d].dono == JOGADOR) ||
                     j->visivel[JOGADOR][y][x];
    if (jogador_ve) {
        char quem[48], alvo[64];
        if (a->dono == JOGADOR)
            snprintf(quem, sizeof quem, "Seu %s", UNIDADES[a->tipo].nome);
        else
            snprintf(quem, sizeof quem, "Um %s inimigo",
                     UNIDADES[a->tipo].nome);
        if (d >= 0 && j->unidades[d].dono == JOGADOR)
            snprintf(alvo, sizeof alvo, "o seu %s",
                     UNIDADES[j->unidades[d].tipo].nome);
        else if (d >= 0)
            snprintf(alvo, sizeof alvo, "um %s inimigo",
                     UNIDADES[j->unidades[d].tipo].nome);
        else if (j->mapa.cidades[c].dono == NEUTRO)
            snprintf(alvo, sizeof alvo, "a guarnição de %s",
                     j->mapa.cidades[c].nome);
        else
            snprintf(alvo, sizeof alvo, "%s, sem defensores",
                     j->mapa.cidades[c].nome);
        int envolvido = a->dono == JOGADOR ||
                        (d >= 0 && j->unidades[d].dono == JOGADOR) ||
                        (c >= 0 && j->mapa.cidades[c].dono == JOGADOR);
        /* do ponto de vista do jogador */
        int bom = a->dono == JOGADOR ? venceu : !venceu;
        if (envolvido)
            diario_poe(&j->diario, "%s ataca %s: %s", quem, alvo,
                       bom ? "\001gvitória\001d" : "\001rderrota\001d");
        else if (!venceu) /* a vitória do inimigo já é noticiada na captura */
            diario_poe(&j->diario, "%s ataca %s e é repelido.", quem, alvo);
    }
    a->agiu = 1;
    a->movimento = 0;
    if (!venceu) {
        a->viva = 0;
        j->perdas[a->dono]++;
        if (d >= 0) {
            j->unidades[d].vida = vd;
            j->vitorias[j->unidades[d].dono]++;
        }
        return;
    }
    a->vida = va;
    j->vitorias[a->dono]++;
    if (d >= 0) {
        j->unidades[d].viva = 0;
        j->perdas[j->unidades[d].dono]++;
    }
    /* exploradores não tomam cidades */
    if (c >= 0 && !UNIDADES[a->tipo].captura) return;
    a->x = x; /* o vencedor ocupa a casa */
    a->y = y;
    if (c >= 0 && j->mapa.cidades[c].dono != a->dono) captura(j, u, c);
}

/* ------------------------------------------------------------------
   movimento */

int unidade_move(Jogo *j, int u, int dx, int dy) {
    Unidade *a = &j->unidades[u];
    int x = a->x + dx, y = a->y + dy;
    if (a->movimento <= 0 || !mapa_passavel(&j->mapa, x, y)) return 0;
    int outra = unidade_em(j, x, y), c = cidade_em(&j->mapa, x, y);
    /* uma por casa */
    if (outra >= 0 && j->unidades[outra].dono == a->dono) return 0;
    int inimiga = outra >= 0 || (c >= 0 && j->mapa.cidades[c].dono != a->dono);
    if (inimiga) {
        if (outra < 0 && !UNIDADES[a->tipo].captura) return 0;
        ataque(j, u, x, y);
    } else {
        int custo = custo_terreno(j->mapa.terreno[y][x]);
        /* sem pontos suficientes, só entra quem ainda não se moveu no turno */
        if (a->movimento < custo && a->movimento < UNIDADES[a->tipo].movimento)
            return 0;
        a->x = x;
        a->y = y;
        a->movimento -= custo;
        if (a->movimento < 0) a->movimento = 0;
        a->agiu = 1;
    }
    jogo_atualiza_visao(j, a->dono);
    return 1;
}

typedef struct {
    const Jogo *j;
    Dono d;
    int ax, ay;
} Consulta;

/* O custo de entrar numa casa, do ponto de vista de um lado: o que ele nunca
   viu é tratado como planície (otimismo); as próprias unidades bloqueiam. */
static int custo_para(void *dados, int x, int y) {
    const Consulta *q = dados;
    const Jogo *j = q->j;
    if (x == q->ax && y == q->ay) return 1;
    if (!j->visto[q->d][y][x]) return 1;
    int c = custo_terreno(j->mapa.terreno[y][x]);
    if (c < 0) return -1;
    if (j->visivel[q->d][y][x]) {
        int u = unidade_em(j, x, y);
        /* aliada no caminho, ou inimiga que não é o alvo */
        if (u >= 0) return -1;
    }
    return c;
}

int caminho_lista(const Jogo *j, int u, int ax, int ay, int *xy, int max,
                  int *custo) {
    const Unidade *a = &j->unidades[u];
    Consulta q = {j, a->dono, ax, ay};
    ResultadoBusca r =
        caminho_busca(MAPA_L, MAPA_A, a->x, a->y, ax, ay, custo_para, &q,
                      BUSCA_ESTRELA, xy, max, NULL);
    if (custo != NULL) *custo = r.custo;
    return r.passos;
}

int caminho_para(const Jogo *j, int u, int ax, int ay, int *dx, int *dy) {
    int xy[2 * MAPA_L * MAPA_A], custo;
    if (caminho_lista(j, u, ax, ay, xy, MAPA_L * MAPA_A, &custo) <= 0)
        return -1;
    *dx = xy[0] - j->unidades[u].x;
    *dy = xy[1] - j->unidades[u].y;
    return custo;
}

/* ------------------------------------------------------------------ turnos */

void cidade_produz(Jogo *j, int c, TipoUnidade tipo) {
    Cidade *ci = &j->mapa.cidades[c];
    /* trocar de ideia custa o trabalho */
    if (ci->produz != tipo) ci->progresso = 0;
    ci->produz = tipo;
}

static void producao(Jogo *j, Dono d) {
    static const int DX[5] = {0, 1, -1, 0, 0}, DY[5] = {0, 0, 0, 1, -1};
    for (int c = 0; c < j->mapa.n_cidades; c++) {
        Cidade *ci = &j->mapa.cidades[c];
        if (ci->dono != d) continue;
        ci->progresso += ci->capital ? 2 : 1; /* a capital produz em dobro */
        if (ci->progresso < UNIDADES[ci->produz].custo) continue;
        /* na cidade, ou numa casa vizinha livre */
        for (int k = 0; k < 5; k++) {
            int x = ci->x + DX[k], y = ci->y + DY[k];
            if (!mapa_passavel(&j->mapa, x, y) || unidade_em(j, x, y) >= 0)
                continue;
            if (k > 0 && cidade_em(&j->mapa, x, y) >= 0) continue;
            if (unidade_cria(j, ci->produz, d, x, y) < 0) break;
            ci->progresso -= UNIDADES[ci->produz].custo;
            if (d == JOGADOR)
                diario_poe(&j->diario, "%s produziu: %s.", ci->nome,
                           UNIDADES[ci->produz].nome);
            break;
        }
    }
}

void turno_comeca(Jogo *j, Dono d) {
    j->vez = d;
    producao(j, d);
    for (int u = 0; u < MAX_UNIDADES; u++) {
        Unidade *un = &j->unidades[u];
        if (!un->viva || un->dono != d) continue;
        int c = cidade_em(&j->mapa, un->x, un->y);
        if (!un->agiu) un->vida += 1; /* descansar cura */
        if (c >= 0 && j->mapa.cidades[c].dono == d) un->vida += 2;
        if (un->vida > UNIDADES[un->tipo].vida)
            un->vida = UNIDADES[un->tipo].vida;
        un->movimento = UNIDADES[un->tipo].movimento;
        un->agiu = 0;
    }
    jogo_atualiza_visao(j, d);
    /* sentinelas acordam quando um inimigo aparece por perto */
    for (int u = 0; u < MAX_UNIDADES; u++) {
        Unidade *un = &j->unidades[u];
        if (!un->viva || un->dono != d || un->ordem != ORDEM_SENTINELA)
            continue;
        for (int v = 0; v < MAX_UNIDADES; v++) {
            const Unidade *o = &j->unidades[v];
            if (!o->viva || o->dono == d || !j->visivel[d][o->y][o->x])
                continue;
            int dx = o->x - un->x, dy = o->y - un->y;
            if (dx * dx + dy * dy <= 8) {
                un->ordem = ORDEM_NENHUMA;
                if (d == JOGADOR)
                    diario_poe(
                        &j->diario,
                        "Sentinela: \001yinimigo à vista\001d perto do %s.",
                        UNIDADES[un->tipo].nome);
                break;
            }
        }
    }
}
