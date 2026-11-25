/* piloto.c - o administrador automático: regras simples, em ordem de
   urgência */
#include "piloto.h"

/* Escolhe a próxima obra pelas necessidades mais urgentes da colônia. */
static Estrutura escolhe_obra(const Colonia *c) {
    Taxas t = colonia_taxas(c);
    if (c->pesquisado[DOBRA] && c->nivel[PORTAL] == 0) return PORTAL;
    double portal_m, portal_k;
    custo_de(PORTAL, 1, &portal_m, &portal_k);
    if (c->recurso[METAL] > 0.8 * t.estoque_max ||
        c->recurso[CRISTAL] > 0.8 * t.estoque_max ||
        (c->pesquisado[SUPERCONDUTORES] && t.estoque_max < portal_m))
        if (nivel_planejado(c, ARMAZEM) == c->nivel[ARMAZEM]) return ARMAZEM;
    if (t.eficiencia_energia < 1) return USINA;
    if (t.producao[COMIDA] < 0.5) return FAZENDA;
    if (t.trabalhadores > 0.8 * t.capacidade || t.eficiencia_trabalho < 1)
        return HABITACAO;
    if (nivel_planejado(c, LABORATORIO) * 3 < nivel_planejado(c, MINA_METAL))
        return LABORATORIO;
    double m1, k1, m2, k2;
    custo_de(MINA_METAL, nivel_planejado(c, MINA_METAL) + 1, &m1, &k1);
    custo_de(MINA_CRISTAL, nivel_planejado(c, MINA_CRISTAL) + 1, &m2, &k2);
    return m1 + k1 <= m2 + k2 ? MINA_METAL : MINA_CRISTAL;
}

void piloto_administra(Colonia *c) {
    for (int p = 0; p < N_PESQUISAS; p++) pesquisa(c, (Pesquisa)p);
    if (c->n_fila < 2 &&
        !(c->nivel[PORTAL] > 0 || nivel_planejado(c, PORTAL) > 0))
        encomenda(c, escolhe_obra(c));
}

void piloto_joga(Colonia *c, double segundos) {
    for (double t = 0; t < segundos && !c->venceu; t += 1) {
        piloto_administra(c);
        colonia_avanca(c, 1);
    }
}
