# Technologies et crédits — France Foot 2D

Conception et direction : Jean Monos, Offgame. Sources initiales réalisées avec l'assistance de Claude (Anthropic). Corrections, imports et développements des présentes builds avec Codex (OpenAI). Ces mentions décrivent les outils d'assistance ; les bibliothèques ci-dessous restent les créations de leurs auteurs respectifs.

## Dans le jeu Windows

- C++17 et bibliothèque standard : simulation, carrière, sauvegardes binaires, interface, sprites procéduraux et musique synthétisée.
- [raylib 5.5](https://github.com/raysan5/raylib/tree/5.5), Ramón Santamaría et contributeurs : graphismes, fenêtre, clavier, souris, manettes et audio. Licence zlib/libpng ; texte intégral fourni dans `LICENSE_raylib.txt`. Bibliothèque liée statiquement.
- OpenGL : interface graphique de Khronos ; pilotes fournis par le système et le fabricant de la carte graphique.
- Win32, GDI, WinMM, Shell et Universal C Runtime : composants Windows de Microsoft. Aucun environnement Python ni .NET à installer pour jouer.
- Dépendances embarquées par raylib : GLFW (Marcus Geelnard, Camilla Löwy et contributeurs), glad (David Herberth et contributeurs), stb (Sean Barrett et contributeurs), miniaudio et décodeurs dr_* (David Reid et contributeurs), ainsi que les autres composants décrits dans le [répertoire external de raylib 5.5](https://github.com/raysan5/raylib/tree/5.5/src/external). Leurs notices sont conservées dans les sources amont ; GLFW et glad emploient leurs licences permissives respectives, stb et miniaudio offrent notamment une licence MIT ou le domaine public.

## Construction et préparation des données

- [LLVM / Clang](https://llvm.org/) et [LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw) : compilateur, édition de liens et ressources Windows 64 bits. Projets LLVM, Martin Storsjö et contributeurs ; les composants MinGW-w64 et leurs en-têtes portent leurs propres notices. La compilation utilisée est celle du paquet LLVM-MinGW 20260616 UCRT x86_64.
- [PowerShell](https://github.com/PowerShell/PowerShell), Microsoft et contributeurs : scripts de construction et contrôles.
- [Python](https://www.python.org/), Python Software Foundation et contributeurs : import des compositions, rapports, archives ZIP et empreintes SHA-256.
- [pypdf](https://github.com/py-pdf/pypdf), équipe py-pdf et contributeurs : lecture des documents officiels PDF, notamment pour le Finistère.
- [Pillow](https://python-pillow.github.io/), Alex Clark, Fredrik Lundh et contributeurs : outil de préparation de l'icône (`tools/make_icon.py`).

Ces outils de préparation ne sont pas intégrés au programme Windows. Leurs pages amont donnent les notices et licences complètes.

## Créations et données

Sprites, interface et effets pixelisés : créations et routines du projet. Sons et musiques : synthèse effectuée par le code du jeu ; aucun enregistrement musical commercial embarqué.

Compositions importées : FFF, Ligues régionales et Districts concernés. Les liens, dates, documents et périmètres figurent dans les rapports France 2026/27 et `data/france_2627_coverage.json`. Les noms et marques restent ceux de leurs titulaires ; ce jeu est un projet de fans sans affiliation officielle.

Crédits de données hérités du projet initial : EA SPORTS FC 26 DataHub (joueurs), INSEE (communes), Wikipédia (palmarès). Ces mentions ne garantissent pas que chaque donnée générée ou ancienne a été vérifiée dans cette build. Les groupes encore générés restent distingués des compositions officiellement importées.

Autres composants optionnels mentionnés dans les en-têtes raylib : msf_gif (Miles Fogle), sinfl/sdefl (Micha Mettke), rprand (Ramón Santamaría), QOI/QOA (Dominic Szablewski), par_shapes (Philip Rideout), tinyobj_loader_c (Syoyo Fujita), cgltf (Johannes Kuhlmann), m3d (bzt), vox_loader (Johann Nadalutti), jar_xm/jar_mod (Joshua Reisenauer). Le jeu 2D n'utilise pas les fonctions de modèles 3D et ne fournit aucun asset dans ces formats. RGFW (ColleagueRiley) est une alternative à GLFW proposée par raylib, sans être le backend Windows de cette livraison.


Remerciement à **Thorn Atari**.
