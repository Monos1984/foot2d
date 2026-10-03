# Super Soccer World / France Foot 2D — 00.03.00 build 5

Extraire le ZIP Win64 et lancer FranceFoot2D.exe. Bibliothèque graphique intégrée, aucune installation de raylib. Sauvegardes dans saves à côté du programme.

## Musée interactif du club

En carrière longue de club ou de joueur, ouvrir Gestion du club → Musée du club. Douze salles : accueil, saisons, trophées, finales, promotions et relégations, records, matchs de légende, légendes et panthéon, entraîneurs, stades, chronologie, maillots. Cliquer sur une ligne ouvre sa fiche ; les flèches gauche/droite changent de salle, haut/bas et la molette parcourent les listes. Échap revient à la liste puis au club. Les filtres parcourent les saisons et les compétitions ; la chronologie possède aussi un filtre de catégorie.

Le bouton Club en haut de la page permet de parcourir les musées des clubs que vous avez dirigés.

Les archives sont enregistrées au fil des événements dans ClubHistory, séparément pour chaque club. Elles restent conservées après un changement de club. Leur ouverture utilise ces données directement, sans recalcul des anciennes compétitions. Les records de même nature améliorés plusieurs fois dans une année sont réunis dans un seul souvenir annuel, tout en conservant la dernière valeur, son détenteur et sa date.

Les saisons conservent division, rang, matchs, résultats, points, buts, mouvement entre divisions, budget disponible, affluence connue, meilleurs contributeurs, trophées et récompenses. Les finales gardent le résultat, la prolongation et les tirs au but, le stade, le capitaine et la composition disponible. Les finales perdues restent présentes. Les tours régionaux de qualification de la Coupe de France ne deviennent pas artificiellement des titres.

Les légendes sont classées par contribution : apparitions, buts, passes, fidélité, capitanat, trophées, récompenses et matchs sans encaisser. Leur note générale n'intervient pas. Le panthéon automatique contient au maximum douze joueurs. Les débuts du club sont vides de palmarès ; aucun titre ancien réel n'est inventé.

Niveaux d'infrastructure : 0 archives, 1 salle des trophées, 2 musée. Ces niveaux modifient le bâtiment et sa présentation ; ils ne suppriment jamais les archives. La galerie garde les couleurs des maillots et le sponsor par saison et lors des changements.

## Données et compatibilité

Sauvegarde version 29 : les anciens formats 23 à 28 restent lisibles. Les sauvegardes antérieures commencent leur musée à partir des événements observés après la mise à jour ; le bouton Archives antérieures garde l'accès aux anciens bilans. Le musée concerne les carrières de club, pas les modes championnat simple, tournoi ponctuel ou sélection nationale.

Les compositions des rencontres jouées sont enregistrées depuis les joueurs présents au coup de sifflet final. Pour les simulations, compositions, capitanat et matchs sans encaisser sont des estimations issues de la sélection automatique ; l'interface le précise. Les budgets et affluences ne sont affichés comme connus que lorsque le système de gestion les fournit. Les anciens palmarès exhaustifs, les maillots sous forme d'images historiques, la décoration libre du musée et les anecdotes personnalisées de supporters restent à développer.

Le moteur des matchs, ses dimensions, ses animations et sa physique ne sont pas modifiés par ce musée. Les améliorations de la build 4 (postes et rôles), les récompenses, les jeunes, les coupes et les adaptations aux pays sont conservées.

## France 2026/27

Couverture inchangée : 3 761 équipes seniors officielles dans 327 groupes importés. Toute la France n'est pas encore complétée ; les groupes restants doivent être vérifiés auprès des ligues et districts. Cette build porte sur le musée demandé, sans prétendre terminer l'import national.

## Validation

Le scénario automatique du musée suit cinq saisons : deux montées, un titre de championnat, une finale perdue, une coupe gagnée, un changement de stade. Il vérifie les records, joueurs, entraîneurs, doubles notifications, changement de club, accès sans bâtiment, sauvegarde/rechargement et refus d'une section musée tronquée. Les autres suites vérifient les fonctionnalités précédentes et les anciens formats. Les preuves de compilation, de tests et les captures d'écran sont incluses dans le ZIP source, dossier verification.

Le document fourni par l'utilisateur est conservé dans docs/FranceFoot2D_Prompt_Musee_Club_Interactif.md.

Résultat de cette livraison : 6 566 contrôles dans les sept suites finales, validateur France réussi, 33 scénarios d’affichage réussis. Architecture Windows x86_64 et absence de dépendance à raylib.dll vérifiées. Les fichiers match.cpp et match.h restent identiques octet pour octet à ceux de la build 4.
