# France Foot 2D — 00.03.00 build 1

Décompresser le ZIP Windows et lancer `FranceFoot2D.exe`. Programme autonome Windows 64 bits ; bibliothèque C++ et raylib liées statiquement. `Verifier-France.exe` contrôle la base France sans ouvrir le jeu.

## Changements

- Classements et résultats : nation, championnats et coupe nationale du club choisi, avec accès aux autres pays. Le contexte est partagé par la carrière manager et le mode Championnat. En tournoi personnalisé et international, la liste présente les compétitions de ce tournoi. Les onglets nationaux ne proposent plus les groupes français à la place de divisions étrangères absentes.
- Fin de saison : champions et coupe du pays joué, classement du club dans sa vraie compétition, qualifications européennes filtrées par nation. Pour les pays hors UEFA, affichage de leur compétition continentale. Palmarès étranger et féminin : archives des compétitions effectivement jouées en carrière.
- DNCG : uniquement en France. Contrôles de recrutement, masse salariale et rétrogradation DNCG désactivés hors France, y compris après chargement d'une ancienne partie contenant ces indicateurs. Un déficit étranger peut toujours réduire la confiance du président.
- Recettes : la règle de partage de la Coupe de France et les messages français ne s'appliquent plus aux coupes étrangères. Les frais financiers génériques restent des paramètres de simulation ; cette build ne prétend pas reproduire tous les régimes fiscaux et règlements réels du monde.
- Début de carrière : panneau du club, réglages compacts et explications séparées. Deux options indépendantes pour la vie privée du manager et la valise à l'arbitre. L'option de vie du manager ne supprime pas la vie du joueur dans une carrière de joueur. Les enquêtes de corruption restent actives si la valise est activée sans vie privée.
- À propos : catégorie Technologies et crédits ; détails dans `CREDITS.md`.
- Coupe du monde 1930 : classement de groupes à 2 points la victoire et 1 le nul, indication à l'écran et restauration du barème historique lors du chargement d'une partie.

## Compatibilité et périmètre

Sauvegardes formats 23, 24 et 25 conservées. Les nouveaux interrupteurs réutilisent deux octets précédemment réservés : dans les anciennes parties ils restent activés par défaut. Le format reste 25 et le bloc d'options conserve sa taille de 8 octets.

Les compositions France restent celles de build 0 : **1 434 équipes / 120 groupes officiellement importés**, Pays de la Loire, Bretagne, Normandie, Maine-et-Loire, Loire-Atlantique et Finistère aux niveaux documentés. Aucune nouvelle ligue n'a été ajoutée dans ce lot consacré aux corrections de jeu. La règle de rééquilibrage des promotions et relégations, avec cascades et protection des réserves, reste présente. Le reste de la France est encore en chantier : voir `FRANCE_2026_27_BUILD0.md`.

## Recompiler et vérifier

Avec LLVM-MinGW x86_64 et raylib 5.5 MinGW x64 :

```powershell
.\build-win64.ps1 -Compiler 'C:\outils\llvm-mingw\bin\clang++.exe' -Raylib 'C:\outils\raylib-5.5_win64_mingw-w64' -Tests
```

Le script construit le jeu, le vérificateur France, les tests des compositions, les anciennes sauvegardes et `tools/test_nation_context.cpp`. Ce dernier vérifie les contextes France/Allemagne/Belgique/Angleterre/Brésil, le féminin français et allemand, les finances DNCG, les options persistées, deux saisons étrangères et le barème 1930. Résultats effectifs dans `VALIDATION.txt` (ZIP Windows) ou `verification/build-and-tests.txt` (ZIP sources).
