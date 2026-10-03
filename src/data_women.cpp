// Football féminin (section 11) : championnats réels (saison 2025-26) et principales joueuses.
// Club : "Nom|ABR|note|couleur1|couleur2|stade|club masculin lié" (les deux derniers champs facultatifs), séparés par ';'.
// Joueuse : "Nom|poste (G/D/M/A)|note|code pays|âge", séparées par ';'.
#include "data.h"
#include <cstring>

const WomenLeagueDef WOMEN_LEAGUES[] = {
    { "FRA", 1, "Arkema Première Ligue", 2,
      "OL Lyonnes|OLL|86|FFFFFF|1C3F94|Groupama OL Training Center|Olympique Lyonnais;PSG Féminines|PSG|80|004170|DA291C|Stade Jean-Bouin|Paris Saint-Germain;"
      "Paris FC Féminines|PFC|79|1B2A63|FFFFFF|Stade Sébastien-Charléty|Paris FC;Montpellier HSC Féminines|MHSC|71|0055A4|F47920|Stade Joseph-Blanc|Montpellier HSC;"
      "FC Fleury 91|FL91|70|5B2C83|FFFFFF|Stade Robert-Bobin|FC Fleury 91;Dijon FCO Féminines|DFCO|68|E2001A|FFFFFF|Stade Gaston-Gérard|Dijon FCO;"
      "Stade de Reims Féminines|SDR|68|E2001A|FFFFFF|Stade Auguste-Delaune|Stade de Reims;Le Havre AC Féminines|HAC|63|87CEEB|1C2F5C|Stade Océane|Le Havre AC;"
      "FC Nantes Féminines|FCN|66|FFE500|00843D|Stade Marcel-Saupin|FC Nantes;RC Strasbourg Féminines|RCSA|63|009FE3|FFFFFF|Stade de la Meinau|RC Strasbourg;"
      "AS Saint-Étienne Féminines|ASSE|63|00843D|FFFFFF|Stade Geoffroy-Guichard|AS Saint-Étienne;OM Féminines|OM|64|FFFFFF|2FAEE0|Orange Vélodrome|Olympique de Marseille" },
    { "FRA", 2, "Seconde Ligue", 2,
      "EA Guingamp Féminines|EAG|60|E2001A|000000|Stade du Roudourou|EA Guingamp;RC Lens Féminines|RCL|61|FFE500|E2001A|Stade Bollaert-Delelis|RC Lens;"
      "LOSC Lille Féminines|LOSC|60|E01E13|24216A|Domaine de Luchin|LOSC Lille;FC Metz Féminines|FCM|56|7B1E2B|FFFFFF|Stade Saint-Symphorien|FC Metz;"
      "Rodez AF Féminines|RAF|55|E2001A|FFE500|Stade Paul-Lignon|Rodez AF;Toulouse FC Féminines|TFC|55|5B2C83|FFFFFF|Stadium de Toulouse|Toulouse FC;"
      "Grenoble Foot 38 Féminines|GF38|54|0055A4|FFFFFF|Stade des Alpes|Grenoble Foot 38;AS Nancy Lorraine Féminines|ASNL|53|E2001A|FFFFFF|Stade Marcel-Picot|AS Nancy Lorraine;"
      "US Orléans Féminines|USO|52|FFE500|E2001A|Stade de la Source|US Orléans LF;Stade Brestois Féminines|SB29|53|E2001A|FFFFFF|Stade Francis-Le Blé|Stade Brestois;"
      "Stade Rennais Féminines|SRFC|55|E2001A|000000|Roazhon Park|Stade Rennais;AJ Auxerre Féminines|AJA|53|FFFFFF|0055A4|Stade de l'Abbé-Deschamps|AJ Auxerre" },
    { "ENG", 1, "Women's Super League", 1,
      "Chelsea FC Women|CHE|88|034694|FFFFFF|Kingsmeadow|Chelsea;Arsenal Women|ARS|87|EF0107|FFFFFF|Meadow Park|Arsenal;Manchester City Women|MCI|84|6CABDD|FFFFFF|Joie Stadium|Manchester City;"
      "Manchester United Women|MUN|80|DA291C|FFFFFF|Leigh Sports Village|Manchester United;Liverpool FC Women|LIV|74|C8102E|FFFFFF|St Helens Stadium|Liverpool;Aston Villa Women|AVL|72|670E36|95BFE5|Villa Park|Aston Villa;"
      "Tottenham Hotspur Women|TOT|74|FFFFFF|132257|Brisbane Road|Tottenham Hotspur;Brighton Women|BHA|73|0057B8|FFFFFF|Broadfield Stadium|Brighton;Everton Women|EVE|70|003399|FFFFFF|Walton Hall Park|Everton;"
      "West Ham United Women|WHU|70|7A263A|1BB1E7|Chigwell Construction Stadium|West Ham United;Leicester City Women|LEI|68|003090|FDBE11|King Power Stadium|Leicester City;London City Lionesses|LCL|72|000000|FFFFFF|Hayes Lane" },
    { "ENG", 2, "Women's Super League 2", 0,
      "Birmingham City Women|BIR|62|0000FF|FFFFFF|St Andrew's|Birmingham City;Bristol City Women|BRC|63|E21B23|FFFFFF|Ashton Gate|Bristol City;Crystal Palace Women|CRY|64|1B458F|C4122E|VBS Community Stadium|Crystal Palace;"
      "Durham Women|DUR|58|0055A4|FFFFFF|Maiden Castle;Newcastle United Women|NEW|60|FFFFFF|241F20|Kingston Park|Newcastle United;Southampton Women|SOU|60|D71920|FFFFFF|St Mary's Stadium|Southampton;"
      "Sheffield United Women|SHU|56|EE2737|FFFFFF|Bramall Lane|Sheffield United;Sunderland Women|SUN|57|EB172B|FFFFFF|Eppleton Colliery Welfare|Sunderland;Charlton Athletic Women|CHA|59|D4021D|FFFFFF|The Valley|Charlton Athletic;"
      "Ipswich Town Women|IPS|56|0033A0|FFFFFF|Portman Road|Ipswich Town;Nottingham Forest Women|NFO|55|DD0000|FFFFFF|City Ground|Nottingham Forest;Portsmouth Women|POR|54|001489|FFFFFF|Fratton Park|Portsmouth" },
    { "ESP", 1, "Liga F", 2,
      "FC Barcelona Femení|FCB|92|A50044|004D98|Estadi Johan Cruyff|FC Barcelone;Real Madrid Femenino|RMA|83|FFFFFF|FEBE10|Estadio Alfredo Di Stéfano|Real Madrid;"
      "Atlético de Madrid Femenino|ATM|76|CB3524|FFFFFF|Centro Deportivo Alcalá de Henares|Atlético Madrid;Real Sociedad Femenino|RSO|72|0067B1|FFFFFF|Zubieta|Real Sociedad;"
      "Athletic Club Femenino|ATH|70|EE2523|FFFFFF|Lezama|Athletic Bilbao;Sevilla FC Femenino|SEV|68|FFFFFF|D71920|Estadio Jesús Navas|Séville FC;Real Betis Féminas|BET|64|00954C|FFFFFF|Ciudad Deportiva Luis del Sol|Real Betis;"
      "Levante UD Femenino|LEV|66|004D98|A50044|Ciutat Esportiva de Buñol|Levante;Madrid CFF|MCF|66|FFFFFF|E2001A|Estadio Fernando Torres;Granada CF Femenino|GRA|64|E2001A|FFFFFF|Ciudad Deportiva del Granada|Grenade;"
      "Deportivo Abanca|DEP|65|0055A4|FFFFFF|Abegondo|Deportivo La Corogne;RC Espanyol Femenino|ESP|62|0055A4|FFFFFF|Ciutat Esportiva Dani Jarque|Espanyol;SD Eibar Femenino|EIB|63|7B1E2B|0055A4|Unbe|Eibar;"
      "Badalona Women|BAD|60|E2001A|FFFFFF|Estadi Municipal de Badalona;Alhama CF|ALH|58|E2001A|FFFFFF|Complejo Deportivo Guadalentín;DUX Logroño|LOG|59|E2001A|FFFFFF|Las Gaunas" },
    { "GER", 1, "Frauen-Bundesliga", 2,
      "FC Bayern München Frauen|FCB|86|DC052D|FFFFFF|FC Bayern Campus|Bayern Munich;VfL Wolfsburg Frauen|WOB|82|65B32E|FFFFFF|AOK Stadion|VfL Wolfsburg;Eintracht Frankfurt Frauen|SGE|79|E1000F|000000|Stadion am Brentanobad|Eintracht Francfort;"
      "TSG Hoffenheim Frauen|TSG|74|1C63B7|FFFFFF|Dietmar-Hopp-Stadion|Hoffenheim;Bayer 04 Leverkusen Frauen|B04|72|E32221|000000|Ulrich-Haberland-Stadion|Bayer Leverkusen;SC Freiburg Frauen|SCF|70|000000|E2001A|Dreisamstadion|SC Fribourg;"
      "SV Werder Bremen Frauen|SVW|68|1D9053|FFFFFF|Weserstadion Platz 11|Werder Brême;1. FC Köln Frauen|KOE|66|FFFFFF|E2001A|Franz-Kremer-Stadion|FC Cologne;SGS Essen|SGS|64|6A0572|FFFFFF|Stadion an der Hafenstraße;"
      "FC Carl Zeiss Jena Frauen|FCC|60|0055A4|FFFFFF|Ernst-Abbe-Sportfeld;Hamburger SV Frauen|HSV|62|0A3F86|FFFFFF|Volksparkstadion|Hamburger SV;1. FC Union Berlin Frauen|FCU|64|E2001A|FFFFFF|Stadion An der Alten Försterei|Union Berlin;"
      "RB Leipzig Frauen|RBL|66|FFFFFF|DD0741|Sportpark Leipzig|RB Leipzig;1. FC Nürnberg Frauen|FCN|60|7B1E2B|FFFFFF|Max-Morlock-Stadion|1. FC Nuremberg" },
    { "ITA", 1, "Serie A Femminile", 1,
      "Juventus Women|JUV|78|FFFFFF|000000|Juventus Center Vinovo|Juventus;AS Roma Femminile|ROM|80|8E1F2F|F0BC42|Stadio Tre Fontane|AS Rome;Inter Women|INT|74|0068A8|000000|Arena Civica|Inter Milan;"
      "AC Milan Women|MIL|70|FB090B|000000|Stadio Brianteo|AC Milan;ACF Fiorentina Femminile|FIO|72|5B2C83|FFFFFF|Viola Park|Fiorentina;Como Women|COM|64|0055A4|FFFFFF|Stadio Ferruccio|Côme;"
      "US Sassuolo Femminile|SAS|63|00843D|000000|Stadio Enzo Ricci|Sassuolo;SS Lazio Women|LAZ|63|87D8F7|FFFFFF|Stadio Mirko Fersini|Lazio;Napoli Women|NAP|60|12A0D7|FFFFFF|Stadio Arechi|Naples;"
      "Genoa Women|GEN|58|A50034|0A1F44|Centro Sportivo Gianluca Signorini|Genoa;Parma Women|PAR|58|FFE500|0055A4|Stadio Ennio Tardini|Parme;Ternana Women|TER|57|E2001A|00843D|Stadio Libero Liberati" },
    { "USA", 1, "NWSL", 0,
      "Orlando Pride|ORL|84|5B2C83|FFFFFF|Inter&Co Stadium;Washington Spirit|WAS|83|000000|E2001A|Audi Field;Kansas City Current|KCC|83|00B2A9|E2001A|CPKC Stadium;"
      "Portland Thorns|POR|80|E2001A|000000|Providence Park;NJ/NY Gotham FC|GOT|81|000000|87CEEB|Red Bull Arena;San Diego Wave|SDW|78|0055A4|F47920|Snapdragon Stadium;"
      "North Carolina Courage|NCC|77|0055A4|E2001A|WakeMed Soccer Park;Seattle Reign|SEA|76|1B2A63|FFE500|Lumen Field;Racing Louisville|LOU|74|5B2C83|FFFFFF|Lynn Family Stadium;"
      "Houston Dash|HOU|72|F47920|FFFFFF|Shell Energy Stadium;Chicago Stars|CHI|70|3FA9F5|E2001A|SeatGeek Stadium;Angel City FC|ACFC|75|000000|FF69B4|BMO Stadium;"
      "Bay FC|BAY|74|000000|0055A4|PayPal Park;Utah Royals|UTA|70|FFE500|0055A4|America First Field" },
    { "MEX", 1, "Liga MX Femenil", 0,
      "Club América Femenil|AME|75|FFE500|0055A4|Estadio Azteca|Club América;Tigres UANL Femenil|TIG|78|FFE500|0055A4|Estadio Universitario|Tigres UANL;CF Monterrey Femenil|MTY|77|0055A4|FFFFFF|Estadio BBVA|CF Monterrey;"
      "Chivas Femenil|GDL|70|E2001A|FFFFFF|Estadio Akron|Chivas Guadalajara;Pachuca Femenil|PAC|72|0055A4|FFFFFF|Estadio Hidalgo|Pachuca;Deportivo Toluca Femenil|TOL|66|E2001A|FFFFFF|Nemesio Díez|Deportivo Toluca;"
      "Cruz Azul Femenil|CAZ|64|0055A4|FFFFFF|Ciudad de los Deportes|Cruz Azul;Pumas Femenil|PUM|63|FFFFFF|0055A4|La Cantera|Pumas UNAM;Club León Femenil|LEO|60|00843D|FFFFFF|Estadio León|Club León;"
      "Santos Laguna Femenil|SAN|58|00843D|FFFFFF|Estadio Corona|Santos Laguna;Atlas Femenil|ATL|60|E2001A|000000|Estadio Jalisco|Atlas;Club Tijuana Femenil|TIJ|62|E2001A|000000|Estadio Caliente|Club Tijuana;"
      "Puebla Femenil|PUE|55|FFFFFF|0055A4|Estadio Cuauhtémoc|Puebla;Necaxa Femenil|NEC|56|E2001A|FFFFFF|Estadio Victoria|Necaxa;Querétaro Femenil|QRO|55|0055A4|000000|Estadio Corregidora|Querétaro;"
      "Mazatlán FC Femenil|MAZ|54|5B2C83|FFFFFF|Estadio El Encanto|Mazatlán FC;FC Juárez Femenil|JUA|57|000000|E2001A|Estadio Olímpico Benito Juárez|FC Juárez;Atlético San Luis Femenil|ASL|56|E2001A|FFFFFF|Estadio Alfonso Lastras|Atlético San Luis" },
    { "AUS", 1, "A-League Women", 0,
      "Melbourne City Women|MCY|72|6CABDD|FFFFFF|City Football Academy;Melbourne Victory Women|MVC|70|0A1F44|FFFFFF|Home of the Matildas;Sydney FC Women|SYD|70|3FA9F5|FFFFFF|Leichhardt Oval;"
      "Western Sydney Wanderers Women|WSW|64|E2001A|000000|Wanderers Football Park;Brisbane Roar Women|BRI|64|F47920|0A1F44|Perry Park;Adelaide United Women|ADE|63|E2001A|FFE500|ServiceFM Stadium;"
      "Central Coast Mariners Women|CCM|66|FFE500|0055A4|Industree Group Stadium;Newcastle Jets Women|NEW|62|FFE500|0055A4|No. 2 Sportsground;Perth Glory Women|PER|58|5B2C83|F47920|Sam Kerr Football Centre;"
      "Wellington Phoenix Women|WEL|60|FFE500|000000|Porirua Park;Canberra United|CAN|60|5B2C83|FFFFFF|McKellar Park;Western United Women|WUN|63|000000|00843D|Ironbark Fields" },
};
const int NUM_WOMEN_LEAGUES = (int)(sizeof(WOMEN_LEAGUES) / sizeof(WOMEN_LEAGUES[0]));

