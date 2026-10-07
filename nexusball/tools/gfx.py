#!/usr/bin/env python3
"""NEXUS BALL - generateur des graphismes temporaires (prototype).

Produit, sans dependance externe :
  data/gen/field.chr   tiles 4bpp du stade (BG1)
  data/gen/field.map   tilemap 64x64 (BG1, palette 2)
  data/gen/font.chr    police 2bpp (BG3) : 0-63 transparente, 64-127 sur panneau, 128+ gros titre
  data/gen/obj.chr     sprites 4bpp (16x16 + 8x8)
  data/gen/pal.bin     256 couleurs BGR555
  build/preview_*.png  apercus
"""
import math
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEN = os.path.join(ROOT, "data", "gen")
PREV = os.path.join(ROOT, "build")

# ---------------------------------------------------------------- geometrie (doit suivre include/constants.inc)
WORLD_W, WORLD_H = 512, 320
FIELD_L, FIELD_R = 24, 487
FIELD_T, FIELD_B = 72, 295
FIELD_CY = 184
RING_Z, RING_R = 20, 22
LONG_R = 150
ZONE_R = 40


def c5(r, g, b):
    return (r & 31) | ((g & 31) << 5) | ((b & 31) << 10)


# ---------------------------------------------------------------- palettes
BG3_PAL = [  # palettes BG3 (couleurs 0-15) : 0 texte blanc, 1 equipe A, 2 titre, 3 equipe B
    c5(1, 1, 4), c5(2, 3, 8), c5(29, 30, 31), c5(31, 24, 6),
    c5(0, 0, 0), c5(2, 3, 8), c5(10, 20, 31), c5(31, 31, 31),
    c5(0, 0, 0), c5(1, 1, 4), c5(31, 27, 8), c5(31, 31, 31),
    c5(0, 0, 0), c5(2, 3, 8), c5(31, 15, 3), c5(31, 31, 31),
    c5(0, 0, 0), c5(2, 3, 8), c5(31, 18, 4), c5(31, 26, 8),     # 4 : texte accentue (menus)
    c5(0, 0, 0), c5(2, 3, 8), c5(12, 24, 31), c5(31, 26, 8),    # 5 : equipe humaine (menus)
    c5(0, 0, 0), c5(2, 3, 8), c5(31, 31, 31), c5(6, 20, 31),    # 6 : cadre du score (HUD)
]
FIELD_PAL = [
    c5(1, 1, 3),     # 0 noir
    c5(4, 6, 10),    # 1 sol sombre
    c5(5, 7, 12),    # 2 sol
    c5(7, 9, 15),    # 3 joints de dalles
    c5(24, 26, 30),  # 4 lignes
    c5(6, 22, 28),   # 5 ligne 2 points
    c5(4, 5, 9),     # 6 mur sombre
    c5(8, 10, 16),   # 7 mur
    c5(13, 15, 22),  # 8 mur clair
    c5(6, 14, 31),   # 9 neon bleu
    c5(31, 14, 2),   # 10 anneau orange
    c5(31, 27, 10),  # 11 anneau jaune
    c5(10, 8, 22),   # 12 public A (cycle)
    c5(20, 10, 18),  # 13 public B (cycle)
    c5(6, 12, 24),   # 14 public C (cycle)
    c5(14, 10, 8),   # 15 visages
]



# ---------------------------------------------------------------- stades (geometrie identique)
def stadium_pal(floor_d, floor, joint, line, long_line, wall_d, wall, wall_l, neon, ring_o, ring_y, crowd):
    return [c5(1, 1, 3), c5(*floor_d), c5(*floor), c5(*joint), c5(*line), c5(*long_line),
            c5(*wall_d), c5(*wall), c5(*wall_l), c5(*neon), c5(*ring_o), c5(*ring_y),
            c5(*crowd[0]), c5(*crowd[1]), c5(*crowd[2]), c5(14, 10, 8)]


STADIUMS = [
    dict(name="ORBITAL ARENA", emblem=(6, 0), words=("NEXUS", "ARENA"), floor="check",
         pal=FIELD_PAL),
    dict(name="MARS DOME", emblem=(5, -90), words=("MARS", "DOME"), floor="stripe",
         pal=stadium_pal((9, 4, 3), (11, 5, 3), (14, 7, 4), (28, 24, 20), (30, 22, 6), (6, 3, 2), (11, 6, 4),
                         (16, 9, 6), (31, 10, 4), (31, 14, 2), (31, 27, 10), ((22, 6, 4), (12, 4, 14), (26, 16, 6)))),
    dict(name="EUROPA ICE", emblem=(6, 30), words=("EUROPA", "ICE"), floor="grid",
         pal=stadium_pal((4, 7, 11), (5, 9, 13), (8, 13, 17), (28, 30, 31), (31, 18, 4), (5, 7, 10), (10, 14, 19),
                         (16, 21, 26), (20, 28, 31), (31, 14, 2), (31, 27, 10), ((6, 14, 24), (24, 26, 30), (4, 8, 16)))),
    dict(name="ANDROMEDA PRIME", emblem=(8, 22.5), words=("ANDRO", "PRIME"), floor="diamond",
         pal=stadium_pal((5, 3, 9), (7, 4, 12), (10, 6, 16), (26, 24, 31), (8, 26, 22), (4, 2, 7), (9, 5, 14),
                         (14, 9, 20), (26, 8, 30), (31, 14, 2), (31, 27, 10), ((20, 6, 24), (6, 10, 26), (28, 16, 28)))),
    dict(name="TITAN INDUSTRIAL", emblem=(4, 45), words=("TITAN", "WORKS"), floor="plate",
         pal=stadium_pal((6, 6, 6), (8, 8, 8), (11, 11, 10), (29, 29, 26), (31, 24, 2), (4, 4, 4), (9, 9, 9),
                         (14, 14, 13), (31, 22, 2), (31, 12, 2), (31, 27, 10), ((16, 16, 16), (24, 20, 6), (10, 10, 12)))),
    dict(name="SOLARIS ARENA", emblem=(12, 0), words=("SOLAR", "ARENA"), floor="stripe",
         pal=stadium_pal((10, 6, 3), (12, 8, 4), (15, 10, 5), (31, 30, 24), (8, 22, 30), (7, 4, 2), (13, 8, 4),
                         (20, 13, 6), (31, 26, 6), (31, 12, 2), (31, 30, 14), ((30, 20, 4), (26, 10, 6), (31, 28, 12)))),
]

# couleurs fixes de l'equipement futuriste (index 11-15 de toutes les palettes de joueurs)
GEAR = [c5(21, 23, 27), c5(9, 10, 14), c5(8, 27, 31), c5(20, 31, 31), c5(31, 31, 31)]


GEAR_GK = [c5(31, 31, 26), c5(17, 17, 11), c5(8, 27, 31), c5(20, 31, 31), c5(31, 31, 31)]


def team_pal(main, dark, trim, shorts, hi, gear=GEAR):
    return [0, c5(2, 2, 4), c5(27, 20, 15), c5(19, 12, 8), c5(6, 4, 3),
            main, dark, trim, shorts, c5(4, 4, 7), hi] + gear


OBJ_PALS = [
    team_pal(c5(4, 12, 30), c5(2, 6, 18), c5(28, 30, 31), c5(3, 5, 14), c5(12, 20, 31)),   # 0 equipe A bleue
    team_pal(c5(31, 13, 2), c5(20, 6, 1), c5(31, 28, 20), c5(10, 4, 2), c5(31, 22, 10)),   # 1 equipe B orange
    team_pal(c5(4, 24, 8), c5(1, 13, 4), c5(28, 31, 24), c5(2, 8, 3), c5(14, 31, 14), GEAR_GK),     # 2 gardien A vert (gants et protections clairs)
    team_pal(c5(30, 26, 2), c5(18, 14, 0), c5(31, 31, 24), c5(12, 9, 0), c5(31, 31, 14), GEAR_GK),  # 3 gardien B jaune
    [0, c5(31, 31, 31), c5(20, 30, 31), c5(8, 22, 31), c5(2, 6, 16), c5(1, 1, 3),
     c5(31, 29, 6), c5(24, 12, 0), c5(31, 8, 8), c5(1, 2, 6), c5(8, 18, 28), c5(4, 9, 16),
     c5(8, 18, 31), c5(31, 15, 3), c5(31, 31, 31), c5(29, 30, 31)],                     # 4 ballon / curseurs
    team_pal(c5(4, 12, 30), c5(2, 6, 18), c5(28, 30, 31), c5(3, 5, 14), c5(12, 20, 31)),   # 5 equipe A, peau foncee (1-10 au runtime)
    team_pal(c5(31, 13, 2), c5(20, 6, 1), c5(31, 28, 20), c5(10, 4, 2), c5(31, 22, 10)),   # 6 equipe B, peau foncee
]


