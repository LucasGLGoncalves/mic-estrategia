/* ia.c - o general do computador: regras simples e uma conta de utilidade.
   Ele joga com as mesmas regras e a mesma névoa de guerra que o jogador. */
#include "ia.h"

#include <stdlib.h>
#include <string.h>

/* Distância em passos de (x0, y0) a todas as casas, pelo que o computador
   conhece do mapa (o desconhecido conta como caminho livre). */
static void distancias(const Jogo *j, Dono eu, int x0, int y0,
                       int dist[MAPA_A][MAPA_L]) {
    static int fila[MAPA_L * MAPA_A];
    int ini = 0, fim = 0;
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) dist[y][x] = -1;
    dist[y0][x0] = 0;
    fila[fim++] = y0 * MAPA_L + x0;
    static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
    while (ini < fim) {
        int p = fila[ini++], px = p % MAPA_L, py = p / MAPA_L;
        for (int d = 0; d < 4; d++) {
            int x = px + DX[d], y = py + DY[d];
            if (!mapa_dentro(x, y) || dist[y][x] >= 0) continue;
            if (j->visto[eu][y][x] && !mapa_passavel(&j->mapa, x, y)) continue;
            dist[y][x] = dist[py][px] + 1;
            fila[fim++] = y * MAPA_L + x;
        }
    }
}

/* Uma casa de fronteira: conhecida, de terra, com um vizinho nunca visto. */
static int fronteira(const Jogo *j, Dono eu, int x, int y) {
    if (!j->visto[eu][y][x] || !mapa_passavel(&j->mapa, x, y)) return 0;
    static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
    for (int d = 0; d < 4; d++) {
        int nx = x + DX[d], ny = y + DY[d];
        if (mapa_dentro(nx, ny) && !j->visto[eu][ny][nx]) return 1;
    }
    return 0;
}

