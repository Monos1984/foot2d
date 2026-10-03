# France Foot 2D - v00.03.00 build 11

Premier lot de remplacement des équipes seniors masculines générées, à partir de l'archive source build 10 fournie. Consultation des publications : 2 octobre 2026.

## Équipes réellement intégrées

- Pays de la Loire R1 : groupes A et B, 24 équipes.
- Pays de la Loire R2 : groupes A à D, 48 équipes.
- Pays de la Loire R3 : groupes A à J, 120 équipes.
- Maine-et-Loire D1 : groupes A et B, 24 équipes.
- Maine-et-Loire D2 : groupes A à C, 36 équipes.
- Maine-et-Loire D3 : groupes A à E, 60 équipes.
- Maine-et-Loire D4 : groupes A à G, 84 équipes.

Total : 396 équipes et 33 groupes de 12. Ces pools n'utilisent plus de clubs de remplacement générés. Les listes conservent les libellés publiés dans le manifeste, les pages et les URLs officielles. Des alias explicitement répertoriés rattachent les réserves aux équipes premières existantes ; les numéros 2, 3, 4 sont conservés, même si certaines équipes intermédiaires sont absentes d'un niveau.

Les listes de juillet sont publiées sous réserve des procédures administratives. Le calendrier rectificatif R1 A du 12 août confirme les mêmes participants. Le lien R3 J manque dans l'article des calendriers, mais le PDF officiel des compositions contient bien A à J. Ce lot reproduit les listes officielles disponibles ; il ne constitue pas une certification de toutes les décisions administratives ultérieures.

## Moteur et contrôles

- Conservation du tirage officiel lors d'une reconstitution sans changement de participants.
- Capacités individuelles par groupe : par exemple 11, 12 et 13, sans créer un club « Exempt ».
- Réaffectation des groupes après une saison et promotions/relégations du moteur conservées. Les règlements de montée/descente de chaque ligue ne sont pas encore tous revérifiés.
- Correction d'un doublon hérité : Stade de Reims C pouvait être créé deux fois à cause d'une numérotation fondée sur le nombre d'équipes plutôt que leur numéro réel.
- Correction du test national : les erreurs de doublons faisaient imprimer une erreur sans faire échouer le résultat final.
- Validateur sur la pyramide réellement construite : affectations uniques, noms en double, géographie, réserves orphelines, équipes sans groupe, effectifs et groupes officiels.
- Sauvegarde au format 24 : groupes et capacités persistants. Lecture des formats 21 à 23 maintenue. Une sauvegarde de format 23 a été créée avec le code de sérialisation original de la build 10 puis rechargée par la build 11.

## Validation effectuée

- Compilation Windows x86-64 avec LLVM-MinGW et raylib 5.5 statique.
- Test national `test_france_2627` : PASS.
- Test d'intégration `test_official_2627` : groupes, réaffectation, contrôles négatifs (doublon, groupe incomplet, réserve orpheline), sauvegarde/rechargement, saison simulée et passage à la suivante, mode gestion et tournoi personnalisé.
- D6/D7/D8 : tests structurels sur des groupes de tailles différentes. Aucune composition réelle D6/D7/D8 n'est encore importée dans ce lot.

Les données nouvelles appartiennent à la base commune utilisée par les modes. Les tests ci-dessus ne représentent pas une vérification manuelle exhaustive de toutes les interfaces et de tous les modes.

## Couverture encore partielle

- 12 autres ligues métropolitaines, l'outre-mer et les autres districts : restent à importer club par club.
- Maine-et-Loire D5 : la publication de septembre est extraite et conservée dans `pending_groups`. 10 groupes, 118 participants et deux places exemptes. L'entente « ENT. COMBRÉE POUANCÉ 3 » doit être modélisée avec ses affiliations réelles ; aucune équipe première fictive n'est créée pour masquer ce cas. La D5 du jeu conserve donc ses clubs générés dans cette build.
- Le Maine-et-Loire possède désormais la structure publiée 2/3/5/7/10 groupes de D1 à D5 ; seuls D1 à D4 ont leurs vrais participants dans ce lot.
- Les noms complets d'affiliation et numéros FFF restent à enrichir. Les libellés abrégés officiels des publications sont conservés plutôt que développés par supposition.
- Effectifs des joueurs amateurs, maillots, notes, stades et équipes de jeunes : restent générés ou hérités ; les listes seniors ne prouvent pas la composition des compétitions jeunes/féminines.
- L'archive originale contient une vieille `saves/auto.sav` de format 10. Elle était déjà incompatible avec le lecteur de la build 10 (minimum 21) et n'est pas distribuée comme sauvegarde utilisable.

## Sources et reproduction

`data/france_2627_official.json` : participants, réserves, groupes, départements, source/page, listes D5 en attente.

`data/official/*-extracted.txt` : textes des publications officielles conservés pour reproduire l'import. Les extractions ont été obtenues par consultation web des PDF ; ces fichiers ne sont pas les PDF originaux.

`tools/import_official_2627.py` : régénère le manifeste et `src/data_france_official.cpp`. Exécution Python 3 depuis n'importe quel dossier.

Pages officielles :

- [Pays de la Loire](https://lfpl.fff.fr/simple/championnats-regionaux-les-groupes-2026-2027/)
- [Maine-et-Loire D1 à D4](https://foot49.fff.fr/simple/seniors-m-decouvrez-les-groupes-de-la-d1-a-la-d4-2/)
- [Maine-et-Loire D5](https://foot49.fff.fr/simple/groupes-seniors-m-d5-et-seniors-f-d1-d2/)

Pour poursuivre : résoudre l'entente D5, puis étendre les ligues régionales par pools complets validés. La publication actuelle du [District d'Alsace](https://alsace.fff.fr/simple/seniors-la-composition-des-groupes-saison-2026-2027/) confirme D1 à D8, mais son image liée porte un chemin de 2025 ; vérifier le calendrier effectif avant de transformer cette image en données 2026/27.
