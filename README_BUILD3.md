# France Foot 2D — 00.03.00 build 3

Décompresser le ZIP Windows, puis lancer **FranceFoot2D.exe**. Windows 64 bits, autonome : aucun environnement Python, navigateur, .NET ou DLL raylib à installer. Le programme et ses dépendances C++/raylib sont liés statiquement. `Verifier-France.exe` est inclus.

## Récompenses de fin de saison

Dans les carrières de club, le bilan ouvre une cérémonie en pixels : joueur de la saison, meilleur buteur, meilleur passeur, gardien, espoir de 21 ans maximum et entraîneur du champion. Les candidats appartiennent au championnat / groupe joué par votre équipe. Les buts et passes sont lus dans les événements de ce championnat, sans ajouter ceux des coupes. Le jury du jeu combine ces statistiques et la note individuelle ; le gardien est évalué avec les buts encaissés par son équipe. Le nombre de matchs affiché est une estimation bornée par les rencontres de championnat, car les anciennes sauvegardes ne séparent pas les apparitions par compétition. L'entraîneur adverse est représenté par son rôle et son club lorsque le jeu ne dispose pas de son identité.

OK révèle le lauréat, puis passe à la distinction suivante. Retour permet de passer la cérémonie. Les identités et le championnat sont conservés dans la sauvegarde. Le menu « Récompenses du championnat » permet de consulter les éditions précédentes. Le Ballon d'or de fin octobre reste une cérémonie distincte.

## Équipes pour la prochaine saison

Une réserve ou une équipe U15/U17/U19 peut être demandée en cours de saison. Elle apparaît comme **créée, en attente d'inscription** dans la gestion du club ; elle n'a aucun match ni place dans le championnat courant. Elle entre en compétition **à la saison suivante**, dans la dernière division disponible de son propre district. Ce comportement remplace l'inscription immédiate de build 2.

Après le bilan, un écran demande si vous souhaitez créer une réserve supplémentaire ou des sections jeunes manquantes. Tous les choix sont facultatifs. Les équipes déjà demandées sont aussi intégrées. Les limites restent de trois réserves et une section par catégorie. Un club fanion commande ses réserves ; la préparation depuis une section du club permet de demander les jeunes du club parent.

## Pôles Espoirs et recrutement U15

Les carrières de club masculin en France disposent de 16 équipes de Pôles Espoirs : INF Clairefontaine, Aix-en-Provence, Ajaccio, Castelmaurou, Châteauroux, Dijon, Liévin, Lisieux, Lyon, Nancy, Ploufragan, Reims, Saint-Sébastien-sur-Loire, Talence, Guadeloupe et Océan Indien. Chaque pôle reçoit 22 **joueurs simulés** de 13 ou 14 ans. Les effectifs réels de mineurs ne sont pas importés.

La **Coupe des Pôles Espoirs est une compétition créée pour le jeu à votre demande**. Elle ne prétend pas être une compétition officielle de la FFF. Quatre groupes de quatre, trois rencontres par équipe, les deux premiers en quarts, puis demi-finales et finale : 31 matchs. Tous ont lieu sur le même site de Pôle Espoirs, qui change chaque année dans la simulation. Chaque match dure **2 × 20 minutes** ; les égalités en élimination directe sont départagées directement aux tirs au but, sans prolongation. Le calendrier du tournoi est simulé sur six journées.

Le menu « Pôles Espoirs : tournoi et recrutement » montre le classement individuel, les buts, passes, âge et note. Après la finale, choisissez un joueur pour le réserver à vos U15 de la saison suivante ; créez d'abord cette section si elle manque. Une réservation ne transfère pas immédiatement le joueur. Les places réservées sont protégées des recrutements automatiques ; une équipe U15 ne peut pas dépasser 30 joueurs. Une demande reste en attente si son effectif est devenu complet. Les joueurs sortants de 15 ans rejoignent différentes équipes U15, en priorité dans leur région. Une nouvelle promotion de 13 ans remplace les partants. Le passage des catégories respecte désormais U15 jusqu'à 15 ans et U17 jusqu'à 17 ans.

Ce fonctionnement est une adaptation de jeu : en réalité, les pensionnaires des pôles restent également inscrits dans leurs clubs de proximité. Cette build couvre les pôles masculins ; les pôles féminins et le futsal ne sont pas ajoutés.

