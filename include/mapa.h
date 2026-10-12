//
// Created by annab on 31/12/2025.
//

#ifndef NAHODNAPOCHODZKA_MAPA_H
#define NAHODNAPOCHODZKA_MAPA_H
#include <stdio.h>

#include "structures.h"


//Funkcia na vytvorenie mapy
void alokuj_mapu(Mapa *mapa, int sirka, int vyska);

//Funkcia na dealokaciu pamate
void uvolni_mapu(Mapa *mapa);

//Funkcia, ktora prepocita suradnicu v 2D poli x,y na index v 1D poli
int get_index_mapy(const Mapa *mapa, int x, int y);

//Funkcia ktora zisti, ci je na suradnici x,y prekazka
bool je_tam_volno(const Mapa *m, int x, int y);

//Funkcia, ktora na danom indexe nastavi prekazku
void nastav_prekazku(Mapa *mapa, int x, int y, bool je_prekazka);

//Funkcia, ktora vygeneruje prekazky na zaklade percentualneho pokrytia
void generuj_prekazky(Mapa *mapa, double perc_pokrytie);

//Funkcia, ktora ulozi informacie o rozmeroch, prekazkach do suboru
void zapis_mapu_do_suboru(const Mapa *mapa, FILE *subor);

//Funkcia ktora nacita mapu zo suboru
bool nacitaj_mapu_zo_suboru(Mapa *mapa, FILE *subor);
//Fukcia, ktora zisti ci je mapa priechodna
bool je_mapa_priechodna(const Mapa *mapa);
//Funkcia na kontrolu okrajov
int wrap_suradnicu(int hodnota, int min, int max);




#endif //NAHODNAPOCHODZKA_MAPA_H