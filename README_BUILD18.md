# France Foot 2D / Super Soccer World — 00.03.00 build 18

Le téléchargement contient `FranceFoot2D_v00.03.00_build18` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build18_Source` (code source complet).

Sauvegardes : format 34. Les sauvegardes des builds précédentes se chargent ; celles de la build 18 ne se chargent pas dans une build plus ancienne.

## Stabilité
- Gardien : `formationTarget()` traite le gardien (slot 0) à part, avant toute lecture des tableaux de formation (indexés `slot - 1`). Le gardien se place devant sa ligne, dans l'axe ballon-but.
- Encodage : textes doublement encodés corrigés sur les écrans Personnalité et Postes / rôles / consignes (« Carrière », « Défense », « rôles », « Personnalité »…).

## Persistance
Nouvel état mensuel enregistré avec la carrière :
- conférence de presse déjà faite pour le match ;
- discussions individuelles du mois ;
- relevé du joueur et de l'entraîneur du mois (buts et points au début du mois).

Ces états ne reposent plus sur des variables `static` : ils survivent à une sauvegarde et un rechargement, et sont remis à zéro en nouvelle carrière.

## Personnalité et discussions
Les discussions individuelles utilisent la personnalité du joueur : susceptibilité, professionnalisme, loyauté, leadership, sang-froid et relation avec l'entraîneur. Les effets restent modérés.
- Un joueur professionnel et peu susceptible accepte mieux un recadrage.
- Un joueur très susceptible peut mal le vivre.
- Une bonne relation avec l'entraîneur réduit le risque de conflit.
- Un leader réagit de façon plus stable.

La relation avec l'entraîneur évolue légèrement après chaque discussion.

## Conférences de presse
- La question s'appuie sur les rivalités des supporters (LOCAL, HISTORICAL, SPORTING, RECENT). Trois nouvelles questions :
  - rivalité historique ;
  - rival sportif ;
  - dernier match tendu.
- La réaction de chaque joueur est modulée légèrement (±2 de moral au plus) par la susceptibilité, le leadership, le sang-froid et le tempérament dans les grands matchs. L'écran indique combien de joueurs s'en nourrissent ou se crispent.

## Cérémonies
La remise du trophée dépend du niveau de la compétition :
- **Montée de district** : table pliante au bord du terrain, délégué du district, bénévole, drapeau du club, petite coupe sur socle en bois, quelques confettis. Bandeau « CHAMPION ET PROMU ».
- **Titre régional** : estrade en bois, panneau des partenaires, coupe argentée à anses, un projecteur. Bandeau « CHAMPION RÉGIONAL ».
- **Coupe de France** : estrade tricolore avec arche dorée, flammes, confettis et rubans bleu-blanc-rouge, bandeau tricolore.
- **Championnat national** : stade assombri autour du podium, halo sur le trophée, flammes, feux d'artifice.
- **Coupe d'Europe** : nuit européenne, estrade bleu nuit étoilée, cinq projecteurs, flammes bleues, feux d'artifice, confettis argent et bleu. Bandeau « VAINQUEUR EUROPÉEN ».

Chaque niveau a son propre commentaire.

## Musée
La salle des trophées est entièrement redessinée :
- vitrine en noyer à coins en laiton, montants dorés, fond en velours aux couleurs du club ;
- étagères en verre, socles en marbre, plaques gravées en laiton ;
- spots avec faisceaux et poussière dans la lumière, reflets sur la vitre.

Les trophées ont une forme propre selon la compétition : grandes oreilles, coupe d'Europe à couvercle, trophée de championnat, coupe nationale dorée, coupe régionale argentée, écusson de district.

## Match (mode Carrière)
Les rôles détaillés modifient réellement les déplacements :
- **Faux neuf** : décroche entre les lignes et attaque la profondeur par moments. Les ailiers montent d'un cran pour profiter de l'espace.
- **Attaquant intérieur** : appels vers l'axe et dans la surface, laisse le couloir au latéral.
- **Latéral offensif** : dépassement côté ballon, appels extérieurs, repli à la perte.
- **Latéral inversé** : rentre dans le milieu à la relance, ressort sur son côté sans le ballon.
- **Mezzala** : demi-espace, courses en diagonale vers la surface.
- **Stoppeur** : sort au contact de l'attaquant qui décroche ; **couverture** : reste juste derrière la ligne, côté ballon.
- **Libéro** : dernier homme sans le ballon, monte d'un cran pour relancer.

L'écran des rôles décrit le comportement de chaque rôle.

Dans tous les modes, **contre-pressing** : juste après une perte de balle, les deux joueurs les plus proches harcèlent le porteur, plus ou moins longtemps selon la consigne de pressing.

## Vérifications
- Le nouveau test `tools/test_build18.cpp` passe (33 contrôles) :
  - gardien ;
  - persistance après sauvegarde / chargement ;
  - rôles ;
  - matchs IA complets en mode Carrière.
- Les tests natifs suivants passent : test_match, test_build4_roles, test_build5_museum, test_build7_director, test_build8_sporting, test_build9_supporters, test_build10_personality, test_build10_match, test_build11_stats, test_fight, test_pens, test_human, test_save, test_build2_fixes.
- test_build3_features et test_ballondor demandent des fichiers de sauvegarde d'anciennes versions, absents de cet environnement.
- Les écrans ont été vérifiés par captures sur une version Linux. L'exe Windows est compilé mais n'a pas été lancé sous Windows.
