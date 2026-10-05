#!/usr/bin/env python3
"""NEXUS BALL - verification d'une ROM construite (appele par build.sh).

Controle : taille (puissance de 2, conforme a l'en-tete), mapping HiROM + FastROM,
type cartouche (ROM + SRAM + pile), taille SRAM, code region attendu, checksum et
complement, vecteurs natifs / emulation (NMI et RESET dans $8000-$FFFF), et
remplissage des segments de chaque banque d'apres la map de ld65.

usage: romcheck.py rom.sfc ntsc|pal [build/nexusball.map]
Code de sortie non nul en cas d'erreur.
"""
import re
import sys

HDR = 0xFFB0
errors = []


def err(msg):
    errors.append(msg)


def main():
    path, region = sys.argv[1], sys.argv[2]
    rom = open(path, "rb").read()
    n = len(rom)
    if n & (n - 1):
        err("taille non puissance de 2 : %d" % n)
    h = rom[HDR:HDR + 0x50]
    title = h[0x10:0x25].decode("ascii", "replace")
    mapmode, carttype, romsize, sramsize, country = h[0x25], h[0x26], h[0x27], h[0x28], h[0x29]
    if mapmode != 0x31:
        err("mapping $%02X (attendu $31 HiROM + FastROM)" % mapmode)
    if carttype != 0x02:
        err("type de cartouche $%02X (attendu $02 ROM + RAM + pile)" % carttype)
    if (1 << romsize) * 1024 != n:
        err("taille d'en-tete %d Kio != fichier %d Kio" % (1 << romsize, n // 1024))
    if sramsize != 0x05:
        err("SRAM $%02X (attendu $05 = 32 Kio)" % sramsize)
    want = 0x01 if region == "ntsc" else 0x02
    if country != want:
        err("code pays $%02X (attendu $%02X pour %s)" % (country, want, region))
    comp = h[0x2C] | h[0x2D] << 8
    chk = h[0x2E] | h[0x2F] << 8
    if comp ^ chk != 0xFFFF:
        err("checksum $%04X et complement $%04X incoherents" % (chk, comp))
    s = (sum(rom) - sum(h[0x2C:0x30]) + 0xFF * 2) & 0xFFFF
    if s != chk:
        err("checksum $%04X != somme $%04X" % (chk, s))
    vec = lambda a: rom[a] | rom[a + 1] << 8
    for name, a in (("NMI natif", 0xFFEA), ("RESET", 0xFFFC)):
        v = vec(a)
        if v < 0x8000:
            err("vecteur %s = $%04X (hors ROM)" % (name, v))
    # segments (map ld65)
    seginfo = []
    if len(sys.argv) > 3:
        txt = open(sys.argv[3]).read()
        part = txt.split("Segment list:")[1].split("Exports list")[0]
        for m in re.finditer(r"^(\w+)\s+([0-9A-F]{6})\s+([0-9A-F]{6})\s+([0-9A-F]{6})", part, re.M):
            name, start, end, size = m.group(1), int(m.group(2), 16), int(m.group(3), 16), int(m.group(4), 16)
            seginfo.append((name, start, end, size))
        limits = {"ZEROPAGE": 0x100, "BSS": 0x1C00}
        for name, start, end, size in seginfo:
            if name in limits and end >= limits[name]:
                err("segment %s depasse $%04X" % (name, limits[name]))
    print("%s : %s, %d Kio, region %s, checksum $%04X" % (path, title.strip(), n // 1024, region, chk))
    for name, start, end, size in seginfo:
        print("   %-9s $%06X-$%06X  %6d octets" % (name, start, end, size))
    if errors:
        for e in errors:
            print("ERREUR :", e)
        sys.exit(1)
    print("   verification : OK")


if __name__ == "__main__":
    main()
