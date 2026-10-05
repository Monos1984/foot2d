# NEXUS BALL — notes techniques et règles arrêtées

Version 0.12.0. Ce document suit l'audit « Correctifs et améliorations à appliquer » :
il décrit l'état du moteur, les décisions prises et ce qui reste à valider sur matériel réel.

## 1. Règles arrêtées

| Sujet | Décision |
|---|---|
| Score | 1 point (lancer / tir près), 2 points (frappe au pied depuis la ligne longue). Un but contre son camp vaut toujours 1 point. |
| Moitié de terrain | Aucun point marqué si le tireur était dans sa propre moitié au lâcher (sauf contre son camp et tirs au but). |
| Passage dans l'anneau | Testé au **point exact de franchissement** du plan de l'anneau : interpolation linéaire de Y et Z entre la frame précédente (`b_prevx/y/z`) et la frame courante (fraction sur 7 bits). Après un but, l'état passe à `MS_GOAL` et le ballon est placé derrière l'anneau : un passage ne peut pas compter deux fois. |
| Port du ballon | 4 s au plus. Au-delà : buzzer, bandeau, arrêt court (`MS_FOUL`), ballon à l'adversaire le plus proche et **coup franc** (même séquence qu'une faute, `fk_start`). |
| Coup franc | Le tireur ne bouge pas (passe / tir autorisés), les adversaires sont tenus à 40 px, 6 s au plus. |
| Raquettes | Réservées aux gardiens (les joueurs de champ sont repoussés sur le bord). Le ballon ne peut pas y rester 4 s : sinon l'adversaire engage au centre. |
| Coup d'envoi | Deux joueurs dans le rond central ; le porteur doit passer à son partenaire ; personne d'autre n'entre dans le rond avant la réception (6 s au plus). |
| Exclusions | Faute grave = 20 s de jeu, **un minuteur par joueur** (`p_pen`) : plusieurs exclusions simultanées possibles. Retour au poste le long du mur du haut, ou du bas si le ballon est près du haut. |
| Fin de période à 0:00 | **Arrêt immédiat** (option A, arcade) : buzzer long, joueurs figés. Un tir déjà parti ne compte pas (la détection du but n'est active qu'en jeu, coup franc et tirs au but). |
| Pause | Le jeu est entièrement figé (chronomètre, physique, IA, fatigue, exclusions). À la reprise, les boutons pressés pendant la frame de sortie sont ignorés (pas d'action fantôme). |

## 2. Sprites (OAM)

Ordre d'écriture = ordre d'affichage **et** priorité de conservation quand une ligne dépasse
la limite matérielle (32 OBJ et 34 tranches de 8 px par ligne : la PPU abandonne les derniers) :

1. curseurs P1 / P2 (2 OBJ 8×8)
2. ballon (1 OBJ 16×16)
3. joueurs, du plus proche au plus lointain (12 × 2 OBJ 16×16)
4. ombre du ballon (1 OBJ 16×16)
5. radar : 13 points 8×8 + 8 OBJ 16×16 de cadre — **en dernier**

Total maximal : 49 OBJ sur 128. Pire cas sur une ligne : 12 joueurs alignés = 24 OBJ / 48
tranches → les joueurs les plus lointains et le radar perdent des morceaux en premier ; le
ballon et les curseurs ne disparaissent jamais. Le radar passe désormais visuellement derrière
les joueurs qui le croisent (choix assumé : le jeu avant la décoration).
La version `DEBUG=1` affiche le nombre d'OBJ de chaque frame.

## 3. VBlank / DMA

Transferts du NMI : OAM 544 o (chaque frame), tilemap BG3 2 Kio (si modifiée), tilemap BG2
2 Kio (si modifiée, menus et changements d'écran), 10 couleurs (public / anneaux), registres de
calques et de défilement. Pire cas : ~4,7 Kio, sous la capacité DMA d'une VBlank NTSC (~6 Kio,
davantage en PAL). Les gros transferts (stades, décors, polices) se font écran éteint
(décompression LZSS en $7F:0000 puis DMA).

## 3 bis. Temps CPU par frame (mesuré, bsnes accuracy, NTSC)

Profil d'une frame de match (lignes vidéo, maximum observé sur 1500-3000 frames, version
`DEBUG`) : IA ~46, joueurs ~79, règles ~43, sprites ~39 (joueurs) + ~26 (radar) + 1 (fin OAM).
Optimisations de la 0.12.1 : fin d'OAM incrémentale (seuls les sprites de la frame précédente
sont cachés : 13 → 1 ligne), tri d'affichage conservé d'une frame à l'autre (presque trié),
table pour les bits hauts de l'OAM, rejet rapide des zones interdites (raquettes, rond
central, coup franc) et de la règle des 4 s. Résultat : **0 frame perdue** sur des matchs
complets PAL et NTSC (compteur de frames du jeu = compteur de NMI), contre ~1 % avant.

