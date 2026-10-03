# France Foot 2D / Super Soccer World — 00.03.00 build 24 : hymnes réels, compétitions perso, test des sons, vibrations

Le téléchargement contient `FranceFoot2D_v00.03.00_build24` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build24_Source` (code source complet).

Les sauvegardes sont inchangées (version 36).

## Hymnes nationaux : les vraies partitions
- **Source** : les notes ne sont plus écrites de mémoire. Elles viennent de partitions libres (Wikipédia, Wikimedia Commons, Wikisource), converties par le paquet `anthem-scores`.
- **Ce qui est joué** :
  - la mélodie ;
  - la basse et les voix intérieures quand la partition les donne ;
  - sinon, une harmonie déduite de la mélodie.
- **34 hymnes réels** :
  - Andorre, Albanie, Autriche, Australie, Belgique, Canada, Suisse, Chine, Tchéquie, Allemagne ;
  - Danemark, Espagne, Finlande, France, Angleterre / Irlande du Nord / Liechtenstein, Grèce, Croatie, Hongrie ;
  - Indonésie, Israël, Inde, Italie, Japon, Mexique, Pays-Bas, Pakistan, Pologne, Russie ;
  - Suède, Slovaquie, Saint-Marin, Ukraine, États-Unis, Afrique du Sud / Tanzanie / Zambie.
- **Limite** : les données couvrent le **début** de chaque hymne (environ 8 à 30 secondes de partition). Pour La Marseillaise, c'est « Allons enfants de la patrie… le jour de gloire est arrivé ». Ce passage est joué deux fois, comme la reprise de la partition (« Contre nous de la tyrannie… » se chante sur la même mélodie). Le refrain « Aux armes, citoyens » n'est pas dans les données. Je ne l'ai pas ajouté de mémoire, pour ne pas le massacrer.
- **Orchestre** : cuivres, cordes, basse, timbales, cymbale finale, réverbération de stade.
- **Autres pays** : ils gardent un hymne générique (composition originale du jeu).
- **Crédits** : les sources, auteurs et licences de chaque partition sont dans `tools/anthems/CREDITS.md`, résumés dans `CREDITS.md` et dans « À propos ».

## Son et musique : tout tester
- **Accès** : nouvel écran **Options > Tester les sons, musiques, hymnes et vibrations**.
- **Rubriques** :
  - bruitages (24) ;
  - ambiance (rumeur du stade et 12 chants des supporters) ;
  - musiques des menus (8) ;
  - jingles (9) ;
  - hymnes (34 réels + 6 génériques) ;
  - vibrations (14 effets).
- **Lecture** : OK lance ou arrête. Ça marche même si le son ou la musique sont coupés dans les options.
- **Affichage** : égaliseur animé ; pour les hymnes, la source de la partition ; pour les vibrations, le niveau des deux moteurs.

## Vibrations des manettes
- **Moteurs** : gauche (grave) et droit (aigu) pilotés séparément. Les effets sont des motifs d'impulsions qui se superposent.
- **Effets, sur la manette du joueur concerné** :
  - passe, frappe ;
  - tacle réussi, ballon perdu ou faute subie ;
  - battements de cœur pendant l'attente d'un penalty.
- **Effets, pour toute l'équipe** :
  - but marqué (trois impulsions), but encaissé (un coup sourd) ;
  - arrêt du gardien, carton ;
  - poteau (les deux équipes), coup de sifflet final.
- **Intensité** : Options > Vibrations : non / faibles / moyennes / fortes.
- **Plateformes** : XInput sous Windows ; ailleurs, l'API de raylib quand la plateforme la gère.

## Deuxième carton jaune
- L'arbitre montre d'abord le **jaune**, puis, après une courte pause, le **rouge** :
  - message « 2E CARTON JAUNE » puis « CARTON ROUGE ! » ;
  - deux coups de sifflet, caméra rapprochée tout du long ;
  - bandeau TV « 2E CARTON JAUNE » puis « 2E JAUNE = CARTON ROUGE ».
- Les incrustations en tribune ne s'affichent plus pendant un carton.

## Compétitions personnalisées
- **Écran refait** : options en rubriques colorées (général, formule, règlement, équipes, joueurs). Le panneau de droite montre le déroulement (« 8 groupes de 4, 2 qualifiés par groupe : 8es > quarts > demies > finale ») et la liste des équipes.
- **Formats** : championnat, coupe, groupes + phase finale, et nouveau **championnat + play-offs** (finale, Final Four ou quarts entre les 2, 4 ou 8 premiers).
- **Nouvelles options** :
  - championnat en aller simple, aller-retour, 3 ou 4 matchs ;
  - nombre de groupes, 1 ou 2 qualifiés par groupe, repêchage des meilleurs 3es ;
  - phase à élimination en matchs secs ou aller-retour ; finale en match unique (terrain neutre) ou aller-retour ;
  - match pour la 3e place ;
  - 3 ou 2 points par victoire ;
  - départage : différence de buts, confrontations directes ou buts marqués ;
  - buts à l'extérieur ;
  - tirage avec chapeaux ou entièrement au hasard ;
  - suspension après 2, 3, 4 ou 5 jaunes, ou jamais.
- **11 modèles prêts à l'emploi** : Mondial à 32 et à 48, Euro à 24, Ligue des champions, coupe à 64, coupe aller-retour, championnat à 20, championnat + Final Four, mini-tournoi, tournoi de clubs créés.
- **Équipes** :
  - jusqu'à 4 équipes contrôlées ;
  - « Ajouter mes clubs créés » ;
  - « Compléter au hasard » avec des équipes de niveau proche ;
  - « Vider la liste ».
  - Le choix des équipes a une rubrique **Mes clubs créés**, et l'écran « Mes clubs créés » un bouton **Tournoi perso**.

## Menus
- **Choix des équipes** : liste plus lisible et fiche rapide à droite (maillots, niveau, stade, ville, 5 joueurs vedettes).
- **Commandes** : écran refait en cartes. Il décrit le changement de joueur directionnel, le contrôle du receveur, F1 et les vibrations.
- **Options** : petit son au déplacement.

## Vérifications
- Compilé sous Windows et sous Linux.
- Nouveaux tests :
  - `tools/test_anthems.cpp` : les 40 hymnes se génèrent, avec contrôle de la durée et du niveau ; export WAV possible ;
  - `tools/test_custom_rules.cpp` (16 contrôles) : groupes avec meilleurs 3es et phases aller-retour, finale aller-retour, 3e place, 2 points par victoire, clubs créés, play-offs, coupe aller-retour, championnat en 3 matchs.
- Tests précédents réussis : test_build10_match, test_save_integrity, test_continents, test_build18, test_offside, test_custom.
- **Contrôle des hymnes** : une analyse de hauteur du rendu de La Marseillaise retrouve 12 notes sur 18 exactes. Les écarts viennent de l'outil d'analyse, gêné par l'accompagnement ; les notes jouées sont celles de la partition.
- **Non testé** : les vibrations ne peuvent pas être ressenties sur la machine de test, sans manette.
