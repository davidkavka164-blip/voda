# HOME WATER CONTROLLER — MASTER 2.4 (REV70)

Datum: 15. 11. 2026  
Základ: uživatelem dodaný DEV KIT REV68 a přesně ověřený archiv REV69. REV70 zachovává nezávislou architekturu a provádí audit DS18B20/W5500 bez neověřené změny síťového stacku.

## 0. Stav balíčku REV70

REV70 zpřesňuje startovní diagnostiku DS18B20 (už neuvádí, že čidlo není čteno), odstraňuje redundantní druhé `begin()` sběrnice a rozšiřuje host test o `millis()` rollover a zachování času posledního platného vzorku při chybě čtení. Servisní web ESP32 #2 zobrazuje DHCP stav a IP adresu W5500, pokud je lease přidělena; výslovně upozorňuje, že samotné DHCP neznamená HTTP dostupnost přes LAN. Přidán audit variant W5500 web serveru a statická kontrola dashboardu.

- Kompletní nativní sada: 77/77 testovacích programů PASS a všechny statické kontroly PASS.
- Python sada: 17/17 PASS.
- ASan/UBSan: 75/75 testovacích programů PASS a všechny statické kontroly PASS. ASan skript záměrně spouští menší podmnožinu než standardní sada.
- Arduino cílový překlad a fyzické testy nebyly provedeny.
- Horní objemová kalibrace z REV67 se funkčně nemění.
- DS18B20 z REV69 zůstává pouze informativní; REV70 rozšiřuje testy a zpřesňuje diagnostiku, ale nepotvrzuje fyzické zapojení ani cílovou kompatibilitu knihoven.
- Web ESP32 #2 je stále dostupný pouze přes servisní AP; W5500 DHCP/IP je nyní zobrazeno v dashboardu, ale HTTP přes W5500/LAN není implementováno.

Autoritativní souhrn implementace: `docs/IMPLEMENTATION_STATUS.md`. Podrobný audit W5500: `docs/W5500_WEB_ACCESS_AUDIT_REV70.md`. Historické záznamy předchozích revizí zůstávají zachovány.

## 1. Závazná architektura

Projekt tvoří dvě samostatné jednotky ESP32. Mezi jednotkami nebude aplikační komunikace, synchronizace konfigurace, společný klíč, vzájemné povolování ani závislost provozu jedné jednotky na druhé.

- **ESP32 #1 – hlavní vodní hospodářství:** studna, dolní dešťová nádrž, všechny elektrické ventily umístěné u dolní nádrže, hlavní webové rozhraní, zadávání a řízení transferů studna → dolní a horní → dolní. Měří přenesené množství podle změny objemu dolní nádrže.
- **ESP32 #2 – horní nádrž:** samostatné měření hladiny, místní ochrana horního čerpadla podle minima a hystereze, objemová kalibrace horní nádrže a měření teploty. Má vlastní webové rozhraní.
- Každá jednotka zachovává své lokální bezpečnostní funkce nezávisle na druhé jednotce, domácí síti a internetu.
- Obě jednotky mají samostatný přístup do domácí sítě/internetu. ESP32 #1 používá Wi-Fi STA. ESP32 #2 může zachovat W5500 Ethernet do routeru/switche a servisní AP. Vzdálený přístup přednostně přes VPN; ESP32 se přímo nevystavují veřejnému internetu.
- Žádné piny, elektrické zapojení ani hardware se nesmějí domýšlet. Změny pinů vyžadují doloženou kontrolu konfliktů.

## 2. Čerpadla a bezpečnostní výstupy

