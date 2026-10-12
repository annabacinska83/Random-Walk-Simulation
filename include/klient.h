//
// Created by annab on 01/01/2026.
//

#ifndef NAHODNAPOCHODZKA_KLIENT_H
#define NAHODNAPOCHODZKA_KLIENT_H

// V mojom projekte klient posiela serveru parametre a potom prijima data cez socket

#include "structures.h"


// Overi ci je port obsadeny, typicky cez pokus o pripojenie alebo bind podla implementacie
// Pouzivam to hlavne pri hladani volneho portu na spustenie novej simulacie
bool je_port_obsadeny(int port);


// Najde volny port v definovanom rozsahu
// Toto sa mi hodi, ked chcem spustit vlastny server a nechcem aby sa mi porty bili
int najdi_volny_port();


// Umozni pouzivatelovi vybrat existujucu simulaciu, ku ktorej sa vie pripojit
// V praxi to robi tak, ze pozrie zoznam portov, kde nieco pocuva a ponukne ich
int vyber_existujucu_simulaciu();


// Spusti simulaciu na danom porte so zadanymi parametrami
// Typicky klient posle serveru Simulacia_parametre a server potom spusti simulacny proces
void spusti_simulaciu(Simulacia_parametre *parametre, int port);


// Funkcia pre vlakno, ktore prijima data zo siete
// Bezi oddelene od hlavneho vlakna, aby sa mi nezaseklo ovladanie alebo vypis
void *vlaknoPrijemca(void *arg);

#endif //NAHODNAPOCHODZKA_KLIENT_H
