# France Foot 2D / Super Soccer World
## Prompt de chantier — Directeur Sportif
### Passe 2 : Mode Carrière Directeur Sportif complet

---

# PROMPT À DONNER POUR L'IMPLÉMENTATION

Tu travailles sur le code source de **France Foot 2D / Super Soccer World**.

Cette passe doit s'appuyer sur la **Passe 1 Directeur Sportif**, déjà conçue pour fournir :

- Directeur sportif comme membre du staff
- compétences
- philosophie sportive
- responsabilités
- délégation
- recrutement
- ventes
- contrats
- renouvellements
- prêts
- rapports
- listes de cibles
- négociations
- sauvegarde
- IA de gestion

## Objectif principal

Créer un véritable mode :

```text
CARRIÈRE DIRECTEUR SPORTIF
```

Dans ce mode, le joueur humain **incarne le Directeur sportif**.

Il ne dirige pas directement les matchs.

Son travail consiste à construire le club sportivement sur plusieurs saisons :

- définir la politique sportive
- construire l'effectif
- recruter
- vendre
- négocier
- gérer les contrats
- gérer les prêts
- choisir l'entraîneur
- construire le staff
- développer les jeunes
- travailler avec le président
- gérer les objectifs sportifs
- préparer l'avenir du club

Le but est de créer un mode réellement différent de la carrière entraîneur.

---

# 1. Principe fondamental

Dans ce mode :

```text
JOUEUR HUMAIN = DIRECTEUR SPORTIF
```

Le joueur humain contrôle principalement :

```text
RECRUTEMENT
EFFECTIF
CONTRATS
VENTES
PRÊTS
STAFF
ENTRAÎNEUR
FORMATION
POLITIQUE SPORTIVE
```

L'entraîneur IA contrôle :

```text
TACTIQUE
COMPOSITION
RÔLES
ENTRAÎNEMENT TERRAIN
REMPLACEMENTS
GESTION DU MATCH
```

Le président contrôle principalement :

```text
BUDGET GLOBAL
OBJECTIFS
INFRASTRUCTURES MAJEURES
PRESSION SUR LES RÉSULTATS
VALIDATION DE CERTAINES DÉPENSES
```

Il faut donc créer une vraie répartition des pouvoirs.

---

# 2. Nouveau type de carrière

Ajouter dans le menu Carrière :

```text
Carrière Entraîneur
Carrière Joueur
Carrière Directeur Sportif
```

Créer un profil spécifique.

Exemple :

```text
CareerType = SPORTING_DIRECTOR
```

Le système ne doit pas être une simple variante visuelle de la carrière entraîneur.

Le flux de jeu doit être adapté au métier.

---

# 3. Création du Directeur sportif humain

À la création de carrière, permettre de définir :

- nom
- prénom
- âge
- nationalité
- expérience initiale
- réputation initiale
- spécialité
- philosophie sportive

Exemples de spécialités :

- Recrutement
- Négociation
- Jeunes
- Trading
- Réseau international
- Gestion des contrats
- Construction d'effectif
- Football local

---

# 4. Compétences du joueur humain

Créer des compétences évolutives.

Exemples :

- Recrutement
- Négociation
- Vente
- Gestion des contrats
- Gestion des prêts
- Analyse d'effectif
- Réseau national
- Réseau international
- Détection jeunes
- Relations agents
- Gestion staff
- Gestion entraîneur
- Diplomatie président
- Vision sportive

Les compétences doivent progresser au fil de la carrière.

---

# 5. Réputation du Directeur sportif

Créer une réputation propre au joueur humain.

Niveaux possibles :

```text
Local
Régional
National
Continental
International
Élite
```

La réputation doit influencer :

- clubs accessibles
- confiance du président
- qualité des agents disponibles
- facilité de négociation
- attractivité auprès des joueurs
- qualité des entraîneurs intéressés
- offres d'emploi
- liberté accordée par les dirigeants

---

# 6. Début de carrière

Permettre plusieurs types de départ :

## Débutant

```text
Petit club / amateur / semi-pro
Réputation faible
Pouvoir limité
Budget faible
```

## Expérimenté

