# France Foot 2D / Super Soccer World — 00.03.00 build 11

Extraire le ZIP Windows 64 bits puis lancer FranceFoot2D.exe. Le programme conserve le format de sauvegarde 33 de la build 10.

## Fiches de joueurs

Les carrières disposent de boutons visibles Fiche générale, Postes / apprentissage et Personnalité, communs aux trois pages. Le bouton de la page active apparaît en jaune. Tab reste disponible. L'historique de personnalité possède son propre bouton de retour aux traits. Le portrait est isolé au-dessus des caractéristiques ; les informations et le tableau des saisons ont été repositionnés. Le mode Simple n'affiche pas les options de gestion réservées à la carrière complète.

## Matchs et résultat

En retransmission TV, l'indicateur d'ambiance et de chants est placé sous le logo de la chaîne. L'affluence du match joué est affichée directement sous le score.

Le résultat propose trois onglets cliquables, également accessibles avec Gauche/Droite : feuille de match, jeu et attaque, discipline et reprises. Les statistiques comprennent possession, tirs tentés, tirs cadrés, tirs bloqués, montants, passes tentées et réussies, précision des passes, arrêts du gardien, corners, touches, coups francs, sorties de but, penalties accordés, fautes, hors-jeu, cartons, blessures et remplacements. Les séances de tirs au but sont exclues des tirs du match.

Les tirs tentés sont classifiés par le moteur selon la frappe et sa direction vers le but ; une passe explicitement adressée à un partenaire est exclue. Un tir cadré est compté lors d'un but ou d'une intervention du gardien sur une trajectoire dirigée dans le cadre. Un blocage par un défenseur et un montant restent des catégories distinctes. Une passe est réussie lors du premier contact d'un coéquipier ; un contact adverse la rend manquée. Les remises en jeu proviennent des décisions du moteur. La précision des passes est arrondie à l'entier.

Ces compteurs concernent le match joué dans le moteur 2D et son écran de fin. Ils ne créent pas de statistiques fictives pour les simulations rapides ou les anciennes archives, et les nouveaux détails ne sont pas conservés dans l'historique des saisons. xG, distance parcourue et suivi individuel des contacts ne sont pas ajoutés. Cette livraison n'annonce pas des mesures qui ne sont pas disponibles.

## Vérification

Compilation Windows x86_64, bibliothèques intégrées ; 14 scénarios d'écran, dont navigation par boutons, fiche Simple, trois pages de résultat, pause et retransmission TV. Les chiffres des captures de résultat sont des fixtures de vérification de présentation.

Tests ciblés : 13 contrôles des compteurs de match, 137 contrôles de personnalité dans le match, 1 934 contrôles des postes et 3 130 contrôles des fonctionnalités de saison et du tournoi des pôles, soit 5 214 assertions. Les tests couvrent les passes vers l'avant près du but, interceptions, doubles contacts, buts, arrêts, blocages, montants, remises en jeu, exclusion des tirs au but et remise à zéro des compteurs.

Les données France restent inchangées : 3 761 équipes officiellement importées et 327 groupes, couverture partielle. Les rapports des builds précédentes documentent les chantiers encore ouverts.
