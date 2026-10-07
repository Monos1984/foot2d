# NEXUS BALL — CHANGELOG

## 0.19.3 — menus (suite)

- Écran titre : « AN OFFGAME PROJECT » retiré ; la description de l'entrée choisie remonte
  d'une ligne.
- Crédits : nouveau fond (panneaux des menus), titre dans la barre, lignes centrées, titres
  en orange et noms en cyan, aide « B BACK » ; le logo ne passe plus sous le texte.
- Éditeurs (CREATE PLAYER / CREATE TEAM) : titre aligné sur les autres écrans, aide en bas
  des listes (« A EDIT  B BACK »).

## 0.19.2 — menus

- Menu principal : un ballon ovale tourne devant l'entrée choisie, et une ligne de description
  (EN / FR, centrée, en cyan) explique chaque entrée.
- Options : titre dans la barre de titre comme les autres écrans, aide en bas
  (« <> CHANGE  B BACK »).
- Menus des compétitions : aide en bas (« A OK  B BACK »).

## 0.19.1 — bandeaux d'action, fin de période

- Bandeau « SAVE! » (« ARRÊT ! ») avec l'équipe et le nom du gardien à chaque parade, et
  « SO CLOSE! » (« TOUT PRÈS ! ») sur un tir qui frôle l'anneau (seulement si aucun autre
  message n'est affiché).
- Chronomètre en orange dans les 10 dernières secondes, bip à chacune des 5 dernières.
- Vérifié : le tirage des côtés inverse bien les camps (équipe de gauche qui défend à droite).

## 0.19.0 — tirage au sort, prolongation à 5 contre 5

- Tirage au sort « futuriste » avant chaque match : pièce holographique aux couleurs des deux
  équipes, qui monte en tournant entourée d'étincelles, ralentit et retombe sur la face du
  gagnant. Le gagnant engage ; les côtés sont aussi tirés (flèches >>> / <<< sous les noms).
- Prolongation : nouveau tirage au sort pour l'engagement, et un joueur de champ de moins par
  équipe (5 contre 5, comme au hockey) jusqu'à la fin ; tout le monde revient pour les tirs au
  but. Règles (EN / FR) mises à jour.

## 0.18.0 — présentation des équipes, écran de fin

- Nouvel écran de présentation avant chaque match : nom du stade, fiches des deux équipes
  (tenue, niveau, style, statistiques), « VS » qui clignote ; trois joueurs de chaque équipe
  entrent en courant depuis les bords puis attendent et célèbrent, clameur du public quand
  tout le monde est en place. START / A / B pour passer (après 1 s), 7 s au plus.
- Écrans de mi-temps et de fin de match : deux joueurs de chaque équipe en bas de l'écran ;
  à la fin, le vainqueur célèbre et le perdant baisse la tête (match nul : ils attendent).
- But : gerbe de 8 étincelles néon qui s'écarte de l'anneau (en plus du tremblement).

## 0.17.2 — tremblement de l'écran

- But : l'écran tremble brièvement (2 px verticaux pendant ~0,4 s, terrain et tribunes ;
  le HUD reste fixe), en plus de la corne et du bandeau.
- Statistiques de fin de match : nouvelle ligne SAVES (« ARRÊTS ») — parades des gardiens par
  équipe (captés et repoussés).

## 0.17.1 — corrections, traînée du ballon

- Traînée néon derrière le ballon libre et rapide (tirs, longues passes) : deux traces
  lumineuses sur la trajectoire (tiles OBJ des points du radar réutilisées, 30 OBJ au plus).
- Coup d'envoi : le bandeau « GO! PASS! » (« GO ! PASSE ! ») rappelle que le porteur doit
  passer (il reste immobile jusqu'à la passe).
- Coup franc : coup de sifflet et bandeau « FREE KICK! » (« COUP FRANC ! ») à la reprise du jeu.

- Compétitions : le score de l'équipe extérieure est plafonné à 99 comme celui de l'équipe à
  domicile (affichage sur 2 chiffres, mot de passe).
- Banc de test : l'alerte « joueur immobile 2 s » ignore les engagements (coup d'envoi, coup
  franc), où le tireur reste immobile par règle. Les alertes restantes viennent d'un joueur
  poussé contre un mur, pas d'une perte de contrôle.

## 0.17.0 — mot de passe des compétitions

- Chaque compétition (CHAMPIONSHIP, CUP, CUSTOM) affiche un **mot de passe** sur l'écran de
  la journée ; PASSWORD dans le menu de la compétition permet de le saisir (grille de 32
  caractères, A ajoute, B efface, START valide) et reprend la compétition au même point.
  Alternative à la sauvegarde pour les supports sans mémoire de sauvegarde.
