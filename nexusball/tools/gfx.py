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
    c5(0, 0, 0), c5(2, 3, 8), c5(10, 20, 31), c5(31, 24, 6),
    c5(0, 0, 0), c5(1, 1, 4), c5(31, 27, 8), c5(31, 31, 31),
    c5(0, 0, 0), c5(2, 3, 8), c5(31, 15, 3), c5(31, 24, 6),
    c5(0, 0, 0), c5(2, 3, 8), c5(31, 18, 4), c5(31, 26, 8),     # 4 : texte accentue (menus)
    c5(0, 0, 0), c5(2, 3, 8), c5(12, 24, 31), c5(31, 26, 8),    # 5 : equipe humaine (menus)
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
    dict(name="ORBITAL ARENA", words=("NEXUS", "ARENA"), floor="check",
         pal=FIELD_PAL),
    dict(name="MARS DOME", words=("MARS", "DOME"), floor="stripe",
         pal=stadium_pal((9, 4, 3), (11, 5, 3), (14, 7, 4), (28, 24, 20), (30, 22, 6), (6, 3, 2), (11, 6, 4),
                         (16, 9, 6), (31, 10, 4), (31, 14, 2), (31, 27, 10), ((22, 6, 4), (12, 4, 14), (26, 16, 6)))),
    dict(name="EUROPA ICE", words=("EUROPA", "ICE"), floor="grid",
         pal=stadium_pal((4, 7, 11), (5, 9, 13), (8, 13, 17), (28, 30, 31), (31, 18, 4), (5, 7, 10), (10, 14, 19),
                         (16, 21, 26), (20, 28, 31), (31, 14, 2), (31, 27, 10), ((6, 14, 24), (24, 26, 30), (4, 8, 16)))),
    dict(name="ANDROMEDA PRIME", words=("ANDRO", "PRIME"), floor="diamond",
         pal=stadium_pal((5, 3, 9), (7, 4, 12), (10, 6, 16), (26, 24, 31), (8, 26, 22), (4, 2, 7), (9, 5, 14),
                         (14, 9, 20), (26, 8, 30), (31, 14, 2), (31, 27, 10), ((20, 6, 24), (6, 10, 26), (28, 16, 28)))),
    dict(name="TITAN INDUSTRIAL", words=("TITAN", "WORKS"), floor="plate",
         pal=stadium_pal((6, 6, 6), (8, 8, 8), (11, 11, 10), (29, 29, 26), (31, 24, 2), (4, 4, 4), (9, 9, 9),
                         (14, 14, 13), (31, 22, 2), (31, 12, 2), (31, 27, 10), ((16, 16, 16), (24, 20, 6), (10, 10, 12)))),
    dict(name="SOLARIS ARENA", words=("SOLAR", "ARENA"), floor="stripe",
         pal=stadium_pal((10, 6, 3), (12, 8, 4), (15, 10, 5), (31, 30, 24), (8, 22, 30), (7, 4, 2), (13, 8, 4),
                         (20, 13, 6), (31, 26, 6), (31, 12, 2), (31, 30, 14), ((30, 20, 4), (26, 10, 6), (31, 28, 12)))),
]

def team_pal(main, dark, trim, shorts, hi):
    return [0, c5(2, 2, 4), c5(27, 20, 15), c5(19, 12, 8), c5(6, 4, 3),
            main, dark, trim, shorts, c5(3, 3, 5), hi, 0, 0, 0, 0, 0]


OBJ_PALS = [
    team_pal(c5(4, 12, 30), c5(2, 6, 18), c5(28, 30, 31), c5(3, 5, 14), c5(12, 20, 31)),   # 0 equipe A bleue
    team_pal(c5(31, 13, 2), c5(20, 6, 1), c5(31, 28, 20), c5(10, 4, 2), c5(31, 22, 10)),   # 1 equipe B orange
    team_pal(c5(4, 24, 8), c5(1, 13, 4), c5(28, 31, 24), c5(2, 8, 3), c5(14, 31, 14)),     # 2 gardien A vert
    team_pal(c5(30, 26, 2), c5(18, 14, 0), c5(31, 31, 24), c5(12, 9, 0), c5(31, 31, 14)),  # 3 gardien B jaune
    [0, c5(31, 31, 31), c5(20, 30, 31), c5(8, 22, 31), c5(2, 6, 16), c5(1, 1, 3),
     c5(31, 29, 6), c5(24, 12, 0), c5(31, 8, 8), c5(1, 2, 6), c5(8, 18, 28), c5(4, 9, 16),
     c5(8, 18, 31), c5(31, 15, 3), c5(31, 31, 31), c5(29, 30, 31)],                     # 4 ballon / curseurs / radar
]


