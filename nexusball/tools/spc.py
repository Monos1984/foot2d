#!/usr/bin/env python3
"""NEXUS BALL - pilote audio SPC700 (assemble ici) + echantillons BRR generes.

Produit :
  data/gen/spc.bin   image chargee a $0200 dans la RAM de l'APU (pilote, tables, echantillons)
  data/gen/spc.inc   constantes (adresse d'execution, numeros des sons)

Protocole (ports CPU -> APU) :
  $2141 = commande, $2142 = parametre, puis $2140 = compteur (change a chaque commande)
  commande 1..$EF : effet sonore (table sfx)
  commande $F0    : arret de la musique, $F1.. : musique n
  commande $FF    : volume de l'ambiance du public = parametre
"""
import math
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEN = os.path.join(ROOT, "data", "gen")
BASE = 0x0200


# ----------------------------------------------------------------- mini assembleur SPC700
class Asm:
    def __init__(self, org):
        self.org = org
        self.b = bytearray()
        self.labels = {}
        self.fix = []          # (pos, label, kind) kind: 'rel' | 'abs' | 'lo' | 'hi'

    @property
    def pc(self):
        return self.org + len(self.b)

    def label(self, name):
        assert name not in self.labels, name
        self.labels[name] = self.pc

    def emit(self, *bs):
        for x in bs:
            if isinstance(x, tuple):     # (kind, label)
                self.fix.append((len(self.b), x[1], x[0]))
                self.b += b"\0\0" if x[0] == "abs" else b"\0"
            else:
                self.b.append(x & 0xFF)

    def resolve(self):
        for pos, lab, kind in self.fix:
            v = self.labels[lab]
            if kind == "rel":
                d = v - (self.org + pos + 1)
                assert -128 <= d <= 127, (lab, d)
                self.b[pos] = d & 0xFF
            elif kind == "abs":
                self.b[pos] = v & 0xFF
                self.b[pos + 1] = v >> 8
            elif kind == "lo":
                self.b[pos] = v & 0xFF
            elif kind == "hi":
                self.b[pos] = v >> 8

    # instructions utilisees
    def mov_a_imm(s, v): s.emit(0xE8, v)
    def mov_x_imm(s, v): s.emit(0xCD, v)
    def mov_y_imm(s, v): s.emit(0x8D, v)
    def mov_a_dp(s, d): s.emit(0xE4, d)
    def mov_dp_a(s, d): s.emit(0xC4, d)
    def mov_dp_imm(s, d, v): s.emit(0x8F, v, d)
    def mov_dp_y(s, d): s.emit(0xCB, d)
    def mov_y_dp(s, d): s.emit(0xEB, d)
    def mov_a_absx(s, lab): s.emit(0xF5, ("abs", lab))
    def mov_a_absy(s, lab): s.emit(0xF6, ("abs", lab))
    def mov_a_dpx(s, d): s.emit(0xF4, d)
    def mov_dpx_a(s, d): s.emit(0xD4, d)
    def mov_a_indy(s, d): s.emit(0xF7, d)
    def mov_x_a(s): s.emit(0x5D)
    def mov_a_x(s): s.emit(0x7D)
    def mov_y_a(s): s.emit(0xFD)
    def mov_a_y(s): s.emit(0xDD)
    def mov_sp_x(s): s.emit(0xBD)
    def cmp_a_imm(s, v): s.emit(0x68, v)
    def cmp_a_dp(s, d): s.emit(0x64, d)
    def cmp_x_imm(s, v): s.emit(0xC8, v)
    def and_imm(s, v): s.emit(0x28, v)
    def or_imm(s, v): s.emit(0x08, v)
    def eor_imm(s, v): s.emit(0x48, v)
    def adc_imm(s, v): s.emit(0x88, v)
    def clc(s): s.emit(0x60)
    def asl_a(s): s.emit(0x1C)
    def inc_x(s): s.emit(0x3D)
    def inc_y(s): s.emit(0xFC)
    def dec_dpx(s, d): s.emit(0x9B, d)
    def push_x(s): s.emit(0x4D)
    def pop_x(s): s.emit(0xCE)
    def push_a(s): s.emit(0x2D)
    def pop_a(s): s.emit(0xAE)
    def call(s, lab): s.emit(0x3F, ("abs", lab))
    def ret(s): s.emit(0x6F)
    def jmp(s, lab): s.emit(0x5F, ("abs", lab))
    def bra(s, lab): s.emit(0x2F, ("rel", lab))
    def beq(s, lab): s.emit(0xF0, ("rel", lab))
    def bne(s, lab): s.emit(0xD0, ("rel", lab))
    def bcs(s, lab): s.emit(0xB0, ("rel", lab))
    def bcc(s, lab): s.emit(0x90, ("rel", lab))


