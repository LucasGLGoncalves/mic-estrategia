/* main.c - Conquista: um pequeno jogo de conquista por turnos, à moda do
   Empire */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../motor/tela.h"
#include "../motor/term.h"
#include "desenho.h"
#include "ia.h"

static Jogo jogo;
static Interface ui = {-1, -1, MODO_NORMAL, 0, 0, 0, 0};
static int animar = 1;

/* Desenha a jogada do computador que o jogador consegue ver, com uma pausa. */
static void mostra_acao(const Jogo *j) {
    if (!animar) return;
    desenha(j, &ui, term_agora());
    tela_mostra();
    term_dorme(70);
}

/* A próxima unidade do jogador que ainda pode agir e não tem ordens. */
static int proxima_unidade(int depois_de) {
    for (int k = 1; k <= MAX_UNIDADES; k++) {
        int u = (depois_de + k + MAX_UNIDADES) % MAX_UNIDADES;
        const Unidade *un = &jogo.unidades[u];
        if (un->viva && un->dono == JOGADOR && un->movimento > 0 &&
            un->ordem == ORDEM_NENHUMA)
            return u;
    }
    return -1;
}

static void seleciona_proxima(void) {
    ui.sel = proxima_unidade(ui.sel < 0 ? -1 : ui.sel);
    ui.cidade = -1;
}

static void escolhe_producao(int c) {
    char titulo[48];
    snprintf(titulo, sizeof titulo, "Produção de %s",
             jogo.mapa.cidades[c].nome);
    const char *opcoes[N_TIPOS] = {
        "Explorador (3 turnos): rápido, enxerga longe",
        "Exército (4 turnos): conquista cidades",
        "Blindado (8 turnos): forte e veloz"};
    int t = ui_escolha(titulo, opcoes, N_TIPOS);
    if (t >= 0) cidade_produz(&jogo, c, (TipoUnidade)t);
}

/* Move a unidade selecionada; se houver inimigo, mostra a chance e pergunta. */
static void ordem_mover(int dx, int dy) {
    Unidade *un = &jogo.unidades[ui.sel];
    int x = un->x + dx, y = un->y + dy;
    if (!mapa_passavel(&jogo.mapa, x, y)) return;
    int outra = unidade_em(&jogo, x, y), c = cidade_em(&jogo.mapa, x, y);
    int inimigo = (outra >= 0 && jogo.unidades[outra].dono != JOGADOR) ||
                  (outra < 0 && c >= 0 && jogo.mapa.cidades[c].dono != JOGADOR);
    if (inimigo) {
        if (outra < 0 && !UNIDADES[un->tipo].captura) {
            ui_aviso("Explorador", "Exploradores não conquistam cidades. Use "
                                   "um exército ou um blindado.");
            return;
        }
        char pergunta[120];
        snprintf(pergunta, sizeof pergunta,
                 "Chance estimada de vitória: %.0f%%. Atacar?",
                 100 * chance_de_vitoria(&jogo, ui.sel, x, y));
        if (!ui_confirma("Ataque", pergunta)) return;
    }
    un->ordem = ORDEM_NENHUMA;
    if (unidade_move(&jogo, ui.sel, dx, dy) &&
        (!jogo.unidades[ui.sel].viva || jogo.unidades[ui.sel].movimento == 0))
        seleciona_proxima();
}

/* Cumpre a ordem "vai até" de uma unidade, passo a passo. */
static void marcha(int u) {
    Unidade *un = &jogo.unidades[u];
    while (un->viva && un->movimento > 0 && un->ordem == ORDEM_IR) {
        if (un->x == un->alvo_x && un->y == un->alvo_y) {
            un->ordem = ORDEM_NENHUMA;
            break;
        }
        int dx, dy;
        if (caminho_para(&jogo, u, un->alvo_x, un->alvo_y, &dx, &dy) < 0) {
            un->ordem = ORDEM_NENHUMA;
            diario_poe(&jogo.diario, "O %s não encontra caminho.",
                       UNIDADES[un->tipo].nome);
            break;
        }
        int x = un->x + dx, y = un->y + dy, outra = unidade_em(&jogo, x, y);
        int c = cidade_em(&jogo.mapa, x, y);
        if ((outra >= 0 && jogo.unidades[outra].dono != JOGADOR) ||
            (c >= 0 && jogo.mapa.cidades[c].dono != JOGADOR)) {
            un->ordem = ORDEM_NENHUMA; /* não ataca sozinha: o jogador decide */
            diario_poe(&jogo.diario, "Seu %s chegou perto do inimigo.",
                       UNIDADES[un->tipo].nome);
            break;
        }
        if (!unidade_move(&jogo, u, dx, dy)) break;
        mostra_acao(&jogo);
    }
}

static void encerra_turno(void) {
    ui.sel = ui.cidade = -1;
    ui.vez_da_ia = 1;
    turno_comeca(&jogo, COMPUTADOR);
    ia_joga(&jogo, COMPUTADOR, mostra_acao);
    ui.vez_da_ia = 0;
    if (jogo.vencedor != NEUTRO) return;
    jogo.turno++;
    turno_comeca(&jogo, JOGADOR);
    for (int u = 0; u < MAX_UNIDADES; u++)
        if (jogo.unidades[u].viva && jogo.unidades[u].dono == JOGADOR)
            marcha(u);
    diario_poe(&jogo.diario, "\001y— Turno %d —\001d", jogo.turno);
    seleciona_proxima();
}

