#include <stdbool.h>
#include <pthread.h>


#ifndef NAHODNA_POCHODZKA_POS_STRUKTURY_H
#define NAHODNA_POCHODZKA_POS_STRUKTURY_H

/**
 * Štruktúry
 */

#define PORT 56214
#define SHM_NAME "/os_sim_pochodzka"
#define MAX_SIRKA 50
#define MAX_VYSKA 50
#define MAX_POLICKA (MAX_SIRKA * MAX_VYSKA)
#define SMERY 4

#include <stdbool.h>

typedef enum {
    DATA_MAPA, //posielam mapu prekazok
    DATA_PROGRESS, //posielam aktualnu replikaciu a celkovy pocet replikacii
    DATA_CHODEC_POZICIA, //posielam x,y
    DATA_MOD_ZMENA,
    DATA_VYSLEDOK_POLICKA, //posielam Vysledok_policka
    DATA_KONIEC
}Typ_dat;


typedef struct {
    int x,y;
    double pravdepodobnost_uspechu;
    double priemerny_pocet_krokov;
} Vysledok_policka;

typedef struct {
    int aktualny_pocet_replikacii;
    int celkovy_pocet_replikacii;
} ProgressData;

typedef enum {
    SMER_HORE=0,
    SMER_DOLE=1,
    SMER_VLAVO=2,
    SMER_VPRAVO=3
} Smer;

typedef struct  {
    int x,y;
    int pocetKrokov;
    bool je_aktivny;
} Chodec;

typedef struct {
    int sirka,vyska;
    bool *ptr_mapa; //mapa bool pre policka : true - je to prekazka, false - nema prekazku
    int maxX,minX,maxY,minY;
} Mapa;

typedef enum {
    NOVA,OPATOVNA
} Druh_simulacie;

typedef struct {
    int pocet_replikacii;
    int vyskaMapy,sirkaMapy;
    double p_smeru[SMERY];
    int max_krokov;
    char vtupny_subor[256],vystupny_subor[256];
    Druh_simulacie druh;
}Simulacia_parametre;

typedef struct {
    pthread_mutex_t mutex;
    bool simulacia_bezi;
    bool je_sumarny_mod;
    bool mapa_prekazok[MAX_POLICKA];
    int aktualna_replikacia;
    int chodec_x,chodec_y;

    long suma_krokov[MAX_POLICKA];
    long pocet_uspechov[MAX_POLICKA];

    Simulacia_parametre parametre;


} Zdielana_pamat;

static const int pohyb[SMERY][2] = {{ 0,1}, {0,-1}, {-1,0}, {1,0} };


#endif //NAHODNA_POCHODZKA_POS_STRUKTURY_H