Sources officielles vérifiées le 2 octobre 2026 : [réseau FFF des Pôles Espoirs masculins](https://www.fff.fr/94-les-poles-espoirs-masculins.html), [carte et programme PPF LAuRAFoot](https://laurafoot.fff.fr/wp-content/uploads/sites/10/2025/08/PROGRAMME-PPF-25-26-LAURAFOOT.pdf), [implantation à Lyon pour 2026/27](https://laurafoot.fff.fr/wp-content/uploads/sites/10/2026/04/Liste-finale-des-joueurs-retenus-saison-26-27.pdf), [promotion Océan Indien 2026/28](https://liguefoot-reunion.fff.fr/simple/pefoi-les-admis-pour-la-rentree-2026-2027/).

## Création de club et coupes

Le créateur de club ne propose plus de division de départ : le moteur choisit la dernière division existante dans le district sélectionné. Un club marnais neuf commence ainsi en D4, même si une ancienne donnée d'éditeur indique un niveau supérieur. L'inscription demandée en cours de carrière reste différée à l'année suivante.

La première saison sportive est enregistrée. Dans les coupes nationales, régionales et de district auxquelles un club neuf est éligible, les exemptions sont attribuées aux clubs déjà établis. Les nouveaux clubs disputent les premiers tours de Coupe de France, y compris les préliminaires nécessaires, et ne reçoivent pas la protection automatique de la Coupe de la Marne. Les conditions d'éligibilité aux compétitions restent applicables. Pour une coupe avec un tableau impair, le moteur fait jouer en priorité le club neuf. Si tous les participants restants sont nouveaux, un match d'ajustement joué remplace l'exemption ; le tableau et le tour courant restent conservés après sauvegarde. Dans les coupes de consolation marnaises, un match d'ajustement du tableau évite de donner une deuxième exemption quand aucun participant ne peut en bénéficier.

## Mode Championnat sur plusieurs saisons

Le bilan permet de commencer une nouvelle saison. Les promotions, relégations, effectifs, archives et équipes contrôlées sont conservés. Les quatre clubs humains éventuels restent contrôlés après le passage de saison et après une sauvegarde/reprise. Le mode garde son fonctionnement sans licenciement de manager. Les cérémonies de carrière et les pôles ne sont pas ajoutés à ce mode.

## Sauvegardes et validation

Le format **27** enregistre les cérémonies du championnat, les débuts sportifs des clubs, les pôles et les recrutements différés. Les formats **23 à 26** restent lisibles. Les parties existantes reçoivent le tournoi des pôles à leur prochain passage de saison, sans remplacer leur calendrier courant. Les anciens clubs ne reçoivent pas rétroactivement le statut de nouveau club. Une ancienne build ne peut pas lire une sauvegarde au format 27.

La compilation portable et les tests sont reproductibles :

```powershell
.\build-win64.ps1 -Compiler 'C:\outils\llvm-mingw\bin\clang++.exe' -Raylib 'C:\outils\raylib-5.5_win64_mingw-w64' -Tests
```

Les journaux de vérification et captures sont dans le ZIP sources (`verification`) ; la livraison Windows fournit `VALIDATION.txt`. Les nouveaux tests couvrent l'inscription différée, le district réel, le départ obligatoire en bas de pyramide, les exemptions, les âges, les 31 matchs, les TAB, l'horloge du match joué, le recrutement et sa sauvegarde, les cérémonies et trois passages de saison en Championnat. Les régressions des compositions France, des nations, du Ballon d'or, de la Marne et des 22 Coupes du monde historiques sont aussi contrôlées.

La couverture France demeure celle de build 2 : **3 761 équipes seniors et 327 groupes officiellement importés**, dont le Grand Est et ses neuf districts. **Les autres ligues et districts restant générés ne sont pas terminés dans cette build.** Voir `data/france_2627_coverage.json`. Les limites réglementaires Marne de build 2 restent présentes : exclusions temporaires, contrôle des joueurs ayant disputé plus de dix matchs en équipe supérieure et limite de trois séquences de remplacements.

Les crédits et le remerciement à **Thorn Atari** sont conservés dans le jeu et dans `CREDITS.md`.
