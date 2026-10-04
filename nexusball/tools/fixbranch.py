#!/usr/bin/env python3
"""Outil de dev : convertit en branches longues (jxx / brl) les branches signalees hors portee par ca65."""
import re, subprocess, sys
INV = {"beq","bne","bcc","bcs","bmi","bpl","bvc","bvs"}
for it in range(20):
    r = subprocess.run(["ca65","--cpu","65816","-I","include","-I","src","-I",".","--bin-include-dir",".","-o","/dev/null","src/main.asm"],capture_output=True,text=True)
    errs = re.findall(r"^(\S+)\((\d+)\): Error: Range error", r.stderr, re.M)
    if not errs:
        print(r.stderr.strip() or "ok"); break
    done = 0
    for f, ln in errs:
        lines = open(f).read().split("\n")
        i = int(ln) - 1
        m = re.match(r"^(\s*(?:[@\w]+:)?\s*)(b\w\w)(\s+)(\S+)(.*)$", lines[i])
        if not m:
            print("??", f, ln, lines[i]); continue
        op = m.group(2)
        if op == "bra":
            new = "brl"
        elif op in INV:
            new = "j" + op[1:]
        else:
            print("??", lines[i]); continue
        lines[i] = m.group(1) + new + m.group(3) + m.group(4) + m.group(5)
        open(f, "w").write("\n".join(lines))
        done += 1
    print("pass", it, "fixed", done)
