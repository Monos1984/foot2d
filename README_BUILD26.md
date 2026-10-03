# France Foot 2D / Super Soccer World — 00.03.00 build 26 : lob, coups francs, gardien, fin de match

Le téléchargement contient `FranceFoot2D_v00.03.00_build26` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build26_Source` (code source complet).

Les sauvegardes sont inchangées.

## Lob
- **Bug corrigé** : au coup franc, le bouton Lob servait à la fois à régler l'effet et à frapper un ballon long. Le lob semblait donc bugé.
- **Lob dosé, comme la frappe** : jauge bleue au-dessus du joueur.
  - appui court : centre ou lob vers le partenaire le mieux placé (comme avant) ;
  - appui long : lob dans la direction choisie, plus fort et plus haut selon la jauge.
- **Près du but** : le lob passe par-dessus le gardien.
- **Mode classique (2 boutons)** : le bouton 2 fonctionne de la même façon.

## Coups francs
- **L2 / R2** (gâchettes ou boutons) maintenus : la flèche se courbe progressivement à gauche ou à droite. Le ballon contourne le mur.
  - Clavier 1 : K / L. Clavier 2 : A / E.
  - Le stick pendant l'élan marche toujours.
- **Lob maintenu** : ballon par-dessus le mur.
  - La jauge règle la distance et la hauteur.
  - L'effet de L2 / R2 s'ajoute.
  - Un appui très court donne un ballon long vers un partenaire.
- **Les autres boutons** :
  - Tir : frappe (appui long = puissance) ;
  - Passe : passe courte ;
  - Profondeur : ballon long.

## Gardien : plus de buts sur les frappes de loin et à bout portant
- **Allonge réduite** du gardien : beaucoup plus sur une frappe de loin (plus de 20 m), encore davantage à bout portant (moins de 9 m).
- **Bonus de l'ordinateur** : le gardien de l'ordinateur avait un bonus contre les frappes du joueur. Il ne s'applique plus qu'à mi-distance, et il est réduit.
- **Frappe puissante** de loin ou à bout portant : le gardien peut l'effleurer sans la retenir, et le ballon file au fond. Les chances dépendent de la puissance de la frappe et du niveau du gardien.
  - Commentaire : « La frappe de loin est trop puissante pour… », « À bout portant, … ne peut que l'effleurer ! ».

## L'arbitre et le ballon
- **Ballon en main** : quand l'arbitre récupère le ballon, il le porte dans la main (sous le bras), et non plus au niveau des pieds. C'est valable pour le ballon du match sur son présentoir avant le coup d'envoi et pour la fin du match.
- **Fin du match** : l'arbitre va chercher le ballon (même au fond des filets) et rentre au vestiaire avec.

## Fin du match
- **Poignées de main** : les joueurs des deux équipes se cherchent sur le terrain, en tous sens, pas en ligne. Ils se serrent la main deux à deux, deux tours avec des adversaires tirés au hasard.
- **Échanges de maillots** : environ une paire sur cinq échange les maillots. Le joueur porte ensuite le maillot de l'adversaire, et le commentateur le signale.
- **Puis** tout le monde rentre au tunnel.
- **Caméra** : rapprochée sur les poignées de main, puis vers le tunnel.
- **Durée** : la scène dure environ 18 secondes, ou moins si OK est pressé. « Homme du match » et « FIN DU MATCH » s'effacent pour la laisser voir.

## Vérifications
- Compilé sous Windows et sous Linux.
- Tests réussis :
  - test_build10_match, test_offside, test_build18, test_save_integrity ;
  - test_pens : 68 % de penalties marqués ;
  - test_gk1v1.
- Scène de fin vérifiée sur captures : paires qui se serrent la main, maillots échangés, arbitre ballon en main.
- **test_human** (matchs contre un joueur simulé) : dans un match, le joueur simulé ne sait pas tirer un penalty (il n'appuie que sur « passe »). C'est une limite du test, déjà connue.
- **Non testé** : je n'ai pas pu essayer les gâchettes L2 / R2 sur une vraie manette.
