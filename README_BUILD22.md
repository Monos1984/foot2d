# France Foot 2D / Super Soccer World — 00.03.00 build 22 : habillage TV, menus, bruitages, commandes

Le téléchargement contient `FranceFoot2D_v00.03.00_build22` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build22_Source` (code source complet).

Les sauvegardes de la build 21 se chargent sans changement.

## Bruitages
- **Menus** : petit son quand on change d'option (menu titre et toutes les listes), son de validation, son de retour, et un clic pour les réglages gauche / droite.
- **Ballon** :
  - passe : coup sourd ;
  - frappe : son plus puissant, avec le claquement du cuir ;
  - rebond plus naturel.
- **Gardien** : claquement des gants sur une prise de balle, choc sec sur une parade.
- **Tacles** : frottement sur la pelouse et choc.
- **Filet** : froissement quand le ballon entre (buts, tirs au but, entraînement).
- **Poteau** : son métallique.
- **Sifflet de l'arbitre** : sifflet à bille, avec trille et souffle (coup bref, coup long, carton, coup de sifflet final).

## Module TV
- **Présentation du match** :
  - logo de la chaîne avec reflet, voyant « EN DIRECT » qui clignote ;
  - bannières des équipes qui glissent depuis les côtés, disque « VS » ;
  - forme récente et note des équipes ;
  - compositions sur un petit terrain tactique (numéros aux couleurs du maillot), et liste qui apparaît joueur par joueur ;
  - bandeau d'infos défilant.
- **Score en match** : incrustation refaite (chrono, couleurs des clubs, score sur fond clair), qui clignote en or après un but. Le temps additionnel est affiché en vert, à côté du score.
- **Logo de la chaîne en match** : dégradé et voyant « EN DIRECT ».
- **Bandeaux au centre de l'écran** (entrée des joueurs, hymnes, hors-jeu...) : ouverture animée, liserés dorés.
- **Plateau TV** : le nom et le rôle de l'intervenant s'affichent à chaque prise de parole.
- **Publicités** : 5 nouveaux spots (Turbo Fibre, Volt Auto, Pizza Penalty, 8-Bit Energy, Banque du But).

## Fin de match
- Bandeau de résultat :
  - panneaux aux couleurs des clubs ;
  - score en grandes cases, celle du vainqueur en or ;
  - mention VICTOIRE, DÉFAITE ou NUL ;
  - étiquette « FIN DU MATCH », « APRÈS PROLONGATION » ou « TIRS AU BUT ».
- Possession et tirs en barres comparatives, aux couleurs des clubs.

## Menus
- La barre de sélection glisse d'une ligne à l'autre, avec un reflet et un curseur qui pulse.
- Boutons en dégradé.
- Fond animé : les lignes défilent lentement, avec un halo lumineux.

## Ballon
- Ballon rond en pixel art, ombré et avec reflet. Ses panneaux tournent quand il roule.
- Il paraît plus gros quand il est haut en l'air.
- Son ombre rétrécit et pâlit quand il monte.
- Traînée plus douce pour les frappes puissantes.

## Sponsors
- **Panneaux du stade** : 12 nouvelles marques imaginaires. Les panneaux changent d'un match à l'autre, avec un relief et une ombre sur le texte.
- **Sponsors maillot** : 16 nouvelles offres.
- **Panneaux LED** : nouvelles marques.

## Commandes du match
- **Après une passe**, le contrôle passe directement au receveur. Un anneau clignote sous lui pendant que le ballon voyage.
- **Changement de joueur** :
  - stick orienté : on prend le partenaire placé dans cette direction ;
  - sinon, le plus proche du ballon.
- **Mode classique (2 boutons)** : sans ballon, loin du porteur, le bouton 1 change de joueur. Près du porteur, il fait toujours un tacle.
- **Repères visuels** :
  - anneau au sol sous le joueur contrôlé ;
  - viseur de trois points dans la direction de frappe quand il a le ballon ;
  - jauge de puissance de tir plus grande (vert, jaune puis rouge), qui clignote au maximum.
- **Aide des commandes** : affichée pendant les premières secondes de jeu (avec et sans ballon) ; F1 la réaffiche.

## Vérifications
- Compilé sous Windows et sous Linux.
- Tests passés :
  - test_build10_match (match en direct) ;
  - test_gk1v1 ;
  - test_build18 ;
  - test_save_integrity.
- test_human (matchs contre un joueur simulé) : 10 buts contre 0, contre 4 à 3 avec la build 21. Le contrôle donné au receveur aide le joueur simulé.
- test_human ne termine pas le match 4 : la séance de tirs au but dépasse la durée du test. La build 21 fait de même (matchs 0 et 4) : c'est une limite du joueur simulé, pas un défaut de cette build.
- Les sons ont été générés et vérifiés dans le code, mais pas écoutés pendant les tests.
