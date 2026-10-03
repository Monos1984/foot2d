# Super Soccer World / France Foot 2D — 00.03.00 build 8

Extraire le ZIP Win64 puis lancer **FranceFoot2D.exe**. Windows 64 bits ; raylib intégrée, aucun raylib.dll à installer. Les sources et le script de compilation sont fournis séparément.

## Carrière de directeur sportif humain

Depuis le menu **Carrière → Directeur sportif** : prénom, nom, nationalité, âge, expérience, réputation, spécialité, philosophie et départ débutant / expérimenté / sans club / personnalisé. Un débutant ne peut pas prendre directement un club professionnel. Il peut chercher des postes accessibles à sa réputation, négocier une proposition, refuser, signer trois saisons, démissionner immédiatement ou programmer son départ. Le contrat personnel est renouvelable pendant sa dernière saison si le président conserve sa confiance.

Le menu dédié comporte vingt rubriques avec défilement. Le profil possède quinze compétences évolutives, des niveaux de réputation et un historique conservé lors des changements de club. Les offres, candidatures, durée, salaire et pouvoir sont affichés ; certaines négociations échouent.

**Vous dirigez l’effectif et choisissez l’entraîneur. Le coach IA décide de la formation, du onze, des rôles, de l’entraînement et des remplacements.** Les rencontres peuvent être observées ou simulées, avec les statistiques et les commandes de vitesse existantes. Les manettes, la gestion humaine du banc, les menus tactiques et l’apprentissage individuel sont verrouillés dans ce métier. Les autres modes gardent leurs commandes.

## Club, entraîneur et président

- Confiance du président, relation avec le coach et quatre niveaux de pouvoir. La situation financière, les résultats, les objectifs annuels, les jeunes et les désaccords influencent le mandat. Les objectifs sportifs initiaux suivent le championnat et la hiérarchie du club.
- Enveloppes transferts, staff, scouting et jeunes ; plafonds du marché de la passe 1 ; recalcul annuel. Une dépense importante requiert l’accord du président pour le joueur et les montants précis. Les budgets, salaires et contraintes DNCG françaises restent contrôlés lors de la signature réelle, du prêt entrant, de l’option, de la prolongation et des changements de salaire. Un accord n’autorise pas une autre recrue ni une hausse des montants.
- Projets à une, trois et cinq saisons : proposition, validation et bilans aux échéances. Réunions de décision bornées à une par mois, demandes de réallocation ou de changement de coach, crises et licenciement.
- Coach avec âge, réputation, salaire, contrat, philosophie, formation, style et attributs ; compatibilité avec le directeur. Prolongation ou recrutement d’un candidat, avec indemnité de rupture et autorisation selon les pouvoirs. Demandes de joueurs à accepter, refuser, reporter ou traiter par une alternative.
- Politique sportive : philosophie, âge cible, priorité jeunes / locale et limite des joueurs âgés influencent le classement des cibles ; priorité jeunes et profil du coach influencent ses choix. Les statuts et protections réutilisent la passe 1 ; les prix minima de vente sont contrôlés.
- Recrutement, dossiers, ventes, contrats, prêts, options, réserves et staff réutilisent les transactions existantes. Suivi des achats, salaires annuels enregistrés, ventes, résultat comptable et jeunes recrutés.
- Missions de scouting avec pays, région, poste, tranche d’âge et potentiel. Coût et réseau personnel ; vingt-quatre agences fictives avec relations qui influencent les négociations. Partenaires de prêts privilégiés lorsqu’ils respectent les critères de niveau et de temps de jeu.
- Historique personnel : clubs, saisons, achats, ventes, jeunes, entraîneurs, trophées et promotions effectives. Les événements de direction sportive apparaissent aussi dans la chronologie du musée du club.

Les coachs IA des divisions nationales du pays joué utilisent progressivement les mêmes profils et décisions de terrain. Le coach d’un ancien club reste conservé. Le moteur de match lui-même n’a pas été modifié.

## Sauvegarde et vérification

Format **31** : profil, compétences, contrat personnel, coachs, budgets, politiques, plans, dossiers de la passe 1, réseau, relations, trajectoires recommandées, partenariats, emplois et historique. Une vraie sauvegarde de build 7 / format 30 a été relue ; les anciennes carrières ne deviennent pas automatiquement des carrières DS.

Les régressions des builds précédentes, les contrôles financiers DS, une sauvegarde après dix changements de saison et les écrans du nouveau métier sont vérifiés. Le test sur dix saisons simule complètement le championnat sélectionné et utilise les transitions réelles du jeu ; les autres compétitions sont clôturées dans cette fixture bornée. Il ne représente pas dix saisons complètes de tous les championnats du monde.

Le script source **build-win64.ps1 -Compiler <clang++.exe> -Raylib <dossier-raylib> -Tests** construit l’exécutable et les tests. Les preuves de livraison sont dans `verification/` du ZIP source.

## Fonctions encore simplifiées / à poursuivre

Cette build livre une première version jouable du métier, et non l’intégralité des 58 sections du document de passe 2. Restent à approfondir : les portefeuilles et commissions individuels des agents ; le scouting différé et les scouts nommés ; la prospection active d’acheteurs ; les prix de vente souhaités et la vente rapide ; les trajectoires automatiques des jeunes (les recommandations sont conservées, les déplacements utilisent le menu des réserves) ; les partenariats financiers et centres communs ; les clauses personnalisées / indemnités du contrat du directeur ; les catégories détaillées de réunions et crises ; le directeur de sélection ; l’application du modèle de direction sportive à tous les clubs IA et à leurs transferts.

Les projets restent fondés sur des objectifs simples de classement. Les enveloppes jeunes et certains choix de stratégies prêts / ventes sont une base de planification ; ils ne pilotent pas encore une simulation complète de formation et de trading. Les agences et coachs sont générés pour la simulation et ne prétendent pas représenter des personnes réelles. Le musée conserve des événements DS ; une salle dédiée aux directeurs historiques reste à créer.

Les compositions officielles France 2026/27 restent **partielles : 3 761 équipes et 327 groupes importés**. Cette build ne complète pas les ligues et districts restants ; la couverture détaillée reste dans les rapports France existants. Aucun groupe généré supplémentaire n’est présenté comme officiel.
