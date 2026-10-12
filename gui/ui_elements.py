# ui_elements.py
import pygame
from .constants import *


class Button:
    def __init__(self, text, x, y, width, height, color, font):
        self.text = text
        self.rect = pygame.Rect(x, y, width, height)
        self.color = color
        self.font = font
        self.is_hovered = False

    # Vykresli tlacidlo: ak je hover farbu  zosvetli
    # Potom vykresli zaobleny obdlznik a do stredu da text
    def draw(self, surface):
        draw_color = self.color
        if self.is_hovered:
            # zosvetlenie farby ked nad nim prejdeme
            draw_color = tuple(max(0, min(255, c + 30)) for c in self.color)

        pygame.draw.rect(surface, draw_color, self.rect, border_radius=25)

        text_color = BLACK if self.color == YELLOW else WHITE
        text_surf = self.font.render(self.text, True, text_color)
        text_rect = text_surf.get_rect(center=self.rect.center)
        surface.blit(text_surf, text_rect)

    #aktualizuj stav hover
    def check_hovered(self, mouse_pos):
        self.is_hovered = self.rect.collidepoint(mouse_pos)

    # Zisti, ci bolo tlacidlo kliknute:
    # musi ist o klik lavym tlacidlom mysi a kurzor musi byt v tlacidle.
    def is_clicked(self, event, mouse_pos):
        return (
                event.type == pygame.MOUSEBUTTONDOWN
                and event.button == 1
                and self.rect.collidepoint(mouse_pos)
        )


class InputBox:
    # Constructor: vytvori obdlznik vstupneho policka, nastavi default text,
    # font a interne stavy
    # active = piseme do neho,
    # is_default = este je tam placeholder
    def __init__(self, x, y, width, height, default_text, font):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = default_text
        self.font = font
        self.active = False
        self.is_default = True


    # Spracuje udalosti:
    # -klik mysou: aktivuje pole a vymaze default text, alebo deaktivuje pri kliku mimo
    # -klavesnica: ak je active, vie mazat , ukoncit vstup ,
    #  alebo pridavat povolene znaky
    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if self.rect.collidepoint(event.pos):
                self.active = True
                if self.is_default:
                    self.text = ""
                    self.is_default = False
            else:
                self.active = False

        if event.type == pygame.KEYDOWN and self.active:
            if event.key == pygame.K_BACKSPACE:
                self.text = self.text[:-1]
            elif event.key == pygame.K_RETURN:
                self.active = False
            else:
                if event.unicode.isalnum() or event.unicode in "._-":
                    self.text += event.unicode

    # Vykresli input box:
    # biele pozadie
    # ramcek modry ked active, inak sedy
    # text: tmavosivy ak je to default,  inak cierny.
    def draw(self, surface):
        pygame.draw.rect(surface, WHITE, self.rect)

        border_color = BLUE if self.active else LIGHT_GRAY
        pygame.draw.rect(surface, border_color, self.rect, 2)

        color = DARK_GRAY if self.is_default else BLACK
        text_surf = self.font.render(self.text, True, color)
        surface.blit(text_surf, (self.rect.x + 5, self.rect.y + 5))

    # Vymaze obsah input boxu a zresetuje stavy
    def clear(self):
        self.text = ""
        self.is_default = False
        self.active = False
