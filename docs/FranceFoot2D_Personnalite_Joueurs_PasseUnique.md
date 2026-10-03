# France Foot 2D / Super Soccer World
## Prompt de chantier — Personnalité complète des joueurs
### Intégration complète en une seule passe

---

# PROMPT À DONNER POUR L'IMPLÉMENTATION

Tu travailles sur le code source de **France Foot 2D / Super Soccer World**.

## Objectif principal

Créer un véritable système de **personnalité des joueurs**, distinct des attributs footballistiques.

Deux joueurs ayant le même niveau général ne doivent plus se comporter de la même manière.

Le système doit agir sur :
- progression
- entraînement
- carrière
- mercato
- contrats
- moral
- vestiaire
- comportement sous pression
- fidélité au club
- adaptation à l'étranger
- relation avec l'entraîneur
- scouting
- rôle de capitaine
- popularité auprès des supporters
- légendes du club
- quelques comportements du moteur de match

Le système doit principalement enrichir les modes Carrière.

Le mode Simple peut ignorer la majorité de ces données.

---

# 1. Principe général

Séparer clairement :

```text
ATTRIBUTS FOOTBALL
```

de :

```text
PERSONNALITÉ
```

La personnalité ne doit pas augmenter artificiellement la note générale.

Exemple :

```text
Joueur A
Niveau : 78
Professionnalisme : 92
Régularité : 88
Loyauté : 35

Joueur B
Niveau : 78
Professionnalisme : 45
Régularité : 40
Loyauté : 91
```

Ces deux joueurs doivent avoir des carrières et comportements très différents.

---

# 2. Structure recommandée

Créer une structure dédiée :

```text
PlayerPersonality
{
    ambition
    professionalism
    loyalty
    aggression
    leadership
    composure
    consistency
    bigMatchTemperament
    sensitivity
    foreignAdaptability
}
```

Échelle interne recommandée :

```text
0..100
```

Ne pas afficher systématiquement les valeurs exactes dans l'interface joueur.

---

# 3. Affichage utilisateur

Afficher plutôt des niveaux lisibles.

Exemple :

```text
Très faible
Faible
Moyen
Bon
Élevé
Très élevé
Exceptionnel
```

Le chiffre exact peut rester disponible dans :
- éditeur
- debug
- outils internes
- éventuellement scouting avancé

---

# 4. Ambition

L'ambition influence :

- désir de progresser
- envie de jouer à un niveau supérieur
- patience dans un petit club
- intérêt pour les compétitions importantes
- demandes de transfert
- exigences de contrat
- volonté de devenir titulaire
- réaction au manque de progression du club

Effets positifs :
- volonté de progresser
- recherche de responsabilité
- motivation dans un projet ambitieux

Effets négatifs possibles :
- impatience
- demande de départ
- refus d'un rôle secondaire

Ne pas traiter l'ambition comme un trait uniquement positif.

---

# 5. Professionnalisme

Le professionnalisme influence fortement :

- progression à l'entraînement
- maintien physique
- apprentissage d'un poste
- récupération
- régularité de travail
- capacité à approcher le potentiel
- vieillissement
- respect des consignes
- discipline générale

Exemple :

```text
Même potentiel théorique

Joueur A
Professionnalisme 92
=> forte probabilité d'atteindre son potentiel

Joueur B
Professionnalisme 38
=> progression plus incertaine
```

---

# 6. Loyauté

La loyauté influence :

- volonté de rester
- facilité de prolongation
- réaction aux offres
- patience après relégation
- acceptation d'un projet long
- relation avec les supporters
- retour possible au club
- importance comme futur joueur emblématique

Séparer si possible :

```text
loyalty
```

de :

```text
clubAttachment
```

Un joueur peut être naturellement peu loyal mais devenir très attaché à un club précis.

---

# 7. Agressivité

L'agressivité influence :

- intensité des duels
- pressing
- engagement
- fréquence de certains tacles
- réactions
- fautes
- cartons

Attention :

```text
aggression élevée
```

ne doit pas signifier automatiquement :