# ---------------------------------------------------------------- police 5x7
FONT = {
    '@': ["00010", "11111", "10000", "11110", "10000", "10000", "11111"],   # E aigu
    '[': ["01000", "11111", "10000", "11110", "10000", "10000", "11111"],   # E grave
    '^': ["01110", "00000", "11111", "10000", "11110", "10000", "11111"],   # E circonflexe
    ']': ["01000", "01110", "10001", "10001", "11111", "10001", "10001"],   # A grave
    '\\': ["01110", "10001", "10000", "10000", "10001", "01110", "00100"],  # C cedille
    '#': ["01110", "00000", "01110", "10001", "10001", "10001", "01110"],   # O circonflexe
    'A': ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    'B': ["11110", "10001", "10001", "11110", "10001", "10001", "11110"],
    'C': ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    'D': ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    'E': ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    'F': ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    'G': ["01110", "10001", "10000", "10111", "10001", "10001", "01111"],
    'H': ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    'I': ["01110", "00100", "00100", "00100", "00100", "00100", "01110"],
    'J': ["00111", "00010", "00010", "00010", "00010", "10010", "01100"],
    'K': ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    'L': ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    'M': ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    'N': ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    'O': ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    'P': ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    'Q': ["01110", "10001", "10001", "10001", "10101", "10010", "01101"],
    'R': ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    'S': ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    'T': ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    'U': ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    'V': ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    'W': ["10001", "10001", "10001", "10101", "10101", "10101", "01010"],
    'X': ["10001", "10001", "01010", "00100", "01010", "10001", "10001"],
    'Y': ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    'Z': ["11111", "00001", "00010", "00100", "01000", "10000", "11111"],
    '0': ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    '1': ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    '2': ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    '3': ["11111", "00010", "00100", "00010", "00001", "10001", "01110"],
    '4': ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    '5': ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    '6': ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    '7': ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    '8': ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    '9': ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],
    ':': ["00000", "01100", "01100", "00000", "01100", "01100", "00000"],
    '-': ["00000", "00000", "00000", "11111", "00000", "00000", "00000"],
    '.': ["00000", "00000", "00000", "00000", "00000", "01100", "01100"],
    ',': ["00000", "00000", "00000", "00000", "01100", "00100", "01000"],
    '!': ["00100", "00100", "00100", "00100", "00100", "00000", "00100"],
    '?': ["01110", "10001", "00001", "00010", "00100", "00000", "00100"],
    "'": ["00100", "00100", "01000", "00000", "00000", "00000", "00000"],
    '/': ["00001", "00010", "00010", "00100", "01000", "01000", "10000"],
    '>': ["01000", "00100", "00010", "00001", "00010", "00100", "01000"],
    '<': ["00010", "00100", "01000", "10000", "01000", "00100", "00010"],
    '+': ["00000", "00100", "00100", "11111", "00100", "00100", "00000"],
    '(': ["00010", "00100", "01000", "01000", "01000", "00100", "00010"],
    ')': ["01000", "00100", "00010", "00010", "00010", "00100", "01000"],
    '=': ["00000", "00000", "11111", "00000", "11111", "00000", "00000"],
    '&': ["01100", "10010", "10100", "01000", "10101", "10010", "01101"],
    '*': ["00000", "10101", "01110", "11111", "01110", "10101", "00000"],
}


# ---------------------------------------------------------------- encodage SNES
def enc_tile(px, bpp):
    """px: 64 indices (ligne par ligne)."""
    out = bytearray(8 * bpp)
    for r in range(8):
        for c in range(8):
            v = px[r * 8 + c]
            bit = 7 - c
            for p in range(bpp):
                if v >> p & 1:
                    out[(p >> 1) * 16 + r * 2 + (p & 1)] |= 1 << bit
    return bytes(out)


def write_png(path, w, h, rgb_rows):
    raw = b"".join(b"\x00" + bytes(row) for row in rgb_rows)
    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(data)


