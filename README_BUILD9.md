# Super Soccer World / France Foot 2D — 00.03.00 build 9

Extraire le ZIP Windows 64 bits et lancer **FranceFoot2D.exe**. Raylib est intégrée à l'exécutable. Les sources et le script de compilation accompagnent la livraison.

## Supporters vivants

En carrière complète, **Gestion du club → Supporters** ouvre huit rubriques : résumé, abonnements, affluence, ambiance, rivalités, confiance, événements et historique. Le directeur sportif dispose aussi d'une rubrique Supporters dans son bureau.

Chaque club possède un public persistant : abonnés, noyau actif, occasionnels, historiques et internationaux. Fidélité, ferveur, patience, exigences et identité locale influencent son évolution. Les équipes techniques de joueurs libres et les pôles de formation ne sont pas considérés comme des clubs de supporters.

La campagne annuelle d'abonnements dépend des prix, du public, des résultats et du stade. Sa capacité est limitée à 70 % des places ; les demandes supplémentaires constituent une liste d'attente. Les recettes sont encaissées une fois par saison pour le club dirigé. Les abonnés présents ne paient pas à nouveau leur billet ordinaire ; buvette, boutique, parking et frais de match réutilisent les finances existantes.

Affluence et déplacements tiennent compte de l'adversaire, de l'enjeu, de la rivalité, de la météo, de l'horaire, des prix et de la confiance. La prévision est fixée au début du match et partagée par le résultat et la billetterie. Les records conservent adversaire, compétition et date ; les événements marquants rejoignent le musée. Les anciens records sans date détaillée restent identifiés comme tels.

Chants, encouragements, célébrations, tension, sifflets et contestation suivent le contexte. Les tifos accompagnent notamment derbys, finales, anniversaires, titres, montées, retours de figures du club, premier rendez-vous européen et premier match dans un stade modifié. Des fêtes de titre et de montée se présentent automatiquement au retour au bureau et peuvent être revues dans les événements.

Les réactions aux ventes tiennent compte de la popularité, de l'âge, du prix, du moral et de la succession. Un prêt ou un passage entre équipes du même club ne provoque pas de réaction de vente définitive. Le retour d'une figure conservée dans le musée peut déclencher un hommage. Les changements de coach, les recrutements et les jeunes influencent la confiance. Un déménagement ou un nouveau nom commercial peut déplaire à un public attaché à son identité.

La contestation progresse par étapes. Un boycott exceptionnel exige une crise prolongée et plusieurs facteurs graves ; il dure deux rencontres à domicile, réduit effectivement la demande et prend fin progressivement. Une ou deux défaites ne suffisent pas. Les influences sportives restent très faibles : stimulation supplémentaire à domicile jusqu'à 0,3 %, variation des attributs des jeunes jusqu'à 0,3 % et variation de l'imprécision sur penalty jusqu'à 2 %. Les tirs ordinaires n'ont pas de nouvelle pénalité de public.

Le mode Simple calcule public, visiteurs, ambiance et chants pendant le match sans gérer de profils, abonnements, crises ou archives persistantes. Les sauvegardes utilisent le format 32 ; les versions précédentes restent lisibles. La migration ne facture pas rétroactivement une campagne.

## Ce qui est simulé et ce qui reste à améliorer

Les distances sont des estimations par commune, district, département, région et pays, sans coordonnées géographiques réelles. Les rivalités sont générées par la proximité et les rencontres du jeu ; elles ne prétendent pas reproduire un inventaire officiel des derbys historiques. Les catégories de supporters et les demandes sont des valeurs de simulation, pas des statistiques de fréquentation officielles.

La présentation utilise des tifos et des foules pixelisés, et des sons synthétiques originaux. Elle pourra recevoir des chants plus variés, de plus grandes animations et des scènes de parade plus détaillées. Les cérémonies actuelles sont des scènes de foule, drapeaux et trophée avec un récit adapté au club et à l'événement. Les traces de tests distinguent scénarios visuels, tests financiers et transitions annuelles ; un essai de dix saisons ne simule intégralement que le championnat sélectionné, les autres compétitions étant clôturées dans ce scénario borné.

Les limites des carrières de directeur sportif restent décrites dans README_BUILD8.md. La saisie officielle France 2026/27 demeure partielle : **3 761 équipes seniors importées, 327 groupes**. Cette build ne remplace pas les groupes encore générés. Les rapports de couverture inclus permettent d'identifier précisément les données déjà vérifiées.

## Recompiler

Voir **build-win64.ps1** : fournir les chemins de LLVM-MinGW et de raylib 5.5 Windows MinGW, puis utiliser `-Tests` pour lancer les vérifications. Le document de demande Supporters est conservé dans `docs/FranceFoot2D_Supporters_Vivants_PasseUnique.md`. Les journaux et captures de la livraison figurent dans `verification/` du ZIP source.

## Validation de cette livraison

33 509 contrôles automatiques réussis, dont 26 699 pour les supporters. Les régressions couvrent les nations, les compétitions et fins de saison, les rôles, les récompenses, le musée et les deux systèmes de directeur sportif. Le scénario Supporters couvre dix transitions annuelles réelles du championnat sélectionné et la relecture de ses archives ; la sauvegarde précédente au format 31 a aussi été relue.

103 scénarios d'écran ont produit leurs captures. Après les dernières corrections d'affichage et de retour au bureau, 17 scénarios ciblés sont relancés ; le programme copié dans le dossier de livraison est contrôlé séparément dans dix situations. Les journaux livrés font foi pour ces vérifications.
