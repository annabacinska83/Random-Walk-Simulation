#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdbool.h>
#include <pthread.h>

// Rozsah portov, v ktorom hladam volne porty pre simulacie
// Utility si z tohto rozsahu vybera port na spustenie servera
#define PORT_START 56000
#define PORT_END   56100

// Povodny nazov shm, aktualne ale v mojom serveri pouzivam vlastny nazov podla portu
// Toto tu nechavam ako referenciu, aby som vedela co to je
#define SHM_NAME "/os_sim_pochodzka"

// Max rozmery mapy, obmedzenie kvoli velkosti zdielanej pamate a prehladnosti
#define MAX_SIRKA 50
#define MAX_VYSKA 50

// Max pocet policok, pouzivam ho na pevne velke polia v zdielanej pamati
#define MAX_POLICKA (MAX_SIRKA * MAX_VYSKA)

// Hustota prekazok pri generovani mapy
#define HUSTOTA_PREKAZOK 0.1



#pragma pack(push, 1) // Zapne balenie na 1 bajt

typedef struct {
    int x, y;
    double pravdepodobnost;
    double priemerny_pocet_krokov;
} Vysledok_policka;

typedef struct {
    int aktualna_replikacia;
    int celkovy_pocet_replikacii;
} Progress_data;

typedef struct {
    int pocet_replikacii;
    int sirka_mapy, vyska_mapy;
    double p_smeru[4];
    int max_krokov;
    char vystupny_subor[256];
    char vstupny_subor[256];
    int druh;
    bool s_prekazkami;
    bool pre_viacerych;
} Simulacia_parametre;


#pragma pack(pop)






// Typy sprav, ktore si posielam medzi serverom a klientom cez TCP
// Najprv sa posle typ a potom data, ktore k nemu patria
typedef enum {
    DATA_VYSLEDOK_POLICKA,  // posielam statistiku pre jedno konkretne policko
    DATA_PROGRESS,          // posielam aktualnu replikaciu a celkovy pocet replikacii
    DATA_POZICIA_CHODEC,    // posielam aktualnu polohu chodca v interaktivnom mode
    DATA_MOD_ZMENA,         // prikaz od klienta na prepnutie modu sumarny vs interaktivny
    DATA_KONIEC,            // signal ze simulacia konci a uz sa nic dalsie neposle
    DATA_MAPA,              // posielam mapu prekazok
    DATA_PARAMETRE          // posielam parametre simulacie,  pri join alebo echo pri opakovanej simulacii
} Typ_dat;






// Typ simulacie, bud ide o novu alebo o opakovanu
typedef enum { NOVA, OPATOVNA } DruhSimulacie;



// Toto je obsah zdielanej pamate medzi procesmi
typedef struct {
    pthread_mutex_t mutex;              // medziprocesny mutex, aby sa mi data neprepisovali naraz
    bool simulacia_bezi;
    Simulacia_parametre aktualne_parametre; // aktualne parametre jednej simulacie

    int aktualna_replikacia;            // kolko replikacii je uz hotovych
    int chodec_x, chodec_y;             // pozicia chodca pre interaktivny mod

    bool je_sumarny_mod;
    bool mapa_prekazok[MAX_POLICKA];    // ulozena mapa prekazok pre posielanie klientom aj pre vypocet

    // statistiky pre kazde policko, indexujem ich cez index mapy
    long suma_krokov[MAX_POLICKA];      // suma krokov zo vsetkych uspesnych pokusov
    int pocet_uspechov[MAX_POLICKA];    // kolko krat sa chodec dostal do ciela
} Zdielane_data;


// Smery pohybu
typedef enum { SMER_HORE=0, SMER_DOLE=1, SMER_VLAVO=2, SMER_VPRAVO=3 } Smer;


// Chodec v simulacii
typedef struct { int x, y, pocet_krokov; bool je_aktivny; } Chodec;


// Mapa sveta
// ptr_mapa ukazuje na pole bool hodnot pre prekazky
typedef struct { int sirka, vyska; bool *ptr_mapa; int min_x, max_x, min_y, max_y; } Mapa;


// Tabulka posunov pre jednotlive smery, aby som mala jednotny vypocet pohybu
// poradie sedi s enum Smer
static const int pohyb[4][2] = {{0,1}, {0,-1}, {-1,0}, {1,0}};

#endif
