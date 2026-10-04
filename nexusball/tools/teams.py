#!/usr/bin/env python3
"""NEXUS BALL - generateur des donnees d'equipes (16 equipes officielles).

Ecrit data/gen/teams.inc (inclus par src/teams.asm).

Enregistrement d'equipe (TEAM_REC = 260 octets) :
  +0   nom (16 octets, termine par 0)
  +16  nom court (4 octets, termine par 0)
  +20  monde d'origine (16 octets, termine par 0)
  +36  couleurs maillot domicile : principal, sombre, liseré, short, reflet (5 mots BGR555)
  +46  couleurs maillot exterieur (5 mots)
  +56  famille de couleur domicile, exterieur (2 octets)
  +58  formation par defaut, MENTALITY, PASSING, PRESSURE, DEF LINE, ATTACK, TEMPO (7 octets)
  +65  style (1 octet), niveau global (1 octet), reserve (1)
  +68  12 joueurs x 16 octets : nom (8, complete par des espaces), poste, puis
       SPEED POWER PASS KICK CONTROL DEFENSE STAMINA (gardien : REFLEX POWER POSITION
       THROW CATCH POSITION STAMINA)
Effectif : 2 GK, 4 DF, 3 MF, 3 FW (les meilleurs en premier).
"""
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "data", "gen", "teams.inc")


def c5(r, g, b):
    return (r & 31) | ((g & 31) << 5) | ((b & 31) << 10)


def shade(rgb, k):
    return tuple(max(0, min(31, int(v * k))) for v in rgb)


# familles de couleur (pour eviter deux maillots semblables)
BLUE, ORANGE, GREEN, YELLOW, PURPLE, WHITE, CYAN, RED, BLACK = range(9)

STYLES = ["SPEED", "POWER", "PASSING", "DEFENSIVE", "OFFENSIVE", "COUNTER", "BALANCED", "TECHNICAL"]
FORMS = ["2-2-1", "2-1-2", "1-3-1", "1-2-2", "3-1-1", "3-2-0"]

