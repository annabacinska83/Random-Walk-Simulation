#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "structures.h"
#include "simulacia.h"
#include "mapa.h"

// Server prijima klientov cez TCP socket, spravuje zdielanu pamat a spusta simulaciu


// Vytvorim a inicializujem zdielanu pamat pre konkretny port
// Pouzivam shm_open a mmap, aby mali vsetky procesy pristup k rovnakym datam
// Mutex nastavujem ako process shared, aby fungoval medzi procesmi
Zdielane_data* inicializuj_zdielane_data(const char* shm_name) {
    // Pred startom vymazem staru pamet, keby ostala po pade programu
    shm_unlink(shm_name);

    // Otvorenie alebo vytvorenie objektu zdielanej pamate
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open zlyhal");
        exit(1);
    }

    // Nastavim velkost segmentu presne na velkost struktury Zdielane_data
    if (ftruncate(fd, sizeof(Zdielane_data)) == -1) {
        perror("ftruncate zlyhal");
        exit(1);
    }

    // Namapujem zdielanu pamat do adresneho priestoru procesu
    Zdielane_data *zdielane = mmap(NULL, sizeof(Zdielane_data),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, 0);

    if (zdielane == MAP_FAILED) {
        perror("mmap zlyhal");
        exit(1);
    }
    close(fd);

    // Inicializujem mutex tak, aby bol pouzitelny medzi procesmi
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&zdielane->mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);

    // Nastavim vychodzie hodnoty v zdielanej pamati
    zdielane->simulacia_bezi = false;
    zdielane->je_sumarny_mod = true;

    // Pre istotu vynulujem statistiky, aby tam neostali nahodne hodnoty
    for (int i = 0; i < MAX_POLICKA; i++) {
        zdielane->suma_krokov[i] = 0;
        zdielane->pocet_uspechov[i] = 0;
    }

    return zdielane;
}

