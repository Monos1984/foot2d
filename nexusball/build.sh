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
ca65 --cpu 65816 -D DEBUG=$DEBUG -I include -I src -I . --bin-include-dir . -g \
     -l build/nexusball.lst -o build/main.o src/main.asm
ld65 -C hirom.cfg -o build/nexusball.sfc -m build/nexusball.map \
     --dbgfile build/nexusball.dbg -Ln build/nexusball.lbl build/main.o
cp build/nexusball.sfc build/nexusball_ntsc.sfc
python3 tools/checksum.py build/nexusball.sfc pal
python3 tools/checksum.py build/nexusball_ntsc.sfc ntsc
python3 tools/romcheck.py build/nexusball.sfc pal build/nexusball.map
python3 tools/romcheck.py build/nexusball_ntsc.sfc ntsc > /dev/null
if [ "$DEBUG" != "0" ]; then
    mv build/nexusball.sfc build/nexusball_debug.sfc
    mv build/nexusball_ntsc.sfc build/nexusball_debug_ntsc.sfc
    git checkout -- build/nexusball.sfc build/nexusball_ntsc.sfc 2>/dev/null || true
    echo "ROM de debug : build/nexusball_debug.sfc"
fi