# nom, court, monde, couleur (r,g,b), famille, maillot exterieur, famille ext, style, niveau, formation, tactique
# tactique : MENTALITY PASSING PRESSURE DEFLINE ATTACK TEMPO (0..2)
TEAMS = [
    ("ORION STARS",    "ORI", "ORION BELT",   (31, 13, 2),  ORANGE, (28, 28, 30), WHITE,  "POWER",     7, 0, (1, 1, 1, 1, 0, 1)),
    ("TITAN CRUSHERS", "TIT", "TITAN",        (14, 4, 4),   RED,    (24, 24, 26), WHITE,  "POWER",     7, 4, (0, 2, 2, 1, 0, 0)),
    ("VEGA STRIKERS",  "VEG", "VEGA PRIME",   (28, 4, 22),  PURPLE, (30, 28, 6),  YELLOW, "OFFENSIVE", 6, 3, (2, 1, 2, 2, 2, 2)),
    ("ANDROMEDA WOLVES", "AND", "ANDROMEDA",  (12, 12, 16), BLACK,  (8, 22, 30),  CYAN,   "COUNTER",   6, 4, (0, 2, 1, 0, 1, 2)),
    ("MARS UNITED",    "MAR", "MARS",         (26, 6, 3),   RED,    (28, 28, 30), WHITE,  "BALANCED",  6, 0, (1, 1, 1, 1, 2, 1)),
    ("SOLARIS FALCONS", "SOL", "SOLARIS",     (31, 26, 4),  YELLOW, (6, 8, 24),   BLUE,   "SPEED",     6, 1, (2, 0, 1, 1, 1, 2)),
    ("EUROPA ICE",     "EUR", "EUROPA",       (4, 12, 30),  BLUE,   (28, 30, 31), WHITE,  "TECHNICAL", 7, 2, (1, 0, 1, 1, 0, 1)),
    ("SIRIUS RAIDERS", "SIR", "SIRIUS",       (4, 26, 26),  CYAN,   (20, 4, 4),   RED,    "SPEED",     5, 1, (2, 1, 2, 1, 1, 2)),
    ("NOVA PRIME",     "NOV", "NOVA",         (30, 30, 31), WHITE,  (4, 6, 16),   BLUE,   "PASSING",   7, 2, (1, 0, 1, 1, 2, 0)),
    ("CENTAURI FORCE", "CEN", "ALPHA CENTAURI", (6, 22, 8), GREEN,  (28, 28, 30), WHITE,  "DEFENSIVE", 5, 5, (0, 1, 0, 0, 0, 0)),
    ("LUNAR KNIGHTS",  "LUN", "THE MOON",     (22, 22, 26), WHITE,  (16, 4, 22),  PURPLE, "DEFENSIVE", 5, 4, (0, 1, 1, 0, 1, 1)),
    ("ALPHA DRAKES",   "ALP", "ALPHA DRACO",  (4, 18, 6),   GREEN,  (30, 26, 4),  YELLOW, "OFFENSIVE", 5, 1, (2, 2, 2, 2, 1, 2)),
    ("NEPTUNE STORM",  "NEP", "NEPTUNE",      (2, 6, 20),   BLUE,   (30, 18, 2),  ORANGE, "BALANCED",  5, 0, (1, 1, 1, 1, 1, 1)),
    ("PHOENIX CORE",   "PHO", "PHOENIX",      (31, 6, 6),   RED,    (30, 26, 6),  YELLOW, "COUNTER",   6, 3, (0, 2, 1, 1, 1, 2)),
    ("HELIOS BLADES",  "HEL", "HELIOS",       (31, 20, 2),  ORANGE, (6, 6, 10),   BLACK,  "TECHNICAL", 6, 2, (1, 0, 1, 2, 0, 1)),
    ("CYGNUS RANGERS", "CYG", "CYGNUS",       (18, 8, 28),  PURPLE, (6, 26, 26),  CYAN,   "PASSING",   4, 0, (1, 0, 0, 1, 2, 0)),
]

KITCOLS = [
    ("BLUE", (4, 12, 30), BLUE), ("NAVY", (2, 5, 16), BLUE), ("SKY", (10, 22, 31), CYAN),
    ("CYAN", (4, 26, 26), CYAN), ("GREEN", (6, 22, 8), GREEN), ("LIME", (18, 30, 6), GREEN),
    ("YELLOW", (31, 26, 4), YELLOW), ("ORANGE", (31, 14, 2), ORANGE), ("RED", (28, 4, 4), RED),
    ("PINK", (31, 12, 22), PURPLE), ("PURPLE", (18, 6, 28), PURPLE), ("WHITE", (29, 29, 31), WHITE),
    ("GREY", (16, 16, 18), WHITE), ("BLACK", (6, 6, 8), BLACK),
]

# profils de stats par style : ajustements SPEED POWER PASS KICK CONTROL DEFENSE STAMINA
STYLE_ADJ = {
    "SPEED":     (2, -1, 0, 0, 0, -1, 0),
    "POWER":     (-1, 2, -1, 1, 0, 1, 0),
    "PASSING":   (0, -1, 2, 0, 1, 0, 0),
    "DEFENSIVE": (0, 1, 0, -1, 0, 2, 0),
    "OFFENSIVE": (1, 0, 0, 2, 1, -2, 0),
    "COUNTER":   (2, 0, 0, 1, 0, 0, -1),
    "BALANCED":  (0, 0, 0, 0, 0, 0, 1),
    "TECHNICAL": (0, -1, 1, 0, 2, 0, 0),
}
# profil par poste : SPEED POWER PASS KICK CONTROL DEFENSE STAMINA (base, avant niveau)
ROLE_BASE = {
    1: (0, 1, -1, -1, -1, 2, 1),   # DF
    2: (0, -1, 2, 0, 1, 0, 1),     # MF
    3: (1, 0, 0, 2, 1, -2, 0),     # FW
}
SYL = ["KA", "RO", "VEN", "TA", "LI", "MOR", "ZE", "NA", "DRA", "KO", "SIL", "VA", "TOR", "EN", "AX", "RIS",
       "BEL", "DU", "OR", "KAI", "LEX", "MI", "SO", "TEK", "VOR", "YAN", "ZU", "NIX", "QUA", "REN", "JO", "PAX"]