// Tato funkcia bezi v procese spravcu klienta
// Najprv vyriesi, ci je klient master alebo joiner
// Potom posle mapu a v cykle streamuje data podla aktualneho modu
void obsluhuj_klienta(int klient_socket, Zdielane_data *zdielane) {
    Simulacia_parametre par;
    bool start_sim = false;
    bool som_master = false;

    // Ak simulacia este nebezi, prvy klient posle parametre a stane sa masterom
    pthread_mutex_lock(&zdielane->mutex);
    if (!zdielane->simulacia_bezi) {
        pthread_mutex_unlock(&zdielane->mutex);

        // Cakam na parametre od klienta
        if (recv(klient_socket, &par, sizeof(par), 0) <= 0) return;

        // Dvojita kontrola aby dvaja klienti naraz nespustili simulaciu
        pthread_mutex_lock(&zdielane->mutex);
        if (!zdielane->simulacia_bezi) {
            zdielane->simulacia_bezi = true;
            som_master = true;
            start_sim = true;

            // Pri opatovnej simulacii nacitam parametre a mapu zo suboru hned tu
            // Potom poslem klientovi echo balik so spravnymi rozmermi
            if (par.druh == 1) {
                Mapa m_temp;
                m_temp.ptr_mapa = NULL;

                if (nacitaj_simulaciu(par.vstupny_subor, &par, &m_temp)) {
                    zdielane->aktualne_parametre = par;
                    zdielane->aktualna_replikacia = 0;

                    // Skopirujem mapu do SHM, aby ju videli vsetci klienti
                    memcpy(zdielane->mapa_prekazok, m_temp.ptr_mapa,
                           par.sirka_mapy * par.vyska_mapy * sizeof(bool));
                    uvolni_mapu(&m_temp);

                    // Poslem typ spravy a potom parametre
                    Typ_dat t_par = DATA_PARAMETRE;
                    send(klient_socket, &t_par, sizeof(t_par), MSG_NOSIGNAL);
                    send(klient_socket, &par, sizeof(par), MSG_NOSIGNAL);
                }
            } else {
                // Pri novej simulacii len ulozim parametre do SHM
                zdielane->aktualne_parametre = par;
            }
        } else {
            // Ak niekto medzicasom spustil simulaciu, tak si vezmem aktualne parametre
            par = zdielane->aktualne_parametre;
        }
        pthread_mutex_unlock(&zdielane->mutex);
    } else {
        // Ak simulacia uz bezi, klient je joiner a dostane aktualne parametre
        par = zdielane->aktualne_parametre;
        pthread_mutex_unlock(&zdielane->mutex);
        send(klient_socket, &par, sizeof(par), MSG_NOSIGNAL);
    }

    // Ak som master, spustim simulaciu v samostatnom procese
    if (start_sim) {
        if (fork() == 0) {
            // Dieta robi len vypocet a zapisuje do zdielanej pamate
            Mapa m;
            alokuj_mapu(&m, par.sirka_mapy, par.vyska_mapy);

            // Zoberiem mapu zo SHM, aby simulacia mala rovnake prekazky ako klienti
            pthread_mutex_lock(&zdielane->mutex);
            memcpy(m.ptr_mapa, zdielane->mapa_prekazok,
                   par.sirka_mapy * par.vyska_mapy * sizeof(bool));
            pthread_mutex_unlock(&zdielane->mutex);

            // Pri novej simulacii mozem vygenerovat prekazky a ulozit startovaci subor
            if (par.druh == 0) {
                if (par.s_prekazkami) generuj_prekazky(&m, HUSTOTA_PREKAZOK);
                uloz_simulaciu(par.vystupny_subor, &par, &m);

                // Ulozim mapu aj do SHM, aby joineri videli to iste
                pthread_mutex_lock(&zdielane->mutex);
                memcpy(zdielane->mapa_prekazok, m.ptr_mapa,
                       m.sirka * m.vyska * sizeof(bool));
                pthread_mutex_unlock(&zdielane->mutex);
            }

            // Spustim vypocet simulacie
            vypocet_simulacie(&par, &m, zdielane, -1);
            uvolni_mapu(&m);
            _exit(0);
        }
    }

    // Pri novej simulacii necham kratky cas aby sa stihla vygenerovat mapa
    if (par.druh == 0) usleep(100000);

    // Klientovi najprv poslem typ spravy mapa a potom samotne data mapy
    Typ_dat t_mapa = DATA_MAPA;
    send(klient_socket, &t_mapa, sizeof(t_mapa), MSG_NOSIGNAL);

    pthread_mutex_lock(&zdielane->mutex);
    int sirka = zdielane->aktualne_parametre.sirka_mapy;
    int vyska = zdielane->aktualne_parametre.vyska_mapy;
    size_t velkost = (size_t)sirka * (size_t)vyska * sizeof(bool);
    send(klient_socket, zdielane->mapa_prekazok, velkost, MSG_NOSIGNAL);
    pthread_mutex_unlock(&zdielane->mutex);

    // Posledna odoslana replikacia, aby som neposielala cele pole stale dookola
    int posledna_odoslana_rep = -1;

    // Hlavna slucka spravcu klienta
    // Prijimam prikazy a podla modu posielam sumarne alebo interaktivne data
    while (1) {
        // Kontrola prikazov od klienta bez blokovania
        Typ_dat prikaz;
        if (recv(klient_socket, &prikaz, sizeof(prikaz), MSG_DONTWAIT) > 0) {
            if (prikaz == DATA_MOD_ZMENA) {
                pthread_mutex_lock(&zdielane->mutex);
                zdielane->je_sumarny_mod = !zdielane->je_sumarny_mod;
                pthread_mutex_unlock(&zdielane->mutex);
            }
        }

        // Nacitanie stavu zo SHM
        pthread_mutex_lock(&zdielane->mutex);
        bool bezi = zdielane->simulacia_bezi;
        bool sumarny = zdielane->je_sumarny_mod;
        int akt_rep = zdielane->aktualna_replikacia;
        int celk_rep = zdielane->aktualne_parametre.pocet_replikacii;

        int c_x = zdielane->chodec_x;
        int c_y = zdielane->chodec_y;

        int m_x = -(sirka / 2);
        int m_y = -(vyska / 2);
        pthread_mutex_unlock(&zdielane->mutex);

        // Odosielanie dat podla modu
        if (sumarny) {
            // Najprv poslem progres replikacii
            Typ_dat t_progres = DATA_PROGRESS;
            Progress_data d_progres = {akt_rep, celk_rep};
            send(klient_socket, &t_progres, sizeof(t_progres), MSG_NOSIGNAL);
            send(klient_socket, &d_progres, sizeof(d_progres), MSG_NOSIGNAL);

            // Ak pribudla nova replikacia, poslem statistiky pre vsetky policka
            if (akt_rep > posledna_odoslana_rep) {
                pthread_mutex_lock(&zdielane->mutex);
                for (int i = 0; i < (sirka * vyska); i++) {
                    // Prekazky neposielam, lebo tie klient uz pozna z DATA_MAPA
                    if (zdielane->mapa_prekazok[i]) continue;

                    int rx = (i % sirka) + m_x;
                    int ry = (i / sirka) + m_y;
                    if (rx == 0 && ry == 0) continue;

                    Vysledok_policka v;
                    v.x = rx;
                    v.y = ry;

                    // Pozor na delenie nulou pri prvych iteraciach
                    int div = (akt_rep > 0) ? akt_rep : 1;
                    v.pravdepodobnost = (double)zdielane->pocet_uspechov[i] / div;
                    v.priemerny_pocet_krokov = (zdielane->pocet_uspechov[i] > 0) ?
                        (double)zdielane->suma_krokov[i] / zdielane->pocet_uspechov[i] : 0;

                    Typ_dat t_vys = DATA_VYSLEDOK_POLICKA;
                    send(klient_socket, &t_vys, sizeof(t_vys), MSG_NOSIGNAL);
                    send(klient_socket, &v, sizeof(v), MSG_NOSIGNAL);
                }
                pthread_mutex_unlock(&zdielane->mutex);
                posledna_odoslana_rep = akt_rep;
            }
        } else {
            // Interaktivny mod posiela len poziciu chodca pre vykreslenie pohybu
            Typ_dat t_chodec = DATA_POZICIA_CHODEC;
            int poz[2] = {c_x, c_y};
            send(klient_socket, &t_chodec, sizeof(t_chodec), MSG_NOSIGNAL);
            send(klient_socket, poz, sizeof(poz), MSG_NOSIGNAL);
        }

        // Ak simulacia uz nebezi, ukoncim streamovanie
        if (!bezi) break;

        // V sumarnom mode posielam pomalsie, v interaktivnom rychlejsie
        usleep(sumarny ? 100000 : 30000);
    }

    // Na konci poslem klientovi posledny signal ze simulacia skoncila
    Typ_dat t_koniec = DATA_KONIEC;
    send(klient_socket, &t_koniec, sizeof(t_koniec), MSG_NOSIGNAL);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Port beriem z argumentu, aby som vedela spustit viac serverov naraz
    int port;
    if (argc > 1) port = atoi(argv[1]);
    else port = PORT_START;

    // Kazdy server ma vlastny nazov zdielanej pamate podla portu
    char shm_custom_name[50];
    sprintf(shm_custom_name, "/os_sim_%d", port);

    Zdielane_data *zdielane = inicializuj_zdielane_data(shm_custom_name);

    // TCP server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    // Nastavenie socket option, aby som vedela rychlejsie znovu pouzit port
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in adresa = {0};
    adresa.sin_family = AF_INET;
    adresa.sin_addr.s_addr = inet_addr("127.0.0.1");;
    adresa.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
        perror("bind"); exit(1);
    }

    if (listen(server_fd, 5) < 0) { perror("listen"); exit(1); }

    // Socket nastavim na nonblocking, aby som vedela priebezne kontrolovat stav simulacie
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags == -1) { perror("fcntl(F_GETFL)"); exit(1); }
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL O_NONBLOCK)");
        exit(1);
    }

    // Sledujem ci uz simulacia raz bezala, aby som vedela server ukoncit po jej skonceni
    bool simulacia_uz_raz_bezala = false;
    int pocet_pripojenych_klientov = 0;

    while (1) {
        struct sockaddr_in klient_adr;
        socklen_t len = sizeof(klient_adr);
        int novy_socket = accept(server_fd, (struct sockaddr *)&klient_adr, &len);

        // Ak accept nema klienta, len pockam a robim udrzbu
        if (novy_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {

                // Upratanie zombie procesov po potomkoch
                while (waitpid(-1, NULL, WNOHANG) > 0) {
                    if (pocet_pripojenych_klientov > 0) {
                        pocet_pripojenych_klientov--;
                    }
                }

                // Zistim ci simulacia bezi, aby som vedela ukoncit server ked skonci
                pthread_mutex_lock(&zdielane->mutex);
                bool bezi = zdielane->simulacia_bezi;
                pthread_mutex_unlock(&zdielane->mutex);

                if (bezi) {
                    simulacia_uz_raz_bezala = true;
                }

                if (simulacia_uz_raz_bezala && !bezi) {
                    printf("[SERVER] Simulacia skoncila. Ukoncujem server...\n");
                    break;
                }

                usleep(50 * 1000);
                continue;
            }

            usleep(50 * 1000);
            continue;
        }

        // Kontrola ci simulacia nie je privatna a ci sa uz niekto nepripojil
        pthread_mutex_lock(&zdielane->mutex);
        bool sim_bezi = zdielane->simulacia_bezi;
        bool simulacia_je_privatna = sim_bezi ? !zdielane->aktualne_parametre.pre_viacerych : false;
        pthread_mutex_unlock(&zdielane->mutex);

        if (sim_bezi && simulacia_je_privatna && pocet_pripojenych_klientov >= 1) {
            printf("[SERVER] Odmietam pripojenie: simulacia je len pre jedneho pouzivatela.\n");
            close(novy_socket);
            continue;
        }

        // Vytvorim potomka, ktory bude obsluhovat konkretneho klienta
        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            obsluhuj_klienta(novy_socket, zdielane);
            close(novy_socket);
            _exit(0);
        }

        // Rodic zatvori socket a dalej prijima dalsich klientov
        close(novy_socket);
        if (pid > 0) pocet_pripojenych_klientov++;
    }

    // Upratanie zdielanej pamate a ukoncenie servera
    printf("[SERVER] Upratujem zdielanu pamat a koncim.\n");
    shm_unlink(shm_custom_name);
    close(server_fd);
    return 0;
}