# ---------------------------------------------------------------- police 5x7
FONT = {
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
def build_field(st):
    im = Img(WORLD_W, WORLD_H, 0)
    # tribunes (motif periodique 16x8 pour limiter le nombre de tiles)
    crowd = [
        "..a...b...c...a.",
        ".aaa.bbb.ccc.aaa",
        ".ofo.ofo.ofo.ofo",
        "..o...o...o...o.",
        "c...a...b...c...",
        "cc.aaa.bbb.ccc.c",
        "fo.ofo.ofo.ofo.o",
        "o...o...o...o...",
    ]
    cmap = {".": 0, "a": 12, "b": 13, "c": 14, "o": 0, "f": 15}
    for y in range(0, 56):
        for x in range(WORLD_W):
            row = crowd[y % 8]
            ch = row[(x + (y // 8) * 4) % 16]
            im.put(x, y, cmap[ch])
    for y in range(304, WORLD_H):
        for x in range(WORLD_W):
            row = crowd[y % 8]
            im.put(x, y, cmap[row[(x + 8) % 16]] if y >= 306 else 0)
    # mur du haut (face visible)
    im.rect(0, 56, WORLD_W - 1, 71, 7)
    im.hline(0, WORLD_W - 1, 56, 8)
    im.hline(0, WORLD_W - 1, 57, 8)
    im.hline(0, WORLD_W - 1, 69, 6)
    im.hline(0, WORLD_W - 1, 70, 9)
    im.hline(0, WORLD_W - 1, 71, 6)
    w1, w2 = st["words"]
    for x0 in range(0, WORLD_W, 128):
        im.rect(x0 + 8, 59, x0 + 63, 67, 6)
        im.text(w1, x0 + 36 - len(w1) * 3, 60, 9)
        im.rect(x0 + 72, 59, x0 + 119, 67, 6)
        im.text(w2, x0 + 96 - len(w2) * 3, 60, 11)
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
    # murs lateraux / fond
    for side in (0, 1):
        x0, x1 = (0, FIELD_L - 1) if side == 0 else (FIELD_R + 1, WORLD_W - 1)
        im.rect(x0, 56, x1, 303, 7)
        edge = x1 if side == 0 else x0
        im.vline(edge, 56, 303, 9)
        im.vline(edge - 1 if side == 0 else edge + 1, 56, 303, 6)
        for y in range(64, 300, 16):
            im.hline(x0 + 2, x1 - 2, y, 6)
    # mur du bas (vu de dessus)
    im.rect(0, 296, WORLD_W - 1, 303, 8)
    im.hline(0, WORLD_W - 1, 296, 9)
    im.hline(0, WORLD_W - 1, 303, 6)
    im.rect(0, 296, FIELD_L - 1, 303, 8)
    im.rect(FIELD_R + 1, 296, WORLD_W - 1, 303, 8)

    inside = lambda x, y: FIELD_L <= x <= FIELD_R and FIELD_T <= y <= FIELD_B
    # lignes
    im.vline(256, FIELD_T, FIELD_B, 4)
    im.ellipse(256, FIELD_CY, 40, 40, 4)
    im.rect(255, FIELD_CY - 1, 257, FIELD_CY + 1, 4)
    for gx, sgn in ((FIELD_L, 1), (FIELD_R, -1)):
        im.ellipse(gx, FIELD_CY, ZONE_R, ZONE_R, 4, clip=lambda x, y, gx=gx, s=sgn: inside(x, y) and (x - gx) * s >= 0)
        im.ellipse(gx, FIELD_CY, LONG_R, LONG_R * 0.9, 5, thick=1.0,
                   clip=lambda x, y, gx=gx, s=sgn: inside(x, y) and (x - gx) * s >= 0, dash=4)
    # anneaux
    for gx in (FIELD_L, FIELD_R):
        cy = FIELD_CY - RING_Z
        im.rect(gx - 1, cy + RING_R - 2, gx + 1, FIELD_CY + 2, 6)  # pied
        im.ellipse(gx, cy, 8, RING_R + 1, 10, thick=1.5)
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


def figure(legs=(8, -8), arms=(20, -20), lean=0, arm_lift=0):
    im = Img(16, 16, 0)
    L = lean
    # bras / jambe arriere
    limb(im, 7 + L, 11, legs[1], 4.4, SKINS, BOOT, 2)
    limb(im, 6 + L, 6, arms[1], 4.0, KITD, SKINS, 2)
    # torse (5 px) et short
    im.rect(5 + L, 5, 10 + L, 9, KIT)
    im.vline(5 + L, 5, 9, KITD)
    im.vline(6 + L, 6, 9, KITD)
    im.hline(5 + L, 10 + L, 5, KITH)
    im.vline(8 + L, 6, 8, TRIM)
    im.rect(5 + L, 10, 10 + L, 11, SHORT)
    # tete
    for y in range(0, 6):
        for x in range(4 + L, 12 + L):
            d = (x - 8.0 - L) ** 2 * 1.1 + (y - 2.6) ** 2
            if d <= 7.2:
                im.put(x, y, HAIR if (y <= 1 or x <= 6 + L) else SKIN)
    im.put(10 + L, 3, SKINS)
    im.put(9 + L, 2, OUT)
    # jambe / bras avant
    limb(im, 9 + L, 11, legs[0], 4.4, SKIN, BOOT, 2)
    limb(im, 9 + L, 6, arms[0], 4.0, KIT, SKIN, 2)
    return im


def outline(im):
    out = Img(16, 16, 0)
    for y in range(16):
        for x in range(16):
            v = im.get(x, y)
            if v:
                out.put(x, y, v)
            elif any(im.get(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out.put(x, y, OUT)
    return out


def rot90(im, cw=True):
    out = Img(16, 16, 0)
    for y in range(16):
        for x in range(16):
            if cw:
                out.put(15 - y, x, im.get(x, y))
            else:
                out.put(y, 15 - x, im.get(x, y))
    return out


def shift(im, dx, dy):
    out = Img(16, 16, 0)
    for y in range(16):
        for x in range(16):
            out.put(x + dx, y + dy, im.get(x, y))
    return out


def player_frames():
    f = []
    f.append(figure((6, -6), (15, -15)))                       # 0 stand
    f.append(figure((40, -40), (-35, 40), lean=1))             # 1 run1
    f.append(figure((5, -5), (5, -5), lean=1))                 # 2 run2
    f.append(figure((-40, 40), (40, -35), lean=1))             # 3 run3
    f.append(figure((85, -10), (-60, 60), lean=-1))            # 4 kick
    f.append(figure((10, -15), (120, 100)))                    # 5 throw / pass
    f.append(figure((45, -45), (80, 70), lean=2))              # 6 charge
    f.append(shift(rot90(figure((10, -10), (60, 40)), cw=False), 0, 0))  # 7 fall (allonge)
    f.append(figure((10, -10), (170, -170)))                   # 8 celebrate
    f.append(rot90(figure((20, -10), (175, 160)), cw=True))    # 9 dive (gardien)
    f.append(figure((15, -15), (150, 140)))                    # 10 catch
    f.append(figure((20, -30), (60, -60), lean=-1))            # 11 stumble
    f.append(figure((40, -40), (100, 90), lean=1))             # 12 carry run1 (ballon en mains)
    f.append(figure((-40, 40), (100, 90), lean=1))             # 13 carry run2
    f.append(figure((6, -6), (100, 90)))                       # 14 carry stand
    f.append(figure((30, -30), (-90, 90)))                     # 15 jump
    out = []
    for im in f:
        # fall/dive : poser au sol
        out.append(outline(im))
    out[7] = outline(shift(rot90(figure((10, -10), (60, 40)), cw=False), 0, 4))
    out[9] = outline(shift(rot90(figure((20, -10), (175, 160)), cw=True), 0, 2))
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
    t["dotA"] = ["CC......", "CC......", "........", "........", "........", "........", "........", "........"]
    t["dotB"] = ["DD......", "DD......", "........", "........", "........", "........", "........", "........"]
    t["dotW"] = ["E.......", "........", "........", "........", "........", "........", "........", "........"]
    t["warn"] = ["...66...", "...66...", "...66...", "...66...", "...66...", "........", "...66...", "........"]
    t["ballhi"] = ["..4444..", ".422224.", "42111124", "42111124", "42211224", "44222244", ".444444.", "..4444.."]
    return t


def panel_frames():
    """radar : coin haut-gauche, haut, coin bas-gauche, bas (16x16, palette 4)."""
    def mk(top, bottom, left):
        im = Img(16, 16, 9)
        if top:
            im.hline(0, 15, 0, 10)
        if bottom:
            im.hline(0, 15, 15, 10)
        if left:
            im.vline(0, 0, 15, 10)
        return im
    return [mk(1, 0, 1), mk(1, 0, 0), mk(0, 1, 1), mk(0, 1, 0)]


def build_obj():
    sheet = Img(128, 128, 0)   # 16 tiles de large
    frames = player_frames() + panel_frames()
    for k, im in enumerate(frames):
        bx, by = (k % 8) * 16, (k // 8) * 16
        for y in range(16):
            for x in range(16):
                sheet.put(bx + x, by + y, im.get(x, y))
    # petites tiles a partir de la tile 96 (ligne 6)
    st = small_tiles()
    order = ["ball", "shadow", "cur1", "cur2", "dotA", "dotB", "dotW", "warn", "ballhi"]
    for i, name in enumerate(order):
        tx, ty = (96 + i) % 16, (96 + i) // 16
        for r, row in enumerate(st[name]):
            for c, ch in enumerate(row):
                v = 0 if ch == "." else int(ch, 16)
                sheet.put(tx * 8 + c, ty * 8 + r, v)
    data = bytearray()
    for ty in range(16):
        for tx in range(16):
            px = [sheet.get(tx * 8 + c, ty * 8 + r) for r in range(8) for c in range(8)]
            data += enc_tile(px, 4)
    return sheet, bytes(data)


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
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0]


def panel_tiles():
    """tuiles des panneaux de menu (BG2, palette 4) : fond, barre, 8 bords."""
    def t(fn):
        return [fn(x, y) for y in range(8) for x in range(8)]
    fill = t(lambda x, y: 1)
    bar = t(lambda x, y: 3 if y == 0 else (2 if y < 7 else 1))
    def border(left, right, top, bottom):
        def f(x, y):
            if (left and x == 0) or (right and x == 7) or (top and y == 0) or (bottom and y == 7):
                return 5
            if (left and x == 1) or (right and x == 6) or (top and y == 1) or (bottom and y == 6):
                corner = (left or right) and (top or bottom)
                return 4 if corner else 3
            return 1
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
    # gros titre : lettres 16x16 (glyph 5x7 x2 + ombre) -> 4 tiles chacune, a partir de 128
    letters = sorted(set(BIG_TITLE))
    for ch in letters:
        im = Img(16, 16, 0)
        g = FONT[ch]
        for r, row in enumerate(g):
            for k, b in enumerate(row):
                if b == "1":
                    im.rect(k * 2 + 2, r * 2 + 2, k * 2 + 3, r * 2 + 3, 1)
        for r, row in enumerate(g):
            for k, b in enumerate(row):
                if b == "1":
                    im.rect(k * 2 + 1, r * 2 + 1, k * 2 + 2, r * 2 + 2, 3 if r < 3 else 2)
        for ty in (0, 1):
            for tx in (0, 1):
                data += enc_tile([im.get(tx * 8 + c, ty * 8 + r) for r in range(8) for c in range(8)], 2)
    return bytes(data), letters


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
        f.write('.segment "RODATA"\n')
        for n, st in enumerate(STADIUMS):
            f.write('stad%d_pal: .incbin "data/gen/stad%d.pal"\n' % (n, n))
            f.write('stad%d_name: .byte "%s", 0\n' % (n, st["name"]))
        f.write("; par stade : chr compresse (long), reserve, map compressee (long), palette, nom\nstadium_tab:\n")
        for n in range(len(STADIUMS)):
            f.write("    .faraddr stad%d_chr\n    .word 0\n    .faraddr stad%d_map\n"
                    "    .word .loword(stad%d_pal), .loword(stad%d_name)\n" % (n, n, n, n))
        f.write('.segment "CODE"\n')
    sheet, obj = build_obj()
    with open(os.path.join(GEN, "obj.chr"), "wb") as f:
        f.write(obj)
    font, letters = build_font()
    with open(os.path.join(GEN, "font.chr"), "wb") as f:
        f.write(font)
    with open(os.path.join(GEN, "bigfont.inc"), "w") as f:
        f.write("; genere par tools/gfx.py : index des gros caracteres (tile = 128 + 4*n)\n")
        for i, ch in enumerate(letters):
            f.write("BIG_%s = %d\n" % (ch, 128 + 4 * i))
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
    mbg = build_menubg()
    mt, mi, mm = [], {}, []
    for ty in range(32):
        for tx in range(32):
            px = tuple(mbg.get(tx * 8 + c, ty * 8 + r) for r in range(8) for c in range(8)) if ty < 28 else (0,) * 64
            if px not in mi:
                mi[px] = len(mt)
                mt.append(px)
            mm.append(mi[px] | (2 << 10))
    assert len(mt) <= 1024
    open(os.path.join(GEN, "menubg.chr"), "wb").write(b"".join(enc_tile(list(t), 4) for t in mt))
    open(os.path.join(GEN, "menubg.map"), "wb").write(b"".join(struct.pack("<H", v) for v in mm))
    open(os.path.join(GEN, "menubg.pal"), "wb").write(b"".join(struct.pack("<H", c) for c in MENU_PAL))
    rows = [[v for x in range(256) for v in rgb(MENU_PAL[mbg.get(x, y)])] for y in range(224)]
    write_png(os.path.join(PREV, "preview_menubg.png"), 256, 224, rows)
    print("fond des menus : %d tiles" % len(mt))
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