```text
Club professionnel
Réputation moyenne
```

## Libre

```text
Sans club
Recherche d'emploi
```

## Personnalisé

Permettre au joueur de choisir :

- réputation
- compétences
- club
- niveau de départ

---

# 7. Recherche d'emploi

Si le joueur commence sans club ou quitte son club :

Créer un système de candidatures.

Afficher :

- clubs intéressés
- budget
- niveau
- championnat
- situation sportive
- attentes président
- pouvoir du Directeur sportif
- structure actuelle
- entraîneur en place
- niveau du centre de formation

Le joueur peut :

```text
POSTULER
REFUSER
NÉGOCIER
```

---

# 8. Contrat du Directeur sportif

Créer un contrat spécifique.

Données :

- salaire
- durée
- objectifs
- pouvoirs
- budget disponible
- clause éventuelle
- indemnité de départ
- confiance du président

Le contrat doit pouvoir être :

- renouvelé
- refusé
- rompu
- terminé
- résilié après mauvais résultats

---

# 9. Relation avec le président

Créer un système de relation :

```text
Très mauvaise
Mauvaise
Neutre
Bonne
Excellente
```

Facteurs :

- respect du budget
- résultats sportifs
- qualité des recrutements
- ventes réussies
- jeunes développés
- objectifs atteints
- qualité de l'entraîneur choisi
- stabilité de l'effectif
- conflits internes
- dépenses excessives

---

# 10. Pouvoirs accordés

Tous les clubs ne doivent pas donner la même liberté.

Créer plusieurs niveaux :

```text
DIRECTEUR SPORTIF LIMITÉ
DIRECTEUR SPORTIF STANDARD
DIRECTEUR SPORTIF FORT
DIRECTEUR SPORTIF TOTAL
```

Exemple :

## Limité

Le président valide :

- gros transferts
- salaire élevé
- licenciement entraîneur
- recrutement staff clé

## Total

Le Directeur sportif peut :

- recruter
- vendre
- négocier
- choisir coach
- construire staff
- décider politique sportive

Le niveau de pouvoir peut évoluer avec la confiance.

---

# 11. Objectifs du président

Le président doit fixer des objectifs.

Exemples :

- maintien
- montée
- qualification européenne
- titre
- réduire masse salariale
- faire jouer les jeunes
- vendre pour X €
- recruter local
- finir la saison avec budget positif
- développer le centre de formation

Les objectifs doivent être évalués en fin de saison.

---

# 12. Politique sportive

Le joueur Directeur sportif doit pouvoir définir une politique globale.

Exemples :

```text
JEUNES
EXPÉRIENCE
FORMATION
TRADING
LOCAL
INTERNATIONAL
STABILITÉ
RÉSULTAT IMMÉDIAT
```

Ajouter des paramètres comme :

- âge moyen cible
- priorité jeunes
- priorité joueurs locaux
- politique salariale
- part budget réservée aux jeunes
- nombre maximum de joueurs de plus de 30 ans
- stratégie de prêts
- stratégie de ventes

---

# 13. Plan sportif pluriannuel

Créer un écran :

```text
PROJET SPORTIF
```

Permettre de définir :

```text
OBJECTIF 1 SAISON
OBJECTIF 3 SAISONS
OBJECTIF 5 SAISONS
```

Exemple :

```text
2027/28
Maintien

2028/29
Top 8

2029/30
Qualification européenne
```

Le président peut accepter ou refuser un projet trop ambitieux.

---

# 14. Analyse d'effectif

Créer une page centrale :

```text
ANALYSE DE L'EFFECTIF
```

Afficher :

- effectif par poste
- âge moyen
- contrats
- salaires
- joueurs en fin de contrat
- joueurs à vendre
- joueurs à prêter
- jeunes prometteurs
- profondeur par poste
- faiblesse par poste
- valeur de l'effectif

Le Directeur sportif doit pouvoir planifier plusieurs saisons.

---

# 15. Planification de l'effectif

Ajouter des statuts :

```text
TITULAIRE
IMPORTANT
ROTATION
ESPOIR
À VENDRE
À PRÊTER
INTOUCHABLE
FIN DE CYCLE
```

Permettre de planifier :

