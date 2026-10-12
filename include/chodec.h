#ifndef CHODEC_H
#define CHODEC_H

#include "structures.h"


// incicializacia chodca
void chodec_init(Chodec *c, int start_x, int start_y);

// Vykona jeden krok + wrapping a kontrola prekazok
void chodec_krok(Chodec *c, Smer s, const Mapa *m);

// Vráti true, ak je chodec v cieli
bool chodec_je_v_cieli(const Chodec *c);

#endif