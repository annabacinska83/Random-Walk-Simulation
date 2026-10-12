
import sys
import time
from .ui_elements import *
from .sockets import Sockets
from . import ui_factory as UIF

class SimulationApp:
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((WIDTH, HEIGHT))
        pygame.display.set_caption(CAPTION)
        self.clock = pygame.time.Clock()
        self.auto_exit_time = None

        # --- FONTY
        self.font_title = pygame.font.SysFont("Arial", 36, bold=True)
        self.font_button = pygame.font.SysFont("Arial", 20, bold=True)
        self.font_subtitle = pygame.font.SysFont("Arial", 24)
        self.font_subtitle_small = pygame.font.SysFont("Arial", 20)

        self.font_mode = pygame.font.SysFont("Arial", 32, bold=True)
        self.font_repl = pygame.font.SysFont("Arial", 48, bold=True)

        self.font_interactive_mode = pygame.font.SysFont("Arial", 30, bold=True)
        self.font_stat_bold = pygame.font.SysFont("Arial", 22, bold=True)
        self.font_stat_normal = pygame.font.SysFont("Arial", 20)
        self.font_error = pygame.font.SysFont("Arial", 18, bold=True)

        # --- UI cez factory
        self.state = STATE_MENU
        self.buttons = UIF.make_main_menu_buttons(self.font_button)
        self.inputs = UIF.make_inputs(self.font_button)
        self.repeat_inputs = UIF.make_repeat_inputs(self.font_button)  #repeat

        self.form_buttons = UIF.make_form_buttons(self.font_button)
        self.simulation_buttons = UIF.make_simulation_buttons(self.font_button)
        self.btn_prekazky, self.btn_viacerych = UIF.make_toggle_buttons(self.font_button)
        self.btn_refresh = UIF.make_refresh_button(self.font_button)
        self.back_to_menu_btn = UIF.make_back_button(self.font_button)

        # join obrazovka
        self.join_buttons = []
        #farba akoou chodec zacina
        self.walker_color = (0, 255, 255)
        # chybové hlášky
        self.error_msg = ""
        # dáta simulácie
        self.curr_repl = 0
        self.max_repl = 0
        self.selected_cell = None
        self.trajectory = []
        self.obstacles = []          #  list 0/1
        self.pedestrian_pos = [0, 0]
        self.stats_cache = {}

        # toggles - tie co sa menia ked ich kliknem
        self.s_prekazkami = True
        self.pre_viacerych = True

        # socket wrapper
        self.sock = Sockets(self)

        # ku kazdemu stavu priradujem funkciu
        self.state_handlers = {
            STATE_MENU: self.handle_menu_event,
            STATE_NEW_SIM: self.handle_new_sim_event,
            STATE_SUMMARY: self.handle_sim_event,
            STATE_INTERACTIVE: self.handle_sim_event,
            STATE_JOIN_LIST: self.handle_join_list_event,
            STATE_REPEAT_CONFIG: self.handle_repeat_config_event,
        }

        self.state_drawers = {
            STATE_MENU: self.draw_menu,
            STATE_NEW_SIM: self.draw_new_sim_config,
            STATE_SUMMARY: self.draw_summary_mode,
            STATE_INTERACTIVE: self.draw_interactive_mode,
            STATE_JOIN_LIST: self.draw_join_list,
            STATE_REPEAT_CONFIG: self.draw_repeat_config,
        }

        #premenna na to ci chcem zobrazovat kroky alebo sancu
        self.summary_display_type = "sanca"
        self.current_port = None

    # ----------------- pomocne funkcie  -----------------
    def draw_header(self, x=50, y=50, subtitle_size="big"):
        # Klasicky titulok a podtitulok
        title_surf = self.font_title.render(TITLE, True, WHITE)
        self.screen.blit(title_surf, (x, y))

        sub_font = self.font_subtitle if subtitle_size == "big" else self.font_subtitle_small
        sub_surf = sub_font.render(SUBTITLE, True, WHITE)
        self.screen.blit(sub_surf, (x, y + 45))

        # Zobrazenie portu
        if self.current_port:
            port_text = f"Port : {self.current_port}"
            port_surf = self.font_subtitle_small.render(port_text, True, WHITE)
            self.screen.blit(port_surf, (x, y + 75))

        # Zobrazim to len ak sme v simulacii
        if self.state in [STATE_SUMMARY, STATE_INTERACTIVE]:
            w = getattr(self, "sirka", 0)
            h = getattr(self, "vyska", 0)
            k = getattr(self, "active_max_krokov", 0)
            probs = getattr(self, "active_probs", (0, 0, 0, 0))

            # Sformatujeme text parametrov
            param_str = f"Mapa: {w}x{h} | Max krokov: {k} | Pravdepodobnosti: [H:{probs[0]:.2f}, D:{probs[1]:.2f}, L:{probs[2]:.2f}, P:{probs[3]:.2f}]"

            # Vykreslim to mensim pismom pod port
            param_surf = self.font_stat_normal.render(param_str, True, YELLOW)
            self.screen.blit(param_surf, (x, y -30))

    #vycisitenie starych dat
    def reset_sim_view(self):
        self.selected_cell = None
        self.trajectory = []
        self.obstacles = []
        self.pedestrian_pos = [0, 0]
        self.stats_cache = {}
        self.curr_repl = 0
        self.max_repl = 0
        self.active_probs = (0, 0, 0, 0)
        self.active_max_krokov = 0



    #vypocet rozmerov mriezky
    #snazim sa prisposobit velkost buniek - cell_w, cell_h, tak aby aj sa aj mapa zmestila do GRID_AREA_SIZE
    #tiez pocita min_x a max_y aby stred sedel na 0,0
    def get_grid_geometry(self):
        # Použivam rozmery, ktoré prišli zo servera
        cols = getattr(self, "sirka", 0)
        rows = getattr(self, "vyska", 0)

        if cols <= 0 or rows <= 0:
            return None

        grid_area_size = GRID_AREA_SIZE
        start_x, start_y = GRID_START_X, GRID_START_Y

        # Skalovanie
        cell_w = grid_area_size // cols
        cell_h = grid_area_size // rows

        # Limit presne ako v c
        min_x = -(cols // 2)
        min_y = -(rows // 2)

        grid_rect = pygame.Rect(start_x, start_y, cols * cell_w, rows * cell_h)

        return grid_rect, start_x, start_y, cols, rows, cell_w, cell_h, min_x, min_y

    #C - 0,0 vlavo hore, Y raste nahor
    #python - 0,0 vlavo hore, Y rastie nadol
    #tato funkcia to prevadza
    def logic_to_pixel(self, lx, ly):
        geo = self.get_grid_geometry()

        if geo is None:
            return 0, 0

        grid_rect, start_x, start_y, cols, rows, cell_w, cell_h, min_x, min_y = geo

        # Stlpec je posun od minima dolava
        c = lx - min_x
        # riadok je posun od minima ale inverzne - (rows - 1 - posun)
        r = (rows - 1) - (ly - min_y)

        px = start_x + c * cell_w + cell_w // 2
        py = start_y + r * cell_h + cell_h // 2
        return px, py

    #iteruje cez moj zoznam tlacidiel a kontroluje ci niekto neklikol, nehovuje, preto niekedy mi to seka
    def handle_button_dict(self, buttons: dict, event, mouse_pos):
        for key, btn in buttons.items():
            btn.check_hovered(mouse_pos)
            if btn.is_clicked(event, mouse_pos):
                return key
        return None

    # ----------------- Hlavny cyklus celej aplikacie  -----------------
    def run(self):
        while True:
            #dokym bezi kreslim obrazovku, obslujem mys - 60 snimok za sekundu
            mouse_pos = pygame.mouse.get_pos()
            self.handle_events(mouse_pos)
            self.draw()
            self.clock.tick(60)

    #podla udalosti urci handlera ktory sa o nu postara
    def handle_events(self, mouse_pos):
        #spracuje vsetky udalosti postupne
        for event in pygame.event.get():
            #ak je nejaka skoncenie, koncim
            if event.type == pygame.QUIT:
                self.quit_app()
                return

            #self.state = aktualny stav obrazovku
            #self.state_handlers = slovnik kde mam stav, funkciu ktora spracuje tu udalost pre dany stav
            handler = self.state_handlers.get(self.state)
            #ak pre dany stav existuje handler, zavola sa
            #posielam mu event a poziciu mysi
            if handler:
                handler(event, mouse_pos)

    #hlavna funckia na renderovanie obrazovky
    def draw(self):
        self.screen.fill(BLACK)

        # ziskane funkciu, ktoru som si defiovala, pre aktualny stav
        draw_fn = self.state_drawers.get(self.state, self.draw_menu)
        draw_fn()

        #logika pre automaticke ukoncenie , zobrazenie odpoctu casu
        if self.auto_exit_time is not None:
            current_time = time.time()
            remaining = int(self.auto_exit_time - current_time)

            if remaining > 0:
                # Vykreslim vypis
                countdown_text = f"SIMULÁCIA SKONČILA. UKONČENIE O: {remaining}s"
                surf = self.font_stat_bold.render(countdown_text, True, RED)
                rect = surf.get_rect(center=(WIDTH // 2, 30))
                # cierne pozadie
                pygame.draw.rect(self.screen, BLACK, rect.inflate(20, 10))
                self.screen.blit(surf, rect)
            else:
                #cas vyprasal, volam funkciu na vypnutie
                print("[GUI] 10 sekúnd uplynulo. Vypínam aplikáciu.")
                self.quit_app()

        pygame.display.flip()

    # ---------------- obsluha udalosti - handlers  -----------------
    def handle_menu_event(self, event, mouse_pos):
        key = self.handle_button_dict(self.buttons, event, mouse_pos)
        if key:
            self.process_menu_click(key)

    def handle_new_sim_event(self, event, mouse_pos):
        # #pre kazdy input zavolam spracovanie udalosti
        for inp in self.inputs.values():
            inp.handle_event(event)

        # prepínače
        if self.btn_prekazky.is_clicked(event, mouse_pos):
            self.s_prekazkami = not self.s_prekazkami
            self.btn_prekazky.text =   f"{'PREKÁŽKY' if self.s_prekazkami else 'NIE PREKÁŽKY'}"
            self.btn_prekazky.color = TEAL if self.s_prekazkami else LIGHT_GRAY

        if self.btn_viacerych.is_clicked(event, mouse_pos):
            self.pre_viacerych = not self.pre_viacerych
            self.btn_viacerych.text = f"{'N KLIENTOV' if self.pre_viacerych else '1 KLIENT'}"
            self.btn_viacerych.color = GREEN if self.pre_viacerych else LIGHT_GRAY

        # spodné tlačidlá
        key = self.handle_button_dict(self.form_buttons, event, mouse_pos)
        if key == "back":
            self.state = STATE_MENU
        elif key == "start":
            self.validate_and_start()

    def handle_repeat_config_event(self, event, mouse_pos):
        #pre kazdy input zavolam spracovanie udalosti
        for inp in self.repeat_inputs.values():
            inp.handle_event(event)

        #prepinac
        if self.btn_viacerych.is_clicked(event, mouse_pos):
            self.pre_viacerych = not self.pre_viacerych
            self.btn_viacerych.text = f" {'N KLIENTOV' if self.pre_viacerych else '1 KLIENT'}"
            self.btn_viacerych.color = GREEN if self.pre_viacerych else LIGHT_GRAY

        key = self.handle_button_dict(self.form_buttons, event, mouse_pos)
        if key == "back": self.state = STATE_MENU
        elif key == "start": self.validate_and_start_repeat()

    def handle_sim_event(self, event, mouse_pos):
        key = self.handle_button_dict(self.simulation_buttons, event, mouse_pos)
        #detekcia kliknutia na prepnutie statistiky

        if key == "stop":
            self.state = STATE_MENU
            self.sock.running = False
            #aby port nesvietil uz v menu
            self.current_port = None
            return
        elif key == "change":
            # posielam spravu serveru
            self.sock.change_mode()
            print("[GUI] Žiadosť o zmenu módu odoslaná serveru.")
            return
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                # Ak kliknem hocikde inde mimo tlacidiel
                if self.summary_display_type == "sanca":
                    self.summary_display_type = "kroky"
                else:
                    self.summary_display_type = "sanca"
                    print(f"[GUI] Prepnutý typ zobrazenia na: {self.summary_display_type}")
        # klik do mriežky iba keď je INTERACTIVE a iba pri kliknutí --- toto uz mi nefunguje
        if self.state == STATE_INTERACTIVE and event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            self.process_grid_click(mouse_pos)




    def handle_join_list_event(self, event, mouse_pos):
        #refreshnutie zoznamu aktivnych
        self.btn_refresh.check_hovered(mouse_pos)

        if self.btn_refresh.is_clicked(event, mouse_pos):
            active_ports = self.sock.get_active_simulations_from_c()
            self.create_join_buttons(active_ports)
            return

        # klik na port tlačidlo
        for btn, port in self.join_buttons:
            btn.check_hovered(mouse_pos)
            if btn.is_clicked(event, mouse_pos):
                self.reset_sim_view()

                #pripoj sa - socket naplnit sirka, vyska, max repl
                if self.sock.connect_to_server(port):
                    self.current_port = port
                    self.state = STATE_SUMMARY
                    print(f"[GUI] uspesne pripojeny k portu {port}")
                else:
                    self.error_msg = "CHYBA: Nepodarilo sa spojiť so serverom!"
                return

        self.back_to_menu_btn.check_hovered(mouse_pos)
        if self.back_to_menu_btn.is_clicked(event, mouse_pos):
            self.state = STATE_MENU

    # ----------------- Drawers -----------------
    def draw_menu(self):
        self.draw_header(50, 50, "big")
        for btn in self.buttons.values():
            btn.draw(self.screen)

    def draw_new_sim_config(self):
        self.draw_header(185, 30, "small")

        self.screen.blit(self.font_button.render("Zadajte parametre :", True, WHITE), (180, 150))
        self.screen.blit(self.font_button.render("Pravdepodobnosti pre smery :", True, WHITE), (500, 140))

        labels_left = [
            ("Port servera:", 110),
            ("Šírka mapy(max 50):", 175),
            ("Výška mapy(max 50) :", 250),
            ("Počet replikácií :", 325),
            ("Maximálny počet krokov :", 400),
            ("Názov výstupného súboru:",475),
        ]
        for text, y in labels_left:
            self.screen.blit(self.font_button.render(text, True, WHITE), (180, y))

        labels_right = [("Hore :", 165), ("Dole :", 235), ("Vľavo :", 305), ("Vpravo :", 375)]
        for text, y in labels_right:
            self.screen.blit(self.font_button.render(text, True, WHITE), (630, y))

        for inp in self.inputs.values():
            inp.draw(self.screen)

        for btn in self.form_buttons.values():
            btn.draw(self.screen)

        self.btn_prekazky.draw(self.screen)
        self.btn_viacerych.draw(self.screen)

        if self.error_msg:
            error_surf = self.font_error.render(self.error_msg, True, RED)
            self.screen.blit(error_surf, (WIDTH // 2 - error_surf.get_width() // 2, 480))
    def draw_repeat_config(self):
        self.draw_header(185, 30, "small")
        self.screen.blit(self.font_button.render("Nastavenia opakovanej simulacie:", True, WHITE), (180, 150))
        labels = [("Port servera:", 110),("Vstupny subor:", 175), ("Novy vystupny subor:", 275), ("Pocet replikacii:", 375)]
        for text, y in labels:
            self.screen.blit(self.font_button.render(text, True, WHITE), (180, y))
        for inp in self.repeat_inputs.values(): inp.draw(self.screen)
        self.btn_viacerych.draw(self.screen)
        for btn in self.form_buttons.values(): btn.draw(self.screen)
        if self.error_msg:
            err = self.font_error.render(self.error_msg, True, RED)
            self.screen.blit(err, (WIDTH//2 - err.get_width()//2, 480))

    def draw_summary_mode(self):
        self.draw_header(50, 50, "big")
        # Vykreslíme mriežku s dátami
        self.draw_grid_content(STATE_SUMMARY)

        # Bočný panel s info
        info_x = 550
        mode_surf = self.font_mode.render("Sumárny mód", True, TEAL)
        self.screen.blit(mode_surf, (info_x, 230))

        repl_text = f"Replikácia: {self.curr_repl}/{self.max_repl}"
        self.screen.blit(self.font_subtitle_small.render(repl_text, True, WHITE), (info_x, 280))

        type_text = f"Zobrazené: {'Úspešnosť' if self.summary_display_type == 'sanca' else 'Priem. kroky'}"
        self.screen.blit(self.font_stat_normal.render(type_text, True, YELLOW), (info_x, 310))
        self.screen.blit(self.font_error.render("(Klikni pre prepnutie typu)", True, WHITE), (info_x, 335))

        for btn in self.simulation_buttons.values():
            btn.draw(self.screen)

    def draw_interactive_mode(self):

        self.draw_header(50, 50, "big")


        self.draw_grid_content(STATE_INTERACTIVE)


        info_x = 550

        # Nadpis módu
        mode_surf = self.font_interactive_mode.render("Interaktívny mód", True, GREEN)
        self.screen.blit(mode_surf, (info_x, 230))

        repl_text = f"Replikácia: {self.curr_repl}/{self.max_repl}"
        self.screen.blit(self.font_subtitle_small.render(repl_text, True, WHITE), (info_x, 280))

        curr_x, curr_y = self.pedestrian_pos
        pos_text = f"Pozícia chodca:  [{curr_x}, {curr_y}]"
        pos_surf = self.font_stat_bold.render(pos_text, True, self.walker_color )
        self.screen.blit(pos_surf,(info_x, 320))


        # 4. Vykreslenie ovládacích tlačidiel (Zmeniť mód / Zastaviť)
        for btn in self.simulation_buttons.values():
            btn.draw(self.screen)






    def draw_join_list(self):
        self.draw_header(50, 50, "big")

        title = self.font_title.render("Dostupné simulácie ", True, WHITE)
        self.screen.blit(title, (WIDTH//2 - title.get_width()//2, 120))

        if not self.join_buttons:
            msg = self.font_button.render("Žiadne aktívne simulácie sa nenašli.", True, RED)
            self.screen.blit(msg, (WIDTH//2 - msg.get_width()//2, 250))
        else:
            for btn, _port in self.join_buttons:
                btn.draw(self.screen)

        self.back_to_menu_btn.draw(self.screen)



    #funckia na vykreslenie do gridu

    def draw_grid_content(self, mode):
        geometry = self.get_grid_geometry()
        if geometry is None :
            return

        grid_rect,start_x, start_y, cols, rows, cell_w, cell_h, min_x, min_y = geometry

        for r in range(rows):
            for c in range(cols):


                rect = pygame.Rect(start_x + c * cell_w, start_y + r * cell_h, cell_w, cell_h)

                #znovu prevratenie suradnic
                curr_lx = c + min_x
                curr_ly = (rows - 1 - r) + min_y
                #vykreslenie pozadia
                pygame.draw.rect(self.screen, (30, 30, 30), rect)
                pygame.draw.rect(self.screen, (100, 100, 100), rect, 1)

                #vypocet indexu, musi sediet s C
                idx = ((rows - 1 - r) * cols) + c

                #kreslenie zoznamu prekazok
                if len(self.obstacles) > idx and self.obstacles[idx]:
                    pygame.draw.rect(self.screen, (120, 120, 120), rect.inflate(-2, -2))
                    continue

                    # kreslenie stredu
                if curr_lx == 0 and curr_ly == 0:
                    pygame.draw.rect(self.screen, RED, rect.inflate(-4, -4))

                # 6. Text pre sumarny mod
                if mode == STATE_SUMMARY:
                    stats = self.stats_cache.get((curr_lx, curr_ly))
                    if stats:
                        val = f"{stats['sanca']*100:.0f}%" if self.summary_display_type == "sanca" else f"{stats['kroky']:.1f}"
                        txt_surf = pygame.font.SysFont("Arial", 10).render(val, True, (0, 255, 0))
                        self.screen.blit(txt_surf, txt_surf.get_rect(center=rect.center))

        # Vykreslenie trajektorie
        if mode == STATE_INTERACTIVE and len(self.trajectory) > 1:
            for i in range(len(self.trajectory) - 1):
                t1 = self.trajectory[i]
                t2 = self.trajectory[i+1]

                # Použitie logic_to_pixel pre presné stredy
                p1 = self.logic_to_pixel(t1[0], t1[1])
                p2 = self.logic_to_pixel(t2[0], t2[1])

                # ciarka len ak to neni skor cez okraj
                if abs(t1[0] - t2[0]) <= 1 and abs(t1[1] - t2[1]) <= 1:
                    pygame.draw.line(self.screen, self.walker_color, p1, p2, 2)

        # Vykreslenie chodca
        if mode == STATE_INTERACTIVE:
            pos_p = self.logic_to_pixel(self.pedestrian_pos[0], self.pedestrian_pos[1])
            pygame.draw.circle(self.screen, self.walker_color, pos_p, min(cell_w, cell_h)// 3)


    # ----------------- Actions -----------------

    #Toto je cast, ktora riesi klik do mriezky a prepocty z pixelov na logicke suradnice (x,y) v simulacii.
    # Na konci si ulozi, ktore policko je oznacene ako vybrate.
    def process_grid_click(self, mouse_pos):
        grid_rect, start_x, start_y, cols, rows, cell_w, cell_h, min_x, min_y = self.get_grid_geometry()
        #Kontrola, ci klik bol vnutri mriezky
        if not grid_rect.collidepoint(mouse_pos):
            return

        #Vypocita, do ktoreho stlpca a riadku si klikla  : pixel -> index v mriežke
        clicked_col = (mouse_pos[0] - start_x) // cell_w
        clicked_row = (mouse_pos[1] - start_y) // cell_h

        #Bezpecnostna kontrola rozsahu
        if clicked_col < 0 or clicked_col >= cols or clicked_row < 0 or clicked_row >= rows:
            return

        #Prevod na logicke suradnice
        logic_x = clicked_col + min_x
        logic_y = (rows - 1 - clicked_row) + min_y

        #Ulozenie vybranej bunky
        self.selected_cell = [logic_x, logic_y]

    #menu na spracovanie tlacitka v menu podla jeho kluca
    def process_menu_click(self, button_key):
        #refreshnutie
        self.error_msg = ""

        if button_key == "exit":
            self.quit_app()

        elif button_key == "new":
            self.state = STATE_NEW_SIM

        elif button_key == "join":
            active_ports = self.sock.get_active_simulations_from_c()
            self.create_join_buttons(active_ports)
            # ak nie su zoznam bude prazdny
            self.state = STATE_JOIN_LIST

        elif button_key == "repeat":
            self.error_msg = ""
            self.state = STATE_REPEAT_CONFIG


    def create_join_buttons(self, active_ports):
        self.join_buttons = []
        y = 200
        for p in active_ports:
            btn = UIF.make_join_button(self.font_button, p, y)
            self.join_buttons.append((btn, p))
            y += 70

    def reset_probability_inputs(self):
        for key in ["hore", "dole", "vlavo", "vpravo"]:
            self.inputs[key].clear()

    def validate_and_start(self):
        try:
            # 1. Získanie hodnôt z GUI
            port = int(self.inputs["port"].text)
            s = int(self.inputs["sirka"].text)
            v = int(self.inputs["vyska"].text)
            rep = int(self.inputs["replikacie"].text)
            k = int(self.inputs["kroky"].text)
            vystup = self.inputs["vystup"].text.strip() or "vysledok.bin"

            h = float(self.inputs["hore"].text)
            d = float(self.inputs["dole"].text)
            vl = float(self.inputs["vlavo"].text)
            vp = float(self.inputs["vpravo"].text)


            # 2. Validácia hodnôt
            if s <= 0 or s > 50 or v <= 0 or v > 50:
                self.error_msg = "CHYBA: Rozmery musia byť 1 až 50!"
                return
            if abs((h + d + vl + vp) - 1.0) > 0.001:
                self.error_msg = "CHYBA: Súčet pravdepodobností musí byť 1.0!"
                return


            self.reset_sim_view()
            self.sirka = s
            self.vyska = v
            self.curr_repl = 0
            self.max_repl = rep
            self.active_max_krokov = k
            self.active_probs = (h, d, vl, vp)
            self.current_port = port

            # ak uz socket existuje zavriem ho
            if self.sock.sock:
                self.sock.running = False
                self.sock.sock.close()
                self.sock.sock = None
                time.sleep(0.2)

            #pripojenie a odoslanie dat
            if self.sock.connect_to_server(port):
                self.sock.send_parameters(
                    s, v, rep, k, h, d, vl, vp,
                    self.s_prekazkami, self.pre_viacerych,
                    vystup, "", 0  # 0 = NOVA simulácia
                )
                self.state = STATE_SUMMARY
                self.error_msg = ""
                print(f"[GUI] Pripojené na frios2:{port} a parametre odoslané.")
            else:
                self.error_msg = f"CHYBA: Server na frios2:{port} nereaguje!"

        except ValueError:
            self.error_msg = "CHYBA: Neplatný formát čísiel!"

    def validate_and_start_repeat(self):
        try:
            port = int(self.repeat_inputs["port"].text)
            vstup = self.repeat_inputs["vstup"].text
            vystup = self.repeat_inputs["vystup"].text
            rep = int(self.repeat_inputs["repl"].text)

            self.reset_sim_view()
            self.current_port = port

            if self.sock.connect_to_server(port):
                # s=0, v=0  server si ich nacita sam
                # 1 = OPATOVNA
                self.sock.send_parameters(0, 0, rep, 0, 0, 0, 0, 0,
                                          False, self.pre_viacerych,
                                          vystup, vstup, 1)
                self.state = STATE_SUMMARY
                self.error_msg = ""
            else:
                self.error_msg = f"CHYBA: Server na frios2:{port} nereaguje!"
        except Exception as e:
            self.error_msg = "CHYBA: Skontrolujte parametre!"
            print(f"Chyba repeat startu: {e}")

    def quit_app(self):
        pygame.quit()
        sys.exit()


if __name__ == "__main__":
    app = SimulationApp()
    app.run()