```text
POSTE ACTUEL
REMPLAÇANT
SUCCESSEUR
```

Exemple :

```text
GB titulaire : 34 ans
Successeur identifié : jeune de 21 ans
```

---

# 16. Choix de l'entraîneur

C'est un élément central du mode.

Le joueur Directeur sportif doit pouvoir :

- garder le coach actuel
- licencier le coach
- rechercher un coach
- négocier un contrat
- recruter un coach
- renouveler son contrat
- refuser une prolongation

L'entraîneur doit avoir :

- réputation
- philosophie
- formations favorites
- style de jeu
- gestion des jeunes
- gestion vestiaire
- discipline
- adaptation
- exigence recrutement
- salaire
- durée contrat

---

# 17. Compatibilité Directeur sportif / entraîneur

Créer un score de compatibilité.

Exemple :

```text
coachCompatibility =
philosophie
+ utilisation des jeunes
+ style tactique
+ politique recrutement
+ personnalité
```

Exemple de conflit :

```text
DS :
recrutement jeunes / possession

Coach :
joueurs expérimentés / jeu direct
```

La relation peut devenir mauvaise.

---

# 18. Relation DS / entraîneur

Créer une relation dynamique.

États :

```text
Excellente
Bonne
Neutre
Tendue
Conflit
```

Facteurs :

- recrues correspondant aux demandes
- ventes contre l'avis du coach
- résultats
- temps de jeu des recrues
- utilisation des jeunes
- demandes refusées
- prolongations
- choix tactiques généraux

---

# 19. Demandes de l'entraîneur

Le coach IA peut demander :

```text
un BU
un MDC
un gardien
un joueur rapide
un défenseur gaucher
un joueur expérimenté
```

Le Directeur sportif peut :

```text
ACCEPTER
REFUSER
REPORTER
PROPOSER UNE ALTERNATIVE
```

---

# 20. Contrôle des matchs

Le joueur Directeur sportif ne doit pas contrôler directement les joueurs.

Pendant un match, il peut :

- regarder
- accélérer
- simuler
- consulter statistiques
- consulter rapport live

Mais il ne peut pas :

- changer tactique
- faire substitutions
- modifier rôles
- déplacer joueurs

Ces décisions appartiennent à l'entraîneur IA.

---

# 21. Matchs importants

Pour :

- finale
- derby
- match de montée
- match de titre
- match européen

le Directeur sportif peut recevoir :

- briefing avant match
- rapport après match
- analyse de l'entraîneur
- réactions président
- réactions supporters

---

# 22. Recrutement

Réutiliser la Passe 1.

Le joueur contrôle directement :

- priorités
- budget
- profils
- cibles
- offres
- négociations
- contrats

Ajouter une vue :

```text
PLAN MERCATO
```

avec :

- poste recherché
- budget maximum
- priorité
- âge
- profil
- cible principale
- alternatives

---

# 23. Scouting

Créer une vraie interaction avec les recruteurs.

Le Directeur sportif peut :

- assigner régions
- assigner pays
- demander poste
- demander profil
- demander âge
- demander potentiel
- suivre joueur
- créer shortlist

Les données peuvent être partielles si le joueur n'est pas connu.

---

# 24. Réseau du Directeur sportif

Créer un réseau personnel.

Exemples :

- France
- Belgique
- Espagne
- Allemagne
- Afrique francophone
- Amérique du Sud

Ce réseau peut progresser avec la carrière.

Il influence :

- qualité des informations
- nombre de joueurs connus
- relations agents
- facilité de négociation

---

# 25. Agents

Le Directeur sportif doit avoir des relations avec les agents.

Chaque agent peut avoir :

- réputation
- relation
- portefeuille joueurs
- niveau d'exigence

Une bonne relation peut :

- faciliter négociation
- donner accès à joueur
- accélérer accord

Une mauvaise relation peut :

- augmenter salaire demandé
- bloquer négociation
- pousser joueur vers autre club

---

# 26. Ventes

Créer une vraie stratégie de vente.

Le joueur peut définir :

```text
PRIX MINIMUM
PRIX SOUHAITÉ
NON TRANSFÉRABLE
VENTE RAPIDE
```

