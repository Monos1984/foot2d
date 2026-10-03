// Championnats du monde complémentaires (section 10) : premières divisions de toutes les confédérations
// et deuxièmes divisions reliées par montée / relégation. Clubs réels (saison 2025-26, effectifs générés).
// Format d'un club : "Nom|ABR|note|couleur1|couleur2|stade" (stade facultatif), clubs séparés par ';'.
// Un club déjà présent dans la base (clubs européens / sud-américains) est repris tel quel (même nom).
#include "data.h"
#include <cstring>

const ExtLeagueDef EXT_LEAGUES[] = {
    // ============================================================ UEFA : deuxièmes divisions
    { "POR2", "Liga Portugal 2", "POR", 2, 2,
      "Académico de Viseu|VIS|60|000000|FFFFFF;Benfica B|BEB|58|E2001A|FFFFFF;FC Porto B|PFB|58|0055A4|FFFFFF;Chaves|CHV|60|0055A4|E2001A;Feirense|FEI|58|0055A4|FFFFFF;"
      "Leixões|LEI|57|E2001A|FFFFFF;Lusitânia Lourosa|LOU|55|E2001A|000000;Marítimo|MAR|60|00843D|E2001A;Oliveirense|OLI|56|E2001A|000000;Paços de Ferreira|PFE|59|FFE500|00843D;"
      "Penafiel|PEN|57|E2001A|000000;Portimonense|PTM|59|000000|FFFFFF;Sporting B|SPB|58|00843D|FFFFFF;Torreense|TOR|57|E2001A|FFFFFF;União de Leiria|LEI|58|E2001A|FFFFFF;"
      "Vizela|VIZ|60|0055A4|FFFFFF;Felgueiras|FEL|55|E2001A|0055A4;Farense|FAR|60|000000|FFFFFF" },
    { "NED2", "Eerste Divisie", "NED", 2, 2,
      "ADO Den Haag|ADO|62|FFE500|00843D;Almere City|ALM|61|E2001A|000000;De Graafschap|DEG|60|0055A4|FFFFFF;Den Bosch|DBO|57|E2001A|FFFFFF;FC Dordrecht|DOR|56|E2001A|FFFFFF;"
      "FC Eindhoven|EIN|57|0055A4|FFFFFF;Emmen|EMM|58|E2001A|FFFFFF;Helmond Sport|HEL|56|F47920|000000;Jong Ajax|JAJ|58|FFFFFF|E2001A;Jong AZ|JAZ|57|E2001A|FFFFFF;"
      "Jong PSV|JPS|57|E2001A|FFFFFF;Jong FC Utrecht|JUT|55|E2001A|FFFFFF;MVV Maastricht|MVV|56|E2001A|FFFFFF;RKC Waalwijk|RKC|60|FFE500|0055A4;Roda JC|ROD|60|FFE500|000000;"
      "SC Cambuur|CAM|60|FFE500|0055A4;TOP Oss|OSS|55|E2001A|000000;VVV-Venlo|VVV|57|FFE500|000000;Vitesse|VIT|59|FFE500|000000;Willem II|WIL|61|E2001A|FFFFFF" },
    { "BEL2", "Challenger Pro League", "BEL", 2, 2,
      "Beerschot|BEE|62|5B2C83|FFFFFF;Beveren|BEV|58|FFE500|0055A4;Club NXT|NXT|57|0055A4|000000;Eupen|EUP|59|FFFFFF|000000;Francs Borains|FBO|55|FFFFFF|E2001A;"
      "Jong Genk|JGE|55|0055A4|FFFFFF;Kortrijk|KOR|60|E2001A|FFFFFF;Lierse|LIE|57|FFE500|000000;Lommel SK|LOM|59|000000|FFFFFF;Olympic Charleroi|OLC|54|000000|FFFFFF;"
      "Patro Eisden|PAT|55|000000|FFFFFF;RFC Liège|RFC|56|E2001A|00843D;RSCA Futures|RSC|56|5B2C83|FFFFFF;RWDM|RWD|58|E2001A|FFFFFF;Seraing|SER|56|E2001A|000000;Jong KAA Gent|JGT|54|0055A4|FFFFFF" },
    { "SCO2", "Scottish Championship", "SCO", 2, 1,
      "Airdrieonians|AIR|52|FFFFFF|E2001A;Ayr United|AYR|54|FFFFFF|000000;Arbroath|ARB|50|7B1E2B|FFFFFF;Dunfermline|DUN|55|000000|FFFFFF;Greenock Morton|MOR|53|0055A4|FFFFFF;"
      "Partick Thistle|PAR|55|E2001A|FFE500;Queen's Park|QPK|52|000000|FFFFFF;Raith Rovers|RAI|54|0055A4|FFFFFF;Ross County|ROS|58|0055A4|FFFFFF;St Johnstone|STJ|58|0055A4|FFFFFF" },
    { "TUR2", "TFF 1. Lig", "TUR", 2, 3,
      "Adana Demirspor|ADS|58|0055A4|FFFFFF;Amedspor|AME|56|00843D|E2001A;Bandırmaspor|BAN|57|E2001A|0055A4;Bodrum FK|BOD|60|00843D|FFFFFF;Boluspor|BOL|56|E2001A|FFFFFF;"
      "Çorum FK|COR|58|E2001A|000000;Erokspor|ERO|57|0055A4|F47920;Esenler Erokspor|ESE|55|0055A4|FFFFFF;Hatayspor|HAT|58|7B1E2B|FFFFFF;İstanbulspor|IST|56|FFE500|000000;"
      "Iğdır FK|IGD|55|0055A4|FFFFFF;Keçiörengücü|KEC|55|5B2C83|FFFFFF;Manisa FK|MAN|55|000000|FFFFFF;Pendikspor|PEN|57|E2001A|FFFFFF;Sakaryaspor|SAK|57|00843D|000000;"
      "Sarıyer|SAR|54|0055A4|FFFFFF;Serik Belediyespor|SER|53|E2001A|FFFFFF;Sivasspor|SIV|60|E2001A|FFFFFF;Ümraniyespor|UMR|56|E2001A|FFFFFF;Van Spor|VAN|55|E2001A|FFFFFF" },
    { "SUI2", "Challenge League", "SUI", 2, 1,
      "FC Aarau|AAR|58|000000|FFFFFF;AC Bellinzona|BEL|54|E2001A|FFFFFF;Étoile Carouge|CAR|53|00843D|FFFFFF;FC Stade Lausanne-Ouchy|SLO|55|000000|FFFFFF;Neuchâtel Xamax|XAM|56|E2001A|000000;"
      "FC Rapperswil-Jona|RAP|53|0055A4|FFFFFF;FC Stade Nyonnais|NYO|52|E2001A|FFFFFF;FC Vaduz|VAD|48|E2001A|0055A4;FC Wil|WIL|54|E2001A|FFFFFF;Yverdon-Sport|YVE|57|FFFFFF|00843D" },
    { "AUT2", "2. Liga", "AUT", 2, 1,
      "Admira Wacker|ADM|56|000000|E2001A;FC Liefering|LIE|56|E2001A|FFFFFF;First Vienna|VIE|55|FFE500|0055A4;SKN St. Pölten|STP|56|0055A4|FFFFFF;Austria Lustenau|LUS|57|00843D|FFFFFF;"
      "SV Ried|RIE|57|000000|E2001A;Kapfenberger SV|KAP|52|E2001A|FFFFFF;Floridsdorfer AC|FAC|52|0055A4|FFFFFF;SV Horn|HOR|51|0055A4|FFFFFF;Sturm Graz II|STU|52|000000|FFFFFF;"
      "SW Bregenz|BRE|52|000000|FFFFFF;Austria Salzburg|AUS|52|5B2C83|FFFFFF;Amstetten|AMS|52|E2001A|000000;Leoben|LEO|51|000000|FFFFFF;Rapid Vienne II|RAP|51|00843D|FFFFFF;Stripfing|STR|50|0055A4|FFFFFF" },
    { "DEN2", "1. Division", "DEN", 2, 2,
      "Aarhus Fremad|AAF|52|000000|FFFFFF;AC Horsens|HOR|56|FFE500|000000;B.93|B93|52|FFFFFF|000000;Esbjerg fB|ESB|55|0055A4|FFFFFF;HB Køge|KOG|53|000000|0055A4;"
      "Hillerød|HIL|51|0055A4|FFFFFF;Hobro IK|HOB|54|FFE500|0055A4;Kolding IF|KOL|53|FFFFFF|0055A4;Lyngby BK|LYN|56|0055A4|FFFFFF;Middelfart|MID|50|E2001A|FFFFFF;AaB Aalborg|AAB|57|E2001A|FFFFFF;Hvidovre IF|HVI|52|E2001A|0055A4" },
    { "SWE2", "Superettan", "SWE", 2, 2,
      "Helsingborgs IF|HIF|55|E2001A|0055A4;Landskrona BoIS|LAN|52|FFFFFF|000000;Örgryte IS|ORG|53|E2001A|0055A4;Östers IF|OST|52|E2001A|FFFFFF;IK Brage|BRA|51|00843D|FFFFFF;"
      "Varbergs BoIS|VAR|51|00843D|FFFFFF;Sandvikens IF|SAN|50|E2001A|FFFFFF;Trelleborgs FF|TRE|50|0055A4|FFFFFF;Utsiktens BK|UTS|49|0055A4|FFFFFF;Falkenbergs FF|FAL|50|FFE500|000000;"
      "GIF Sundsvall|GIF|51|0055A4|FFFFFF;Kalmar FF|KAL|53|E2001A|FFFFFF;Oddevold|ODD|49|0055A4|FFFFFF;Östersunds FK|OFK|51|E2001A|000000;Umeå FC|UME|48|FFE500|000000;Västerås SK|VAS|52|00843D|000000" },
    { "NOR2", "OBOS-ligaen", "NOR", 2, 2,
      "Aalesund|AAL|54|F47920|0055A4;Egersund|EGE|49|000000|FFFFFF;Hødd|HOD|49|0055A4|FFFFFF;Kongsvinger|KON|51|E2001A|FFFFFF;Lillestrøm|LIL|56|FFE500|000000;"
      "Lyn|LYN|51|E2001A|000000;Mjøndalen|MJO|50|F47920|000000;Moss|MOS|50|FFE500|0055A4;Odd|ODD|53|FFFFFF|000000;Ranheim|RAN|50|FFFFFF|E2001A;Raufoss|RAU|49|FFE500|0055A4;"
      "Sogndal|SOG|51|E2001A|000000;Stabæk|STA|53|0055A4|FFFFFF;Start|STA|52|FFE500|000000;Åsane|ASA|49|0055A4|FFFFFF;Skeid|SKE|48|E2001A|0055A4" },
    { "POL2", "I liga", "POL", 2, 3,
      "Chrobry Głogów|CHR|50|F47920|000000;Górnik Łęczna|GLE|51|00843D|000000;Miedź Legnica|MIE|53|00843D|0055A4;Odra Opole|ODR|51|0055A4|E2001A;Polonia Warszawa|POL|52|000000|FFFFFF;"
      "Pogoń Grodzisk|POG|50|0055A4|FFFFFF;Puszcza Niepołomice|PUS|51|00843D|000000;Ruch Chorzów|RUC|53|0055A4|FFFFFF;Śląsk Wrocław|SLA|56|00843D|E2001A;Stal Mielec|STA|53|0055A4|FFFFFF;"
      "Stal Rzeszów|SRZ|50|0055A4|FFFFFF;Wisła Kraków|WIS|56|E2001A|FFFFFF;Wisła Płock|WPL|53|0055A4|FFFFFF;Znicz Pruszków|ZNI|48|E2001A|FFFFFF;GKS Tychy|TYC|50|E2001A|000000;Kotwica Kołobrzeg|KOT|47|E2001A|FFFFFF;Warta Poznań|WAR|51|00843D|FFFFFF;ŁKS Łódź|LKS|52|E2001A|FFFFFF" },
    { "GRE2", "Super League 2", "GRE", 2, 2,
      "Iraklis Thessalonique|IRA|50|0055A4|FFFFFF;Kalamata|KAL|48|000000|FFFFFF;Panachaiki|PAN|48|000000|E2001A;PAS Giannina|PAS|52|0055A4|FFFFFF;Chania|CHA|47|000000|FFFFFF;Niki Volos|NIK|48|0055A4|FFFFFF;"
      "Egaleo|EGA|46|0055A4|FFFFFF;Athens Kallithea|KAL|48|0055A4|FFFFFF;Olympiakos B|OLB|48|E2001A|FFFFFF;Panathinaïkos B|PAB|47|00843D|FFFFFF;AEK B|AEB|47|FFE500|000000;PAOK B|PAB|47|000000|FFFFFF" },
    // ============================================================ UEFA : premières divisions complémentaires
    { "GRE1", "Super League (Grèce)", "GRE", 1, 2,
      "Olympiakos|OLY|77|E2001A|FFFFFF|Stade Georgios-Karaïskakis;PAOK Salonique|PAOK|73|000000|FFFFFF|Toumba;AEK Athènes|AEK|74|FFE500|000000|OPAP Arena;Panathinaïkos|PAO|74|00843D|FFFFFF|Stade Apostolos-Nikolaïdis;"
      "Aris Salonique|ARI|62|FFE500|000000|Kleanthis-Vikelidis;Asteras Tripolis|AST|55|FFE500|000000;Atromitos|ATR|55|0055A4|FFFFFF;OFI Crète|OFI|56|000000|FFFFFF;Panetolikos|PNT|53|FFE500|0055A4;"
      "Panserraikos|PSE|52|E2001A|FFFFFF;Levadiakos|LEV|54|00843D|FFFFFF;Volos NFC|VOL|53|E2001A|0055A4;AEL Larissa|AEL|53|7B1E2B|FFFFFF;Kifisia|KIF|52|FFFFFF|0055A4" },
    { "CRO1", "SuperSport HNL", "CRO", 1, 1,
      "Dinamo Zagreb|DZG|69|0055A4|FFFFFF|Stade Maksimir;Hajduk Split|HAJ|67|FFFFFF|0055A4|Stade Poljud;HNK Rijeka|RIJ|61|FFFFFF|3FA9F5|Stade de Rujevica;NK Osijek|OSI|58|0055A4|FFFFFF|Opus Arena;"
      "NK Varaždin|VAR|54|0055A4|FFFFFF;HNK Gorica|GOR|54|000000|FFFFFF;Slaven Belupo|SLA|53|0055A4|FFFFFF;NK Istra 1961|IST|53|FFE500|00843D;HNK Vukovar 1991|VUK|50|0055A4|FFFFFF;NK Lokomotiva|LOK|53|0055A4|FFFFFF" },
    { "SRB1", "Superliga (Serbie)", "SRB", 1, 2,
      "Étoile rouge de Belgrade|CZV|68|E2001A|FFFFFF|Stade Rajko-Mitić;Partizan Belgrade|PAR|63|000000|FFFFFF|Stade du Partizan;TSC Bačka Topola|TSC|58|0055A4|FFFFFF;Vojvodina Novi Sad|VOJ|57|E2001A|FFFFFF;"
      "FK Čukarički|CUK|55|000000|FFFFFF;FK Radnički Niš|RAD|53|E2001A|0055A4;FK Novi Pazar|NPA|52|E2001A|FFFFFF;FK Spartak Subotica|SPA|52|0055A4|FFFFFF;FK Mladost Lučani|MLA|51|E2001A|FFFFFF;"
      "OFK Beograd|OFK|51|0055A4|FFFFFF;FK Železničar Pančevo|ZEL|50|0055A4|FFFFFF;FK IMT Belgrade|IMT|50|FFFFFF|0055A4;FK Napredak Kruševac|NAP|50|E2001A|FFFFFF;FK Radnik Surdulica|RSU|49|E2001A|FFFFFF;FK Javor Ivanjica|JAV|49|0055A4|FFFFFF;FK Radnički 1923|R23|50|E2001A|FFFFFF" },
    { "CZE1", "Chance Liga", "CZE", 1, 2,
      "Slavia Prague|SLA|74|E2001A|FFFFFF|Fortuna Arena;Sparta Prague|SPA|72|7B1E2B|FFFFFF|Stade de Letná;Viktoria Plzeň|PLZ|70|E2001A|0055A4|Doosan Arena;Baník Ostrava|BAN|60|3FA9F5|FFFFFF|Městský stadion;"
      "Slovan Liberec|LIB|57|FFFFFF|0055A4;Sigma Olomouc|OLO|57|0055A4|FFFFFF;FK Jablonec|JAB|57|00843D|FFFFFF;Mladá Boleslav|MBO|56|0055A4|FFFFFF;Hradec Králové|HRA|56|000000|FFFFFF;"
      "Bohemians 1905|BOH|54|00843D|FFFFFF;Slovácko|SLO|55|E2001A|0055A4;FK Teplice|TEP|53|FFE500|0055A4;FK Pardubice|PAR|53|E2001A|FFFFFF;Karviná|KAR|53|00843D|FFFFFF;Dukla Prague|DUK|52|FFE500|7B1E2B;Zlín|ZLI|53|FFE500|0055A4" },
    { "UKR1", "Premier Liha", "UKR", 1, 2,
      "Chakhtar Donetsk|SHA|70|F47920|000000|Arena Lviv;Dynamo Kiev|DYN|70|FFFFFF|0055A4|Stade Lobanovsky;Dnipro-1|DNI|58|0055A4|FFFFFF;Zorya Louhansk|ZOR|57|E2001A|FFFFFF;Polissya Jytomyr|POL|60|FFE500|00843D;"
      "Kryvbas Kryvyi Rih|KRY|58|E2001A|FFFFFF;Oleksandriya|OLE|56|FFE500|000000;Karpaty Lviv|KAR|55|00843D|FFFFFF;Rukh Lviv|RUK|54|FFE500|000000;LNZ Tcherkassy|LNZ|55|0055A4|FFFFFF;"
      "Kolos Kovalivka|KOL|54|FFE500|0055A4;Obolon Kiev|OBO|52|0055A4|FFFFFF;Veres Rivne|VER|52|00843D|FFFFFF;Metalist 1925|MET|54|FFE500|0055A4;Epitsentr|EPI|50|E2001A|FFFFFF;Kudrivka|KUD|49|00843D|FFFFFF" },
    { "HUN1", "NB I", "HUN", 1, 2,
      "Ferencváros|FTC|69|00843D|FFFFFF|Groupama Aréna;Puskás Akadémia|PAK|57|FFE500|0055A4;Paksi FC|PAK|55|00843D|FFFFFF;MOL Fehérvár|FEH|55|E2001A|0055A4;Debreceni VSC|DVS|54|E2001A|FFFFFF;"
      "Győri ETO|ETO|56|00843D|FFFFFF;Újpest FC|UTE|54|5B2C83|FFFFFF;ZTE Zalaegerszeg|ZTE|53|0055A4|E2001A;Diósgyőri VTK|DVT|53|E2001A|FFFFFF;Kisvárda|KIS|51|FFFFFF|0055A4;Nyíregyháza|NYI|51|0055A4|FFFFFF;Kazincbarcika|KAZ|49|0055A4|FFE500" },
    { "ISR1", "Ligat HaAl", "ISR", 1, 2,
      "Maccabi Tel-Aviv|MTA|63|FFE500|0055A4|Stade Bloomfield;Maccabi Haïfa|MHA|62|00843D|FFFFFF|Sammy Ofer;Hapoël Beer-Sheva|HBS|60|E2001A|FFFFFF|Turner Stadium;Hapoël Tel-Aviv|HTA|56|E2001A|FFFFFF;"
      "Beitar Jérusalem|BEI|58|FFE500|000000|Teddy Stadium;Hapoël Haïfa|HHA|53|E2001A|FFFFFF;Maccabi Netanya|MNE|53|FFE500|000000;Bnei Sakhnin|SAK|52|E2001A|FFFFFF;Hapoël Jérusalem|HJE|52|E2001A|000000;"
      "Ironi Kiryat Shmona|IKS|51|0055A4|FFFFFF;Maccabi Bnei Reineh|REI|52|0055A4|FFFFFF;Hapoël Petah-Tikva|HPT|51|0055A4|FFFFFF;Ashdod SC|ASH|51|E2001A|FFE500;Ironi Tiberias|TIB|50|0055A4|FFFFFF" },
    { "CYP1", "Protathlima Cyta", "CYP", 1, 3,
      "APOEL Nicosie|APO|65|FFE500|0055A4|GSP Stadium;Pafos FC|PAF|62|0055A4|FFFFFF;Omonia Nicosie|OMO|60|00843D|FFFFFF;AEK Larnaca|AEK|59|FFE500|00843D;Aris Limassol|ARL|57|00843D|FFFFFF;"
      "Anorthosis Famagouste|ANO|54|0055A4|FFFFFF;Apollon Limassol|APL|55|0055A4|FFFFFF;AEL Limassol|AEL|53|FFE500|0055A4;Ethnikos Achna|ETH|49|E2001A|FFFFFF;Omonia Aradippou|OAR|47|00843D|FFFFFF;"
      "Enosis Paralimni|ENP|48|E2001A|0055A4;Olympiakos Nicosie|OLN|48|000000|00843D;Akritas Chlorakas|AKR|46|E2001A|FFFFFF;Krasava|KRA|47|E2001A|000000" },
    { "BUL1", "Parva Liga", "BUL", 1, 2,
      "Ludogorets|LUD|63|00843D|FFFFFF|Huvepharma Arena;CSKA Sofia|CSK|56|E2001A|FFFFFF|Stade Balgarska Armia;Levski Sofia|LEV|55|0055A4|FFFFFF|Stade Georgi-Asparuhov;Lokomotiv Plovdiv|LPL|50|000000|E2001A;"
      "Cherno More Varna|CHM|52|00843D|FFFFFF;CSKA 1948|C48|52|E2001A|FFFFFF;Arda Kardzhali|ARD|51|0055A4|FFFFFF;Botev Plovdiv|BOT|51|FFE500|000000;Slavia Sofia|SLA|49|FFFFFF|000000;"
      "Beroe Stara Zagora|BER|49|00843D|FFFFFF;Lokomotiv Sofia|LSO|49|E2001A|000000;Spartak Varna|SPV|48|0055A4|FFFFFF;Botev Vratsa|BVR|48|E2001A|FFFFFF;Septemvri Sofia|SEP|47|E2001A|FFFFFF;Dobrudzha|DOB|47|FFE500|00843D;Montana|MON|47|0055A4|FFFFFF" },
    { "SVK1", "Niké liga", "SVK", 1, 1,
      "Slovan Bratislava|SLO|60|3FA9F5|FFFFFF|Tehelné pole;DAC Dunajská Streda|DAC|55|FFE500|0055A4;Spartak Trnava|TRN|53|E2001A|000000;MŠK Žilina|ZIL|52|FFE500|00843D;FC Košice|KOS|50|FFE500|0055A4;"
      "Podbrezová|POD|50|E2001A|000000;Ružomberok|RUZ|50|FFFFFF|0055A4;MFK Skalica|SKA|47|0055A4|FFFFFF;Trenčín|TRE|49|FFFFFF|E2001A;Michalovce|MIC|48|0055A4|FFE500;KFC Komárno|KOM|47|0055A4|FFFFFF;Tatran Prešov|TAT|48|E2001A|000000" },
    { "SVN1", "PrvaLiga", "SVN", 1, 1,
      "NK Celje|CEL|57|FFE500|0055A4;Olimpija Ljubljana|OLI|57|00843D|FFFFFF;NK Maribor|MAR|55|5B2C83|FFFFFF;NK Koper|KOP|50|FFE500|0055A4;NK Bravo|BRA|48|FFE500|000000;"
      "NK Mura|MUR|49|000000|FFFFFF;NK Radomlje|RAD|46|FFE500|0055A4;NK Primorje|PRI|46|0055A4|FFFFFF;NK Domžale|DOM|46|FFE500|0055A4;NK Aluminij|ALU|45|FFFFFF|0055A4" },
    { "RUS1", "Premier Liga (Russie)", "RUS", 1, 2,
      "Zénith Saint-Pétersbourg|ZEN|72|0055A4|FFFFFF|Gazprom Arena;Spartak Moscou|SPA|68|E2001A|FFFFFF|Loukoïl Arena;CSKA Moscou|CSK|67|E2001A|0055A4|VEB Arena;Lokomotiv Moscou|LOK|66|E2001A|00843D;"
      "Dynamo Moscou|DYN|66|0055A4|FFFFFF;FK Krasnodar|KRA|69|00843D|000000;Rubin Kazan|RUB|60|7B1E2B|00843D;Rostov|ROS|59|FFE500|0055A4;Akhmat Grozny|AKH|57|00843D|FFFFFF;"
      "Krylia Sovetov|KRY|57|0055A4|FFFFFF;FK Orenbourg|ORE|55|5B2C83|FFFFFF;Pari NN|PNN|55|0055A4|000000;Dynamo Makhatchkala|DMK|55|0055A4|FFFFFF;Akron Togliatti|AKR|54|E2001A|000000;Baltika Kaliningrad|BAL|55|0055A4|FFFFFF;Sotchi|SOC|55|0055A4|FFFFFF" },
    { "FIN1", "Veikkausliiga", "FIN", 1, 1,
      "HJK Helsinki|HJK|58|0055A4|FFFFFF;KuPS Kuopio|KUP|52|FFE500|000000;SJK Seinäjoki|SJK|50|000000|FFFFFF;FC Inter Turku|INT|49|000000|FFFFFF;Ilves Tampere|ILV|49|FFE500|00843D;"
      "FC Lahti|LAH|46|FFFFFF|000000;VPS Vaasa|VPS|47|000000|E2001A;IFK Mariehamn|IFK|46|00843D|FFFFFF;AC Oulu|OUL|47|FFFFFF|000000;Gnistan|GNI|45|000000|FFFFFF;FF Jaro|JAR|44|0055A4|FFFFFF;KTP Kotka|KTP|44|00843D|FFFFFF" },
    { "ALB1", "Kategoria Superiore", "ALB", 1, 2,
      "KF Egnatia|EGN|48|E2001A|000000;KF Partizani|PAR|47|E2001A|FFFFFF;KF Tirana|TIR|47|FFFFFF|0055A4;FK Vllaznia|VLL|46|E2001A|0055A4;FK Kukësi|KUK|44|F47920|000000;"
      "KF Teuta|TEU|44|5B2C83|FFFFFF;KF Dinamo City|DIN|45|0055A4|FFFFFF;KF Elbasani|ELB|43|E2001A|000000;KS Bylis|BYL|43|0055A4|FFFFFF;AF Vora|VOR|42|0055A4|FFFFFF" },
    { "AND1", "Primera Divisió", "AND", 1, 1,
      "UE Santa Coloma|USC|32|00843D|FFFFFF;Inter Club d'Escaldes|INT|33|0055A4|FFFFFF;FC Santa Coloma|FSC|31|E2001A|FFFFFF;Atlètic Club d'Escaldes|ATE|30|FFE500|000000;"
      "UE Engordany|ENG|29|0055A4|FFFFFF;FC Pas de la Casa|PAS|28|0055A4|FFE500;CE Carroi|CAR|28|E2001A|000000;FC Ordino|ORD|28|000000|FFFFFF" },
    { "ARM1", "Premier League (Arménie)", "ARM", 1, 1,
      "FC Noah|NOA|54|FFFFFF|000000;FC Pyunik|PYU|50|0055A4|FFFFFF;FC Ararat-Armenia|ARA|50|FFE500|0055A4;FC Urartu|URA|48|FFE500|000000;FC Ararat Erevan|ARE|45|E2001A|FFFFFF;"
      "Alashkert|ALA|46|FFE500|0055A4;Shirak|SHI|43|000000|F47920;BKMA Erevan|BKM|43|E2001A|FFFFFF;FC Van|VAN|42|0055A4|FFFFFF;Gandzasar|GAN|42|FFFFFF|E2001A" },
    { "AZE1", "Premyer Liqa", "AZE", 1, 1,
      "Qarabağ|QAR|65|000000|FFFFFF|Stade Tofiq-Bəhramov;Neftçi Bakou|NEF|55|000000|FFFFFF;Sabah FK|SAB|55|F47920|000000;Zirə FK|ZIR|52|FFE500|0055A4;Sumqayıt FK|SUM|48|0055A4|FFFFFF;"
      "Turan Tovuz|TUR|48|E2001A|FFFFFF;Araz-Naxçıvan|ARZ|48|0055A4|FFFFFF;Kapaz|KAP|46|0055A4|FFFFFF;Şamaxı FK|SAM|45|0055A4|FFFFFF;İmişli|IMI|44|E2001A|FFFFFF" },
    { "BIH1", "Premijer Liga BiH", "BIH", 1, 2,
      "Zrinjski Mostar|ZRI|54|E2001A|FFFFFF;FK Borac Banja Luka|BOR|53|E2001A|0055A4;FK Sarajevo|SAR|50|7B1E2B|FFFFFF;Željezničar Sarajevo|ZEL|49|0055A4|FFFFFF;Velež Mostar|VEL|48|E2001A|FFFFFF;"
      "Široki Brijeg|SIR|47|0055A4|FFFFFF;Sloga Doboj|SLO|45|0055A4|FFFFFF;FK Posušje|POS|45|0055A4|FFFFFF;Radnik Bijeljina|RAD|44|0055A4|FFFFFF;GOŠK Gabela|GOS|43|0055A4|FFFFFF" },
    { "BLR1", "Vysheyshaya Liga", "BLR", 1, 2,
      "Dinamo Minsk|DMI|52|0055A4|FFFFFF;BATE Borisov|BAT|51|FFE500|0055A4;Shakhtyor Soligorsk|SHS|50|F47920|000000;Neman Grodno|NEM|48|00843D|FFFFFF;Torpedo-BelAZ Zhodino|TOR|48|000000|00843D;"
      "Isloch|ISL|46|0055A4|FFFFFF;Dinamo Brest|DBR|46|0055A4|FFFFFF;FK Minsk|MIN|45|E2001A|0055A4;Slavia Mozyr|SLA|45|E2001A|FFFFFF;FK Gomel|GOM|45|00843D|FFFFFF;ML Vitebsk|VIT|44|E2001A|FFFFFF;Arsenal Dzerzhinsk|ARS|43|E2001A|FFFFFF" },
    { "EST1", "Meistriliiga", "EST", 1, 1,
      "FCI Levadia|LEV|47|00843D|FFFFFF;FC Flora Tallinn|FLO|47|00843D|FFFFFF;Paide Linnameeskond|PAI|44|E2001A|FFFFFF;Nõmme Kalju|KAL|43|FF69B4|000000;Tartu Tammeka|TAM|40|0055A4|FFFFFF;"
      "JK Narva Trans|NAR|40|0055A4|FFFFFF;FC Kuressaare|KUR|39|FFE500|0055A4;Harju JK|HAR|39|0055A4|FFFFFF;Nõmme United|NUN|39|0055A4|FFFFFF;Tallinna Kalev|TKA|38|E2001A|FFFFFF" },
    { "FRO1", "Betri deildin", "FRO", 1, 1,
      "KÍ Klaksvík|KIK|47|3FA9F5|FFFFFF;Víkingur Gøta|VIK|43|E2001A|000000;HB Tórshavn|HB|44|E2001A|000000;B36 Tórshavn|B36|42|FFFFFF|000000;NSÍ Runavík|NSI|41|FFE500|0055A4;"
      "EB/Streymur|EBS|39|E2001A|FFFFFF;ÍF Fuglafjørður|IFF|38|E2001A|0055A4;TB Tvøroyri|TB|37|000000|FFFFFF;B68 Toftir|B68|37|E2001A|FFFFFF;07 Vestur|07V|37|0055A4|FFFFFF" },
    { "GEO1", "Erovnuli Liga", "GEO", 1, 1,
      "Dinamo Tbilissi|DTB|52|FFFFFF|0055A4;Dinamo Batoumi|DBA|50|0055A4|FFFFFF;Torpedo Koutaïssi|TOR|49|000000|FFFFFF;Iberia 1999|IBE|48|E2001A|FFFFFF;Dila Gori|DIL|46|0055A4|FFFFFF;"
      "FC Samgurali|SAM|44|00843D|FFFFFF;Kolkheti Poti|KOL|43|0055A4|FFFFFF;FC Gagra|GAG|44|0055A4|FFFFFF;Spaeri|SPA|42|E2001A|FFFFFF;Meshakhte Tkibuli|MES|41|FFE500|000000" },
    { "GIB1", "Gibraltar Football League", "GIB", 1, 0,
      "Lincoln Red Imps|LRI|44|E2001A|FFFFFF;St Joseph's FC|STJ|40|0055A4|FFFFFF;Europa FC|EUR|40|00843D|FFFFFF;Bruno's Magpies|BRU|38|000000|FFFFFF;Manchester 62|M62|35|E2001A|FFFFFF;"
      "Lions Gibraltar|LIO|34|E2001A|FFE500;Glacis United|GLA|34|0055A4|FFFFFF;FC College 1975|COL|33|000000|FFFFFF;Mons Calpe|MCA|33|5B2C83|FFFFFF;Europa Point|EPO|32|0055A4|FFFFFF" },
    { "ISL1", "Besta deild", "ISL", 1, 2,
      "Víkingur Reykjavík|VIK|50|E2001A|000000;Breiðablik|BRE|50|00843D|FFFFFF;Valur|VAL|48|E2001A|FFFFFF;KR Reykjavík|KR|46|FFFFFF|000000;Stjarnan|STJ|46|0055A4|FFFFFF;"
      "FH Hafnarfjörður|FH|46|000000|FFFFFF;ÍA Akranes|IA|44|FFE500|000000;KA Akureyri|KA|44|FFE500|0055A4;Fram|FRA|43|0055A4|FFFFFF;ÍBV|IBV|42|FFFFFF|0055A4;Vestri|VES|42|0055A4|FFFFFF;Afturelding|AFT|41|E2001A|000000" },
    { "KAZ1", "Premier Liga (Kazakhstan)", "KAZ", 1, 2,
      "FC Astana|AST|55|FFE500|3FA9F5;Kairat Almaty|KAI|56|FFE500|000000;Tobol Kostanay|TOB|52|00843D|FFFFFF;Ordabasy|ORD|50|0055A4|FFFFFF;Aktobe|AKT|50|E2001A|FFFFFF;"
      "Elimai Semey|ELI|47|0055A4|FFFFFF;Yelimay|YEL|45|0055A4|FFFFFF;Zhenis Astana|ZHE|45|0055A4|FFE500;Atyrau|ATY|45|FFE500|000000;Kyzylzhar|KYZ|45|000000|FFFFFF;Okzhetpes|OKZ|44|00843D|FFFFFF;Kaisar|KAS|45|E2001A|FFFFFF;Zhetysu|ZHT|43|FFE500|000000;Ulytau|ULY|42|0055A4|FFFFFF" },
    { "KVX1", "Superliga e Kosovës", "KVX", 1, 2,
      "KF Ballkani|BAL|50|F47920|000000;KF Drita|DRI|49|0055A4|FFFFFF;FC Prishtina|PRI|48|0055A4|FFFFFF;KF Llapi|LLA|45|E2001A|FFFFFF;KF Malisheva|MAL|43|0055A4|FFFFFF;"
      "KF Dukagjini|DUK|43|E2001A|FFFFFF;KF Gjilani|GJI|42|0055A4|FFFFFF;KF Ferizaj|FER|42|E2001A|FFFFFF;KF Prizreni|PRZ|41|000000|FFFFFF;KF Drenica|DRE|41|FFE500|000000" },
    { "LVA1", "Virslīga", "LVA", 1, 1,
      "RFS Riga|RFS|52|0055A4|FFFFFF;Riga FC|RIG|50|000000|FFFFFF;FK Auda|AUD|46|E2001A|FFFFFF;Valmiera FC|VAL|45|0055A4|FFFFFF;FK Liepāja|LIE|44|E2001A|FFFFFF;"
      "BFC Daugavpils|DAU|42|E2001A|FFFFFF;FK Jelgava|JEL|41|0055A4|FFFFFF;Grobiņa|GRO|40|00843D|FFFFFF;Tukums 2000|TUK|40|0055A4|FFFFFF;Metta|MET|39|0055A4|FFFFFF" },
    { "LTU1", "A Lyga", "LTU", 1, 1,
      "Žalgiris Vilnius|ZAL|50|00843D|FFFFFF;FK Panevėžys|PAN|47|FFE500|000000;Hegelmann Kaunas|HEG|45|FFFFFF|E2001A;Sūduva Marijampolė|SUD|45|E2001A|FFFFFF;FA Šiauliai|SIA|43|E2001A|FFFFFF;"
      "Kauno Žalgiris|KZA|44|00843D|FFFFFF;Banga Gargždai|BAN|42|0055A4|FFFFFF;Dainava Alytus|DAI|41|E2001A|FFFFFF;Riteriai|RIT|40|0055A4|FFFFFF;Transinvest|TRA|40|0055A4|FFFFFF" },
    { "LUX1", "BGL Ligue", "LUX", 1, 2,
      "F91 Dudelange|F91|46|E2001A|FFFFFF;Swift Hesperange|SWI|44|3FA9F5|FFFFFF;Differdange 03|DIF|45|E2001A|000000;Progrès Niederkorn|PRO|43|000000|FFFFFF;Racing Luxembourg|RFC|42|0055A4|FFFFFF;"
      "Union Titus Pétange|PET|41|0055A4|FFFFFF;UNA Strassen|STR|41|0055A4|FFFFFF;Jeunesse Esch|JEU|41|000000|FFFFFF;FC Victoria Rosport|ROS|39|0055A4|FFFFFF;US Mondorf|MON|39|E2001A|FFFFFF;"
      "Fola Esch|FOL|40|E2001A|FFFFFF;Marisca Mersch|MER|38|00843D|FFFFFF;Rodange 91|ROD|38|FFE500|000000;Mamer 32|MAM|37|E2001A|FFFFFF;Hostert|HOS|37|0055A4|FFFFFF;Wiltz 71|WIL|37|E2001A|FFFFFF" },
    { "MLT1", "Premier League (Malte)", "MLT", 1, 2,
      "Hamrun Spartans|HAM|45|E2001A|000000;Floriana FC|FLO|42|00843D|FFFFFF;Valletta FC|VAL|41|FFFFFF|E2001A;Birkirkara FC|BIR|41|E2001A|FFE500;Hibernians|HIB|42|000000|FFFFFF;"
      "Marsaxlokk|MAX|40|E2001A|FFFFFF;Sliema Wanderers|SLI|39|3FA9F5|FFFFFF;Gzira United|GZI|39|E2001A|FFFFFF;Balzan|BAL|39|E2001A|FFFFFF;Mosta|MOS|38|E2001A|FFFFFF;Naxxar Lions|NAX|37|FFE500|000000;Zabbar St. Patrick|ZAB|37|00843D|FFFFFF" },
    { "MDA1", "Super Liga (Moldavie)", "MDA", 1, 1,
      "Sheriff Tiraspol|SHE|55|FFE500|000000;Petrocub Hîncești|PET|47|00843D|FFFFFF;Zimbru Chișinău|ZIM|45|FFE500|00843D;Milsami Orhei|MIL|44|E2001A|FFFFFF;Dacia Buiucani|DAC|42|FFE500|0055A4;"
      "Spartanii Selemet|SPA|40|E2001A|FFFFFF;Bălți|BAL|41|0055A4|FFFFFF;Real Succes|RSU|40|0055A4|FFFFFF" },
    { "MKD1", "Prva Liga (Macédoine)", "MKD", 1, 2,
      "Shkëndija|SHK|50|E2001A|000000;Struga|STR|46|FFFFFF|0055A4;FK Vardar|VAR|46|E2001A|000000;Shkupi|SHU|45|000000|FFFFFF;FK Rabotnički|RAB|44|E2001A|FFFFFF;"
      "Sileks|SIL|43|0055A4|FFFFFF;Tikvesh|TIK|42|E2001A|FFFFFF;Bregalnica Štip|BRE|42|E2001A|FFFFFF;Pelister|PEL|42|00843D|FFFFFF;Voska Sport|VOS|41|FFE500|000000;Arsimi|ARS|41|0055A4|FFFFFF;Makedonija GP|MAK|41|E2001A|FFFFFF" },
    { "MNE1", "Prva CFL", "MNE", 1, 1,
      "FK Budućnost|BUD|48|0055A4|FFFFFF;FK Dečić|DEC|44|00843D|FFFFFF;FK Sutjeska|SUT|44|0055A4|FFFFFF;FK Mornar Bar|MOR|42|0055A4|FFFFFF;FK Petrovac|PET|41|0055A4|FFFFFF;"
      "FK Jezero|JEZ|40|0055A4|FFFFFF;OFK Petrovac|OPE|40|0055A4|FFFFFF;FK Arsenal Tivat|ARS|40|E2001A|FFFFFF;FK Mladost Lješkopolje|MLA|39|00843D|FFFFFF;FK Bokelj|BOK|39|0055A4|FFFFFF" },
    { "NIR1", "NIFL Premiership", "NIR", 1, 1,
      "Linfield|LIN|48|0055A4|FFFFFF;Larne FC|LAR|47|E2001A|FFFFFF;Glentoran|GLE|46|00843D|E2001A;Cliftonville|CLI|45|E2001A|FFFFFF;Coleraine|COL|44|0055A4|FFFFFF;Crusaders|CRU|44|E2001A|000000;"
      "Dungannon Swifts|DUN|41|FFFFFF|0055A4;Glenavon|GLV|42|0055A4|FFFFFF;Portadown|POR|41|E2001A|FFFFFF;Carrick Rangers|CAR|40|FFE500|000000;Ballymena United|BAL|40|0055A4|FFFFFF;Bangor|BAN|39|FFE500|0055A4" },
    { "SMR1", "Campionato Sammarinese", "SMR", 1, 0,
      "Virtus|VIR|30|FFE500|000000;SS Tre Penne|TRE|30|0055A4|FFFFFF;La Fiorita|FIO|29|5B2C83|FFFFFF;SP Tre Fiori|TFI|28|FFE500|0055A4;SS Folgore|FOL|28|000000|FFFFFF;"
      "SP Cosmos|COS|27|0055A4|FFFFFF;Murata|MUR|27|000000|FFFFFF;Fiorentino|FIR|26|5B2C83|FFFFFF;Domagnano|DOM|26|E2001A|000000;Cailungo|CAI|25|FFE500|E2001A" },
    { "WAL1", "Cymru Premier", "WAL", 1, 2,
      "The New Saints|TNS|48|00843D|FFFFFF;Connah's Quay Nomads|CQN|43|E2001A|FFFFFF;Penybont FC|PEN|42|E2001A|000000;Caernarfon Town|CAE|41|FFE500|00843D;Bala Town|BAL|41|E2001A|000000;"
      "Barry Town United|BAR|40|0055A4|FFFFFF;Cardiff Metropolitan|CMU|39|000000|FFFFFF;Haverfordwest County|HAV|40|0055A4|FFFFFF;Newtown AFC|NEW|39|E2001A|FFFFFF;Flint Town United|FLI|38|000000|FFFFFF;Colwyn Bay|COL|38|F47920|000000;Briton Ferry|BRI|37|FFE500|000000" },
    // ============================================================ CONMEBOL
    { "BRA2", "Brasileirão Série B", "BRA", 2, 4,
      "Athletic Club (MG)|ATH|57|000000|FFFFFF;Athletico Paranaense|CAP|64|E2001A|000000;Atlético Goianiense|ACG|59|E2001A|000000;Avaí|AVA|57|0055A4|FFFFFF;Botafogo-SP|BSP|55|E2001A|FFFFFF;"
      "Chapecoense|CHA|58|00843D|FFFFFF;Coritiba|CFC|61|00843D|FFFFFF;CRB|CRB|56|E2001A|FFFFFF;Criciúma|CRI|58|FFE500|000000;Cuiabá|CUI|59|FFE500|00843D;"
      "Ferroviária|FER|55|7B1E2B|FFFFFF;Goiás|GOI|59|00843D|FFFFFF;Novorizontino|NOV|58|FFE500|000000;Operário Ferroviário|OPE|55|000000|FFFFFF;Paysandu|PAY|56|3FA9F5|FFFFFF;"
      "Remo|REM|57|0055A4|FFFFFF;Amazonas FC|AMA|54|FFE500|000000;Vila Nova|VIL|56|E2001A|FFFFFF;Volta Redonda|VRE|54|FFE500|000000;América Mineiro|AME|60|00843D|000000" },
    { "COL1", "Liga BetPlay", "COL", 1, 2,
      "Atlético Nacional|ATL|70|00843D|FFFFFF|Atanasio Girardot;América de Cali|AMR|67|E2001A|FFFFFF|Pascual Guerrero;Millonarios|MIL|66|0055A4|FFFFFF|El Campín;Junior Barranquilla|JUN|65|E2001A|FFFFFF|Metropolitano;"
      "Independiente Santa Fe|SFE|64|E2001A|FFFFFF;Deportivo Cali|CAL|62|00843D|FFFFFF;Independiente Medellín|DIM|63|E2001A|0055A4;Once Caldas|ONC|65|FFFFFF|000000;Atlético Bucaramanga|ATB|65|FFE500|00843D;"
      "Deportes Tolima|TOL|63|7B1E2B|FFE500;Deportivo Pereira|PER|58|FFE500|E2001A;Alianza FC|ALI|56|FFE500|0055A4;Águilas Doradas|AGU|57|FFE500|000000;La Equidad|EQU|56|00843D|FFFFFF;"
      "Boyacá Chicó|CHI|55|FFE500|000000;Envigado|ENV|55|F47920|00843D;Fortaleza CEIF|FOR|55|FFE500|0055A4;Llaneros|LLA|54|E2001A|FFFFFF;Unión Magdalena|UMA|54|0055A4|E2001A;Deportivo Pasto|PAS|56|E2001A|0055A4" },
    { "CHI1", "Liga de Primera", "CHI", 1, 2,
      "Colo-Colo|COL|68|FFFFFF|000000|Estadio Monumental;Universidad de Chile|UCH|68|0055A4|FFFFFF|Estadio Nacional;Universidad Católica|UCA|65|FFFFFF|0055A4|Claro Arena;CD Palestino|CDP|65|FFFFFF|E2001A;"
      "Deportes Iquique|IQU|60|3FA9F5|FFFFFF;Unión Española|UES|61|E2001A|FFE500;Audax Italiano|AUD|60|00843D|FFFFFF;Huachipato|HUA|61|0055A4|000000;Everton de Viña|EVE|59|0055A4|FFE500;"
      "Coquimbo Unido|COQ|60|FFE500|000000;O'Higgins|OHI|60|3FA9F5|FFFFFF;Cobresal|COB|58|F47920|FFFFFF;Ñublense|NUB|58|E2001A|FFFFFF;Unión La Calera|ULC|57|E2001A|FFFFFF;Deportes La Serena|LSE|56|E2001A|FFFFFF;Deportes Limache|LIM|55|E2001A|000000" },
    { "URU1", "Liga AUF Uruguaya", "URU", 1, 3,
      "Peñarol|PEA|69|FFE500|000000|Campeón del Siglo;Club Nacional de Football|NAC|68|FFFFFF|0055A4|Gran Parque Central;Boston River|BOS|60|00843D|FFFFFF;Cerro Largo|CER|58|0055A4|FFFFFF;Racing Club de Montevideo|RAC|60|00843D|FFFFFF;"
      "Defensor Sporting|DEF|61|5B2C83|FFFFFF;Liverpool (Montevideo)|LIV|61|000000|0055A4;Danubio|DAN|59|FFFFFF|000000;Montevideo Wanderers|WAN|59|000000|FFFFFF;Plaza Colonia|PLA|56|00843D|FFFFFF;"
      "Cerro|CRR|56|3FA9F5|E2001A;Progreso|PRO|55|E2001A|FFE500;Juventud de Las Piedras|JUV|55|00843D|FFFFFF;Miramar Misiones|MIR|54|000000|3FA9F5;Deportivo Maldonado|MAL|56|E2001A|00843D;Montevideo City Torque|MCT|58|3FA9F5|FFFFFF" },
    { "PAR1", "División de Honor", "PAR", 1, 2,
      "Cerro Porteño|CER|67|E2001A|0055A4|La Nueva Olla;Club Olimpia|OLI|62|FFFFFF|000000|Manuel Ferreira;Libertad|LIB|68|000000|FFFFFF;Guaraní|GUA|61|FFE500|000000;Sportivo Luqueño|LUQ|59|FFE500|0055A4;"
      "Nacional Asunción|NAC|58|FFFFFF|0055A4;Sportivo Trinidense|TRI|56|0055A4|FFFFFF;Sportivo Ameliano|AME|56|3FA9F5|FFFFFF;General Caballero JLM|GCA|55|E2001A|FFFFFF;2 de Mayo|DMA|55|E2001A|FFFFFF;Recoleta FC|REC|54|FFE500|000000;Deportivo Recoleta|DRE|54|0055A4|FFFFFF" },
    { "PER1", "Liga 1 (Pérou)", "PER", 1, 3,
      "Universitario de Deportes|UNI|67|FFFFFF|7B1E2B|Estadio Monumental;Alianza Lima|ALI|67|0055A4|FFFFFF|Alejandro Villanueva;Sporting Cristal|SCR|65|3FA9F5|FFFFFF;FBC Melgar|MEL|64|E2001A|000000;Cienciano|CIE|62|E2001A|FFFFFF;"
      "Atlético Grau|GRA|61|FFE500|000000;Cusco FC|CUS|59|FFE500|000000;Deportivo Garcilaso|GAR|58|3FA9F5|FFFFFF;ADT Tarma|ADT|57|E2001A|FFFFFF;Sport Huancayo|HUA|58|E2001A|FFFFFF;"
      "Alianza Atlético|AAT|57|FFFFFF|000000;Los Chankas|CHA|56|E2001A|FFFFFF;Comerciantes Unidos|COM|56|3FA9F5|FFFFFF;Juan Pablo II College|JP2|55|0055A4|FFFFFF;UTC Cajamarca|UTC|56|0055A4|FFFFFF;Sport Boys|SBO|56|FF69B4|000000;Ayacucho FC|AYA|55|F47920|FFFFFF;Alianza Universidad|AUN|55|E2001A|FFFFFF" },
    { "ECU1", "LigaPro", "ECU", 1, 2,
      "LDU Quito|LDU|66|FFFFFF|0055A4|Rodrigo Paz Delgado;Barcelona de Guayaquil|BSC|65|FFE500|E2001A|Monumental Isidro Romero;Independiente del Valle|IDV|65|000000|0055A4;Emelec|EME|62|0055A4|FFFFFF;"
      "Mushuc Runa|MUS|61|E2001A|FFFFFF;Universidad Católica del Ecuador|UCE|62|3FA9F5|FFFFFF;Aucas|AUC|60|FFE500|E2001A;Deportivo Cuenca|CUE|58|E2001A|000000;Orense|ORE|59|00843D|FFFFFF;"
      "El Nacional|NAC|57|E2001A|0055A4;Técnico Universitario|TEC|56|E2001A|FFFFFF;Macará|MAC|56|3FA9F5|FFFFFF;Libertad FC|LIB|56|0055A4|FFFFFF;Delfín|DEL|56|FFE500|0055A4;Vinotinto|VIN|55|7B1E2B|FFFFFF;Manta FC|MAN|54|0055A4|FFFFFF" },
    { "BOL1", "División Profesional", "BOL", 1, 2,
      "Bolívar|BOL|65|3FA9F5|FFFFFF|Hernando Siles;The Strongest|STR|62|FFE500|000000|Rafael Mendoza;Gualberto Villarroel SJ|GVS|56|0055A4|FFFFFF;Nacional Potosí|NPO|58|FFFFFF|5B2C83;San Antonio Bulo Bulo|SAN|58|0055A4|3FA9F5;"
      "Always Ready|ALW|59|FFFFFF|E2001A;Blooming|BLO|57|3FA9F5|FFFFFF;Oriente Petrolero|ORI|56|00843D|FFFFFF;Jorge Wilstermann|WIL|57|E2001A|FFFFFF;Aurora|AUR|55|3FA9F5|FFFFFF;"
      "Real Tomayapo|TOM|54|0055A4|FFFFFF;Guabirá|GUB|54|E2001A|FFFFFF;GV San José|GSJ|54|0055A4|FFFFFF;Independiente Petrolero|IPE|54|0055A4|FFFFFF;Universitario de Vinto|UVI|53|FFE500|0055A4;Real Oruro|ROR|53|FFE500|000000" },
    { "VEN1", "Liga FUTVE", "VEN", 1, 2,
      "Academia Puerto Cabello|APC|60|F47920|7B1E2B;Carabobo FC|CAR|62|7B1E2B|FFFFFF;Caracas FC|CFC|59|E2001A|FFFFFF;Deportivo Táchira|TAC|62|FFE500|000000;Universidad Central|UCV|58|0055A4|FFFFFF;"
      "Deportivo La Guaira|LGU|58|F47920|FFFFFF;Monagas SC|MON|57|0055A4|FFFFFF;Metropolitanos|MET|57|5B2C83|FFFFFF;Zamora FC|ZAM|56|000000|FFFFFF;Portuguesa FC|POR|56|E2001A|FFFFFF;Rayo Zuliano|RAY|55|FFE500|000000;Anzoátegui|ANZ|54|E2001A|FFFFFF;Estudiantes de Mérida|EME|54|E2001A|FFFFFF;Deportivo Rayo|DRA|53|0055A4|FFFFFF" },
    // ============================================================ CONCACAF
    { "MEX1", "Liga MX", "MEX", 1, 0,
      "Club América|AME|72|FFE500|0055A4|Estadio Azteca;Chivas Guadalajara|GDL|68|E2001A|FFFFFF|Estadio Akron;Cruz Azul|CAZ|70|0055A4|FFFFFF|Estadio Ciudad de los Deportes;Tigres UANL|TIG|71|FFE500|0055A4|Estadio Universitario;"
      "CF Monterrey|MTY|71|0055A4|FFFFFF|Estadio BBVA;Pumas UNAM|PUM|65|FFFFFF|0055A4|Estadio Olímpico Universitario;Deportivo Toluca|TOL|68|E2001A|FFFFFF|Nemesio Díez;Club León|LEO|65|00843D|FFFFFF;"
      "Pachuca|PAC|66|0055A4|FFFFFF;Santos Laguna|SAN|62|00843D|FFFFFF;Atlas|ATL|62|E2001A|000000;Club Tijuana|TIJ|63|E2001A|000000;Puebla|PUE|59|FFFFFF|0055A4;Necaxa|NEC|60|E2001A|FFFFFF;"
      "Querétaro|QRO|58|0055A4|000000;Mazatlán FC|MAZ|58|5B2C83|FFFFFF;FC Juárez|JUA|60|000000|E2001A;Atlético San Luis|ASL|60|E2001A|FFFFFF" },
    { "CAN1", "Canadian Premier League", "CAN", 1, 0,
      "Forge FC|FOR|52|F47920|000000;Cavalry FC|CAV|52|E2001A|00843D;Pacific FC|PAC|50|5B2C83|FFFFFF;Atlético Ottawa|OTT|51|E2001A|FFFFFF;York United|YOR|49|00843D|FFFFFF;"
      "HFX Wanderers|HFX|48|0055A4|FFFFFF;Vancouver FC|VAN|47|000000|F47920;Valour FC|VAL|47|7B1E2B|FFE500" },
    { "CRC1", "Primera División (Costa Rica)", "CRC", 1, 1,
      "Deportivo Saprissa|SAP|60|5B2C83|FFFFFF|Estadio Ricardo Saprissa;LD Alajuelense|LDA|61|E2001A|000000|Alejandro Morera Soto;CS Herediano|HER|59|E2001A|FFE500;CS Cartaginés|CAR|55|0055A4|FFFFFF;"
      "Municipal Liberia|LIB|53|FFE500|0055A4;Sporting FC|SPO|52|000000|FFFFFF;Pérez Zeledón|PZE|51|E2001A|FFFFFF;Puntarenas FC|PUN|51|F47920|FFFFFF;AD Guanacasteca|GUA|50|FFE500|000000;Santa Ana|SAN|49|0055A4|FFFFFF;San Carlos|SCA|51|00843D|FFFFFF;Municipal Grecia|GRE|49|E2001A|FFFFFF" },
    { "HON1", "Liga Nacional (Honduras)", "HON", 1, 1,
      "Olimpia|OLI|56|FFFFFF|0055A4;Motagua|MOT|55|0055A4|FFFFFF;Real España|RES|54|FFE500|000000;Marathón|MAR|52|00843D|E2001A;Olancho FC|OLA|50|0055A4|FFFFFF;"
      "Génesis|GEN|48|0055A4|FFFFFF;Lobos UPNFM|LOB|48|0055A4|FFFFFF;Victoria|VIC|48|0055A4|FFFFFF;Juticalpa|JUT|47|E2001A|FFFFFF;CD Choloma|CHO|46|E2001A|FFFFFF" },
    { "GUA1", "Liga Nacional (Guatemala)", "GUA", 1, 2,
      "CSD Municipal|MUN|55|E2001A|0055A4;Comunicaciones|COM|55|FFFFFF|000000;Antigua GFC|ANT|54|5B2C83|FFFFFF;Xelajú MC|XEL|52|E2001A|0055A4;Cobán Imperial|COB|50|E2001A|FFFFFF;"
      "Malacateco|MAL|49|E2001A|0055A4;Guastatoya|GUA|50|E2001A|FFFFFF;Mixco|MIX|48|0055A4|FFFFFF;Achuapa|ACH|47|0055A4|FFFFFF;Marquense|MAR|47|0055A4|FFFFFF;Mictlán|MIC|46|E2001A|FFFFFF;Aurora FC|AUR|46|FFE500|000000" },
    { "PAN1", "Liga Panameña", "PAN", 1, 1,
      "Plaza Amador|PLA|52|E2001A|FFFFFF;CD Árabe Unido|ARA|53|0055A4|FFFFFF;Tauro FC|TAU|53|000000|E2001A;CA Independiente|CAI|53|E2001A|000000;San Francisco FC|SFR|50|E2001A|FFFFFF;"
      "Alianza FC (PAN)|ALI|49|0055A4|FFFFFF;UMECIT|UME|48|5B2C83|FFFFFF;Sporting San Miguelito|SSM|49|F47920|000000;Herrera FC|HER|47|E2001A|FFFFFF;Veraguas CD|VER|47|00843D|FFFFFF" },
    { "JAM1", "Jamaica Premier League", "JAM", 1, 2,
      "Cavalier FC|CAV|45|0055A4|FFFFFF;Mount Pleasant FA|MPL|46|00843D|FFFFFF;Harbour View|HBV|44|FFE500|000000;Waterhouse FC|WAT|44|3FA9F5|FFFFFF;Arnett Gardens|ARN|43|E2001A|00843D;"
      "Portmore United|POR|43|E2001A|FFFFFF;Dunbeholden|DUN|41|00843D|FFFFFF;Tivoli Gardens|TIV|42|000000|FFE500;Molynes United|MOL|40|E2001A|FFFFFF;Montego Bay United|MBU|40|0055A4|FFFFFF" },
    // ============================================================ CAF
    { "EGY1", "Egyptian Premier League", "EGY", 1, 3,
      "Al Ahly|AHL|66|E2001A|FFFFFF|Stade international du Caire;Zamalek|ZAM|62|FFFFFF|E2001A|Stade international du Caire;Pyramids FC|PYR|64|0055A4|FFFFFF;Al Masry|MAS|56|00843D|FFFFFF;"
      "Ismaily|ISM|53|FFE500|0055A4;Future FC|FUT|55|F47920|000000;El Gouna|GOU|52|0055A4|FFFFFF;ZED FC|ZED|53|000000|FFFFFF;Ceramica Cleopatra|CER|55|F47920|FFFFFF;"
      "Smouha|SMO|53|0055A4|FFFFFF;ENPPI|ENP|53|E2001A|FFFFFF;Al Ittihad Alexandrie|ITT|53|00843D|FFFFFF;Pharco|PHA|51|0055A4|FFFFFF;El Mokawloon|MOK|51|FFE500|000000;National Bank|NBE|52|0055A4|FFFFFF;Petrojet|PET|50|FFE500|000000;Ghazl El Mahalla|GHA|50|FFFFFF|000000;Wadi Degla|WAD|50|FFE500|000000" },
    { "MAR1", "Botola Pro", "MAR", 1, 2,
      "Wydad Casablanca|WAC|62|E2001A|FFFFFF|Stade Mohammed-V;Raja Casablanca|RCA|62|00843D|FFFFFF|Stade Mohammed-V;AS FAR Rabat|FAR|61|E2001A|00843D|Stade Moulay-Abdallah;RS Berkane|RSB|61|F47920|000000;"
      "FUS Rabat|FUS|56|E2001A|FFFFFF;Maghreb de Fès|MAS|55|FFE500|000000;Hassania Agadir|HUS|54|E2001A|000000;Ittihad Tanger|IRT|55|0055A4|FFFFFF;Olympique Safi|OCS|53|E2001A|FFFFFF;"
      "Difaâ El Jadida|DHJ|53|00843D|E2001A;Renaissance Zemamra|RCZ|52|00843D|FFFFFF;Union Touarga|UTS|52|0055A4|FFFFFF;Kawkab Marrakech|KAC|53|E2001A|FFFFFF;Olympic Dcheira|OCD|51|0055A4|FFFFFF;COD Meknès|CODM|51|E2001A|00843D;Yacoub El Mansour|YSM|50|FFE500|0055A4" },
    { "ALG1", "Ligue 1 Mobilis", "ALG", 1, 2,
      "MC Alger|MCA|58|E2001A|00843D;CR Belouizdad|CRB|58|E2001A|FFFFFF;JS Kabylie|JSK|57|FFE500|00843D|Stade Hocine-Aït-Ahmed;USM Alger|USMA|57|E2001A|000000;CS Constantine|CSC|55|00843D|000000;"
      "ES Sétif|ESS|54|000000|FFFFFF;MC Oran|MCO|53|E2001A|FFFFFF;ASO Chlef|ASO|52|E2001A|FFFFFF;Paradou AC|PAC|53|FFE500|0055A4;USM Khenchela|USK|50|00843D|FFFFFF;"
      "JS Saoura|JSS|52|FFE500|00843D;Olympique Akbou|OAK|50|0055A4|FFFFFF;ES Mostaganem|ESM|50|00843D|FFFFFF;MB Rouissat|MBR|49|00843D|FFFFFF;ES Ben Aknoun|ESBA|48|00843D|FFFFFF;MC El Bayadh|MCEB|49|0055A4|FFFFFF" },
    { "TUN1", "Ligue 1 (Tunisie)", "TUN", 1, 2,
      "Espérance de Tunis|EST|62|E2001A|FFE500|Stade Hamadi-Agrebi;Club africain|CA|58|E2001A|FFFFFF;Étoile du Sahel|ESS|58|E2001A|FFFFFF|Stade olympique de Sousse;CS Sfaxien|CSS|57|000000|FFFFFF;"
      "US Monastir|USM|56|0055A4|FFFFFF;Stade tunisien|ST|53|E2001A|00843D;CA Bizertin|CAB|51|000000|FFE500;US Ben Guerdane|USBG|51|E2001A|FFFFFF;ES Métlaoui|ESM|49|E2001A|FFFFFF;"
      "JS Omrane|JSO|49|0055A4|FFFFFF;AS Marsa|ASM|50|FFE500|E2001A;Olympique Béja|OB|50|E2001A|FFFFFF;ES Zarzis|ESZ|50|FFE500|000000;AS Soliman|ASS|49|E2001A|FFFFFF;US Tataouine|UST|48|0055A4|FFFFFF;JS Kairouan|JSK|48|0055A4|FFFFFF" },
    { "RSA1", "Betway Premiership", "RSA", 1, 1,
      "Mamelodi Sundowns|SUN|65|FFE500|0055A4|Loftus Versfeld;Orlando Pirates|ORL|62|000000|FFFFFF|Orlando Stadium;Kaizer Chiefs|KCH|59|FFE500|000000|FNB Stadium;Stellenbosch FC|STE|56|7B1E2B|FFFFFF;"
      "Sekhukhune United|SEK|54|000000|FFE500;SuperSport United|SSU|55|0055A4|FFFFFF;AmaZulu|AMA|54|00843D|FFFFFF;TS Galaxy|TSG|54|FFFFFF|E2001A;Polokwane City|POL|53|FFE500|00843D;"
      "Golden Arrows|GAR|52|00843D|FFE500;Chippa United|CHI|52|3FA9F5|FFFFFF;Richards Bay|RIC|52|0055A4|FFFFFF;Magesi FC|MAG|51|E2001A|FFFFFF;Marumo Gallants|MAR|51|FFE500|E2001A;Durban City|DUR|51|0055A4|FFFFFF;Orbit College|ORB|50|0055A4|FFFFFF" },
    { "NGA1", "Nigeria Premier Football League", "NGA", 1, 4,
      "Enyimba|ENY|52|0055A4|FFFFFF;Rivers United|RIV|52|0055A4|FFFFFF;Remo Stars|REM|52|3FA9F5|FFFFFF;Enugu Rangers|RAN|51|E2001A|FFFFFF;Kano Pillars|KAN|50|E2001A|FFE500;"
      "Shooting Stars|3SC|50|0055A4|FFFFFF;Plateau United|PLA|50|E2001A|FFFFFF;Bendel Insurance|BEN|49|E2001A|0055A4;Kwara United|KWA|49|00843D|FFFFFF;Lobi Stars|LOB|48|00843D|FFFFFF;"
      "Nasarawa United|NAS|48|0055A4|FFFFFF;Abia Warriors|ABI|48|F47920|FFFFFF;Ikorodu City|IKO|48|FFE500|0055A4;Katsina United|KAT|47|FFE500|E2001A;Niger Tornadoes|NIG|47|FFE500|00843D;El Kanemi Warriors|ELK|47|0055A4|FFFFFF;Bayelsa United|BAY|47|0055A4|FFFFFF;Kun Khalifat|KUN|46|00843D|FFFFFF;Wikki Tourists|WIK|46|00843D|FFFFFF;Warri Wolves|WAR|46|0055A4|FFFFFF" },
    { "GHA1", "Ghana Premier League", "GHA", 1, 3,
      "Asante Kotoko|KOT|52|E2001A|FFFFFF|Baba Yara Stadium;Hearts of Oak|HOA|52|FFFFFF|E2001A|Accra Sports Stadium;Medeama SC|MED|50|FFE500|0055A4;Bibiani Gold Stars|BGS|50|FFE500|000000;Nations FC|NAT|49|E2001A|FFFFFF;"
      "Aduana FC|ADU|49|FFE500|00843D;Dreams FC|DRE|49|000000|FFFFFF;Berekum Chelsea|BCH|48|0055A4|FFFFFF;Karela United|KAR|47|0055A4|FFFFFF;Samartex|SAM|48|0055A4|FFFFFF;Bechem United|BEC|47|E2001A|FFFFFF;"
      "Heart of Lions|HOL|46|0055A4|FFFFFF;Accra Lions|ACC|47|0055A4|FFFFFF;Vision FC|VIS|46|E2001A|FFFFFF;Young Apostles|YAP|45|0055A4|FFFFFF;Swedru All Blacks|SAB|45|000000|FFFFFF;Basake Holy Stars|BHS|45|0055A4|FFFFFF;Eleven Wonders|ELW|45|FFE500|000000" },
    { "SEN1", "Ligue 1 (Sénégal)", "SEN", 1, 2,
      "Jaraaf|JAR|50|00843D|FFFFFF;Teungueth FC|TFC|50|E2001A|FFFFFF;Génération Foot|GEF|49|F47920|000000;AS Pikine|PIK|48|0055A4|FFFFFF;Casa Sports|CAS|48|E2001A|00843D;"
      "US Gorée|GOR|48|E2001A|FFE500;Guédiawaye FC|GUE|47|0055A4|FFFFFF;Diambars|DIA|47|FFE500|000000;AS Douanes|DOU|47|00843D|FFFFFF;Linguère|LIN|46|FFFFFF|000000;HLM Dakar|HLM|45|0055A4|FFFFFF;Wally Daan|WAL|45|0055A4|FFFFFF;Jamono Fatick|JAM|45|E2001A|FFFFFF;Oslo FA|OSL|44|E2001A|FFFFFF" },
    { "CIV1", "Ligue 1 (Côte d'Ivoire)", "CIV", 1, 2,
      "ASEC Mimosas|ASEC|53|FFE500|000000|Stade Félix-Houphouët-Boigny;Africa Sports|AFR|50|E2001A|00843D;Stade d'Abidjan|STA|50|0055A4|FFFFFF;SOA|SOA|49|00843D|FFFFFF;San Pédro FC|SPE|50|FFE500|0055A4;"
      "Sporting Gagnoa|SGA|49|0055A4|FFFFFF;SC Gagnoa|SCG|48|0055A4|FFFFFF;AFAD Djékanou|AFA|49|FFFFFF|E2001A;Racing Club Abidjan|RCA|48|000000|FFFFFF;Lys Sassandra|LYS|47|0055A4|FFFFFF;Mouna FC|MOU|46|00843D|FFFFFF;Zoman FC|ZOM|46|F47920|FFFFFF;ES Bafing|ESB|45|FFE500|00843D;Bouaké FC|BOU|46|E2001A|FFFFFF" },
    { "COD1", "Linafoot", "COD", 1, 2,
      "TP Mazembe|MAZ|56|000000|FFFFFF|Stade TP Mazembe;AS Vita Club|VIT|53|00843D|000000|Stade des Martyrs;DC Motema Pembe|DCM|52|00843D|FFFFFF;FC Saint-Éloi Lupopo|LUP|52|E2001A|FFFFFF;AS Maniema Union|MAN|51|E2001A|FFFFFF;"
      "FC Lupopo|FCL|50|E2001A|FFFFFF;Les Aigles du Congo|AIG|50|0055A4|FFFFFF;AS Simba|SIM|48|E2001A|FFFFFF;FC Renaissance du Congo|REN|48|0055A4|FFFFFF;JS Kinshasa|JSK|47|0055A4|FFFFFF;OC Bukavu Dawa|BUK|47|00843D|FFFFFF;CS Don Bosco|DBO|47|0055A4|FFFFFF" },
    { "CMR1", "Elite One", "CMR", 1, 2,
      "Coton Sport Garoua|COT|52|00843D|FFFFFF;Canon Yaoundé|CAN|50|00843D|E2001A;Union Douala|UDO|49|00843D|FFFFFF;Tonnerre Yaoundé|TON|48|FFFFFF|000000;Colombe Sportive|COL|48|0055A4|FFFFFF;"
      "Dynamo Douala|DYN|47|0055A4|FFFFFF;Fovu Baham|FOV|47|FFE500|000000;Victoria United|VIC|47|0055A4|FFFFFF;Gazelle FA|GAZ|46|00843D|FFFFFF;Bamboutos|BAM|46|E2001A|FFFFFF;Aigle Royal|AIG|46|0055A4|FFFFFF;Stade Renard|REN|46|F47920|FFFFFF" },
    // ============================================================ AFC
    { "JPN1", "J1 League", "JPN", 1, 3,
      "Vissel Kobe|KOB|66|7B1E2B|FFFFFF|Noevir Stadium;Sanfrecce Hiroshima|HIR|65|5B2C83|FFFFFF|Edion Peace Wing;Kashima Antlers|KAS|66|7B1E2B|0055A4|Kashima Soccer Stadium;Urawa Red Diamonds|URA|64|E2001A|FFFFFF|Saitama Stadium;"
      "Kawasaki Frontale|KAW|64|3FA9F5|000000;Yokohama F. Marinos|YFM|63|0055A4|FFFFFF|Nissan Stadium;Machida Zelvia|MAC|64|0055A4|FFFFFF;Gamba Osaka|GAM|62|0055A4|000000;Cerezo Osaka|CER|61|FF69B4|0055A4;"
      "FC Tokyo|TOK|62|0055A4|E2001A;Kashiwa Reysol|KSW|63|FFE500|000000;Nagoya Grampus|NAG|61|E2001A|FFFFFF;Kyoto Sanga|KYO|62|5B2C83|FFFFFF;Avispa Fukuoka|FUK|60|0055A4|FFFFFF;"
      "Tokyo Verdy|VER|60|00843D|FFFFFF;Albirex Niigata|NII|59|F47920|FFFFFF;Shonan Bellmare|SHO|59|00843D|0055A4;Shimizu S-Pulse|SHI|60|F47920|FFFFFF;Fagiano Okayama|OKA|58|7B1E2B|FFFFFF;Yokohama FC|YFC|58|3FA9F5|FFFFFF" },
    { "JPN2", "J2 League", "JPN", 2, 3,
      "Júbilo Iwata|IWA|57|3FA9F5|FFFFFF;Consadole Sapporo|SAP|57|E2001A|000000;Sagan Tosu|TOS|56|3FA9F5|FF69B4;V-Varen Nagasaki|NAG|57|F47920|0055A4;Vegalta Sendai|SEN|56|FFE500|0055A4;"
      "JEF United Chiba|CHI|56|FFE500|00843D;Tokushima Vortis|TOK|55|0055A4|00843D;Montedio Yamagata|YAM|55|0055A4|FFFFFF;Omiya Ardija|OMI|55|F47920|0055A4;Ventforet Kofu|KOF|54|0055A4|E2001A;"
      "Mito HollyHock|MIT|54|0055A4|FFFFFF;RB Omiya|RBO|55|E2001A|FFFFFF;Roasso Kumamoto|KUM|53|E2001A|FFFFFF;Blaublitz Akita|AKI|53|0055A4|FFFFFF;Iwaki FC|IWK|53|E2001A|FFFFFF;"
      "Fujieda MYFC|FUJ|52|5B2C83|FFFFFF;Renofa Yamaguchi|REN|52|F47920|FFFFFF;Ehime FC|EHI|52|F47920|FFFFFF;Kataller Toyama|TOY|51|0055A4|FFFFFF;Imabari|IMA|52|0055A4|FFE500" },
    { "QAT1", "Qatar Stars League", "QAT", 1, 1,
      "Al Sadd|SAD|64|FFFFFF|000000|Jassim-bin-Hamad;Al Duhail|DUH|62|E2001A|FFFFFF;Al Rayyan|RAY|60|E2001A|000000;Al Gharafa|GHA|60|FFE500|0055A4;Al Arabi (QAT)|ARA|58|E2001A|FFFFFF;"
      "Al Wakrah|WAK|56|0055A4|FFFFFF;Qatar SC|QSC|55|0055A4|FFFFFF;Al Ahli Doha|AHD|55|E2001A|FFFFFF;Umm Salal|UMS|54|F47920|FFFFFF;Al Shamal|SHA|53|3FA9F5|FFFFFF;Al Shahania|SHN|52|000000|FFFFFF;Al Khor|KHO|53|FFE500|000000" },
    { "UAE1", "UAE Pro League", "UAE", 1, 2,
      "Al Ain|AIN|67|5B2C83|FFFFFF|Hazza bin Zayed;Shabab Al Ahli|SHA|66|E2001A|000000;Al Wahda|WAH|62|7B1E2B|FFFFFF;Al Jazira|JAZ|61|E2001A|000000;Sharjah FC|SHJ|62|FFFFFF|E2001A;"
      "Al Nasr Dubaï|NAS|60|0055A4|FFE500;Al Wasl|WAS|60|FFE500|000000;Baniyas|BAN|56|E2001A|FFFFFF;Ajman Club|AJM|54|0055A4|FFFFFF;Kalba|KAL|54|0055A4|FFFFFF;Khor Fakkan|KHF|53|F47920|FFFFFF;Al Bataeh|BAT|52|0055A4|FFFFFF;Dibba Al Fujairah|DIB|51|E2001A|FFFFFF;Al Dhafra|DHA|51|0055A4|FFFFFF" },
    { "IRN1", "Persian Gulf Pro League", "IRN", 1, 2,
      "Persépolis|PER|62|E2001A|FFFFFF|Stade Azadi;Esteghlal|EST|61|0055A4|FFFFFF|Stade Azadi;Sepahan|SEP|61|FFE500|0055A4;Tractor|TRA|62|E2001A|FFFFFF;Foolad Khuzestan|FOO|56|FFE500|E2001A;"
      "Gol Gohar Sirjan|GOL|56|F47920|FFFFFF;Zob Ahan|ZOB|54|00843D|FFFFFF;Malavan|MAL|53|E2001A|FFFFFF;Aluminium Arak|ALU|53|0055A4|FFFFFF;Mes Rafsanjan|MES|52|F47920|FFFFFF;"
      "Chadormalou|CHA|52|FFE500|000000;Esteghlal Khuzestan|ESK|52|0055A4|FFFFFF;Havadar|HAV|51|0055A4|FFFFFF;Kheybar Khorramabad|KHE|51|E2001A|FFFFFF;Paykan|PAY|51|0055A4|FFFFFF;Zob Ahan B|ZAB|48|00843D|FFFFFF" },
    { "THA1", "Thai League 1", "THA", 1, 3,
      "Buriram United|BUR|58|0055A4|F47920|Chang Arena;Bangkok United|BKU|55|E2001A|FFFFFF;BG Pathum United|BGP|55|0055A4|FFFFFF;Port FC|POR|54|0055A4|F47920;Muangthong United|MTU|54|FFE500|E2001A;"
      "Chiangrai United|CHI|52|0055A4|F47920;Ratchaburi FC|RAT|53|E2001A|000000;Uthai Thani|UTH|51|F47920|FFFFFF;Sukhothai FC|SUK|50|FFE500|000000;Prachuap FC|PRA|50|E2001A|FFFFFF;"
      "Nakhon Ratchasima|NAK|50|F47920|FFFFFF;Lamphun Warriors|LAM|50|000000|FFFFFF;Rayong FC|RAY|49|0055A4|FFFFFF;Chonburi FC|CHO|51|0055A4|FFFFFF;Kanchanaburi Power|KAN|49|E2001A|FFFFFF;Ayutthaya United|AYU|48|E2001A|FFFFFF" },
    { "UZB1", "Super League (Ouzbékistan)", "UZB", 1, 2,
      "Pakhtakor Tachkent|PAK|56|00843D|FFFFFF;Nasaf Qarshi|NAS|55|0055A4|FFFFFF;Navbahor Namangan|NAV|53|00843D|FFFFFF;Neftchi Fergana|NEF|53|FFFFFF|0055A4;AGMK|AGM|53|E2001A|FFFFFF;"
      "Bunyodkor|BUN|51|0055A4|FFFFFF;Surkhon Termez|SUR|50|00843D|FFFFFF;Andijan|AND|50|FFE500|0055A4;Dinamo Samarkand|DIN|50|0055A4|FFFFFF;Olympic Tachkent|OLY|50|0055A4|FFFFFF;Qizilqum Zarafshon|QIZ|49|E2001A|FFFFFF;Kokand 1912|KOK|49|0055A4|FFFFFF;Bukhara|BUK|49|E2001A|FFFFFF;Shortan Guzar|SHO|48|00843D|FFFFFF" },
    { "IDN1", "Super League (Indonésie)", "IDN", 1, 3,
      "Persib Bandung|PSB|52|0055A4|FFFFFF|Gelora Bandung Lautan Api;Persija Jakarta|PSJ|51|F47920|E2001A;Bali United|BAL|50|E2001A|000000;Persebaya Surabaya|PSS|50|00843D|FFFFFF;Borneo FC|BOR|50|F47920|000000;"
      "PSM Makassar|PSM|49|E2001A|FFFFFF;Arema FC|ARE|48|0055A4|FFFFFF;Dewa United|DEW|49|000000|FFE500;Persis Solo|PSO|48|E2001A|000000;Malut United|MAL|48|F47920|000000;"
      "PSIS Semarang|PSI|47|0055A4|FFFFFF;Persita Tangerang|PTA|47|5B2C83|FFFFFF;Madura United|MAD|47|E2001A|FFFFFF;Bhayangkara FC|BHA|47|0055A4|FFFFFF;Semen Padang|SEM|46|E2001A|000000;PSBS Biak|BIA|46|0055A4|FFFFFF;Persijap Jepara|PJE|45|E2001A|FFFFFF;Persik Kediri|PKD|46|5B2C83|FFFFFF" },
    { "VIE1", "V.League 1", "VIE", 1, 1,
      "Nam Định|NAM|48|00843D|FFFFFF;Hà Nội FC|HAN|49|5B2C83|FFFFFF;Thể Công-Viettel|VIE|48|E2001A|FFFFFF;Công An Hà Nội|CAH|49|E2001A|FFE500;Thép Xanh Nam Định|TXN|47|00843D|FFFFFF;"
      "Hải Phòng|HAI|46|E2001A|FFFFFF;Bình Dương|BIN|46|0055A4|FFFFFF;Thanh Hóa|THA|46|FFE500|000000;Hoàng Anh Gia Lai|HAG|45|FFE500|0055A4;SHB Đà Nẵng|DAN|45|F47920|FFFFFF;"
      "Sông Lam Nghệ An|SLN|45|FFE500|E2001A;Hồng Lĩnh Hà Tĩnh|HLT|44|7B1E2B|FFFFFF;Ninh Bình FC|NIB|46|0055A4|FFFFFF;PVF-CAND|PVF|44|E2001A|FFFFFF" },
    { "MAS1", "Malaysia Super League", "MAS", 1, 0,
      "Johor Darul Ta'zim|JDT|58|E2001A|0055A4|Sultan Ibrahim Stadium;Selangor FC|SEL|50|E2001A|FFE500;Sabah FC|SAB|48|0055A4|FFFFFF;Terengganu FC|TER|48|FFFFFF|000000;Kuching City|KUC|47|E2001A|FFFFFF;"
      "Negeri Sembilan|NS|46|FFE500|E2001A;Penang FC|PEN|46|0055A4|FFE500;Kuala Lumpur City|KLC|47|0055A4|E2001A;PDRM FC|PDR|45|0055A4|FFFFFF;Kelantan Darul Naim|KEL|44|E2001A|FFFFFF;Immigration FC|IMM|44|000000|FFFFFF;Melaka FC|MEL|44|00843D|FFFFFF;DPMM FC|DPM|45|FFE500|000000" },
    { "IRQ1", "Iraq Stars League", "IRQ", 1, 3,
      "Al Shorta|SHO|55|0055A4|FFFFFF;Al Zawraa|ZAW|55|FFFFFF|0055A4;Al Quwa Al Jawiya|QWJ|54|3FA9F5|FFFFFF;Duhok SC|DUH|53|E2001A|FFE500;Erbil SC|ERB|51|FFE500|000000;"
      "Al Talaba|TAL|52|FFE500|00843D;Al Karma|KAR|51|0055A4|FFFFFF;Naft Al Basra|NAB|50|E2001A|FFFFFF;Al Najaf|NAJ|50|00843D|FFFFFF;Zakho SC|ZAK|50|E2001A|FFFFFF;"
      "Al Kahrabaa|KAH|49|0055A4|FFFFFF;Al Naft|NAF|49|E2001A|FFFFFF;Karbala FC|KBL|48|0055A4|FFFFFF;Al Qasim|QAS|47|E2001A|FFFFFF;Al Diwaniya|DIW|47|0055A4|FFFFFF;Newroz SC|NEW|47|00843D|FFFFFF" },
    { "KOR2", "K League 2", "KOR", 2, 1,
      "Suwon Samsung Bluewings|SSB|58|0055A4|FFFFFF;Incheon United|INC|57|0055A4|000000;Busan IPark|BUS|55|E2001A|FFFFFF;Jeonnam Dragons|JEO|55|FFE500|000000;Seoul E-Land|ELA|55|000000|FFFFFF;"
      "Seongnam FC|SEO|54|000000|FFFFFF;Bucheon FC 1995|BUC|54|E2001A|000000;Gimpo FC|GIM|53|00843D|FFE500;Chungnam Asan|ASA|53|0055A4|FFFFFF;Cheongju FC|CHE|52|0055A4|FFFFFF;"
      "Cheonan City|CHC|51|3FA9F5|FFFFFF;Ansan Greeners|ANS|51|00843D|FFFFFF;Gyeongnam FC|GYE|53|E2001A|FFFFFF;Hwaseong FC|HWA|50|F47920|FFFFFF" },
    // ============================================================ OFC
    { "NZL1", "National League (NZ)", "NZL", 1, 0,
      "Auckland City|AUC|45|0055A4|FFFFFF|Kiwitea Street;Wellington Olympic|WOL|42|E2001A|FFFFFF;Christchurch United|CHU|41|E2001A|000000;Birkenhead United|BIR|40|000000|FFE500;Auckland United|AUU|40|FFE500|000000;"
      "Western Springs|WES|39|000000|FFFFFF;Eastern Suburbs|EAS|40|000000|FFFFFF;Napier City Rovers|NAP|38|0055A4|FFFFFF;Cashmere Technical|CAS|39|E2001A|FFFFFF;Wellington Phoenix Reserves|WPR|40|FFE500|000000" },
    { "NCL1", "Super Ligue (Nouvelle-Calédonie)", "NCL", 1, 0,
      "AS Magenta|MAG|36|E2001A|FFFFFF|Stade Numa-Daly;Hienghène Sport|HIE|35|FFE500|000000;AS Tiga Sports|TIG|33|00843D|FFFFFF;AS Lössi|LOS|32|0055A4|FFFFFF;AS Wetr|WET|31|E2001A|FFFFFF;"
      "Mont-Dore FC|MDO|31|0055A4|FFFFFF;AS Kunié|KUN|30|00843D|FFFFFF;Olympique Nouméa|OLN|30|0055A4|FFFFFF" },
    { "TAH1", "Ligue 1 (Tahiti)", "TAH", 1, 0,
      "AS Pirae|PIR|35|0055A4|FFFFFF|Stade Pater;AS Vénus|VEN|34|0055A4|FFFFFF;AS Central Sport|CEN|33|E2001A|FFFFFF;AS Tefana|TEF|33|FFE500|000000;AS Dragon|DRA|32|00843D|FFFFFF;"
      "AS Tiare Tahiti|TIA|30|0055A4|FFFFFF;AS Manu-Ura|MAN|30|E2001A|FFFFFF;AS Excelsior|EXC|29|0055A4|FFFFFF" },
    { "FIJ1", "Fiji Premier League", "FIJ", 1, 0,
      "Rewa FC|REW|33|E2001A|FFFFFF;Lautoka FC|LAU|33|E2001A|FFE500;Ba FC|BA|32|000000|FFFFFF;Labasa FC|LAB|31|E2001A|00843D;Suva FC|SUV|31|0055A4|FFFFFF;Nadi FC|NAD|30|E2001A|FFE500;Navua FC|NAV|29|0055A4|FFFFFF;Tailevu Naitasiri|TAI|29|00843D|FFFFFF" },
};
const int NUM_EXT_LEAGUES = (int)(sizeof(EXT_LEAGUES) / sizeof(EXT_LEAGUES[0]));