```text
carton rouge fréquent
```

Combiner avec :
- sang-froid
- professionnalisme
- discipline éventuelle
- contexte du match

Exemple :

```text
Agressivité 90
Sang-froid 90
=> défenseur engagé mais maîtrisé

Agressivité 90
Sang-froid 25
=> risque plus élevé de fautes et réactions
```

---

# 8. Leadership

Le leadership influence :

- choix du capitaine
- moral collectif
- réaction après un but encaissé
- comportement en période de crise
- influence sur les jeunes
- cohésion du vestiaire
- intégration des recrues
- résistance à la pression
- impact du brassard

Un joueur moyen mais très leader peut être extrêmement important dans l'effectif.

---

# 9. Sang-froid

Le sang-froid psychologique doit être distinct de la technique.

Il influence :

- pénaltys
- duels importants
- finales
- fin de match
- maintien
- titre
- public hostile
- tirs sous pression
- décisions sous pression

Il ne doit pas remplacer les attributs techniques.

Il doit seulement réduire ou accentuer les effets du stress.

---

# 10. Régularité

Créer une vraie variation de performance.

Exemple :

```text
Niveau 78
Régularité 92
=> performance généralement stable

Niveau 78
Régularité 35
=> forte variation d'un match à l'autre
```

La régularité doit influencer :
- amplitude de forme
- notes de match
- périodes bonnes/mauvaises
- fiabilité sur une saison

Ne pas la confondre avec la condition physique.

---

# 11. Tempérament grands matchs

Créer un trait :

```text
bigMatchTemperament
```

Il influence les matchs comme :

- finales
- derbys
- match de montée
- match pour le titre
- barrage
- élimination directe
- Coupe d'Europe
- match décisif de maintien

Un bon tempérament grands matchs :
- réduit le risque de sous-performance liée à la pression
- peut améliorer légèrement concentration et sang-froid

Éviter un gros bonus artificiel.

---

# 12. Susceptibilité

La susceptibilité influence les réactions à :

- banc
- remplacement
- critique
- perte du brassard
- refus de prolongation
- refus d'augmentation
- promesse non tenue
- conflit vestiaire
- déclaration publique si système médias

Exemples d'événements :

```text
Martin a mal accepté sa mise sur le banc.
```

```text
Dupont estime que les critiques publiques de l'entraîneur sont injustes.
```

Ce trait doit générer surtout des réactions de carrière, pas des pénalités systématiques.

---

# 13. Adaptation à l'étranger

Créer :

```text
foreignAdaptability
```

et si pertinent :

```text
adaptationProgress
```

Exemple :

```text
adaptationProgress = 0..100
```

Facteurs d'évolution :
- temps
- langue
- compatriotes
- âge
- expérience
- temps de jeu
- résultats
- personnalité
- stabilité du club

Pendant l'adaptation :
- légère variation de moral
- régularité réduite
- intégration plus lente

Une fois adapté :
- plus de pénalité

---

# 14. Relation au club

Créer une structure séparée.

Exemple :

```text
PlayerClubRelation
{
    clubId
    attachment
    fanPopularity
    managerRelationship
    squadInfluence
}
```

Ne pas mélanger ces valeurs avec la personnalité intrinsèque.

---

# 15. Attachement au club

L'attachement évolue avec :

- ancienneté
- formation au club
- titres
- capitanat
- promotions
- grands matchs
- relation supporters
- événements historiques

Effets :
- prolongations
- volonté de rester
- popularité
- légende du club

---

# 16. Popularité supporters

Créer :

```text
fanPopularity
```

Facteurs :
- fidélité
- formation au club
- buts
- grands matchs
- ancienneté
- capitanat
- leadership
- titres
- comportement public

Cette valeur doit se connecter au système Supporters.

---

# 17. Profils synthétiques

À partir des valeurs internes, générer une personnalité lisible.

Exemples :

```text
Professionnel ambitieux
Leader loyal
Talent irrégulier
Compétiteur caractériel
Jeune très professionnel
Capitaine naturel
Ambitieux instable
Professionnel discret
```

