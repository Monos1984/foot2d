# OFFGAME — NEXUS BALL SNES
## Cahier des charges de production — Version 0.2

**Nom de travail :** NEXUS BALL  
**Plateforme cible :** Super Nintendo Entertainment System / Super Famicom  
**Genre :** sport futuriste, descendant du football et du rugby  
**Langue du jeu :** anglais uniquement  
**Développement :** assembleur 65C816 natif SNES  
**Modes vidéo :** PAL 50 Hz + NTSC 60 Hz  
**Joueurs :** 1 joueur contre IA ou 2 joueurs locaux  
**Production / direction :** Jean Monos — OFFGAME  
**Pas de mode en ligne. Pas de mode carrière.**

---

# 1. Vision générale

Créer un **véritable jeu de sport futuriste pour Super Nintendo**, pensé dès le départ autour des capacités et des contraintes de la machine.

Le sport est un descendant lointain du **football** et du **rugby** :

- occupation tactique du terrain ;
- défenseurs, milieux, attaquants et gardien ;
- passes, tirs, interceptions et construction collective ;
- possibilité de porter le ballon ;
- contacts physiques et charges ;
- progression collective ;
- formations et tactiques.

Le résultat ne doit cependant pas être un simple mélange de football et de rugby : il doit avoir ses **propres règles**, devenir immédiatement identifiable et rester agréable à jouer à la manette SNES.

Le ton recherché est :

> **fun, sportif, rapide et tactique — entre arcade et simulation.**

Le jeu ne doit être ni une simulation lourde, ni un jeu d'action sans profondeur.

---

# 2. Principes incontournables

Le projet doit respecter les points suivants :

- vraie ROM Super Nintendo / Super Famicom ;
- code principal écrit en assembleur **65C816** ;
- compilation reproductible en `.sfc` ;
- fonctionnement sur matériel réel ;
- fonctionnement sur émulateurs SNES précis ;
- PAL 50 Hz et NTSC 60 Hz ;
- gameplay à vitesse réelle équivalente dans les deux régions ;
- graphismes dignes d'un bon jeu commercial SNES ;
- 6 joueurs par équipe sur le terrain :
  - 5 joueurs de champ ;
  - 1 gardien ;
- 12 joueurs dans l'effectif de chaque équipe ;
- solo contre l'IA ;
- 2 joueurs locaux ;
- Championship ;
- Cup ;
- compétitions personnalisées ;
- formations ;
- tactiques ;
- plusieurs stades ;
- création de joueurs ;
- création d'équipes ;
- sauvegarde SRAM ;
- mini-radar activable/désactivable ;
- interface et textes du jeu en anglais ;
- pas de réseau ;
- pas de carrière ;
- pas de gestion financière, contrats ou transferts.

---

# 3. Référence visuelle fournie

Une image de référence est fournie séparément au développeur.

Elle définit la **direction souhaitée**, pas un écran à reproduire pixel par pixel.

Points importants de cette référence :

- vue sportive du dessus légèrement inclinée ;
- terrain sombre futuriste mais très lisible ;
- deux équipes fortement différenciées par couleur ;
- grand public dans les tribunes ;
- buts sous forme de grands anneaux lumineux ;
- ballon immédiatement visible ;
- HUD compact en haut ;
- petits sprites mais suffisamment détaillés ;
- ambiance 16 bits / Super Nintendo ;
- jeu contre les murs ;
- actions aériennes ;
- mini-radar ;
- fort contraste bleu / orange.

À modifier par rapport à la maquette :

- **aucun mode Career** ;
- tous les menus du jeu final sont en **anglais** ;
- l'interface doit respecter les contraintes réelles de la SNES ;
- l'image sert de concept art et non de preuve de faisabilité technique.

---

# 4. Univers

Le sport est devenu une compétition majeure à l'échelle interplanétaire.

Les équipes représentent :

- planètes ;
- lunes ;
- colonies ;
- systèmes stellaires ;
- mondes artificiels ;
- régions galactiques ;
- civilisations sportives.

Le jeu doit donner l'impression d'un championnat venu d'un futur très lointain, mais son univers reste avant tout **sportif**, pas militaire.

Éviter :

- soldats ;
- armes ;
- armures massives ;
- pouvoirs magiques ;
- super-héros.

Les personnages restent des sportifs.

---

# 5. Équipes de base

Objectif recommandé pour la première version complète :

**16 équipes officielles.**

Cela permet naturellement :

- Championship ;
- Cup à 16 ;
- tournoi personnalisé ;
- variété de styles.

Exemples provisoires :

1. Orion Stars
2. Titan Crushers
3. Vega Strikers
4. Andromeda Wolves
5. Mars United
6. Solaris Falcons
7. Europa Ice
8. Sirius Raiders
9. Nova Prime
10. Centauri Force
11. Lunar Knights
12. Alpha Drakes
13. Neptune Storm
14. Phoenix Core
15. Helios Blades
16. Cygnus Rangers

Les noms peuvent être remplacés durant la production.

Chaque équipe possède :

- Team Name ;
- Short Name ;
- Home World ;
- couleurs ;
- emblème ;
- Home Kit ;
- Away Kit ;
- effectif de 12 joueurs ;
- formation par défaut ;
- tactique par défaut ;
- niveau global ;
- style de jeu.

---

# 6. Identité sportive des équipes

Les équipes doivent réellement se jouer différemment.

Exemples de styles :

### Speed
Équipe très rapide, moins puissante.

### Power
Contacts physiques forts, mobilité plus faible.

### Passing
Excellente circulation du ballon.

### Defensive
Bloc compact, gardien solide.

### Offensive
Deux attaquants, pressing haut.

### Counter
Transitions rapides après récupération.

### Balanced
Aucune faiblesse majeure.

### Technical
Très bon contrôle et passes précises.

Ces différences viennent des joueurs, de la tactique et de la formation, pas de bonus artificiels cachés.

