#ifndef SIMULACIA_H
#define SIMULACIA_H

#include "structures.h"


// Spusti vypocet simulacie
void vypocet_simulacie(const Simulacia_parametre *par, const Mapa *mapa, Zdielane_data *data, int socket);


// Vypise zakladne nastavenia simulacie do konzoly
void vypis_zakladne_info_simulacie(const Simulacia_parametre *par);


// Ulozi simulaciu do binarneho suboru
void uloz_simulaciu(const char* cesta, Simulacia_parametre* par, Mapa* mapa);


// Nacita simulaciu z binarneho suboru
bool nacitaj_simulaciu(const char* cesta, Simulacia_parametre* par, Mapa* mapa);

#endif
