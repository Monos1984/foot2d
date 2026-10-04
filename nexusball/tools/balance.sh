#!/bin/sh
# Equilibrage : matchs CPU contre CPU (2 x 2 min) entre equipes de niveaux differents.
# usage : S9=/chemin/snes9x_libretro.so sh tools/balance.sh
S9=${S9:-/tmp/lr/usr/lib/x86_64-linux-gnu/libretro/snes9x_libretro.so}
for pair in "6 0" "1 5" "2 9" "3 8" "15 0" "4 12"; do
 set -- $pair
 timeout 300 python3 tools/lrtest.py $S9 build/nexusball_ntsc.sfc "w 100; p STA 2; w 150; p STA 2; w 300; p A 2; w 100; p A 2; w 150; p L 2; w 4; k team_id $1; k team_id+2 $2; k menu_len 0; p STA 2; w 17000; m score 4; m st_shots 4; m dbg_ringmiss 2" /tmp/b 2>&1 | grep -E "score|st_shots|dbg" | tr '\n' ' '; echo " [$1 vs $2]"
done
