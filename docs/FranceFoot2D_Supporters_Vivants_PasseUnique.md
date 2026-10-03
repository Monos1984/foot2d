# France Foot 2D / Super Soccer World
## Prompt de chantier — Supporters plus vivants
### Intégration complète en une seule passe

# Objectif

Transformer les supporters en un véritable système persistant de carrière.

À intégrer en une seule passe :
- fidélité
- exigence
- ferveur
- patience
- abonnés
- liste d'attente
- déplacements
- tifos
- chants
- sifflets
- contestations
- fête de montée
- fête du titre
- affluence record
- boycott exceptionnel
- liens finances
- liens musée
- effets légers en match

Le système doit être complet dans les modes Carrière et simplifié automatiquement en mode Simple.

---

# 1. Structure principale

Créer une structure centralisée :

```text
SupporterProfile
{
    clubId

    totalSupporters
    seasonTicketHolders
    waitingList
    activeCore

    loyalty
    fervor
    expectations
    patience
    localIdentity

    sportingSatisfaction
    managerTrust
    boardTrust
    transferTrust

    atmosphereIndex

    rivalries[]
    attendanceHistory[]
    supporterEvents[]
}
```

Échelle conseillée pour les valeurs internes : 0 à 100.

---

# 2. Valeurs principales

## Fidélité
Influence :
- maintien de l'affluence malgré les mauvais résultats
- renouvellement des abonnements
- patience
- résistance aux crises

## Exigence
Dépend de :
- réputation
- histoire du club
- division
- budget
- résultats récents
- trophées

Un club de District nouvellement promu ne doit pas avoir les mêmes attentes qu'un grand club historique.

## Ferveur
Influence :
- chants
- tifos
- déplacements
- ambiance
- célébrations
- pression dans les grands matchs

## Patience
Influence :
- vitesse d'apparition des sifflets
- contestations
- banderoles
- boycott

## Identité locale
Influence la réaction à :
- jeunes formés au club
- joueurs locaux
- ancien capitaine
- changement de stade
- naming
- vente d'un joueur emblématique

---

# 3. Groupes de supporters

Créer des populations agrégées :

```text
Occasionnels
Abonnés
Noyau actif
Supporters historiques
Supporters internationaux
```

Ne pas simuler chaque individu.

---

# 4. Évolution du nombre de supporters

Facteurs positifs :
- montée
- titre
- coupe
- qualification européenne
- grande série
- recrutement marquant
- nouveau stade
- joueur emblématique
- beau parcours

Facteurs négatifs :
- relégation
- plusieurs mauvaises saisons
- crise financière
- disparition d'identité
- politique sportive très impopulaire

La progression doit rester réaliste et progressive.

---

# 5. Abonnements

Créer une campagne d'abonnements chaque intersaison.

Données :

```text
seasonTicketHolders
waitingList
seasonTicketCapacity
```

Facteurs :
- résultats saison précédente
- montée
- titre
- prix
- capacité
- fidélité
- ferveur
- recrutement
- nouveau stade

Exemple :

```text
Abonnés saison précédente : 4 800
Demandes : 7 200
Capacité abonnements : 6 000
Liste d'attente : 1 200
```

---

# 6. Affluence enrichie

Le calcul doit prendre en compte :

```text
stadiumCapacity
clubSupporters
seasonTickets
opponentPrestige
competitionImportance
derbyIntensity
weather
day/time
ticketPrice
teamForm
leagueLevel
promotionRace
titleRace
relegationBattle
fervor
loyalty
sportingSatisfaction
```

Conserver :
- moyenne saison
- meilleur match
- pire match
- taux de remplissage
- évolution historique

---

# 7. Record d'affluence

Créer un record persistant :

```text
42 318 spectateurs
vs Marseille
Coupe de France
12 février 2041
```

Lorsqu'il est battu :
- notification
- événement supporters
- entrée Musée du club si disponible

---