- Les matchs CPU contre CPU sont simulés à partir d'une graine (12 bits) et du numéro du
  match : ils se recalculent à l'identique, le mot de passe ne garde que les réglages, les
  équipes, la position dans le calendrier et les scores des matchs humains (et des matchs CPU
  regardés). 12 à 16 caractères en début de championnat de 8 équipes ; contrôle CRC 10 bits.
- Si une compétition est déjà sauvegardée, valider un mot de passe demande « REPLACE SAVE? »
  (NO par défaut : la sauvegarde est conservée).
- Sauvegarde des compétitions en version 4 (graine ajoutée) ; les sauvegardes version 3
  restent lisibles.
- Tests : aller-retour mot de passe -> compétition identique (ligue, coupe, perso, en cours
  et terminées, avec match humain), mots de passe altérés refusés.

## 0.16.0 — confirmation dans les menus, animations des joueurs

- Retour au menu principal confirmé partout où un choix peut être perdu (« MAIN MENU? »
  NO / YES, NO par défaut) : choix des équipes, réglages du match, menus des compétitions
  (B, BACK, SAVE & EXIT), listes des éditeurs de joueurs et d'équipes.
- Joueurs : 24 cadres d'animation au lieu de 16 — course en 8 phases (appuis avec corps
  abaissé, jambes intermédiaires), course avec ballon en 4 phases, accompagnement après la
  frappe et le lancer, respiration à l'arrêt (décalée d'un joueur à l'autre), célébration
  animée. Les tiles OBJ du radar retiré sont réutilisées (ballon et petites tiles déplacés).

## 0.15.0 — confirmation, choix des équipes, gardiens

- Pause : QUIT MATCH demande une confirmation (QUIT MATCH? NO / YES, NO par défaut) ;
  B reprend le match, A valide.
- Choix des équipes : la fiche en cours de modification est signalée par deux curseurs
  animés au-dessus de son titre et des flèches clignotantes autour du nom ; HAUT / BAS change
  de fiche.
- Gardien : un ballon repoussé part toujours vers le terrain (plus de but contre son camp sur
  une parade), une relance ne vise qu'un coéquipier nettement devant, sinon dégagement long
  vers le camp adverse ; ballon relâché toujours devant la ligne de fond.
- Gardien de l'équipe du joueur : quand il a le ballon, la manette en prend le contrôle (passe,
  tir, lancer), sans pouvoir sortir de sa raquette ; dès qu'il l'a lâché, retour au joueur de
  champ le plus proche.
- Tests : compteurs de buts contre son camp (`dbg_og`, `dbg_gkog`) ; 3 matchs CPU contre CPU :
  aucun.

## 0.14.1 — menu pause et chronomètre

