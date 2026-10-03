# France Foot 2D — 00.03.00 build 2

Décompresser le ZIP Windows puis lancer `FranceFoot2D.exe`. Programme autonome Windows 64 bits ; raylib et la bibliothèque C++ sont liées statiquement. Le vérificateur France est inclus.

## Grand Est, réserves et calendrier historique

Les compositions seniors 2026/27 de R1, R2 et R3 du Grand Est, ainsi que celles de ses **neuf districts**, sont importées : Alsace, Ardennes, Aube, Haute-Marne, Marne, Meurthe-et-Moselle, Meuse, Moselle et Vosges. Cela représente **2 327 équipes dans 207 groupes**, dont les vraies D6, D7 et D8 d'Alsace. Les exceptions de taille publiées sont conservées ; aucune équipe fictive ne complète ces groupes. L'Aube et la Meuse s'arrêtent en D3 : aucune D4 fictive ne leur est ajoutée.

Les libellés publiés, sources et correspondances des ententes restent consultables dans `data/france_2627_official.json` et `data/official/grandest-aliases.json`. Les équipes d'une même entente sont rattachées à leur équipe supérieure sous un nom commun pour préserver les liens du moteur ; leur libellé publié est conservé dans le manifeste. Les affiliations officielles traversant une frontière départementale sont conservées.

Le validateur vérifie l'unicité des équipes, les groupes, les affiliations et les parents des réserves. Les changements de division conservent les clubs et rééquilibrent les niveaux inférieurs. Les cibles Grand Est suivent la transition publiée : R3 à 13 groupes en 2027/28, 12 en 2028/29 ; R2 à 6 groupes en 2029/30. Les quotas d'accession des neuf districts totalisent 28 places, avec classement des meilleurs suivants si le quota dépasse le nombre de poules. La simulation des mouvements et des barrages reste une approximation sportive ; elle ne reproduit pas les décisions administratives de commission.

Une nouvelle réserve rejoint immédiatement la dernière division **de son district réel**, dès sa première année. Le calendrier est reconstruit avant le premier match ; après le début du championnat, seuls de nouveaux matchs futurs sont ajoutés. Une création très tardive peut prolonger la saison pour permettre ces rencontres. Les résultats déjà joués sont conservés.

Les 22 éditions historiques de la Coupe du monde, **1930 à 2022**, commencent et finissent aux dates demandées, y compris novembre–décembre au Qatar. Les calendriers sont corrigés lors du chargement d'une ancienne partie. Le barème 1930 reste à deux points par victoire. **Les dates intermédiaires des rencontres sont réparties dans la période historique : elles ne reproduisent pas chaque affiche du calendrier d'archives.**

## Trois coupes de la Marne

La Coupe de la Marne Neotec – Challenge Éric Collinet remplace l'ancienne coupe générique. Les clubs de R1 à D4 engagent leur équipe admissible la plus élevée. Les clubs régionaux entrent au quatrième tour. Les clubs encore engagés en Coupe de France ou Coupe du Grand Est sont protégés jusqu'aux 16es inclus. Le club inférieur reçoit en cas d'écart d'au moins deux divisions ; la finale est neutre.

Les perdants de district aux quatre premiers tours alimentent automatiquement la **Coupe D1/D2** ou la **Coupe D3/D4**, selon leur division. Aucune équipe n'est repêchée après ces tours. Les petites coupes limitent les exemptions à une par équipe avant les quarts et les réceptions successives à deux, sauf confrontation entre deux équipes déjà contraintes. Des journées sans tirage permettent d'attendre les repêchées suivantes sans attribuer une deuxième exemption.

Les trois finales ont lieu le **16 mai 2027**, sur le même site. Le calendrier est celui du District. Le lieu 2027 n'étant pas annoncé lors de la vérification, **Auguste-Delaune à Reims est un choix de la simulation**, et les horaires 13h/16h/19h sont simulés. Tous les tours, finale comprise, se jouent sans prolongation : tirs au but après un nul. Les tours précédents gardent les remplacements libres de District ; les finales passent à une feuille de 16 joueurs et cinq remplacements, sans retour d'un joueur remplacé. Les tours hivernaux commencent à 14h30.

