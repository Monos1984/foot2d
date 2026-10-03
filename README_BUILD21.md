# France Foot 2D / Super Soccer World — 00.03.00 build 21 : sauvegardes

Le téléchargement contient `FranceFoot2D_v00.03.00_build21` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build21_Source` (code source complet).

Cette build est consacrée aux sauvegardes. Le gameplay n'a pas changé.

## Le « souci dans les sauvegardes » : chargement impossible
- **Problème** : une carrière sauvegardée ne se rechargeait plus.
- **Cause** : un club pouvait devenir « rival de lui-même » dans le module Supporters, et le contrôle au chargement rejetait alors toute la sauvegarde.
- **Correction** :
  - une rivalité avec soi-même n'est plus créée ;
  - au chargement, une rivalité invalide est retirée au lieu de faire échouer la sauvegarde ;
  - les sauvegardes existantes touchées par ce problème se rechargent donc à nouveau.

## Profil de la sauvegarde (FOOT_SAVE_PROFILE=1 ou option --save-profile)
Même carrière, à la 10e journée.

| Bloc | Avant | Après (brut) |
|---|---|---|
| Musée | 427,6 Mo | 0,8 Mo |
| Personnalité | 128,7 Mo | 17,4 Mo |
| Supporters | 53,7 Mo | 21,7 Mo |
| Postes / tactiques | 30,1 Mo | 8,0 Mo |
| Joueurs (effectifs) | 23,3 Mo | 21,2 Mo |
| Compétitions de la saison | 21,3 Mo | 13,0 Mo |
| Clubs | 10,3 Mo | 5,4 Mo |
| V10-V11 (historique des joueurs) | 7,7 Mo | 4,4 Mo |
| **Total brut** | **703,6 Mo** | **92,4 Mo** |
| **Sur disque** | **737,8 Mo** | **35,7 Mo** |

| Temps (machine de test Linux) | Avant | Après |
|---|---|---|
| Sauvegarde | 1,4 s (pour 738 Mo écrits) | 2,7 s (dont 1,3 s de compression) |
| Chargement | 4,1 s | 2,8 s |

## Ce qui prenait la place, et la correction
1. **Musée** :
   - **Quels clubs** : il suivait les 26 608 clubs du monde. Il suit désormais vos clubs (et ceux que vous avez dirigés) et les 6 premiers niveaux de votre pays. Les autres clubs gardent leurs « archives antérieures » ; leur Musée commence le jour où vous y arrivez.
   - **Clubs jamais dirigés** : 8 matchs de légende sans les compositions, 60 événements dans la chronologie, sans les événements de record qui faisaient doublon avec la liste des records, et les 80 joueurs les plus marquants.
   - **Fiches joueurs vides** : elles ne sont plus créées.
   - **Matchs de légende** : un record n'en fait un que s'il est marquant (écart de 4 buts ou plus, 7 buts ou plus dans le match).
2. **Personnalité** :
   - nom omis s'il est celui de l'effectif ;
   - traits et relations notés en écart à leur valeur par défaut ;
   - retraités et joueurs sortis des effectifs conservés seulement s'ils comptent pour le Musée ou un de vos clubs ;
   - joueurs sans lien avec vos clubs : relation actuelle et précédente, dernier fait marquant.
3. **Supporters**, pour les clubs autres que le vôtre : 6 événements, 25 popularités de joueurs, 6 rivalités.
4. **Postes** : non sauvegardés quand ils sont identiques à la valeur générée.
5. **Format compact dans tout le fichier** :
   - entiers en varint ;
   - suites de zéros regroupées ;
   - chaînes à longueur variable ;
   - registres « déjà traité » encodés par écarts.
6. **Compression LZ rapide** (sans dépendance), par blocs.

## Sécurité
- **Écriture atomique** : fichier temporaire vérifié, puis l'ancienne sauvegarde devient `.bak`.
- **CRC32** sur les données. Un fichier corrompu (en-tête, données, troncature) est refusé proprement, et la copie `.bak` est chargée automatiquement.
- **Nombres d'éléments** bornés à la lecture.
- **Compatibilité** : les anciennes sauvegardes (versions 21 à 34) se chargent et sont réécrites au nouveau format (version 35) à la sauvegarde suivante.

## Croissance (simulation automatique, sauvegarde de fin de saison sur disque)

| Saison | Sur disque | Brut |
|---|---|---|
| Début de carrière | 4,3 Mo | 21 Mo |
| 1 | 38,5 Mo | 120 Mo |
| 2 | 45,6 Mo | 134 Mo |
| 3 | 52,8 Mo | 154 Mo |
| 4 | 57,7 Mo | 167 Mo |
| 5 | 61,4 Mo | 175 Mo |
| 6 | 64,7 Mo | 183 Mo |
| 7 | 67,0 Mo | 188 Mo |
| 8 | 69,5 Mo | 193 Mo |

À partir de la 5e saison, la croissance n'est plus que d'environ +2,5 Mo par saison. Avec l'ancien format, la même simulation donnait 518 Mo dès la 3e saison, avec +50 Mo par saison.

La mesure prévue sur 10 saisons a été arrêtée à la 9e ; 30 saisons n'ont pas été testées.

Une sauvegarde faite en milieu de saison est 10 à 11 Mo plus grosse qu'en fin de saison (80,2 Mo en cours de 9e saison) : la saison en cours contient plus de données.

## Documentation et tests
- `docs/SAVE_FORMAT.md` : format, bornes des historiques, outils.
- `tools/test_save_integrity.cpp` : aller-retour identique au bit près, état restauré, corruption, repli sur `.bak`.
- `tools/test_save_growth.cpp N budget` : taille saison par saison, échec si le budget est dépassé.
- `tools/test_save_legacy.cpp` : chargement d'une ancienne sauvegarde (version 34) et conversion.

## Points restant à optimiser
- **Effectifs** (21 Mo brut) et **compétitions de la saison** : données de jeu nécessaires, déjà compactées.
- **Supporters des clubs lointains** (environ 30 Mo brut) : un profil complet pour chacun des 26 608 clubs. Un profil allégé pour les clubs jamais rencontrés réduirait encore la taille.
- **Historique de carrière des joueurs** (V10-V11, environ +3 Mo brut par saison) : borné naturellement par les retraites.
- **Temps de sauvegarde** : 3 à 6 s sur la machine de test pour une carrière avancée. Il est en partie dû au passage par un fichier brut temporaire, qu'une écriture directe en mémoire supprimerait.
- **Nombre de joueurs du monde** : il augmente d'environ 10 % en quelques saisons (les jeunes générés dépassent les départs). C'est un réglage de jeu, sans rapport avec le format, mais il fait grossir les blocs Joueurs et Personnalité.
