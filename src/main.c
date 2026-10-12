#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "inputHandler.h"
#include "klient.h"
#include "structures.h"

// Funkcia na vyčistenie klávesnicového bufferu
void vycisti_vstup() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    int port = -1;
    bool bezi_program = true;
    Simulacia_parametre parametre;

    printf("Startuje proces KLIENT...\n");

    while (bezi_program) {
        int volba = zobrazMenu();

        switch (volba) {
            case 1:
                // NOVA SIMULACIA
                port = najdi_volny_port();
                if (port == -1) {
                    printf("Chyba: Vsetky porty v rozsahu (56000-56100) su obsadene!\n");
                } else {
                    printf("Vytvaram novu simulaciu na porte: %d\n", port);
                    getParametreNovejSimulacie(&parametre);
                    spusti_simulaciu(&parametre, port);
                }
                break;

            case 2:
                // OPATOVNA SIMULACIA
                port = najdi_volny_port();
                if (port == -1) {
                    printf("Chyba: Nie je volny ziadny port pre start servera!\n");
                } else {
                    getParametreOpatovnejSimulacie(&parametre);
                    // parametre sa nacitali zo suboru v inputHandleri
                    spusti_simulaciu(&parametre, port);
                }
                break;

            case 3:
                // PRIPOJENIE K EXISTUJUCEJ
                printf("\nHladam beziace simulacie na localhoste...\n");
                int najdene[20];
                int pocet = 0;

                for (int p = PORT_START; p <= PORT_END && pocet < 20; p++) {
                    if (je_port_obsadeny(p)) {
                        najdene[pocet] = p;
                        printf("%d. Simulacia bezi na porte %d\n", pocet + 1, p);
                        pocet++;
                    }
                }

                if (pocet == 0) {
                    printf("Nenasli sa ziadne beziace simulacie.\n");
                } else {
                    int vyber;
                    printf("Vyber si cislo simulacie (1-%d): ", pocet);
                    if (scanf("%d", &vyber) == 1 && vyber >= 1 && vyber <= pocet) {
                        vycisti_vstup(); // Odstranime \n po scanf
                        // Pri join posielame NULL ako parametre
                        spusti_simulaciu(NULL, najdene[vyber - 1]);
                    } else {
                        printf("Neplatny vyber.\n");
                        vycisti_vstup();
                    }
                }
                break;

            case 4:
                // RYCHLY TEST
                port = najdi_volny_port();
                if (port != -1) {
                    printf("Rychly test na porte %d...\n", port);
                    naplnDefaultneParametre(&parametre);
                    spusti_simulaciu(&parametre, port);
                } else {
                    printf("Chyba: Porty su plne.\n");
                }
                break;

            case 0:
                bezi_program = false;
                break;

            default:
                printf("Nespravna volba, skus znova.\n");
                // Ak uzivatel zadal pismeno namiesto cisla, scanf zlyha a buffer treba vycistit
                vycisti_vstup();
                break;
        }
    }

    printf("Program sa ukoncil. Dovidenia!\n");
    return 0;
}