def clamp(v):
    return max(1, min(9, v))


def name(rng, used):
    while True:
        n = "".join(rng.choice(SYL) for _ in range(rng.choice((2, 2, 3))))
        if len(n) <= 8 and n not in used:
            used.add(n)
            return n


def strfield(s, n):
    b = s.encode()[:n - 1]
    return b + b"\0" * (n - len(b))


def kit(rgb):
    return [c5(*rgb), c5(*shade(rgb, 0.6)), c5(28, 30, 31) if sum(rgb) < 60 else c5(4, 4, 8),
            c5(*shade(rgb, 0.35)), c5(*[min(31, v + 8) for v in rgb])]


def main():
    rng = random.Random(1984)
    used = set()
    out = ["; genere par tools/teams.py - ne pas modifier", "NUM_TEAMS = %d" % len(TEAMS), "TEAM_REC = 260", "",
           "; enregistrements en banque $C0 (copies a la demande dans trec_buf par team_rec_id)",
           '.segment "DATA0"']
    for ti, (nm, sh, world, col, fam, away, afam, style, level, form, tac) in enumerate(TEAMS):
        rec = bytearray()
        rec += strfield(nm, 16) + strfield(sh, 4) + strfield(world, 16)
        for c in kit(col) + kit(away):
            rec += c.to_bytes(2, "little")
        rec += bytes((fam, afam, form) + tac + (STYLES.index(style), level, 0))
        assert len(rec) == 68, len(rec)
        adj = STYLE_ADJ[style]
        roster = [0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3]
        for k, role in enumerate(roster):
            first = k in (0, 2, 3, 6, 9)
            base = level - 2 + (1 if first else 0) - (1 if k in (5, 11) else 0)
            if role == 0:
                st = [clamp(base + 1 + rng.randint(-1, 1)) for _ in range(7)]
                st[1] = clamp(base + adj[1])
            else:
                rb = ROLE_BASE[role]
                st = [clamp(base + rb[i] + adj[i] + rng.randint(-1, 1)) for i in range(7)]
            rec += (name(rng, used).ljust(8)).encode() + bytes([role] + st)
        assert len(rec) == 260
        out.append("team_%d:  ; %s" % (ti, nm))
        for i in range(0, len(rec), 20):
            out.append("    .byte " + ",".join("$%02X" % b for b in rec[i:i + 20]))
    out.append('.segment "RODATA"')
    out.append("team_ptr:")
    out.append("    .word " + ", ".join(".loword(team_%d)" % i for i in range(len(TEAMS))))
    # couleurs proposees par l'editeur d'equipe : nom, kit (5 mots), famille
    out.append("NUM_KITCOLS = %d" % len(KITCOLS))
    out.append("kitcol_names:")
    for nm, rgb, fam in KITCOLS:
        out.append('    .byte "%-8s", 0' % nm)
    out.append("kitcol_kit:")
    for nm, rgb, fam in KITCOLS:
        out.append("    .word " + ", ".join("$%04X" % c for c in kit(rgb)))
    out.append("kitcol_main:")
    out.append("    .word " + ", ".join("$%04X" % c5(*rgb) for nm, rgb, fam in KITCOLS))
    out.append("kitcol_fam:")
    out.append("    .byte " + ", ".join(str(fam) for nm, rgb, fam in KITCOLS))
    out.append("style_names:")
    for s in STYLES:
        out.append('    .byte "%-9s", 0' % s)
    out.append("form_names:")
    for f in FORMS:
        out.append('    .byte "%s", 0' % f)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, "w").write("\n".join(out) + "\n")
    print("teams: %d equipes" % len(TEAMS))


if __name__ == "__main__":
    main()