Ces profils sont des résumés, pas des données indépendantes.

---

# 18. Combinaisons de traits

Le système doit produire des comportements complexes.

Exemple A :

```text
Ambition 90
Professionnalisme 92
Loyauté 35
Régularité 87
```

=> excellent professionnel, mais peut chercher un club plus ambitieux.

Exemple B :

```text
Ambition 55
Professionnalisme 78
Loyauté 95
Leadership 88
Régularité 82
```

=> futur capitaine / légende possible.

Exemple C :

```text
Ambition 80
Professionnalisme 42
Susceptibilité 90
Grands matchs 91
Régularité 38
```

=> joueur capable de grands matchs mais difficile à gérer.

---

# 19. Évolution des traits

Tous les traits ne doivent pas être figés.

Peuvent évoluer lentement :

```text
Professionnalisme
Leadership
Attachement au club
Adaptation étrangère
Susceptibilité
```

Facteurs :
- âge
- mentor
- capitanat
- expérience
- ancienneté
- événements carrière

Éviter les changements brutaux.

---

# 20. Mentorat

Créer un système léger de mentorat.

Exemple :

```text
Mentor :
Professionnalisme 94
Leadership 91

Jeune :
Professionnalisme 55
Leadership 32
```

Effets possibles :
- petite progression de professionnalisme
- petite progression de leadership
- meilleure intégration
- adaptation plus rapide

Pas de transformation miraculeuse.

---

# 21. Capitanat

Le choix du capitaine doit utiliser :

- leadership
- ancienneté
- loyauté
- attachement
- statut
- professionnalisme

Créer éventuellement :

```text
captainScore
```

---

# 22. Vestiaire

Les traits doivent influencer :

- influence dans le groupe
- réaction aux résultats
- conflits
- intégration des recrues
- réaction aux décisions du coach

Un joueur leader et loyal peut stabiliser un vestiaire.

Un joueur ambitieux et susceptible peut devenir une source de tension.

---

# 23. Moral

La personnalité doit modifier la manière dont le moral évolue.

Exemple :

- joueur loyal : moins affecté par une mauvaise série
- joueur ambitieux : plus frustré par manque d'objectifs
- joueur susceptible : plus affecté par critiques
- leader : récupère plus vite après un revers

---

# 24. Mercato

Les traits influencent :

- demande de transfert
- choix du club
- acceptation contrat
- réaction au banc
- intérêt pour club plus prestigieux
- volonté de rester

Exemple :

```text
Ambition élevée
Loyauté faible
=> intérêt fort pour club supérieur
```

---

# 25. Contrats

La personnalité doit influencer :

- volonté de prolonger
- salaire demandé
- statut souhaité
- durée
- clauses
- réaction aux négociations

---

# 26. Directeur sportif

Le Directeur sportif doit pouvoir utiliser les traits.

Exemples :

DS orienté formation privilégie :
- professionnalisme
- ambition
- potentiel

DS orienté stabilité privilégie :
- loyauté
- leadership
- régularité

DS orienté résultat immédiat privilégie :
- sang-froid
- grands matchs
- régularité

---

# 27. Scouting

Les recruteurs ne doivent pas connaître parfaitement la personnalité immédiatement.

Créer un niveau de connaissance.

Exemple faible :

```text
Personnalité difficile à évaluer.
```

Exemple moyen :

```text
Semble professionnel et ambitieux.
```

Exemple élevé :

```text
Très professionnel.
Ambitieux.
Loyauté moyenne.
Adaptation internationale correcte.
```

---

# 28. Incertitude scouting

Un mauvais recruteur peut se tromper légèrement.

Éviter cependant de rendre le système arbitraire.

La qualité du recruteur influence :
- précision
- nombre de traits révélés
- confiance dans l'évaluation

---

# 29. Formation des jeunes

La génération de jeunes doit créer des profils variés.

Éviter :
- tous les jeunes très professionnels
- tous les jeunes ambitieux
- profils identiques

Créer des distributions cohérentes.

---

# 30. Progression