# 8. Déplacements

Créer une logique de supporters visiteurs selon :
- distance
- rivalité
- importance du match
- division
- jour
- capacité visiteurs
- ferveur
- résultats

Exemple :
un derby à 20 km peut attirer énormément de visiteurs, contrairement à un déplacement ordinaire à 700 km.

---

# 9. Ambiance du stade

Créer :

```text
atmosphereIndex = 0..100
```

Libellés :

```text
CALME
CORRECTE
CHAUDE
TRÈS CHAUDE
BOUILLANTE
HOSTILE
```

Facteurs :
- remplissage
- ferveur
- rivalité
- importance
- satisfaction
- score
- dynamique du match

---

# 10. Effets légers en match

Ne pas créer de gros bonus artificiels.

Effets possibles :
- petite stimulation à domicile
- jeunes plus sensibles aux ambiances hostiles
- penalty sous pression légèrement plus difficile
- sifflets en période de crise
- encouragement après action importante

---

# 11. Chants

Créer plusieurs états :

```text
CHANT_NORMAL
CHANT_FORT
ENCOURAGEMENT
SILENCE_TENDU
SIFFLETS
CELEBRATION
PROTESTATION
```

La fréquence dépend :
- ferveur
- score
- rivalité
- importance
- satisfaction

---

# 12. Tifos

Déclencheurs :
- derby
- finale
- anniversaire du club
- hommage
- titre
- montée
- premier match nouveau stade
- premier match européen
- match historique

Structure :

```text
TifoEvent
{
    type
    date
    matchId
    reason
    intensity
}
```

Les grands tifos peuvent entrer dans le Musée.

---

# 13. Sifflets

Déclencheurs possibles :
- lourde défaite
- série catastrophique
- entraîneur contesté
- joueur contesté
- vente sensible
- prestation très mauvaise

Éviter les sifflets automatiques après chaque défaite.

---

# 14. Contestations

Niveaux :

```text
0 Aucune
1 Mécontentement
2 Banderoles / chants hostiles
3 Manifestation / tribune silencieuse
4 Boycott partiel
5 Boycott exceptionnel
```

Raisons possibles :
- résultats
- direction
- entraîneur
- prix billets
- vente d'une légende
- mauvais mercato
- identité du club
- déménagement
- relégation

---

# 15. Boycott exceptionnel

Doit rester rare.

Exemple de conditions :

```text
boardTrust très faible
+ sportingSatisfaction très faible
+ patience faible
+ événement déclencheur grave
```

Effets :
- affluence en baisse
- ambiance réduite
- revenus billetterie diminués
- pression direction
- durée limitée

---

# 16. Fête de montée

Créer un événement carrière.

Exemple :

```text
FÊTE DE LA MONTÉE
8 500 supporters
Tour du stade
Réception officielle
Présentation équipe
```

L'ampleur dépend :
- niveau atteint
- taille club
- ferveur
- importance historique

---

# 17. Fête du titre

Prévoir plusieurs niveaux :

```text
Titre District
Titre régional
Titre national
Coupe nationale
Titre européen
```

Plus le titre est important, plus la cérémonie est forte.

Prévoir :
- rassemblement
- tour d'honneur
- présentation trophée
- parade
- réception officielle

---

# 18. Rivalités

Créer :

```text
SupporterRivalry
{
    clubId
    intensity
    type
}
```

Types :

```text
LOCAL
HISTORICAL
SPORTING
RECENT
```

Les rivalités influencent :
- affluence
- déplacements
- ferveur
- chants
- pression

Une rivalité sportive peut naître et évoluer avec le temps.

---

# 19. Confiance entraîneur

Créer :

```text
managerTrust
```

Facteurs :
- résultats
- style de jeu
- jeunes
- derbys
- titres
- séries

---

# 20. Confiance direction

Créer :

```text
boardTrust
```

Facteurs :
- finances
- prix
- stade
- mercato
- vente de joueurs
- ambitions
- résultats