# ----------------------------------------------------------------- BRR
def brr(samples, loop=False):
    while len(samples) % 16:
        samples.append(0)
    out = bytearray()
    nblk = len(samples) // 16
    for b in range(nblk):
        blk = samples[b * 16:(b + 1) * 16]
        peak = max(abs(v) for v in blk)
        shift = 0
        while shift < 12 and peak * 2 > (7 << shift):     # decode = (nibble << shift) >> 1
            shift += 1
        hdr = (shift << 4) | (2 if loop else 0) | (1 if b == nblk - 1 else 0)
        out.append(hdr)
        nib = []
        for v in blk:
            n = int(round(v * 2 / (1 << shift))) if shift else int(round(v * 2))
            nib.append(max(-8, min(7, n)) & 15)
        for i in range(0, 16, 2):
            out.append((nib[i] << 4) | nib[i + 1])
    return bytes(out)


def gen_samples():
    rng = random.Random(7)
    s = {}
    s["square"] = ([9000] * 16 + [-9000] * 16, True)
    s["sine"] = ([int(14000 * math.sin(2 * math.pi * i / 32)) for i in range(32)], True)
    thump = []
    for i in range(1024):
        f = 160 - i * 0.1
        env = math.exp(-i / 220)
        thump.append(int(14000 * env * math.sin(2 * math.pi * f * i / 32000 * 8)))
    s["thump"] = (thump, False)
    click = [int(12000 * math.exp(-i / 30) * (rng.random() * 2 - 1)) for i in range(256)]
    s["click"] = (click, False)
    # buzzer : dent de scie + carre (riche en harmoniques), boucle de 64 echantillons
    # impact metallique (mur de l'arene) : partiels inharmoniques amortis
    clang = []
    for i in range(1600):
        v = 0.0
        for f, tau, a in ((610, 520, 0.5), (1430, 380, 0.35), (2290, 260, 0.25), (3370, 180, 0.15)):
            v += a * math.exp(-i / tau) * math.sin(2 * math.pi * f * i / 32000)
        v += 0.4 * math.exp(-i / 40) * (rng.random() * 2 - 1)
        clang.append(int(15000 * v))
    s["clang"] = (clang, False)
    s["buzz"] = ([int(6000 * (2 * (i / 64) - 1) + (5000 if (i // 8) % 2 else -5000)) for i in range(64)], True)
    return s


SAMPLE_ORDER = ["square", "sine", "thump", "click", "buzz", "clang"]
SQUARE, SINE, THUMP, CLICK, BUZZ, CLANG = range(6)

# effets : nom -> (voix, echantillon, pitch, volume, adsr1, adsr2)
SFX = [
    ("KICK",    6, THUMP, 0x0E00, 0x7F, 0x8F, 0xE0),
    ("PASS",    6, CLICK, 0x1000, 0x68, 0x8F, 0xE0),
    ("BOUNCE",  7, THUMP, 0x1A00, 0x48, 0x8F, 0xE0),
    ("WHISTLE", 5, SINE,  0x299A, 0x2C, 0x8F, 0xD3),
    ("GOAL",    3, SINE,  0x1000, 0x58, 0x89, 0xEB),   # voix 3 = bruit : clameur
    ("TACKLE",  7, THUMP, 0x0B00, 0x68, 0x8F, 0xE0),
    ("SAVE",    6, THUMP, 0x1600, 0x60, 0x8F, 0xE0),
    ("MENU",    7, SINE,  0x2000, 0x28, 0x8F, 0xFA),
    ("OK",      7, SINE,  0x3000, 0x30, 0x8F, 0xF8),
    ("OOH",     3, SINE,  0x1000, 0x30, 0x88, 0xE9),
    ("BUZZER",  5, BUZZ,  0x0700, 0x60, 0x8F, 0xF1),   # fautes
    ("BUZZLONG", 5, BUZZ, 0x0680, 0x70, 0x8F, 0xEB),   # fin de periode : buzzer long
    ("WALL",    7, CLANG, 0x1000, 0x58, 0x8F, 0xE0),   # ballon contre un mur
    ("CATCH",   6, THUMP, 0x2400, 0x40, 0x8F, 0xE0),   # reception
    ("BOO",     3, SINE,  0x1000, 0x38, 0x84, 0xEC),   # sifflets du public (faute)
    ("HORN",    5, BUZZ,  0x0B00, 0x58, 0x8F, 0xEE),   # corne de but
    ("CHEER",   3, SINE,  0x1000, 0x40, 0x8C, 0xED),   # clameur (beau geste)
]

# ----------------------------------------------------------------- musique
NOTE = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}


def note_num(n):
    name, octv = n[:-1], int(n[-1])
    return 1 + (octv - 1) * 12 + NOTE[name]          # 1 = C1


def pitch_of(num, sample_len=32):
    f = 32.703 * 2 ** ((num - 1) / 12)                # C1
    return min(0x3FFF, int(round(f * 4096 * sample_len / 32000)))


E = 14   # croche (ticks a 100 Hz)
LEAD = ("A4 C5 E5 A5 G5 E5 D5 E5 C5 D5 E5 G5 E5 D5 C5 B4 "
        "A4 C5 E5 A5 B5 A5 G5 E5 F5 E5 D5 C5 B4 C5 D5 E5").split()
BASS_ROOTS = ["A2", "C3", "F2", "G2", "A2", "C3", "F2", "G2"]
PAD = ["C4", "E4", "A3", "B3"] * 2


def song_title():
    lead = [(note_num(n), E) for n in LEAD] * 2
    bass = []
    for r in BASS_ROOTS:
        lo = note_num(r)
        for k in range(8):
            bass.append((lo + (12 if k % 2 else 0), E))
    pad = [(note_num(n), E * 8) for n in PAD]
    return [lead, bass, pad]


JINGLE = [("C5", 1), ("E5", 1), ("G5", 1), ("C6", 3)]


def song_jingle():
    lead = [(note_num(n), d * 10) for n, d in JINGLE] + [(0, 10)]
    bass = [(note_num("C3"), 30), (note_num("C2"), 30), (0, 10)]
    pad = [(note_num("G4"), 30), (note_num("E4"), 30), (0, 10)]
    return [lead, bass, pad]


def song_chant():
    """ambiance de stade : grosse caisse, claquements de mains, cor (voix 0, 1, 2)"""
    boom, clap = note_num("G5"), note_num("D6")
    drum = [(boom, 25), (boom, 25), (0, 25), (0, 25)] * 4
    hands = [(0, 50), (clap, 25), (0, 25)] * 4
    horn = [(note_num("E4"), 50), (note_num("G4"), 50), (note_num("A4"), 100), (0, 200)]
    return [drum, hands, horn]


def song_chant2():
    """rythme rapide : boum . boum-boum . / claps sur 2 et 4 / fanfare"""
    boom, clap = note_num("F5"), note_num("D6")
    drum = [(boom, 20), (0, 20), (boom, 10), (boom, 10), (0, 20)] * 8
    hands = [(0, 20), (clap, 20), (0, 20), (clap, 20)] * 8
    mel = ["C5", "C5", "D5", "E5", "G5", "E5", "D5", "C5"]
    horn = [(note_num(n), 40) for n in mel] + [(0, 320)]
    return [drum, hands, horn]


def song_chant3():
    """chant de supporters : grosse caisse reguliere, contretemps, 'o-le o-le'"""
    boom, clap = note_num("A5"), note_num("E6")
    drum = [(boom, 25), (boom, 25), (boom, 25), (0, 25)] * 4
    hands = [(0, 12), (clap, 13), (0, 25)] * 8
    horn = [(note_num("G4"), 50), (note_num("E4"), 50), (note_num("G4"), 25), (note_num("A4"), 25),
            (note_num("G4"), 50), (0, 200)]
    return [drum, hands, horn]


SONGS = [("TITLE", song_title, True, "inst_tab"), ("JINGLE", song_jingle, False, "inst_tab"),
         ("CHANT", song_chant, True, "inst_chant"), ("CHANT2", song_chant2, True, "inst_chant"),
         ("CHANT3", song_chant3, True, "inst_chant")]


def build():
    a = Asm(BASE)
    # direct page
    LAST, TMP, PTR = 0x00, 0x01, 0x02
    CH_LO, CH_HI, CH_T, CH_ACT, CH_SLO, CH_SHI = 0x10, 0x13, 0x16, 0x19, 0x1C, 0x1F

    a.label("start")
    a.mov_x_imm(0xCF); a.mov_sp_x()
    # registres DSP
    a.mov_x_imm(0)
    a.label("init")
    a.mov_a_absx("dsp_init"); a.cmp_a_imm(0xFF); a.beq("init_done")
    a.mov_dp_a(0xF2); a.inc_x(); a.mov_a_absx("dsp_init"); a.mov_dp_a(0xF3); a.inc_x()
    a.bra("init")
    a.label("init_done")
    # musique arretee
    a.mov_a_imm(0); a.mov_dp_a(CH_ACT); a.mov_dp_a(CH_ACT + 1); a.mov_dp_a(CH_ACT + 2)
    # timer 0 : 100 Hz, ports remis a zero
    a.mov_dp_imm(0xFA, 80)
    a.mov_dp_imm(0xF1, 0x31)
    a.mov_dp_imm(0xF1, 0x01)
    a.mov_a_dp(0xF4); a.mov_dp_a(LAST)
    # boucle principale
    a.label("main")
    a.mov_a_dp(0xFD); a.beq("nomus")
    a.call("music_tick")
    a.label("nomus")
    a.mov_a_dp(0xF4); a.cmp_a_dp(LAST); a.beq("main")
    a.mov_dp_a(LAST)
    a.mov_a_dp(0xF5)
    a.cmp_a_imm(0xF0); a.bcs("special")
    a.call("sfx")
    a.bra("main")
    a.label("special")
    a.cmp_a_imm(0xFF); a.bne("muscmd")
    a.mov_a_dp(0xF6)
    a.mov_dp_imm(0xF2, 0x40); a.mov_dp_a(0xF3)
    a.mov_dp_imm(0xF2, 0x41); a.mov_dp_a(0xF3)
    a.bra("main")
    a.label("muscmd")
    a.and_imm(0x0F)
    a.call("music_start")
    a.bra("main")

    # --- effet sonore : A = numero (1..)
    a.label("sfx")
    a.asl_a(); a.asl_a(); a.asl_a(); a.mov_x_a()
    a.mov_a_absx("sfx_tab"); a.mov_dp_a(TMP)
    a.asl_a(); a.asl_a(); a.asl_a(); a.asl_a(); a.mov_y_a()
    a.mov_a_absx("sfx_vol"); a.call("wdsp"); a.inc_y(); a.call("wdsp"); a.inc_y()
    a.mov_a_absx("sfx_pl"); a.call("wdsp"); a.inc_y()
    a.mov_a_absx("sfx_ph"); a.call("wdsp"); a.inc_y()
    a.mov_a_absx("sfx_src"); a.call("wdsp"); a.inc_y()
    a.mov_a_absx("sfx_ad1"); a.call("wdsp"); a.inc_y()
    a.mov_a_absx("sfx_ad2"); a.call("wdsp")
    a.mov_y_dp(TMP)
    a.label("keyon")            # Y = voix
    a.mov_dp_imm(0xF2, 0x5C); a.mov_dp_imm(0xF3, 0)
    a.mov_a_absy("bit_tab")
    a.mov_dp_imm(0xF2, 0x4C); a.mov_dp_a(0xF3)
    a.ret()
    a.label("wdsp")
    a.mov_dp_y(0xF2); a.mov_dp_a(0xF3); a.ret()

    # --- musique : A = 0 arret, n = morceau n
    a.label("music_start")
    a.cmp_a_imm(0); a.bne("ms_go")
    a.mov_dp_a(CH_ACT); a.mov_dp_a(CH_ACT + 1); a.mov_dp_a(CH_ACT + 2)
    a.mov_dp_imm(0xF2, 0x5C); a.mov_dp_imm(0xF3, 0x07)
    a.ret()
    a.label("ms_go")
    # X = (n-1) * 6
    a.clc(); a.adc_imm(0xFF)          # n-1
    a.mov_dp_a(TMP); a.asl_a(); a.clc(); a.adc_imm(0); a.mov_x_a()
    a.mov_a_x(); a.asl_a(); a.clc()
    a.mov_dp_a(PTR)                   # (n-1)*4
    a.mov_a_dp(TMP); a.asl_a(); a.clc()
    a.emit(0x84, PTR)                 # adc a,dp -> (n-1)*6
    a.mov_x_a()
    a.mov_y_imm(0)
    a.label("ms_ch")
    a.mov_a_absx("song_tab"); a.mov_dp_a(PTR)
    a.inc_x()
    a.mov_a_absx("song_tab"); a.mov_dp_a(PTR + 1)
    a.inc_x()
    a.push_x()
    a.mov_a_y(); a.mov_x_a()
    a.mov_a_dp(PTR); a.mov_dpx_a(CH_LO); a.mov_dpx_a(CH_SLO)
    a.mov_a_dp(PTR + 1); a.mov_dpx_a(CH_HI); a.mov_dpx_a(CH_SHI)
    a.mov_a_imm(1); a.mov_dpx_a(CH_T); a.mov_dpx_a(CH_ACT)
    a.pop_x()
    a.inc_y(); a.mov_a_y(); a.cmp_a_imm(3); a.bne("ms_ch")
    # instruments des voix 0..2 : table propre au morceau (TMP = n-1)
    a.mov_a_dp(TMP); a.asl_a(); a.mov_x_a()
    a.mov_a_absx("inst_ptr"); a.mov_dp_a(PTR); a.inc_x()
    a.mov_a_absx("inst_ptr"); a.mov_dp_a(PTR + 1)
    a.mov_y_imm(0)
    a.label("ms_inst")
    a.mov_a_indy(PTR); a.cmp_a_imm(0xFF); a.beq("ms_done")
    a.mov_dp_a(0xF2); a.inc_y(); a.mov_a_indy(PTR); a.mov_dp_a(0xF3); a.inc_y()
    a.bra("ms_inst")
    a.label("ms_done")
    a.ret()

    # --- un tick de musique (100 Hz)
    a.label("music_tick")
    a.mov_x_imm(0)
    a.label("mt_ch")
    a.mov_a_dpx(CH_ACT); a.beq("mt_next")
    a.dec_dpx(CH_T); a.bne("mt_next")
    a.label("mt_fetch")
    a.mov_a_dpx(CH_LO); a.mov_dp_a(PTR)
    a.mov_a_dpx(CH_HI); a.mov_dp_a(PTR + 1)
    a.mov_y_imm(0)
    a.mov_a_indy(PTR)
    a.cmp_a_imm(0xFF); a.bne("mt_notloop")
    a.mov_a_dpx(CH_SLO); a.mov_dpx_a(CH_LO)
    a.mov_a_dpx(CH_SHI); a.mov_dpx_a(CH_HI)
    a.bra("mt_fetch")
    a.label("mt_notloop")
    a.cmp_a_imm(0xFE); a.bne("mt_note")
    a.mov_a_imm(0); a.mov_dpx_a(CH_ACT)
    a.bra("mt_off")
    a.label("mt_note")
    a.cmp_a_imm(0); a.beq("mt_rest")
    # pitch : table[note]
    a.asl_a(); a.mov_y_a()
    a.push_x()
    a.mov_a_x(); a.asl_a(); a.asl_a(); a.asl_a(); a.asl_a(); a.or_imm(2); a.mov_x_a()
    a.mov_a_absy("ptab"); a.mov_dp_a(TMP)
    a.mov_a_x(); a.mov_dp_a(0xF2); a.mov_a_dp(TMP); a.mov_dp_a(0xF3)
    a.mov_a_absy("ptab1")
    a.inc_x(); a.mov_dp_a(TMP)
    a.mov_a_x(); a.mov_dp_a(0xF2); a.mov_a_dp(TMP); a.mov_dp_a(0xF3)
    a.pop_x()
    a.mov_a_x(); a.mov_y_a()
    a.push_x()
    a.call("keyon")
    a.pop_x()
    a.bra("mt_dur")
    a.label("mt_rest")
    a.label("mt_off")
    a.mov_a_absx("bit_tab")
    a.mov_dp_imm(0xF2, 0x5C); a.mov_dp_a(0xF3)
    a.mov_a_dpx(CH_ACT); a.beq("mt_next")
    a.label("mt_dur")
    a.mov_y_imm(1)
    a.mov_a_indy(PTR); a.mov_dpx_a(CH_T)
    a.mov_a_dpx(CH_LO); a.clc(); a.adc_imm(2); a.mov_dpx_a(CH_LO)
    a.mov_a_dpx(CH_HI); a.adc_imm(0); a.mov_dpx_a(CH_HI)
    a.label("mt_next")
    a.inc_x(); a.cmp_x_imm(3); a.beq("mt_ret")
    a.jmp("mt_ch")
    a.label("mt_ret")
    a.ret()

    # ----------------------------------------------------------- tables
    a.label("bit_tab")
    a.emit(1, 2, 4, 8, 16, 32, 64, 128)
    a.label("dsp_init")
    regs = [(0x6C, 0x60), (0x0C, 0x5F), (0x1C, 0x5F), (0x2C, 0), (0x3C, 0), (0x4D, 0), (0x2D, 0),
            (0x3D, 0x18), (0x5D, 0x00), (0x6D, 0x00), (0x7D, 0x00), (0x5C, 0x00),
            # ambiance du public : voix 4 en bruit, GAIN direct
            (0x40, 0x00), (0x41, 0x00), (0x42, 0x00), (0x43, 0x10), (0x44, SINE), (0x45, 0x00), (0x47, 0x7F),
            (0x6C, 0x3A),   # FLG : echo desactive, bruit ~ 1 kHz
            (0x4C, 0x10)]
    for r, v in regs:
        a.emit(r, v)
    a.emit(0xFF)
    # instruments musique (voix 0 : carre, 1 : sinus basse, 2 : nappe)
    a.label("inst_tab")
    for r, v in [(0x00, 0x24), (0x01, 0x24), (0x04, SQUARE), (0x05, 0x8E), (0x06, 0xB4),
                 (0x10, 0x40), (0x11, 0x40), (0x14, SINE), (0x15, 0x8E), (0x16, 0xD0),
                 (0x20, 0x16), (0x21, 0x16), (0x24, SQUARE), (0x25, 0x8A), (0x26, 0xEA)]:
        a.emit(r, v)
    a.emit(0xFF)
    a.label("inst_chant")
    for r, v in [(0x00, 0x34), (0x01, 0x34), (0x04, THUMP), (0x05, 0x8F), (0x06, 0xE0),
                 (0x10, 0x22), (0x11, 0x22), (0x14, CLICK), (0x15, 0x8F), (0x16, 0xE0),
                 (0x20, 0x12), (0x21, 0x12), (0x24, SQUARE), (0x25, 0x8A), (0x26, 0xEA)]:
        a.emit(r, v)
    a.emit(0xFF)
    a.label("inst_ptr")
    for s in SONGS:
        a.emit(("abs", s[3]))
    # effets (8 octets par entree, l'entree 0 est inutilisee)
    a.label("sfx_tab")
    rows = [(0, 0, 0, 0, 0, 0)] + [x[1:] for x in SFX]
    sfx_bytes = bytearray()
    for (v, src, p, vol, a1, a2) in rows:
        sfx_bytes += bytes((v, src, p & 255, p >> 8, vol, a1, a2, 0))
    a.emit(*sfx_bytes)
    for off, nm in enumerate(["sfx_v", "sfx_src", "sfx_pl", "sfx_ph", "sfx_vol", "sfx_ad1", "sfx_ad2"]):
        a.labels[nm] = a.labels["sfx_tab"] + off
    # pitch des notes (num 0..96), sample de 32 points
    a.label("ptab")
    pt = bytearray()
    for n in range(97):
        p = pitch_of(n) if n else 0
        pt += bytes((p & 255, p >> 8))
    a.emit(*pt)
    a.labels["ptab1"] = a.labels["ptab"] + 1
    # morceaux
    songs_streams = []
    for name, fn, loop, _inst in SONGS:
        chans = fn()
        ptrs = []
        for ch in chans:
            lab = "s_%s_%d" % (name, len(ptrs))
            ptrs.append(lab)
            songs_streams.append((lab, ch, loop))
        a.labels  # noqa
        songs_streams.append(None)
    a.label("song_tab")
    for name, fn, loop, _inst in SONGS:
        for c in range(3):
            a.emit(("abs", "s_%s_%d" % (name, c)))
    for item in songs_streams:
        if item is None:
            continue
        lab, ch, loop = item
        a.label(lab)
        for n, d in ch:
            a.emit(n, max(1, min(255, d)))
        a.emit(0xFF if loop else 0xFE, 1)
    # repertoire des echantillons (aligne sur 256)
    while a.pc & 0xFF:
        a.emit(0)
    dir_addr = a.pc
    a.label("dir")
    samples = gen_samples()
    blobs = [(nm, brr(list(samples[nm][0]), samples[nm][1])) for nm in SAMPLE_ORDER]
    addr = dir_addr + 4 * len(blobs)
    entries = []
    for nm, bl in blobs:
        entries.append(addr)
        addr += len(bl)
    for e in entries:
        a.emit(e & 255, e >> 8, e & 255, e >> 8)
    for nm, bl in blobs:
        a.emit(*bl)
    a.resolve()
    # DIR dans la table d'init
    di = a.labels["dsp_init"] - BASE
    for i in range(di, di + 2 * len(regs), 2):
        if a.b[i] == 0x5D:
            a.b[i + 1] = dir_addr >> 8
    return a


def main():
    a = build()
    os.makedirs(GEN, exist_ok=True)
    open(os.path.join(GEN, "spc.bin"), "wb").write(a.b)
    with open(os.path.join(GEN, "spc.inc"), "w") as f:
        f.write("; genere par tools/spc.py\nSPC_ORG = $%04X\nSPC_SIZE = %d\n" % (BASE, len(a.b)))
        for i, s in enumerate(SFX):
            f.write("SFX_%s = %d\n" % (s[0], i + 1))
        for i, s in enumerate(SONGS):
            f.write("MUS_%s = $%02X\n" % (s[0], 0xF1 + i))
        f.write("MUS_STOP = $F0\nSND_CROWD = $FF\n")
    print("spc: %d octets ($%04X-$%04X)" % (len(a.b), BASE, BASE + len(a.b) - 1))


if __name__ == "__main__":
    main()
