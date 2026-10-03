# France Foot 2D / Super Soccer World — 00.03.00 build 19

Le téléchargement contient `FranceFoot2D_v00.03.00_build19` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build19_Source` (code source complet). Format de sauvegarde inchangé (version 34).

## Gardien en 1 contre 1
- Le gardien ne fonce plus droit sur l'attaquant. Il sort sur l'axe ballon-but pour fermer l'angle, en gardant 2 à 3 m de distance.
- Il se place sur ses appuis quand l'attaquant arrive à une dizaine de mètres, puis glisse latéralement pour rester sur l'axe.
- Il ne plonge dans les pieds que si l'attaquant vient sur lui. Face à un crochet sur le côté, il reste debout et suit.
- Test scripté « humain qui contourne le gardien » :

| Gardien | Buts marqués |
|---|---|
| Build 18 | 100 % |
| Build 19 | environ 20 % |

- En face-à-face, l'IA attaquante frappe plus tôt, avant que le gardien ne ferme l'angle.

## Hors-jeu
Nouveau module `src/offside.cpp`, qui reprend la logique existante (photographie puis participation).
- **Photographie** : la position est figée au moment où un joueur joue le ballon. Chaque nouveau jeu d'un partenaire crée une nouvelle phase.
- **Ligne de hors-jeu** : calculée à un seul endroit (`offsideLine`), qui sert aussi à l'IA. Marge en mètres : positive quand le joueur est hors-jeu, négative quand il est en jeu.
- **Parade du gardien** : ne remet plus un attaquant hors-jeu en jeu. Le contrôle général qui annulait la phase à chaque toucher d'un adversaire est supprimé.
- **Touchers adverses** :

| Toucher d'un adversaire | Effet sur le hors-jeu |
|---|---|
| Déviation, contrôle manqué, tacle | la phase continue |
| Parade du gardien | la phase continue |
| Contrôle, passe, prise du gardien, dégagement | nouvelle situation |
| Ballon sur le poteau (aucun toucher) | la phase continue |

- **Relance du gardien pendant le jeu** : le hors-jeu s'applique (humain et IA).
- **Six mètres, corner, touche** : pas de hors-jeu sur la réception directe, mais un nouveau jeu d'un partenaire relance une phase normale.
- **Participation active** : un joueur signalé est sanctionné s'il :
  - joue le ballon ;
  - profite d'un rebond, d'une parade ou d'un poteau ;
  - dispute le ballon à un adversaire ;
  - gêne le gardien (contact) ;
  - masque sa vue : trajectoire ballon-gardien, distance et angle.

  Un joueur passif loin de l'action n'est pas sanctionné.
- **But marqué alors que le drapeau se lève** : le but est refusé.
- **Fins de phase centralisées** : nouveau jeu d'un partenaire, jeu volontaire adverse, sortie du ballon, faute (le hors-jeu antérieur prime), but, coup de pied arrêté.

## Arbitres assistants
- Deux assistants par match, avec un nom et des caractéristiques de 0 à 100 :
  - précision hors-jeu ;
  - placement ;
  - concentration ;
  - expérience ;
  - communication ;
  - décision ;
  - réputation.
- Leur niveau suit celui de l'arbitre central, avec une grande variété individuelle : un excellent assistant de district reste possible.
- L'équipe arbitrale est la même pour un même match (reproductible).
- Chaque assistant couvre une moitié de terrain. La décision revient à celui qui est responsable de la zone.
- **La vérité et la décision sont séparées.** La position réelle n'est jamais modifiée. L'assistant juge la marge avec une petite erreur de perception, qui dépend de :
  - sa précision ;
  - son placement ;
  - son expérience ;
  - la vitesse de croisement de la ligne ;
  - un léger effet de fatigue en fin de match.
- Erreurs possibles selon la marge réelle :

| Marge réelle | Erreur de l'assistant |
|---|---|
| Plus de 40 cm | jamais, la décision est toujours correcte |
| 15 à 40 cm | exceptionnelle |
| Moins de 15 cm | crédible, dans les deux sens (hors-jeu non signalé ou faux hors-jeu) |

- Les décisions sont déterministes (graine du match, numéro de séquence, assistant), donc stables pour les replays et les tests.
- Pour l'humain comme pour l'IA, les règles sont identiques, et les erreurs ne favorisent aucun camp.
- **Drapeau** : levé par l'assistant responsable après un petit délai humain, plus long sur une position serrée, avant le coup de sifflet.
- **Commentaires** : « Le drapeau se lève… Hors-jeu très serré. Décision limite de l'arbitre assistant. », et le motif quand il y en a un (dispute le ballon, gêne le gardien…). Le jeu n'annonce jamais qu'il y a eu erreur.
- Les assistants sont présentés au coup d'envoi.

## IA
- **Attaquants** : ils ne restent plus toujours 50 cm derrière la ligne. Leur marge dépend du placement et du sang-froid (environ 0,3 à 0,6 m). Un attaquant peu attentif part parfois trop tôt : les hors-jeu viennent des appels, pas de positions truquées.
- **Piège du hors-jeu** : avec une ligne haute, la défense remonte d'un cran quand le porteur adverse est au milieu. Un défenseur peu discipliné traîne et casse la ligne.

## Débogage
- Variable d'environnement `FOOT_OFFSIDE_DEBUG=1` : écrit `offside_debug.log` à côté du jeu.
- Le journal contient : séquence, passeur, joueur, ballon, ligne, marge réelle, marge perçue, drapeau, vitesse de la ligne, assistant et ses notes, type de participation, motif de fin de phase.
- Mode test `FOOT_TEST=offside` : passe vers un attaquant hors-jeu.

## Vérifications
- **`tools/test_offside.cpp`** : 90 contrôles, sur les deux équipes et les deux sens d'attaque. Ils couvrent :
  - les hors-jeu flagrants et les positions largement en jeu ;
  - la parade, la déviation, le jeu volontaire, le poteau ;
  - la relance du gardien, le six mètres, le corner, la touche, le nouveau toucher après un corner ;
  - la gêne du gardien, le joueur passif, le duel ;
  - le passeur humain et le passeur IA ;
  - le but refusé.

  Statistiques sur 2000 positions serrées (−20 à +20 cm) :

| Assistant | Taux d'erreur |
|---|---|
| Bon | 8 % |
| Moyen | 17 % |
| Faible | 25 % |

  Les erreurs vont dans les deux sens. Sur les positions flagrantes, les décisions sont justes à 100 % (3000 décisions). Dans la zone de 15 à 40 cm, l'assistant faible se trompe dans 1,25 % des cas.
- **`tools/test_gk1v1.cpp`** : 1 contre 1 face à un humain qui contourne le gardien.
- **Autres tests natifs qui passent** : test_build18, test_build4_roles, test_build10_match, test_build9_supporters, test_save, test_fight, test_pens, test_build11_stats, test_pensonly.
- **Moyenne de buts IA contre IA** (matchs courts) : environ 0,6 en mode simple et environ 0,4 en carrière, comme en build 18.
- **`test_human`** : deux matchs sur cinq s'arrêtent avant la fin sur 0-0. Après la prolongation, la séance de tirs au but attend un choix du joueur humain que le script ne fait pas.
- **Pas fait** : arbitres persistants (progression, retraite, historique) et VAR.
- L'exe Windows est compilé mais n'a pas été lancé sous Windows. Les écrans ont été vérifiés sur une version Linux.