---

# 7. Terrain

Le terrain est rectangulaire et fermé.

Contrairement au football traditionnel :

**les murs latéraux font partie du terrain.**

Le ballon peut rebondir contre eux.

Le joueur peut volontairement utiliser un mur pour :

- passer autour d'un adversaire ;
- se faire une passe à lui-même ;
- changer rapidement l'angle d'une attaque ;
- effectuer une passe indirecte ;
- tenter un tir avec rebond.

Il n'y a donc pratiquement pas de touche.

Cela accélère le jeu et donne au sport une identité propre.

---

# 8. Dimensions logiques recommandées

Les dimensions exactes sont internes au moteur et peuvent être ajustées au prototype.

Ratio recommandé :

- longueur : environ 2 fois la largeur ;
- terrain plus compact qu'un terrain de football ;
- assez d'espace pour 12 joueurs sans provoquer de grappes permanentes.

Le terrain doit comporter :

- ligne médiane ;
- cercle central ;
- deux zones de gardien ;
- une ligne de tir longue distance ;
- deux anneaux de but ;
- murs latéraux ;
- murs de fond autour de la zone de but.

Les graphismes de stades peuvent changer, mais la géométrie officielle du terrain reste identique.

---

# 9. Buts

Chaque équipe défend un grand **anneau vertical**.

Le ballon doit entièrement traverser l'anneau pour que les points soient accordés.

Le cercle doit être suffisamment grand pour permettre :

- tirs directs ;
- tirs avec rebond ;
- frappes aériennes ;
- interceptions du gardien.

Le gardien se place devant l'anneau et protège une petite zone.

Le ballon ratant l'anneau peut rebondir sur le mur de fond et rester en jeu.

---

# 10. Règles de score

Pour conserver un système immédiatement compréhensible :

## Hand / close score — 1 point

Un ballon envoyé dans l'anneau depuis la zone offensive ou à courte distance vaut :

**1 POINT**

## Kick / long score — 2 points

Une frappe au pied réussie depuis l'extérieur de la ligne de tir longue distance vaut :

**2 POINTS**

La ligne doit être clairement visible.

Le jeu ne doit pas multiplier les valeurs de score.

Le choix tactique devient simple :

- construire une action sûre à 1 point ;
- tenter une frappe difficile à 2 points.

---

# 11. Utilisation des mains et des pieds

Le sport doit conserver clairement son héritage football / rugby.

Le joueur peut :

- contrôler le ballon ;
- le porter brièvement ;
- le passer à la main ;
- le passer au pied ;
- le frapper vers le but.

### Passe à la main

Très rapide et précise, principalement à courte distance.

### Passe au pied

Plus longue, plus difficile à intercepter mais moins précise.

### Tir au pied

Principal moyen d'obtenir les tirs longue distance à 2 points.

### Lancer vers l'anneau

Autorisé à courte portée et vaut normalement 1 point.

---

# 12. Règle de port du ballon

Un joueur peut courir avec le ballon en main.

Pour éviter qu'un joueur très rapide traverse seul tout le terrain :

- temps de possession individuelle maximum recommandé : **4 secondes** ;
- un compteur interne commence lors de la prise en main ;
- le joueur doit ensuite :
  - passer ;
  - frapper ;
  - tirer ;
  - relâcher le ballon.

Une alerte visuelle discrète peut apparaître durant la dernière seconde.

Si le joueur conserve le ballon trop longtemps :

- perte de possession ;
- remise en jeu rapide pour l'adversaire.

Cette durée pourra être ajustée après tests.

---

# 13. Règle de passe avant

Pour conserver une influence du rugby sans compliquer excessivement le jeu :

### Passe à la main

Une passe manuelle effectuée clairement vers l'avant est interdite.

Elle doit être :

- latérale ;
- ou vers l'arrière.

### Jeu au pied

Le ballon peut être envoyé vers l'avant librement au pied.

Cette règle crée naturellement :

- soutien collectif ;
- lignes d'attaque ;
- progression ;
- passes en retrait ;
- dégagements ;
- jeu de profondeur.

Elle donne au sport une identité forte.

Le prototype doit cependant vérifier que cette règle reste agréable à la manette.

Si elle rend le jeu trop lent ou confuse, elle pourra être assouplie.

---

# 14. Pas de hors-jeu complexe

Le jeu ne doit pas utiliser un système de hors-jeu comparable au football.

La contrainte de la passe à la main et le placement de l'IA suffisent pour structurer le terrain.

Cela évite :

- arrêts permanents ;
- règles difficiles à lire ;
- calculs inutiles ;
- frustration.

---

# 15. Contacts physiques

Les contacts font partie du sport.

Un défenseur peut effectuer une charge contrôlée sur le porteur du ballon.

Résultats possibles :

- porteur résiste ;
- ballon lâché ;
- ballon récupéré ;
- porteur déséquilibré ;
- charge ratée.

Le résultat dépend principalement de :

- POWER de l'attaquant ;
- POWER du défenseur ;
- DEFENSE ;
- vitesse relative ;
- angle du contact.

Le système doit rester déterministe et lisible, avec une faible variation aléatoire.

---

# 16. Charges interdites

Sont considérées comme fautes :

- charge par derrière à pleine vitesse ;
- charge sur un joueur déjà au sol ;
- contact sur le gardien dans sa zone protégée ;
- contact très tardif après une passe ou un tir.

Il ne faut pas créer une simulation arbitrale lourde.

Le but est simplement d'empêcher que la meilleure stratégie soit de frapper tous les adversaires.

---

# 17. Sanctions

Les sanctions doivent interrompre le jeu le moins longtemps possible.

### Foul

Possession rendue à l'équipe victime au lieu de la faute.

### Defensive zone foul

Remise en jeu offensive proche de la zone.

### Major foul

Exclusion temporaire possible pendant environ **20 secondes de temps de jeu**.