**Limites réglementaires restantes :** le moteur ne gère pas encore les exclusions temporaires, le contrôle de trois joueurs ayant disputé plus de dix rencontres en équipe supérieure, ni la limite de trois séquences de remplacements. Ces dispositions figurent dans les PDF officiels livrés ; elles ne doivent pas être considérées comme implémentées. Les dispositions de qualification des licences et les décisions disciplinaires réelles ne sont pas reproduites.

Sources : [compositions Grand Est](https://lgef.fff.fr/simple/championnats-regionaux-les-calendriers-detailles-26-27/), [règlement des championnats](https://lgef.fff.fr/wp-content/uploads/sites/14/2026/05/Championnats-Seniors-LGEF-26-27.pdf), [documents du District Marne](https://marne.fff.fr/documents/?cid=52), [calendrier Marne 2026/27](https://marne.fff.fr/wp-content/uploads/sites/59/2026/07/Calendrier-SENIORS-2026-2027-au-24-juillet.pdf). Les liens de chacun des neuf districts sont conservés dans le manifeste.

## Ballon d'or

Une cérémonie annuelle se déclenche le **28 octobre** du calendrier simulé. Elle concerne les carrières de club (jeu, manager, manager + jeu, joueur et joueuse) et les carrières de sélectionneur. Elle n'apparaît pas dans les tournois isolés, les compétitions personnalisées ou le mode Championnat allégé.

Le calendrier s'arrête avant le match suivant. La scène pixelisée révèle le podium, le trophée et les trois premiers du prix masculin ou féminin. Tab ou les boutons permettent de changer de catégorie. OK révèle immédiatement le podium puis permet de continuer ; Retour passe la cérémonie. Le prochain match reste en attente. Le menu **Distinctions / Ballon d'or** permet de consulter les éditions précédentes.

Les deux prix sont attribués par un **jury simulé**, indépendamment du pays joué. Le classement prend en compte le niveau individuel, les buts, les passes et les apparitions de la saison précédente et de l'automne courant. Lors de la première saison, les statistiques historiques absentes ne sont pas inventées. Les effectifs de l'élite encore non générés sont préparés pour établir les nominations ; les joueurs déjà présents dans les autres effectifs seniors restent également éligibles. Ce prix du jeu ne prétend pas reproduire les votes réels.

Les identités et les podiums sont conservés même après un transfert ou une retraite. La sauvegarde enregistre aussi si la cérémonie a été vue : elle ne se redéclenche pas après un chargement. Chaque année produit une seule édition.

## À propos

Ajout d'un **remerciement à Thorn Atari** dans les catégories « Le jeu » et « Technologies et crédits », et dans `CREDITS.md`.

## Sauvegardes et vérification

Le nouveau format **26** conserve les éditions du Ballon d'or. Les parties aux formats **23, 24 et 25** restent lisibles ; aucune ancienne distinction fictive n'est ajoutée à leur historique. Une ancienne build ne peut pas lire les nouvelles sauvegardes au format 26.

```powershell
.\build-win64.ps1 -Compiler 'C:\outils\llvm-mingw\bin\clang++.exe' -Raylib 'C:\outils\raylib-5.5_win64_mingw-w64' -Tests
```

Les tests couvrent la date, l'arrêt du calendrier, les catégories, l'unicité des éditions, les sauvegardes avant/après cérémonie, la reprise du prochain match, l'année suivante et les anciennes parties. Les tests France, nations et options de build 1 sont conservés. Résultats réels dans `VALIDATION.txt` ou `verification/build-and-tests.txt`.

Les corrections de build 1 restent actives. La France compte maintenant **3 761 équipes et 327 groupes officiellement importés**. **Toute la France n'est pas terminée** : les ligues et districts non couverts utilisent encore le générateur. Le détail actuel est dans `data/france_2627_coverage.json` ; les rapports des builds précédentes décrivent leur périmètre historique, pas la couverture de cette livraison. Les anciennes carrières gardent leurs compositions sauvegardées : démarrer une nouvelle carrière 2026/27 pour utiliser toutes les nouvelles compositions.
