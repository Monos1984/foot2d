# Super Soccer World / France Foot 2D — 00.03.00 build 6

Extraire le ZIP Win64 et lancer FranceFoot2D.exe. Les sauvegardes de la build 5 restent compatibles (format 29 inchangé).

## Menu de carrière corrigé

Le menu dépassait la hauteur disponible lorsque les distinctions, récompenses, Pôles Espoirs et options de partie étaient présents. La réduction automatique des lignes s'arrêtait à une hauteur minimale et les dernières rubriques passaient sous le bandeau du bas.

Le menu possède maintenant une grille de catégories : Match, Compétitions, Club (ou Ma carrière / Sélection), Palmarès, Formation lorsqu'elle est disponible, et Partie. Seules les options de la catégorie choisie apparaissent sous la grille. Leur hauteur est limitée à la zone disponible ; une liste plus longue défile dans ce panneau.

Palmarès regroupe le palmarès des compétitions, le Ballon d'or, les récompenses du championnat et le musée du club, selon les fonctions disponibles dans le mode joué. Il n'y a plus de rubriques séparées Distinctions et Fin de saison pour les cérémonies. Le bilan de fin de saison reste dans Match une fois la saison terminée.

Sauvegarder, Options et Menu principal sont accessibles dans Partie. Les classements et les derniers résultats restent à droite.

## Commandes

Cliquer sur une catégorie, ou utiliser Gauche/Droite ou Tab. Haut/Bas, la molette et le clic parcourent les options ; Entrée ouvre l'option choisie. Retour garde le comportement du menu principal avec sauvegarde automatique.

## Vérifications

Les scénarios d'affichage couvrent les six catégories d'une carrière française, le bilan de fin de saison, le championnat simple, l'Allemagne et la Belgique. Ils vérifient aussi l'ouverture effective du palmarès, du Ballon d'or, des récompenses, du musée, de la sauvegarde et des options depuis le nouveau menu. Une vérification automatique contrôle que le panneau des options reste au-dessus du pied de page. Les scènes des fonctions précédentes sont également rejouées.

Les preuves de cette build figurent dans verification du ZIP source. Les résultats des suites de la build 5 sont conservés comme vérifications antérieures, sans être présentés comme de nouvelles exécutions de ces suites dans cette build. Les changements de cette livraison concernent l'interface et le numéro de build.

Résultats de cette build : 49 scénarios d’affichage et d’ouverture réussis ; validateur France réussi (3 761 équipes importées) ; exécutable Windows x86_64, bibliothèque raylib intégrée.