Cette dernière sanction peut être réservée aux charges les plus manifestes.

Pas de cartons complexes.

---

# 18. Avantage

Si l'équipe victime garde clairement le ballon et possède une bonne situation offensive :

**l'arbitre laisse jouer.**

Le système peut mémoriser brièvement la faute et ne revenir dessus que si nécessaire.

Cela évite de casser le rythme.

---

# 19. Remise en jeu après un point

Après chaque score :

- petite célébration rapide ;
- score mis à jour ;
- ballon replacé au centre ;
- équipe ayant encaissé engage.

La transition doit être courte.

Objectif :

**retour au jeu en quelques secondes.**

---

# 20. Début du match

Au coup d'envoi :

- les deux équipes sont dans leur moitié ;
- ballon au centre ;
- une équipe engage ;
- possession alternée au début de la seconde période.

Pas de longue animation obligatoire.

Les présentations peuvent être passées avec START.

---

# 21. Durée d'un match

Réglage recommandé par défaut :

**2 halves × 4 minutes de temps réel.**

Options :

- 2 × 2 min
- 2 × 3 min
- 2 × 4 min
- 2 × 5 min

Le chronomètre doit représenter le même temps réel en PAL et en NTSC.

Le temps peut être brièvement arrêté lors :

- d'un point ;
- d'une pause ;
- d'une sanction majeure.

---

# 22. Égalité

### Exhibition / Championship

Le match peut se terminer sur une égalité si le règlement du championnat l'autorise.

### Cup

En cas d'égalité :

1. overtime de 2 minutes ;
2. golden score pendant l'overtime ;
3. si toujours égalité : shootout.

---

# 23. Shootout

Format recommandé :

- 3 tentatives par équipe ;
- attaquant contre gardien ;
- départ depuis une marque fixe ;
- 5 secondes maximum pour tirer ;
- alternance des équipes.

En cas d'égalité après trois tentatives :

- mort subite.

Ce système doit être rapide et spectaculaire.

---

# 24. Composition d'équipe

Chaque équipe possède **12 joueurs** :

- 2 Goalkeepers ;
- 4 Defenders ;
- 3 Midfielders ;
- 3 Forwards.

Cette distribution est une base recommandée et peut varier légèrement selon les équipes.

Sur le terrain :

- 1 GK ;
- 5 field players.

Sur le banc :

- 6 substitutes.

---

# 25. Postes

Postes principaux :

- GK — Goalkeeper
- DF — Defender
- MF — Midfielder
- FW — Forward

Certains joueurs peuvent avoir un poste secondaire.

Exemples :

- DF/MF
- MF/FW

Le poste influence surtout :

- placement IA ;
- position préférée ;
- choix de passes ;
- comportement défensif/offensif.

---

# 26. Formations

Formations minimum :

### 2-2-1
2 DF — 2 MF — 1 FW  
Équilibrée.

### 2-1-2
2 DF — 1 MF — 2 FW  
Offensive.

### 1-3-1
1 DF — 3 MF — 1 FW  
Possession et soutien.

### 1-2-2
1 DF — 2 MF — 2 FW  
Très offensive.

### 3-1-1
3 DF — 1 MF — 1 FW  
Défensive.

### 3-2-0
3 DF — 2 MF  
Très prudente, sans véritable attaquant.

Les joueurs peuvent changer de formation avant le match et pendant une pause.

---

# 27. Tactiques

Paramètres simples :

## MENTALITY
- DEFENSIVE
- BALANCED
- OFFENSIVE

## PASSING
- SHORT
- MIXED
- LONG

## PRESSURE
- LOW
- NORMAL
- HIGH

## DEFENSIVE LINE
- DEEP
- NORMAL
- HIGH

## ATTACK
- CENTER
- SIDES
- MIXED

## TEMPO
- SLOW
- NORMAL
- FAST

Les réglages doivent réellement modifier l'IA.

---

# 28. Caractéristiques des joueurs

Échelle :

**1 à 9**

Caractéristiques des joueurs de champ :

- SPEED
- POWER
- PASS
- KICK
- CONTROL
- DEFENSE
- STAMINA

Gardien :

- REFLEX
- CATCH
- POSITION
- THROW
- POWER
- STAMINA

Éviter les statistiques cachées inutiles.

---

# 29. Effet des caractéristiques

### SPEED
Vitesse maximale et accélération.

### POWER
Résistance aux contacts et puissance des charges.

### PASS
Précision des passes à la main et au pied.

### KICK
Puissance et précision des frappes.

### CONTROL
Réception, contrôle d'un ballon libre et perte de balle sous pression.

### DEFENSE
Placement, interceptions et efficacité des duels.

### STAMINA
Vitesse de fatigue et récupération.

### REFLEX
Temps de réaction du gardien.

### CATCH
Probabilité de capter plutôt que repousser.

### POSITION
Qualité du placement automatique du gardien.

### THROW
Qualité et distance des relances à la main.

---

# 30. Fatigue

La fatigue existe, mais reste légère.

Elle influence progressivement :

- sprint ;
- vitesse ;
- précision ;
- résistance aux contacts.

Elle doit justifier les remplacements sans transformer le jeu en simulation de gestion.

La jauge de fatigue n'a pas besoin d'être affichée en permanence.

---

# 31. Remplacements

Changements autorisés :

- à la mi-temps ;
- après un point ;
- depuis le menu Pause si le ballon n'est pas en action.

Pour favoriser le fun :

**changements illimités** dans les règles standard.

Le joueur peut donc adapter sa tactique sans compter des substitutions.

---

# 32. Contrôles SNES — proposition

Les commandes doivent rester simples et contextuelles.

## D-PAD
Déplacement.

## B — PASS
- appui : passe à la main ;
- direction + B : choix de la cible/direction.

## X — KICK / LONG PASS
- frappe au pied ;
- passe longue ;
- dégagement selon la situation.

