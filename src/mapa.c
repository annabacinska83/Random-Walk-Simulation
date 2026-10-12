#include "mapa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tento subor riesi mapu sveta a prekazky
// Mapa je v pamati ulozena ako 1D pole boolov, aby sa s tym dobre pracovalo

void alokuj_mapu(Mapa *mapa, int sirka, int vyska) {
    // Ked su rozmery zle, nic nerobim
    if (sirka <=  0 || vyska <= 0) return;

    mapa->sirka = sirka;
    mapa->vyska = vyska;

    // Vypocitam minimalne a maximalne logicke suradnice
    // Pri parnych rozmeroch je rozsah posunuty tak, aby tam stale existovalo policko 0 0
    mapa->min_x = -(sirka / 2);
    mapa->max_x = (sirka % 2 == 0) ? (sirka / 2 - 1) : (sirka / 2);

    mapa->min_y = -(vyska / 2);
    mapa->max_y = (vyska % 2 == 0) ? (vyska / 2 - 1) : (vyska / 2);

    // Alokujem pole prekazok a inicializujem ho na false
    // False znamena volne policko, true znamena prekazka
    mapa->ptr_mapa = calloc(sirka * vyska, sizeof(bool));
}

// Uvolnim pamat mapy a nastavim hodnoty na nulu
void uvolni_mapu(Mapa *mapa) {
    if (mapa->ptr_mapa == NULL) return;

    free(mapa->ptr_mapa);
    mapa->ptr_mapa = NULL;
    mapa->sirka = 0;
    mapa->vyska = 0;
}

// Prevod logickych suradnic x y na index v 1D poli
// Najprv si spravim lokalne suradnice od 0 po sirka alebo vyska
int get_index_mapy(const Mapa *mapa, int x, int y) {
    int lokalne_x = x - mapa->min_x;
    int lokalne_y = y - mapa->min_y;

    // Index je riadok krat sirka plus stlpec
    return (lokalne_y * mapa->sirka) + lokalne_x;
}

// Kontrola ci je na policku volno
// Volno znamena ze v poli je false
bool je_tam_volno(const Mapa *mapa, int x, int y) {
    if (mapa->ptr_mapa == NULL) {
        printf("Mapa je NULL\n");
        return true;
    }

    int index_mapy = get_index_mapy(mapa, x, y);
    return !(mapa->ptr_mapa[index_mapy]);
}

// Nastavim alebo zrusim prekazku na konkretnom policku
void nastav_prekazku(Mapa *mapa, int x, int y, bool je_prekazka) {
    if (mapa->ptr_mapa == NULL) {
        printf("Mapa je NULL\n");
        return;
    }

    int index_mapy = get_index_mapy(mapa, x, y);
    mapa->ptr_mapa[index_mapy] = je_prekazka;
}

// Nahodne generovanie prekazok
// Robim to v cykle, kym mapa nie je priechodna, aby sa dalo dostat vsade
void generuj_prekazky(Mapa *mapa, double perc_pokrytie) {
    if (mapa->ptr_mapa == NULL) {
        printf("Mapa je NULL\n");
        return;
    }

    int pokusy = 0;

    do {
        pokusy++;

        // Najprv vycistim celu mapu
        for (int i = 0; i < mapa->sirka * mapa->vyska; i++) {
            mapa->ptr_mapa[i] = false;
        }

        // Potom prechadzam policka a s urcitou sance nastavim prekazku
        for (int y = mapa->min_y; y <= mapa->max_y; y++) {
            for (int x = mapa->min_x; x <= mapa->max_x; x++) {
                // Ciel 0 0 musi ostat vzdy volny
                if (x == 0 && y == 0) {
                    continue;
                }

                double random = (double)rand() / RAND_MAX;
                if (random < perc_pokrytie) {
                    nastav_prekazku(mapa, x, y, true);
                }
            }
        }

    } while (!je_mapa_priechodna(mapa));

    // Toto si viem zapnut pri debugovani
    // printf("[SERVER] Mapa vygenerovana na %d pokus.\n", pokusy);
}

