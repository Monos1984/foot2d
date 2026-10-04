#!/usr/bin/env python3
"""NEXUS BALL - compression LZSS des donnees graphiques.

Format (decompresse par lz_decompress, src/lz.asm) :
  +0 : taille decompressee (mot)
  puis des groupes : 1 octet de drapeaux (bit 0 en premier),
    bit = 1 : un octet litteral
    bit = 0 : reference arriere sur 2 octets :
              o0 = distance & $FF, o1 = (distance >> 8) << 4 | (longueur - 3)
              distance 1..4095, longueur 3..18

usage : lz.py entree sortie   (ou lz.py --all : compresse data/gen/*.raw.* )
"""
import os
import sys

MIN_LEN, MAX_LEN, MAX_DIST = 3, 18, 4095


def compress(data):
    out = bytearray(len(data).to_bytes(2, "little"))
    i, n = 0, len(data)
    # index des positions par triplet pour accelerer la recherche
    table = {}
    while i < n:
        flag_pos = len(out)
        out.append(0)
        flags = 0
        for bit in range(8):
            if i >= n:
                break
            best_len, best_dist = 0, 0
            if i + MIN_LEN <= n:
                key = bytes(data[i:i + MIN_LEN])
                for j in reversed(table.get(key, [])):
                    d = i - j
                    if d > MAX_DIST:
                        break
                    l = 0
                    while l < MAX_LEN and i + l < n and data[j + l] == data[i + l]:
                        l += 1
                    if l > best_len:
                        best_len, best_dist = l, d
                        if l == MAX_LEN:
                            break
            if best_len >= MIN_LEN:
                out.append(best_dist & 0xFF)
                out.append(((best_dist >> 8) << 4) | (best_len - MIN_LEN))
                step = best_len
            else:
                flags |= 1 << bit
                out.append(data[i])
                step = 1
            for k in range(step):
                if i + k + MIN_LEN <= n:
                    table.setdefault(bytes(data[i + k:i + k + MIN_LEN]), []).append(i + k)
            i += step
        out[flag_pos] = flags
    return bytes(out)


def decompress(blob):
    size = blob[0] | blob[1] << 8
    out = bytearray()
    p = 2
    while len(out) < size:
        flags = blob[p]
        p += 1
        for bit in range(8):
            if len(out) >= size:
                break
            if flags >> bit & 1:
                out.append(blob[p])
                p += 1
            else:
                o0, o1 = blob[p], blob[p + 1]
                p += 2
                d = o0 | (o1 >> 4) << 8
                l = (o1 & 15) + MIN_LEN
                for _ in range(l):
                    out.append(out[-d])
    return bytes(out)


def pack(src, dst):
    data = open(src, "rb").read()
    blob = compress(data)
    assert decompress(blob) == data, src
    open(dst, "wb").write(blob)
    return len(data), len(blob)


if __name__ == "__main__":
    a, b = pack(sys.argv[1], sys.argv[2])
    print("%s : %d -> %d octets" % (os.path.basename(sys.argv[2]), a, b))