## Y — SHOOT / ACTION
- tir vers l'anneau ;
- action offensive contextuelle.

## A — CHARGE / TACKLE
Sans ballon :
- charge ;
- interception.

Avec ballon :
- protection / esquive courte contextuelle.

## L — SWITCH PLAYER
Changer de joueur contrôlé.

## R — SPRINT
Accélération avec consommation d'endurance.

## START
Pause.

Les boutons pourront être réaffectables dans OPTIONS si la place ROM/UI le permet.

---

# 33. Contrôle contextuel

Le jeu peut sélectionner automatiquement une animation adaptée :

- réception ;
- reprise ;
- volée ;
- tir en extension ;
- interception ;
- petit saut ;
- plongeon du gardien.

Le joueur ne doit pas avoir besoin de mémoriser des combinaisons complexes.

---

# 34. Sauts et jeu aérien

Le jeu aérien existe mais ne doit pas devenir un jeu de plateformes.

Les joueurs peuvent :

- sauter pour intercepter ;
- reprendre un ballon haut ;
- tirer en l'air ;
- capter une passe haute.

Le saut peut être déclenché automatiquement dans certaines situations ou via Y selon le contexte.

Les hauteurs restent modestes pour conserver la lisibilité.

---

# 35. Physique du ballon

Le ballon possède :

- position X/Y ;
- hauteur Z logique ;
- vitesse X/Y ;
- vitesse verticale ;
- friction ;
- rebond ;
- puissance ;
- propriétaire éventuel.

Le moteur utilise des nombres fixed-point.

Le ballon doit pouvoir :

- rouler ;
- glisser ;
- voler ;
- rebondir sur le sol ;
- rebondir sur les murs ;
- traverser l'anneau ;
- être repoussé par le gardien ;
- être dévié par un joueur.

---

# 36. Jeu contre les murs

Le mur est une mécanique importante, pas seulement une limite graphique.

Il doit être possible de :

- passer contre le mur ;
- anticiper un rebond ;
- contourner une ligne défensive ;
- créer un une-deux avec soi-même ;
- tirer avec rebond.

Les angles doivent être suffisamment cohérents pour que les joueurs apprennent à les maîtriser.

---

# 37. Gardien

Le gardien est majoritairement contrôlé par l'IA.

Il doit :

- suivre la position du ballon ;
- se recentrer ;
- anticiper les tirs ;
- sortir légèrement ;
- plonger ;
- capter ;
- repousser ;
- relancer.

Dans sa petite zone :

- il peut utiliser librement ses mains ;
- il bénéficie d'une protection contre les charges.

Le gardien ne doit pas être invincible.

---

# 38. Changement de joueur

En phase défensive :

- sélection automatique du joueur le plus utile ;
- L permet de forcer le changement.

La sélection doit tenir compte :

- distance au ballon ;
- trajectoire ;
- direction de l'attaque ;
- rôle tactique.

Un curseur clair indique le joueur contrôlé.

---

# 39. Intelligence artificielle

L'IA doit donner l'impression d'une équipe organisée.

Chaque joueur IA possède un état simple, par exemple :

- HOLD_POSITION
- SUPPORT
- ATTACK_SPACE
- MARK
- PRESS
- INTERCEPT
- RECEIVE
- SHOOT
- RETURN_DEFENSE
- GOALKEEP

Les décisions majeures n'ont pas besoin d'être recalculées à chaque frame.

---

# 40. IA et difficulté

Niveaux :

- EASY
- NORMAL
- HARD
- EXPERT

La difficulté agit sur :

- anticipation ;
- délai de réaction ;
- choix de passe ;
- précision tactique ;
- pressing ;
- placement.

Ne jamais tricher en donnant au CPU :

- vitesse impossible ;
- tirs automatiquement réussis ;
- collisions favorisées ;
- endurance infinie.

---

# 41. Mini-radar

Le radar est placé dans une zone discrète de l'écran.

Il affiche :

- équipe 1 ;
- équipe 2 ;
- gardiens ;
- ballon.

Option :

- RADAR ON
- RADAR OFF

Le réglage doit être sauvegardé en SRAM.

Le radar doit rester optionnel : le jeu doit être jouable sans lui.

---

# 42. Caméra

La caméra suit une cible légèrement anticipée devant le ballon.

Elle ne doit pas suivre brutalement chaque rebond.

Caractéristiques :

- scrolling fluide ;
- anticipation dans le sens de l'action ;
- limitation aux bords du terrain ;
- priorité à la lisibilité.

Le joueur doit pouvoir voir suffisamment loin devant lui pour préparer une passe.

---

# 43. Modes de jeu

Menu principal final recommandé :

```text
EXHIBITION
CHAMPIONSHIP
CUP
CUSTOM COMPETITION
CREATE TEAM
CREATE PLAYER
OPTIONS
CREDITS
```

Aucun menu Career.

---

# 44. Exhibition

Match immédiat.

Choix :

- teams ;
- human / CPU ;
- stadium ;
- match length ;
- difficulty ;
- formation ;
- tactics.

Configurations :

- Player 1 vs CPU ;
- Player 1 vs Player 2 ;
- CPU vs CPU pour test / spectacle si retenu.

---

# 45. Championship

Championnat de type ligue.

Fonctions :

- choix des équipes ;
- calendrier ;
- classement ;
- résultats ;
- sauvegarde ;
- reprise ;
- écran de champion.

Barème par défaut proposé :

- WIN : 3 pts
- DRAW : 1 pt
- LOSS : 0 pt

Critères de départage :

1. points ;
2. score difference ;
3. points scored ;
4. direct match si nécessaire.

---

# 46. Cup

Format élimination directe.

Formats supportés :

- 4 teams ;
- 8 teams ;
- 16 teams.

Gestion :

- bracket ;
- overtime ;
- shootout ;
- sauvegarde entre les tours ;
- finale dans un stade spécial possible.

---

