# NEXUS BALL — CHANGELOG

## 0.1.0 — prototype jouable (Milestones 0 à 3, début du 4)

- Boot : FastROM HiROM 256 Kio, SRAM 32 Kio, effacement WRAM/VRAM, détection PAL/NTSC.
- NMI court : OAM, tilemap BG3, couleurs du public, scroll. Lecture manettes automatique.
- Stade généré (Mode 1, BG1 64×64) : tribunes, murs, ligne médiane, cercle, zones de gardien,
  ligne des 2 points, anneaux. HUD et textes sur BG3, gros titre.
- Moteur de match 6 vs 6 : ballon 3D fixed-point, murs actifs, anneaux, score 1 / 2 points.
- Actions : passe main (interdite vers l'avant), passe pied lobée, tir visé, charge, esquive, saut.
- Règles : port 4 s, fautes (par derrière, gardien protégé, contact tardif), avantage,
  faute grave avec exclusion 20 s, remise en jeu rapide.
- Gardiens IA, IA d'équipe, fatigue, changement de joueur automatique / L.
- Modes 1P vs CPU, 1P vs 2P, CPU vs CPU ; durée 2 à 5 min par période ; mi-temps, fin de match.
- Mini-radar ON/OFF et durée sauvegardés en SRAM (signature, version, checksum).
- Menu pause, crédits, public animé.
- Outils : générateur graphique, correcteur de checksum, banc de test headless libretro.
