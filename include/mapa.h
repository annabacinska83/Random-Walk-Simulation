#ifndef NAHODNA_POCHODZKA_POS_MAPA_H
#define NAHODNA_POCHODZKA_POS_MAPA_H

#include "struktury.h"
#include <stdio.h>

//Funkcia na vytvorenie mapy v pamati
void alokujMapu(Mapa *mapa,int sirka, int vyska);

//Funkcia na dealokaciu mapy
void uvolniMapu(Mapa *mapa);

//Funkcia na prepocitanie suradnic z 2D pola n splostene 1D pole
int getIndexMapa(Mapa *mapa, int x, int y);

//Funkcia na zistenie, ci na policku nie je prekazka
bool jeVolno(const Mapa *mapa, int x, int y);

//Funkcia na nastavenie policka - bez alebo s prekazkou
void nastavPolicko(Mapa *mapa, int x, int y, bool jePrekazka);

//Kontrola ci sa zovsadial da dostat do stredu
bool jeMapaPriechodna(const Mapa *mapa);

//Funkcia na generovanie prekazok podla hustoty
void generujPrekazky(Mapa *mapa, double hustota);

#endif //NAHODNA_POCHODZKA_POS_MAPA_H