def rgb(c):
    return ((c & 31) * 255 // 31, (c >> 5 & 31) * 255 // 31, (c >> 10 & 31) * 255 // 31)


# ---------------------------------------------------------------- image indexee
class Img:
    def __init__(self, w, h, fill=0):
        self.w, self.h = w, h
        self.p = [[fill] * w for _ in range(h)]

    def put(self, x, y, c):
        x, y = int(x), int(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            self.p[y][x] = c

    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.p[y][x]
        return 0

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, c)

    def hline(self, x0, x1, y, c):
        self.rect(x0, y, x1, y, c)

    def vline(self, x, y0, y1, c):
        self.rect(x, y0, x, y1, c)

    def ellipse(self, cx, cy, rx, ry, c, thick=1.0, clip=None, dash=0):
        n = int(2 * math.pi * max(rx, ry) * 3)
        for i in range(n):
            a = 2 * math.pi * i / n
            if dash and int(a * max(rx, ry) / dash) % 2:
                continue
            for t in range(int(thick * 2)):
                k = t * 0.5
                x = cx + (rx - k) * math.cos(a)
                y = cy + (ry - k) * math.sin(a)
                if clip and not clip(x, y):
                    continue
                self.put(round(x), round(y), c)

    def text(self, s, x, y, c, scale=1):
        for ch in s:
            g = FONT.get(ch)
            if g:
                for r, row in enumerate(g):
                    for k, b in enumerate(row):
                        if b == "1":
                            self.rect(x + k * scale, y + r * scale, x + k * scale + scale - 1, y + r * scale + scale - 1, c)
            x += 6 * scale


# ---------------------------------------------------------------- stade
def build_crowd():
    """tribunes (BG2, palette 2) : 256x256 repete, rangees de spectateurs de 8 px.
    Corps en couleurs 12-14 (cycle de palette = public anime), tetes 15."""
    im = Img(256, 256, 1)
    for tier in range(32):
        y0 = tier * 8
        # marche de la tribune
        im.hline(0, 255, y0 + 7, 6)
        for px in range(0, 256, 8):
            v = (px // 8 * 7 + tier * 3) % 11
            if v == 10:
                continue                          # place vide
            body = 12 + (px // 8 + tier) % 3
            x = px + (tier % 2) * 4 % 8
            raised = v in (2, 7)
            # tete
            im.rect(x + 2, y0, x + 4, y0 + 1, 15)
            im.put(x + 3, y0, 1 if v % 3 == 0 else 15)
            # corps
            im.rect(x + 1, y0 + 2, x + 5, y0 + 6, body)
            im.put(x + 1, y0 + 6, 1)
            im.put(x + 5, y0 + 6, 1)
            if raised:
                im.put(x, y0, 15)
                im.put(x + 6, y0, 15)
                im.vline(x, y0 + 1, y0 + 2, body)
                im.vline(x + 6, y0 + 1, y0 + 2, body)
            if v == 5:                            # drapeau
                im.rect(x + 5, y0 - 2 if y0 else 0, x + 7, y0, 10)
    return im


def build_field(st):
    im = Img(WORLD_W, WORLD_H, 0)            # 0 = transparent : les tribunes (BG2) apparaissent
    # rambarde en haut des tribunes basses
    im.rect(0, 50, WORLD_W - 1, 55, 6)
    im.hline(0, WORLD_W - 1, 50, 8)
    im.hline(0, WORLD_W - 1, 51, 9)
    for x in range(0, WORLD_W, 32):
        im.rect(x, 50, x + 1, 55, 8)
    # mur du haut : panneaux publicitaires lumineux et piliers
    im.rect(0, 56, WORLD_W - 1, 71, 7)
    im.hline(0, WORLD_W - 1, 56, 8)
    im.hline(0, WORLD_W - 1, 57, 6)
    im.hline(0, WORLD_W - 1, 68, 6)
    im.hline(0, WORLD_W - 1, 69, 9)
    im.hline(0, WORLD_W - 1, 70, 8)
    im.hline(0, WORLD_W - 1, 71, 6)
    w1, w2 = st["words"]
    for x0 in range(0, WORLD_W, 128):
        for bx, word, col in ((x0 + 6, w1, 9), (x0 + 70, w2, 11)):
            im.rect(bx, 58, bx + 51, 67, 6)
            im.rect(bx + 1, 59, bx + 50, 66, 0 if False else 6)
            im.hline(bx, bx + 51, 58, col)
            im.hline(bx, bx + 51, 67, col)
            im.vline(bx, 58, 67, col)
            im.vline(bx + 51, 58, 67, col)
            im.text(word, bx + 26 - len(word) * 3, 60, col)
        for px in (x0 + 0, x0 + 64):
            im.rect(px, 57, px + 3, 68, 8)
            im.vline(px + 1, 57, 68, 9)
    # sol
    im.rect(FIELD_L, FIELD_T, FIELD_R, FIELD_B, 2)
    for y in range(FIELD_T, FIELD_B + 1):
        for x in range(FIELD_L, FIELD_R + 1):
            f = st["floor"]
            if f == "check":
                if (x // 32 + y // 32) % 2:
                    im.put(x, y, 1)
                if x % 32 == 0 or y % 32 == 8:
                    im.put(x, y, 3)
            elif f == "stripe":
                if (x // 24) % 2:
                    im.put(x, y, 1)
                if y % 32 == 8:
                    im.put(x, y, 3)
            elif f == "grid":
                if x % 16 == 0 or y % 16 == 8:
                    im.put(x, y, 3)
                elif ((x // 16) * 7 + (y // 16) * 3) % 5 == 0:
                    im.put(x, y, 1)
            elif f == "diamond":
                if ((x + y) % 32 == 0) or ((x - y) % 32 == 0):
                    im.put(x, y, 3)
                elif ((x + y) // 32 + (x - y) // 32) % 2:
                    im.put(x, y, 1)
            elif f == "plate":
                if x % 32 == 0 or y % 16 == 8:
                    im.put(x, y, 3)
                elif (x % 32 in (4, 28)) and (y % 16 in (12, 4)):
                    im.put(x, y, 3)
                if (y // 16) % 2:
                    im.put(x, y, 1) if im.get(x, y) == 2 else None
    # eclairage : 4 halos de projecteurs (tramage clair) et bords assombris (vignette)
    bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
    pools = [(FIELD_L + 120, FIELD_T + 60), (FIELD_R - 120, FIELD_T + 60),
             (FIELD_L + 120, FIELD_B - 50), (FIELD_R - 120, FIELD_B - 50)]
    for y in range(FIELD_T, FIELD_B + 1):
        for x in range(FIELD_L, FIELD_R + 1):
            c = im.get(x, y)
            if c not in (1, 2):
                continue
            light = max(0.0, 1.0 - min(((x - px) / 70.0) ** 2 + ((y - py) / 46.0) ** 2 for px, py in pools))
            edge = min(x - FIELD_L, FIELD_R - x, (y - FIELD_T) * 2, (FIELD_B - y) * 2)
            dark = max(0.0, 1.0 - edge / 28.0)
            b = bayer[y & 3][x & 3] / 16.0
            if light * 0.55 > b and c == 2:
                im.put(x, y, 3)                      # halo
            elif dark * 0.8 > b:
                im.put(x, y, 1)                      # vignette
    # raquettes : hachures (zone reservee aux gardiens)
    for gx, sgn in ((FIELD_L, 1), (FIELD_R, -1)):
        for y in range(FIELD_CY - ZONE_R, FIELD_CY + ZONE_R + 1):
            for x in range(FIELD_L, FIELD_R + 1):
                if (x - gx) * sgn < 0:
                    continue
                if (x - gx) ** 2 + (y - FIELD_CY) ** 2 < (ZONE_R - 2) ** 2:
                    if (x + y * sgn) % 4 == 0:
                        im.put(x, y, 3)              # hachures claires
                    elif im.get(x, y) == 2 and (x + y) % 2 == 0:
                        im.put(x, y, 1)              # fond assombri
    # mots du stade peints au sol de chaque moitie (tramage discret)
    w1, w2 = st["words"]
    for word, mx in ((w1, (FIELD_L + 256) // 2), (w2, (256 + FIELD_R) // 2)):
        sc = 3
        tx = mx - (len(word) * 6 * sc - sc) // 2
        ty = FIELD_CY - 10
        for ch in word:
            g = FONT.get(ch)
            if g:
                for r, row in enumerate(g):
                    for k, b in enumerate(row):
                        if b == "1":
                            for yy in range(ty + r * sc, ty + r * sc + sc):
                                for xx in range(tx + k * sc, tx + k * sc + sc):
                                    if (xx + yy) % 2 == 0 and im.get(xx, yy) in (1, 2):
                                        im.put(xx, yy, 3)
            tx += 6 * sc
    # balises lumineuses le long des lignes de touche
    for x in range(FIELD_L + 16, FIELD_R - 8, 32):
        for yy in (FIELD_T + 9, FIELD_B - 4):
            im.rect(x, yy, x + 1, yy, 9)
            im.put(x - 1, yy, 8)
            im.put(x + 2, yy, 8)
    # emblème central : double polygone (forme propre a chaque stade) et rayons
    cx, cy = 256, FIELD_CY
    ns, rot = st.get("emblem", (6, 0))
    axes = [(math.cos(math.radians(rot + 360.0 * k / ns)), math.sin(math.radians(rot + 360.0 * k / ns)))
            for k in range(ns)]
    for y in range(cy - 32, cy + 33):
        for x in range(cx - 34, cx + 35):
            dx, dy = x - cx, y - cy
            hexd = max(dx * ax + dy * ay for ax, ay in axes)   # distance "polygonale"
            if 27 <= hexd < 28.5 or 20 <= hexd < 21:
                im.put(x, y, 5 if hexd > 24 else 3)
            elif hexd < 20 and (abs(dx) + abs(dy)) % 7 == 0 and im.get(x, y) in (1, 2):
                im.put(x, y, 3)
    # liseré neon du perimetre du terrain
    im.hline(FIELD_L, FIELD_R, FIELD_B, 9)
    im.hline(FIELD_L, FIELD_R, FIELD_B - 1, 8)
    im.vline(FIELD_L, FIELD_T, FIELD_B, 9)
    im.vline(FIELD_R, FIELD_T, FIELD_B, 9)
    # coins : quarts de cercle
    for (qx, qy) in ((FIELD_L, FIELD_T), (FIELD_R, FIELD_T), (FIELD_L, FIELD_B), (FIELD_R, FIELD_B)):
        im.ellipse(qx, qy, 12, 12, 4, thick=1.0, clip=lambda x, y: FIELD_L < x < FIELD_R and FIELD_T < y < FIELD_B)
    # ombre portee du mur du haut
    for y in range(FIELD_T + 1, FIELD_T + 7):
        for x in range(FIELD_L, FIELD_R + 1):
            if (y < FIELD_T + 4) or (x + y) % 2 == 0:
                if im.get(x, y) != 4:
                    im.put(x, y, 1)
    # murs lateraux techniques
    for side in (0, 1):
        x0, x1 = (0, FIELD_L - 1) if side == 0 else (FIELD_R + 1, WORLD_W - 1)
        im.rect(x0, 56, x1, 303, 7)
        edge = x1 if side == 0 else x0
        inner = edge - 3 if side == 0 else edge + 3
        im.vline(edge, 56, 303, 9)
        im.vline(edge - 1 if side == 0 else edge + 1, 56, 303, 6)
        im.vline(inner, 56, 303, 8)
        for y in range(64, 300, 24):
            im.rect(x0 + 2, y, x1 - 2, y + 2, 6)
            im.put(x0 + 4, y + 1, 9)
            im.put(x1 - 4, y + 1, 9)
    # mur du bas : vitre avec bande neon
    im.rect(0, 296, WORLD_W - 1, 303, 7)
    im.hline(0, WORLD_W - 1, 296, 9)
    im.hline(0, WORLD_W - 1, 297, 8)
    im.hline(0, WORLD_W - 1, 303, 6)
    for x in range(0, WORLD_W, 16):
        im.vline(x, 298, 302, 6)
        im.put(x + 4, 299, 8)

    inside = lambda x, y: FIELD_L <= x <= FIELD_R and FIELD_T <= y <= FIELD_B
    # lignes (plus epaisses)
    im.rect(255, FIELD_T, 256, FIELD_B, 4)
    im.ellipse(256, FIELD_CY, 40, 40, 4, thick=1.5)
    im.ellipse(256, FIELD_CY, 10, 10, 5, thick=1.0)
    im.rect(254, FIELD_CY - 1, 257, FIELD_CY + 1, 4)
    for gx, sgn in ((FIELD_L, 1), (FIELD_R, -1)):
        im.ellipse(gx, FIELD_CY, ZONE_R, ZONE_R, 4, thick=1.5,
                   clip=lambda x, y, gx=gx, s=sgn: inside(x, y) and (x - gx) * s >= 0)
        im.ellipse(gx, FIELD_CY, LONG_R, LONG_R * 0.9, 5, thick=1.0,
                   clip=lambda x, y, gx=gx, s=sgn: inside(x, y) and (x - gx) * s >= 0, dash=4)
    # anneaux : plaque sombre, halo, anneau epais
    for gx in (FIELD_L, FIELD_R):
        cy = FIELD_CY - RING_Z
        for y in range(cy - RING_R - 4, cy + RING_R + 5):
            for x in range(gx - 12, gx + 13):
                d = ((x - gx) / 12.0) ** 2 + ((y - cy) / (RING_R + 4.0)) ** 2
                if d <= 1.0 and not inside(x, y):
                    im.put(x, y, 6)
        im.rect(gx - 1, cy + RING_R - 2, gx + 1, FIELD_CY + 2, 6)  # pied
        im.ellipse(gx, cy, 10, RING_R + 3, 10, thick=1.0, dash=2)  # halo
        im.ellipse(gx, cy, 8, RING_R + 1, 10, thick=2.0)
        im.ellipse(gx, cy, 6, RING_R - 1, 11, thick=1.0)
    im.hline(FIELD_L, FIELD_R, FIELD_T, 8)
    return im


def tiles_from_image(im, bpp=4):
    tiles, index, tmap = [], {}, []
    for ty in range(64):
        for tx in range(64):
            if ty * 8 >= im.h or tx * 8 >= im.w:
                px = (0,) * 64
            else:
                px = tuple(im.get(tx * 8 + c, ty * 8 + r) for r in range(8) for c in range(8))
            key = None
            flips = {}
            for hf in (0, 1):
                for vf in (0, 1):
                    q = tuple(px[(7 - r if vf else r) * 8 + (7 - c if hf else c)] for r in range(8) for c in range(8))
                    if q in index:
                        key = (index[q], hf, vf)
                        break
                if key:
                    break
            if key is None:
                index[px] = len(tiles)
                tiles.append(px)
                key = (index[px], 0, 0)
            t, hf, vf = key
            tmap.append(t | (2 << 10) | (hf << 14) | (vf << 15))
    return tiles, tmap


# ---------------------------------------------------------------- sprites joueurs (procedural)
OUT, SKIN, SKINS, HAIR, KIT, KITD, TRIM, SHORT, BOOT, KITH = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
METAL, METALD, VISOR, NEON, WHITE = 11, 12, 13, 14, 15


def limb(im, x0, y0, ang, ln, c_up, c_end, width=1):
    a = math.radians(ang)
    dx, dy = math.sin(a), math.cos(a)
    steps = int(ln * 3)
    for i in range(steps + 1):
        t = i / steps * ln
        x, y = x0 + dx * t, y0 + dy * t
        col = c_end if t > ln - 1.6 else c_up
        im.put(round(x), round(y), col)
        if width > 1:
            im.put(round(x - 0.5), round(y), col)


FW, FH = 16, 32          # cadre d'un joueur : 2 sprites 16x16 empiles (figure de 16x24)
FEET = 22


def figure(legs=(8, -8), arms=(20, -20), lean=0, arm_lift=0, crouch=0):
    """athlete du futur : casque a visiere neon et crete aux couleurs de l'equipe,
    epaulieres et plastron, combinaison, genouilleres, bottes a semelle lumineuse."""
    im = Img(FW, FH, 0)
    L = lean
    hip = 16 + crouch
    sh = 9 + crouch

    def leg(x0, ang, front):
        ln = 6.0 - crouch
        a = math.radians(ang)
        dx, dy = math.sin(a), math.cos(a)
        for i in range(int(ln * 3) + 1):
            t = i / (ln * 3) * ln
            x, y = round(x0 + dx * t), round(hip + dy * t)
            c = SHORT if front else KITD
            if 2.2 < t < 3.6:
                c = METAL if front else METALD          # genouillere
            if t > ln - 1.8:
                c = BOOT
            im.put(x, y, c)
            im.put(round(x0 + dx * t - 0.5), y, c)
        fx, fy = round(x0 + dx * ln), round(hip + dy * ln)
        im.put(fx, fy + 1, NEON if front else VISOR)     # semelle lumineuse
        im.put(fx - 1, fy + 1, VISOR)

    def arm(x0, ang, front):
        ln = 5.2
        a = math.radians(ang)
        dx, dy = math.sin(a), math.cos(a)
        for i in range(int(ln * 3) + 1):
            t = i / (ln * 3) * ln
            x, y = round(x0 + dx * t), round(sh + dy * t)
            c = (KIT if front else KITD) if t < ln - 1.6 else (METAL if front else METALD)   # gant
            im.put(x, y, c)
            im.put(round(x0 + dx * t - 0.5), y, c)

    # membres arriere
    leg(6 + L, legs[1], False)
    arm(5 + L, arms[1], False)
    # torse : combinaison + plastron
    im.rect(5 + L, sh - 1, 11 + L, hip - 2, KIT)
    im.vline(5 + L, sh, hip - 2, KITD)
    im.vline(11 + L, sh + 1, hip - 3, KITH)
    im.rect(7 + L, sh, 10 + L, sh + 3, KITD)            # plastron
    im.hline(7 + L, 10 + L, sh, METAL)
    im.vline(8 + L, sh + 1, sh + 3, NEON)               # ligne neon
    im.put(10 + L, sh + 2, TRIM)
    # epaulieres
    im.rect(4 + L, sh - 1, 6 + L, sh, METAL)
    im.put(4 + L, sh, METALD)
    im.rect(10 + L, sh - 1, 12 + L, sh, METAL)
    im.put(12 + L, sh, METALD)
    im.put(11 + L, sh - 1, WHITE)
    # ceinture
    im.hline(5 + L, 11 + L, hip - 2, METALD)
    im.put(8 + L, hip - 2, VISOR)
    # bas de combinaison
    im.rect(5 + L, hip - 1, 11 + L, hip, SHORT)
    # casque
    hy = 4 + crouch
    for y in range(hy - 4, hy + 4):
        for x in range(4 + L, 13 + L):
            d = (x - 8.5 - L) ** 2 * 1.05 + (y - hy) ** 2
            if d <= 11.5:
                c = KIT if y < hy else METAL               # coque aux couleurs de l'equipe
                if x <= 5 + L:
                    c = KITD if y < hy else METALD
                if y >= hy + 2:
                    c = METALD                             # mentonniere
                if y == hy - 3 and x in (8 + L, 9 + L):
                    c = KITH                               # reflet
                im.put(x, y, c)
    for x in range(7 + L, 10 + L):                         # crete metallique
        im.put(x, hy - 4, METAL)
    im.put(8 + L, hy - 4, WHITE)
    # visiere transparente : le visage se voit derriere le verre (cadre et reflet neon)
    im.put(9 + L, hy, VISOR)
    im.put(10 + L, hy, SKIN)
    im.put(11 + L, hy, OUT)                                # oeil
    im.put(12 + L, hy, NEON)                               # reflet
    im.put(9 + L, hy + 1, VISOR)
    im.put(10 + L, hy + 1, SKIN)
    im.put(11 + L, hy + 1, SKIN)
    im.put(12 + L, hy + 1, VISOR)
    im.put(9 + L, hy - 1, VISOR)                           # bord superieur du verre
    im.put(10 + L, hy - 1, VISOR)
    im.put(11 + L, hy - 1, VISOR)
    im.put(10 + L, hy + 2, SKINS)                          # menton
    im.put(11 + L, hy + 2, SKIN)
    im.vline(8 + L, hy + 4, sh - 2, METALD)                # col
    # membres avant
    leg(9 + L, legs[0], True)
    arm(10 + L, arms[0], True)
    return im


def lying(flip=False):
    """joueur allonge (chute) ou en extension (plongeon), 16 px de long."""
    im = Img(FW, FH, 0)
    y0 = 17
    for y in range(y0 - 3, y0 + 3):                 # casque
        for x in range(0, 6):
            if (x - 2.5) ** 2 + (y - y0) ** 2 <= 6.5:
                im.put(x, y, KIT if y < y0 else METALD)
    im.put(1, y0 - 3, METAL)
    im.put(2, y0 - 3, METAL)
    im.vline(0, y0, y0 + 1, VISOR)
    im.rect(5, y0 - 2, 10, y0 + 2, KIT)
    im.hline(5, 10, y0 + 2, KITD)
    im.hline(5, 7, y0 - 2, METAL)                   # epauliere
    im.hline(7, 9, y0, NEON)
    im.rect(11, y0 - 2, 12, y0 + 2, SHORT)
    im.hline(13, 14, y0 - 1, METAL)
    im.hline(13, 15, y0 + 1, METALD)
    im.put(15, y0 - 1, BOOT)
    im.put(15, y0 + 1, NEON)
    im.hline(6, 9, y0 + 3, KIT)                     # bras
    im.put(10, y0 + 3, METAL)
    if flip:
        out = Img(FW, FH, 0)
        for y in range(FH):
            for x in range(FW):
                out.put(15 - x, y, im.get(x, y))
        return out
    return im


def outline(im):
    out = Img(FW, FH, 0)
    for y in range(FH):
        for x in range(FW):
            v = im.get(x, y)
            if v:
                out.put(x, y, v)
            elif any(im.get(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out.put(x, y, OUT)
    return out


def with_shadow(im, y=FEET + 1, rx=5):
    for x in range(8 - rx, 8 + rx):
        for dy in (0, 1):
            if dy == 1 and (x < 8 - rx + 2 or x > 8 + rx - 3):
                continue
            if im.get(x, y + dy) == 0:
                im.put(x, y + dy, OUT)
    return im


def player_frames():
    f = []
    f.append(figure((6, -6), (15, -15)))                       # 0 stand
    f.append(figure((42, -42), (-35, 40), lean=1))             # 1 run1
    f.append(figure((5, -5), (5, -5), lean=1, crouch=1))       # 2 run2 (appui : corps bas)
    f.append(figure((-42, 42), (40, -35), lean=1))             # 3 run3
    f.append(figure((85, -10), (-60, 60), lean=-1))            # 4 kick
    f.append(figure((10, -15), (120, 100)))                    # 5 throw / pass
    f.append(figure((50, -45), (80, 70), lean=2))              # 6 charge
    f.append(lying())                                          # 7 fall
    f.append(figure((10, -10), (170, -170)))                   # 8 celebrate
    f.append(lying(flip=True))                                 # 9 dive
    f.append(figure((15, -15), (150, 140)))                    # 10 catch
    f.append(figure((20, -30), (60, -60), lean=-1, crouch=1))  # 11 stumble
    f.append(figure((42, -42), (100, 90), lean=1))             # 12 carry run1
    f.append(figure((-42, 42), (100, 90), lean=1))             # 13 carry run2
    f.append(figure((6, -6), (100, 90)))                       # 14 carry stand
    f.append(figure((30, -30), (-90, 90)))                     # 15 jump
    f.append(figure((24, -22), (-20, 24), lean=1))             # 16 run : jambe avant qui descend
    f.append(figure((-22, 24), (24, -20), lean=1))             # 17 run : jambe arriere qui descend
    f.append(figure((-5, 5), (-5, 5), lean=1, crouch=1))       # 18 run : second appui
    f.append(figure((5, -5), (100, 90), lean=1, crouch=1))     # 19 carry : appui
    f.append(figure((115, -5), (-80, 75), lean=-1))            # 20 kick : accompagnement
    f.append(figure((15, -20), (60, -50), lean=1))             # 21 throw : bras relache
    f.append(figure((6, -6), (10, -10), crouch=1))             # 22 stand : respiration
    f.append(figure((-25, 25), (165, 120)))                    # 23 celebrate : bras qui s'agitent
    out = [outline(im) for im in f]
    for k in range(len(out)):
        if k != 15:
            with_shadow(out[k], rx=6 if k in (7, 9) else 5)
    return out


def small_tiles():
    """tiles 8x8 de la palette 4."""
    t = {}
    ball = ["..3333..", ".322223.", "3211122.", "3211112.", "3221112.", "3322223.", ".333333.", "..4444.."]
    ball = [".4333 4.".replace(" ", "3"), "43222234", "32111223", "32111123", "32211123", "33222233", "43333334", ".444444."]
    t["ball"] = ball
    t["shadow"] = ["........", "........", "........", "........", "..5555..", ".555555.", "..5555..", "........"]
    t["cur1"] = ["66666666", ".666666.", "..6666..", "...66...", "........", "........", "........", "........"]
    t["cur2"] = ["88888888", ".888888.", "..8888..", "...88...", "........", "........", "........", "........"]
    # trainee du ballon (tirs rapides) : grande et petite traces neon
    t["dotA"] = ["........", "...66...", "..6116..", ".611116.", ".611116.", "..6116..", "...66...", "........"]
    t["dotB"] = ["........", "........", "...66...", "..6116..", "..6116..", "...66...", "........", "........"]
    t["dotW"] = ["E.......", "........", "........", "........", "........", "........", "........", "........"]
    t["warn"] = ["...66...", "...66...", "...66...", "...66...", "...66...", "........", "...66...", "........"]
    t["ballhi"] = ["..4444..", ".422224.", "42111124", "42111124", "42211224", "44222244", ".444444.", "..4444.."]
    return t


def build_obj():
    """feuille OBJ 128x128 :
    joueurs : 24 cadres k (16x32) -> tile haut = (k//8)*64 + (k%8)*2, tile bas = haut + 32
    ballon ovale 16x16 : tiles 192.. ; petites tiles 8x8 : 224.. ; ombre ovale : 238"""
    sheet = Img(128, 128, 0)
    for k, im in enumerate(player_frames()):
        bx, by = (k % 8) * 16, (k // 8) * 32
        for y in range(32):
            for x in range(16):
                sheet.put(bx + x, by + y, im.get(x, y))
    st = small_tiles()
    order = ["ball", "shadow", "cur1", "cur2", "dotA", "dotB", "dotW", "warn", "ballhi"]
    for i, name in enumerate(order):
        tx, ty = (224 + i) % 16, (224 + i) // 16
        for r, row in enumerate(st[name]):
            for c, ch in enumerate(row):
                v = 0 if ch == "." else int(ch, 16)
                sheet.put(tx * 8 + c, ty * 8 + r, v)
    # piece du tirage au sort (palettes des equipes) : face (tile 234) et tranche (236)
    for y in range(16):
        for x in range(16):
            dx, dy = x - 7.5, y - 7.5
            d = (dx * dx + dy * dy) ** 0.5
            if d <= 7.6:
                if d > 6.4:
                    c = OUT
                elif d > 5.0:
                    c = METAL if dx + dy < 0 else METALD
                else:
                    c = KITH if dx + dy < -4 else KIT if dx + dy < 3 else KITD
                    # hexagone neon au centre
                    ax, ay = abs(dx), abs(dy)
                    if 1.6 < max(ax * 0.87 + ay * 0.5, ay) < 2.8:
                        c = NEON if dx + dy < 0 else WHITE
                sheet.put(80 + x, 112 + y, c)
            if abs(dx) <= 2.0 and abs(dy) <= 7.4:
                c = OUT if abs(dx) > 1.2 or abs(dy) > 6.6 else (NEON if abs(dy) < 1 else METAL)
                sheet.put(96 + x, 112 + y, c)
    # ballon ovale 16x16 : 4 orientations x 2 phases de rotation (tiles 192 + 2*f), ombre (238)
    for f, im in enumerate(ball_frames()):
        bx, by = f * 16, 96
        for y in range(16):
            for x in range(16):
                sheet.put(bx + x, by + y, im.get(x, y))
    for y in range(16):
        for x in range(16):
            dx, dy = (x - 7.5) / 6.5, (y - 9.5) / 3.0
            if dx * dx + dy * dy <= 1.0:
                sheet.put(112 + x, 112 + y, 5)
    data = bytearray()
    for ty in range(16):
        for tx in range(16):
            px = [sheet.get(tx * 8 + c, ty * 8 + r) for r in range(8) for c in range(8)]
            data += enc_tile(px, 4)
    return sheet, bytes(data)


def ball_frames():
    """ballon ovale futuriste (palette 4) : coque metal bleu, couture neon orange le long du
    grand axe, anneau blanc au centre, pointes lumineuses. Orientation : 0 horizontal,
    1 diagonale descendante, 2 vertical, 3 diagonale montante ; phase = couture qui tourne."""
    out = []
    for o in range(4):
        ang = math.radians(o * 45)
        ca, sa = math.cos(ang), math.sin(ang)
        for ph in range(2):
            im = Img(16, 16, 0)
            A, B = 6.6, 4.2
            for y in range(16):
                for x in range(16):
                    px, py = x - 7.5, y - 7.5
                    u = px * ca + py * sa          # grand axe
                    v = -px * sa + py * ca         # petit axe
                    d = (u / A) ** 2 + (v / B) ** 2
                    if d > 1.0:
                        continue
                    if d > 0.72:
                        c = 5                       # contour
                    else:
                        # eclairage par le haut-gauche
                        lum = -(px * 0.55 + py * 0.8) / 6.0 + (1 - d) * 0.6
                        c = 1 if lum > 1.0 else 2 if lum > 0.7 else 3 if lum > 0.2 else 4
                    # anneau central (petit axe)
                    if abs(u) < 0.6 and d <= 0.72:
                        c = 1 if c in (3, 4) else 15
                    # couture neon le long du grand axe, en tirets qui defilent
                    seam = v * (1 if ph == 0 else -1)
                    if abs(seam - 0.0) < 0.55 and 1.2 < abs(u) < A - 1.0 and d <= 0.8:
                        if int((u + 20 + ph * 1.5) // 1.5) % 2 == 0:
                            c = 13
                        else:
                            c = 6
                    # pointes lumineuses
                    if abs(u) > A - 1.3 and abs(v) < 0.8:
                        c = 6
                    im.put(x, y, c)
            out.append(im)
    return out


# ---------------------------------------------------------------- police BG3
BIG_TITLE = "NEXUSBALL"



# ---------------------------------------------------------------- logo du titre (BG2, palette 3)
LOGO_PAL = [0, c5(1, 1, 4), c5(3, 4, 12),
            c5(6, 14, 31), c5(10, 20, 31), c5(16, 26, 31), c5(26, 30, 31),     # bleus (NEXUS)
            c5(26, 8, 1), c5(31, 14, 2), c5(31, 21, 4), c5(31, 28, 12),         # oranges (BALL)
            c5(8, 12, 22), c5(14, 20, 30), c5(31, 31, 31), c5(2, 2, 6), c5(20, 10, 2)]
LOGO_W, LOGO_H = 224, 72


def build_logo():
    im = Img(LOGO_W, LOGO_H, 0)
    # orbite derriere le texte
    im.ellipse(112, 34, 104, 22, 11, thick=1.5)
    im.ellipse(112, 34, 102, 20, 12, thick=1.0)
    def word(txt, x0, y0, cols, scale=4, slant=0.25):
        mask = Img(LOGO_W, LOGO_H, 0)
        x = x0
        for ch in txt:
            g = FONT[ch]
            for r, row in enumerate(g):
                for k, b in enumerate(row):
                    if b == "1":
                        for dy in range(scale):
                            yy = y0 + r * scale + dy
                            off = int((y0 + 7 * scale - yy) * slant)
                            for dx in range(scale + 2):
                                mask.put(x + k * scale + dx + off, yy, 1)
            x += 6 * scale
        # contour sombre (2 px), puis degrade vertical
        h = 7 * scale
        for y in range(LOGO_H):
            for xx in range(LOGO_W):
                if mask.get(xx, y):
                    continue
                near = any(mask.get(xx + a, y + b) for a in (-2, -1, 0, 1, 2) for b in (-2, -1, 0, 1, 2))
                if near:
                    im.put(xx, y, 1 if any(mask.get(xx + a, y + b) for a in (-1, 0, 1) for b in (-1, 0, 1)) else 14)
        for y in range(LOGO_H):
            for xx in range(LOGO_W):
                if mask.get(xx, y):
                    t = (y - y0) / max(1, h - 1)
                    c = cols[min(len(cols) - 1, int(t * len(cols)))]
                    if mask.get(xx, y - 1) == 0:
                        c = 13 if t < 0.5 else c          # reflet sur le haut des lettres
                    im.put(xx, y, c)
    word("NEXUS", 30, 3, [6, 5, 4, 3, 3, 2])
    word("BALL", 90, 37, [10, 9, 8, 8, 7, 15])
    return im


PANEL_PAL = [0, c5(4, 6, 16), c5(10, 16, 30), c5(8, 24, 31), c5(31, 16, 4), c5(1, 1, 4),
             c5(3, 5, 13),                                   # 6 lignes de balayage
             c5(12, 20, 31), c5(6, 12, 26),                  # 7-8 degrade de la barre
             c5(31, 16, 4),                                  # 9 accent orange
             c5(24, 30, 31),                                 # 10 reflet de la barre
             0, 0, 0, 0, 0]


def panel_tiles():
    """tuiles des panneaux de menu (BG2, palette 4) : fond, barre, 8 bords.
    Fond a lignes de balayage, barre de selection en degrade, cadre neon double a coins
    biseautes avec accent orange."""
    def t(fn):
        return [fn(x, y) for y in range(8) for x in range(8)]
    fill = t(lambda x, y: 6 if y % 2 else 1)                        # lignes de balayage
    bar = t(lambda x, y: [3, 10, 7, 7, 8, 8, 8, 2][y])               # degrade lumineux
    def border(left, right, top, bottom):
        corner = (left or right) and (top or bottom)
        def f(x, y):
            xx = x if left else 7 - x                                 # distance au bord vertical
            yy = y if top else 7 - y                                  # distance au bord horizontal
            if corner:
                if xx + yy < 3:
                    return 0                                          # coin biseaute (transparent)
                if xx + yy == 3:
                    return 3                                          # biseau neon
                if xx + yy == 4:
                    return 2
                if (xx == 2 and yy in (2, 3)) or (yy == 2 and xx == 3):
                    return 9                                          # accent orange
                return 6 if y % 2 else 1
            if (left or right) and xx == 0 or (top or bottom) and yy == 0:
                return 3                                              # filet neon exterieur
            if (left or right) and xx == 1 or (top or bottom) and yy == 1:
                return 5                                              # creux sombre
            if (left or right) and xx == 2 or (top or bottom) and yy == 2:
                return 2                                              # filet interieur
            return 6 if y % 2 else 1
        return t(f)
    return [fill, bar,
            border(1, 0, 1, 0), border(0, 0, 1, 0), border(0, 1, 1, 0),
            border(1, 0, 0, 0), border(0, 1, 0, 0),
            border(1, 0, 0, 1), border(0, 0, 0, 1), border(0, 1, 0, 1)]


def build_logo_data(im):
    tiles, index, tmap = [bytes(32)], {(0,) * 64: 0}, []
    for pt in panel_tiles():                  # tuiles 1..10 : panneaux
        tiles.append(enc_tile(pt, 4))
    for ty in range(32):
        for tx in range(32):
            if 1 <= ty < LOGO_H // 8 + 1 and 2 <= tx < 2 + LOGO_W // 8:
                px = tuple(im.get((tx - 2) * 8 + c, (ty - 1) * 8 + r) for r in range(8) for c in range(8))
            else:
                px = (0,) * 64
            if px not in index:
                index[px] = len(tiles)
                tiles.append(enc_tile(list(px), 4))
            tmap.append(index[px] | (3 << 10) | (1 << 13))
    return b"".join(tiles), b"".join(struct.pack("<H", v) for v in tmap), len(tiles)


# ---------------------------------------------------------------- fond des menus (BG1, palette 2)
MENU_PAL = [c5(0, 0, 3), c5(1, 1, 6), c5(2, 2, 9), c5(4, 2, 12), c5(6, 3, 14), c5(8, 4, 15),
            c5(4, 6, 14), c5(7, 10, 20), c5(31, 14, 4), c5(3, 10, 18),
            c5(8, 22, 31), c5(31, 18, 6),                 # 10-11 neons (cycle "anneaux")
            c5(14, 18, 31), c5(26, 26, 31), c5(10, 12, 24),  # 12-14 etoiles (cycle "public")
            c5(31, 31, 31)]


def build_menubg():
    im = Img(256, 224, 0)
    rng = __import__("random").Random(42)
    bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
    for y in range(224):
        v = y / 150 * 5                      # degrade nuit -> violet jusqu'a l'horizon
        for x in range(256):
            base = int(v)
            frac = v - base
            c = base + (1 if frac * 16 > bayer[y & 3][x & 3] else 0)
            im.put(x, y, max(0, min(5, c)))
    # etoiles
    for _ in range(140):
        x, y = rng.randrange(256), rng.randrange(140)
        im.put(x, y, rng.choice((12, 13, 14, 14, 15)))
    # planete annelee en haut a droite
    cx, cy, r = 204, 58, 34
    im.ellipse(cx, cy + 2, 58, 10, 11, thick=1.0, clip=lambda x, y: y > cy + 2)
    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            d = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
            if d < r:
                shade = 7 if (x - cx) * 0.6 + (cy - y) > r * 0.1 else 6
                if d > r - 2:
                    shade = 8
                im.put(x, y, shade)
    im.ellipse(cx, cy + 2, 58, 10, 11, thick=1.0, clip=lambda x, y: y >= cy + 2 or ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5 >= r)
    # sol en perspective (grille neon)
    hy = 152
    im.hline(0, 255, hy, 10)
    for k in range(1, 9):
        y = hy + int(k * k * 1.0)
        if y < 224:
            im.hline(0, 255, y, 9)
    for k in range(-12, 13):
        x0 = 128 + k * 12
        x1 = 128 + k * 60
        for y in range(hy, 224):
            t = (y - hy) / (224 - hy)
            im.put(int(x0 + (x1 - x0) * t), y, 9)
    return im


def build_font():
    data = bytearray()
    for opaque in (0, 1):
        for code in range(32, 96):
            ch = chr(code)
            px = [1 if opaque else 0] * 64
            g = FONT.get(ch)
            if g:
                for r, row in enumerate(g):
                    for k, b in enumerate(row):
                        if b == "1":
                            px[r * 8 + k + 1] = 2
                if not opaque:
                    # ombre portee (couleur 1) en bas a droite pour la lisibilite sur les fonds
                    for r in range(7, -1, -1):
                        for c in range(7, -1, -1):
                            if px[r * 8 + c] == 2:
                                for dr, dc in ((1, 1), (0, 1), (1, 0)):
                                    rr, cc = r + dr, c + dc
                                    if rr < 8 and cc < 8 and px[rr * 8 + cc] == 0:
                                        px[rr * 8 + cc] = 1
            if opaque and code == 95:  # '_' -> bord accentue
                px = [3] * 8 + [1] * 56
            data += enc_tile(px, 2)
    letters = []
    hud = build_hud_tiles(128)
    for t in hud["tiles"]:
        data += enc_tile(t, 2)
    assert len(data) // 16 <= 256, len(data) // 16
    return bytes(data), letters, hud["syms"]


def build_hud_tiles(base):
    """Tiles 2bpp du HUD de match (a la suite de la police).
    Plaques d'equipe (palettes 1 / 3 : 1 sombre, 2 couleur d'equipe, 3 blanc) : 16 px de haut,
    lettres A-Z blanches ombrees, extremites biseautees.
    Cadre du score (palette 6 : 1 fond, 2 chiffres blancs, 3 filet cyan) : gros chiffres 8x16."""
    tiles, syms = [], {}

    def add(name, im16):
        syms[name] = base + len(tiles)
        for ty in range(im16.h // 8):
            tiles.append([im16.get(c, ty * 8 + r) for r in range(8) for c in range(8)])

    def plate(ch=None, cap=0):
        im = Img(8, 16, 0)
        for y in range(16):
            for x in range(8):
                if cap == 1 and x < (15 - y) // 2:
                    continue
                if cap == 2 and x > 7 - (y // 2) + 0:
                    continue
                edge = y in (0, 15) or (cap == 1 and x == (15 - y) // 2) or (cap == 2 and x == 7 - y // 2)
                im.put(x, y, 1 if edge else 2)
        for x in range(8):
            if im.get(x, 1) == 2:
                im.put(x, 1, 3 if cap == 0 else 2)
        if ch:
            g = FONT[ch]
            for r, row in enumerate(g):
                for k, b in enumerate(row):
                    if b == "1":
                        im.put(k + 2, r + 5, 1)
            for r, row in enumerate(g):
                for k, b in enumerate(row):
                    if b == "1":
                        im.put(k + 1, r + 4, 3)
        return im

    add("PL_BLANK", plate())
    add("PL_CAPL", plate(cap=1))
    add("PL_CAPR", plate(cap=2))
    for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        add("PL_" + ch, plate(ch))

    def box(top=True, cap=0):
        im = Img(8, 16, 0)
        for y in range(16):
            for x in range(8):
                if cap == 1 and x < (7 - y) and y <= 7:
                    continue
                if cap == 2 and x > 7 - (7 - y) and y <= 7:
                    continue
                im.put(x, y, 1)
        if top:
            for x in range(8):
                if im.get(x, 0) == 1:
                    im.put(x, 0, 3)
        if cap == 1:
            for y in range(16):
                for x in range(8):
                    if im.get(x, y) == 1:
                        im.put(x, y, 3)
                        break
        if cap == 2:
            for y in range(16):
                for x in range(7, -1, -1):
                    if im.get(x, y) == 1:
                        im.put(x, y, 3)
                        break
        return im

    add("SB_BLANK", box())
    add("SB_CAPL", box(cap=1))
    add("SB_CAPR", box(cap=2))
    for d in "0123456789":
        im = box()
        g = FONT[d]
        for r, row in enumerate(g):
            for k, b in enumerate(row):
                if b == "1":
                    for yy in (r * 2 + 1, r * 2 + 2):
                        im.put(k + 1, yy, 2)
                        im.put(k + 2, yy, 2)
        add("SB_" + d, im)
    # tiret centre sur la frontiere de deux colonnes (moitie droite / moitie gauche)
    for name, xs in (("SB_DASHL", range(5, 8)), ("SB_DASHR", range(0, 3))):
        im = box()
        for x in xs:
            for y in (8, 9):
                im.put(x, y, 3)
        add(name, im)
    # coins bas de la ligne du temps (8x8)
    for name, side in (("TM_L", 1), ("TM_R", 2)):
        syms[name] = base + len(tiles)
        t = []
        for y in range(8):
            for x in range(8):
                lim = y + 1
                inside = x >= lim if side == 1 else x <= 7 - lim
                edge = x == lim if side == 1 else x == 7 - lim
                t.append(3 if edge else (1 if inside else 0))
        tiles.append(t)
    # bandeau des messages du match (8x8) : 1 fond sombre, 2 couleur d'accent, 3 blanc
    def tile8(name, f):
        syms[name] = base + len(tiles)
        tiles.append([f(x, y) for y in range(8) for x in range(8)])

    def top(x, y, cut):
        if y < 3:
            return 0
        if cut == 1 and x < 7 - (y - 3) * 2:
            return 0
        if cut == 2 and x > (y - 3) * 2:
            return 0
        if y in (3, 4):
            return 3 if y == 3 and cut == 0 and x % 4 == 0 else 2
        return 1

    def bot(x, y, cut):
        if y > 4:
            return 0
        if cut == 1 and x < y * 2 - 1:
            return 0
        if cut == 2 and x > 8 - y * 2:
            return 0
        if y in (3, 4):
            return 2
        return 1
    tile8("BN_TL", lambda x, y: top(x, y, 1))
    tile8("BN_T", lambda x, y: top(x, y, 0))
    tile8("BN_TR", lambda x, y: top(x, y, 2))
    tile8("BN_BL", lambda x, y: bot(x, y, 1))
    tile8("BN_B", lambda x, y: bot(x, y, 0))
    tile8("BN_BR", lambda x, y: bot(x, y, 2))
    tile8("BN_L", lambda x, y: 2 if x in (2, 3) else (3 if x == 5 and 2 <= y <= 5 else (1 if x > 3 else 0)))
    tile8("BN_R", lambda x, y: 2 if x in (4, 5) else (3 if x == 2 and 2 <= y <= 5 else (1 if x < 4 else 0)))
    tile8("BN_CHEV", lambda x, y: 2 if (abs(y - 3.5) < 4 and (x - abs(y - 3.5)) in (1, 2, 4, 5)) else 1)
    return {"tiles": tiles, "syms": syms}


def main():
    os.makedirs(GEN, exist_ok=True)
    os.makedirs(PREV, exist_ok=True)
    counts = []
    for n, st in enumerate(STADIUMS):
        field = build_field(st)
        tiles, tmap = tiles_from_image(field)
        if len(tiles) > 1024:
            sys.exit("trop de tiles terrain: %d" % len(tiles))
        counts.append(len(tiles))
        with open(os.path.join(GEN, "stad%d.chr" % n), "wb") as f:
            for t in tiles:
                f.write(enc_tile(t, 4))
        with open(os.path.join(GEN, "stad%d.map" % n), "wb") as f:
            # ordre SNES 64x64 : 4 ecrans 32x32 (TL, TR, BL, BR)
            for scr in range(4):
                sx, sy = (scr & 1) * 32, (scr >> 1) * 32
                for y in range(32):
                    for x in range(32):
                        f.write(struct.pack("<H", tmap[(sy + y) * 64 + sx + x]))
        with open(os.path.join(GEN, "stad%d.pal" % n), "wb") as f:
            for c in st["pal"]:
                f.write(struct.pack("<H", c))
        rows = [[v for x in range(field.w) for v in rgb(st["pal"][field.get(x, y)])] for y in range(field.h)]
        write_png(os.path.join(PREV, "preview_stad%d.png" % n), field.w, field.h, rows)
    with open(os.path.join(GEN, "stadiums.inc"), "w") as f:
        f.write("; genere par tools/gfx.py\nNUM_STADIUMS = %d\n" % len(STADIUMS))
        for n, st in enumerate(STADIUMS):
            seg = "DATA2"
            f.write('.segment "%s"\nstad%d_chr: .incbin "data/gen/stad%d.chr.lz"\n'
                    'stad%d_map: .incbin "data/gen/stad%d.map.lz"\n' % (seg, n, n, n, n))
        f.write('.segment "DATA0"\n')
        for n, st in enumerate(STADIUMS):
            f.write('stad%d_pal: .incbin "data/gen/stad%d.pal"\n' % (n, n))
        f.write('.segment "RODATA"\n')
        for n, st in enumerate(STADIUMS):
            f.write('stad%d_name: .byte "%s", 0\n' % (n, st["name"]))
        f.write("; par stade : chr compresse (long), reserve, map compressee (long), palette, nom\nstadium_tab:\n")
        for n in range(len(STADIUMS)):
            f.write("    .faraddr stad%d_chr\n    .word 0\n    .faraddr stad%d_map\n"
                    "    .word .loword(stad%d_pal), .loword(stad%d_name)\n" % (n, n, n, n))
        f.write('.segment "CODE"\n')
    sheet, obj = build_obj()
    with open(os.path.join(GEN, "obj.chr"), "wb") as f:
        f.write(obj)
    font, letters, hudsyms = build_font()
    with open(os.path.join(GEN, "font.chr"), "wb") as f:
        f.write(font)
    with open(os.path.join(GEN, "bigfont.inc"), "w") as f:
        f.write("; genere par tools/gfx.py : index des gros caracteres (tile = 128 + 4*n)\n")
        for i, ch in enumerate(letters):
            f.write("BIG_%s = %d\n" % (ch, 128 + 4 * i))
        f.write("; tiles du HUD de match (16 px : tile du haut, celle du bas = +1)\n")
        for k, v in hudsyms.items():
            f.write("HT_%s = %d\n" % (k, v))
    pal = [0] * 256
    for i, c in enumerate(BG3_PAL):
        pal[i] = c
    for i, c in enumerate(FIELD_PAL):
        pal[32 + i] = c
    for p, cols in enumerate(OBJ_PALS):
        for i, c in enumerate(cols):
            pal[128 + p * 16 + i] = c
    for i, c in enumerate(LOGO_PAL):
        pal[48 + i] = c
    for i, c in enumerate(PANEL_PAL):
        pal[64 + i] = c
    # tribunes (BG2) : tile 0 vide, 1..10 panneaux (comme le logo), puis la foule
    crowd = build_crowd()
    ct = [bytes(32)] + [enc_tile(pt, 4) for pt in panel_tiles()]
    ci, cm = {}, []
    for ty in range(32):
        for tx in range(32):
            px = tuple(crowd.get(tx * 8 + c, ty * 8 + r) for r in range(8) for c in range(8))
            if px not in ci:
                ci[px] = len(ct)
                ct.append(enc_tile(list(px), 4))
            cm.append(ci[px] | (2 << 10))
    assert len(ct) <= 256, len(ct)
    open(os.path.join(GEN, "crowd.chr"), "wb").write(b"".join(ct))
    open(os.path.join(GEN, "crowd.map"), "wb").write(b"".join(struct.pack("<H", v) for v in cm))
    print("tribunes : %d tiles" % len(ct))
    import scenes
    scenes.FONT = FONT
    def scene_out(name, cv, groups):
        st, sm = scenes.to_tiles(cv, groups)
        assert len(st) <= 1024, len(st)
        open(os.path.join(GEN, name + ".chr"), "wb").write(b"".join(enc_tile(list(t), 4) for t in st))
        open(os.path.join(GEN, name + ".map"), "wb").write(b"".join(struct.pack("<H", v) for v in sm))
        cols = []
        for g in sorted(groups):
            cols += groups[g]
        if name.startswith("ad"):
            cols += [0] * (64 - len(cols))
        open(os.path.join(GEN, name + ".pal"), "wb").write(b"".join(struct.pack("<H", c) for c in cols))
        write_png(os.path.join(PREV, "preview_%s.png" % name), cv.w, cv.h, scenes.preview(cv, groups))
        print("%s : %d tiles" % (name, len(st)))
    scene_out("menubg", scenes.build_menubg(), scenes.MENU_GROUPS)
    for n in range(2):
        scene_out("ad%d" % n, scenes.build_ad(n), scenes.AD_GROUPS)
    scene_out("ad2", scenes.build_offgame(), scenes.LOGO_GROUPS)
    pal[0] = FIELD_PAL[0]
    logo = build_logo()
    lchr, lmap, lcount = build_logo_data(logo)
    assert len(lchr) <= 8192, len(lchr)
    open(os.path.join(GEN, "logo.chr"), "wb").write(lchr)
    open(os.path.join(GEN, "logo.map"), "wb").write(lmap)
    rows = [[v for x in range(logo.w) for v in (rgb(LOGO_PAL[logo.get(x, y)]) if logo.get(x, y) else (20, 30, 60))] for y in range(logo.h)]
    write_png(os.path.join(PREV, "preview_logo.png"), logo.w, logo.h, rows)
    with open(os.path.join(GEN, "pal.bin"), "wb") as f:
        for c in pal:
            f.write(struct.pack("<H", c))
    with open(os.path.join(GEN, "tables.inc"), "w") as f:
        f.write("; genere par tools/gfx.py\ncos_tab:\n")
        cs = [round(127 * math.cos(2 * math.pi * k / 64)) & 0xFF for k in range(64)]
        for i in range(0, 64, 16):
            f.write("    .byte " + ",".join("$%02X" % v for v in cs[i:i + 16]) + "\n")
        at = [round(math.atan(r / 32) * 64 / (2 * math.pi)) for r in range(33)]
        f.write("atan_tab:\n    .byte " + ",".join(str(v) for v in at) + "\n")
    # apercus
    sc = 4
    rows = []
    for y in range(128 * sc):
        row = []
        for x in range(128 * sc):
            sx, sy = x // sc, y // sc
            v = sheet.get(sx, sy)
            pidx = 4 if sy >= 64 else 0
            row += rgb(OBJ_PALS[pidx][v]) if v else (40, 40, 60)
        rows.append(row)
    write_png(os.path.join(PREV, "preview_obj.png"), 128 * sc, 128 * sc, rows)
    print("stades: %s tiles, obj: %d octets, font: %d octets" % (counts, len(obj), len(font)))


if __name__ == "__main__":
    main()
