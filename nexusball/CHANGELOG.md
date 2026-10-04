# NEXUS BALL — CHANGELOG

## 0.6.0 — finition (début du Milestone 9)

- Teinte de peau en match : palettes OBJ 5/6 (maillot de l'équipe + peau foncée), masque par
  équipe officielle, teinte des joueurs créés reprise dans CREATE TEAM (format d'équipe 262 octets,
  bloc d'édition SRAM version 2).
- Compétitions : option WATCH CPU MATCHES (les matchs CPU se jouent à l'écran), choix PLAY AS
  HOME / AWAY quand la même manette possède les deux équipes (sauvegarde version 3).
- OPTIONS : 3 configurations de boutons (TYPE A / B / C), sauvegardées en SRAM.
- Jeu contextuel : X ou Y sans ballon près d'un ballon libre (même en l'air) = reprise de volée.
- Les anneaux clignotent pendant la célébration d'un but (couleurs du stade restaurées ensuite).
- Écran de fin de match : tirs, possession (%), fautes de chaque équipe.

## 0.5.0 — création (Milestone 8)

- Menu principal complet : EXHIBITION, CHAMPIONSHIP, CUP, CUSTOM COMPETITION, CREATE TEAM,
  CREATE PLAYER, OPTIONS, CREDITS.
- Clavier virtuel (A ajoute, B efface, START termine), répétition automatique des directions.
- CREATE PLAYER : 32 emplacements, budget de caractéristiques (8 et 9 coûtent plus cher),
  libellés gardien (REFLEX, CATCH, POSITION, THROW).
- CREATE TEAM : 8 emplacements, couleurs prédéfinies compatibles CGRAM (maillots domicile et
  extérieur générés), formation, tactique, effectif choisi parmi les joueurs créés et officiels,
  niveau global calculé.
- Équipes créées (numéros 16 à 23) dans les listes d'exhibition et de compétitions
  (liste défilante) ; sauvegarde de compétition passée en version 2.
- Données d'équipes officielles déplacées en banque $C0 (copie MVN à la demande).

## 0.4.0 — modes de jeu (Milestone 7)

- Menu principal conforme au cahier des charges (sans Career) ; écran OPTIONS (radar, durée).
- MATCH SETUP : choix du mode (1P vs CPU, 1P vs 2P, CPU vs CPU).
- Compétitions : CHAMPIONSHIP (ligue), CUP (4/8/16), CUSTOM COMPETITION (ligue ou coupe) ;
  chaque équipe CPU / P1 / P2 ; calendrier par la méthode du cercle (tools/sched.py) ;
  tirage au sort de la coupe ; prolongation et tirs au but en coupe.
- Simulation instantanée des matchs CPU contre CPU (niveau des équipes, 1 ou 2 points).
- Écrans : journée / tour, classement, tableau, avant-match, champion.
- Sauvegarde automatique après chaque match et chaque journée, reprise (CONTINUE).
- Textes de menu sur des palettes BG3 dédiées (plus d'interférence avec les maillots).

## 0.3.0 — présentation (Milestone 6)

- 6 stades générés (palettes, sols, bannières différents ; géométrie officielle identique),
  stockés en banques $C2/$C3, choix dans MATCH SETUP.
- Audio : pilote SPC700 assemblé par tools/spc.py (mini-assembleur intégré), chargé par le
  protocole IPL au démarrage. Effets sonores, ambiance du public par canal de bruit,
  séquenceur 3 voix (musique de titre en boucle, jingle de fin de match).
- File de commandes son (une commande par frame).

## 0.2.0 — règles complètes et tactique (Milestones 4 et 5)

- 16 équipes officielles générées (tools/teams.py) : effectif de 12 (2 GK, 4 DF, 3 MF, 3 FW),
  styles SPEED / POWER / PASSING / DEFENSIVE / OFFENSIVE / COUNTER / BALANCED / TECHNICAL,
  maillots domicile / extérieur (extérieur automatique si couleurs proches).
- Écran MATCH SETUP : équipes, tactiques des deux équipes, difficulté, règle d'égalité.
- 6 formations, 6 tactiques réellement utilisées par l'IA (placement, pressing, passes,
  couloir d'attaque, rythme), difficulté appliquée aux équipes CPU (réaction, charges).
- Composition et remplacements illimités (avant match et menu pause), fatigue par joueur,
  récupération à la mi-temps.
- Prolongation 2 min avec but en or, tirs au but (3 tentatives, mort subite, fin anticipée).
- Duel de charge : POWER + CONTROL contre POWER + DEFENSE ; l'IA évite les charges par
  derrière et ne charge plus le gardien dans sa zone.
- Corrections : signe perdu dans la division signée (tirs), gardien qui captait tous les tirs.

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
