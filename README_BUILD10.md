# Super Soccer World / France Foot 2D — 00.03.00 build 10

Extraire le ZIP Windows 64 bits et lancer **FranceFoot2D.exe**. Raylib est intégrée. Cette livraison ajoute la personnalité des joueurs en carrière complète ; les évolutions des builds précédentes sont conservées.

## Utilisation

Ouvrir la fiche d'un joueur, puis utiliser **Tab** pour passer entre fiche, postes/rôles et personnalité. Les joueurs du club affichent dix traits qualitatifs, leur profil, leur relation au club et leur adaptation. La rubrique historique conserve les événements marquants. Pour un jeune de 23 ans maximum, le bouton Mentor propose les coéquipiers de 25 ans minimum avec professionnalisme et leadership suffisants.

Les rapports de joueurs extérieurs révèlent progressivement les traits selon les connaissances et la qualité d'observation. Un joueur inconnu reste difficile à évaluer ; une observation faible révèle deux traits, une observation intermédiaire cinq et une observation approfondie les dix. Le directeur sportif utilise les traits connus dans ses recommandations, avec la même incertitude que le rapport.

## Comportement implémenté

Les dix traits (ambition, professionnalisme, loyauté, agressivité, leadership, sang-froid psychologique, régularité, tempérament des grands matchs, susceptibilité et adaptation étrangère) sont séparés des attributs footballistiques. Ils sont générés de façon déterministe, sans consommer le hasard du moteur. **Ces profils sont des simulations de jeu, pas des évaluations psychologiques documentées de personnes réelles.**

Le professionnalisme et l'ambition influencent l'entraînement et l'apprentissage de postes. Le professionnalisme modère le déclin et aide la récupération. L'ambition peut créer une demande de départ après plusieurs saisons de stagnation ; une montée ou un titre apaise cette demande. La loyauté et l'attachement modèrent les exigences salariales et facilitent les prolongations. La demande de départ n'impose pas une vente automatique.

Le mentorat fait progresser lentement le professionnalisme et le leadership. Le capitaine est choisi selon son leadership, sa loyauté, son professionnalisme et son ancienneté. La perte du brassard, le banc et un remplacement prématuré peuvent décevoir les joueurs sensibles ; les réactions sont limitées et ne se cumulent pas à chaque clic. Les leaders loyaux atténuent les pertes de moral du groupe.

Chaque joueur conserve une relation propre à chaque club : attachement, popularité, relation au manager, influence, intégration, ancienneté et mentor. Les transferts conservent les traits intrinsèques et les anciens liens. Un prêt marque la relation au club d'accueil comme temporaire ; un retour retrouve l'intégration déjà acquise. Les archives restent sauvegardées après la retraite, y compris pour les joueurs libres.

L'intégration étrangère progresse mensuellement selon le profil, les compatriotes, les apparitions, le moral, l'expérience et le mentor. La langue est représentée indirectement par le pays et les compatriotes ; il n'y a pas de cours de langue ou de liste de langues individuelles dans cette passe.

La popularité rejoint le système Supporters. Les événements majeurs rejoignent le musée. Un leader fidèle présent depuis plusieurs saisons reçoit un petit bonus pour l'admission parmi les légendes ; ses statistiques historiques restent inchangées.

Les matchs simulés et joués reçoivent des variations limitées de forme et de pression. Le sang-froid intervient sur les situations stressantes, le tempérament sur les rencontres importantes, l'agressivité étant modérée par le professionnalisme et le sang-froid. Les mêmes hooks s'appliquent aux remplaçants. Les modes Simple et entraînement n'activent pas cette gestion psychologique persistante.

## Équilibrage et limites

- Multiplicateur d'entraînement : 0,60 à 1,45 ; apprentissage : professionnalisme ; gains annuels limités par le potentiel existant.
- Demande salariale : 0,80 à 1,20 de la demande calculée par les règles existantes.
- Variation de performance en match : 0,955 à 1,040. L'influence moyenne sur une équipe est généralement beaucoup plus faible.
- Stress : 0,87 à 1,13 ; facteur de sévérité lié au tempérament : 0,88 à 1,12. Ces facteurs modulent le moteur existant, sans garantir un but ou un carton.
- Mentorat : un point par trait concerné tous les trois mois ; vieillissement des traits : généralement un à deux points par saison.
- Bonus légende : au plus 250 points, après trois saisons, pour un seuil historique d'admission de 1 500 points.
- Historique : 60 événements et 80 relations de club maximum par joueur. Les profils de retraités sont conservés ; la taille globale augmente avec une longue carrière.

Les demandes de départ restent des signaux et des refus contractuels, sans ultimatum automatique ni agent négociant autonome. Les réactions aux critiques existent dans le mécanisme de décisions, mais cette passe n'ajoute pas de dialogue individuel de critique publique. L'adaptation des joueurs IA est actualisée lorsqu'ils jouent et lors des bilans ; elle n'est pas simulée quotidiennement pour tout le monde. Les archives retraitées sont conservées mais n'ont pas de nouvel annuaire psychologique indépendant dans le musée. L'équilibre des effets demande encore des retours de parties humaines longues.

## Sauvegardes et vérification

Le format **33** ajoute les profils, relations, événements et connaissances d'observation. Les anciennes sauvegardes restent lisibles ; elles génèrent les traits à la première utilisation, sans inventer un historique psychologique passé. Une vraie sauvegarde de build 9 / format 32 est vérifiée dans les tests.

La suite dédiée compare des joueurs de même niveau, vérifie progression, déclin, potentiel, émotions, capitanat, mentorat, contrats, transferts, prêts, adaptation, scouting, supporters, musée et limites de performance. Dix saisons du championnat sélectionné sont terminées avec de vraies transitions annuelles ; les autres compétitions sont marquées terminées pour isoler ce test. Ce contrôle parcourt aussi les profils des effectifs IA et recharge l'archive finale ; il ne constitue pas dix saisons intégrales de toutes les compétitions mondiales. Les vérifications des autres fonctionnalités et des écrans figurent dans `verification/` du ZIP sources.

## Données France 2026/27

Cette passe ne modifie pas les compositions officielles : **3 761 équipes importées, 327 groupes**. La couverture régionale et départementale reste partielle. Les équipes et groupes générés encore présents ne sont pas présentés comme officiellement vérifiés. Consulter les rapports de couverture et les README antérieurs pour les travaux précédents, notamment le directeur sportif et les supporters.
