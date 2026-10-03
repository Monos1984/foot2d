# France Foot 2D — v00.03.00 build 10

## Mise à jour France 2026/2027

Cette build corrige la base nationale française à partir des compositions effectivement jouées au début de la saison 2026/2027, après les décisions administratives connues au 2 octobre 2026.

### Corrigé

- Ligue 1 : 18 clubs, composition conservée après validation.
- Ligue 2 : 18 clubs, composition conservée après validation.
- Ligue 3 : 18 clubs ; plusieurs noms ont été normalisés (US Orléans LF, Le Puy-en-Velay FC, SM Caen, SC Aubagne Air Bel, FC Villefranche-Beaujolais, Quevilly Rouen Métropole, Football Bourg-en-Bresse Péronnas 01, VFC La-Roche-sur-Yon).
- National 1 : groupes 2026/27 remplacés par les groupes effectivement joués ; 49 clubs au total (16 + 16 + 17), avec l'UF Touraine dans le groupe C.
- National 2 : groupes 2026/27 remplacés ; 111 clubs au total (7 groupes de 14 + groupe H de 13 après la montée administrative de l'UF Touraine en N1).
- Réserves : suppression des doublons de réserves déjà présentes en National 2 et correction des liens vers les équipes premières.
- National 1 / National 2 : le moteur accepte maintenant les tailles de groupes exceptionnelles de la saison 2026/27 au lieu de forcer 16 et 14 partout.
- District : capacité structurelle étendue de D1 à D8, avec indices de niveau sécurisés pour éviter les accès hors limites.
- Formats régionaux : corrections structurelles 2026/27 pour Hauts-de-France, Grand Est, Occitanie, Auvergne-Rhône-Alpes et Méditerranée.
- Version : build 10.
- Test automatique ajouté : `tools/test_france_2627.cpp`.

### Validation automatique

Le test France 2026/27 vérifie :

- L1 = 18
- L2 = 18
- Ligue 3 = 18
- National 1 = 49
- National 2 = 111
- absence de doublon entre les niveaux nationaux
- résolution correcte des équipes premières pour les réserves nationales

Résultat attendu : `PASS`.

## Important — ce qui reste à compléter

Cette build ne prétend pas encore contenir la composition club-par-club exacte de toutes les divisions Régional et District de France. Une partie des clubs des niveaux inférieurs est toujours générée par l'ancien moteur.

Le moteur peut désormais représenter jusqu'à D8, mais `distLevels()` continue à déterminer automatiquement le nombre de niveaux actifs d'un district selon sa taille. Les D6/D7/D8 réelles devront donc être activées district par district à partir des données officielles.

Le tableau Grand Est utilise encore une taille uniforme par groupe dans le moteur alors que la saison 2026/27 possède des groupes de tailles différentes à certains niveaux. Une évolution du modèle `Pool` sera nécessaire pour reproduire cela exactement.

La prochaine étape de fiabilisation France est donc de remplacer progressivement la génération régionale/départementale par une base de données réelle 2026/27, Ligue par Ligue puis District par District.
