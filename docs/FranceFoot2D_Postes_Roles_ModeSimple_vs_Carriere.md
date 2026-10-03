# Super Soccer World / France Foot 2D
## Spécification — séparation des règles « Simple » et « Carrière »
### Chantier : postes détaillés et rôles tactiques

---

## 1. Objectif général

Le jeu doit proposer deux niveaux de profondeur distincts :

1. **Mode Simple**
   - Pour les matchs amicaux, championnats, coupes, tournois, compétitions personnalisées, matchs rapides et multijoueur local.
   - L'objectif est de sélectionner une équipe, choisir une formation simple et jouer immédiatement.
   - Les joueurs restent classés dans une seule catégorie de base.
   - Pas de micro-gestion tactique lourde.

2. **Mode Carrière**
   - Pour les modes carrière joueur, entraîneur, club et autres modes de gestion longue durée.
   - Les joueurs utilisent des postes détaillés, plusieurs positions possibles, niveaux de maîtrise, rôles tactiques et consignes individuelles.
   - Les systèmes de développement, entraînement, scouting, personnalité et progression peuvent utiliser ces données avancées.

Le principe central est :

> **Simple = football immédiat et lisible.**  
> **Carrière = simulation détaillée et management avancé.**

Il n'est pas nécessaire d'assurer la compatibilité avec les anciennes sauvegardes.

---

# 2. Un seul moteur de match

Il ne faut pas créer deux moteurs de match différents.

À éviter :

```text
simpleMatchEngine()
careerMatchEngine()
```

À conserver :

```text
matchEngine()
```

avec un profil de règles :

```text
RULESET_SIMPLE
RULESET_CAREER
```

Les deux modes utilisent donc le même moteur physique, les mêmes collisions, les mêmes règles du football, les mêmes animations et la même logique générale.

Ce qui change est la **quantité d'informations tactiques et individuelles fournies au moteur**.

---

# 3. Mode Simple

## 3.1 Catégories de joueur

Chaque joueur possède une seule catégorie :

```text
GK
DEF
MID
FWD
```

Correspondance :

| Code | Catégorie |
|---|---|
| GK | Gardien |
| DEF | Défenseur |
| MID | Milieu |
| FWD | Attaquant |

Exemple :

```text
Lucas Martin
MID
Niveau : 78
```

Aucune notion de :
- DD
- DG
- DC
- MDC
- MOC
- ailier intérieur
- faux neuf
- maîtrise de poste en pourcentage
- rôle spécialisé

Le joueur humain sélectionne simplement ses joueurs et joue.

---

## 3.2 Position déterminée par la formation

En mode simple, c'est le **slot de la formation** qui indique au moteur où le joueur doit évoluer.

Exemple :

```text
player.category = MID
formationSlot = LEFT_MIDFIELD
```

Le moteur comprend automatiquement que ce joueur doit se comporter comme un milieu gauche standard.

Le joueur n'a pas besoin de posséder lui-même une donnée `MG`.

---

## 3.3 Tactique simple

Le mode simple peut conserver quelques choix globaux faciles à comprendre.

### Mentalité

```text
Défensive
Équilibrée
Offensive
```

### Style de jeu

```text
Court
Mixte
Direct
```

### Pressing

```text
Faible
Normal
Fort
```

### Largeur

```text
Étroit
Normal
Large
```

Ces options doivent rester facultatives et compréhensibles immédiatement.

---

# 4. Mode Carrière

Le mode carrière conserve les catégories de base comme familles de poste, mais ajoute un système détaillé.

Exemple :

```text
family = MID
primaryPosition = MC
```

La famille reste utile pour :
- trier l'effectif
- afficher les groupes de joueurs
- rechercher rapidement
- compter gardiens / défenseurs / milieux / attaquants
- simplifier certains traitements internes

---

# 5. Postes détaillés en carrière

## 5.1 Liste de base

| Code | Poste |
|---|---|
| GB | Gardien |
| DD | Arrière droit |
| DC | Défenseur central |
| DG | Arrière gauche |
| PIS_D | Piston droit |
| PIS_G | Piston gauche |
| MDC | Milieu défensif |
| MC | Milieu central |
| MD | Milieu droit |
| MG | Milieu gauche |
| MOC | Milieu offensif central |
| AD | Ailier droit |
| AG | Ailier gauche |
| SA | Second attaquant |
| BU | Avant-centre |

Cette liste doit rester suffisamment détaillée pour créer de vraies différences tactiques sans multiplier inutilement les postes.

---

# 6. Plusieurs postes par joueur

