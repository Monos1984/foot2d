# NEXUS BALL — sources des graphismes

Ce dossier accueillera les **sources** des graphismes définitifs. Les fichiers de
`data/gen/` sont **produits automatiquement** par `build.sh` et ne doivent jamais être
retouchés à la main : toute modification se fait ici ou dans les scripts de conversion.

```
assets sources (PNG indexés)
     ↓  tools/ (conversion : découpe en tiles, palettes, dédoublonnage, compression LZSS)
data/gen/
     ↓  ca65 / ld65
ROM
```

## Arborescence

| Dossier | Contenu | Contraintes SNES |
|---|---|---|
| `players/` | planches des joueurs de champ (cadres 16×32) | 4bpp, 15 couleurs + transparence ; couleurs du maillot aux index 5-8 et 10 (remplacées en jeu par celles de l'équipe) |
| `goalkeepers/` | planches des gardiens (mêmes cadres) | palette OBJ dédiée (2 / 3) |
| `ball/` | ballon (4 orientations × 2 phases, 16×16) et ombre | palette OBJ 4 partagée avec curseurs et radar |
| `stadiums/` | un PNG 512×320 par stade + sa palette | BG1 4bpp, ≤ 1024 tiles uniques, palette BG 2 ; index 10-14 = couleurs animées (anneaux, public) |
| `ui/` | fonds des menus, écrans de publicité, écran titre | BG1 4bpp multi-palettes (BG 2, 5, 6, 7), 256×224 |
| `hud/` | plaques d'équipe, chiffres du score, bandeaux | BG3 2bpp, 4 couleurs, tiles 128-255 de la police |
| `radar/` | cadre et points du mini-radar | OBJ, priorité la plus basse de l'OAM |
| `logos/` | logo NEXUS BALL (BG2) et logo OFFGAME | BG2 4bpp palette 3 / BG1 |
| `portraits/` | (prévu) portraits de joueurs pour les éditeurs | |
| `effects/` | (prévu) étincelles, halo des anneaux, traînée du ballon | OBJ 8×8 / 16×16 |
| `fonts/` | police 5×7 (codes 32-95, accents sur @ [ ^ ] \ #) | BG3 2bpp |

## Règles à respecter

- Garder le gabarit actuel des joueurs (cadres 16×32 = 2 OBJ 16×16) : il a été validé pour
  les limites OAM (32 OBJ / 34 tranches de 8 px par ligne). Améliorer le dessin et les
  animations plutôt que la taille.
- Ne pas changer les dimensions du terrain d'un stade à l'autre (géométrie officielle commune).
- Chaque nouveau convertisseur doit être appelé par `build.sh` et vérifié par
  `tools/romcheck.py` (place restante par banque affichée à chaque build).

Les graphismes actuels sont générés par `tools/gfx.py` et `tools/scenes.py` ; quand un
fichier source est déposé ici, le convertisseur correspondant le remplacera.
