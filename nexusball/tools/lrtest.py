#!/usr/bin/env python3
"""Banc de test headless : execute la ROM dans un coeur libretro (snes9x / bsnes)
et enregistre des captures d'ecran.

usage: lrtest.py core.so rom.sfc script out_prefix
  script : suite de commandes separees par ';'
     w N          attendre N frames
     p BTN[+BTN] N  maintenir des boutons pendant N frames (B Y SEL STA U D L R A X L1 R1)
     p2 ...       idem manette 2
     s NAME       capture -> out_prefix_NAME.png
     m ADDR LEN   affiche LEN octets de WRAM a ADDR (hex ou symbole)
     k ADDR VAL   ecrit un mot en WRAM
     t N ADDR     N frames, compte les changements de l'octet ADDR
"""
import ctypes as C
import os
import struct
import sys
import zlib

BTN = {"B": 0, "Y": 1, "SEL": 2, "STA": 3, "U": 4, "D": 5, "L": 6, "R": 7, "A": 8, "X": 9, "L1": 10, "R1": 11}

ENV_CB = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VID_CB = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUD_CB = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
AUDB_CB = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL_CB = C.CFUNCTYPE(None)
STATE_CB = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)


class GameInfo(C.Structure):
    _fields_ = [("path", C.c_char_p), ("data", C.c_void_p), ("size", C.c_size_t), ("meta", C.c_char_p)]


class Geometry(C.Structure):
    _fields_ = [("bw", C.c_uint), ("bh", C.c_uint), ("mw", C.c_uint), ("mh", C.c_uint), ("ar", C.c_float)]


class Timing(C.Structure):
    _fields_ = [("fps", C.c_double), ("rate", C.c_double)]


class AV(C.Structure):
    _fields_ = [("geom", Geometry), ("timing", Timing)]


state = {"fmt": 0, "frame": None, "pad": [set(), set()], "audio": bytearray()}
sysdir = C.c_char_p(b"/tmp")


def env(cmd, data):
    if cmd == 10:  # SET_PIXEL_FORMAT
        state["fmt"] = C.cast(data, C.POINTER(C.c_int))[0]
        return True
    if cmd == 9 or cmd == 31:  # SYSTEM_DIRECTORY / SAVE_DIRECTORY
        C.cast(data, C.POINTER(C.c_char_p))[0] = sysdir.value
        return True
    if cmd == 3:  # GET_CAN_DUPE
        C.cast(data, C.POINTER(C.c_bool))[0] = True
        return True
    return False


def video(data, w, h, pitch):
    if data:
        state["frame"] = (C.string_at(data, pitch * h), w, h, pitch)


def inp(port, dev, idx, i):
    if port < 2 and dev == 1:
        return 1 if i in state["pad"][port] else 0
    return 0


def audio_batch(data, frames):
    state["audio"] += C.string_at(data, frames * 4)
    return frames


cbs = [ENV_CB(env), VID_CB(video), AUD_CB(lambda l, r: None), AUDB_CB(audio_batch), POLL_CB(lambda: None), STATE_CB(inp)]


def save_png(path):
    raw, w, h, pitch = state["frame"]
    rows = []
    for y in range(h):
        row = bytearray()
        for x in range(w):
            if state["fmt"] == 1:
                b, g, r, _ = raw[y * pitch + x * 4:y * pitch + x * 4 + 4]
            else:
                v = struct.unpack_from("<H", raw, y * pitch + x * 2)[0]
                if state["fmt"] == 2:
                    r, g, b = (v >> 11) << 3, ((v >> 5) & 63) << 2, (v & 31) << 3
                else:
                    r, g, b = ((v >> 10) & 31) << 3, ((v >> 5) & 31) << 3, (v & 31) << 3
            row += bytes((r, g, b))
        rows.append(b"\x00" + bytes(row))
    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(b"".join(rows))) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


SYMS = {}


def addr_of(tok):
    """adresse WRAM : hexadecimal ou nom de symbole (build/nexusball.lbl), +offset possible"""
    off = 0
    if "+" in tok:
        tok, o = tok.split("+")
        off = int(o, 0)
    if tok in SYMS:
        return SYMS[tok] + off
    return int(tok, 16) + off


