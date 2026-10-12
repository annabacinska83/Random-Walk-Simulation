import socket
import struct
import time
import threading
import random
from gui.constants import STATE_INTERACTIVE, STATE_SUMMARY, STATE_JOIN_LIST

#typu sprav podla structures.h
#aby python rozoznal, aky balik dat prisiel
DATA_VYSLEDOK_POLICKA = 0
DATA_PROGRESS = 1
DATA_POZICIA_CHODEC = 2
DATA_MOD_ZMENA = 3
DATA_KONIEC = 4
DATA_MAPA = 5
DATA_PARAMETRE = 6

# Nastavenie servera -- robim tunely a pripajam sa ako lokalny stroj
SERVER_HOST = "127.0.0.1"

class Sockets:
    def __init__(self, app):
        self.app = app
        self.sock = None
        self.running = False
        self.fmt_params = "=iii4x4di256s256si??6x"
        # '=' standarne zarovnanie
        # 'iii' -> 3x int (pocet_replikacii, sirka, vyska)
        # '4x'  -> 4 prázdne bajty (C-čkový padding, aby double začínal na adrese delitelnej 8)
        # '4d'  -> 4x double (pravdepodobnosti smerov)
        # 'i'   -> 1x int (max_krokov)
        # '256s' -> 256 bajtov (názov výstupného súboru)
        # '256s' -> 256 bajtov (názov vstupného súboru)
        # 'i'   -> 1x int (druh simulácie)
        # '??'  -> 2x bool (s_prekazkami, pre_viacerych)
        # '6x'  -> 6 prázdnych bajtov na konci (zarovnanie na celkovú velkost struktury)



    def get_active_simulations_from_c(self):
        #skenuje porty v rozsahu a riesi problem, ze aj ked server skoncil SSH nan stale bezi
        active_ports = []
        for port in range(56000, 56100):
            #vytvorenie sietoveho socketu
            #AF_INET = ipv4 adresy, SOCK_STREAM = tcp
            temp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            temp_sock.settimeout(0.1)

            try:
                # pokus o spojenie
                result = temp_sock.connect_ex(('127.0.0.1', port))
                if result == 0:
                    # skusim poslat nulovy bajt
                    #ak ssh nie je, tak mi vyhodi chybu
                    temp_sock.send(b'\x00')

                    # dvojita kontrola
                    temp_sock.settimeout(0.1)
                    size = struct.calcsize(self.fmt_params)
                    data = temp_sock.recv(size)
                    if len(data) == size:
                        # Rozbalíme dáta na jednotlivé hodnoty
                        p = struct.unpack(self.fmt_params, data)

                        # p[12] je bool hodnota 'pre_viacerych'
                        # Ak je True, simulácia je verejná a pridáme ju do zoznamu
                        if p[12] == True:
                            active_ports.append(port)
            except (socket.timeout, ConnectionResetError, BrokenPipeError, ConnectionAbortedError):
                # Ak nastane akákoľvek chyba pri komunikácii, server tam nie je
                pass
            except Exception:
                pass
            finally:
                temp_sock.close()
        return active_ports

    def get_free_port_from_c(self) -> int:
        #pomocna funkcia na najdenie free portov
        for port in range(56000, 56004):
            temp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            temp_sock.settimeout(0.2)
            if temp_sock.connect_ex(('127.0.0.1', port)) != 0:
                temp_sock.close()
                return port
            temp_sock.close()
        return 56000


    #pomocna funkcia
    #cita zo socketu kym nedostanem presne n bajtov
    def _recv_exact(self, n: int) -> bytes:
        data = b""
        while len(data) < n:
            chunk = self.sock.recv(n - len(data))
            if not chunk:
                self.running = False
                raise ConnectionError("Server ukončil spojenie.")
            data += chunk
        return data

    #incializuje socket a pripoji k serveru
    def connect_to_server(self, port: int) -> bool:
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5.0)
        try:
            print(f"[SOCKETS] Pripájanie k {SERVER_HOST}:{port}...")
            self.sock.connect((SERVER_HOST, port))
            self.sock.settimeout(None) #prepinam na blokujuci rezim
            self.running = True

            #Ak sme v rezime na pripojenie k simulacii, potrebujeme parametre aby sme vedeli ako vykreslit mapu
            if self.app.state == STATE_JOIN_LIST:
                self.receive_parameters()
                # spustenie prijimacieho vlakna na pozadi
                threading.Thread(target=self.receive_loop, daemon=True).start()
                #ak si zoberem potrebne udaje spustim vlakno, ktore bude na pozadi pocuvat zo socketu

            # Pri NEW_SIM a REPEAT_CONFIG nemusime cakat
            # najprv odosielame parametre my a spustame loop
            return True
        except Exception as e:
            print(f"[SOCKETS] Nepodarilo sa pripojiť: {e}")
            return False

    def receive_parameters(self):
        #prijme a rozbali parametre z C servera
        #kolko a ake typy sa maju v sprave nachadzat
        size = struct.calcsize(self.fmt_params)
        #zo siete nacitam presne tolko bajtov kolko chcem
        raw_params = self._recv_exact(size)
        #zoberiem to co som ziskala a rozbalim to
        p = struct.unpack(self.fmt_params, raw_params)

        #takisto poradie musi sediet s C
        self.app.max_repl = p[0]
        self.app.sirka = p[1]
        self.app.vyska = p[2]
        self.app.active_probs = (p[3], p[4], p[5], p[6])
        self.app.active_max_krokov = p[7]
        print(f"[SOCKETS] Parametre načítané: {self.app.sirka}x{self.app.vyska}")

    def send_parameters(self, s, v, rep, k, h, d, vl, vp, s_prek, s_viac, vystup_subor, vstup_subor="", druh=0):
        #Zabalim parametre to struktury a odoslem ich serveru
        #pridam \0, lebo je to C a v mojom kode mam takuto dlzku
        vystup_b = vystup_subor.encode("utf-8").ljust(256, b"\0")
        vstup_b = vstup_subor.encode("utf-8").ljust(256, b"\0")

        try:
            #vytvorenie balika dat
            data = struct.pack(self.fmt_params, rep, s, v, h, d, vl, vp, k, vystup_b, vstup_b, druh, s_prek, s_viac)
            #poslanie dat
            self.sock.sendall(data)
            import time
            time.sleep(0.5)
            print(f"[SOCKETS] Parametre odoslané ({len(data)} bajtov).")
            threading.Thread(target=self.receive_loop, daemon=True).start()
        except Exception as e:
            print(f"[SOCKETS] Chyba pri balení dát: {e}")

    def change_mode(self):
        #odosielam serveru poziadavku na zmenu modu
        if self.sock:
            try: self.sock.sendall(struct.pack("=i", DATA_MOD_ZMENA))
            except: pass

    #toto je to vlakno, co neustale pocuva na sockete a spracuvava spravy
    def receive_loop(self):
        #print("[NET] Slučka príjmu spustená.")
        try:
            while self.running:
                # 1. Čítame 4 bajty  - TypSpravy ako v C
                raw_head = self._recv_exact(4)
                msg_type = struct.unpack("@i", raw_head)[0]

                # prisla statistika pre policko
                if msg_type == DATA_VYSLEDOK_POLICKA:
                    raw = self._recv_exact(24)
                    x, y, p, kr = struct.unpack("@iidd", raw)
                    self.app.stats_cache[(x, y)] = {"sanca": p, "kroky": kr}
                    # Mala som problem s prepinanim modov
                    #toto je ako poista, ze ak toto prijimam
                    #musi aj moje GUI ukazovat toto
                    if self.app.state != STATE_SUMMARY:
                        print("[S - receive loop]  prepnutie do SUMMARY ")
                        self.app.state = STATE_SUMMARY

                # --- prisiel pocit replikacii
                elif msg_type == DATA_PROGRESS:
                    raw = self._recv_exact(8)
                    akt, celk = struct.unpack("@ii", raw)
                    self.app.curr_repl, self.app.max_repl = akt, celk

                # --- prisli indexy chodca
                elif msg_type == DATA_POZICIA_CHODEC:
                    raw = self._recv_exact(8)
                    x, y = struct.unpack("@ii", raw)
                    self.app.pedestrian_pos = [x, y]
                    self.app.trajectory.append((x, y)) #pridame do trajektorie na  kreslenie ciary
                    # Poistka pre prepnutie do INTERACTIVE
                    if self.app.state != STATE_INTERACTIVE:
                        print("[S - receive loop] prepnutie do INTERACTIVE ")
                        self.app.state = STATE_INTERACTIVE

                # --- zmenia modu
                elif msg_type == DATA_MOD_ZMENA:
                    print(f" SERVER zmenil mód. Aktuálny stav v GUI: {self.app.state}")
                    if self.app.state == STATE_SUMMARY:
                        self.app.state = STATE_INTERACTIVE
                        self.app.trajectory = [] #vynulujem trajektoriu nech sa mu ukazuje od zaciatku
                    else:
                        self.app.state = STATE_SUMMARY
                    print(f"Nový stav aplikácie: {self.app.state}")

                # --- koniec simulacie
                elif msg_type == DATA_KONIEC:
                    print("[NET] Server oznámil koniec simulácie. Aplikácia sa o 10s zavrie.")
                    # Cas ze kedy sa aplikacia vypne odteraz
                    self.app.auto_exit_time = time.time() + 5
                    # tu koncime, server konci
                    self.running = False
                    break


                    # prichadza mapa - binarky zoznam prekazok
                elif msg_type == DATA_MAPA:
                    w, h = getattr(self.app, "sirka", 0), getattr(self.app, "vyska", 0)
                    if w > 0 and h > 0:
                        map_data = self._recv_exact(w * h)
                        self.app.obstacles = list(map_data)
                        print("[NET] Mapa prijatá.")

                # --- pri opatovnej simulacii my dostavame parametre od serveru
                elif msg_type == DATA_PARAMETRE:
                    self.receive_parameters()

        except Exception as e:
            print(f"[NET] KRITICKÁ CHYBA V SLUČKE: {e}")
            import traceback
            traceback.print_exc()
        finally:
            self.running = False