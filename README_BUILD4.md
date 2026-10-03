# France Foot 2D — Super Soccer World 00.03.00 build 4

## Jouer

Extraire le ZIP Win64 puis lancer `FranceFoot2D.exe`. La bibliothèque graphique est intégrée ; aucune installation de raylib n'est nécessaire. Les sauvegardes sont créées à côté du programme, dans `saves`.

## Postes et rôles : intégration du document fourni

Le même moteur physique et le même rendu servent tous les modes. Les matchs rapides, championnats sans gestion, tournois et coupes utilisent le profil simple. Les carrières de club, de joueur et de sélectionneur utilisent le profil carrière.

Le mode simple conserve les quatre familles gardien/défenseur/milieu/attaquant. Les rôles, tâches, familiarités et consignes individuelles ne modifient ni le placement ni les qualités des joueurs dans ce profil. Les consignes collectives accessibles sont la formation, la mentalité, le pressing, la largeur et les passes. Le moral et les contrats sont masqués dans l'effectif et la fiche.

La carrière ajoute :

- Quinze postes : GB, DD, DC, DG, PIS D/G, MDC, MC, MD/MG, MOC, AD/AG, SA et BU.
- Poste naturel, pied préféré et familiarités de 0 à 100, distincts du poste de l'emplacement dans la formation.
- Rôles spécifiques aux postes, tâches défense/soutien/attaque et dix-sept consignes individuelles.
- Effets physiques sur la hauteur du placement, la largeur, le décrochage, la couverture, le pressing, les tirs, les centres, la conservation et les choix de passes/dribbles. Le demi-centre redescend ; le faux neuf décroche ; l'attaquant intérieur se rapproche de l'axe. Les instructions alimentent les paramètres du moteur existant.
- Deux modèles de jeu : possession et jeu direct compact. Les entraîneurs adverses emploient des rôles et consignes collectives cohérents avec leur modèle.
- Aptitudes de rôle calculées depuis les caractéristiques du joueur ; aucune note de rôle indépendante n'est sauvegardée.
- Apprentissage lié à l'âge, au placement, au staff, à la proximité des postes, aux séances et aux minutes réellement suivies lors d'un match joué. En simulation, les minutes sont estimées pour les titulaires (40, 90 ou 120 selon le match).
- Travail individuel d'un poste et développement d'un rôle depuis la fiche et l'entraînement.
- Mercato filtrable par poste détaillé et tri par aptitude/familiarité, avec comparaison au meilleur joueur de votre effectif pour ce rôle.

### Accès

Dans **Effectif → Consignes / adjoint → Postes / rôles individuels**, sélectionner un emplacement, puis son poste détaillé (dans la même famille), le rôle, la tâche et les consignes. Les flèches parcourent les emplacements et les consignes ; les boutons changent les valeurs. Pour le marquage individuel, choisir une cible parmi les prochains adversaires.

Dans une **fiche joueur**, `Tab` affiche les postes, familiarités et aptitudes. Pour un joueur de votre équipe, choisir le poste à apprendre et le rôle à développer. Le bouton dédié de l'écran entraînement ouvre cette même fiche. Dans le mercato, `F` ouvre la fiche du candidat.

## Changements conservés de la build 3

Cérémonie des récompenses du championnat en fin de saison ; création différée des réserves et sections jeunes ; seize Pôles Espoirs avec joueurs générés de 13–14 ans, tournoi fictif de 2 × 20 minutes et recrutement U15 ; nouveau club placé dans la dernière division de son district ; première saison sans exemption de coupe ; plusieurs saisons en championnat simple.

Les corrections précédentes des dates des Coupes du monde, du barème 1930, des pays sélectionnés, de la DNCG française et des trois coupes de la Marne sont conservées. Le remerciement à Thorn Atari reste dans les crédits.

## Limites explicites

Ce lot introduit les règles et interfaces tactiques demandées. Les modèles des entraîneurs restent deux profils déterministes ; ils ne constituent pas encore un système complet de personnalités ou d'analyse adaptative des adversaires. Le développement individuel de rôle travaille une caractéristique principale adaptée au poste ; il ne dispose pas encore d'un calendrier détaillé de séances personnalisées. L'estimation des minutes des matchs simulés ne reconstitue pas leurs remplacements. La comparaison de recrutement est directe et ne simule pas l'incertitude d'un rapport de scout.

La saisie officielle **de toute la France 2026/27 n'est pas terminée**. Le Grand Est et ses neuf districts déjà intégrés sont conservés. La couverture livrée compte 3 761 équipes et 327 groupes officiels ; d'autres championnats restent générés. Les listes des Pôles sont fictives, les structures correspondent aux sources documentées dans la build 3. Le tournoi des Pôles est une compétition du jeu demandée par l'utilisateur.

## Sauvegardes et sources

Format de sauvegarde 28 pour les familiarités, l'apprentissage et les consignes. Les formats précédemment pris en charge restent lisibles. Le document de spécification fourni est inclus dans `docs` sans modification. Les logs de compilation, tests et captures sont joints dans `verification`.

Le script `build-win64.ps1` compile avec LLVM-MinGW et raylib 5.5. Utiliser `-Compiler <clang++.exe> -Raylib <dossier raylib> -Tests` pour reconstruire et lancer les suites de validation. Voir `CREDITS.md` pour les technologies et licences.

## Validation de cette livraison

Compilation Windows x86_64 réussie. Validation nationale et contrôle des 3 761 équipes importées réussis ; 6 492 assertions des suites officiels, pays, Ballon d'or, corrections build 2, fonctionnalités build 3 et tactique build 4 réussies. Quinze écrans ont été lancés et capturés, dont tactique détaillée, fiche des postes, recrutement et tactique simple. Les tests incluent 120 mises à jour du moteur physique pour le suivi des minutes et des comparaisons de placement ; ils ne remplacent pas une longue partie jouée par un humain. Les imports de l'EXE ne demandent que les composants Windows/UCRT.