Un joueur peut maîtriser plusieurs positions.

Exemple :

```text
Poste principal : MC

MC  = 100
MDC = 86
MOC = 74
MD  = 45
```

Le jeu peut traduire ces valeurs dans l'interface :

| Maîtrise | Valeur interne |
|---|---:|
| Naturel | 90–100 |
| Très bon | 75–89 |
| Correct | 55–74 |
| Dépannage | 30–54 |
| Inadapté | 0–29 |

La valeur numérique reste utile au moteur et à l'entraînement.

---

# 7. Apprentissage de poste

En carrière, la maîtrise d'un poste peut évoluer.

Exemple :

Un `MC` utilisé régulièrement en `MDC` peut progressivement passer de :

```text
MDC = 52
```

à :

```text
MDC = 78
```

La progression peut dépendre de :
- âge
- intelligence tactique
- temps de jeu à ce poste
- entraînement spécifique
- qualité du staff
- caractéristiques naturelles
- proximité entre les postes

Un changement `MC -> MDC` doit être plus facile qu'un changement `BU -> DG`.

---

# 8. Distinction fondamentale

Il faut séparer clairement :

```text
player.position
player.positionFamiliarity

formation.slot.position

tactic.role
tactic.duty
```

Cela signifie :

1. **Le joueur** possède ses capacités naturelles.
2. **Le slot de formation** indique où il est placé.
3. **Le rôle tactique** indique comment il doit jouer.
4. **La mentalité du rôle** indique son niveau de prise de risque.

Ces quatre notions ne doivent pas être confondues.

---

# 9. Rôles tactiques

## 9.1 Gardien

- Gardien classique
- Gardien relanceur
- Gardien-libéro

Effets possibles :
- distance de sortie
- prise de risque
- jeu au pied
- relance courte / longue
- position moyenne

---

## 9.2 Défenseur central

- Défenseur
- Stoppeur
- Couverture
- Défenseur relanceur
- Libéro

Effets possibles :
- hauteur de ligne
- sortie sur le porteur
- couverture
- prise de risque à la relance
- liberté de déplacement

---

## 9.3 Latéraux

Pour `DD / DG` :

- Latéral défensif
- Latéral soutien
- Latéral offensif
- Latéral inversé

Effets :
- fréquence des montées
- largeur
- centres
- position intérieure ou extérieure
- participation à la construction

---

## 9.4 Pistons

Pour `PIS_D / PIS_G` :

- Piston prudent
- Piston
- Piston offensif

Un piston doit réellement avoir une position moyenne plus haute qu'un arrière latéral classique.

---

## 9.5 Milieu défensif

Pour `MDC` :

- Sentinelle
- Récupérateur
- Meneur en retrait
- Demi-centre

Le demi-centre peut redescendre entre les défenseurs centraux pendant la possession.

---

## 9.6 Milieu central

Pour `MC` :

- Milieu soutien
- Box-to-box
- Récupérateur
- Meneur de jeu
- Mezzala / milieu excentré

Effets :
- volume de course
- pressing
- projection
- liberté
- position dans les demi-espaces

---

## 9.7 Milieu offensif

Pour `MOC` :

- Meneur offensif
- Numéro 10
- Attaquant de soutien
- Trequartista

---

## 9.8 Ailiers

Pour `AD / AG` :

- Ailier
- Ailier offensif
- Ailier intérieur
- Meneur excentré

Le pied préféré pourra plus tard fortement influencer ces rôles.

---

## 9.9 Attaquants

Pour `BU` :

- Avant-centre
- Renard
- Pivot
- Attaquant complet
- Attaquant en profondeur
- Faux neuf
- Attaquant pressing

Pour `SA` :

- Second attaquant
- Attaquant libre
- Neuf et demi

---

# 10. Mentalité du rôle

Chaque rôle peut recevoir une mentalité :

```text
DEFENSE
SUPPORT
ATTACK
```

Exemple :

```text
Poste : DD
Rôle : Latéral
Mentalité : Attaque
```

ou :

```text
Poste : MC
Rôle : Meneur
Mentalité : Soutien
```

Cette séparation évite de créer des dizaines de variantes de rôles presque identiques.

---

# 11. Consignes individuelles

Réservées principalement au mode carrière.

Exemples :

- reste derrière
- monte davantage
- presse davantage
- presse moins
- décroche
- prends la profondeur
- repique dans l'axe
- reste large
- centre rapidement
- centre de plus loin
- marque ce joueur
- liberté créative
- joue plus simple
- tente davantage de frappes
- conserve davantage le ballon
- dribble davantage
- dribble moins