---

# 21. Directeur sportif

Si le système existe, créer des impacts :

Positifs :
- recrutement marquant
- jeunes locaux
- conservation des cadres
- bonne vente
- effectif cohérent

Négatifs :
- vente d'une légende
- mercato raté
- absence de renfort
- décisions incomprises

---

# 22. Popularité des joueurs

Créer :

```text
fanPopularity
```

Facteurs :
- ancienneté
- buts
- performances
- fidélité
- capitanat
- formation club
- grands matchs

---

# 23. Vente d'une légende

La réaction dépend :
- âge
- prix reçu
- volonté du joueur
- importance sportive
- remplaçant disponible
- relation avec la direction

Ne pas traiter toutes les ventes comme une trahison.

---

# 24. Retour d'une légende

Créer :

```text
SupporterEvent = LEGEND_RETURN
```

Effets :
- hausse satisfaction
- hausse affluence
- événement spécial

---

# 25. Stade

Les supporters réagissent à :
- nouveau stade
- rénovation
- agrandissement
- changement de nom
- naming
- déménagement

Créer :

```text
FIRST_MATCH_NEW_STADIUM
```

avec affluence et ambiance renforcées.

---

# 26. Page Supporters

Créer une page Carrière :

```text
Résumé
Abonnements
Affluence
Ambiance
Rivalités
Satisfaction
Événements
Historique
```

Exemple :

```text
Supporters : 48 000
Abonnés : 12 500
Liste d'attente : 1 200
Noyau actif : 2 300

Fidélité : 82
Ferveur : 76
Exigence : 68
Patience : 61

Confiance entraîneur : 54
Confiance direction : 61
Satisfaction sportive : 67
```

---

# 27. Événements supporters

Structure :

```text
SupporterEvent
{
    type
    date
    intensity
    reason
    duration
    relatedMatch
    relatedPlayer
}
```

Types :

```text
TIFO
PROTEST
BOYCOTT
TITLE_PARTY
PROMOTION_PARTY
TRIBUTE
RECORD_ATTENDANCE
LEGEND_RETURN
FIRST_MATCH_NEW_STADIUM
HOSTILE_CROWD
```

---

# 28. Notifications

Exemples :

```text
Les supporters préparent un grand tifo pour le derby.
Le club bat son record d'affluence.
Les supporters protestent après la vente du capitaine.
La campagne d'abonnements dépasse les attentes.
Une partie des supporters annonce un boycott du prochain match.
```

---

# 29. Finances

Les supporters doivent influencer :
- billetterie
- abonnements
- boutique
- merchandising
- revenus matchday

Réutiliser les systèmes existants.

---

# 30. Musée du club

Relier automatiquement :
- record d'affluence
- fête de montée
- fête de titre
- premier match nouveau stade
- tifo historique
- grande rivalité
- retour d'une légende

---

# 31. Sauvegarde

Sauvegarder :
- profil supporters
- abonnés
- liste d'attente
- satisfaction
- confiance
- rivalités
- historique affluence
- événements
- boycott actif

---

# 32. Points de mise à jour

Créer des fonctions centralisées :

```text
supportersOnMatchEnd()
supportersOnSeasonEnd()
supportersOnTransfer()
supportersOnPromotion()
supportersOnTitle()
supportersOnTicketPriceChange()
supportersOnManagerChange()
supportersOnStadiumChange()
```

---

# 33. Fin de saison

À chaque fin de saison :

1. évolution du nombre de supporters
2. recalcul de l'exigence
3. campagne abonnements
4. archivage de l'affluence
5. mise à jour records
6. évolution rivalités
7. ajustement confiance entraîneur
8. ajustement confiance direction
9. archivage événements majeurs

---

# 34. Petit club amateur

Le système doit fonctionner en District.

Exemple :

```text
Supporters : 380
Abonnés : 120
Noyau actif : 40
```