- Čerpadla běžně spíná jejich vlastní tlaková automatika. ESP32 není běžný regulátor motorů; ovládá pouze nezávislé bezpečnostní povolení/odpojení.
- Potvrzené bezpečnostní relé: Drátek 24 V DC / 30 A HIGH–LOW na DIN lištu, 24V napájení, jumper HIGH; GPIO HIGH = povolení napájení motoru, GPIO LOW = přerušení.
- ESP32 #1 GPIO25 = bezpečnostní relé studničního čerpadla; GPIO32 = bezpečnostní relé čerpadla dolní nádrže.
- ESP32 #2 GPIO25 = bezpečnostní relé čerpadla horní nádrže.
- Pumpová GPIO musí být nastavena do bezpečného LOW před síťovou/NVS inicializací.
- Studna: oddělit minimální hladinu pro transfer od kritické hladiny pro ochranu čerpadla. Kritická ochrana má hysterezi a obnovovací hranici.
- Horní nádrž: ESP32 #2 samostatně hlídá nastavitelné minimum a hysterezi/obnovovací hladinu. Ochrana nesmí záviset na ESP32 #1 ani na síti.
- Nouzový režim každého čerpadla je samostatná vědomá akce uživatele s fyzickým ověřením hladiny. Překonává softwarové blokace hladinové automatiky, chybnou konfiguraci a chybu/nedostupnost NVS, ale neobchází fyzickou připravenost výstupu ani nezávislé elektrické/mechanické ochrany motoru. Je pouze v RAM a po restartu se zruší.
- Žádná změna tlakových automatik.

## 3. Ventily a transfery

- Povolené transfery: studna → dolní nádrž a horní nádrž → dolní nádrž. Oba se zadávají v litrech přes web ESP32 #1.
- Žádný automatický transfer na základě nízké hladiny dolní nádrže.
- ESP32 #1 řídí oba transferové ventily. Ventily bez napětí = CLOSE. Nikdy nesmí být současně otevřeny obě cesty; změna cesty používá break-before-make.
- Čerpadla nespíná ESP32 #1; běžné spuštění zajišťuje tlaková automatika. ESP32 #2 samostatně odebere povolení hornímu čerpadlu při dosažení svého minima nebo poruše místního měření.
- ESP32 #1 pro horní transfer nepotřebuje a nesmí vyžadovat hladinu horní nádrže, peer telemetrii, vzdálený stav ochrany ani potvrzení ESP32 #2. Horní čerpadlo chrání ESP32 #2 lokálně.
- Před startem ESP32 #1 ověří vlastní čerstvé/platné měření dolní nádrže, požadované litry, volnou kapacitu, připravenost ventilového výstupu a stav transferového žurnálu. U transferu ze studny navíc ověří místní měření a bezpečnostní limity studny.
- Požadované množství se měří přírůstkem objemu dolní nádrže. Timeout je pouze záložní pojistka, nikoli primární měření.
- Při STOP, chybě měření, dosažení cíle, dosažení maxima dolní nádrže, timeoutu nebo poruše se výstup nastaví na CLOSE_ALL. Výsledek a důvod ukončení se uloží. Po restartu se transfer automaticky neobnoví.

### 3.1 Ochrana horního → dolního transferu proti nulovému/přerušenému přítoku

Tento požadavek uživatel výslovně potvrdil.

- ESP32 #1 sleduje přírůstek hladiny/objemu dolní nádrže jako nepřímý důkaz přítoku z horní nádrže; další průtokoměr se v první verzi nepřidává.
- Po otevření ventilu musí po omezeném rozběhovém okně nastat měřitelný kladný přírůstek nad šum a rozlišení čidla.
- Pokud po rozběhovém okně přírůstek nenastane, transfer se ukončí jako nulový přítok. První přírůstek pozorovaný až po překročení rozběhového okna se nepřijme jako opožděný úspěch; vzorek přesně na hranici je ještě přípustný.
- Během transferu se sleduje další pokrok. Pokud se po stanovenou dobu neobjeví dostatečný přírůstek, transfer se ukončí jako přerušený/nedostatečný přítok.
- Neplatné nebo zastaralé měření dolní nádrže znamená okamžité bezpečné ukončení.
- Časové a objemové prahy se nesmějí vymyslet. Určí se podle skutečného intervalu vzorkování, šumu snímače, rozlišení měření a reálného průtoku při fyzické zkoušce. Do té doby zůstává tato ochrana požadovaná, ale neověřená a horní transfer se nesmí označit za připravený k běžnému provozu.
- Kontrola přírůstku nedokazuje mechanickou polohu ventilu. Bez koncového spínače je stav ventilu pouze stav povelu.
- Souběžný odběr z dolní nádrže může maskovat přírůstek a zkreslit množství. V první verzi se souběžný odběr nekompenzuje; během přesného transferu a kalibrace se z dolní nádrže nemá odebírat.