static void tecla_mira(int t) {
    /* cima, baixo, esquerda, direita */
    static const int DX[4] = {0, 0, -1, 1}, DY[4] = {-1, 1, 0, 0};
    if (t >= TECLA_CIMA && t <= TECLA_DIREITA) {
        int k = t - TECLA_CIMA, x = ui.cx + DX[k], y = ui.cy + DY[k];
        if (mapa_dentro(x, y)) {
            ui.cx = x;
            ui.cy = y;
        }
    } else if (t == TECLA_ENTER) {
        Unidade *un = &jogo.unidades[ui.sel];
        un->ordem = ORDEM_IR;
        un->alvo_x = ui.cx;
        un->alvo_y = ui.cy;
        ui.modo = MODO_NORMAL;
        marcha(ui.sel);
        if (un->movimento == 0 || un->ordem == ORDEM_IR) seleciona_proxima();
    } else if (t == TECLA_ESC) {
        ui.modo = MODO_NORMAL;
    }
}

/* Piloto automático: o general do computador joga pelo jogador por alguns
   turnos. Serve para testar e para começar uma partida já adiantada. */
static void piloto_automatico(int turnos) {
    animar = 0;
    for (int k = 0; k < turnos && jogo.vencedor == NEUTRO; k++) {
        ia_joga(&jogo, JOGADOR, NULL);
        /* as marchas do piloto não continuam */
        for (int u = 0; u < MAX_UNIDADES; u++)
            if (jogo.unidades[u].dono == JOGADOR &&
                jogo.unidades[u].ordem == ORDEM_IR)
                jogo.unidades[u].ordem = ORDEM_NENHUMA;
        encerra_turno();
    }
    for (int u = 0; u < MAX_UNIDADES; u++)
        if (jogo.unidades[u].dono == JOGADOR)
            jogo.unidades[u].ordem = ORDEM_NENHUMA;
    animar = 1;
    seleciona_proxima();
}

static int pendentes(void) { return proxima_unidade(-1) >= 0; }

static void tecla_normal(int t) {
    /* cima, baixo, esquerda, direita */
    static const int DX[4] = {0, 0, -1, 1}, DY[4] = {-1, 1, 0, 0};
    int tem = ui.sel >= 0 && jogo.unidades[ui.sel].viva;
    if (t >= TECLA_CIMA && t <= TECLA_DIREITA && tem) {
        ordem_mover(DX[t - TECLA_CIMA], DY[t - TECLA_CIMA]);
    } else if (t == TECLA_TAB || t == 'n' || t == 'N') {
        seleciona_proxima();
    } else if (t == ' ' && tem) {
        /* espera: fica parada e descansa */
        jogo.unidades[ui.sel].movimento = 0;
        seleciona_proxima();
    } else if ((t == 'f' || t == 'F') && tem) {
        jogo.unidades[ui.sel].ordem = ORDEM_SENTINELA;
        jogo.unidades[ui.sel].movimento = 0;
        seleciona_proxima();
    } else if ((t == 'g' || t == 'G') && tem) {
        ui.modo = MODO_MIRA;
        ui.cx = jogo.unidades[ui.sel].x;
        ui.cy = jogo.unidades[ui.sel].y;
    } else if (t == 'c' || t == 'C') {
        int n = jogo.mapa.n_cidades;
        for (int k = 1; k <= n; k++) {
            int c = (ui.cidade + k + n) % n;
            if (jogo.mapa.cidades[c].dono == JOGADOR) {
                ui.cidade = c;
                ui.sel = -1;
                break;
            }
        }
    } else if (t == TECLA_ENTER) {
        int c = ui.cidade;
        if (tem)
            c = cidade_em(&jogo.mapa, jogo.unidades[ui.sel].x,
                          jogo.unidades[ui.sel].y);
        if (c >= 0 && jogo.mapa.cidades[c].dono == JOGADOR) escolhe_producao(c);
    } else if (t == 't' || t == 'T') {
        if (!pendentes() ||
            ui_confirma("Fim do turno",
                        "Ainda há unidades que podem agir. Encerrar o turno?"))
            encerra_turno();
    }
}

static void tela_final(void) {
    char texto[400];
    int venceu = jogo.vencedor == JOGADOR;
    snprintf(texto, sizeof texto,
             "%s\n\nTurnos: %d\nInimigos destruídos: %d\nUnidades perdidas: %d",
             venceu ? "\001gVitória!\001d A última cidade inimiga caiu. O "
                      "continente é seu."
                    : "\001rDerrota.\001d A sua última cidade caiu.",
             jogo.turno, jogo.vitorias[JOGADOR], jogo.perdas[JOGADOR]);
    ui_aviso(venceu ? "Vitória" : "Derrota", texto);
}

int main(int argc, char **argv) {
    uint64_t semente = (uint64_t)time(NULL);
    int automaticos = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--semente") == 0 && i + 1 < argc)
            semente = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--revela") == 0) ui.revela = 1;
        else if (strcmp(argv[i], "--auto") == 0 && i + 1 < argc)
            automaticos = atoi(argv[++i]);
    }
    if (!term_inicia() || !tela_inicia(100, 30)) return 1;
    jogo_novo(&jogo, semente);
    seleciona_proxima();
    piloto_automatico(automaticos);
    while (jogo.vencedor == NEUTRO) {
        desenha(&jogo, &ui, term_agora());
        tela_mostra();
        int t = term_espera_tecla(100);
        if (t == 0) continue;
        if (t == TECLA_SAIR || (t == TECLA_ESC && ui.modo == MODO_NORMAL)) {
            if (ui_confirma("Sair", "Abandonar a guerra?")) break;
            continue;
        }
        if (ui.modo == MODO_MIRA) tecla_mira(t);
        else tecla_normal(t);
    }
    if (jogo.vencedor != NEUTRO) {
        desenha(&jogo, &ui, term_agora());
        tela_final();
    }
    tela_encerra();
    term_encerra();
    return 0;
}
