# France Foot 2D / Super Soccer World — 00.03.00 build 25 : Marseillaise complète, voix du public, applaudissements, sifflets

Le téléchargement contient `FranceFoot2D_v00.03.00_build25` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build25_Source` (code source complet).

Les sauvegardes sont inchangées.

## La Marseillaise complète
- **Contenu** : premier couplet en entier, puis le refrain « Aux armes, citoyens ! Formez vos bataillons ! Marchons, marchons ! Qu'un sang impur abreuve nos sillons ! ».
- **Source** : la partition piano et chant d'Alexis Jeandeau, dédiée au domaine public (CC0) : https://github.com/jeandeaual/lilypond-piano-la-marseillaise. Rien n'est écrit de mémoire : la partition LilyPond est convertie note par note par `tools/ly_to_notes.py`.
- **Contrôle** :
  - les trois voix (chant, main droite, main gauche) font exactement 29 mesures ;
  - la mélodie a été relue mesure par mesure : « Allons enfants », « Contre nous de la tyrannie », « l'étendard sanglant est levé », « Entendez-vous… », « Ils viennent jusque dans nos bras », puis « Aux armes, citoyens » (fa tenu, puis fa ré si♭ do).
- **Arrangement de fanfare**, en si bémol majeur, à 104 à la noire (environ 70 secondes) :
  - trompettes au chant, doublées à l'octave grave ;
  - cors et cordes sur les accords du piano ;
  - basse sur la main gauche ;
  - roulement de timbales sur les trémolos ;
  - caisse claire de marche, cymbales au refrain et à la fin.
- **En match** : l'hymne est joué en entier avant le match (la cérémonie suit sa durée, jusqu'à 90 s ; OK passe toujours la cérémonie). On peut aussi l'écouter dans Options > Tester les sons > Hymnes.

## Voix du public (chants)
- **Problème** : les voix étaient trop synthétiques.
- **Nouvelle synthèse** :
  - **la foule** : chaque supporter a sa propre voix, avec une hauteur un peu instable, une voix plus ou moins éraillée et une justesse approximative ; il attaque un peu en avance ou en retard, et saute parfois une syllabe. Ce sont surtout des voix d'hommes, avec 40 à 60 voix par chant ;
  - **les voyelles** : la foule passe dans trois formants qui glissent d'une voyelle à l'autre (a, é, o, i, ou), plus la résonance de la poitrine et le souffle des cris ;
  - **les consonnes** : sifflantes (« Dé-mis-**s**ion »), occlusives (« **D**é… »), attaques douces (« al-**l**ez », « o-**l**é »), souffle (« **H**ou ! » du clapping viking) ;
  - **le stade** : son étouffé par la distance, écho de la tribune d'en face, réverbération.
- **Fanfare des tribunes** : elle reprend « Olé, olé » et le riff « Oh oh oh oh oh ».
- **Chants concernés** : les 12, ainsi que les huées et la ola.

## Applaudissements
- **Avant** : un bruit filtré.
- **Maintenant** : 220 personnes applaudissent, chacune à son rythme (3 à 7 claquements par seconde), avec des mains différentes (24 timbres de claquement), à des distances différentes, plus la rumeur et l'acoustique du stade.
- **Clappements en rythme** (« clap clap », clapping viking) : 40 paires de mains, pas tout à fait ensemble.

## Sifflets du public
- **Sifflements aux doigts** : son pur et souffle filtré à la même fréquence, par salves.
- **Quatre formes** : tenu, montant, descendant, et le sifflet « loup » qui monte puis redescend.
- **Variété** : distances différentes, acoustique du stade.

## Vérifications
- Compilé sous Windows et sous Linux.
- Tests :
  - `tools/test_anthems.cpp` : 40 hymnes générés, dont La Marseillaise (70,7 s) ;
  - nouveau `tools/test_crowd_audio.cpp` : les 12 chants, les applaudissements et les sifflets, avec contrôle du niveau, sans saturation.
- **Temps de calcul** : environ 2,5 s au total sur la machine de test (Linux, version de test moins optimisée), pendant le chargement du jeu.
- **Non écouté** : je ne peux pas écouter le son sur la machine de test. Le réalisme des voix, des applaudissements et des sifflets est à juger à l'oreille. Tous ces sons s'écoutent dans Options > Tester les sons.