// principales joueuses (effectifs complétés automatiquement)
const WomenSquadDef WOMEN_SQUADS[] = {
    { "OL Lyonnes", "Christiane Endler|G|85|CHI|34;Wendie Renard|D|85|FRA|35;Selma Bacha|D|84|FRA|25;Ellie Carpenter|D|83|AUS|25;Lindsey Heaps|M|84|USA|31;Melchie Dumornay|M|87|HAI|22;"
                      "Damaris Egurrola|M|80|NED|26;Kadidiatou Diani|A|85|FRA|30;Ada Hegerberg|A|85|NOR|30;Tabitha Chawinga|A|84|MWI|29;Marie-Antoinette Katoto|A|85|FRA|27;Vicki Becho|A|78|FRA|22" },
    { "PSG Féminines", "Katarzyna Kiedrzynek|G|80|POL|34;Elisa De Almeida|D|80|FRA|27;Sakina Karchaoui|D|82|FRA|29;Thiniba Samoura|D|76|FRA|21;Jade Le Guilly|D|76|FRA|23;"
                        "Tara Elimbi Gilbert|D|72|FRA|18;Romée Leuchter|A|78|NED|24;Merveille Kanjinga|A|73|COD|21" },
    { "Paris FC Féminines", "Mylène Chavas|G|78|FRA|27;Julie Soyer|D|75|FRA|40;Daphnée Corboz|M|79|FRA|31;Clara Matéo|M|76|FRA|27;Kessya Bussy|A|77|FRA|24;Mathilde Bourdieu|M|74|FRA|26;"
                              "Deja Davis|A|72|USA|25;Lou Bogaert|D|72|FRA|20" },
    { "FC Barcelona Femení", "Cata Coll|G|86|ESP|24;Irene Paredes|D|85|ESP|34;Ona Batlle|D|86|ESP|26;Mapi León|D|85|ESP|30;Patri Guijarro|M|88|ESP|27;Aitana Bonmatí|M|92|ESP|27;"
                               "Alexia Putellas|M|89|ESP|31;Caroline Graham Hansen|A|89|NOR|30;Ewa Pajor|A|89|POL|28;Salma Paralluelo|A|86|ESP|21;Claudia Pina|A|85|ESP|24;Kika Nazareth|M|81|POR|22" },
    { "Real Madrid Femenino", "Misa Rodríguez|G|82|ESP|26;Olga Carmona|D|83|ESP|25;María Méndez|D|80|ESP|24;Linda Caicedo|A|86|COL|20;Caroline Weir|M|85|SCO|30;Alba Redondo|A|81|ESP|29;"
                                "Signe Bruun|A|80|DEN|27;Athenea del Castillo|A|81|ESP|25;Filippa Angeldahl|M|80|SWE|28;Maëlle Lakrar|D|81|FRA|25" },
    { "Chelsea FC Women", "Hannah Hampton|G|86|ENG|25;Lucy Bronze|D|85|ENG|34;Millie Bright|D|83|ENG|32;Niamh Charles|D|81|ENG|26;Erin Cuthbert|M|84|SCO|27;Sjoeke Nüsken|M|84|GER|25;"
                          "Lauren James|A|87|ENG|24;Mayra Ramírez|A|85|COL|26;Guro Reiten|A|84|NOR|31;Aggie Beever-Jones|A|80|ENG|22;Sandy Baltimore|A|80|FRA|26;Johanna Rytting Kaneryd|A|81|SWE|28" },
    { "Arsenal Women", "Daphne van Domselaar|G|84|NED|25;Leah Williamson|D|85|ENG|28;Katie McCabe|D|84|IRL|30;Steph Catley|D|82|AUS|31;Mariona Caldentey|M|88|ESP|29;Frida Maanum|M|83|NOR|26;"
                       "Kim Little|M|82|SCO|35;Alessia Russo|A|87|ENG|26;Beth Mead|A|84|ENG|30;Chloe Kelly|A|83|ENG|27;Stina Blackstenius|A|82|SWE|29;Caitlin Foord|A|83|AUS|31" },
    { "Manchester City Women", "Khiara Keating|G|80|ENG|21;Alex Greenwood|D|85|ENG|32;Yui Hasegawa|M|86|JPN|28;Jill Roord|M|83|NED|28;Lauren Hemp|A|85|ENG|25;Khadija Shaw|A|88|JAM|28;"
                               "Vivianne Miedema|A|85|NED|29;Aoba Fujino|A|80|JPN|21;Kerstin Casparij|D|80|NED|24;Mary Fowler|A|80|AUS|22" },
    { "Manchester United Women", "Phallon Tullis-Joyce|G|80|USA|28;Maya Le Tissier|D|81|ENG|23;Ella Toone|M|83|ENG|26;Elisabeth Terland|A|80|NOR|24;Melvine Malard|A|80|FRA|25;Grace Clinton|M|79|ENG|22" },
    { "FC Bayern München Frauen", "Maria Luisa Grohs|G|82|GER|24;Glódís Perla Viggósdóttir|D|85|ISL|30;Giulia Gwinn|D|85|GER|26;Magdalena Eriksson|D|84|SWE|32;Lena Oberdorf|M|87|GER|23;"
                                  "Georgia Stanway|M|85|ENG|26;Klara Bühl|A|86|GER|24;Lea Schüller|A|84|GER|27;Pernille Harder|A|85|DEN|32;Linda Dallmann|M|82|GER|31;Sydney Lohmann|M|81|GER|25" },
    { "VfL Wolfsburg Frauen", "Merle Frohms|G|82|GER|30;Kathrin Hendrich|D|81|GER|33;Svenja Huth|A|82|GER|34;Alexandra Popp|A|83|GER|34;Jule Brand|A|84|GER|23;Janina Minge|M|80|GER|26;Lineth Beerensteyn|A|81|NED|29" },
    { "AS Roma Femminile", "Camelia Ceasar|G|78|ROU|25;Elena Linari|D|79|ITA|31;Manuela Giugliano|M|85|ITA|28;Giada Greggi|M|79|ITA|25;Valentina Giacinti|A|82|ITA|31;Evelyne Viens|A|80|CAN|28" },
    { "Juventus Women", "Pauline Peyraud-Magnin|G|80|FRA|33;Cecilia Salvai|D|79|ITA|32;Cristiana Girelli|A|83|ITA|35;Barbara Bonansea|A|81|ITA|34;Arianna Caruso|M|80|ITA|26;Sofia Cantore|A|78|ITA|26" },
    { "Orlando Pride", "Anna Moorhouse|G|80|ENG|30;Emily Sams|D|80|USA|26;Kerry Abello|D|78|USA|25;Marta|A|84|BRA|39;Barbra Banda|A|87|ZAM|25;Angelina|M|80|BRA|25;Summer Yates|M|76|USA|25" },
    { "Washington Spirit", "Aubrey Kingsbury|G|81|USA|33;Tara McKeown|D|80|USA|26;Croix Bethune|M|80|USA|24;Trinity Rodman|A|86|USA|23;Ashley Hatch|A|80|USA|30;Hal Hershfelt|M|78|USA|23" },
    { "Kansas City Current", "Adrianna Franch|G|79|USA|34;Debinha|M|83|BRA|33;Temwa Chawinga|A|87|MWI|27;Bia Zaneratto|A|81|BRA|31;Michelle Cooper|A|78|USA|22;Lo'eau LaBonta|M|80|USA|32" },
    { "NJ/NY Gotham FC", "Ann-Katrin Berger|G|84|GER|34;Emily Sonnett|D|80|USA|31;Rose Lavelle|M|84|USA|30;Esther González|A|84|ESP|32;Jaedyn Shaw|A|81|USA|20;Tierna Davidson|D|81|USA|26" },
    { "Portland Thorns", "Bella Bixby|G|78|USA|30;Sam Coffey|M|82|USA|26;Olivia Moultrie|M|79|USA|20;Christine Sinclair|A|76|CAN|42;Jessie Fleming|M|80|CAN|27" },
    { "San Diego Wave", "Kailen Sheridan|G|80|CAN|30;Naomi Girma|D|86|USA|25;María Sánchez|A|78|MEX|29;Delphine Cascarino|A|81|FRA|28" },
    { "Club América Femenil", "Itzel González|G|70|MEX|29;Kiana Palacios|A|74|MEX|29;Scarlett Camberos|A|73|MEX|24;Sarah Luebbert|A|74|USA|26" },
    { "Tigres UANL Femenil", "Cecilia Santiago|G|73|MEX|30;Jenni Hermoso|M|82|ESP|35;Stephany Mayor|A|77|MEX|33;Lizbeth Ovalle|A|78|MEX|25;Jacqueline Ovalle|M|77|MEX|25" },
    { "CF Monterrey Femenil", "Pamela Tajonar|G|72|MEX|40;Christina Burkenroad|A|76|MEX|30;Rebeca Bernal|D|74|MEX|27;Daniela Solís|M|72|MEX|22" },
    { "Melbourne City Women", "Emily Shields|G|66|AUS|25;Holly McNamara|A|70|AUS|21;Leticia McKenna|D|66|AUS|22" },
    { "Sydney FC Women", "Jada Whyman|G|67|AUS|24;Natalie Tobin|D|66|AUS|30;Princess Ibini|A|68|AUS|25" },
};
const int NUM_WOMEN_SQUADS = (int)(sizeof(WOMEN_SQUADS) / sizeof(WOMEN_SQUADS[0]));

