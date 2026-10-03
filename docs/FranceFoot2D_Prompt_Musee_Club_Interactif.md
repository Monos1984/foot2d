# France Foot 2D / Super Soccer World
## Prompt de chantier — Musée du club interactif

---

# PROMPT À DONNER POUR L'IMPLÉMENTATION

Tu travailles sur le code source de **France Foot 2D / Super Soccer World**.

## Objectif principal

Transformer l'infrastructure actuellement appelée **Musée du club** en un véritable système interactif de carrière, capable de conserver et présenter toute l'histoire construite par le joueur au fil des saisons.

Le musée doit devenir la **mémoire permanente du club**.

Il doit fonctionner principalement dans les modes **Carrière** et exploiter automatiquement les données déjà produites par le jeu :
- saisons
- classements
- promotions
- relégations
- trophées
- coupes
- finales
- joueurs
- statistiques
- records
- stades
- affluences
- entraîneurs
- distinctions
- événements historiques

Le système doit rester indépendant du moteur de match : ne pas modifier inutilement la logique terrain pour ce chantier.

---

# 1. Philosophie générale

Le musée doit raconter l'histoire du club.

Au début d'une carrière avec un petit club, le musée peut être presque vide.

Après 10, 20 ou 30 saisons, il doit devenir une véritable archive comprenant :
- trophées
- joueurs emblématiques
- records
- saisons historiques
- grands matchs
- montées
- stades
- entraîneurs
- chronologie du club

Le joueur doit pouvoir regarder plusieurs décennies de carrière et comprendre immédiatement comment son club a évolué.

---

# 2. Disponibilité

Le musée doit être disponible principalement dans :

- Carrière entraîneur
- Carrière club
- Carrière longue durée
- autres modes de gestion utilisant un club persistant

Le mode Simple / Match amical / Tournoi rapide n'a pas besoin de ce système complet.

---

# 3. Évolution de l'infrastructure

Le musée peut évoluer selon le niveau d'infrastructure du club.

Exemple :

## Niveau 0
```text
Archives du club
```

Fonctions :
- palmarès
- saisons
- records simples

## Niveau 1
```text
Salle des trophées
```

Ajoute :
- trophées
- finales
- grands joueurs
- grands matchs

## Niveau 2
```text
Musée du club
```

Ajoute :
- chronologie complète
- stades historiques
- maillots historiques
- entraîneurs
- capitaines
- panthéon
- records détaillés

L'infrastructure ne doit jamais supprimer les données historiques.
Elle ne fait qu'améliorer leur présentation.

---

# 4. Structure de données recommandée

Créer une structure centralisée du type :

```text
ClubHistory
{
    clubId

    seasons[]
    trophies[]
    finals[]
    promotions[]
    relegations[]
    records
    legendaryPlayers[]
    hallOfFame[]
    legendaryMatches[]
    managers[]
    captains[]
    stadiumHistory[]
    shirtHistory[]
    timeline[]
}
```

Éviter de reconstruire en permanence tout l'historique en parcourant l'intégralité de la sauvegarde.

Le système doit mettre à jour les archives au moment où les événements ont lieu.

---

# 5. Historique des saisons

Chaque saison terminée doit générer une entrée.

Exemple :

```text
SeasonHistory
{
    seasonId
    yearStart
    yearEnd

    competition
    division
    finalPosition

    played
    wins
    draws
    losses

    goalsFor
    goalsAgainst

    points

    bestScorer
    bestAssist
    bestPlayer

    managerId

    averageAttendance
    maxAttendance

    budgetStart
    budgetEnd

    promoted
    relegated

    trophiesWon[]
}
```

Afficher une fiche consultable par saison.

---

# 6. Salle des trophées

Afficher tous les trophées remportés par le club.

Catégories :

- championnat
- coupe nationale
- coupe régionale
- coupe départementale
- coupe européenne
- coupe mondiale
- trophée des champions
- compétition de jeunes
- compétition féminine
- compétition personnalisée

Pour chaque trophée :

```text
TrophyHistory
{
    competitionId
    competitionName
    season
    dateWon
    opponentFinal
    scoreFinal
    venue
}
```

Prévoir :
- nombre total de trophées
- trophées par compétition
- première victoire
- dernière victoire
- record de victoires

---

# 7. Finales

Créer une section distincte regroupant toutes les finales jouées.

Pour chaque finale :

- compétition
- saison
- date
- adversaire
- stade
- score
- résultat
- prolongation
- tirs au but
- capitaine
- meilleur joueur
- composition si disponible

Une finale perdue doit également être mémorisée.

---

# 8. Grandes montées

Ajouter une section spécifique aux promotions.

Exemple :

