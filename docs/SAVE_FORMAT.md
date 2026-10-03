# Format des sauvegardes — version 35 (build 21)

## Conteneur (`src/savefile.cpp`)

Le fichier `.sav` est un conteneur vérifié, qui enveloppe les données brutes de la carrière.

| Champ | Type | Contenu |
|---|---|---|
| magic | u32 | `SSWZ` |
| version du conteneur | u32 | 2 (compression LZ rapide) |
| drapeaux | u32 | bit 0 : données compressées |
| taille brute | u64 | taille des données décompressées |
| CRC32 | u32 | somme de contrôle des données brutes |
| nombre de blocs | u32 | blocs de 16 Mo au plus |
| blocs | — | pour chaque bloc : u32 taille brute, u32 taille compressée, octets compressés |

- **Compression** : LZ (format de bloc de type LZ4), écrite dans le projet, sans dépendance externe. Chaînes de hachage à 8 candidats, fenêtre de 64 Ko, évaluation paresseuse.
- **Écriture atomique** :
  1. données brutes dans `x.sav.raw` ;
  2. conteneur dans `x.sav.tmp`, puis vidage, fermeture et relecture de l'en-tête ;
  3. l'ancien `x.sav` devient `x.sav.bak`, `x.sav.tmp` devient `x.sav` ;
  4. le fichier brut est supprimé.

  Un plantage pendant l'écriture laisse donc intacte la sauvegarde précédente.
- **Chargement** :
  1. vérification de l'en-tête, des tailles et du CRC32 ;
  2. décompression dans `x.sav.rawload`, puis lecture habituelle ;
  3. un fichier sans en-tête `SSWZ` est lu comme ancien format brut (versions 21 à 34) ;
  4. si le fichier est corrompu ou illisible, `x.sav.bak` est essayé automatiquement.

## Données brutes (`Career::saveRaw` / `Career::loadRaw`)

- Après `SAVE_MAGIC` et la version (non compressés), tout le flux est en **mode packed** (`src/serial.h`) :
  - toute valeur triviale est lue comme une suite d'entiers 32 bits, chacun en varint zigzag ; une suite de zéros devient un seul jeton (0 + longueur) ;
  - chaînes de taille fixe (`char[N]`) : longueur + caractères utiles ;
  - registres triés « déjà traité » (`std::vector<uint64_t>`) : écarts successifs en varint.
- **Modules à format explicite** (`src/packio.h`) :
  - **Personnalité** : identifiant en écart au précédent, club et âge en écart à l'effectif actuel. Le nom est omis s'il est celui du joueur en effectif. Les traits sont notés en écart aux traits générés depuis l'identifiant, les relations en écart à la relation par défaut, les textes à longueur variable.
  - **Postes** : omis s'ils sont identiques à la valeur générée depuis l'identifiant, sinon notés en écart à cette valeur.
  - **Musée, Supporters** : mêmes fonctions d'entrée-sortie, écrites en mode packed.

## Bornes des historiques

| Donnée | Clubs du joueur (et clubs dirigés) | Autres clubs |
|---|---|---|
| Musée | complet | seulement les 5 premiers niveaux du pays du joueur ; ailleurs, les « archives antérieures » (`archive.cpp`) |
| Matchs de légende | 150 | 8, sans les compositions |
| Chronologie du Musée | 1 200 événements | 60 événements, sans les événements de record (déjà dans la liste des records) |
| Fiches joueurs du Musée | toutes | les 80 plus marquantes (panthéon, effectif actuel et saison en cours toujours gardés) |
| Événements supporters | 120 | 6 |
| Popularité des joueurs (supporters) | 180 | 25 |
| Personnalité d'un joueur retraité | gardée | gardée seulement s'il compte pour le Musée, un club du joueur ou a des événements |

- Les fiches joueurs du Musée sans aucune donnée ne sont plus créées. Une fiche est créée au premier match du joueur, à l'identique de ce qu'elle aurait été.
- Les registres « déjà traité » (`seen*`, `prepared*`) sont vidés à chaque saison.

## Outils

- `FOOT_SAVE_PROFILE=1` ou l'option `--save-profile` affiche, à chaque sauvegarde et chargement :
  - le profil par bloc (taille, nombre d'éléments, octets par élément) ;
  - le détail du Musée et des Supporters ;
  - les temps de sérialisation, compression, écriture, lecture et décompression.

  Avec `--save-profile`, le rapport est écrit dans `save_profile.txt` à côté du jeu.
- Tests :
  - `tools/test_save_integrity.cpp` : aller-retour identique au bit près, état restauré, corruption (en-tête, données, troncature, blocs absurdes) et repli sur `.bak` ;
  - `tools/test_save_growth.cpp N [budget]` : taille saison par saison ; échoue si le budget en Mo sur disque est dépassé ;
  - `tools/test_save_legacy.cpp` : chargement d'une sauvegarde version 34 et conversion.
