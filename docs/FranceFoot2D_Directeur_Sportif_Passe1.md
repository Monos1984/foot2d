# France Foot 2D / Super Soccer World
## Prompt de chantier — Directeur Sportif
### Passe 1 : Staff + Délégation + IA de gestion

---

# PROMPT À DONNER POUR L'IMPLÉMENTATION

Tu travailles sur le code source de **France Foot 2D / Super Soccer World**.

## Objectif principal

Intégrer un véritable **Directeur sportif** dans les modes Carrière existants.

Dans cette première passe, le joueur humain reste entraîneur/manager.

Le Directeur sportif est un membre du staff capable de prendre en charge tout ou partie de la gestion sportive selon les responsabilités que le joueur lui délègue.

### Important

- Ne pas créer encore le mode **Carrière Directeur sportif**.
- Ne pas modifier inutilement le moteur de match.
- Réutiliser autant que possible les systèmes existants : mercato, contrats, prêts, staff, jeunes, budgets, scouting, agents s'ils existent déjà.
- Éviter de dupliquer les logiques déjà présentes.
- Le système doit fonctionner avec les clubs professionnels comme amateurs, avec des capacités adaptées au niveau du club.

---

# 1. Directeur sportif comme membre du staff

Ajouter un véritable poste :

```text
DIRECTEUR SPORTIF
```

Créer une fiche dédiée comprenant au minimum :

- nom
- âge
- nationalité
- salaire
- durée du contrat
- réputation
- expérience
- compétences
- philosophie sportive
- préférences de recrutement

Compétences principales proposées :

- Recrutement
- Négociation
- Vente
- Gestion des contrats
- Gestion des prêts
- Connaissance du marché
- Réseau national
- Réseau international
- Détection des jeunes
- Analyse de l'effectif
- Relations avec les agents
- Gestion du staff

Utiliser une échelle cohérente avec le reste du jeu.

---

# 2. Philosophie du Directeur sportif

Chaque Directeur sportif doit posséder une philosophie propre.

Exemples :

- privilégier les jeunes
- recruter des joueurs confirmés
- recruter local
- recruter national
- recruter international
- privilégier les joueurs libres
- acheter pour revendre
- conserver les cadres
- limiter les gros salaires
- privilégier le centre de formation
- rechercher des joueurs polyvalents
- privilégier les joueurs correspondant à la tactique de l'entraîneur

Ces philosophies ne doivent pas être de simples textes décoratifs.

Elles doivent réellement influencer ses décisions.

---

# 3. Écran des responsabilités

Créer dans la carrière un écran :

```text
RESPONSABILITÉS / DÉLÉGATION
```

Le joueur doit pouvoir décider précisément qui gère chaque domaine.

Pour chaque domaine, proposer plusieurs niveaux de délégation.

## Manuel
Le joueur humain fait tout.

## Conseil
Le Directeur sportif propose mais ne décide pas.

## Négociation
Le joueur choisit la cible et le Directeur sportif négocie.

## Délégation complète
Le Directeur sportif prend les décisions dans les limites fixées par le club.

---

# 4. Domaines délégables

Prévoir au minimum :

- recherche de joueurs
- présélection de recrues
- offres de transfert
- négociation des indemnités
- négociation des contrats
- prolongations
- ventes
- joueurs placés sur la liste des transferts
- prêts entrants
- prêts sortants
- recherche de clubs pour les joueurs à prêter
- négociations avec les agents
- recrutement de jeunes
- gestion de certaines arrivées du staff
- renouvellement du staff si pertinent

Le joueur humain doit pouvoir garder la main sur certains domaines et déléguer les autres.

---

# 5. Recrutement

Le Directeur sportif doit analyser les besoins de l'effectif.

Il ne doit pas simplement rechercher les joueurs ayant la meilleure note générale.

Prendre en compte :

- postes manquants
- âge de l'effectif
- fins de contrats
- blessures longues
- départs probables
- budget
- masse salariale
- niveau du club
- niveau de compétition
- philosophie du Directeur sportif
- besoins tactiques de l'entraîneur
- joueurs déjà présents chez les jeunes ou en réserve

Créer une fonction conceptuellement proche de :

```text
analyseSquadNeeds()
```

qui retourne des priorités.

Exemple :

```text
1. BU titulaire
2. DC de rotation
3. jeune DD
4. gardien remplaçant
```

---

# 6. Liste de cibles

Le Directeur sportif peut créer une liste de cibles.

Afficher pour chaque joueur proposé :

- nom
- club
- âge
- poste
- valeur estimée
- salaire estimé
- niveau estimé
- potentiel si disponible
- disponibilité
- intérêt probable
- raison de la recommandation

Exemples :

```text
Recommandé : manque d'avant-centre dans l'effectif.
```

```text
Profil jeune correspondant à la politique du club.
```

---

# 7. Négociations

Lorsque la négociation est déléguée, le Directeur sportif doit pouvoir :