- Chronomètre arrêté pendant les engagements : coup d'envoi (jusqu'à la passe reçue par le
  partenaire), arrêt de jeu après une faute et coup franc (jusqu'à la passe ou au tir).
- Nouveau menu pause : cadre néon (style des bandeaux), titre PAUSE en cyan avec filet doré,
  ligne choisie en orange entre deux chevrons.

## 0.14.0 — terrains et menus

- Terrains : éclairage (4 halos de projecteurs tramés, bords assombris), raquettes hachurées
  (zone réservée aux gardiens visible), emblème hexagonal au centre, liseré néon autour du
  terrain, quarts de cercle dans les coins (6 stades, 500 à 600 tiles chacun).
- Menus : nouveaux panneaux (fond à lignes de balayage, cadre néon double à coins biseautés
  avec accent orange, barre de sélection et bandeau de titre en dégradé lumineux).

## 0.13.1 — visière transparente, choix des tenues

- Visière transparente : le visage (et la teinte de peau) se voit derrière le verre, cadre
  et reflet néon.
- Choix des équipes : X (ou Y) change la tenue de la fiche active (domicile / extérieur), pour
  les deux équipes, avec mise à jour immédiate des joueurs affichés et des plaques du HUD. Sans
  choix, la règle automatique reste (l'équipe extérieure change si les couleurs sont proches).

## 0.13.0 — joueurs du futur

- Nouveaux sprites des joueurs (même gabarit 16×32, 16 poses) : casque à coque aux couleurs de
  l'équipe avec visière néon cyan et crête métallique, épaulières et plastron avec ligne
  lumineuse, ceinture à boucle néon, combinaison, gants et genouillères métalliques, bottes à
  semelle lumineuse.
- Couleurs d'équipement fixes (métal, néon, blanc) sur les index 11-15 des palettes de joueurs ;
  les gardiens ont des protections claires (gants et genouillères dorés) pour être reconnus.

## 0.12.3 — suite de la feuille de route

- Pause : la foule se tait (reprise du volume au retour en jeu).
- WRAM basse : sauvegarde des lignes du menu pause déplacée en WRAM haute ($7E:3000), −320 octets.
- Version DEBUG : SELECT maintenu = test de stress (12 joueurs alignés sur la ligne du ballon).
- Banc de test : `bot` sur les deux manettes (1P vs 2P), ROM de debug construite à part avec
  ses propres symboles (la version normale n'est plus touchée).
- Tests de bout en bout : Championship et Cup terminés, tirs au but, surcharge de sprites,
  1P vs 2P (détails dans docs/TECHNIQUE.md).

## 0.12.2 — sans radar

- Mini-radar retiré : plus d'affichage en match, plus d'option RADAR dans OPTIONS ni dans le menu
  pause (RESUME / TEAM SETUP / QUIT MATCH), SELECT n'a plus d'effet. 28 OBJ au plus par frame.
- Vérifié sur bsnes : match NTSC complet sans frame perdue (0.12.1).

## 0.12.1 — performances

- Mesure : ~1 % de frames perdues en NTSC sur bsnes (matchs complets). Profilage par étape
  (version DEBUG, ligne vidéo atteinte) puis optimisations : fin d'OAM incrémentale, tri des
  sprites persistant, bits hauts de l'OAM par table, radar sans décalages arithmétiques, rejets
  rapides des zones interdites et de la règle des 4 s, réflexions IA étalées au coup d'envoi.
  Résultat : 0 frame perdue.

## 0.12.0 — fiabilisation (audit « Correctifs et améliorations »)

- But : passage dans l'anneau testé au point exact de franchissement du plan (interpolation de
  Y et Z entre deux frames), plus de but refusé / accordé à tort sur les tirs rapides.
- Port > 4 s : même séquence qu'une faute (buzzer, arrêt court, coup franc avec adversaires à
  distance) au lieu d'un simple changement de porteur.
- OAM : curseurs, ballon et joueurs écrits avant le radar (le radar perd ses sprites en premier
  si une ligne est surchargée).
- Exclusions : un minuteur par joueur, plusieurs exclusions simultanées ; retour sans
  réapparaître sur le ballon ; la manette passe au premier coéquipier présent.
- Audio : priorités dans la file des sons (un point / une fin de match ne sont jamais perdus,
  les petits bruitages ne s'accumulent plus).
- SRAM : compétitions sauvegardées en double (copies A / B avec numéro de génération, signature
  écrite en dernier).
- PAL / NTSC : anticipation et amortissement de la caméra en temps réel identique ; durées des
  écrans de publicité et de score en temps logique.
- Pause : les boutons de sortie de pause ne déclenchent plus d'action.
- Fin de match : ligne WINNER / DRAW (après tirs au but si besoin).
- Compétition : nom du tour de coupe mal placé (écrasait le titre) corrigé.
- Build : vérification automatique de la ROM (`tools/romcheck.py`), version `DEBUG=1` avec
  affichage de contrôle, arborescence `assets/` pour les futurs graphismes, documentation
  technique `docs/TECHNIQUE.md`, README à jour.

## 0.11.6 — gardien et ballon au sol

- Correction : après un arrêt manqué, le gardien ne pouvait plus ramasser le ballon tant que
  personne d'autre ne l'avait touché (drapeau « tentative déjà faite sur ce tir »). Ce blocage ne
  s'applique plus que tant que le tir file : un ballon lent ou arrêté dans la raquette est
  toujours ramassé par le gardien.

## 0.11.5 — règle des 4 secondes en raquette

- Le ballon ne peut pas rester 4 secondes dans une raquette (porté par le gardien ou libre).
  Sinon : buzzer, bandeau « 4 SEC IN THE ZONE! » à la couleur de l'équipe fautive, et
  l'adversaire reçoit le ballon au centre (coup d'envoi à deux).
- Règles du jeu (EN / FR) mises à jour.

## 0.11.4 — relance du gardien

- Le gardien ne relance plus vers un coéquipier à côté ou derrière lui : seulement vers un
  joueur au moins 48 pixels devant (le plus démarqué). Sinon, dégagement long au pied vers le camp
  adverse.
- Musiques d'ambiance du match retirées (le bruit de la foule et les bruitages restent).

## 0.11.3 — bruitages

- Nouvel échantillon « impact métallique » : le ballon qui touche un mur de l'arène sonne
  (dès une vitesse faible).
- Rebond au sol audible, réception du ballon (« pop »), passes et frappes plus fortes.
- Ambiance du public : « ooh » sur un tir qui frôle l'anneau, sifflets sur une faute, clameur
  quand un ballon est arraché, corne de but à chaque point.

## 0.11.2 — curseurs des menus

- Correction : curseur et barre de sélection absents ou mal placés dans CREATE PLAYER,
  CREATE TEAM, les compétitions (CONTINUE / NEW, réglages, journée, avant-match). Les tables de
  lignes de ces écrans étaient restées dans le segment de code déplacé en $C0:0000-7FFF, où la
  lecture par la banque de données ($80) tombait en WRAM : elles sont revenues en RODATA.
- Correction : retour d'un sous-écran (tactiques depuis MATCH SETUP ou l'avant-match,
  composition depuis les tactiques) avec le curseur sur la mauvaise ligne (ui_sel partagé).
