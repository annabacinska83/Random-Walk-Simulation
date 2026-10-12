#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

#include "simulacia.h"

#include <pthread.h>
#include <sys/socket.h>

#include "chodec.h"
#include "mapa.h"
#include "structures.h"

// Spomalenie pre interaktivny mod, aby sa dal vidiet pohyb
#define USLEEP_KROK 50000

// Spomalenie medzi replikaciami, aby sa sumarny mod dal priebezne vykreslovat
#define USLEEP_REPLIKACIA 80000

// Jednoduchy random v intervale 0 az 1
static inline double nahodne01(void) {
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

// Vyber smeru na zaklade pravdepodobnosti v par p_smeru
// Hranica sa postupne nascitava a prve prekrocenie vyhra
static Smer getSmerPodlaPravdepodobnosti(const Simulacia_parametre *par) {
    double r = nahodne01();
    double hranica = 0.0;

    for (int i = 0; i < 4; i++) {
        hranica += par->p_smeru[i];
        if (r < hranica) {
            return (Smer)i;
        }
    }

    // Zaloha keby sa sucet pravdepodobnosti nerovnal presne 1
    return SMER_VPRAVO;
}

// Ulozi kompletne vysledky do binarneho suboru
// Zapisujem parametre, mapu a potom statistiky zo zdielanej pamate
void uloz_kompletnu_simulaciu(const char *nazov, const Simulacia_parametre *par, const Mapa *mapa, Zdielane_data *zdielane) {
    FILE *file = fopen(nazov, "wb");
    if (!file) {
        perror("Nepodarilo sa vytvorit vystupny subor");
        return;
    }

    // Ulozenie parametrov simulacie
    fwrite(par, sizeof(Simulacia_parametre), 1, file);

    // Ulozenie mapy prekazok a rozmerov mapy
    zapis_mapu_do_suboru(mapa, file);

    // Ulozenie statistik pod mutexom, lebo do nich mozu zapisovat aj ine procesy
    pthread_mutex_lock(&zdielane->mutex);
    fwrite(zdielane->suma_krokov, sizeof(long), MAX_POLICKA, file);
    fwrite(zdielane->pocet_uspechov, sizeof(int), MAX_POLICKA, file);
    pthread_mutex_unlock(&zdielane->mutex);

    fclose(file);
    printf("[SIMULATOR] Vysledky boli ulozene do suboru: %s\n", nazov);
}

// Hlavny vypocet simulacie
// Vysledky zapisujem do zdielanej pamate, server ich potom posiela klientom
void vypocet_simulacie(const Simulacia_parametre *par, const Mapa *mapa, Zdielane_data *zdielane, int socket) {
    (void)socket;

    // Inicializacia zdielanych dat pred startom simulacie
    pthread_mutex_lock(&zdielane->mutex);

    zdielane->aktualne_parametre = *par;
    zdielane->aktualna_replikacia = 0;
    zdielane->simulacia_bezi = true;

    // Vynulovanie statistik pre vsetky policka
    for (int i = 0; i < MAX_POLICKA; i++) {
        zdielane->suma_krokov[i] = 0;
        zdielane->pocet_uspechov[i] = 0;
    }

    // ulozim do zdielanej pamate aj mapu prekazok
    zdielane->aktualne_parametre.pocet_replikacii = par->pocet_replikacii;
    for (int i = 0; i < (par->sirka_mapy * par->vyska_mapy); i++) {
        zdielane->mapa_prekazok[i] = mapa->ptr_mapa[i];
    }

    pthread_mutex_unlock(&zdielane->mutex);

    // Pre kazdu replikaciu prechadzam vsetky startovacie policka
    // Pre kazde start policko ---- simulujem pohyb chodca, kym nedojde do ciela alebo neprekroci max krokov
    for (int rep = 1; rep <= par->pocet_replikacii; rep++) {

        //neukazovalo mi to v interaktivnom mode tak to aktualizujem aj tu
        pthread_mutex_lock(&zdielane->mutex);
        zdielane->aktualna_replikacia = rep;
        pthread_mutex_unlock(&zdielane->mutex);

        for (int y = mapa->min_y; y <= mapa->max_y; y++) {
            for (int x = mapa->min_x; x <= mapa->max_x; x++) {

                // Nepocitam ciel a nepocitam policka s prekazkou
                if ((x == 0 && y == 0) || !je_tam_volno(mapa, x, y)) continue;

                int index_mapy = get_index_mapy(mapa, x, y);


                Chodec chodec;
                chodec_init(&chodec, x, y);

                // Kroky chodca v ramci jedneho pokusu
                while (chodec.je_aktivny && (chodec.pocet_krokov < par->max_krokov)) {
                    chodec_krok(&chodec, getSmerPodlaPravdepodobnosti(par), mapa);

                    // Aktualizujem poziciu chodca v SHM pre interaktivny mod
                    pthread_mutex_lock(&zdielane->mutex);
                    zdielane->chodec_x = chodec.x;
                    zdielane->chodec_y = chodec.y;

                    // Vytiahnem si informaciu o mode a hned odomknem
                    bool je_interaktivny = !zdielane->je_sumarny_mod;
                    pthread_mutex_unlock(&zdielane->mutex);

                    // Ked je interaktivny mod, spomalim
                    if (je_interaktivny) {
                        usleep(USLEEP_KROK);
                    }

                    if (chodec_je_v_cieli(&chodec)) {
                        pthread_mutex_lock(&zdielane->mutex);
                        zdielane->pocet_uspechov[index_mapy]++;
                        zdielane->suma_krokov[index_mapy] += chodec.pocet_krokov;
                        pthread_mutex_unlock(&zdielane->mutex);
                        break;
                    }
                }

                // Priebezne hodnoty
                Vysledok_policka priebezny;
                priebezny.x = x;
                priebezny.y = y;

                pthread_mutex_lock(&zdielane->mutex);
                priebezny.pravdepodobnost = (double)zdielane->pocet_uspechov[index_mapy] / rep;
                priebezny.priemerny_pocet_krokov = (zdielane->pocet_uspechov[index_mapy] > 0) ?
                                                   (double)zdielane->suma_krokov[index_mapy] / zdielane->pocet_uspechov[index_mapy] : 0;
                pthread_mutex_unlock(&zdielane->mutex);

                // TOTO JE STARE, lebo posielanie robi server a simulacia len rata
                // Typ_dat t = DATA_VYSLEDOK_POLICKA;
                // send(socket, &t, sizeof(t), 0);
                // send(socket, &priebezny, sizeof(priebezny), 0);
            }
        }

        // Update progresu po dokonceni replikacie
        pthread_mutex_lock(&zdielane->mutex);
        zdielane->aktualna_replikacia = rep;
        pthread_mutex_unlock(&zdielane->mutex);

        usleep(USLEEP_REPLIKACIA);
    }

    // Po skonceni celeho vypoctu ulozim kompletne vysledky do suboru
    uloz_kompletnu_simulaciu(par->vystupny_subor, par, mapa, zdielane);

    // Zapis ze simulacia uz nebezi
    pthread_mutex_lock(&zdielane->mutex);
    zdielane->simulacia_bezi = false;
    pthread_mutex_unlock(&zdielane->mutex);
}

// Vypis do konzoly  na kontrolu parametrov
void vypis_zakladne_info_simulacie(const Simulacia_parametre *parametre) {
    printf("\n--- Nastavenia simulacie ---\n");
    printf("Rozmery: %dx%d | Replikacie: %d | K: %d\n",
           parametre->sirka_mapy, parametre->vyska_mapy,
           parametre->pocet_replikacii, parametre->max_krokov);
    printf("Pravdepodobnosti: H:%.2f D:%.2f L:%.2f P:%.2f\n",
           parametre->p_smeru[0], parametre->p_smeru[1],
           parametre->p_smeru[2], parametre->p_smeru[3]);
}

// Zapise parametre a mapu, bez statistik
void uloz_simulaciu(const char* cesta, Simulacia_parametre* par, Mapa* mapa) {
    FILE* f = fopen(cesta, "wb");
    if (!f) return;

    fwrite(par, sizeof(Simulacia_parametre), 1, f);
    zapis_mapu_do_suboru(mapa, f);

    fclose(f);
}

// Precitam parametre aj mapu a vratim true ak sa to podarilo
bool nacitaj_simulaciu(const char* cesta, Simulacia_parametre* par, Mapa* mapa) {
    FILE* f = fopen(cesta, "rb");
    if (!f) return false;

    fread(par, sizeof(Simulacia_parametre), 1, f);
    nacitaj_mapu_zo_suboru(mapa, f);

    fclose(f);
    return true;
}