# 47. Custom Competition

Le joueur peut créer une compétition.

Types :

- LEAGUE
- CUP

Paramètres :

- competition name ;
- number of teams ;
- participating teams ;
- stadium selection ;
- match duration ;
- difficulty ;
- human / CPU assignment.

---

# 48. Affectation des équipes

Pour chaque équipe :

- CPU
- PLAYER 1
- PLAYER 2

Exemple :

```text
ORION STARS       PLAYER 1
TITAN CRUSHERS    CPU
VEGA STRIKERS     PLAYER 2
MARS UNITED       CPU
SOLARIS FALCONS   PLAYER 1
```

Il est autorisé qu'un même joueur humain possède plusieurs équipes dans une compétition.

Si deux équipes appartenant au même contrôleur doivent s'affronter :

- le jeu demande quelle équipe le contrôleur veut jouer ;
- l'autre est temporairement contrôlée par le CPU.

Si PLAYER 1 rencontre PLAYER 2 :

- match local deux joueurs normal.

---

# 49. Matchs CPU vs CPU

Dans une compétition :

- possibilité de simuler instantanément un match CPU vs CPU ;
- option WATCH CPU MATCH peut être ajoutée si elle reste simple.

La simulation rapide utilise :

- attaque globale ;
- défense globale ;
- gardien ;
- tactique ;
- petite variation aléatoire.

Elle ne doit pas produire des scores absurdes.

---

# 50. Création de joueur

Menu CREATE PLAYER.

Paramètres :

- NAME
- NUMBER
- POSITION
- SECONDARY POSITION
- APPEARANCE
- SKIN TONE
- HAIR
- HAIR COLOR
- KIT NUMBER
- SPEED
- POWER
- PASS
- KICK
- CONTROL
- DEFENSE
- STAMINA

Pour un gardien :

- REFLEX
- CATCH
- POSITION
- THROW

Le système d'apparence utilise des éléments graphiques prédéfinis compatibles avec la ROM.

---

# 51. Équilibrage des joueurs créés

Pour éviter les joueurs 9 partout :

- budget total de points ;
- coût supérieur pour les notes 8 et 9 ;
- ou limite du nombre de caractéristiques à 9.

Une option FREE EDIT pourra éventuellement être ajoutée, mais le mode équilibré est le comportement standard.

---

# 52. Création d'équipe

Menu CREATE TEAM.

Paramètres :

- TEAM NAME
- SHORT NAME
- HOME WORLD
- PRIMARY COLOR
- SECONDARY COLOR
- EMBLEM
- HOME KIT
- AWAY KIT
- ROSTER
- DEFAULT FORMATION
- DEFAULT TACTICS

L'utilisateur peut :

- utiliser des joueurs créés ;
- éventuellement copier puis modifier un joueur générique.

---

# 53. Équipes personnalisées

Objectif recommandé :

**jusqu'à 8 équipes personnalisées** en SRAM.

Ce nombre pourra être augmenté selon la taille SRAM définitive.

Chaque équipe personnalisée possède 12 joueurs.

---

# 54. SRAM

Cible initiale recommandée :

**32 KiB SRAM minimum.**

Passage à **64 KiB** autorisé si nécessaire.

Données sauvegardées :

- options ;
- joueurs créés ;
- équipes créées ;
- Championship en cours ;
- Cup en cours ;
- Custom Competition ;
- affectations human/CPU ;
- classements ;
- résultats ;
- radar ;
- contrôles.

Chaque bloc doit contenir :

- version ;
- longueur ;
- checksum.

En cas de sauvegarde invalide :

- conserver les autres slots valides si possible ;
- proposer CLEAR / RESET uniquement pour le bloc corrompu.

---

# 55. ROM

Le projet peut utiliser une grosse cartouche.

Cible recommandée de départ :

**4 MiB HiROM.**

Si le contenu final l'exige :

- passage à ExHiROM jusqu'à 6–8 MiB possible.

Ne pas augmenter artificiellement la ROM si le contenu tient dans moins.

Le code doit séparer clairement :

- executable code ;
- player/team data ;
- graphics ;
- stadium data ;
- music ;
- SFX.

---

# 56. Pas de coprocesseur requis

La première cible doit fonctionner sur une cartouche SNES standard :

- pas de Super FX obligatoire ;
- pas de SA-1 obligatoire ;
- pas de DSP obligatoire.

Un coprocesseur ne doit être envisagé que si une raison technique majeure apparaît plus tard.

Le moteur de base doit exploiter correctement le 65C816 et le PPU.

---

# 57. Résolution et vidéo

Résolution de jeu recommandée :

**256 × 224**

sur PAL et NTSC.

Avantages :

- même layout ;
- même HUD ;
- même logique de caméra ;
- développement simplifié.

Le jeu détecte PAL/NTSC au démarrage.

---

# 58. PAL 50 Hz / NTSC 60 Hz

Le jeu ne doit pas être simplement 16,7 % plus lent en PAL.

Les systèmes à normaliser :

- mouvements ;
- vitesse du ballon ;
- accélération ;
- sprint ;
- IA ;
- animation ;
- timers ;
- match clock ;
- fatigue ;
- pénalités ;
- transitions.

Approche recommandée :

- constantes de mouvement distinctes 50/60 Hz ;
- fixed-point ;
- timers exprimés en temps logique ;
- tables d'animation adaptées.

Exemple :

un match de 4 minutes doit durer approximativement 4 minutes réelles sur les deux régions.

---

# 59. Mode graphique SNES

Mode recommandé :

**PPU Mode 1**, sauf raison technique différente démontrée.

Usage possible :

- BG1 : terrain principal ;
- BG2 : décors / éléments du stade ;
- BG3 : HUD / texte / éléments légers ;
- OBJ : joueurs, ballon, curseurs, effets.

Le HUD peut également employer une zone fixe selon la conception finale.

---

# 60. Sprites joueurs