### 3.2 Uživatelsky nastavitelné parametry watchdogu (potvrzeno 14. 11. 2026)

Uživatel výslovně požaduje, aby časové limity ochrany nulového/přerušeného přítoku byly nastavitelné a mohl je doladit při skutečném provozu.

- `startupGraceMs` — čas od otevření transferového ventilu, během kterého se očekává první měřitelný přírůstek dolní nádrže. Zobrazení a zadávání v sekundách.
- `noProgressTimeoutMs` — maximální doba bez dalšího dostatečného přírůstku po potvrzení přítoku. Zobrazení a zadávání v sekundách.
- Obě hodnoty budou nastavitelné z webového rozhraní ESP32 #1, uloženy do NVS a načteny po restartu.
- Ukládání musí mít kontrolu rozsahu, kontrolní součet/verzi záznamu a ověření zpětným načtením. Neplatná nebo poškozená konfigurace nesmí být tiše nahrazena hodnotou, která by povolila transfer; horní → dolní transfer zůstane blokován, dokud uživatel konfiguraci neopraví.
- Změna nastavení během probíhajícího transferu se projeví až při následujícím transferu. Aktivní watchdog používá neměnnou kopii parametrů pořízenou při startu transferu.
- Web musí zobrazovat aktuálně uložené hodnoty a jasně uvést, že jde o dobu pro detekci nulového/přerušeného přítoku, nikoli o celkový timeout transferu. Celkový timeout požadavku zůstává samostatnou pojistkou.
- Pro první uvedení do provozu budou připraveny výchozí hodnoty, ale budou výslovně označené jako počáteční a uživatel je doladí při měření. Výchozí hodnoty se nesmějí vydávat za ověřené provozní nastavení.
- Rozsahy vstupů budou omezené a validované; konkrétní limity a výchozí hodnoty musí být uvedeny v implementaci a testech. Nesmí být možné uložit nulu, zápornou hodnotu, přetečení převodu ani nesmyslně dlouhý interval.
- `minimumProgressLiters` zůstává samostatným prahem pro odlišení skutečného přírůstku od šumu. Jeho hodnotu je nutné stanovit podle rozlišení a šumu dolního snímače; nesmí se zaměnit s časovými limity.

### 3.3 Stav implementace watchdogu (REV66)

