//
// Created by annab on 31/12/2025.
//

#include "../include/chodec.h"

#include "mapa.h"

// Inicializacia chodca na start poziciu a vychodzie hodnoty
void chodec_init(Chodec *c, int start_x, int start_y) {
    c->x = start_x;
    c->y = start_y;
    c->pocet_krokov = 0;
    c->je_aktivny = true;
}

// Jeden krok chodca podla smeru a podla mapy
// Ak narazi na prekazku, zostane stat na mieste ale krok sa mu aj tak zapocita
void chodec_krok(Chodec *c, Smer s, const Mapa *m) {
    if (!c->je_aktivny) return;

    // Posun v smere, plus wrap aby som ostala v hraniciach mapy
    int nove_x = wrap_suradnicu(c->x + pohyb[s][0], m->min_x, m->max_x);
    int nove_y = wrap_suradnicu(c->y + pohyb[s][1], m->min_y, m->max_y);

    // Ak je cielove policko volne, tak sa presuniem
    if (je_tam_volno(m, nove_x, nove_y)) {
        c->x = nove_x;
        c->y = nove_y;
    }

    // Krok sa pocita vzdy, aj ked som narazila na prekazku
    c->pocet_krokov++;
}

// Kontrola ci chodec dorazil do ciela
bool chodec_je_v_cieli(const Chodec *c) {
    return (c->x == 0 && c->y == 0);
}