Les joueurs doivent être visuellement plus riches que de simples petits bonshommes.

Objectif :

- silhouette lisible ;
- couleur d'équipe évidente ;
- numéro lorsque possible ;
- animation claire ;
- ombre au sol discrète.

Animations minimum :

- idle ;
- run ;
- sprint ;
- receive ;
- carry ;
- hand pass ;
- kick pass ;
- shoot ;
- charge ;
- stumble ;
- fall ;
- get up ;
- jump ;
- aerial action ;
- celebrate.

Gardien :

- idle ;
- shuffle ;
- catch ;
- dive left ;
- dive right ;
- punch / deflect ;
- throw ;
- kick release.

---

# 61. Limites OBJ

Le moteur doit tenir compte des limites réelles de la SNES :

- 128 OBJ maximum à l'écran ;
- limite de sprites / tiles par scanline ;
- risques de disparition de sprites.

Avec 12 joueurs + ballon + curseurs :

- limiter le nombre de sous-sprites par joueur ;
- trier et planifier OAM ;
- éviter les compositions trop larges ;
- tester les regroupements de joueurs sur la même ligne.

Le jeu ne doit pas dépendre d'un comportement d'émulateur permissif.

---

# 62. Palette et lisibilité

Le terrain ne doit pas être trop lumineux.

Les joueurs doivent se détacher immédiatement.

Règles :

- une équipe chaude, l'autre froide lorsque possible ;
- kits alternatifs en cas de couleurs proches ;
- ballon très contrasté ;
- curseur de joueur visible ;
- anneaux de but lumineux mais pas au point de masquer les sprites.

---

# 63. Stades

Minimum recommandé pour la version complète :

**6 stades.**

Exemples :

### ORBITAL ARENA
Station spatiale, planète visible.

### MARS DOME
Teintes rouges et structure sous dôme.

### EUROPA ICE
Bleu froid, glace au-delà des tribunes.

### ANDROMEDA PRIME
Métropole technologique.

### TITAN INDUSTRIAL
Structures métalliques lourdes.

### SOLARIS ARENA
Ambiance lumineuse et solaire.

Les stades modifient :

- graphismes ;
- palettes ;
- public ;
- musique / ambiance éventuelle.

Ils ne changent pas les caractéristiques physiques officielles du terrain.

---

# 64. Public et ambiance

Le public doit donner de la vie au match sans consommer trop de CPU/VRAM.

Techniques possibles :

- animations de tiles ;
- palette cycling ;
- petits groupes animés ;
- réactions déclenchées sur événements.

Réactions :

- tir dangereux ;
- charge ;
- arrêt du gardien ;
- point marqué ;
- fin de match ;
- victoire.

---

# 65. Audio

Le SPC700 doit produire :

- musiques de menus ;
- musique / ambiance de stade ;
- crowd loop ;
- impacts ;
- frappes ;
- rebonds ;
- sifflet/signal ;
- confirmation menus ;
- but ;
- victoire.

Réserver suffisamment de canaux audio pour que les bruitages importants restent audibles pendant la musique.

Éviter une musique trop chargée qui étouffe les sons du match.

---

# 66. HUD

HUD compact.

Informations :

- team names / abbreviations ;
- score ;
- match time ;
- half ;
- éventuellement nom/numéro du joueur contrôlé ;
- mini-radar si activé.

Ne pas couvrir une grande partie du terrain.

---

# 67. Écran pré-match

Avant le match :

```text
ORION STARS
VS
TITAN CRUSHERS

ORBITAL ARENA
```

Puis accès rapide à :

- LINEUP
- FORMATION
- TACTICS
- START MATCH

Le joueur doit pouvoir passer immédiatement au match.

---

# 68. Menu Pause

```text
RESUME
FORMATION
TACTICS
SUBSTITUTION
RADAR
OPTIONS
QUIT MATCH
```

Les menus doivent être rapides, avec peu de sous-niveaux.

---

# 69. Présentation et rythme

Le jeu doit avoir une bonne ambiance sans ralentir les matchs.

Animations optionnelles :

- entrée des équipes ;
- logo de compétition ;
- présentation du stade ;
- célébration ;
- trophée.

Toutes les scènes non essentielles doivent pouvoir être passées.

---

# 70. Langue

Le jeu complet est en **anglais**.

Cela inclut :

- menus ;
- aides ;
- tactiques ;
- options ;
- éditeurs ;
- crédits ;
- messages de sauvegarde.

Le cahier des charges peut rester en français.

---

# 71. Crédits

Écran CREDITS obligatoire.

Base proposée :

```text
NEXUS BALL

AN OFFGAME PROJECT

GAME DESIGN
JEAN MONOS

PROJECT DIRECTION
JEAN MONOS / OFFGAME

DESIGN & DEVELOPMENT ASSISTANCE
CHATGPT — OPENAI
ÉLÉA — OPENAI

SUPER NINTENDO VERSION
OFFGAME

SPECIAL THANKS
[TO BE COMPLETED]
```

La liste doit pouvoir être complétée avec les futurs contributeurs.

---

# 72. Architecture du projet assembleur

Structure recommandée :

```text
/
  build/
  docs/
  src/
    main.asm
    boot.asm
    system.asm
    nmi.asm
    irq.asm
    video.asm
    dma.asm
    input.asm
    camera.asm
    match.asm
    rules.asm
    ball.asm
    player.asm
    goalkeeper.asm
    collision.asm
    physics.asm
    ai.asm
    tactics.asm
    formation.asm
    referee.asm
    hud.asm
    radar.asm
    menu.asm
    competition.asm
    championship.asm
    cup.asm
    custom_comp.asm
    editor_player.asm
    editor_team.asm
    save.asm
    audio.asm

  include/
    snes_regs.inc
    constants.inc
    macros.inc
    memory.inc
    structs.inc

  data/
    teams/
    players/
    stadiums/
    graphics/
    palettes/
    fonts/
    music/
    sfx/

  tools/
  build.bat
  build.sh
  README.md
  CHANGELOG.md
```