Le système doit prendre en compte :

- valeur
- âge
- contrat
- importance
- potentiel
- salaire
- remplacement disponible

---

# 27. Revente / Trading

Pour une philosophie trading :

Suivre :

- prix d'achat
- salaire total
- prix de vente
- plus-value
- durée au club

Créer des statistiques :

```text
MEILLEURES PLUS-VALUES
PIRES TRANSFERTS
MEILLEURES VENTES
```

---

# 28. Contrats

Le joueur Directeur sportif gère :

- renouvellement
- durée
- salaire
- primes
- clauses
- options
- promesses
- statut

Créer une vue :

```text
CONTRATS À SURVEILLER
```

---

# 29. Jeunes / Centre de formation

Le Directeur sportif doit pouvoir définir la politique jeunes.

Paramètres :

- budget jeunes
- recrutement local
- recrutement national
- recrutement international
- priorité U17
- priorité U19
- prêts
- intégration première

Le coach IA reste libre d'utiliser ou non un jeune selon ses choix.

---

# 30. Passerelle jeunes -> équipe première

Créer un écran :

```text
PLAN DE DÉVELOPPEMENT
```

Pour chaque jeune :

```text
U19
Réserve
Prêt
Rotation première
Titulaire
```

Le Directeur sportif choisit une trajectoire recommandée.

---

# 31. Staff

Le Directeur sportif peut gérer :

- recruteurs
- analystes
- responsables jeunes
- préparateurs selon pouvoirs
- responsable performance
- éventuellement médical

Le coach garde une influence sur son staff terrain selon le niveau de pouvoir.

---

# 32. Clubs partenaires

Permettre de rechercher :

- club satellite
- club de prêt
- partenariat jeunes
- partenariat international

Utilité :

- prêts
- développement jeunes
- recrutement
- réseau

---

# 33. Budget

Le Directeur sportif gère un budget sportif.

Exemples :

```text
Budget transferts
Masse salariale
Budget staff
Budget scouting
Budget jeunes
```

Le président fixe les limites.

Le joueur peut proposer des réallocations.

Exemple :

```text
-2 M€ transferts
+1 M€ masse salariale
+1 M€ scouting
```

Le président peut accepter ou refuser.

---

# 34. DNCG / Fair-play financier

Réutiliser les systèmes existants.

Le Directeur sportif doit recevoir des alertes :

```text
Risque DNCG
Masse salariale excessive
Budget négatif
Dépenses trop élevées
```

Les sanctions doivent rester cohérentes avec le reste du jeu.

---

# 35. Réunions avec le président

Créer des événements périodiques.

Exemples :

```text
Réunion début de saison
Réunion mercato hiver
Bilan fin de saison
Réunion crise
```

Le joueur peut expliquer :

- mauvais résultats
- mercato raté
- besoin de budget
- changement d'entraîneur
- projet jeunes

---

# 36. Réunions avec l'entraîneur

Créer des échanges réguliers :

```text
Analyse effectif
Besoins mercato
Joueurs à vendre
Jeunes à intégrer
Contrats
Objectifs
```

---

# 37. Conflits internes

Ajouter des événements raisonnables :

- coach refuse une recrue
- président refuse une dépense
- joueur refuse prolongation
- agent mécontent
- recruteur demande plus de moyens
- club veut vendre un joueur important
- coach menace de partir

Ne pas surcharger la carrière avec trop d'événements aléatoires.

---

# 38. Licenciement du Directeur sportif

Le joueur peut être licencié.

Facteurs :

- résultats
- finances
- mauvais recrutement
- conflit président
- mauvais choix d'entraîneur
- objectifs non atteints

Créer un écran expliquant clairement la raison.

---

# 39. Démission

Le joueur peut quitter le club.

Créer :

```text
DÉMISSIONNER
```

avec possibilité :

- départ immédiat
- fin de saison
- fin de contrat

---

# 40. Offres d'autres clubs

Les autres clubs peuvent contacter le joueur.

Exemple :

```text
Olympique X souhaite vous recruter comme Directeur sportif.
```

Le joueur peut :

- accepter
- refuser
- négocier

---