La progression globale doit prendre en compte :

```text
potentiel
professionnalisme
ambition
temps de jeu
entraînement
staff
âge
blessures
```

La personnalité ne doit pas remplacer le potentiel.

---

# 31. Déclin

Le professionnalisme peut influencer :
- maintien physique
- récupération
- vitesse de déclin

Un joueur professionnel peut mieux conserver son niveau.

---

# 32. Moteur de match

Ne pas réécrire le moteur.

Ajouter seulement des hooks légers.

Exemples :

```text
aggression
composure
consistency
bigMatchTemperament
leadership
```

Peuvent influencer légèrement :
- décisions
- fautes
- pression
- variation de performance
- comportement en grand match

Éviter des modificateurs trop forts.

---

# 33. Grands matchs

Créer une fonction conceptuelle :

```text
isBigMatch()
```

Peut détecter :
- finale
- derby
- barrage
- titre
- maintien
- Coupe d'Europe
- élimination directe importante

Puis appliquer une influence légère de :

```text
bigMatchTemperament
composure
leadership
```

---

# 34. Régularité et forme

Ne pas appliquer une variation aléatoire énorme à chaque match.

Créer une variation contrôlée.

Exemple conceptuel :

```text
performanceVariance =
baseVariance * (1 - consistency)
```

Adapter au système réel de notes du jeu.

---

# 35. Supporters

Connecter :

```text
loyalty
leadership
clubAttachment
fanPopularity
```

au système Supporters.

Exemples :
- vente d'une légende
- retour d'un ancien
- capitaine fidèle
- joueur formé au club

---

# 36. Musée du club

La personnalité et relation au club peuvent alimenter :

- joueurs légendaires
- Hall of Fame
- capitaines historiques
- records de longévité
- joueurs emblématiques

Un joueur très loyal et leader peut avoir un bonus historique.

---

# 37. Joueur légendaire

Ne pas utiliser seulement le niveau.

Exemple de score :

```text
legendScore =
matches
+ goals
+ assists
+ seasons
+ captaincy
+ trophies
+ fanPopularity
+ clubAttachment
+ loyalty
+ leadership
```

Les coefficients doivent rester équilibrés.

---

# 38. Notifications carrière

Exemples :

```text
Martin souhaite évoluer à un niveau supérieur.
```

```text
Dupont a très bien réagi à son nouveau rôle de capitaine.
```

```text
Bernard a mal accepté sa mise sur le banc.
```

```text
Lopez semble mieux intégré depuis l'arrivée d'un compatriote.
```

```text
Le professionnalisme de Petit impressionne le staff.
```

---

# 39. Interface fiche joueur

Ajouter une section :

```text
PERSONNALITÉ
```

Afficher :
- profil synthétique
- quelques traits connus
- relation au club
- popularité supporters
- influence vestiaire

Ne pas surcharger l'écran.

---

# 40. Historique personnalité

Éviter de stocker toutes les micro-variations.

Stocker seulement les changements importants :
- capitaine
- mentor
- amélioration leadership notable
- attachement devenu très fort
- adaptation complète

---

# 41. Mode Simple

En mode Simple :

- ignorer la majorité du système
- conserver éventuellement agressivité / sang-froid si déjà utiles au moteur
- ne pas afficher la gestion psychologique complète

---

# 42. Sauvegarde

Sauvegarder :

```text
PlayerPersonality
PlayerClubRelation
adaptationProgress
fanPopularity
captaincy
mentorLinks
```

Vérifier :
- changement de club
- transfert
- prêt
- retour
- nouvelle saison
- retraite

---

# 43. Changement de club

Lors d'un transfert :

Conserver :
- personnalité intrinsèque

Réinitialiser partiellement :
- attachement au nouveau club
- relation entraîneur
- adaptation locale

Conserver éventuellement l'historique de l'ancien club.

---

# 44. Prêt

Un prêt doit :
- conserver personnalité
- créer une relation temporaire avec le club d'accueil
- faire évoluer adaptation si pays étranger

---

# 45. Retraite