- `LowerTankInflowWatchdog` je integrován do `TransferLogic` pro horní → dolní transfer. První pozorovaný přírůstek po překročení startup grace se odmítne; vzorek přesně na limitu je ještě přijatelný.
- Dva časové limity jsou nastavitelné přes web ESP32 #1, ukládané do NVS, kontrolované a zpětně načítané. Změna během aktivního transferu je odmítnuta.
- Horní transfer nyní nevyžaduje vzdálenou hladinu, peer telemetrii, peer boot epoch ani vzdálené povolení ventilu. `TransferLogic` už nemá vstupní pole `peerDataValid` ani `upperTransferValvePermitted`.
- ESP32 #1 používá pro horní transfer pouze validitu/hladinu dolní nádrže, připravenost vlastního ventilového výstupu, NVS nastavení watchdogu, transferový žurnál a objemový cíl. ESP32 #2 chrání horní čerpadlo výhradně místně.
- ESP32 #1 již nemá produkční UDP peer receiver ani vlastní kopii horních bezpečnostních limitů. ESP32 #2 již nevysílá peer UDP status; W5500 je ponechán pro samostatnou síťovou cestu.
- Časové limity jsou výchozí 30 s / 30 s a jsou uživatelsky nastavitelné v rozsahu 1–3600 s. Jsou počáteční, ne fyzicky ověřené.
- Minimální přírůstek 12,32 l zůstává provizorní deadband odpovídající 0,2 cm při ploše dolní nádrže 6,16 m². Musí být ověřen podle skutečného šumu/rozlišení čidla.
- Horní transfer není ještě připraven pro běžný provoz: Arduino cílový build a fyzické zkoušky nebyly provedeny. Je nutné ověřit také, že horní pump safety relay ESP32 #2 spolehlivě odebere povolení při nízké hladině a že zavření ventilu skutečně ukončí přítok.
- Historické peer knihovny a jejich hostitelské testy zůstávají pouze v `firmware/common` a `tests` jako auditní stopa. Z obou Arduino `src/` stromů i produkčních `.ino` sketchů byly odstraněny.
- Přístup webového rozhraní ESP32 #2 přes W5500/LAN/internet není v této revizi prokázán; W5500 má DHCP a vlastní obnovu linky, ale lokální web stále běží přes servisní AP. Kandidátní knihovna `WebServer_ESP32_W5500` uvádí požadavek ESP32 core 2.0.6+, zatímco projekt musí zachovat kompatibilitu s dosud nepotvrzenou verzí core; bez zjištění přesné verze a ověření změny Ethernet stacku se knihovna nepřidává. Vzdálený přístup obou jednotek je samostatný nedokončený úkol.

## 4. Studna a dolní nádrž

### Studna
- Výchozí vnitřní průměr 80 cm a využitelná hloubka 800 cm; rozměry jsou uživatelsky nastavitelné.
- Základní geometrický válec je minimální objem, nikoli tvrzení o přesně známé zásobě. Skutečný objem může být vyšší kvůli dutinám a rozšířením.
- Objemová kalibrace smí výsledek pouze zvýšit nad geometrické minimum; nikdy jej nesmí snížit. Interpolace mezi body; mimo rozsah označit odhad. Bez absolutního kotevního bodu je zásoba neověřená.
- Chybějící/neprovedená objemová kalibrace neblokuje základní provoz, pokud je platná elektrická kalibrace snímače a samostatné bezpečnostní meze.
- Běžný transfer nemění kalibrační křivku. Body se mění pouze v uživatelem aktivovaném kalibračním režimu.

### Dolní nádrž
- Vnitřní půdorys 220 × 280 cm, plocha 6,16 m².
- Geometrický rozsah 0–121 cm; 1 cm = 61,6 l; objem [l] = hladina [cm] × 61,6.
- Údaj 11 cm je pouze informativní hranice běžně využitelného vyčerpání; není prahem ochrany čerpadla ani transferu.
- Dolní nádrž je primárním měřidlem množství přeneseného při obou transferech.

## 5. Horní nádrž — objemová kalibrace a teplota