# 41. Carrière internationale

Préparer éventuellement une extension future :

```text
Directeur technique national
```

Ne pas obligatoirement l'implémenter maintenant.

Mais éviter une architecture qui empêcherait plus tard :

- fédération
- formation nationale
- développement jeunes
- sélection entraîneurs

---

# 42. Statistiques de carrière du Directeur sportif

Afficher :

- clubs
- saisons
- transferts
- dépenses
- ventes
- balance mercato
- plus-values
- trophées
- promotions
- entraîneurs recrutés
- jeunes lancés
- joueurs devenus internationaux
- réputation

---

# 43. Palmarès personnel

Créer un écran :

```text
CARRIÈRE DU DIRECTEUR SPORTIF
```

Exemple :

```text
2027-2031 : Sedan
2 promotions

2031-2036 : Nantes
1 Coupe de France

2036-2043 : Marseille
2 titres
1 Coupe d'Europe
```

---

# 44. Musée du club

Le système de Musée du club peut enregistrer le Directeur sportif dans :

- dirigeants historiques
- grands architectes du club
- périodes majeures

Exemple :

```text
2035-2047
Directeur sportif : Jean Martin

3 titres
1 Coupe
Construction de l'effectif champion
```

---

# 45. IA des autres Directeurs sportifs

Les clubs IA doivent progressivement utiliser la même logique.

Chaque club peut posséder son propre DS IA.

Cela influence :

- recrutement
- ventes
- profils recherchés
- politique jeunes
- choix coach
- renouvellements

---

# 46. Changement d'entraîneur IA

Le Directeur sportif IA peut :

- conserver coach
- licencier
- recruter
- renouveler

Prendre en compte :

- résultats
- compatibilité
- salaire
- réputation
- objectifs

---

# 47. Moteur de match

Ne pas modifier le moteur de match sauf nécessité.

Le mode Directeur sportif observe les matchs mais n'en change pas directement la logique.

Le coach IA doit utiliser les systèmes tactiques existants.

---

# 48. Interface principale

Créer un tableau de bord spécifique.

Exemple :

```text
DIRECTEUR SPORTIF

[ Effectif ]
[ Recrutement ]
[ Contrats ]
[ Ventes ]
[ Prêts ]
[ Entraîneur ]
[ Staff ]
[ Jeunes ]
[ Scouting ]
[ Finances ]
[ Projet sportif ]
[ Président ]
```

---

# 49. Tableau de bord

Afficher :

- prochain match
- forme équipe
- confiance président
- confiance coach
- budget
- besoins effectif
- contrats urgents
- négociations
- joueurs suivis
- jeunes à surveiller

---

# 50. Calendrier

Le calendrier doit inclure :

- matchs
- mercato
- fins de contrat
- réunions président
- réunions coach
- échéances objectifs
- renouvellements
- périodes inscription joueurs

---

# 51. Sauvegarde

Sauvegarder :

- carrière DS
- réputation
- compétences
- historique clubs
- contrats
- relations
- objectifs
- projets sportifs
- négociations
- réseau
- staff
- coach
- politiques

Tester sur plusieurs saisons.

---

# 52. Pas de compatibilité ancienne sauvegarde obligatoire

Si le projet n'exige pas de conserver les anciennes sauvegardes :

- privilégier une structure propre
- éviter les rustines de migration
- centraliser les données
- versionner clairement la nouvelle sauvegarde

---

# 53. Tests prioritaires

## Test A — Petit club amateur

Le joueur commence Directeur sportif d'un petit club.

Vérifier :

- faible budget
- recrutement local
- pouvoir limité
- coach simple
- prêts
- jeunes

## Test B — Club professionnel

Vérifier :

- plusieurs recruteurs
- marché national/international
- contrats
- budget
- attentes président

## Test C — Mauvais entraîneur

Les résultats chutent.

Vérifier :

- pression président
- possibilité licenciement
- recherche remplaçant

## Test D — Bon recrutement

Vérifier :

- confiance augmente
- réputation augmente
- historique carrière mis à jour

## Test E — Mauvaise gestion financière

Vérifier :

- alertes
- restrictions
- président mécontent

