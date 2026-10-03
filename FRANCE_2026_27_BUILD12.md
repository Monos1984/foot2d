# Règle de régularisation — build 12

## Effectif courant et effectif suivant

Pool.groupSizes représente les groupes de la saison courante. Pool.nextGroupSizes permet une taille réglementaire distincte pour chaque groupe de la saison suivante. Si ce dernier champ est vide, le moteur utilise le nombre configuré de groupes (nGroups) et la taille standard (size). Ces objectifs sont des réglages du jeu, pas une certification de tous les règlements officiels de chaque ligue.

Les changements prennent effet au passage à la saison suivante, après les résultats sportifs ordinaires et les barrages. Les groupes publiés du départ ne sont pas modifiés pendant la saison.

## Ordre des mouvements

1. Calcul des promotions, relégations et barrages habituels.
2. Respect des restrictions des réserves et affectation dans leur zone.
3. Retour à la taille cible, du sommet vers le bas. Le moteur descend prioritairement les équipes déjà présentes dans le niveau, classées selon leur position relative et les points par match, pour préserver les entrants issus des mouvements ordinaires.
4. Comblement des places par les meilleurs clubs éligibles du niveau inférieur réellement existant. Une équipe déjà promue ne peut pas franchir une deuxième division existante pendant la même transition. À défaut de promouvable, un relégué éligible peut être repêché.
5. Nouveau contrôle des réserves et répétition des ajustements si leur descente provoque un dépassement plus bas.
6. Recomposition géographique des groupes aux tailles prévues.

Les descentes peuvent cascader à travers plusieurs divisions. Les promotions, repêchages et descentes restent dans la bonne ligue ou le bon district. Le club reste présent si aucun niveau inférieur n'existe : l'ancien chemin qui pouvait alors dupliquer une équipe a été corrigé.

La régularisation ne crée aucun club. Les niveaux terminaux ou flexibles accueillent les effectifs restants. Un objectif non atteint faute de mouvement éligible est annoncé, plutôt que présenté comme régularisé.

## Validation

Tests contrôlés : surplus sur deux niveaux ; déficit ; déséquilibre entre groupes avec total correct ; objectifs différents par groupe ; absence de niveau inférieur ; réserve entraînée par son équipe première ; division intermédiaire inexistante ; limitation des relégations lorsque les réserves du dessous ne peuvent monter ; conservation et unicité des équipes. Deux saisons complètes de carrière France sont simulées, avec contrôle des effectifs de chaque niveau fixe après transition.

Le validateur possède un mode normalized pour rejeter les groupes dont l'effectif ne correspond pas à la cible après transition. Les sauvegardes des builds 10 et 11 sont chargées dans les tests ; la persistance des nouvelles cibles est contrôlée.

Les compositions officielles et les sources sont inchangées par rapport à FRANCE_2026_27_BUILD11.md. Ce lot applique la règle demandée ; le chantier d'import des autres ligues et districts reste ouvert.
