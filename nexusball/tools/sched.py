#!/usr/bin/env python3
"""NEXUS BALL - calendriers de championnat (methode du cercle), N pair de 4 a 16.

data/gen/sched.inc :
  rr_ptr  : .word adresse de la table pour N = 0, 2, 4, ... 16 (index N/2)
  table N : (N-1) journees x N/2 matchs x 2 octets (domicile, exterieur), indices de participants.
  Pour un nombre impair de participants, on utilise N+1 : l'indice N est l'exempt.
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "data", "gen", "sched.inc")


def rounds(n):
    arr = list(range(n))
    res = []
    for r in range(n - 1):
        rd = []
        for i in range(n // 2):
            a, b = arr[i], arr[n - 1 - i]
            if (i == 0 and r % 2) or (i > 0 and (r + i) % 2):
                a, b = b, a
            rd.append((a, b))
        res.append(rd)
        arr = [arr[0], arr[-1]] + arr[1:-1]
    return res


def main():
    out = ["; genere par tools/sched.py"]
    for n in range(4, 17, 2):
        rs = rounds(n)
        pairs = set()
        for rd in rs:
            for a, b in rd:
                pairs.add(frozenset((a, b)))
        assert len(pairs) == n * (n - 1) // 2
        out.append("rr_%d:" % n)
        for rd in rs:
            out.append("    .byte " + ", ".join("%d,%d" % ab for ab in rd))
    out.append("rr_ptr:")
    out.append("    .word 0, 0, " + ", ".join(".loword(rr_%d)" % n for n in range(4, 17, 2)))
    open(OUT, "w").write("\n".join(out) + "\n")
    print("sched: ok")


if __name__ == "__main__":
    main()
