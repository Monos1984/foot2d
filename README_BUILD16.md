# France Foot 2D / Super Soccer World — 00.03.00 build 16

Le téléchargement contient `FranceFoot2D_v00.03.00_build16` (jeu Windows 64 bits : lancez FranceFoot2D.exe) et `FranceFoot2D_v00.03.00_build16_Source` (code source complet). Format de sauvegarde inchangé.

## Menu pause
- Nouveau panneau : tableau d'affichage, mini-terrain avec la position des joueurs et du ballon, possession, tirs, tirs cadrés, corners et fautes.
- Consignes rapides en plein match : pressing (faible / normal / fort) et ligne défensive (basse / normale / haute), en plus de la mentalité.

## Bagarres
- Le coéquipier le plus proche de chaque bagarreur vient le ceinturer, les autres font cercle, l'arbitre accourt et s'interpose.
- Échauffourée générale possible : un joueur de chaque camp s'en mêle et peut être averti à la fin.
- Après la bagarre, les capitaines calment leurs coéquipiers.
- Graphismes : nuage de bagarre façon dessin animé (bras et jambes qui dépassent, étoiles, injures en symboles), poussière et « !! » pour les bousculades ; combat au corps à corps dans un ring entouré du public, onomatopées (PAF ! BAM ! POW !), étoiles quand un joueur est sonné.

## Coups de pied arrêtés
- Corners de l'ordinateur variés : premier poteau, second poteau, point de penalty ou corner à deux, ballon rentrant ou sortant, sur le meilleur joueur de tête démarqué.
- Coups francs excentrés ou indirects près de la surface : centre sur le meilleur joueur de tête ; le mur saute au moment de la frappe directe.
- Longues touches dans la surface près du but adverse.
- Penalties : élan du tireur, gardien qui sautille sur sa ligne, lecture occasionnelle de la frappe par le gardien, panenka rare pour les meilleurs tireurs (taux de réussite inchangé, environ 70 %).

## Scénettes télévisées
Pendant les arrêts de jeu, incrustations façon réalisation télé : un supporter en gros plan, le kop (capo, tambour), le banc et l'entraîneur, l'échauffement, la mascotte, le président en loge, le parcage visiteur, un jeune supporter, la tribune de presse.

## Envahissement de terrain
Dernier match à domicile d'un club qui termine à une place de montée : au coup de sifflet final, les supporters peuvent envahir la pelouse (environ une fois sur deux) et fêter la montée avec les joueurs, drapeaux compris. Entrée : passer.

## Supporters
Bâches « ALLEZ ... » devant le kop et bâche du parcage visiteur, capo au mégaphone et deux tambours (plus rapides quand l'ambiance monte), ultras en noir, torse nu par forte chaleur, familles en tenue claire dans les tribunes latérales.

## Éditeur : publicités (images)
- Panneaux du stade : taille conseillée 276 x 40 px (rapport 7:1), affichés sur 69 x 10 pixels du terrain, un panneau sur deux (image tournée le long des lignes de touche).
- Pages de pub TV : taille conseillée 640 x 360 px (ou 1280 x 720), ajoutées aux pauses publicitaires des matchs télévisés.
- Aperçu, dimensions réelles de l'image et rapport conseillé ; formats PNG, JPG, BMP ; glisser-déposer accepté. Enregistré dans pubs.txt.

## Ambiance et télévision
- Cabine des commentateurs au début des matchs télévisés : vue sur le stade et l'échauffement, les deux voix de la chaîne avec casques et micros, phrase d'ouverture, puis les compositions.
- Musique de but jouée par la sono du stade après un but à domicile (grands stades ou grosse ambiance).
- Écran de résultat : bandeau des tribunes et confettis aux couleurs du vainqueur.

## Moteur et IA
- Passes en profondeur dans le dos de la défense pour un attaquant au contact de la ligne, qui file vers le ballon.
- Passes en retrait depuis la ligne de but vers un partenaire seul au point de penalty.
- Simulation ordinateur contre ordinateur (120 matchs) : buts stables (0,56 par match courte durée), moins de hors-jeu.