def main():
    lbl = os.path.join(os.path.dirname(sys.argv[2]), "nexusball.lbl")
    if os.path.exists(lbl):
        for line in open(lbl):
            p = line.split()
            if len(p) == 3 and p[2].startswith("."):
                SYMS[p[2][1:]] = int(p[1], 16) & 0xFFFF
    core = C.CDLL(sys.argv[1])
    rom = open(sys.argv[2], "rb").read()
    script = sys.argv[3]
    out = sys.argv[4]
    core.retro_set_environment(cbs[0])
    core.retro_set_video_refresh(cbs[1])
    core.retro_set_audio_sample(cbs[2])
    core.retro_set_audio_sample_batch(cbs[3])
    core.retro_set_input_poll(cbs[4])
    core.retro_set_input_state(cbs[5])
    core.retro_init()
    buf = C.create_string_buffer(rom, len(rom))
    gi = GameInfo(sys.argv[2].encode(), C.cast(buf, C.c_void_p), len(rom), None)
    if not core.retro_load_game(C.byref(gi)):
        sys.exit("load_game a echoue")
    av = AV()
    core.retro_get_system_av_info(C.byref(av))
    print("fps %.2f, region %d" % (av.timing.fps, core.retro_get_region()))
    core.retro_get_memory_data.restype = C.c_void_p
    core.retro_get_memory_size.restype = C.c_size_t
    nframe = 0
    for cmd in script.split(";"):
        a = cmd.split()
        if not a:
            continue
        if a[0] == "w":
            for _ in range(int(a[1])):
                core.retro_run(); nframe += 1
        elif a[0] in ("p", "p2"):
            port = 0 if a[0] == "p" else 1
            state["pad"][port] = {BTN[b] for b in a[1].split("+")}
            for _ in range(int(a[2])):
                core.retro_run(); nframe += 1
            state["pad"][port] = set()
        elif a[0] == "s":
            save_png("%s_%s.png" % (out, a[1]))
        elif a[0] == "t":
            # t N ADDR : N frames, compte les entrees dans chaque valeur de l'octet ADDR
            ptr = core.retro_get_memory_data(2)
            addr = addr_of(a[2])
            prev, hist = None, {}
            for _ in range(int(a[1])):
                core.retro_run(); nframe += 1
                v = C.string_at(ptr + addr, 1)[0]
                if v != prev:
                    hist[v] = hist.get(v, 0) + 1
                    prev = v
            print("trace %04X:" % addr, dict(sorted(hist.items())))
        elif a[0] == "a":
            # a NAME : enregistre le son capture -> out_prefix_NAME.wav, affiche le niveau
            import array, wave
            pcm = array.array("h", bytes(state["audio"]))
            rms = (sum(v * v for v in pcm) / max(1, len(pcm))) ** 0.5
            w = wave.open("%s_%s.wav" % (out, a[1]), "wb")
            w.setnchannels(2); w.setsampwidth(2); w.setframerate(int(av.timing.rate) or 32040)
            w.writeframes(bytes(state["audio"])); w.close()
            print("audio %s: %.1f s, rms %.0f, max %d" % (a[1], len(pcm) / 2 / (av.timing.rate or 32040), rms, max(pcm or [0])))
            state["audio"] = bytearray()
        elif a[0] == "k":
            # k ADDR VALEUR : ecrit un mot en WRAM
            ptr = core.retro_get_memory_data(2)
            v = int(a[2], 0)
            C.memmove(ptr + addr_of(a[1]), bytes((v & 255, v >> 8)), 2)
        elif a[0] == "m":
            ptr = core.retro_get_memory_data(2)  # SYSTEM_RAM
            addr, ln = addr_of(a[1]), int(a[2])
            print(a[1], end=' ')
            print("%04X:" % addr, C.string_at(ptr + addr, ln).hex(" "))
    print("frames:", nframe)


if __name__ == "__main__":
    main()