static int perto_de_inimigo(const Jogo *j, Dono eu, int x, int y, int raio) {
    for (int u = 0; u < MAX_UNIDADES; u++) {
        const Unidade *o = &j->unidades[u];
        if (!o->viva || o->dono == eu || !j->visivel[eu][o->y][o->x]) continue;
        if (abs(o->x - x) + abs(o->y - y) <= raio) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------
   produção */

static void producao(Jogo *j, Dono eu) {
    int conta[N_TIPOS] = {0}, vistas = 0, terra = 0;
    for (int u = 0; u < MAX_UNIDADES; u++)
        if (j->unidades[u].viva && j->unidades[u].dono == eu)
            conta[j->unidades[u].tipo]++;
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++) {
            vistas += j->visto[eu][y][x];
            terra += 1;
        }
    double desconhecido = 1.0 - (double)vistas / terra;
    for (int c = 0; c < j->mapa.n_cidades; c++) {
        Cidade *ci = &j->mapa.cidades[c];
        /* não joga trabalho fora */
        if (ci->dono != eu || ci->progresso >= 2) continue;
        TipoUnidade t = EXERCITO;
        if (conta[EXPLORADOR] == 0 && desconhecido > 0.3) t = EXPLORADOR;
        else if (perto_de_inimigo(j, eu, ci->x, ci->y, 5)) t = BLINDADO;
        else if (conta[EXERCITO] > 2 * conta[BLINDADO] + 1) t = BLINDADO;
        cidade_produz(j, c, t);
        /* conta a encomenda, para a próxima cidade decidir diferente */
        conta[t]++;
    }
}

/* ------------------------------------------------------------------ alvos */

typedef struct {
    int x, y;
    double utilidade;
    int defender; /* ao chegar, fica de sentinela */
} Alvo;

static int indo_para(const Jogo *j, Dono eu, int x, int y, int exceto) {
    int n = 0;
    for (int u = 0; u < MAX_UNIDADES; u++) {
        const Unidade *o = &j->unidades[u];
        n += u != exceto && o->viva && o->dono == eu && o->ordem == ORDEM_IR &&
             o->alvo_x == x && o->alvo_y == y;
    }
    return n;
}

/* Utilidade = valor x chance de sucesso / distância. Cada alvo concorre com
   os outros, e o maior vence: é assim que o general "decide". */
static Alvo escolhe_alvo(const Jogo *j, int u, int dist[MAPA_A][MAPA_L]) {
    Dono eu = j->unidades[u].dono;
    Alvo melhor = {-1, -1, 0, 0};
    for (int c = 0; c < j->mapa.n_cidades; c++) {
        const Cidade *ci = &j->mapa.cidades[c];
        int d = dist[ci->y][ci->x];
        if (d < 0 || !j->visto[eu][ci->y][ci->x]) continue;
        Dono dono = j->lembra[eu][c];
        double valor, chance;
        if (dono == eu) {
            /* defender: uma cidade minha, vazia, com inimigos por perto */
            if (ci->dono != eu || unidade_em(j, ci->x, ci->y) >= 0) continue;
            if (!perto_de_inimigo(j, eu, ci->x, ci->y, 4)) continue;
            valor = 14;
            chance = 1;
        } else {
            valor = dono == NEUTRO ? 8 : 10 + (ci->capital ? 6 : 0);
            chance = j->visivel[eu][ci->y][ci->x]
                         ? chance_de_vitoria(j, u, ci->x, ci->y)
                         : 0.6;
        }
        double ut = valor * chance / (d + 2) /
                    (1 + 0.5 * indo_para(j, eu, ci->x, ci->y, u));
        if (ut > melhor.utilidade)
            melhor = (Alvo){ci->x, ci->y, ut, dono == eu};
    }
    for (int v = 0; v < MAX_UNIDADES; v++) {
        const Unidade *o = &j->unidades[v];
        if (!o->viva || o->dono == eu || !j->visivel[eu][o->y][o->x]) continue;
        int d = dist[o->y][o->x];
        if (d < 0) continue;
        double chance = chance_de_vitoria(j, u, o->x, o->y);
        if (chance < 0.5) continue;
        double valor = o->tipo == BLINDADO ? 9 : o->tipo == EXERCITO ? 7 : 4;
        double ut = valor * chance / (d + 2);
        if (ut > melhor.utilidade) melhor = (Alvo){o->x, o->y, ut, 0};
    }
    return melhor;
}

static Alvo explora(const Jogo *j, Dono eu, int dist[MAPA_A][MAPA_L]) {
    Alvo a = {-1, -1, 0, 0};
    int menor = 1 << 30;
    for (int y = 0; y < MAPA_A; y++)
        for (int x = 0; x < MAPA_L; x++)
            if (dist[y][x] > 0 && dist[y][x] < menor &&
                fronteira(j, eu, x, y)) {
                menor = dist[y][x];
                a = (Alvo){x, y, 1, 0};
            }
    return a;
}

/* ------------------------------------------------------------------ ações */

static void age(Jogo *j, int u, MostraAcao mostra) {
    Unidade *un = &j->unidades[u];
    Dono eu = un->dono;
    static int dist[MAPA_A][MAPA_L];
    distancias(j, eu, un->x, un->y, dist);

    int c = cidade_em(&j->mapa, un->x, un->y);
    int guarnicao = c >= 0 && j->mapa.cidades[c].dono == eu &&
                    perto_de_inimigo(j, eu, un->x, un->y, 4);

    Alvo alvo = {-1, -1, 0, 0};
    if (un->tipo != EXPLORADOR) alvo = escolhe_alvo(j, u, dist);
    if (guarnicao && !(alvo.utilidade > 1.5 && dist[alvo.y][alvo.x] == 1)) {
        /* a última linha de defesa não sai da cidade */
        un->ordem = ORDEM_SENTINELA;
        return;
    }
    int explorando = alvo.x < 0 || alvo.utilidade < 0.4;
    if (explorando) alvo = explora(j, eu, dist);
    while (un->viva && un->movimento > 0 && j->vencedor == NEUTRO) {
        if (alvo.x < 0) { /* nada a fazer: guarda a posição */
            un->ordem = ORDEM_SENTINELA;
            return;
        }
        un->ordem = ORDEM_IR;
        un->alvo_x = alvo.x;
        un->alvo_y = alvo.y;
        if (un->x == alvo.x && un->y == alvo.y) {
            if (alvo.defender) un->ordem = ORDEM_SENTINELA;
            if (!explorando) break;
            /* chegou: procura a próxima fronteira */
            distancias(j, eu, un->x, un->y, dist);
            alvo = explora(j, eu, dist);
            continue;
        }
        int dx, dy;
        if (caminho_para(j, u, alvo.x, alvo.y, &dx, &dy) < 0) break;
        int nx = un->x + dx, ny = un->y + dy;
        int ci = cidade_em(&j->mapa, nx, ny);
        int ataque = unidade_em(j, nx, ny) >= 0 ||
                     (ci >= 0 && j->mapa.cidades[ci].dono != eu);
        if (ataque &&
            (un->tipo == EXPLORADOR || chance_de_vitoria(j, u, nx, ny) < 0.4))
            break;
        int visto =
            j->visivel[JOGADOR][un->y][un->x] || j->visivel[JOGADOR][ny][nx];
        if (!unidade_move(j, u, dx, dy)) break;
        if (visto && mostra) mostra(j);
    }
}

void ia_joga(Jogo *j, Dono eu, MostraAcao mostra) {
    producao(j, eu);
    for (int u = 0; u < MAX_UNIDADES && j->vencedor == NEUTRO; u++) {
        const Unidade *un = &j->unidades[u];
        if (un->viva && un->dono == eu && un->movimento > 0) age(j, u, mostra);
    }
}