Les consignes doivent modifier de vrais paramètres du moteur de match.

---

# 12. Compatibilité joueur / rôle

Le jeu peut afficher une aptitude calculée :

```text
Lucas Martin
Poste : MC

Box-to-box          ★★★★★
Meneur de jeu       ★★★★☆
Récupérateur        ★★★☆☆
Mezzala             ★★★☆☆
```

Il est préférable de **ne pas stocker une note indépendante pour chaque rôle**.

L'aptitude doit être calculée depuis les caractéristiques du joueur.

Exemple conceptuel :

```text
Box-to-box =
endurance
+ activité
+ vitesse
+ tacle
+ passe
+ placement
```

Cela permet aussi d'expliquer au joueur :

> Très adapté grâce à son endurance et son activité.

---

# 13. Effets réels dans le moteur de match

Les rôles ne doivent pas être purement décoratifs.

Ils doivent agir sur :

- position moyenne
- déplacements sans ballon
- appels
- pressing
- couverture
- largeur
- décrochages
- fréquence des centres
- fréquence des tirs
- prises de risque
- passes courtes / longues
- projection
- soutien offensif
- replis
- marquage
- hauteur de ligne

Exemples :

### Faux neuf
- décroche davantage
- quitte plus souvent la surface
- cherche les combinaisons
- libère de l'espace pour les ailiers

### Stoppeur
- sort davantage sur le porteur
- prend plus de risques défensifs

### Couverture
- reste plus bas
- protège la profondeur

### Box-to-box
- couvre une grande zone
- accompagne les attaques
- revient défendre

### Ailier intérieur
- repique vers l'axe
- cherche davantage le tir
- laisse le couloir au latéral

---

# 14. Même formation, comportements différents

Deux équipes peuvent jouer en 4-3-3 tout en ayant un style totalement différent.

## Exemple A — possession et mouvements intérieurs

```text
                BU
             Faux neuf
               Soutien

AG                                    AD
Ailier intérieur                  Ailier intérieur
Attaque                             Soutien

        MC               MC
     Mezzala          Box-to-box
     Attaque            Soutien

               MDC
            Sentinelle
             Défense

DG          DC       DC          DD
Offensif Défenseur Couverture Offensif
```

---

## Exemple B — jeu direct et récupération

```text
                BU
              Pivot
              Attaque

AG                                    AD
Ailier                              Ailier
Soutien                             Soutien

        MC               MC
    Récupérateur       Récupérateur

               MDC
             Sentinelle
              Défense

DG          DC       DC          DD
Défensif Stoppeur Stoppeur Défensif
```

Même dessin de formation, mais comportements très différents.

---

# 15. Entraîneurs IA

Cette architecture doit préparer les futures personnalités d'entraîneurs.

Un coach IA ne doit pas simplement choisir une formation.

Il doit pouvoir choisir un **modèle de jeu**.

Exemple :

```text
Coach A
Formation : 4-3-3
Meneur en retrait
Latéraux offensifs
Faux neuf
Pressing fort
Possession
```

contre :

```text
Coach B
Formation : 4-4-2
Deux lignes compactes
Pivot + profondeur
Latéraux prudents
Jeu direct
```

---

# 16. Interface utilisateur

## Mode Simple

L'interface doit rester légère.

Afficher :
- catégorie du joueur
- niveau
- formation
- poste dans le schéma
- quelques réglages collectifs

Ne pas afficher :
- pourcentages de maîtrise
- rôles détaillés
- apprentissage de postes
- dizaines de consignes

---

## Mode Carrière

L'interface peut afficher :

### Fiche joueur
- famille
- poste principal
- postes secondaires
- maîtrise
- pied préféré
- aptitude aux rôles
- progression

### Écran tactique
- poste
- rôle
- mentalité
- consignes individuelles

### Entraînement
- apprentissage d'un nouveau poste
- perfectionnement du rôle
- travail spécifique

---

# 17. Architecture de données recommandée

Exemple conceptuel :

```text
Player
{
    family: MID,

    positions:
    {
        MC: 100,
        MDC: 86,
        MOC: 74,
        MD: 45
    },

    primaryPosition: MC
}
```

Slot tactique :

```text
FormationSlot
{
    position: MC
}
```

Instruction :

```text
PlayerTactic
{
    role: BOX_TO_BOX,
    duty: SUPPORT,
    instructions: [...]
}
```

Profil de partie :

```text
GameRuleProfile
{
    mode: SIMPLE
}
```

ou :

```text
GameRuleProfile
{
    mode: CAREER
}
```

---

# 18. Architecture générale

