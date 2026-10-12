# ui_factory.py
from . import constants as C
from .ui_elements import Button, InputBox

# Kluc je "nazov akcie a hodnota je Button s pevnou poziciou a stylom.

def make_main_menu_buttons(font_button):
    return {
        "new":    Button("SPUSTIŤ NOVÚ SIMULÁCIU", 110, 250, 360, 120, C.TEAL, font_button),
        "join":   Button("PRIPOJIŤ SA K SIMULACII", 530, 250, 360, 120, C.RED, font_button),
        "repeat": Button("SPUSTIŤ OPATOVNU SIMULÁCIU", 110, 400, 360, 120, C.GREEN, font_button),
        "exit":   Button("KONIEC", 530, 400, 360, 120, C.YELLOW, font_button),
    }

def make_inputs(font_button):
    return {
        "port":       InputBox(180, 110, 240, 30, "56000", font_button),
        "sirka": InputBox(180, 200, 240, 30, "10", font_button),
        "vyska": InputBox(180, 275, 240, 30, "10", font_button),
        "replikacie": InputBox(180, 350, 240, 30, "100", font_button),
        "kroky": InputBox(180, 425, 240, 30, "50", font_button),
        "vystup": InputBox(180, 490, 300, 40,"vysledok.bin",font_button),
        "hore": InputBox(580, 190, 160, 30, "0.25", font_button),
        "dole": InputBox(580, 260, 160, 30, "0.25", font_button),
        "vlavo": InputBox(580, 330, 160, 30, "0.25", font_button),
        "vpravo": InputBox(580, 400, 160, 30, "0.25", font_button),
    }

def make_form_buttons(font_button):
    return {
        "start": Button("SPUSTIŤ", 240, 540, 220, 60, C.YELLOW, font_button),
        "back":  Button("KONIEC", 490, 540, 220, 60, C.YELLOW, font_button),
    }

def make_simulation_buttons(font_button):
    return {
        "change": Button("ZMEŇ MÓD", 260, 520, 220, 60, C.YELLOW, font_button),
        "stop":   Button("KONIEC", 510, 520, 220, 60, C.YELLOW, font_button),
    }

# Vytvori dvojicu prepinacich tlacidiel, ktore menia farbu
def make_toggle_buttons(font_button):
    btn_prekazky = Button("PREKÁŽKY", 530, 460, 175, 40, C.TEAL, font_button)
    btn_viacerych = Button("N KLIENTOV", 715, 460, 175, 40, C.GREEN, font_button)
    return btn_prekazky, btn_viacerych

def make_back_button(font_button):
    return Button("SPÄŤ", C.WIDTH//2 - 100, 500, 200, 50, C.YELLOW, font_button)

def make_join_button(font_button, port, y):
    return Button(f"PRIPOJIŤ K PORTU: {port}", C.WIDTH//2 - 150, y, 300, 50, C.TEAL, font_button)

def make_repeat_inputs(font_button):
    return {
        # x, y, sirka, vyska, default_text, font
        "port":   InputBox(180, 110, 200, 30, "56000", font_button),
        "vstup":  InputBox(180, 200, 350, 30, "vysledok.bin", font_button),
        "vystup": InputBox(180, 300, 350, 30, "vysledok_novy.bin", font_button),
        "repl":   InputBox(180, 400, 200, 30, "500", font_button),
    }
def make_refresh_button(font_button):
    return Button("OBNOVIŤ", C.WIDTH//2 + 20, 500, 180, 50, C.GREEN, font_button)