## 4. Mémoire

- ROM : 256 Kio HiROM FastROM. Le code est relié en banque $C0 (miroir rapide de $80) :
  $C0:8000-FFFF (code principal, chaînes, tables lues par la banque de données $80) et
  $C0:0000-7FFF (compétitions, éditeurs, écran de score + tables lues en adressage long).
  Les tables utilisées en adressage absolu doivent rester en `RODATA`.
- WRAM basse : `BSS` $0200-$18AB (~5,8 Kio sur 6,5 Kio disponibles avant la pile). Les
  prochains gros buffers doivent aller en WRAM haute ($7E:2000+ ou $7F:xxxx, accès long).
  Déjà en WRAM haute : copie de la tilemap BG2 ($7E:2000), tampon de décompression ($7F:0000).
- `build.sh` affiche à chaque build l'occupation de chaque segment (`tools/romcheck.py`).

## 5. SRAM (32 Kio)

| Adresse | Bloc | Protection |
|---|---|---|
| $A0:6000 | Options (radar, durée, boutons, langue) | signature, version, longueur, checksum |
| $A0:6100 | Compétitions, copie A (3 × $400) | signature, version, taille, checksum, **génération** |
| $A0:6D00 | Compétitions, copie B (3 × $400) | idem |
| $A1:6000 | Joueurs et équipes créés | signature, version, longueur, checksum ; seul le bloc corrompu est réinitialisé |

Sauvegarde A/B : on écrit la copie la plus ancienne (ou invalide), signature effacée pendant
l'écriture puis réécrite en dernier, avec une génération +1. Au chargement, la copie valide la
plus récente est retenue : une coupure pendant l'écriture laisse l'autre copie intacte. Les
anciennes sauvegardes (sans copie B) restent lisibles.

## 6. PAL / NTSC

- Vitesses et accélérations : tables NTSC dans `include/region.inc`, table PAL calculée à
  l'assemblage (×6/5, ×36/25).
- Durées : unités logiques « tu » (1/300 s ; NTSC −5 par frame, PAL −6). Converties dans
  cette version : écrans de publicité et de score, coup d'envoi, coup franc, règle des 4 s.
- Caméra : l'anticipation vaut 133 ms de trajectoire dans les deux régions (×5/6 en PAL) et
  l'amortissement est réglé pour le même temps de réponse (1/16 par frame à 60 Hz, ~1/13 à 50 Hz).
- Restent en frames (purement visuels) : clignotement de PRESS START, cycle des étoiles des menus.

## 7. Audio

File de 8 commandes, une envoyée par frame, avec 3 priorités :
haute (point, buzzers, sifflet, corne, validation, musique) — moyenne (frappe, passe, arrêt,
charge) — basse (rebonds, murs, réception, réactions du public, menus).
Un son de priorité basse est ignoré si la file contient déjà 3 commandes (pas de retard
accumulé) ; un son important remplace la dernière commande si la file est pleine : un point
ou une fin de match ne sont jamais perdus.

## 8. Tests

- `tools/lrtest.py` : banc headless libretro (snes9x, bsnes-mercury accuracy) — captures,
  lecture / écriture WRAM, `bot N SEED` (entrées aléatoires + invariants du contrôle humain :
  manette sur un joueur de sa propre équipe, un seul joueur contrôlé, joueur bloqué).
- `tools/balance.sh` : matchs CPU contre CPU entre équipes de niveaux différents.
- Vérifié en émulation : passage interpolé dans l'anneau, gardien et ballon lent dans la
  raquette, coup franc (distance 40 px), exclusions simultanées, sauvegarde / reprise A/B,
  matchs complets PAL et NTSC sans frame perdue (bsnes).

**Reste à valider sur matériel réel** (non faisable ici) : SF Forge / Mesen2 / vraie console /
flashcart, reset à chaud, deux manettes physiques, comportement réel des lignes surchargées
d'OBJ, SRAM à pile.