À la retraite :
- conserver profil historique
- conserver popularité
- possibilité de devenir staff si système existe
- utilisation possible dans Musée

---

# 46. Tests prioritaires

## Test A — Deux joueurs niveau 78

Créer deux profils opposés.

Vérifier :
- carrière différente
- progression différente
- réactions différentes

## Test B — Jeune professionnel

Vérifier :
- meilleure progression
- apprentissage plus rapide

## Test C — Joueur ambitieux petit club

Vérifier :
- demande de départ cohérente si club stagne
- pas de départ automatique si club progresse

## Test D — Joueur loyal

Vérifier :
- prolongation facilitée
- meilleure patience

## Test E — Joueur susceptible

Vérifier :
- réactions aux décisions
- pas de conflit permanent

## Test F — Étranger

Vérifier :
- adaptation progressive
- disparition de pénalité après adaptation

## Test G — Capitaine

Vérifier :
- leadership
- influence vestiaire
- supporters

## Test H — Grand match

Vérifier :
- tempérament grands matchs utilisé légèrement

## Test I — Scouting

Vérifier :
- information partielle
- meilleure précision avec bon recruteur

## Test J — 10 saisons

Vérifier :
- stabilité
- progression logique
- sauvegarde
- pas d'explosion des événements

---

# 47. Critères de validation

Le chantier est terminé lorsque :

- les 10 traits existent
- ils sont sauvegardés
- ils influencent la carrière
- ambition fonctionne
- professionnalisme fonctionne
- loyauté fonctionne
- agressivité fonctionne
- leadership fonctionne
- sang-froid fonctionne
- régularité fonctionne
- grands matchs fonctionne
- susceptibilité fonctionne
- adaptation étrangère fonctionne
- relation au club est séparée
- fanPopularity existe
- scouting utilise la personnalité
- Directeur sportif peut l'utiliser
- progression est influencée
- contrats et mercato sont influencés
- supporters sont connectés
- musée est connecté
- moteur de match reçoit seulement des effets légers
- mode Simple reste simple
- aucune régression majeure n'est introduite

---

# 48. Architecture recommandée

Séparer clairement :

```text
PlayerPersonality
PlayerClubRelation
PersonalityLogic
PersonalityEvents
PersonalityUI
```

Ne pas mettre toute la logique dans la fiche joueur.

---

# 49. Ordre d'implémentation conseillé

```text
1. PlayerPersonality
2. PlayerClubRelation
3. Génération des traits
4. Progression / entraînement
5. Mercato / contrats
6. Moral / vestiaire
7. Capitanat / leadership
8. Adaptation étrangère
9. Scouting
10. Directeur sportif
11. Supporters
12. Musée
13. Hooks moteur de match
14. Interface
15. Sauvegarde
16. Tests multi-saisons
```

---

# 50. Résultat recherché

Le but est de créer de vraies histoires de joueurs.

Exemple :

```text
Joueur A
Très professionnel
Ambitieux
Peu loyal
Régulier

=> progresse vite
=> devient très bon
=> demande à partir quand le club stagne
```

```text
Joueur B
Loyal
Leader
Très régulier
Ambition moyenne

=> reste longtemps
=> devient capitaine
=> devient une légende du club
```

```text
Joueur C
Très ambitieux
Peu professionnel
Très susceptible
Excellent dans les grands matchs

=> carrière irrégulière
=> parfois exceptionnel
=> difficile à gérer
```

Deux joueurs de même niveau doivent pouvoir laisser des souvenirs totalement différents.

---

# 51. Livrables attendus

À la fin de cette passe :

1. structure PlayerPersonality
2. structure PlayerClubRelation
3. génération des traits
4. progression liée aux traits
5. comportement carrière
6. mercato / contrats
7. vestiaire / moral
8. capitanat
9. adaptation étrangère
10. scouting
11. liens Directeur sportif
12. liens Supporters
13. liens Musée
14. hooks moteur de match
15. interface
16. sauvegarde
17. tests
18. documentation mise à jour
19. rapport des éventuels éléments restant à équilibrer