Le projet doit rester facile à reprendre.

---

# 73. Build

Le build doit :

1. assembler toutes les sources ;
2. générer la ROM ;
3. générer les headers/checksums nécessaires ;
4. produire une ROM `.sfc` directement testable ;
5. échouer clairement en cas d'erreur.

Ne pas considérer un fichier source non assemblé comme une livraison fonctionnelle.

Chaque étape importante doit être réellement compilée.

---

# 74. Assembleur

Le choix définitif peut être :

- WLA-DX ;
- ca65 ;
- Asar ;
- autre assembleur SNES sérieux.

Conditions :

- support des banques SNES ;
- includes ;
- macros ;
- labels ;
- génération reproductible ;
- build automatisable.

Une fois choisi, éviter de changer d'assembleur sans nécessité.

---

# 75. Boucle principale

Architecture générale recommandée :

```text
MAIN:
    wait_for_frame
    read_controllers
    update_match_clock
    update_human_player
    update_ball
    update_collisions
    update_ai_group
    update_goalkeepers
    update_rules
    update_camera
    prepare_oam
    prepare_vram_jobs
    goto MAIN

NMI:
    transfer_oam
    transfer_vram_jobs
    update_scroll
    update_palette_jobs
    acknowledge_nmi
    rti
```

Le NMI doit rester court et prévisible.

---

# 76. DMA

Utiliser DMA pour :

- OAM ;
- tiles ;
- tilemaps ;
- palettes ;
- chargements d'écran.

Éviter les gros transferts VRAM hors VBlank.

Prévoir une file de petits jobs VRAM pour les mises à jour dynamiques.

---

# 77. Coordonnées et physique

Utiliser des coordonnées monde fixed-point.

Exemple conceptuel :

- X : 16 bits entier + fraction ;
- Y : 16 bits entier + fraction ;
- Z : hauteur logique ;
- VX/VY/VZ : vitesses fixed-point.

Ne pas utiliser de flottants.

---

# 78. Collisions

Séparer :

- player/player ;
- player/ball ;
- ball/wall ;
- ball/ring ;
- goalkeeper/ball.

Utiliser :

- boîtes simples ;
- rayons ;
- distances approximées ;
- tables précalculées si nécessaire.

Le gameplay compte davantage que la précision géométrique absolue.

---

# 79. Budget CPU de l'IA

Tous les joueurs IA ne doivent pas recalculer une décision lourde à chaque frame.

Approche :

- mouvement simple chaque frame ;
- décision tactique à fréquence réduite ;
- groupes d'IA alternés ;
- mise à jour immédiate lors d'un événement critique.

Exemple :

- frame N : defenders ;
- frame N+1 : midfielders ;
- frame N+2 : forwards ;
- goalkeeper : fréquence plus élevée.

---

# 80. Données pilotées par tables

Les données suivantes doivent être séparées du code :

- équipes ;
- joueurs ;
- formations ;
- tactiques ;
- stades ;
- palettes ;
- textes.

Objectif :

permettre de modifier le contenu sans réécrire le moteur.

---

# 81. Sauvegarde sûre

SRAM :

- signature ;
- version ;
- checksum ;
- slots indépendants ;
- valeurs par défaut.

Ne jamais écrire toute la SRAM à chaque frame.

Écrire uniquement :

- sur validation ;
- fin de match ;
- sauvegarde volontaire ;
- modification d'équipe/joueur.

---

# 82. Écrans de création

Les éditeurs doivent être conçus pour une manette.

Éviter de faire saisir trop de texte.

Pour les noms :

- clavier virtuel ;
- A valide ;
- B efface ;
- START termine.

Prévoir une limite raisonnable :

- Player Name : environ 10–12 caractères ;
- Team Name : environ 12–16 caractères ;
- Short Name : 3 caractères.

---

# 83. Police et textes

Police lisible en basse résolution.

Prévoir :

- capitales ;
- minuscules si nécessaire ;
- chiffres ;
- ponctuation ;
- symboles tactiques.

Les menus ne doivent jamais dépendre de textes minuscules illisibles.

---

# 84. Gestion des couleurs d'équipe

L'éditeur d'équipe ne peut pas permettre 32768 couleurs arbitraires si cela complique les palettes.

Préférer une palette de couleurs prédéfinies compatibles avec :

- sprites ;
- kits ;
- contrastes ;
- contraintes CGRAM.

Le joueur choisit :

- primary color ;
- secondary color.

Le moteur génère les variantes de maillot parmi des templates.

---

# 85. Apparence des joueurs créés

Utiliser des composants prédéfinis :

- plusieurs tons de peau ;
- coiffures ;
- couleurs de cheveux ;
- éventuellement accessoires sportifs simples.

Ne pas créer un éditeur graphique libre qui consommerait trop de ROM/SRAM.

---

# 86. Tests matériel

Le jeu doit être testé sur :

- émulateur précis ;
- NTSC ;
- PAL ;
- matériel réel dès que possible.

Points à tester particulièrement :

- scanline sprite limits ;
- OAM ;
- DMA ;
- timing NMI ;
- SRAM ;
- input ;
- PAL/NTSC ;
- audio ;
- longues sessions.

---

# 87. Critères de performance

Objectif :

- gameplay stable à 60 Hz NTSC ;
- gameplay stable à 50 Hz PAL ;
- pas de ralentissement permanent ;
- pas de disparition massive de sprites ;
- réponse manette immédiate ;
- scrolling stable ;
- musique sans coupure.

Des frames ponctuellement lourdes doivent être évitées plutôt que masquées.

---

# 88. Pas de fausse version SNES

Le projet final ne doit pas être :

- un jeu HTML ressemblant à la SNES ;
- un programme PC avec skin SNES ;
- une vidéo ;
- une maquette non compilable.

