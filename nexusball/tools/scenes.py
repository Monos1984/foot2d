#!/usr/bin/env python3
"""NEXUS BALL - grands decors BG1 multi-palettes : fond des menus et ecrans de publicite.

Couleurs globales : id = 16 * groupe + index. Les index 0..5 (degrade du ciel) sont communs a
toutes les palettes : un id < 6 est valable dans n'importe quel groupe.
Groupe -> palette BG : 1 -> 2 (CGRAM 32), 2 -> 5 (CGRAM 80), 3 -> 6 (CGRAM 96), 4 -> 7 (CGRAM 112).
Chaque tile 8x8 prend la palette du groupe majoritaire ; une couleur d'un autre groupe est
remplacee par la plus proche de cette palette.
"""
import math
import random


def c5(r, g, b):
    return (r & 31) | ((g & 31) << 5) | ((b & 31) << 10)


SKY = [c5(0, 0, 3), c5(1, 1, 6), c5(2, 2, 9), c5(4, 2, 12), c5(6, 3, 14), c5(8, 4, 15)]
GROUP_PAL = {1: 2, 2: 5, 3: 6, 4: 7}

# fond des menus
MENU_GROUPS = {
    # 1 : ciel, nebuleuse, etoiles (10-11 neons et 12-14 etoiles : couleurs animees)
    1: SKY + [c5(5, 1, 9), c5(10, 3, 15), c5(17, 6, 19), c5(3, 7, 19),
              c5(8, 22, 31), c5(31, 18, 6), c5(14, 18, 31), c5(26, 26, 31), c5(10, 12, 24), c5(31, 31, 31)],
    # 2 : planete, anneaux, lune
    2: SKY + [c5(3, 5, 13), c5(5, 9, 19), c5(8, 13, 24), c5(13, 19, 29), c5(20, 26, 31),
              c5(10, 20, 31), c5(31, 18, 6), c5(22, 11, 4), c5(31, 27, 16), c5(13, 12, 17)],
    # 3 : ville, arene, sol
    3: SKY + [c5(1, 1, 5), c5(3, 3, 9), c5(6, 6, 15), c5(8, 22, 31), c5(31, 18, 6),
              c5(16, 6, 22), c5(3, 9, 17), c5(6, 16, 28), c5(14, 22, 31), c5(29, 30, 31)],
}


