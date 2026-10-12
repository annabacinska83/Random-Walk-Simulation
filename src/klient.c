#include "klient.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include "structures.h"

typedef struct {
    int socket;
    Simulacia_parametre *parametre;
    double cache_sanca[MAX_SIRKA][MAX_VYSKA];
    double cache_kroky[MAX_SIRKA][MAX_VYSKA];
    bool mapa_prekazok[MAX_POLICKA];
    int chodec_x, chodec_y;
    int akt_replikacia;
    int celk_replikacie;
    bool je_sumarny_mod;
    bool zobrazovat_sancu;
} VlaknoData;

//ci prijimam spravny pocet bajtov
int recv_all(int sock, void *buffer, size_t len) {
    size_t prijate = 0;
    char *ptr = (char *)buffer;
    while (prijate < len) {
        ssize_t n = recv(sock, ptr + prijate, len - prijate, 0);
        if (n <= 0) return -1;
        prijate += n;
    }
    return 0;
}

void vykresli_ui(VlaknoData *data) {
    int sirka = data->parametre->sirka_mapy;
    int vyska = data->parametre->vyska_mapy;
    int offset_x = sirka / 2;
    int offset_y = vyska / 2;

    // Vymazanie obrazovky (ANSI)
    printf("\033[2J\033[H\033[?25l");

    printf("=== NAHODNA POCHODZKA - KLIENT ===\n");
    printf("Port: %d | Replikacia: %d/%d | Mod: %s\n",
           PORT_START, data->akt_replikacia, data->celk_replikacie,
           data->je_sumarny_mod ? "SUMARNY (Statistiky)" : "INTERAKTIVNY (Pohyb)");
    printf("------------------------------------------------------------\n");

    for (int y = 0; y < vyska; y++) {
        for (int x = 0; x < sirka; x++) {
            int log_x = x - offset_x;
            int log_y = y - offset_y;


            if (!data->je_sumarny_mod && log_x == data->chodec_x && log_y == data->chodec_y) {
                printf(" [C] ");
            }
            // 2. Cieľ
            else if (log_x == 0 && log_y == 0) {
                printf(" [0] ");
            }
            // 3. Prekážka
            else if (data->mapa_prekazok[y * sirka + x]) {
                printf(" ### ");
            }

            else if (data->je_sumarny_mod) {
                double s = data->cache_sanca[x][y];
                if (s <= 0.0001) printf("  .  ");
                else {
                    if (data->zobrazovat_sancu) printf("%3.0f%% ", s * 100);
                    else printf("%4.1f ", data->cache_kroky[x][y]);
                }
            }

            else {
                printf("  .  ");
            }
        }
        printf("\n");
    }

    printf("------------------------------------------------------------\n");
    printf("OVLADANIE: [m] Zmen mod | [t] Prepni %% / Kroky | [q] Odpojit\n");
    printf("\033[?25h");
    fflush(stdout);
}

