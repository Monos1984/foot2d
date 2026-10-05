#!/bin/sh
# NEXUS BALL - build reproductible
#   produit build/nexusball.sfc (en-tete Europe/PAL) et build/nexusball_ntsc.sfc
#   le jeu detecte lui-meme la region de la console (50/60 Hz)
#   DEBUG=1 ./build.sh : version de developpement (affichage de controle en match)
set -e
DEBUG=${DEBUG:-0}
cd "$(dirname "$0")"
mkdir -p build data/gen
python3 tools/gfx.py
for f in data/gen/stad?.chr data/gen/stad?.map data/gen/obj.chr data/gen/font.chr \
         data/gen/logo.chr data/gen/logo.map data/gen/menubg.chr data/gen/menubg.map data/gen/crowd.chr data/gen/crowd.map data/gen/ad0.chr data/gen/ad0.map data/gen/ad1.chr data/gen/ad1.map data/gen/ad2.chr data/gen/ad2.map; do
    python3 tools/lz.py "$f" "$f.lz" > /dev/null
done
python3 tools/teams.py
python3 tools/spc.py
python3 tools/sched.py
python3 tools/lang.py
OUT=nexusball
[ "$DEBUG" != "0" ] && OUT=nexusball_debug
ca65 --cpu 65816 -D DEBUG=$DEBUG -I include -I src -I . --bin-include-dir . -g \
     -l build/$OUT.lst -o build/$OUT.o src/main.asm
ld65 -C hirom.cfg -o build/$OUT.sfc -m build/$OUT.map \
     --dbgfile build/$OUT.dbg -Ln build/$OUT.lbl build/$OUT.o
cp build/$OUT.sfc build/${OUT}_ntsc.sfc
cp build/$OUT.lbl build/${OUT}_ntsc.lbl
python3 tools/checksum.py build/$OUT.sfc pal
python3 tools/checksum.py build/${OUT}_ntsc.sfc ntsc
python3 tools/romcheck.py build/$OUT.sfc pal build/$OUT.map
python3 tools/romcheck.py build/${OUT}_ntsc.sfc ntsc > /dev/null