- envoyer une offre
- augmenter progressivement son offre
- abandonner une négociation trop chère
- négocier le salaire
- négocier la durée du contrat
- négocier les primes
- gérer les agents si le système existe
- négocier les options de prêt
- négocier une option d'achat si disponible

Sa compétence **Négociation** doit influencer :

- prix obtenu
- salaire négocié
- probabilité d'accord
- vitesse des discussions
- risque d'échec

---

# 8. Limites financières

Le Directeur sportif ne doit jamais pouvoir dépenser librement sans contraintes.

Respecter :

- budget transferts
- budget salarial
- plafond défini par le président
- politique du club
- éventuelles règles financières existantes
- DNCG / fair-play financier si déjà implémentés

Permettre au joueur de définir des limites comme :

- prix maximum par joueur
- salaire maximum
- âge maximum ou minimum
- part maximale du budget utilisable

---

# 9. Ventes

Le Directeur sportif doit pouvoir gérer les ventes.

Prendre en compte :

- valeur du joueur
- temps de jeu
- âge
- salaire
- contrat restant
- potentiel
- importance dans l'équipe
- profondeur du poste
- demandes du joueur
- finances du club

Il ne doit pas vendre automatiquement un titulaire important sans raison.

Les joueurs peuvent être classés par statut :

- Intouchable
- Important
- Rotation
- Disponible
- À vendre
- À prêter

Le joueur humain doit pouvoir verrouiller :

```text
NE PAS VENDRE
```

sur certains joueurs.

---

# 10. Prêts

Créer une vraie logique de prêts.

Pour un jeune :

- rechercher un club où il jouera
- privilégier un niveau adapté
- prendre en compte son poste
- éviter un club où il serait troisième choix
- possibilité de prêt avec option
- possibilité de prise en charge du salaire

Pour un club demandeur :

- rechercher des joueurs disponibles
- éviter des joueurs trop faibles ou trop chers

---

# 11. Contrats et prolongations

Le Directeur sportif doit surveiller :

- contrats expirant dans 6 mois
- contrats expirant dans 12 mois
- joueurs importants
- jeunes prometteurs
- joueurs devenus trop chers
- joueurs vieillissants

Créer un système de priorités.

Exemple :

```text
URGENT
Capitaine — contrat dans 4 mois

IMPORTANT
Jeune titulaire — contrat dans 10 mois

FAIBLE
Remplaçant de 34 ans — salaire élevé
```

---

# 12. Relation avec l'entraîneur

Le Directeur sportif doit tenir compte des demandes de l'entraîneur.

Créer un système permettant à l'entraîneur humain de demander :

- un poste
- un profil
- un âge
- un niveau
- un type de joueur

Exemple :

```text
Je cherche un MDC récupérateur de moins de 25 ans.
```

Le Directeur sportif génère alors une liste de cibles.

Prévoir aussi la possibilité de désaccord.

Exemple :

```text
L'effectif possède déjà trois joueurs capables d'évoluer à ce poste.
```

Le joueur humain garde toutefois la décision finale selon le niveau de délégation.

---

# 13. IA du Directeur sportif

Créer une logique centrale claire.

Exemple conceptuel :

```text
DirectorAI
{
    analyseSquad()
    analyseBudget()
    determineNeeds()
    searchTargets()
    rankTargets()
    negotiateTransfer()
    negotiateContract()
    manageLoans()
    manageSales()
    manageRenewals()
}
```

Éviter d'éparpiller la logique dans l'interface.

Séparer :

```text
DATA
IA
INTERFACE
```

---

# 14. Score des cibles

Créer une fonction de classement interne.

Exemple :

```text
targetScore =
besoinPoste
+ niveau
+ potentiel
+ âge
+ prix
+ salaire
+ adaptationTactique
+ philosophieDS
+ connaissanceJoueur
+ intérêtJoueur
```

Les coefficients peuvent varier selon le Directeur sportif.

Exemple :

- un DS orienté formation favorise potentiel + âge
- un DS orienté résultat immédiat favorise le niveau actuel
- un DS orienté trading favorise le potentiel de revente

---

# 15. Petits clubs / football amateur

Le système doit fonctionner également dans les divisions basses.

Un club de District ne doit pas fonctionner comme le PSG.

Adapter :

- réseau de recrutement
- nombre de joueurs connus
- budgets
- salaires
- rayon géographique
- réputation des cibles
- professionnalisme du staff

Un petit Directeur sportif local doit principalement rechercher :

- joueurs libres
- joueurs locaux
- clubs voisins
- jeunes libérés
- prêts accessibles

---

# 16. Rapport du Directeur sportif

Créer un écran :

```text
RAPPORT DU DIRECTEUR SPORTIF
```

Exemple :

```text
POINTS FORTS
- effectif jeune
- milieu bien fourni

BESOINS
- avant-centre titulaire
- doublure au poste de DD

CONTRATS
- 3 joueurs en fin de contrat

TRANSFERTS
- 4 joueurs suivis

PRÊTS
- 2 jeunes devraient être prêtés

FINANCES
- masse salariale proche de la limite
```

---

# 17. Notifications