```text
PromotionHistory
{
    season
    fromDivision
    toDivision
    finalPosition
    managerId
    topScorer
}
```

Très important pour les clubs amateurs.

Exemple de chronologie :

```text
2027/28 — Champion de D3 District
2028/29 — Promotion en D1 District
2031/32 — Promotion en R3
2036/37 — Promotion en National 3
2042/43 — Première accession en Ligue 1
```

---

# 9. Joueurs légendaires

Créer un système automatique de score historique.

Exemple conceptuel :

```text
legendScore =
matches * coefficient
+ goals
+ assists
+ seasonsAtClub
+ captainMatches
+ trophies
+ individualAwards
+ loyaltyBonus
+ recordBonus
```

Ne pas utiliser uniquement le niveau général du joueur.

Un joueur moyen mais présent pendant 15 saisons peut devenir une légende.

Critères possibles :
- matchs
- buts
- passes
- clean sheets
- saisons
- capitanat
- titres
- fidélité
- records détenus
- distinctions
- importance dans une promotion ou une finale

---

# 10. Panthéon / Hall of Fame

Créer une distinction supérieure à "joueur légendaire".

Exemple :

```text
HallOfFame
```

Conditions possibles :
- très haut legendScore
- record historique
- très longue carrière au club
- plusieurs trophées majeurs
- capitaine emblématique
- choix manuel possible par le joueur

Prévoir un nombre raisonnable de membres.

---

# 11. Meilleurs buteurs historiques

Créer plusieurs classements :

- buts toutes compétitions
- buts championnat
- buts coupes
- buts européens
- buts sur une saison
- meilleur ratio buts/match

Même principe pour :

- passes décisives
- clean sheets
- apparitions
- cartons éventuellement
- sélections comme capitaine

---

# 12. Records du club

Ajouter un écran Records.

Records minimum :

## Joueurs
- plus de matchs
- plus de buts
- plus de passes
- plus de clean sheets
- plus jeune joueur
- plus jeune buteur
- plus vieux joueur
- plus vieux buteur
- plus longue présence au club

## Matchs
- plus grosse victoire
- plus grosse défaite
- match avec le plus de buts
- plus longue série sans défaite
- plus longue série de victoires
- plus longue série sans encaisser

## Saison
- plus de points
- plus de victoires
- plus de buts
- moins de buts encaissés
- meilleure différence de buts

## Club
- record d'affluence
- record d'abonnés
- plus gros transfert entrant
- plus gros transfert sortant
- plus gros budget
- plus grande valeur d'effectif

Chaque record doit conserver :
- valeur
- détenteur
- date
- saison
- compétition si nécessaire

---

# 13. Grandes dates

Créer automatiquement des événements historiques.

Exemples :

- création du club
- premier match professionnel
- première promotion
- première accession au niveau national
- première accession au niveau professionnel
- première accession en Ligue 1
- premier trophée
- première Coupe de France
- première qualification européenne
- premier match européen
- première victoire européenne
- première finale européenne
- premier titre européen
- première saison invaincue
- record historique
- nouveau stade
- agrandissement majeur
- changement de nom du stade
- record d'affluence
- joueur dépassant un record historique

---

# 14. Chronologie du club

Créer une frise chronologique consultable.

Exemple :

```text
2027
Création du club

2029
Première montée

2032
Champion de Régional 2

2037
Première accession au National

2042
Première accession en Ligue 1

2046
Vainqueur de la Coupe de France

2048
Premier match européen
```

Filtres possibles :

- Tous
- Trophées
- Montées
- Joueurs
- Stades
- Records
- Europe
- Direction

---

# 15. Matchs légendaires

Ajouter un système automatique.

Un match peut devenir historique s'il correspond à certains critères :

- finale
- match de montée
- match de titre
- victoire contre un très gros adversaire
- remontée spectaculaire
- très gros score
- séance de tirs au but
- record d'affluence
- premier match européen
- première victoire européenne
- derby exceptionnel
- record battu

Structure :

```text
LegendaryMatch
{
    matchId
    date
    season
    competition

    homeTeam
    awayTeam

    score
    extraTime
    penalties

    venue
    attendance

    reason[]
}
```

---

# 16. Entraîneurs historiques

Créer une galerie des entraîneurs.

Pour chaque entraîneur :

- nom
- date d'arrivée
- date de départ
- matchs
- victoires
- nuls
- défaites
- pourcentage de victoires
- promotions
- trophées
- meilleure performance
- durée
- raison du départ si connue

Prévoir également :
- entraîneur le plus titré
- entraîneur le plus longtemps en poste
- meilleur pourcentage de victoires

---

# 17. Capitaines historiques

Mémoriser :

- capitaine
- saisons
- matchs comme capitaine
- trophées comme capitaine
- promotions comme capitaine