void *vlaknoPrijemca(void *arg) {
    VlaknoData *args = (VlaknoData *)arg;
    Typ_dat typ;

    while (recv_all(args->socket, &typ, sizeof(typ)) == 0) {
        switch (typ) {
            case DATA_MAPA: {
                int velkost = args->parametre->sirka_mapy * args->parametre->vyska_mapy;
                recv_all(args->socket, args->mapa_prekazok, velkost * sizeof(bool));
                vykresli_ui(args);
                break;
            }
            case DATA_PROGRESS: {
                Progress_data p;
                if (recv_all(args->socket, &p, sizeof(p)) == 0) {
                    args->akt_replikacia = p.aktualna_replikacia;
                    args->celk_replikacie = p.celkovy_pocet_replikacii;
                    vykresli_ui(args);
                }
                break;
            }
            case DATA_POZICIA_CHODEC: {
                int poz[2];
                if (recv_all(args->socket, poz, sizeof(poz)) == 0) {
                    args->chodec_x = poz[0];
                    args->chodec_y = poz[1];
                    // Ak sme v interaktívnom móde, prekresľujeme pri každom pohybe
                    if (!args->je_sumarny_mod) vykresli_ui(args);
                }
                break;
            }
            case DATA_VYSLEDOK_POLICKA: {
                Vysledok_policka v;
                if (recv_all(args->socket, &v, sizeof(v)) == 0) {
                    int ix = v.x + (args->parametre->sirka_mapy / 2);
                    int iy = v.y + (args->parametre->vyska_mapy / 2);
                    if (ix >= 0 && ix < MAX_SIRKA && iy >= 0 && iy < MAX_VYSKA) {
                        args->cache_sanca[ix][iy] = v.pravdepodobnost;
                        args->cache_kroky[ix][iy] = v.priemerny_pocet_krokov;
                    }
                }
                break;
            }
            case DATA_MOD_ZMENA:
                args->je_sumarny_mod = !args->je_sumarny_mod;
                vykresli_ui(args);
                break;
            case DATA_PARAMETRE:
                recv_all(args->socket, args->parametre, sizeof(Simulacia_parametre));
                break;
            case DATA_KONIEC:
                printf("\n[KONIEC] Simulacia dokoncena. Stlac [q] pre navrat.\n");
                return NULL;
            default: break;
        }
    }
    return NULL;
}

bool je_port_obsadeny(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    struct sockaddr_in adresa = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = inet_addr("127.0.0.1") };
    struct timeval tv = {0, 50000};
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    bool obsadeny = (connect(sock, (struct sockaddr*)&adresa, sizeof(adresa)) == 0);
    close(sock);
    return obsadeny;
}

int najdi_volny_port() {
    for (int p = PORT_START; p <= PORT_END; p++) {
        if (!je_port_obsadeny(p)) return p;
    }
    return -1;
}

void spusti_simulaciu(Simulacia_parametre *parametre, int port) {
    bool je_master = (parametre != NULL);
    Simulacia_parametre par_lokalne;

    if (je_master) {
        par_lokalne = *parametre;
        if (!je_port_obsadeny(port)) {
            if (fork() == 0) {
                char p_str[10]; sprintf(p_str, "%d", port);
                execl("./bin/server", "server", p_str, NULL);
                _exit(1);
            }
            sleep(1);
        }
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adresa = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = inet_addr("127.0.0.1") };

    if (connect(sock, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
        printf("Chyba: Server na porte %d neodpoveda.\n", port);
        return;
    }

    if (je_master) {
        send(sock, &par_lokalne, sizeof(par_lokalne), 0);
    } else {
        if (recv_all(sock, &par_lokalne, sizeof(par_lokalne)) != 0) {
            close(sock); return;
        }
    }

    VlaknoData *v_data = calloc(1, sizeof(VlaknoData));
    v_data->socket = sock;
    v_data->parametre = malloc(sizeof(Simulacia_parametre));
    memcpy(v_data->parametre, &par_lokalne, sizeof(Simulacia_parametre));
    v_data->je_sumarny_mod = true;
    v_data->zobrazovat_sancu = true;

    pthread_t tid;
    pthread_create(&tid, NULL, vlaknoPrijemca, v_data);

    int volba;
    while ((volba = getchar()) != EOF) {
        if (volba == 'q' || volba == 'Q') break;
        if (volba == 'm' || volba == 'M') {
            Typ_dat cmd = DATA_MOD_ZMENA;
            send(sock, &cmd, sizeof(cmd), 0);
            v_data->je_sumarny_mod = !v_data->je_sumarny_mod;
            vykresli_ui(v_data);
        }
        if (volba == 't' || volba == 'T') {
            v_data->zobrazovat_sancu = !v_data->zobrazovat_sancu;
            vykresli_ui(v_data);
        }
    }

    pthread_cancel(tid);
    pthread_join(tid, NULL);
    close(sock);
    free(v_data->parametre);
    free(v_data);
    printf("\033[2J\033[H");
}