```text
GAME RULE PROFILE
│
├── SIMPLE
│   ├── GK
│   ├── DEF
│   ├── MID
│   └── FWD
│
└── CAREER
    ├── famille GK / DEF / MID / FWD
    ├── postes détaillés
    ├── maîtrise des postes
    ├── rôles
    ├── mentalités
    ├── consignes individuelles
    ├── apprentissage
    └── évolution
```

---

# 19. Philosophie générale à réutiliser ailleurs

Cette séparation Simple / Carrière doit devenir une règle globale du projet.

| Système | Simple | Carrière |
|---|---|---|
| Poste joueur | GK/DEF/MID/FWD | détaillé |
| Plusieurs postes | Non | Oui |
| Rôles | Non | Oui |
| Consignes individuelles | Très limitées | Oui |
| Blessures | simple | détaillées |
| Fatigue | barre simple | condition + charge |
| Suspensions | automatiques | règles détaillées |
| Moral | Non | Oui |
| Contrats | Non | Oui |
| Staff | Non | Oui |
| Entraînement | Non | Oui |
| Formation jeunes | Non | Oui |
| Finances | Non | Oui |
| Scouting | Non | Oui |
| Relations joueurs | Non | Oui |

---

# 20. Ordre d'implémentation recommandé

## Étape 1 — Profils de règles

Créer :

```text
RULESET_SIMPLE
RULESET_CAREER
```

sans modifier le moteur physique.

---

## Étape 2 — Familles et postes détaillés

Conserver :

```text
GK
DEF
MID
FWD
```

comme familles.

Ajouter les postes détaillés uniquement pour la carrière.

---

## Étape 3 — Formation et slots

Séparer clairement :
- catégorie du joueur
- position naturelle
- slot de formation

---

## Étape 4 — Maîtrise des postes

Ajouter les valeurs de familiarité.

---

## Étape 5 — Interface carrière

Ajouter :
- poste principal
- postes secondaires
- niveau de maîtrise

---

## Étape 6 — Rôles tactiques

Ajouter progressivement les rôles par famille de poste.

Ne pas tout implémenter dans le moteur en une seule fois.

---

## Étape 7 — Mentalités

Ajouter :

```text
DEFENSE
SUPPORT
ATTACK
```

---

## Étape 8 — Effets moteur

Relier chaque rôle à de vrais paramètres :
- position
- appels
- pressing
- largeur
- projection
- centres
- tirs
- passes

---

## Étape 9 — Consignes individuelles

Ajouter les consignes une fois les rôles fonctionnels.

---

## Étape 10 — IA

Faire choisir aux entraîneurs IA :
- formation
- rôle
- mentalité
- modèle de jeu

---

## Étape 11 — Entraînement

Ajouter :
- apprentissage de poste
- perfectionnement
- évolution

---

## Étape 12 — Scouting et données avancées

Utiliser ensuite ces données dans :
- recrutement
- rapports
- comparaison joueurs
- génération de jeunes

---

# 21. Pas de compatibilité ancienne sauvegarde

La compatibilité avec les anciennes sauvegardes n'est pas nécessaire.

Cela permet de :
- restructurer proprement les données
- supprimer les anciens champs devenus inutiles
- éviter les couches de migration
- repartir sur une architecture claire

Il vaut mieux profiter de cette phase de développement pour faire une base saine.

---

# 22. Répartition possible du travail

Le chantier peut être séparé en deux grandes familles.

## Données / systèmes / simulation

Travail adapté à :
- structures des joueurs
- listes de postes
- rôles
- règles
- coefficients
- génération de données
- scouting
- progression
- compétitions
- règles de carrière
- cohérence de base de données
- documentation technique

## Graphismes / moteur de match / rendu

Travail adapté à :
- animations
- comportements visibles sur le terrain
- collisions
- déplacement
- placement
- effets des rôles
- interface tactique visuelle
- caméra
- rendu
- sensations de jeu

Les deux parties doivent utiliser une spécification commune afin d'éviter que les données et le moteur évoluent séparément.

---

# 23. Objectif final

Le mode Simple doit rester immédiatement accessible :

> choisir une équipe, une formation et jouer.

Le mode Carrière doit devenir beaucoup plus fin :

> choisir non seulement qui joue, mais où, comment, avec quel rôle, quelle mentalité et quelle évolution à long terme.

Le jeu conserve ainsi ses deux identités :

1. **un vrai jeu de football jouable rapidement**
2. **un vrai jeu de management très profond**

sans imposer la complexité du mode carrière aux joueurs qui veulent simplement jouer au football.