Créer éventuellement :

```text
CaptainHistory[]
```

---

# 18. Maillots historiques

Si le système graphique le permet :

Mémoriser chaque maillot utilisé par saison importante.

Au minimum stocker les paramètres nécessaires pour le reconstruire :

```text
ShirtHistory
{
    season
    type
    colors
    pattern
    sponsor
    manufacturer
}
```

Types :

- domicile
- extérieur
- troisième

Priorité aux saisons :
- titre
- montée
- finale
- première campagne européenne

Ne pas stocker inutilement de grosses images si les maillots peuvent être régénérés depuis des paramètres.

---

# 19. Stade historique

Créer une vraie histoire des stades.

Structure possible :

```text
StadiumHistory
{
    stadiumName
    startSeason
    endSeason

    capacityStart
    capacityEnd

    location
    recordAttendance

    renovations[]
}
```

Événements :
- construction
- déménagement
- agrandissement
- changement de nom
- naming
- rénovation
- capacité modifiée

Afficher :

```text
Stade municipal
2027–2038
Capacité : 1 200 -> 4 500

Stade Jean-Moulin
2038–2051
Capacité : 8 000 -> 22 000

Super Soccer Arena
2051–
Capacité : 42 000
```

---

# 20. Saisons historiques

Identifier automatiquement les saisons particulièrement importantes.

Critères :

- titre
- promotion
- doublé
- triplé
- invincibilité
- record de points
- parcours européen majeur
- meilleure saison de l'histoire

Créer un écran permettant de revoir :

- classement
- parcours
- trophées
- meilleur joueur
- meilleur buteur
- entraîneur
- stade
- budget
- affluence

---

# 21. Supporters et histoire

Si les données existent, ajouter :

- record d'abonnés
- record d'affluence
- moyenne d'affluence historique
- meilleur taux de remplissage
- saison avec plus forte progression de supporters

Événements :
- première fois à guichets fermés
- record d'abonnés
- premier déplacement européen
- célébration majeure

---

# 22. Valeur historique du club

Créer éventuellement un indicateur interne.

Exemple :

```text
clubPrestigeHistoryScore
```

Facteurs :
- titres
- niveau atteint
- longévité au plus haut niveau
- performances européennes
- légendes
- records
- affluence
- historique

Cet indicateur ne doit pas remplacer la réputation actuelle.

Il représente le poids historique du club.

---

# 23. Interface

Créer une page principale :

```text
MUSÉE DU CLUB
```

Sous-sections possibles :

```text
Accueil
Trophées
Saisons
Chronologie
Légendes
Records
Finales
Grands matchs
Entraîneurs
Stades
Maillots
```

## Accueil

Afficher en résumé :

- âge du club
- nombre de trophées
- meilleur joueur historique
- meilleur buteur
- record de matchs
- plus grande victoire
- record d'affluence
- stade actuel
- dernière grande date

---

# 24. Navigation

Le musée doit être agréable à explorer.

Prévoir :
- filtres par période
- filtres par compétition
- tri
- fiches détaillées
- retour rapide
- navigation par saison

Éviter un seul écran surchargé.

---

# 25. Mise à jour automatique

Créer des fonctions centralisées.

Exemples :

```text
historyOnSeasonEnd()
historyOnTrophyWon()
historyOnPromotion()
historyOnFinalPlayed()
historyOnRecordBroken()
historyOnStadiumChanged()
historyOnManagerChanged()
historyOnPlayerRetired()
historyOnLegendaryMatch()
```

Éviter de disperser la logique dans des dizaines de fichiers si possible.

---

# 26. Fin de saison

À chaque fin de saison :

1. enregistrer le résumé de saison
2. enregistrer classement et statistiques
3. détecter trophées
4. détecter promotion / relégation
5. recalculer records
6. mettre à jour joueurs légendaires
7. mettre à jour entraîneur
8. identifier saison historique
9. ajouter événements à la chronologie
10. vérifier nouvelles entrées au Hall of Fame

---

# 27. Événements immédiats

Certains événements doivent être enregistrés immédiatement.

Exemples :

- trophée remporté
- finale
- record battu
- nouveau stade
- record d'affluence
- grande victoire
- premier match européen
- transfert record

Ne pas attendre obligatoirement la fin de saison.

---

# 28. Club créé par le joueur

Le système doit être particulièrement intéressant pour les clubs créés.

Au départ :

```text
Archives presque vides
0 trophée
0 légende
0 record significatif
```

Puis l'histoire se construit exclusivement à partir de la carrière.

C'est une priorité du design.

---

# 29. Clubs existants

Pour les clubs réels déjà présents dans la base :

Prévoir deux possibilités :