Créer des messages de carrière.

Exemples :

```text
Le Directeur sportif recommande de prolonger Martin.
```

```text
Une offre de 350 000 € a été reçue pour Dupont.
```

```text
Le Directeur sportif estime l'offre insuffisante.
```

```text
Trois joueurs correspondant au profil demandé ont été identifiés.
```

```text
Le Directeur sportif a trouvé un club intéressé par le prêt de Bernard.
```

---

# 18. Interface

Ajouter une page Directeur sportif comprenant :

- fiche
- compétences
- philosophie
- responsabilités
- rapport
- cibles
- négociations
- contrats
- prêts

Ne pas créer une interface inutilement complexe.

Les informations importantes doivent rester lisibles rapidement.

---

# 19. Sauvegarde

Sauvegarder :

- Directeur sportif actuel
- contrat
- compétences
- philosophie
- responsabilités
- listes de cibles
- négociations en cours
- préférences
- décisions programmées

Vérifier sauvegarde / chargement après plusieurs saisons.

---

# 20. IA des clubs non humains

Si possible, réutiliser progressivement le même système pour les clubs IA.

Un club possédant un bon Directeur sportif devrait prendre de meilleures décisions de mercato qu'un club possédant un mauvais Directeur sportif.

Ne pas obligatoirement refaire tout le mercato IA dans cette première passe si cela devient trop risqué.

Préparer toutefois l'architecture pour cette évolution.

---

# 21. Ce qu'il ne faut pas faire dans cette passe

Ne pas encore créer :

- mode Carrière Directeur sportif
- contrôle complet du club par le joueur en tant que DS
- licenciement/recrutement approfondi de l'entraîneur par le joueur DS
- simulation politique président / DS / coach extrêmement complexe
- refonte complète du moteur de match

Ces éléments appartiennent à la **Passe 2**.

---

# 22. Tests

## Cas 1 — Club riche professionnel

Déléguer complètement le recrutement.

Vérifier :

- besoins cohérents
- offres cohérentes
- budgets respectés

## Cas 2 — Petit club amateur

Vérifier :

- recrutement local
- joueurs accessibles
- pas de cibles irréalistes

## Cas 3 — Délégation négociation uniquement

Le joueur choisit la cible.

Le DS négocie.

## Cas 4 — Joueur marqué intransférable

Le DS ne doit jamais accepter une offre sans autorisation.

## Cas 5 — Jeune joueur à prêter

Le DS doit rechercher un club adapté.

## Cas 6 — Contrats expirants

Le DS doit identifier les priorités.

## Cas 7 — Sauvegarde / chargement

Les responsabilités et dossiers en cours doivent rester corrects.

---

# 23. Critères de validation

La Passe 1 est terminée lorsque :

- le Directeur sportif existe comme membre réel du staff
- ses compétences influencent ses décisions
- sa philosophie influence son recrutement
- le joueur peut déléguer précisément les responsabilités
- il peut proposer des recrues
- il peut négocier un transfert
- il peut négocier un contrat
- il peut gérer certaines ventes
- il peut gérer des prêts
- il peut surveiller les renouvellements
- les budgets sont respectés
- un joueur peut être déclaré intouchable
- le système fonctionne avec petits et grands clubs
- les décisions sont sauvegardées
- le reste de la carrière continue de fonctionner
- le moteur de match n'a subi aucune régression

---

# 24. Architecture à préparer pour la Passe 2

La conception doit permettre plus tard de créer :

```text
MODE CARRIÈRE DIRECTEUR SPORTIF
```

Dans ce futur mode, le joueur humain utilisera directement les systèmes développés dans cette passe :

- recrutement
- contrats
- ventes
- prêts
- budget sportif
- politique de recrutement
- staff
- choix de l'entraîneur

Ne pas implémenter ce mode maintenant.

Simplement éviter toute architecture qui empêcherait son ajout.

---

# 25. Livrables attendus

À la fin du chantier :

1. nouvelle source complète
2. Directeur sportif intégré au staff
3. écran de responsabilités
4. écran / fiche du Directeur sportif
5. IA de recrutement et gestion
6. délégation fonctionnelle
7. sauvegarde fonctionnelle
8. tests effectués
9. documentation mise à jour
10. résumé des modifications
11. liste claire des éléments laissés pour la Passe 2

Avant de terminer, effectuer un audit rapide des régressions sur :

- carrière
- mercato
- contrats
- prêts
- finances
- sauvegarde
- chargement
- composition de l'effectif

---

# Résultat attendu

Cette première passe doit rendre le Directeur sportif réellement utile dans la carrière même sans mode de jeu dédié.

Le joueur humain doit pouvoir choisir entre :

```text
Je fais tout moi-même.
```

ou :

```text
Je délègue une partie de la gestion sportive.
```

ou :

```text
Je laisse presque toute la gestion du mercato au Directeur sportif.
```

La Passe 2 pourra ensuite réutiliser exactement ces systèmes pour créer une véritable carrière jouable en tant que Directeur sportif.
