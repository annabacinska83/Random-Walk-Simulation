#include <stdio.h>
#include "inputHandler.h"

#include <string.h>

#include "structures.h"


// Naplni strukturu pre novu simulaciu defaultnymi hodnotami
void naplnDefaultneParametre(Simulacia_parametre *nova_sim) {
    nova_sim->druh = NOVA;
    nova_sim->sirka_mapy = 10;
    nova_sim->vyska_mapy = 10;
    nova_sim->pocet_replikacii = 500;


    nova_sim->p_smeru[SMER_HORE] = 0.25;
    nova_sim->p_smeru[SMER_DOLE] = 0.25;
    nova_sim->p_smeru[SMER_VLAVO] = 0.25;
    nova_sim->p_smeru[SMER_VPRAVO] = 0.25;
    nova_sim->s_prekazkami = true;
    nova_sim->max_krokov = 150;
    strcpy(nova_sim->vystupny_subor, "rychly_test.bin");
    nova_sim->vstupny_subor[0] = '\0';
    printf("\n[IH] Parametre boli automaticky vyplnene.\n");
}

// Vypise menu do konzoly a vrati cislo volby
// Ak sa zada zly vstup, vraciam -1 aby sa menu mohlo zobrazit znova
int zobrazMenu() {
    int volba = -1;
    printf("\n- Hlavne menu -\n");
    printf("1. Spustit novu simulaciu\n");
    printf("2. Obnovit opatovne simulaciu\n");
    printf("3. Pripojit sa k existujucej simulacii\n");
    printf("4. RYCHLY TEST (automaticke parametre)\n");
    printf("0. Ukoncit program\n");
    printf("Vasa volba: ");

    if (scanf("%d", &volba) != 1) {
        // Vyčistenie vstupneho bufferu po zlom vstupe
        while(getchar() != '\n');
        return -1;
    }
    return volba;
}

// Nacita od pouzivatela parametre pre novu simulaciu
// Kontrolujem hlavne sucet pravdepodobnosti, aby bol blizko 1.0
void getParametreNovejSimulacie(Simulacia_parametre *nova_sim) {
    nova_sim->druh = NOVA;

    // Pri novej simulacii nemam vstupny subor
    nova_sim->vstupny_subor[0] = '\0';

    printf("\n- Nova Simulacia --\n");

    printf("Zadaj sirku sveta: ");
    scanf("%d", &nova_sim->sirka_mapy);

    printf("Zadaj vysku sveta: ");
    scanf("%d", &nova_sim->vyska_mapy);

    // Zapnutie alebo vypnutie prekazok
    int volba_prekazky;
    printf("Chcete generovat mapu s prekazkami? (1 = Ano, 0 = Bez prekazok): ");
    if (scanf("%d", &volba_prekazky) != 1) {
        nova_sim->s_prekazkami = false;
    } else {
        nova_sim->s_prekazkami = (volba_prekazky == 1);
    }

    printf("Zadaj pocet replikacii: ");
    scanf("%d", &nova_sim->pocet_replikacii);

    // Nacitavam pravdepodobnosti v cykle, kym ich sucet nebude sediet
    double suma = 0.0;
    do {
        printf("\nZadaj pravdepodobnosti (sucet musi byt 1.0):\n");

        printf("HORE: ");
        if (scanf("%lf", &nova_sim->p_smeru[SMER_HORE]) != 1) {
            while(getchar() != '\n') {
            }
            continue;
        }

        printf("DOLE: ");
        if (scanf("%lf", &nova_sim->p_smeru[SMER_DOLE]) != 1) {
            while(getchar() != '\n') {
            }
            continue;
        }

        printf("VLAVO: ");
        if (scanf("%lf", &nova_sim->p_smeru[SMER_VLAVO]) != 1) {
            while(getchar() != '\n') {
            }
            continue;
        }

        printf("VPRAVO: ");
        if (scanf("%lf", &nova_sim->p_smeru[SMER_VPRAVO]) != 1) {
            while(getchar() != '\n') {
            }
            continue;
        }

        // Sucet pravdepodobnosti
        suma = nova_sim->p_smeru[0] + nova_sim->p_smeru[1] +
               nova_sim->p_smeru[2] + nova_sim->p_smeru[3];


        if (suma < 0.99 || suma > 1.01) {
            printf("Chyba! Sucet musi byt 1.0 (tvoj je %.2f). Skus znova.\n", suma);
            while(getchar() != '\n') {
            }
            continue;
        }

    } while (suma < 0.999 || suma > 1.001);

    printf("Zadaj maximalny pocet krokov (K): ");
    scanf("%d", &nova_sim->max_krokov);

    // Nazov suboru kam sa ulozi vysledok
    printf("Zadaj nazov vystupneho suboru: ");
    scanf("%255s", nova_sim->vystupny_subor);
}

// Nacita parametre pre opatovnu simulaciu zo suboru
// Potom dovoli zmenit pocet replikacii a nazov noveho vystupu
void getParametreOpatovnejSimulacie(Simulacia_parametre *opatovna) {
    char cesta[256];
    printf("Zadaj nazov suboru pre nacitanie simulacie: ");
    scanf("%255s", cesta);

    // Najprv si overim ze subor existuje
    FILE *f = fopen(cesta, "rb");
    if (!f) {
        printf("Chyba: Subor neexistuje!\n");
        return;
    }

    // Precitam povodne parametre priamo zo suboru
    fread(opatovna, sizeof(Simulacia_parametre), 1, f);
    fclose(f);

    // Prepnem typ simulacie na opatovnu a ulozim cestu k vstupu
    opatovna->druh = OPATOVNA;
    strcpy(opatovna->vstupny_subor, cesta);

    // Len informacne vypisem povodne hodnoty
    printf("Povodne rozmery: %dx%d. Povodne K: %d\n", opatovna->sirka_mapy, opatovna->vyska_mapy, opatovna->max_krokov);

    // Pri opatovnej simulacii chcem nove replikacie a novy vystupny subor
    printf("Zadaj pocet NOVYCH replikacii: ");
    scanf("%d", &opatovna->pocet_replikacii);

    printf("Zadaj nazov NOVEHO vystupneho suboru: ");
    scanf("%255s", opatovna->vystupny_subor);
}
