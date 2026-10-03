# Super Soccer World / France Foot 2D — 00.03.00 build 7

Extraire le ZIP Win64 puis lancer **FranceFoot2D.exe**. Version du jeu : **00.03.00** ; build : **7**. Bibliothèque graphique intégrée ; aucun raylib.dll à installer. Le ZIP source fournit aussi le programme de vérification des données France.

## Directeur sportif — première passe

Depuis **Club → Gestion du club → Directeur sportif**, ou depuis le poste correspondant dans **Staff** :

- Fiche : nom, âge, nationalité, salaire, années de contrat, expérience, réputation, douze compétences et philosophie.
- Responsabilités : quinze domaines indépendants, réglables entre Manuel, Conseil, Négociation et Complète. Tout est manuel au départ. Les préréglages Conseil et Complète nécessitent un clic explicite.
- Limites : indemnité, salaire annuel individuel, part du budget, tranche d'âge et plafond salarial joueurs + staff. L'enveloppe du président est recalculée par saison ; les plafonds choisis sont conservés, y compris zéro. Une limite humaine ne permet pas de dépasser le président, les fonds disponibles ou les contraintes DNCG françaises.
- Rapport : couverture des postes dans la formation actuelle, blessures et suspensions, réserves / jeunes déjà disponibles, vieillissement, contrats courts et finances.
- Cibles : listes bornées, informations estimées, motifs, niveau, potentiel, âge, coût, intérêt et connaissance. Le réseau et la philosophie affectent le classement ; la recherche amateur privilégie les joueurs accessibles localement. Les effectifs du réseau sont créés à la demande.
- Négociations : préparation, discussion avec le club, contrat, validation, signature ou abandon. Les compétences influencent les propositions, le délai et le risque d'échec. Les accords passent par les transactions existantes. En mode Négociation, vous choisissez la cible et validez avant engagement. Les étapes financières requièrent chacune une délégation suffisante ; reprendre une responsabilité met le dossier en pause. Une contre-proposition qui modifie le montant exige une nouvelle validation humaine. Les fenêtres de mercato et quotas restent actifs.
- Contrats : priorités des titulaires, capitaines et jeunes prometteurs. La délégation complète prolonge les contrats urgents sans répéter la prolongation à chaque match. Le contrat d'un joueur prêté appartient à son club prêteur.
- Prêts : part salariale choisie pour les futurs dossiers, recherche d'un club adapté au niveau et au poste, refus d'une destination où le joueur serait troisième choix, option d'achat des prêts négociés. La part salariale est enregistrée dans le dossier ; modifier le réglage ne change pas un accord en cours. L'option reste soumise aux limites et au budget.
- Statuts : Intouchable, Important, Rotation, Disponible, À vendre, À prêter, plus **NE PAS VENDRE**. Ce verrou empêche toute vente ou prêt par le directeur sportif. Les titulaires et capitaines sont protégés automatiquement tant que vous ne les placez pas explicitement sur les listes.
- Demande du coach : poste, rôle, tranche d'âge, niveau minimum et profil. Une demande peut rester prioritaire malgré le désaccord signalé dans le rapport.

Le staff reçoit des contrats de trois saisons ; leur renouvellement est accessible depuis le poste dans Staff. La responsabilité de renouvellement complet permet de les reconduire à échéance. Le directeur sportif peut recruter un recruteur, un formateur ou un médecin manquant en délégation complète, dans les limites financières.

Le directeur sportif est disponible en carrière de club comme entraîneur / manager. Cette passe ne crée pas un mode de carrière « directeur sportif ». Les modes Championnat simple, sélections, compétitions libres et carrière de joueur conservent leur fonctionnement.

## Sauvegardes et prêts

Format de sauvegarde **30** : profil, compétences, responsabilités, plafonds, demande, cibles, contrats, statuts, échéances du staff, prêts négociés et dossiers en cours sont conservés. Les sauvegardes antérieures restent lisibles ; dans une ancienne carrière sans directeur sportif, recruter ce poste dans Staff. Le rapport textuel est recalculé depuis l'effectif.

Correction de la comptabilité des prêts : l'emprunteur paie sa part et le prêteur paie le reste. Lever une option retire le prêt et engage la totalité du salaire au club acquéreur, sans créer de doublon. Les joueurs retournent normalement à leur club en fin de saison si l'option n'a pas été levée.

## Portée de cette passe

Profils, compétences, connaissances, intérêt, disponibilité et négociations sont des données simulées du jeu ; ils ne représentent pas des dirigeants ou contrats réels. Les mois de contrat sont une estimation à partir de la saison. La destination d'un prêt indique une concurrence raisonnable au moment de l'accord ; elle ne garantit pas les choix futurs de l'entraîneur adverse.

Les contacts sont un facteur des négociations : le jeu ne dispose pas encore d'entités agents ni de commissions d'agence distinctes. La durée des contrats et les primes réutilisent les règles actuelles ; il n'y a pas de clause complexe supplémentaire. Les responsabilités Conseil proposent des informations et les responsabilités Négociation assistent un choix humain ; les ventes et recrutements du staff en dehors de la délégation complète restent décidés dans leurs écrans habituels.

Cette passe centralise les décisions sportives du club du joueur. Les autres clubs gardent leur logique de transferts existante ; une généralisation à tous les clubs et un futur mode directeur sportif restent des chantiers distincts.

Les données France n'ont pas été complétées dans cette livraison : **3 761 équipes seniors officielles importées, 327 groupes** ; la couverture de toutes les ligues et de tous les districts reste partielle. Les fonctions et corrections des builds précédentes sont conservées.

## Compilation et vérification

Depuis le dossier source, avec LLVM-MinGW x86_64 et raylib 5.5 MinGW x64 :

```powershell
.\build-win64.ps1 -Compiler 'C:\chemin\clang++.exe' -Raylib 'C:\chemin\raylib-5.5_win64_mingw-w64' -Tests
```

Le moteur des matchs n'a pas été modifié par cette passe. Les preuves des tests, de la compatibilité des sauvegardes, des scénarios d'affichage et des dépendances Windows figurent dans **verification** du ZIP source. Le document demandé est conservé dans **docs/FranceFoot2D_Directeur_Sportif_Passe1.md**.

Résultats de cette build : **6 698 contrôles réussis**, dont **132 consacrés au directeur sportif**. Le validateur France passe avec 3 761 équipes importées. Les scènes d'affichage sont enregistrées dans verification.

Affichage : **64 scénarios réussis**, avec les dix pages du directeur sportif, les accès Club / Staff et les scènes de régression des fonctions précédentes.