Un derby local peut attirer bien plus que la moyenne.

---

# 35. Grand club

Peut avoir :
- énorme base supporters
- abonnements au maximum
- liste d'attente
- ferveur élevée
- exigence très élevée

---

# 36. Mode Simple

Ne pas afficher toute la gestion.

Calculer seulement :
- affluence
- ambiance
- chants
- rivalité
- importance du match

---

# 37. Moteur de match

Ne pas le réécrire.

Ajouter seulement les hooks nécessaires pour :
- ambiance
- chants
- sifflets
- tifos
- réactions tribunes
- effets psychologiques légers

---

# 38. Rendu visuel minimal attendu

Cette passe doit au minimum permettre :
- ambiance variable
- tifo déclenchable
- chants
- sifflets
- célébration montée/titre
- notification record affluence

Le polissage graphique pourra être amélioré ensuite sans refaire la logique.

---

# 39. Tests prioritaires

## Test A — Petit club District
- faible base supporters
- derby
- montée
- campagne abonnements

## Test B — Grand club en crise
- forte exigence
- défaites
- mauvais mercato
- contestation progressive

## Test C — Vente d'une légende
- réaction adaptée au contexte

## Test D — Record d'affluence
- record
- notification
- Musée

## Test E — Nouveau stade
- premier match spécial
- abonnements
- événement historique

## Test F — Boycott
- rare
- impact réel
- durée limitée
- récupération progressive

## Test G — 10 saisons
- pas de doublons
- historiques conservés
- performances correctes

---

# 40. Critères de validation

La passe est terminée lorsque :

- chaque club possède un profil supporters
- fidélité fonctionne
- exigence fonctionne
- ferveur fonctionne
- patience fonctionne
- abonnements fonctionnent
- liste d'attente fonctionne
- affluence est enrichie
- déplacements existent
- tifos existent
- chants et sifflets sont contextuels
- contestations sont progressives
- boycott existe
- fête de montée existe
- fête de titre existe
- record d'affluence est persistant
- rivalités influencent les supporters
- confiance entraîneur/direction existe
- événements sont sauvegardés
- page Supporters existe
- Musée reçoit les grands événements
- finances utilisent les données
- mode Simple reste léger
- moteur de match n'a pas régressé

---

# 41. Architecture recommandée

Séparer :

```text
SupporterData
SupporterLogic
SupporterEvents
SupporterUI
MatchAtmosphere
```

Éviter de mettre toute la logique dans l'interface.

---

# 42. Ordre d'implémentation

```text
1. SupporterProfile
2. Fidélité / exigence / ferveur / patience
3. Abonnements
4. Affluence
5. Déplacements
6. Ambiance
7. Rivalités
8. Chants / sifflets
9. Tifos
10. Contestations
11. Boycott
12. Fêtes
13. Records
14. Page Supporters
15. Musée / finances
16. Sauvegarde
17. Tests multi-saisons
```

---

# 43. Résultat final recherché

Exemple d'histoire :

```text
2027
Petit club de District
380 supporters

2029
Première montée
Fête avec 1 200 personnes

2034
Record d'affluence : 3 850

2039
Premier grand derby régional
Tifo historique

2044
Accession au National
6 500 abonnés

2049
Première Ligue 1
Liste d'attente abonnements

2052
Vente du capitaine emblématique
Contestations

2055
Premier titre
Grande parade en ville
```

Les supporters doivent devenir une mémoire vivante du club, pas seulement un chiffre d'affluence.

---

# 44. Livrables attendus

1. structure SupporterProfile
2. système d'évolution
3. abonnements
4. affluence enrichie
5. déplacements
6. ambiance
7. tifos
8. chants
9. sifflets
10. contestations
11. boycott
12. fêtes
13. records
14. page Supporters
15. liens finances
16. liens Musée
17. sauvegarde
18. tests
19. documentation mise à jour
20. rapport des éléments restant à polir visuellement