- Compétition : retour du classement / tableau avec le curseur sur TABLE / BRACKET.

## 0.11.1 — corrections et coup franc

- Correction : animation de course figée (l'index du cycle valait toujours 0).
- Correction : ballon bloqué près d'une raquette. Le gardien ne sortait que dans un rayon plus petit
  que la zone interdite aux autres joueurs ; il va maintenant chercher tout ballon libre dans sa
  raquette et ses abords.
- Coup franc après une faute (ou un port trop long) : le joueur qui reçoit le ballon ne bouge
  pas, les adversaires restent à 40 pixels jusqu'à sa passe ou son tir (6 s au plus).
- Ambiance : 3 airs de stade différents (grosse caisse, mains, cor) qui changent au coup d'envoi,
  après chaque point et à la mi-temps.

## 0.11.0 — coup d'envoi, écrans de score, ambiance

- Raquettes : aucun joueur de champ n'entre dans aucune des deux raquettes (seuls les gardiens).
- Coup d'envoi : deux joueurs dans le rond central ; le porteur ne bouge pas et doit passer à son
  partenaire (humain : n'importe quel bouton d'action ; IA : après 2/3 s). Le jeu reprend quand
  le partenaire a le ballon ; jusque-là, personne d'autre n'entre dans le rond central.
- Mi-temps : écran de score (plaques, gros chiffres, noms, statistiques) avant les publicités.
- Fin de match : nouvel écran de statistiques (barres aux couleurs des équipes pour tirs,
  possession, fautes ; tirs au but).
- Son : buzzer long pour la fin des périodes (buzzer court pour les fautes), ambiance de stade
  (grosse caisse, claquements de mains, cor) pendant le match, bruit de foule diminué,
  silence pendant le logo OFFGAME (volume de la foule à zéro au démarrage du SPC700).
- Pilote SPC700 : table d'instruments propre à chaque morceau.
- Équilibrage après les raquettes interdites : gardien un peu moins sûr, tir moins gêné par un
  adversaire au contact.
- Code relié en banque $C0 (miroir FastROM de $80) : les modules compétitions, éditeurs et écran de
  score sont dans la moitié basse $C0:0000-7FFF (place libérée en banque haute).

## 0.10.0 — règles, buzzer, bandeaux, intro, choix des équipes

- Règle : seul le gardien peut entrer dans sa raquette (les joueurs de champ sont repoussés sur
  le bord de la zone).
- Règle : aucun point ne peut être marqué depuis sa propre moitié de terrain (position du
  lanceur au moment du tir ; message « NO SCORE FROM OWN HALF »). Les buts contre son camp et
  les tirs au but ne sont pas concernés.
- Buzzer (nouvel échantillon SPC700) pour la fin des périodes et les fautes.
- Messages du match en bandeau : cadre néon sur 3 lignes, à la couleur de l'équipe concernée,
  avec son nom court et le nom du joueur (marqueur, fautif). Fautes, points, avantage, port trop
  long, tir depuis sa moitié.
- Démarrage : logo OFFGAME, écran titre « PRESS START », puis le menu.
- Choix visuel des équipes (avant MATCH SETUP, ou A sur HOME / AWAY) : deux fiches avec les
  joueurs au maillot de l'équipe qui courent, nom, planète, style, niveau en étoiles et moyennes
  de l'effectif (vitesse, puissance, passe, défense) ; gauche / droite change d'équipe,
  haut / bas change de côté.
- Place en banque $80 : tables de calendrier, couleurs de maillots, constantes de région,
  formations et palettes des stades déplacées en banque $C0.

## 0.9.1 — corrections, publicités, fond des menus

- Correction : un but contre son camp vaut toujours 1 point (il pouvait en donner 2 si le
  dernier tir venait de loin).
- Correction : perte des commandes / contrôle d'un joueur adverse. Le changement automatique
  de joueur gardait le candidat dans t5 alors que dist_to_ball l'écrase : la manette pouvait
  recevoir un numéro quelconque (adversaire ou hors tableau). Variables dédiées sw_cand / sw_lim.
- Correction : après une faute grave, la manette ne peut plus reprendre le joueur exclu.
- Écrans de publicité futuristes (NOVA COLA, ZENTEK, ORBITEL, HYPERION FUEL) avant le match et à
  la mi-temps (3 s, A / B / START pour passer).
- Nouveau fond des menus et de l'écran titre : nébuleuse, planète annelée ombrée, lune, ville
  futuriste et arène à l'horizon, sol à grille néon. Décors BG1 multi-palettes (tools/scenes.py,
  palettes BG 2, 5, 6, 7).
