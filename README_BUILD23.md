# France Foot 2D / Super Soccer World — 00.03.00 build 23 : médias, supportrices, Coupe des Continents

Le téléchargement contient `FranceFoot2D_v00.03.00_build23` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build23_Source` (code source complet).

Les sauvegardes passent en version 36. Celles des builds précédentes se chargent toujours.

## Match télévisé : la caméra filme les supportrices
- **Nouveaux plans en tribune** dans les incrustations pendant les arrêts de jeu :
  - « FAN CAM » : gros plan sur une supportrice ;
  - « LES SUPPORTRICES » : deux supportrices et un drapeau du club.
- **Fréquence** : en match télévisé, environ un plan sur trois.
- **Après un but**, la caméra cherche une supportrice du club buteur qui fête le but.
- **Look** :
  - coiffure (longs, queue de cheval, boucles, tresses) ;
  - maquillage et écharpe aux couleurs du club ;
  - réaction selon le score : elle chante et salue, fait un clin d'œil, envoie un baiser, ou se tient les joues quand son équipe est menée ;
  - viseur de caméra et voyant « REC ».

## Présentateurs et présentatrices
- **Plateau TV** : 12 présentateurs et présentatrices. Un est tiré au hasard à chaque match.
- **Consultant** : environ une fois sur trois, il est remplacé par une consultante ou un autre consultant.
- **Affichage** :
  - le plateau et la cabine montrent les intervenants en buste, avec leur visage ;
  - la bouche de la personne qui parle s'anime ;
  - le rôle affiché s'accorde (Présentatrice / Présentateur, Consultante / Consultant).

## Carrière : invitations à la TV ou à la radio
- **Fréquence** : une ou deux fois par saison.
  - La première invitation arrive à l'automne.
  - La seconde arrive au printemps si le club fait parler de lui (élite et 2e division, haut ou bas de classement, ou au hasard).
- **Le média dépend de la division** :
  - émission nationale pour l'élite ;
  - chaîne de la 2e division ;
  - télévision régionale pour les 3e et 4e divisions ;
  - radio locale en dessous.
- **Format** : une émission avec un plateau, ou une interview. Le décor est un plateau TV ou un studio de radio (voyant ON AIR, console, micros).
- **Questions** : trois, choisies selon le contexte (en tête, en difficulté, objectif, vestiaire, mercato, jeunes, derby, et une question légère pour finir).
- **Réponses** : trois tons (prudent, confiant, audacieux). Chacune a un effet sur la confiance du président, les supporters et le moral du vestiaire.
- **Fin de l'émission** : bilan des effets, cachet versé au club, et une ligne dans les actualités.

## Carrière : coupures de journal
- Quand une coupe se termine, une coupure de journal annonce le vainqueur :
  - coupe nationale de votre pays ;
  - Coupe de la Ligue ;
  - coupes nationales d'Angleterre, d'Espagne, d'Italie, d'Allemagne et du Portugal ;
  - Ligue des champions, Ligue Europa / Coupe UEFA et Ligue Conférence.
- **Mise en page** : papier jauni aux bords déchirés, punaisé sur un panneau de liège, avec :
  - gros titre ;
  - photo des vainqueurs avec le trophée ;
  - score de la finale (prolongation, tirs au but) ;
  - un texte différent si votre club gagne ou perd la finale.
- Chaque coupure n'est montrée qu'une fois (sauvegardé).

## Mode International : Coupe des Continents
- Nouveau mode dans **International > Coupe des Continents**.
- **Les équipes** : chaque confédération (UEFA, CONMEBOL, CONCACAF, CAF, AFC, OFC) envoie une sélection de ses meilleurs joueurs :
  - 23 joueurs : 3 gardiens, 8 défenseurs, 7 milieux, 5 attaquants ;
  - au plus 6 joueurs par nation ;
  - un maillot propre à chaque continent.
- **Phase de groupes** : un groupe unique de 6, en matchs aller-retour (10 journées, à domicile et à l'extérieur).
- **Final Four** : les 4 premiers jouent des demi-finales sèches (1er contre 4e, 2e contre 3e), puis une finale.
- **Contrôle** : 1 à 4 sélections contrôlées.
- Pas d'hymne national pour ces sélections.

## Vérifications
- Compilé sous Windows et sous Linux.
- Nouveau test `tools/test_continents.cpp` (25 contrôles, réussi). Il vérifie :
  - les 6 sélections (effectifs, gardiens, pas de doublon) ;
  - les 30 matchs de groupe à domicile ;
  - la sauvegarde puis le rechargement en cours de groupe ;
  - les demi-finales sèches, la finale et le vainqueur.
- Autres tests réussis :
  - test_save_integrity et test_save_legacy (sauvegarde version 34 convertie) ;
  - test_custom ;
  - test_build10_match.
- Écrans vérifiés sur captures : fan cam, plateau et cabine, invitations TV / radio, coupure de journal, Coupe des Continents.
- Les invitations médias et les coupures de journal sont désactivées pendant les tests automatiques. Elles n'ont pas été jouées sur une saison complète : je les ai vérifiées par les modes de test (FOOT_MEDIA, FOOT_CUPNEWS).