// noms des coupes nationales (sinon « Coupe de <pays> »)
const char* extCupName(const char* country) {
    static const char* T[][2] = {
        { "USA", "U.S. Open Cup" }, { "BRA", "Copa do Brasil" }, { "ARG", "Copa Argentina" }, { "MEX", "Copa MX" }, { "JPN", "Coupe de l'Empereur" },
        { "KOR", "Korea Cup" }, { "CHN", "Coupe de Chine" }, { "AUS", "Australia Cup" }, { "IND", "Super Cup (Inde)" }, { "KSA", "Coupe du Roi" },
        { "COL", "Copa Colombia" }, { "CHI", "Copa Chile" }, { "URU", "Copa AUF Uruguay" }, { "PAR", "Copa Paraguay" }, { "PER", "Copa Bicentenario" },
        { "ECU", "Copa Ecuador" }, { "BOL", "Copa de la División Profesional" }, { "VEN", "Copa Venezuela" }, { "CAN", "Championnat canadien" },
        { "EGY", "Coupe d'Égypte" }, { "MAR", "Coupe du Trône" }, { "ALG", "Coupe d'Algérie" }, { "TUN", "Coupe de Tunisie" }, { "RSA", "Nedbank Cup" },
        { "QAT", "Coupe de l'Émir" }, { "UAE", "Coupe du Président" }, { "IRN", "Hazfi Cup" }, { "THA", "Thai FA Cup" }, { "GRE", "Coupe de Grèce" },
        { "CRO", "Coupe de Croatie" }, { "SRB", "Coupe de Serbie" }, { "CZE", "MOL Cup" }, { "UKR", "Coupe d'Ukraine" }, { "HUN", "Coupe de Hongrie" },
        { "ISR", "Coupe d'Israël" }, { "CYP", "Coupe de Chypre" }, { "BUL", "Coupe de Bulgarie" }, { "RUS", "Coupe de Russie" }, { "WAL", "Welsh Cup" },
        { "NIR", "Irish Cup" }, { "NZL", "Chatham Cup" }, { "NCL", "Coupe de Nouvelle-Calédonie" }, { "TAH", "Coupe de Polynésie" },
    };
    for (auto& e : T) if (!strcmp(e[0], country)) return e[1];
    return nullptr;
}