## A. Historique initial minimal
- année de création
- stade
- quelques trophées connus

## B. Historique complet plus tard
- palmarès réel
- grands joueurs historiques
- anciens stades

Ne pas bloquer le chantier actuel sur la saisie complète de l'histoire réelle de tous les clubs.

L'objectif immédiat est surtout de conserver l'histoire générée par la partie.

---

# 30. Persistance / sauvegarde

Toutes les données du musée doivent être sauvegardées avec la carrière.

Attention :
- ne pas perdre les anciennes saisons
- ne pas écraser les records précédents
- ne pas créer de doublons
- gérer les changements de club du joueur humain
- chaque club doit conserver son propre historique

---

# 31. Performance

Une carrière peut durer plusieurs dizaines de saisons.

Le système doit rester performant.

Éviter :
- rescanner tous les matchs de 30 saisons à chaque ouverture du musée
- recalculer tous les records en permanence
- stocker des données inutiles très lourdes

Préférer :
- événements persistants
- statistiques agrégées
- mise à jour au moment opportun

---

# 32. Séparation avec le moteur de match

Ne pas restructurer le moteur de match dans ce chantier.

Le musée peut récupérer les résultats et événements déjà produits.

Si une donnée manque réellement, ajouter seulement un petit événement ou champ de sortie du match.

Exemple :

```text
MatchHistoricalSummary
```

mais ne pas modifier le comportement du football pour le musée.

---

# 33. Première version recommandée

Pour une première intégration complète, prioriser :

1. structure `ClubHistory`
2. historique des saisons
3. trophées
4. finales
5. promotions
6. records
7. chronologie
8. joueurs légendaires
9. entraîneurs historiques
10. stade historique
11. interface musée

Puis ajouter ensuite :
- maillots
- Hall of Fame plus poussé
- supporters
- visuels décoratifs
- photos / trophées graphiques

---

# 34. Critères de validation

Le chantier est considéré fonctionnel si :

- chaque club possède un historique indépendant
- une saison terminée est conservée
- les trophées apparaissent automatiquement
- les promotions sont enregistrées
- les finales gagnées et perdues sont conservées
- les records sont persistants
- les joueurs légendaires peuvent être identifiés
- les entraîneurs sont archivés
- les changements de stade sont conservés
- une chronologie est générée
- les données restent disponibles après sauvegarde / chargement
- le musée fonctionne après plusieurs saisons
- aucun moteur de match n'est cassé
- aucun recalcul lourd inutile n'est effectué à chaque ouverture

---

# 35. Test de validation conseillé

Créer une partie de test avec un club de bas niveau.

Simuler plusieurs saisons avec :

```text
Saison 1 :
- montée

Saison 2 :
- maintien
- record d'affluence

Saison 3 :
- titre de champion
- promotion

Saison 4 :
- finale de coupe perdue

Saison 5 :
- coupe remportée
- changement de stade
```

Vérifier que le musée affiche correctement :

- 5 saisons
- 2 promotions
- 2 trophées
- 2 finales
- record d'affluence
- ancien stade
- nouveau stade
- meilleur buteur historique
- entraîneur
- chronologie complète

---

# 36. Important

Ne pas transformer le musée en simple page décorative.

Le but est d'obtenir un véritable **système historique persistant**.

L'interface doit être construite au-dessus de données robustes.

La priorité est donc :

```text
DATA / HISTORIQUE
        ↓
RÈGLES DE MISE À JOUR
        ↓
SAUVEGARDE
        ↓
INTERFACE
        ↓
POLISSAGE VISUEL
```

---

# 37. Résultat recherché

Après trente saisons, un joueur ayant commencé avec un petit club doit pouvoir ouvrir son musée et retrouver toute son histoire :

```text
2027 — Début en District
2029 — Première montée
2033 — Accession en Régional
2039 — Première accession au National
2045 — Première montée en Ligue 2
2048 — Première accession en Ligue 1
2051 — Première Coupe de France
2052 — Première qualification européenne
2055 — Premier titre de champion
2058 — Nouveau stade de 45 000 places
```

Il doit pouvoir retrouver :
- les joueurs qui ont construit cette histoire
- les entraîneurs
- les grands matchs
- les trophées
- les records
- les stades
- les saisons les plus importantes

Le musée doit devenir l'un des écrans les plus attachants du mode Carrière.

---

# 38. Livrables attendus après implémentation

À la fin du chantier :

1. code source modifié
2. structure `ClubHistory` ou équivalent
3. système de mise à jour automatique
4. sauvegarde / chargement
5. écran Musée
6. sections prioritaires fonctionnelles
7. données de test
8. documentation mise à jour
9. liste des éléments restant à polir graphiquement
10. aucun changement inutile du moteur de match