def rgb(c):
    return ((c & 31) * 8, (c >> 5 & 31) * 8, (c >> 10 & 31) * 8)


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.p = [[0] * w for _ in range(h)]

    def put(self, x, y, c):
        x, y = int(x), int(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            self.p[y][x] = c

    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.p[y][x]
        return 0

    def rect(self, x0, y0, x1, y1, c):
        for y in range(int(y0), int(y1) + 1):
            for x in range(int(x0), int(x1) + 1):
                self.put(x, y, c)

    def hline(self, x0, x1, y, c):
        self.rect(x0, y, x1, y, c)

    def line(self, x0, y0, x1, y1, c):
        n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
        for i in range(n + 1):
            t = i / n
            self.put(round(x0 + (x1 - x0) * t), round(y0 + (y1 - y0) * t), c)


BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def dith(v, x, y):
    """v reel -> entier avec tramage ordonne"""
    b = int(math.floor(v))
    return b + (1 if (v - b) * 16 > BAYER[y & 3][x & 3] else 0)


def noise(x, y, seed=0):
    return (math.sin(x * 0.045 + seed) * math.cos(y * 0.06 - seed * 0.7)
            + 0.6 * math.sin(x * 0.11 + y * 0.07 + seed * 1.3)
            + 0.35 * math.sin(x * 0.23 - y * 0.19 + seed * 2.1)) / 1.95


def sky(cv, y_end, top=0.0, bottom=5.0, span=None):
    span = span or y_end
    for y in range(y_end):
        v = top + (bottom - top) * y / span
        for x in range(cv.w):
            cv.put(x, y, max(0, min(5, dith(v, x, y))))


def to_tiles(cv, groups, rows=28):
    """-> (tiles 4bpp en listes de 64 index, tilemap 32x32 mots, palettes {slot: 16 couleurs})"""
    pals = {g: groups[g] for g in groups}
    tiles, index, tmap = [], {}, []
    for ty in range(32):
        for tx in range(32):
            if ty >= rows or tx * 8 >= cv.w:
                px, g = (0,) * 64, min(groups)
            else:
                ids = [cv.get(tx * 8 + c, ty * 8 + r) for r in range(8) for c in range(8)]
                cnt = {}
                for i in ids:
                    if i >= 6:
                        cnt[i >> 4] = cnt.get(i >> 4, 0) + 1
                g = max(cnt, key=cnt.get) if cnt else min(groups)
                pal = pals[g]
                px = []
                for i in ids:
                    if i < 6:
                        px.append(i)
                    elif i >> 4 == g:
                        px.append(i & 15)
                    else:
                        r0, g0, b0 = rgb(pals[i >> 4][i & 15])
                        best = min(range(16), key=lambda k: sum((a - b) ** 2 for a, b in zip(rgb(pal[k]), (r0, g0, b0))))
                        px.append(best)
                px = tuple(px)
            key = (px, g)
            if px not in index:
                index[px] = len(tiles)
                tiles.append(px)
            tmap.append(index[px] | (GROUP_PAL[g] << 10))
    return tiles, tmap


def preview(cv, groups):
    rows = []
    for y in range(cv.h):
        row = []
        for x in range(cv.w):
            i = cv.get(x, y)
            c = SKY[i] if i < 6 else groups[i >> 4][i & 15]
            row += rgb(c)
        rows.append(row)
    return rows


# ----------------------------------------------------------------------------- fond des menus
def build_menubg():
    G1, G2, G3 = 16, 32, 48
    cv = Canvas(256, 224)
    rng = random.Random(7)
    hy = 150                                       # horizon
    sky(cv, hy, 0.0, 5.0)
    # nebuleuse : nuages violets / bleus tramees
    for y in range(hy - 10):
        for x in range(256):
            n = noise(x, y, 1.7)
            band = math.exp(-((y - 60 - 25 * math.sin(x * 0.02)) / 38.0) ** 2)
            v = n * 0.9 + band * 1.2 - 0.55
            if v > 0:
                lvl = dith(v * 3.2, x, y)
                if lvl >= 1:
                    col = [G1 + 6, G1 + 9, G1 + 7, G1 + 8][min(3, lvl - 1)] if x > 90 else \
                          [G1 + 6, G1 + 9, G1 + 9, G1 + 7][min(3, lvl - 1)]
                    cv.put(x, y, col)
    # etoiles (couleurs animees 12-14) et quelques etoiles en croix
    for _ in range(170):
        x, y = rng.randrange(256), rng.randrange(hy - 14)
        cv.put(x, y, G1 + rng.choice((12, 13, 14, 14, 15)))
    for _ in range(7):
        x, y = rng.randrange(8, 248), rng.randrange(8, hy - 30)
        c = G1 + rng.choice((12, 13))
        for d in (-2, -1, 1, 2):
            cv.put(x + d, y, c)
            cv.put(x, y + d, c)
        cv.put(x, y, G1 + 15)
    # planete geante (haut droite) : ombrage, atmosphere, anneaux
    cx, cy, r = 206, 62, 40
    ring = lambda x, y: ((x - cx) / 66.0) ** 2 + ((y - cy - 4 + (x - cx) * 0.18) / 12.0) ** 2
    for y in range(cy - r - 3, cy + r + 4):
        for x in range(cx - r - 3, cx + r + 4):
            d = math.hypot(x - cx, y - cy)
            if d <= r:
                lx = ((x - cx) * -0.55 + (y - cy) * -0.65) / r     # lumiere en haut a gauche
                v = 2.2 + lx * 2.6 + math.sin((y - cy) * 0.35 + (x - cx) * 0.05) * 0.45
                v -= max(0, (d / r) - 0.85) * 4
                k = max(0, min(4, dith(v, x, y)))
                cv.put(x, y, G2 + 6 + k)
            elif d <= r + 2.5:
                if (x - cx) + (y - cy) < r * 0.6:
                    cv.put(x, y, G2 + 11)                            # halo d'atmosphere
    for y in range(cy - 30, cy + 40):
        for x in range(cx - 80, 256):
            e = ring(x, y)
            if 0.72 < e < 1.0:
                behind = math.hypot(x - cx, y - cy) <= r and (y - cy - 4 + (x - cx) * 0.18) < 0
                if behind:
                    continue
                k = 12 if e < 0.8 else 14 if e < 0.9 else 13
                if 0.86 < e < 0.88:
                    continue                                           # division des anneaux
                cv.put(x, y, G2 + k)
    # lune (haut gauche)
    mx, my, mr = 38, 34, 11
    for y in range(my - mr, my + mr + 1):
        for x in range(mx - mr, mx + mr + 1):
            d = math.hypot(x - mx, y - my)
            if d <= mr:
                v = 2.0 + ((x - mx) * 0.7 + (y - my) * -0.5) / mr * 1.8
                cv.put(x, y, G2 + [15, 7, 8, 9, 10][max(0, min(4, dith(v, x, y)))])
    for (ox, oy, rr) in ((-4, -2, 2.5), (3, 4, 2), (5, -4, 1.5)):
        for y in range(-3, 4):
            for x in range(-3, 4):
                if math.hypot(x, y) <= rr:
                    cv.put(mx + ox + x, my + oy + y, G2 + 6)
    # ville futuriste a l'horizon
    x = 0
    while x < 256:
        w = rng.randint(6, 16)
        h = rng.randint(8, 34)
        if 96 < x < 160:
            h = rng.randint(4, 10)
        top = hy - h
        cv.rect(x, top, x + w - 1, hy - 1, G3 + 7)
        cv.rect(x, top, x, hy - 1, G3 + 8)
        if rng.random() < 0.4:
            cv.rect(x + w // 2, top - rng.randint(3, 9), x + w // 2, top - 1, G3 + 8)
            cv.put(x + w // 2, top - 10, G3 + 10)
        for wy in range(top + 2, hy - 2, 3):
            for wx in range(x + 2, x + w - 1, 2):
                if rng.random() < 0.28:
                    cv.put(wx, wy, G3 + (9 if rng.random() < 0.7 else 10))
        x += w + rng.randint(0, 2)
    # arene (dome) au centre
    ax, aw, ah = 128, 46, 26
    for y in range(hy - ah, hy):
        for x in range(ax - aw, ax + aw + 1):
            e = ((x - ax) / aw) ** 2 + ((y - hy) / ah) ** 2
            if e <= 1.0:
                cv.put(x, y, G3 + (8 if e > 0.86 else 7))
                if 0.9 < e <= 1.0:
                    cv.put(x, y, G3 + 14)
    for k in range(-4, 5):
        x0 = ax + k * 9
        for y in range(hy - 12, hy):
            if ((x0 - ax) / aw) ** 2 + ((y - hy) / ah) ** 2 <= 0.85:
                cv.put(x0, y, G3 + 12)
    cv.hline(ax - aw + 6, ax + aw - 6, hy - 8, G3 + 11)
    cv.hline(ax - 20, ax + 20, hy - ah - 1, G3 + 15)
    # lueur d'horizon puis sol reflechissant avec grille neon
    cv.hline(0, 255, hy, G3 + 13)
    cv.hline(0, 255, hy + 1, G3 + 11)
    for y in range(hy + 2, 224):
        t = (y - hy) / (224 - hy)
        for x in range(256):
            v = 4.6 - t * 4.2
            c = max(0, min(5, dith(v, x, y)))
            # reflet de l'arene
            if abs(x - ax) < aw - 6 and y < hy + 22 and (y + x) % 3 == 0:
                c = G3 + 8
            cv.put(x, y, c)
    for k in range(1, 12):
        y = hy + int(k * k * 0.62) + 2
        if y < 224:
            cv.hline(0, 255, y, G3 + (12 if k > 3 else 6 + 0))
    for k in range(-14, 15):
        x0 = 128 + k * 10
        x1 = 128 + k * 52
        for y in range(hy + 2, 224):
            t = (y - hy) / (224 - hy)
            cv.put(round(x0 + (x1 - x0) * t), y, G3 + (13 if k == 0 else 12))
    return cv


# ----------------------------------------------------------------------------- publicites
FONT = None  # injecte par gfx.py (police 5x7)

AD_GROUPS = {
    # 1 : fond (ciel commun) + neons cyan
    1: SKY + [c5(2, 6, 14), c5(4, 12, 22), c5(8, 22, 31), c5(18, 28, 31), c5(31, 31, 31),
              c5(8, 22, 31), c5(31, 18, 6), c5(14, 18, 31), c5(26, 26, 31), c5(10, 12, 24)],
    # 2 : rouge / orange (NOVA COLA)
    2: SKY + [c5(10, 1, 3), c5(18, 2, 4), c5(28, 5, 5), c5(31, 14, 6), c5(31, 24, 10),
              c5(31, 31, 24), c5(31, 31, 31), c5(6, 1, 3), c5(20, 10, 4), c5(31, 8, 14)],
    # 3 : vert / jaune (ZENTEK)
    3: SKY + [c5(1, 8, 4), c5(2, 14, 6), c5(6, 24, 8), c5(16, 31, 10), c5(28, 31, 16),
              c5(31, 31, 31), c5(1, 4, 3), c5(31, 28, 6), c5(10, 18, 6), c5(4, 10, 10)],
    # 4 : violet / magenta (HYPERION)
    4: SKY + [c5(8, 2, 14), c5(14, 4, 22), c5(22, 8, 30), c5(30, 14, 31), c5(31, 24, 31),
              c5(31, 31, 31), c5(4, 1, 8), c5(10, 20, 31), c5(18, 6, 16), c5(31, 31, 12)],
}


def text(cv, s, x, y, scale, col, shadow=None, outline=None, spacing=1, italic=0.0):
    """texte en police 5x7 agrandie ; renvoie la largeur"""
    cx = x
    pts = []
    for ch in s:
        g = FONT.get(ch)
        if g:
            for r, row in enumerate(g):
                for k, b in enumerate(row):
                    if b == "1":
                        pts.append((cx + k * scale, y + r * scale))
        cx += (5 + spacing) * scale
    def stamp(ox, oy, c):
        for (px, py) in pts:
            for dy in range(scale):
                sh = int(italic * (7 * scale - (py - y + dy)))
                for dx in range(scale):
                    cv.put(px + dx + ox + sh, py + dy + oy, c)
    if outline is not None:
        for ox, oy in ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, 1), (-1, 1), (1, -1)):
            stamp(ox, oy, outline)
    if shadow is not None:
        stamp(2, 2, shadow)
    stamp(0, 0, col)
    return cx - x


def text_w(s, scale, spacing=1):
    return len(s) * (5 + spacing) * scale - spacing * scale


def panel(cv, x0, y0, x1, y1, fill, edge, glow, cut=6):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            # coins coupes
            if (x - x0) + (y - y0) < cut or (x1 - x) + (y1 - y) < cut:
                continue
            border = (x in (x0, x1) or y in (y0, y1) or (x - x0) + (y - y0) == cut or (x1 - x) + (y1 - y) == cut)
            cv.put(x, y, edge if border else fill)
    for x in range(x0 + cut, x1 + 1):
        cv.put(x, y0 - 1, glow)
    for x in range(x0, x1 - cut + 1):
        cv.put(x, y1 + 1, glow)


def scan(cv, x0, y0, x1, y1, c, step=3):
    for y in range(y0, y1 + 1, step):
        for x in range(x0, x1 + 1):
            if cv.get(x, y) == c - 1 or cv.get(x, y) in (c - 2,):
                cv.put(x, y, c)


def ad_frame(cv, title):
    """cadre commun : ciel, grille, bandeau 'SPONSORS'"""
    G1 = 16
    sky(cv, 224, 0.0, 4.0)
    for k in range(0, 256, 16):
        for y in range(0, 224):
            if (y // 2) % 2 == 0:
                cv.put(k, y, 1)
    for y in range(0, 224, 16):
        cv.hline(0, 255, y, 2)
    cv.rect(0, 0, 255, 17, G1 + 6)
    cv.hline(0, 255, 18, G1 + 8)
    cv.hline(0, 255, 205, G1 + 8)
    cv.rect(0, 206, 255, 223, G1 + 6)
    w = text_w(title, 2)
    text(cv, title, 128 - w // 2, 3, 2, G1 + 9, shadow=G1 + 7)
    for i in range(0, 256, 8):
        cv.put(i + 2, 214, G1 + 8)
        cv.put(i + 3, 214, G1 + 8)


def ad_nova(cv, x0, y0, x1, y1):
    G = 32
    panel(cv, x0, y0, x1, y1, G + 6, G + 9, G + 10)
    # rayures diagonales
    for y in range(y0 + 2, y1 - 1):
        for x in range(x0 + 2, x1 - 1):
            if cv.get(x, y) == G + 6 and (x + y) % 12 < 2:
                cv.put(x, y, G + 7)
    # canette stylisee
    cx = x0 + 24
    cv.rect(cx - 9, y0 + 10, cx + 9, y1 - 10, G + 8)
    cv.rect(cx - 9, y0 + 10, cx - 6, y1 - 10, G + 9)
    cv.rect(cx + 6, y0 + 10, cx + 9, y1 - 10, G + 13)
    cv.rect(cx - 7, y0 + 7, cx + 7, y0 + 9, G + 15)
    cv.rect(cx - 7, y1 - 9, cx + 7, y1 - 7, G + 15)
    for k in range(5):
        cv.put(cx - 3 + k, y0 + 22 + k, G + 11)
        cv.put(cx + 3 - k, y0 + 22 + k, G + 11)
    w = text(cv, "NOVA", x0 + 46, y0 + 10, 3, G + 11, shadow=G + 13, outline=G + 13, italic=0.15)
    text(cv, "COLA", x0 + 46, y0 + 36, 3, G + 10, shadow=G + 13, outline=G + 13, italic=0.15)
    text(cv, "TASTE THE STARS", x0 + 46, y1 - 14, 1, G + 12)
    # eclat d'etoile a droite
    sx, sy = x1 - 40, (y0 + y1) // 2
    for a in range(0, 360, 2):
        r = math.radians(a)
        L = 26 if a % 90 == 0 else 14 if a % 45 == 0 else 7
        for t in range(L):
            k = 10 if t < L * 0.3 else 9 if t < L * 0.6 else 8
            cv.put(round(sx + math.cos(r) * t), round(sy + math.sin(r) * t * 0.8), G + k)
    for y in range(sy - 4, sy + 5):
        for x in range(sx - 4, sx + 5):
            if math.hypot(x - sx, y - sy) < 4.5:
                cv.put(x, y, G + 12)


def ad_zentek(cv, x0, y0, x1, y1):
    G = 48
    panel(cv, x0, y0, x1, y1, G + 12, G + 9, G + 9)
    # circuit
    rng = random.Random(3)
    for _ in range(14):
        x, y = rng.randrange(x0 + 4, x1 - 4), rng.randrange(y0 + 4, y1 - 4)
        dx = rng.choice((-1, 1))
        for i in range(rng.randint(6, 20)):
            if cv.get(x, y) == G + 12:
                cv.put(x, y, G + 6)
            x += dx
        cv.put(x, y, G + 9)
    # embleme hexagonal
    hx, hy, hr = x0 + 22, (y0 + y1) // 2, 14
    for y in range(hy - hr, hy + hr + 1):
        for x in range(hx - hr, hx + hr + 1):
            dx, dy = abs(x - hx), abs(y - hy)
            if dy <= hr * 0.87 and dx + dy * 0.577 <= hr:
                inner = dy <= hr * 0.87 - 3 and dx + dy * 0.577 <= hr - 3
                cv.put(x, y, G + 8 if inner else G + 9)
    text(cv, "Z", hx - 5, hy - 7, 2, G + 13, shadow=G + 12)
    sc = 3 if x1 - x0 > 160 else 2
    text(cv, "ZENTEK", x0 + 44, y0 + 9, sc, G + 9, shadow=G + 12, outline=G + 12)
    text(cv, "ROBOTICS", x0 + 46, y1 - 16, 1, G + 10)


def ad_hyperion(cv, x0, y0, x1, y1):
    G = 64
    panel(cv, x0, y0, x1, y1, G + 12, G + 9, G + 10)
    # flamme / reacteur
    fx, fy = x0 + 20, (y0 + y1) // 2
    for y in range(y0 + 6, y1 - 5):
        for x in range(x0 + 6, x0 + 38):
            d = math.hypot((x - fx) * 1.4, (y - fy))
            if d < 15:
                k = 10 if d < 4 else 9 if d < 8 else 8 if d < 12 else 7
                cv.put(x, y, G + k)
    text(cv, "HYPERION", x0 + 44, y0 + 8, 2, G + 10, shadow=G + 12, outline=G + 12)
    text(cv, "FUEL", x0 + 44, y0 + 26, 2, G + 15, shadow=G + 12, outline=G + 12)
    text(cv, "MORE THRUST", x0 + 44, y1 - 14, 1, G + 9)


def ad_orbitel(cv, x0, y0, x1, y1):
    G = 16
    panel(cv, x0, y0, x1, y1, G + 6, G + 8, G + 8)
    ox, oy = x0 + 20, (y0 + y1) // 2
    for a in range(0, 360, 3):
        r = math.radians(a)
        cv.put(ox + math.cos(r) * 13, oy + math.sin(r) * 5, G + 9)
    for y in range(oy - 6, oy + 7):
        for x in range(ox - 6, ox + 7):
            if math.hypot(x - ox, y - oy) <= 6:
                cv.put(x, y, G + 8 if x - ox + y - oy > 2 else G + 9)
    text(cv, "ORBITEL", x0 + 38, y0 + 10, 2 if x1 - x0 > 130 else 1, G + 10, shadow=G + 7) if x1 - x0 > 130 else \
        text(cv, "ORBITEL", x0 + 36, y0 + 12, 2, G + 10, shadow=G + 7, spacing=0)
    text(cv, "CALL THE", x0 + 38, y1 - 24, 1, G + 9)
    text(cv, "GALAXY", x0 + 38, y1 - 14, 1, G + 9)


def build_ad(n):
    cv = Canvas(256, 224)
    if n == 0:
        ad_frame(cv, "NEXUS BALL SPONSORS")
        ad_nova(cv, 12, 30, 243, 104)
        ad_zentek(cv, 12, 116, 126, 196)
        ad_orbitel(cv, 132, 116, 243, 196)
    else:
        ad_frame(cv, "NEXUS BALL SPONSORS")
        ad_hyperion(cv, 12, 30, 243, 104)
        ad_zentek(cv, 12, 116, 243, 196)
    return cv


# ----------------------------------------------------------------------------- logo OFFGAME
LOGO_GROUPS = {
    1: SKY + [c5(2, 4, 12), c5(4, 10, 22), c5(6, 18, 30), c5(14, 26, 31), c5(26, 30, 31),
              c5(31, 31, 31), c5(16, 6, 24), c5(24, 10, 28), c5(31, 16, 6), c5(3, 3, 8)],
}


def build_offgame():
    G = 16
    cv = Canvas(256, 224)
    for y in range(224):
        v = 1.6 - abs(y - 100) / 70.0
        for x in range(256):
            cv.put(x, y, max(0, min(3, dith(v + 0.4 * math.exp(-((x - 128) / 90.0) ** 2), x, y))))
    word = "OFFGAME"
    sc = 5
    w = text_w(word, sc)
    x0, y0 = 128 - w // 2, 92
    # embleme : anneau lumineux et triangle "play"
    ex, ey = 128, 50
    for yy in range(ey - 20, ey + 21):
        for xx in range(ex - 20, ex + 21):
            d = math.hypot(xx - ex, yy - ey)
            if 15 <= d <= 18:
                cv.put(xx, yy, G + (10 if d < 16 else 9 if d < 17 else 8))
            elif 18 < d <= 19.5 and (xx + yy) % 2 == 0:
                cv.put(xx, yy, G + 7)
    for yy in range(-8, 9):
        for xx in range(-6, 10):
            if xx >= -6 and abs(yy) <= (10 - xx) * 0.5 and xx <= 9:
                cv.put(ex + xx, ey + yy, G + (14 if xx < 2 else 13))
    # contour sombre puis degrade vertical (blanc -> cyan -> bleu)
    text(cv, word, x0, y0, sc, G + 15, outline=G + 15)
    pts = []
    for yy in range(y0, y0 + 7 * sc):
        for xx in range(x0, x0 + w):
            pass
    tmp = Canvas(256, 224)
    text(tmp, word, x0, y0, sc, 1)
    for yy in range(y0, y0 + 7 * sc):
        t = (yy - y0) / (7 * sc)
        col = G + (10 if t < 0.2 else 9 if t < 0.45 else 8 if t < 0.75 else 7)
        for xx in range(x0, x0 + w + 2):
            if tmp.get(xx, yy):
                cv.put(xx, yy, col)
    # reflet attenue sous le mot
    for yy in range(0, 12):
        sy = y0 + 7 * sc - 1 - yy * 2
        for xx in range(x0, x0 + w + 2):
            if tmp.get(xx, sy) and (xx + yy) % 2 == 0:
                cv.put(xx, y0 + 7 * sc + 6 + yy, G + 6)
    # filets neon
    for xx in range(x0 - 20, x0 + w + 20):
        a = 1 - abs(xx - 128) / (w / 2 + 20)
        if a > 0.15:
            cv.put(xx, y0 - 8, G + (12 if a > 0.5 else 13))
            cv.put(xx, y0 + 7 * sc + 3, G + (14 if a > 0.6 else 12))
    for k, xx in enumerate(range(x0 - 20, x0 + w + 20, 6)):
        pass
    pw = text_w("PRESENTS", 1, spacing=2)
    text(cv, "PRESENTS", 128 - pw // 2, y0 + 7 * sc + 24, 1, G + 9, spacing=2)
    return cv