Il doit s'agir d'un **vrai programme 65C816 produisant une ROM Super Nintendo fonctionnelle**.

---

# 89. Méthode de développement demandée

Travailler progressivement.

À chaque étape :

1. partir de la dernière build fonctionnelle ;
2. faire une modification cohérente ;
3. compiler ;
4. corriger les erreurs ;
5. vérifier que la ROM boote ;
6. conserver un changelog.

Ne pas attendre des dizaines de fonctionnalités avant de recompiler.

---

# 90. Milestone 0 — Boot technique

Livrable :

- ROM bootable ;
- écran stable ;
- détection PAL/NTSC ;
- NMI ;
- contrôleurs ;
- affichage de texte ;
- build automatique.

---

# 91. Milestone 1 — Terrain

Livrable :

- un stade ;
- terrain scrollable ;
- caméra ;
- un sprite joueur ;
- déplacements 8 directions ;
- 50/60 Hz corrects.

---

# 92. Milestone 2 — Ballon

Livrable :

- ballon ;
- prise de balle ;
- passe ;
- frappe ;
- rebond murs ;
- collisions ;
- anneau de but ;
- détection score.

Cette étape doit déjà être agréable à manipuler.

---

# 93. Milestone 3 — Match minimal

Livrable :

- 6 vs 6 ;
- une équipe humaine ;
- une équipe IA ;
- gardiens ;
- score ;
- horloge ;
- engagement ;
- changement de joueur.

---

# 94. Milestone 4 — Règles complètes

Ajouter :

- port 4 secondes ;
- passes à la main ;
- passes au pied ;
- faute ;
- charge ;
- avantage ;
- overtime ;
- shootout.

Les règles doivent être testées pour vérifier qu'elles produisent réellement un jeu amusant.

---

# 95. Milestone 5 — Tactique

Ajouter :

- 6 formations ;
- tactiques ;
- postes ;
- caractéristiques ;
- fatigue ;
- remplacements.

---

# 96. Milestone 6 — Présentation

Ajouter :

- HUD ;
- radar ;
- public ;
- animations ;
- bruitages ;
- musique ;
- plusieurs stades ;
- écrans avant/après match.

---

# 97. Milestone 7 — Modes

Ajouter :

- Exhibition ;
- Championship ;
- Cup ;
- Custom Competition ;
- save/resume.

---

# 98. Milestone 8 — Création

Ajouter :

- Create Player ;
- Create Team ;
- sauvegarde SRAM ;
- équipes custom utilisables dans les compétitions.

---

# 99. Milestone 9 — Finition

- équilibrage ;
- graphismes finaux ;
- animations supplémentaires ;
- IA ;
- optimisation ;
- tests PAL ;
- tests NTSC ;
- tests SRAM ;
- tests matériel réel ;
- crédits ;
- correction finale.

---

# 100. Prototype prioritaire

Le prototype ne doit pas commencer par les menus, la coupe ou l'éditeur.

La première vraie question est :

> **Est-ce que le sport est amusant avec une manette SNES ?**

Prototype prioritaire :

- 1 terrain ;
- 2 équipes ;
- graphismes temporaires mais lisibles ;
- 6 vs 6 ;
- ballon ;
- main + pied ;
- charge ;
- murs ;
- anneaux ;
- score ;
- gardien ;
- IA simple.

Une fois le gameplay validé, seulement ensuite développer les gros modes.

---

# 101. Ce qu'il ne faut pas développer

Hors périmètre :

- Career Mode ;
- transferts ;
- salaires ;
- contrats ;
- finances ;
- entraînement RPG ;
- progression de joueur sur plusieurs saisons ;
- monde ouvert ;
- scénario long ;
- réseau ;
- DLC ;
- compte utilisateur ;
- système de connexion ;
- statistiques géantes.

Le jeu doit rester un **jeu de sport complet et fun**.

---

# 102. Objectif final

Le joueur doit pouvoir :

1. allumer la Super Nintendo ;
2. lancer un match en quelques secondes ;
3. comprendre rapidement les règles ;
4. jouer avec les pieds et les mains ;
5. utiliser les murs ;
6. faire circuler le ballon ;
7. construire une attaque ;
8. charger un adversaire ;
9. changer de tactique ;
10. marquer dans l'anneau ;
11. demander immédiatement une revanche.

Le jeu doit avoir suffisamment de profondeur pour que deux bons joueurs développent :

- leurs formations préférées ;
- leurs tactiques ;
- leurs équipes ;
- leur façon d'utiliser les murs ;
- leurs stratégies de score à 1 ou 2 points.

---

# 103. Résumé très court pour le développeur

**NEXUS BALL** est un jeu de sport futuriste original pour Super Nintendo, héritier du football et du rugby.

- véritable ROM SNES ;
- assembleur 65C816 ;
- PAL + NTSC ;
- 5 joueurs de champ + 1 gardien par équipe ;
- 12 joueurs par effectif ;
- ballon jouable à la main et au pied ;
- port limité ;
- passe manuelle vers l'avant interdite dans la règle proposée ;
- tirs dans un anneau ;
- 1 point à courte portée ;
- 2 points pour une frappe longue au pied ;
- contacts physiques ;
- murs actifs ;
- formations ;
- tactiques ;
- IA ;
- 1 ou 2 joueurs ;
- Championship ;
- Cup ;
- Custom Competition ;
- équipes assignables à P1 / P2 / CPU ;
- création de joueurs ;
- création d'équipes ;
- SRAM ;
- plusieurs stades ;
- mini-radar ON/OFF ;
- jeu en anglais ;
- aucun mode carrière ;
- aucune fonction en ligne ;
- graphismes et audio dignes de la Super Nintendo.

**Projet : OFFGAME**  
**Direction : Jean Monos**  
**Assistance conception/développement : ChatGPT / OpenAI — Éléa / OpenAI**