// sélections féminines : joueuses réelles (hors clubs de la base)
const WomenSquadDef WOMEN_NATIONS[] = {
    { "FRA", "Pauline Peyraud-Magnin|G|80|FRA|33;Constance Picaud|G|76|FRA|27;Wendie Renard|D|85|FRA|35;Griedge Mbock|D|82|FRA|30;Sakina Karchaoui|D|82|FRA|29;Selma Bacha|D|84|FRA|25;Elisa De Almeida|D|80|FRA|27;"
             "Maëlle Lakrar|D|81|FRA|25;Grace Geyoro|M|82|FRA|28;Oriane Jean-François|M|78|FRA|24;Sandie Toletti|M|80|FRA|30;Kenza Dali|M|79|FRA|34;Kadidiatou Diani|A|85|FRA|30;Marie-Antoinette Katoto|A|85|FRA|27;"
             "Delphine Cascarino|A|81|FRA|28;Sandy Baltimore|A|80|FRA|25;Clara Matéo|M|76|FRA|27;Vicki Becho|A|78|FRA|22" },
    { "ESP", "Cata Coll|G|86|ESP|24;Misa Rodríguez|G|82|ESP|26;Irene Paredes|D|85|ESP|34;Ona Batlle|D|86|ESP|26;Olga Carmona|D|83|ESP|25;Laia Aleixandri|D|82|ESP|25;Aitana Bonmatí|M|92|ESP|27;"
             "Alexia Putellas|M|89|ESP|31;Patri Guijarro|M|88|ESP|27;Mariona Caldentey|M|88|ESP|29;Salma Paralluelo|A|86|ESP|21;Claudia Pina|A|85|ESP|24;Esther González|A|84|ESP|32;Athenea del Castillo|A|81|ESP|25" },
    { "ENG", "Hannah Hampton|G|86|ENG|25;Mary Earps|G|84|ENG|32;Leah Williamson|D|85|ENG|28;Lucy Bronze|D|85|ENG|34;Alex Greenwood|D|85|ENG|32;Jess Carter|D|82|ENG|28;Georgia Stanway|M|85|ENG|26;"
             "Keira Walsh|M|85|ENG|28;Ella Toone|M|83|ENG|26;Alessia Russo|A|87|ENG|26;Lauren James|A|87|ENG|24;Lauren Hemp|A|85|ENG|25;Beth Mead|A|84|ENG|30;Chloe Kelly|A|83|ENG|27" },
    { "USA", "Alyssa Naeher|G|82|USA|37;Aubrey Kingsbury|G|81|USA|33;Naomi Girma|D|86|USA|25;Tierna Davidson|D|81|USA|26;Emily Fox|D|83|USA|27;Crystal Dunn|D|82|USA|33;Rose Lavelle|M|84|USA|30;"
             "Lindsey Heaps|M|84|USA|31;Sam Coffey|M|82|USA|26;Sophia Wilson|A|87|USA|25;Trinity Rodman|A|86|USA|23;Mallory Swanson|A|86|USA|27;Alyssa Thompson|A|80|USA|20" },
    { "GER", "Ann-Katrin Berger|G|84|GER|34;Merle Frohms|G|82|GER|30;Giulia Gwinn|D|85|GER|26;Kathrin Hendrich|D|81|GER|33;Lena Oberdorf|M|87|GER|23;Sjoeke Nüsken|M|84|GER|25;Linda Dallmann|M|82|GER|31;"
             "Klara Bühl|A|86|GER|24;Lea Schüller|A|84|GER|27;Jule Brand|A|84|GER|23;Alexandra Popp|A|83|GER|34" },
    { "BRA", "Lorena|G|78|BRA|28;Tarciane|D|79|BRA|22;Antônia|D|77|BRA|31;Angelina|M|80|BRA|25;Kerolin|A|82|BRA|25;Debinha|M|83|BRA|33;Marta|A|84|BRA|39;Ludmila|A|80|BRA|30;Gabi Portilho|A|79|BRA|30" },
    { "NED", "Daphne van Domselaar|G|84|NED|25;Dominique Janssen|D|82|NED|30;Jill Roord|M|83|NED|28;Damaris Egurrola|M|80|NED|26;Vivianne Miedema|A|85|NED|29;Lineth Beerensteyn|A|81|NED|29;Esmee Brugts|A|80|NED|22" },
    { "SWE", "Zećira Mušović|G|82|SWE|29;Magdalena Eriksson|D|84|SWE|32;Amanda Ilestedt|D|82|SWE|32;Filippa Angeldahl|M|80|SWE|28;Kosovare Asllani|M|81|SWE|36;Stina Blackstenius|A|82|SWE|29;Fridolina Rolfö|A|84|SWE|32" },
    { "NOR", "Ada Hegerberg|A|85|NOR|30;Caroline Graham Hansen|A|89|NOR|30;Guro Reiten|A|84|NOR|31;Frida Maanum|M|83|NOR|26;Elisabeth Terland|A|80|NOR|24" },
    { "CAN", "Kailen Sheridan|G|80|CAN|30;Kadeisha Buchanan|D|83|CAN|30;Ashley Lawrence|D|82|CAN|30;Jessie Fleming|M|80|CAN|27;Evelyne Viens|A|80|CAN|28;Cloé Lacasse|A|79|CAN|32" },
    { "AUS", "Mackenzie Arnold|G|82|AUS|31;Steph Catley|D|82|AUS|31;Ellie Carpenter|D|83|AUS|25;Caitlin Foord|A|83|AUS|31;Mary Fowler|A|80|AUS|22;Sam Kerr|A|85|AUS|32;Hayley Raso|A|79|AUS|30" },
    { "JPN", "Ayaka Yamashita|G|80|JPN|29;Moeka Minami|D|81|JPN|26;Yui Hasegawa|M|86|JPN|28;Fuka Nagano|M|80|JPN|26;Hinata Miyazawa|M|82|JPN|25;Aoba Fujino|A|80|JPN|21;Mina Tanaka|A|81|JPN|31" },
    { "ITA", "Laura Giuliani|G|78|ITA|32;Elena Linari|D|79|ITA|31;Manuela Giugliano|M|85|ITA|28;Cristiana Girelli|A|83|ITA|35;Barbara Bonansea|A|81|ITA|34;Arianna Caruso|M|80|ITA|26" },
    { "COL", "Linda Caicedo|A|86|COL|20;Mayra Ramírez|A|85|COL|26;Leicy Santos|M|79|COL|29;Catalina Usme|A|78|COL|35" },
    { "CHI", "Christiane Endler|G|85|CHI|34" }, { "MWI", "Tabitha Chawinga|A|84|MWI|29;Temwa Chawinga|A|87|MWI|27" },
    { "ZAM", "Barbra Banda|A|87|ZAM|25;Racheal Kundananji|A|81|ZAM|25" }, { "JAM", "Khadija Shaw|A|88|JAM|28" }, { "HAI", "Melchie Dumornay|M|87|HAI|22" },
    { "NGA", "Asisat Oshoala|A|80|NGA|30;Rasheedat Ajibade|A|79|NGA|26;Chiamaka Nnadozie|G|80|NGA|24" }, { "MAR", "Ghizlane Chebbak|M|76|MAR|34;Ibtissam Jraidi|A|74|MAR|32" },
    { "POL", "Ewa Pajor|A|89|POL|28" }, { "DEN", "Pernille Harder|A|85|DEN|32;Signe Bruun|A|80|DEN|27" }, { "SCO", "Caroline Weir|M|85|SCO|30;Erin Cuthbert|M|84|SCO|27" },
    { "ISL", "Glódís Perla Viggósdóttir|D|85|ISL|30" }, { "IRL", "Katie McCabe|D|84|IRL|30" }, { "POR", "Kika Nazareth|M|81|POR|22;Jéssica Silva|A|79|POR|30" },
    { "MEX", "Lizbeth Ovalle|A|78|MEX|25;Kiana Palacios|A|74|MEX|29;Rebeca Bernal|D|74|MEX|27" },
};
const int NUM_WOMEN_NATIONS = (int)(sizeof(WOMEN_NATIONS) / sizeof(WOMEN_NATIONS[0]));

const char* womenCupName(const char* country) {
    static const char* T[][2] = {
        { "FRA", "Coupe de France féminine" }, { "ENG", "Women's FA Cup" }, { "ESP", "Copa de la Reina" }, { "GER", "DFB-Pokal der Frauen" },
        { "ITA", "Coppa Italia femminile" }, { "USA", "NWSL Challenge Cup" }, { "MEX", "Copa MX Femenil" }, { "AUS", "Australia Cup (F)" },
    };
    for (auto& e : T) if (!strcmp(e[0], country)) return e[1];
    return "Coupe nationale féminine";
}