// Wrap znamena ze ked prejdem cez okraj, objavim sa na druhej strane
int wrap_suradnicu(int hodnota, int min, int max) {
    if (hodnota > max) return min;
    if (hodnota < min) return max;
    return hodnota;
}

// Overenie priechodnosti mapy
// Chcem mat istotu, ze kazde volne policko je dosiahnutelne z 0 0
// Toto je inspirovane flood fill pristupom, tento algoritmus mi pomohla doplnit AI
bool je_mapa_priechodna(const Mapa *mapa) {
    int pocet_policok = mapa->sirka * mapa->vyska;

    // Pole dosiahnutelnych policok, na zaciatku je vsade false
    bool *dosiahnutelne = calloc(pocet_policok, sizeof(bool));
    if (!dosiahnutelne) return false;

    // Startujem z 0 0, tam nikdy nedam prekazku
    dosiahnutelne[get_index_mapy(mapa, 0, 0)] = true;

    bool zmena;

    // Opakujem prechod, kym sa nieco meni
    // Ked sa uz nic nezmeni, mam stabilny stav dosiahnutelnosti
    do {
        zmena = false;

        for (int y = mapa->min_y; y <= mapa->max_y; y++) {
            for (int x = mapa->min_x; x <= mapa->max_x; x++) {

                // Ak je toto policko uz dosiahnutelne, skusim rozsirit dosiahnutelnost na susedov
                if (dosiahnutelne[get_index_mapy(mapa, x, y)]) {
                    for (int smer = 0; smer < 4; smer++) {
                        int posun_x = wrap_suradnicu(x + pohyb[smer][0], mapa->min_x, mapa->max_x);
                        int posun_y = wrap_suradnicu(y + pohyb[smer][1], mapa->min_y, mapa->max_y);

                        int index_nasledujuceho = get_index_mapy(mapa, posun_x, posun_y);

                        if (je_tam_volno(mapa, posun_x, posun_y) && !dosiahnutelne[index_nasledujuceho]) {
                            dosiahnutelne[index_nasledujuceho] = true;
                            zmena = true;
                        }
                    }
                }
            }
        }

    } while (zmena);

    // Nakoniec skontrolujem, ci existuje nejake volne policko, ktore nie je dosiahnutelne
    bool je_mapa_dosiahnutelna = true;
    for (int i = 0; i < pocet_policok; i++) {
        if (mapa->ptr_mapa[i] == false && dosiahnutelne[i] == false) {
            je_mapa_dosiahnutelna = false;
            break;
        }
    }

    free(dosiahnutelne);
    return je_mapa_dosiahnutelna;
}

// Zapis mapy do otvoreneho suboru
// Najprv zapisujem rozmery, potom cele pole prekazok
void zapis_mapu_do_suboru(const Mapa *mapa, FILE *subor) {
    if (mapa == NULL || subor == NULL) return;

    fwrite(&mapa->sirka, sizeof(int), 1, subor);
    fwrite(&mapa->vyska, sizeof(int), 1, subor);
    fwrite(mapa->ptr_mapa, sizeof(bool), mapa->sirka * mapa->vyska, subor);

    // Toto si viem zapnut pri debugovani
    // printf("[MAPA] Mapa %dx%d bola ulozena.\n", mapa->sirka, mapa->vyska);
}

// Nacitanie mapy zo suboru
// Najprv precitam rozmery, potom alokujem mapu a precitam prekazky
bool nacitaj_mapu_zo_suboru(Mapa *mapa, FILE *subor) {
    if (mapa == NULL || subor == NULL) return false;

    int sirka, vyska;

    if (fread(&sirka, sizeof(int), 1, subor) != 1) return false;
    if (fread(&vyska, sizeof(int), 1, subor) != 1) return false;

    // Pre istotu uvolnim staru mapu a vytvorim novu s precitanymi rozmermi
    uvolni_mapu(mapa);
    alokuj_mapu(mapa, sirka, vyska);

    // Precitam vsetky policka do 1D pola
    if (fread(mapa->ptr_mapa, sizeof(bool), sirka * vyska, subor) != (size_t)(sirka * vyska)) {
        return false;
    }

    // printf("[MAPA] %dx%d uspesne nacitana.\n", sirka, vyska);

    return true;
}