- Outil de test : commande `bot` de tools/lrtest.py (entrées aléatoires + invariants du contrôle).

## 0.9.0 — ballon ovale, règles, français

- Ballon ovale futuriste (sprite 16×16) : coque bleu métal, couture néon qui tourne, anneau
  central, pointes lumineuses ; 4 orientations selon la trajectoire (ou la direction du porteur),
  ombre ovale.
- Nouveau menu RULES / RÈGLES : 5 pages (le jeu, le score, port et passes, charges et fautes,
  match et commandes), gauche / droite pour changer de page.
- Langue : option LANGUAGE dans OPTIONS (ENGLISH / FRANÇAIS), sauvegardée en SRAM (octet 9 du
  bloc options). Traduction de tous les menus, écrans de match, éditeurs et compétitions :
  print remplace la chaîne anglaise par sa traduction (table générée par tools/lang.py, banque
  $C0, recherche par hash + vérification du texte). Lettres accentuées É È Ê À Ç Ô ajoutées
  à la police.

## 0.8.0 — joueurs, stades et HUD

- Joueurs agrandis : silhouette 16×24 dans des cadres 16×32 (deux sprites), tête, maillot avec
  col et numéro, short, bras et jambes animés, ombre au sol ; 16 poses (course, frappe, lancer,
  charge, chute, plongeon, célébration…).
- Terrains refaits : sol à motifs avec reflets des projecteurs, panneaux publicitaires néon sur le
  mur du haut, murs latéraux techniques, mur vitré en bas, lignes plus épaisses, emblème central,
  anneaux sur plaque sombre avec halo.
- Tribunes sur BG2 : foule animée (couleurs cyclées) visible à travers les zones transparentes du
  stade, défilement en parallaxe (½ vitesse horizontale).
- Nouveau HUD : plaques biseautées à la couleur de chaque équipe, cadre central à gros chiffres
  8×16 pour le score, horloge et période sous le score (tiles BG3 dédiées, palette 6).

## 0.7.0 — compression et menus graphiques

- Compression LZSS (tools/lz.py) de tous les graphismes : stades, sprites, police, logo, fond des
  menus. Décompression 65C816 (src/lz.asm) vers la WRAM $7F:0000 puis DMA vers la VRAM.
  ~110 Kio de graphismes bruts tiennent dans ~38 Kio de ROM.
- Menus refaits : fond dédié (ciel étoilé qui scintille, planète annelée, sol à grille néon) sur BG1,
  panneaux encadrés translucides sur BG2 (color math : moyenne avec le fond), barre de sélection
  sur la ligne choisie, texte avec ombre portée.
- Écran titre : logo opaque (HDMA sur CGADSUB), menu dans un panneau translucide sur le stade.
- En match, l'écran TEAM SETUP de la pause s'affiche en panneau translucide sur le terrain.
- Fondus enchaînés (luminosité) à chaque changement d'écran ; bandeau de titre dans les menus.
- Registres de calques (TM, TS, CGWSEL, CGADSUB, HDMAEN) écrits au NMI depuis des ombres ;
  tilemap BG2 copiée en WRAM $7E:2000.

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
- Équilibrage mesuré (tools/balance.sh, matchs CPU entre équipes de niveaux différents) :
  gardien moins infaillible (portée et réussite réduites), duels de charge plus incertains,
  écart de vitesse entre SPEED 1 et 9 réduit. Avant : ~1 point pour 30 tirs et domination totale
  d'une équipe de niveau +1 ; après : 2 à 8 points par match de 4 min, le favori gagne sans écraser.
- IA : jeu contre le mur — un porteur bloqué frappe en diagonale vers le mur le plus proche
  pour contourner le défenseur (rebond vers l'avant).
- Écran titre : logo NEXUS / BALL en dégradé bleu / orange avec contour et orbite (BG2, 4bpp),
  généré par tools/gfx.py.

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