- ESP32 #2 zobrazuje hladinu i objem v litrech, ale objem označuje podle skutečné kvality kalibrace; nesmí předstírat přesnost.
- Elektrická kalibrace snímače (4–20 mA → výška cm) je oddělena od objemové kalibrace nádrže (výška cm → litry).
- Použije se uživatelsky editovatelná tabulka bodů výška [cm] → absolutní objem [l], interpolace mezi sousedními body a zřetelné označení odhadu mimo kalibrovaný rozsah. REV67 doplňuje lokální model, NVS úložiště, webové přidání/úpravu bodu a zobrazení tabulky; mimo rozsah se objem neextrapoluje.
- Kalibrační bod se přidá pouze v uživatelem aktivovaném kalibračním režimu.
- První bod musí být známý absolutní kotevní objem při aktuálně změřené hladině. Bez něj lze sledovat pouze relativní změny, nikoli důvěryhodnou absolutní zásobu.
- Před transferem uživatel na ESP32 #2 uloží výchozí hladinu a odpovídající absolutní objem (musí ležet v již kalibrovaném rozsahu). Po transferu zadá skutečně přečerpané litry z výsledku ESP32 #1. ESP32 #2 vytvoří nový bod: nový absolutní objem = uložený výchozí objem − skutečně odebrané litry. Výchozí reference se po úspěšném uložení spotřebuje; při chybě zápisu zůstává zachována.
- Mezi referenčním bodem a kalibračním transferem nesmí být nezapočítaný přítok/odběr. Přesnost závisí na přesnosti přírůstku dolní nádrže.
- Kalibrace se nesmí automaticky měnit při běžném transferu, pokud uživatel výslovně nepotvrdí kalibrační úkon. Každý bod vyžaduje fyzické potvrzení a monotónně rostoucí výšku i objem. První bod je absolutní kotva; jeden bod dovoluje pouze odečet přesně v této výšce, nikoli interpolaci.
- Teplota horní nádrže je informativní; sama neovládá čerpadla ani ventily. REV69 implementuje DS18B20 na předběžně rezervovaném GPIO4: přesně jedno zařízení rodiny 0x28, 12bitový asynchronní převod, vzorek každých 10 s, stale limit 15 s, nové hledání čidla po 30 s. Chyba, chybějící/nejednoznačné čidlo, neplatná hodnota nebo zastaralý vzorek se zobrazí jako nedostupné. Fyzické zapojení není ověřeno; požadovaný pull-up je přibližně 4,7 kΩ mezi DATA a 3,3 V. Knihovny OneWire a DallasTemperature musí být instalovány a ověřeny proti skutečné verzi ESP32 core.

## 6. Snímače, NVS a historie

- Předpokládané hladinové snímače 4–20 mA / 10 m; 150 Ω dává nominálně 0,6–3,0 V. Skutečné zapojení a elektrickou kalibraci ověřit.
- Elektrická kalibrace `SensorConfig` je oddělena od objemové kalibrace. Neplatné/neověřené elektrické nastavení znamená neplatné měření pro běžné bezpečnostní rozhodování.
- NVS záznamy musí být verzované, kontrolované a při zápisu ověřené zpětným načtením. Poškozený záznam se nesmí bez výslovné servisní obnovy automaticky přepsat.
- Denní přítok studny = součet kladných změn objemu; záporné změny se neodečítají. Uchovat 30denní historii. Neplatný čas se nesmí přiřadit falešnému datu.
- Transferový žurnál musí zabránit automatickému obnovení po restartu a uchovat výsledek/neúplný stav.

## 7. Síť, web a aktualizace

- Lokální web každé jednotky musí fungovat bez internetu.
- ESP32 #1 používá Wi-Fi STA; při neúspěchu startovního připojení může spustit vlastní servisní AP s webem. Přechodný výpadek za běhu vede k opakovanému připojování, nikoli k okamžitému přepnutí do AP.
- ESP32 #2 používá samostatné připojení do LAN/internetu (přednostně stávající W5500 Ethernet) a může zachovat servisní AP.
- Servisní AP není mostem do domácí sítě/internetu.
- OTA přes webové Nastavení → Systém, zachovat NVS, nastavení a kalibrace. Aktualizace každé jednotky musí mít bezpečné chování a ověřený výsledek.
- Vzdálený přístup přednostně přes VPN. Router není podmínkou lokální funkčnosti.

## 8. Odstranění původní peer architektury

REV66 odstranilo peer UDP příjem z ESP32 #1, peer UDP vysílání z ESP32 #2, peer monitor a peer boot/freshness gate z provozních `.ino` sketchů. Z `TransferLogic` byla odstraněna runtime pole `peerDataValid` a `upperTransferValvePermitted`. ESP32 #1 již nepoužívá ani nekonfiguruje vlastní kopii horních limitů; horní bezpečnostní limity jsou výhradně lokální na ESP32 #2.

