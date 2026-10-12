#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include  "structures.h"


// Zobrazi menu v konzole a vrati volbu pouzivatela
// Podla navratovej hodnoty viem, ci chcem novu simulaciu, opatovnu simulaciu alebo koniec
int zobrazMenu();
// Nacita od pouzivatela parametre pre novu simulaciu a naplni strukturu nova_sim
void getParametreNovejSimulacie(Simulacia_parametre *nova_sim);
// Nacita od pouzivatela parametre pre opatovnu simulaciu, hlavne nazov vstupneho suboru a rep
void getParametreOpatovnejSimulacie(Simulacia_parametre *opatovnaSimulacia);

// Nastavi defaultne hodnoty pre novu simulaciu
void naplnDefaultneParametre(Simulacia_parametre *nova_sim);
// Nacita rozmery mapy od pouzivatela
void getRozmery(Mapa *mapa);
// Nacita pravdepodobnosti pohybu
void getPravdepodobnostiPohybu();
// Nacita maximalny pocet krokov pre jeden pokus
void getMaxKrokov();
// Nacita nazov vstupneho suboru pre opatovnu simulaciu
void getNazovVstupnehoSuboru();
// Nacita pocet replikacii, ktore chcem spustit
void getPocetReplikacii();




#endif
