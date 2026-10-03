# France Foot 2D / Super Soccer World — 00.03.00 build 20

Le téléchargement contient `FranceFoot2D_v00.03.00_build20` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build20_Source` (code source complet). Format de sauvegarde inchangé.

## Plantage corrigé (journal envoyé : écran 42 = tirage au sort)
- **Cause** : le tirage d'un tour de coupe à plus de 16 équipes triait les affiches avec une valeur aléatoire recalculée à chaque comparaison. Un tri avec un comparateur incohérent peut lire hors du tableau (SIGSEGV).
- **Correction** : la valeur de chaque affiche est tirée une seule fois avant le tri.
- **Vérification** : une saison complète a été jouée pour un club de chaque niveau du championnat de France, en affichant chaque tirage à l'écran (équipes de jeunes et réserves contrôlées comprises). Il n'y a plus de plantage, alors que l'ancien code plantait dès le premier club.

## Match
- **Passe en retrait** : le gardien ne peut plus prendre à la main une touche ou un coup de pied arrêté joué par un partenaire. Il joue au pied, et un commentaire le signale. Cela s'ajoute aux passes au pied déjà gérées. Une passe de la tête vers le gardien reste autorisée.
- **Gardien moins infaillible** :
  - allonge réduite sur les frappes de loin (plus de 20 m) et sur les tirs à bout portant (moins de 9 m) ;
  - bonus du gardien de l'ordinateur face au joueur humain diminué (réflexes et sorties dans les pieds).
- **Changement de maillot à la mi-temps** depuis l'écran de la pause (domicile, extérieur ou troisième tenue), avec une alerte si les couleurs sont proches de celles de l'adversaire.
- **Tenue des arbitres** : noire, jaune, verte, rouge ou bleue. Elle se choisit dans l'avant-match ou dans les options, et elle est mémorisée.
- **Temps additionnel** : incrustation façon scénette TV (gros plan sur le 4e arbitre et son panneau lumineux) avec le bandeau « TEMPS ADDITIONNEL : N MIN ».
- **Tirage au sort** : nouveau panneau, avec :
  - un fond en dégradé et des ornements dorés ;
  - les onglets aux couleurs des équipes ;
  - les deux capitaines avec leur brassard ;
  - l'arbitre qui lance la pièce ;
  - des paillettes aux couleurs du vainqueur.

## Arbitres
- **Arbitres assistants** :
  - ils lèvent le drapeau pour les touches, pointé vers le camp de l'équipe qui en bénéficie ;
  - ils signalent aussi les corners et les six mètres ;
  - ils rejoignent le point de touche le long de la ligne.
- **Arbitre central** :
  - il tend le bras vers le but de l'équipe qui bénéficie d'un coup franc, lève le bras pour un coup franc indirect et désigne le point de penalty ;
  - sur un coup franc, il se rend sur place et recule pour placer le mur ;
  - il anticipe le jeu et s'écarte de la trajectoire des passes.

## Ambiance
- **Trois nouveaux chants** :
  - « AUX ARMES ! » : un virage lance, l'autre répond, puis « Nous sommes les… et nous allons gagner ! » ;
  - le riff « Oh oh oh oh oh oh oh » repris par tout le stade ;
  - le chant sauté (« Qui ne saute pas… »), avec les tribunes qui tremblent.
- **Répertoire** : il passe à cinq chants par humeur (calme, encouragements, fête).
- **Sifflets** : ils arrivent par vagues (entrée des joueurs, quelques minutes de temps en temps, avant la pause, fin de match) et ne durent plus tout le match.
- **Musiques** : une nouvelle musique coupe la précédente (menu, jingle, musique d'entrée des joueurs, hymne).

## Carrière
- **Mode Full Manager** : les temps forts sont supprimés. Le match se regarde en entier, sur la durée choisie par le joueur. La touche Tab règle la vitesse (x1, x2, x4) pendant les phases calmes.
- **Engagements de la saison** : l'écran est refait.
  - Bandeau aux couleurs du club (stade, division, saison).
  - Une carte par équipe du club : l'équipe première en pleine largeur, puis la réserve, les jeunes et la section féminine sur deux colonnes.
  - Un pictogramme par compétition (championnat, coupe, Europe) et des étiquettes pour le tour d'entrée.
- **Menu de la carrière** : catégories en majuscules sans accent (COMPETITIONS, PALMARES…).

## Menus
- **Options** : présentation en cartes par rubrique (MATCH, AFFICHAGE, SON & MUSIQUE, COMMANDES), avec une pastille par valeur (oui en vert, non en rouge). La tenue des arbitres y est ajoutée.
- **Entraînement** :
  - deux nouveaux exercices : face-à-face avec le gardien (départ de 25 m) et frappes de loin (22 à 30 m, frapper avant la surface) ;
  - séries de 10 essais, avec un bilan sur 3 étoiles ;
  - un record par exercice, enregistré dans `entrainement.txt` et affiché dans le menu.

## Vérifications
- **Écran de tirage** : mode test `FOOT_TEST=drawall`, qui joue des saisons complètes avec l'affichage de tous les tirages. Il passe.
- **Données du tirage** : `tools/test_draws_screen.cpp`, qui contrôle les données lues par l'écran de tirage. Il passe.
- **Autres tests natifs qui passent** : test_offside, test_build18, test_build4_roles, test_pens, test_fight, test_build10_match, test_save, test_build9_supporters, test_match.
- **Moyenne de buts IA contre IA** : 0,68 (matchs courts).
- **`tools/test_gk1v1`** : le script de crochet est trop grossier pour mesurer l'effet des réglages du gardien. Il convertit environ 20 % de ses face-à-face, et une bonne partie de ses tirs partent à côté.
- **Captures vérifiées** : options, tirage au sort, entraînement, engagements de la saison.
- **Pas testé à l'écran** : changement de maillot à la mi-temps, nouveaux chants (audio), gestes des arbitres, incrustation du temps additionnel, Full Manager.
- L'exe Windows est compilé mais n'a pas été lancé sous Windows.
