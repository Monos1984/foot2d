# France 2026/27 — couverture de la build 0

Relevés vérifiés le 2 octobre 2026. Le chantier national complet n'est pas terminé. Un test structurel réussi ne certifie pas les équipes encore générées, ni tous les règlements sportifs locaux.

## Compositions importées

- Pays de la Loire R1/R2/R3 : 192 équipes, 16 groupes. R3 confrontée aux dix classements FFF actuels.
- Bretagne R1/R2/R3 : 244 équipes, 20 groupes. R1 : 2 groupes de 14 ; R2 : 6 groupes de 12 ; R3 : 12 groupes de 12.
- Normandie R1/R2/R3 : 168 équipes, 14 groupes. R1 : 24 ; R2 : 49 (groupe D à 13) ; R3 : 95 (groupe H à 11).
- Maine-et-Loire D1/D2/D3/D4 : 204 équipes, 17 groupes, déjà importées dans la build 11.
- Loire-Atlantique D1/D2/D3/D4 : 276 équipes, 23 groupes. D4 : neuf groupes sur la publication mise à jour le 25 août, malgré le texte de l'article qui en annonce encore sept.
- Finistère D1/D2/D3 : 350 équipes, 30 groupes. D1 : 72 ; D2 : 121 avec groupe A à 13 ; D3 : 157 dans 14 groupes de 11 ou 12.

Total hors divisions nationales préexistantes : 1 434 équipes / 120 groupes. Les joueurs, notes, maillots non documentés et installations des nouveaux clubs demeurent issus du moteur de génération ; ce lot vérifie les équipes et leurs groupes, pas leurs effectifs de joueurs.

## Sources officielles

- [Pays de la Loire, publication](https://lfpl.fff.fr/wp-content/uploads/sites/20/2026/07/Groupes-Seniors-20262027.pdf) ; [R3 FFF actuelle](https://epreuves.fff.fr/competition/engagement/452589-regional-3/phase/1).
- [Bretagne, groupes du 17 juillet](https://footbretagne.fff.fr/simple/les-groupes-pour-la-saison-2026-2027/). Les sept images officielles sont conservées sous data/official/images.
- Normandie : [R1](https://epreuves.fff.fr/competition/engagement/453390/phase/1), [R2](https://epreuves.fff.fr/competition/engagement/453391/phase/1), [R3](https://epreuves.fff.fr/competition/engagement/453392/phase/1). Les quatorze listes importées concordent exactement avec les relevés DOM conservés dans normandie-live-2627.json, qui incluent les identifiants FFF.
- [Loire-Atlantique, groupes seniors](https://foot44.fff.fr/simple/seniors-les-groupes-seniors-masculins-feminines/). Les tableaux officiels complets sont conservés.
- [Finistère, D1/D2](https://foot29.fff.fr/simple/groupes-d1-d2-saison-2026-2027/) et [D3](https://foot29.fff.fr/simple/groupe-seniors-d3/). Les trois PDF sont inclus dans les sources.
- Maine-et-Loire : liens précis et relevés conservés dans FRANCE_2026_27_BUILD11.md et data/france_2627_official.json.

## Corrections du moteur et des affiliations

L'appartenance vérifiée à une ligue/district prime sur le département géographique : FC Atlantique Morbihan peut jouer dans le district 44 malgré son implantation dans le 56. Les clubs de Gourin, Guiscriff, Arzano et Roudouallec restent dans les groupes publiés par le Finistère. Une normalisation ultérieure ne doit pas annuler ces affiliations.

La comparaison des noms du générateur ignore désormais la casse, pour éviter un doublon tel que ROSTRENEN FC / Rostrenen FC. FC Entente du Vignoble est corrigé en Loire-Atlantique avec ses réserves ; il était enregistré sous une forme abrégée et un mauvais département. Les abréviations des publications sont rapprochées explicitement d'une équipe première unique, sans recherche approximative ni parent inventé.

Le Finistère s'arrête à D4 : la D5 ajoutée par l'ancien calcul démographique est supprimée dans les nouvelles parties. Les groupes D4 restent générés en attente d'une réconciliation complète des calendriers et ententes.

## Saison suivante

Les groupes exceptionnels actuels sont conservés au départ. Les cibles du jeu sont notamment N1 : 48, N2 : 112 ; Normandie R2 : 48 et R3 : 96 ; Finistère D2 : 120. Les contrôles vérifient les mouvements et leurs cascades sans perte ni duplication des clubs.

Les tailles 11/12 de D3 du Finistère sont considérées autorisées et conservées comme cibles spécifiques. Cela évite de traiter automatiquement un groupe de onze équipes comme une anomalie. La règle de régularisation et ses limites sont détaillées dans FRANCE_2026_27_BUILD12.md.

## Travail restant

Les autres ligues régionales et la plupart des districts conservent des équipes générées. Aucun groupe réel de D6/D7/D8 n'a encore été importé.

Maine-et-Loire D5 : dix groupes / 118 équipes relevés, deux places exemptes exclues ; l'affiliation de l'entente Combrée Pouancé 3 reste à résoudre. Le niveau n'est pas présenté comme terminé.

Loire-Atlantique D5 et Finistère D4 : compositions encore à intégrer. Les ententes de D4 demandent un lien documenté avec leurs clubs participants ; elles ne doivent pas être assimilées arbitrairement à une réserve ordinaire.

Île-de-France : la publication initiale contient encore des alternatives « ou » et des clubs à réconcilier avec la base nationale ; les classements actuels doivent trancher avant import. Corse : le portail officiel affiche R1 à R4, ce qui nécessite de corriger sa structure actuelle avant d'importer les niveaux bas sans faux districts.

Le relevé machine data/france_2627_coverage.json distingue les zones importées et restantes. Les cinq territoires ultramarins y sont recensés séparément, sans être annoncés complétés par les imports métropolitains.
