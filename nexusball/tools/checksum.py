#!/usr/bin/env python3
"""Corrige le checksum de l'en-tete HiROM ($FFDC-$FFDF) et fixe le code pays.

usage: checksum.py rom.sfc [ntsc|pal]
"""
import sys

HDR = 0xFFB0


def main():
    path = sys.argv[1]
    region = sys.argv[2] if len(sys.argv) > 2 else None
    rom = bytearray(open(path, "rb").read())
    if len(rom) & (len(rom) - 1):
        sys.exit("taille de ROM non puissance de 2 : %d" % len(rom))
    if region:
        rom[HDR + 0x29] = 0x01 if region == "ntsc" else 0x02
    rom[HDR + 0x2C:HDR + 0x30] = b"\xFF\xFF\x00\x00"
    s = sum(rom) & 0xFFFF
    rom[HDR + 0x2C] = (s ^ 0xFFFF) & 0xFF
    rom[HDR + 0x2D] = (s ^ 0xFFFF) >> 8
    rom[HDR + 0x2E] = s & 0xFF
    rom[HDR + 0x2F] = s >> 8
    open(path, "wb").write(rom)
    print("%s : %d Kio, checksum %04X (%s)" % (path, len(rom) // 1024, s, region or "inchange"))


if __name__ == "__main__":
    main()