Historické peer moduly a jejich hostitelské testy zůstávají pouze ve `firmware/common` a `tests` jako auditní stopa. Z obou Arduino `firmware/ESP32_*/src` stromů a produkčních sketchů byly odstraněny. Nepřidávat zpět HMAC, handshake, společný klíč ani peer komunikaci.

## 9. Další pořadí implementace

1. Dokončit úklid nepoužívaných peer knihoven/testů a odstranit zbylé zastaralé texty/dokumentační odkazy.
2. Dokončit a otestovat editaci horní objemové tabulky, zejména opravu/výměnu bodů a chování při přerušení napájení během zápisu.
3. DS18B20 a webové zobrazení teploty jsou implementovány a hostitelsky/staticky testovány; ověřit instalaci OneWire/DallasTemperature a cílový překlad.
4. Vyřešit W5500/LAN webové rozhraní po ověření přesné verze Arduino-ESP32 core; zatím zůstává web ESP32 #2 pouze na servisním AP.
5. Přeložit oba sketch skutečným Arduino-ESP32 toolchainem odpovídajícím uživatelovu Arduino IDE 1.8.19 a přesné verzi ESP32 core.
6. Provést postupné fyzické testy senzorů, relé, ventilů, přítoku, nulového/přerušeného přítoku, restartů a ztráty sítě.

## 10. Stav ověření a pravidla označování

Každá funkce musí být označena jedním ze stavů: požadovaná, implementovaná, hostitelsky testovaná, přeložená cílovým Arduino toolchainem, testovaná na hardware, nebo neověřená. Tyto stavy se nesmějí zaměňovat.

REV70: standardní nativní sada 77/77 PASS; Python 17/17 PASS; statické integrační kontroly včetně DS18B20 a W5500 dashboard status PASS. ASan/UBSan 75/75 PASS; všechny statické kontroly PASS. Testy ověřují model kalibrace a NVS adapter, nikoli skutečnou přesnost čidla, objemu nádrže nebo fyzický postup přepouštění. Arduino cílový překlad a fyzické testy nebyly provedeny.


## 11. Změny REV67

- ESP32 #2 dostalo samostatný model objemové kalibrace horní nádrže s absolutní kotvou, kontrolním součtem, interpolací a stavem mimo kalibrovaný rozsah.
- Přidáno NVS úložiště `hwc_up_vol` s ověřením zpětného načtení a blokováním přepsání poškozeného záznamu.
- Web ESP32 #2 umožňuje zadat první absolutní bod, přidat/upravit další absolutní bod, uložit výchozí stav před transferem a po transferu ručně zadat skutečně odebrané litry z ESP32 #1.
- Přenosový kalibrační bod se vypočítá z uloženého výchozího absolutního objemu minus skutečně odebrané litry a aktuální hladiny. Reference se spotřebuje až po úspěšném uložení nového bodu.
- Objemová kalibrace je pouze informační; není zapojena do ochrany čerpadla, ventilů ani transferové logiky.
- Mimo kalibrovaný rozsah se objem neextrapoluje. Jeden kotevní bod umožňuje pouze odečet přesně v této výšce; pro interpolaci je potřeba nejméně dva body.
- W5500/LAN web nebyl v této revizi implementován. Prozkoumaná knihovna `WebServer_ESP32_W5500` vyžaduje ESP32 core 2.0.6+, a proto se bez potvrzení instalované verze core a posouzení změny síťového stacku nezavádí.
- Nativní testy kalibračního modelu a NVS: PASS. Arduino cílový překlad a fyzické testy zůstávají neprovedené.

## 12. Změny REV63

