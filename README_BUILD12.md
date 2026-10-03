# France Foot 2D — v00.03.00 build 12

Lancez FranceFoot2D.exe. Application autonome Windows 64 bits, bibliothèques raylib et C++ liées statiquement. Les composants système de Windows restent nécessaires.

Cette build ajoute le retour aux effectifs prévus à la fin des saisons : descentes supplémentaires en cascade, promotions supplémentaires, puis repêchage d'un relégué si les équipes de la division inférieure ne sont pas éligibles. Les réserves restent sous leurs équipes supérieures, selon les niveaux disponibles. Une division inexistante est franchie pour atteindre le niveau réellement suivant de la même zone.

Les groupes exceptionnels de départ restent conservés en 2026/27. Dans la configuration actuelle, N1 passe de 49 à 48 équipes et N2 de 111 à 112 pour la saison suivante. Chaque groupe est ensuite recomposé à sa taille cible. Les anciennes sauvegardes conservent leurs équipes et leurs réglages : elles ne reçoivent pas automatiquement les nouvelles compositions officielles.

Un effectif cible impossible à atteindre avec les clubs éligibles est signalé dans les nouvelles de saison. Le dernier niveau d'un district absorbe les équipes restantes et ajuste son nombre de groupes ; aucun club n'est supprimé ou inventé pour atteindre un multiple arbitraire.

La couverture officielle reste celle de la build 11 : 396 équipes, 33 groupes, Pays de la Loire R1/R2/R3 et Maine-et-Loire D1 à D4. Aucune nouvelle ligue ni aucun district supplémentaire n'a été importé dans ce lot. Voir FRANCE_2026_27_BUILD11.md pour les sources et les limites, notamment la D5 en attente et les D6/D7/D8 encore sans compositions réelles importées.

## Recompiler

Utiliser LLVM-MinGW x86_64 et raylib 5.5 MinGW x64, puis PowerShell :

```powershell
.\build-win64.ps1 -Compiler 'C:\outils\llvm-mingw\bin\clang++.exe' -Raylib 'C:\outils\raylib-5.5_win64_mingw-w64' -Tests
```

Le script produit le jeu et Verifier-France.exe. Avec -Tests, il contrôle également les transitions et la lecture des sauvegardes formats 23 et 24 à partir des écrivains d'origine figés sous tools/fixtures. Les nouvelles sauvegardes sont au format 25.

Verifier-France.exe contrôle la base de départ. Le test d'intégration vérifie aussi les effectifs après changement de saison ; cela ne certifie pas les compositions encore générées dans le reste de la France.