## Test F — Changement de club

Vérifier :

- ancien club continue
- historique joueur conservé
- nouveau club chargé correctement

## Test G — Sauvegarde 10 saisons

Vérifier :

- aucune perte
- aucune duplication
- performances correctes

---

# 54. Critères de validation

La Passe 2 est terminée lorsque :

- un nouveau mode Carrière Directeur sportif existe
- le joueur peut commencer ou chercher un emploi
- le joueur possède réputation et compétences
- le président fixe objectifs et budgets
- le joueur contrôle recrutement / ventes / contrats / prêts
- le joueur peut gérer le staff
- le joueur peut choisir et licencier un entraîneur
- l'entraîneur IA gère réellement les matchs
- le joueur ne contrôle pas directement la tactique en match
- les relations président / coach fonctionnent
- le projet sportif pluriannuel fonctionne
- le joueur peut être licencié
- le joueur peut changer de club
- l'historique de carrière est conservé
- la sauvegarde fonctionne sur plusieurs saisons
- aucun système majeur de la carrière entraîneur n'est cassé
- aucun moteur de match n'est régressé

---

# 55. Architecture recommandée

Éviter de créer des versions séparées des systèmes existants.

Réutiliser :

```text
TransferSystem
ContractSystem
LoanSystem
ScoutingSystem
StaffSystem
YouthSystem
FinanceSystem
```

Puis ajouter une couche :

```text
SportingDirectorCareer
```

Le même système de transfert doit pouvoir être utilisé par :

```text
Carrière entraîneur
Carrière Directeur sportif
IA club
```

---

# 56. Séparation des responsabilités

Exemple :

```text
PRESIDENT
    ↓
budget + objectifs

DIRECTEUR SPORTIF
    ↓
effectif + recrutement + contrats + coach

ENTRAÎNEUR
    ↓
tactique + entraînement terrain + matchs

JOUEURS
    ↓
performance
```

Cette séparation doit rester claire.

---

# 57. Résultat recherché

Le mode doit permettre une carrière comme :

```text
Je commence Directeur sportif d'un club de District.

Je recrute localement.

Je choisis un entraîneur adapté.

Je construis un centre de formation.

Nous montons en Régional.

Je vends un jeune pour financer le club.

Je recrute un meilleur entraîneur.

Nous atteignons le National.

Un club professionnel me propose un contrat.

Je change de club.

Après 20 saisons, ma carrière possède son propre palmarès.
```

Le joueur ne doit pas avoir l'impression de jouer une carrière entraîneur privée du moteur de match.

Il doit réellement jouer **le métier de Directeur sportif**.

---

# 58. Livrables attendus

À la fin du chantier :

1. mode Carrière Directeur sportif
2. création de profil DS
3. réputation et compétences
4. contrats et recherche d'emploi
5. relation président
6. objectifs
7. politique sportive
8. projet pluriannuel
9. gestion effectif
10. recrutement
11. ventes
12. contrats
13. prêts
14. scouting
15. gestion jeunes
16. gestion staff
17. gestion entraîneur
18. relation DS / coach
19. tableau de bord dédié
20. sauvegarde complète
21. historique carrière
22. tests
23. documentation mise à jour
24. rapport des éventuelles limitations restantes

---

# PRIORITÉ DE DÉVELOPPEMENT

Ordre conseillé :

```text
1. Nouveau type de carrière
2. Profil / réputation DS
3. Président / objectifs / pouvoir
4. Effectif / recrutement / contrats
5. Gestion entraîneur
6. Relations DS / coach
7. Jeunes / scouting / staff
8. Carrière / offres / licenciement
9. Sauvegarde
10. Interface
11. Tests multi-saisons
12. Polissage
```

---

# IMPORTANT

Ne pas sacrifier les modes existants pour créer celui-ci.

La Passe 2 doit **réutiliser** les systèmes de la Passe 1 au lieu de les dupliquer.

Le résultat final doit apporter une troisième grande manière de jouer à Super Soccer World :

```text
JOUER AU FOOTBALL
ENTRAÎNER UNE ÉQUIPE
CONSTRUIRE UN CLUB COMME DIRECTEUR SPORTIF
```
