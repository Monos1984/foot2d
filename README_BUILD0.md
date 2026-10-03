# France Foot 2D — 00.03.00 build 0

La numérotation repart à build 0, à la demande de l'utilisateur. Les prochaines livraisons compilées incrémenteront uniquement le numéro de build de la version 00.03.00.

Décompresser le ZIP Windows puis lancer FranceFoot2D.exe. Le jeu est autonome pour Windows 64 bits : raylib et la bibliothèque C++ sont liées statiquement. Verifier-France.exe contrôle la cohérence de la base initiale sans ouvrir le jeu.

Ce lot porte les compositions régionales et départementales officiellement importées à 1 434 équipes et 120 groupes. Pays de la Loire, Bretagne et Normandie : R1/R2/R3 ; Maine-et-Loire et Loire-Atlantique : D1 à D4 ; Finistère : D1 à D3. Le reste de la France demeure en chantier. Le détail et les sources sont dans FRANCE_2026_27_BUILD0.md et data/france_2627_coverage.json.

La règle de régularisation demandée reste active : effectif courant conservé, puis promotions, relégations et repêchages ajustés à la saison suivante, avec répercussions dans les niveaux inférieurs et protection des liens entre équipes premières et réserves. Les tailles autorisées différentes par groupe peuvent être conservées. Une cible impossible à atteindre est signalée sans supprimer ou inventer de club.

Pour bénéficier des nouvelles compositions, commencer une nouvelle partie. Les anciennes sauvegardes restent lisibles mais conservent leurs propres équipes et réglages.

## Recompiler les sources

Avec LLVM-MinGW Windows x86_64 et raylib 5.5 MinGW x64 :

```powershell
.\build-win64.ps1 -Compiler 'C:\outils\llvm-mingw\bin\clang++.exe' -Raylib 'C:\outils\raylib-5.5_win64_mingw-w64' -Tests
```

Le script produit FranceFoot2D.exe et Verifier-France.exe. L'option -Tests vérifie les groupes, les réserves, deux transitions de saison, les effectifs cibles, les sauvegardes formats 23/24/25 et l'utilisation des clubs importés en tournoi personnalisé. La présence d'une division D6/D7/D8 dans le moteur ne signifie pas que sa composition réelle a déjà été importée.