- Potvrzen požadavek uživatele na nastavitelnost obou časových limitů watchdogu z webového rozhraní ESP32 #1.
- Stanoveno ukládání do NVS, ověření zpětným načtením, validace rozsahu a blokování horního transferu při chybějící/neplatné konfiguraci.
- Změny nastavení se projeví až při dalším transferu; aktivní transfer používá kopii konfigurace načtenou při startu.
- Výslovně oddělen celkový timeout transferu od časů watchdogu a od minimálního objemového přírůstku.
- REV63 je aktualizace specifikace; provozní firmware zatím nebyl změněn.


## 13. Změny REV64–REV65

### REV64 — nastavitelné časové limity
- Web ESP32 #1 obsahuje chráněný formulář pro `startupGraceSeconds` a `noProgressTimeoutSeconds` v rozsahu 1–3600 s.
- Nastavení se ukládá do NVS jako verzovaný záznam s CRC a kontrolním načtením.
- Neuložená nebo poškozená konfigurace blokuje horní automatický transfer; uživatel musí nastavení výslovně uložit.
- Změna během aktivního transferu je odmítnuta.

### REV65 — integrace watchdogu do TransferLogic
- Horní → dolní transfer při vstupu do RUNNING zahájí `LowerTankInflowWatchdog` s výchozím objemem dolní nádrže a kopií nastavení načteného při startu požadavku.
- Není-li v nastaveném rozběhovém čase detekován přírůstek, transfer se přeruší. Po potvrzení přítoku se hlídá další přírůstek; při jeho výpadku se transfer také přeruší.
- Nový výsledek `TRANSFER_REASON_NO_INFLOW` se ukládá jako důvod přerušení; stavový automat vrací `VALVES_CLOSE_ALL`. Celkový timeout zůstává samostatný.
- Prozatímní `minimumProgressLiters = 12.32` odpovídá 0,2 cm při ploše dolní nádrže 6,16 m². Tato hodnota není fyzicky ověřena a musí se zkontrolovat podle šumu a rozlišení snímače.
- Hostitelské testy pokrývají nulový počáteční přítok, opožděný první přírůstek po limitu, přírůstek přesně na hranici, přerušení po potvrzeném přítoku, fail-closed neplatnou konfiguraci a zavření ventilového povelu.
- Horní transfer v `.ino` je stále blokován původní peer závislostí a `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED=false`. Samostatná architektura tedy není dokončena a transfer není připraven k provozu.
- Arduino cílový překlad a fyzické testy nebyly provedeny.

## 14. Změny REV66 — nezávislý provoz jednotek

- ESP32 #1: horní transfer nyní nevyžaduje vzdálenou hladinu ani peer status; před startem a během běhu ověřuje vlastní měření dolní nádrže, kapacitu, systémovou připravenost a watchdog.
- ESP32 #2: odstraněno peer UDP vysílání. Zachováno W5500 DHCP připojení a lokální obnova Ethernet linky; místní ochrana horního čerpadla zůstává beze změny.
- ESP32 #1: odstraněn peer UDP listener a horní limity přestaly být duplicitně spravovány na hlavní jednotce.
- Ruční servisní otevření horní cesty na ESP32 #1 kontroluje vlastní dolní nádrž a ventilový výstup; horní čerpadlo zůstává chráněno výhradně ESP32 #2. Nouzový ruční režim zůstává samostatný a vyžaduje fyzické ověření obsluhou.
- `TransferLogic` už nemá runtime pole `peerDataValid` a `upperTransferValvePermitted`; historické enum kódy důvodů ukončení zůstávají zachovány kvůli kompatibilitě uložené historie.
- Odstraněny staré peer závislosti z produkčních sketchů, nikoli ještě všechny historické peer moduly z archivu.
- Standardní C++ testy 74/74 PASS; ASan/UBSan 72/72 PASS; Python 17/17 PASS; všechny statické kontroly PASS. Test watchdogu navíc ověřuje pozdní první přítok a přesnou hranici startup grace.
- Nebyl proveden Arduino cílový build ani fyzické testy. W5500 webová dostupnost ESP32 #2 přes LAN/internet zůstává neověřená a je dalším síťovým úkolem.
