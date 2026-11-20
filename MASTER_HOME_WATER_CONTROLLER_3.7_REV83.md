# HOME WATER CONTROLLER — MASTER 3.7 (REV83)

Datum: 18. 11. 2026  
Základ: DEV KIT 1.00 REV79. REV78 zavádí transakční dvouslotový NVS žurnál horních limitů s ověřenou migrací původního klíče `state` a obnovou předchozí potvrzené konfigurace při přerušení zápisu. U horní objemové kalibrace zachovává ověřený kontrakt reference a výsledného bodu: po restartu se obnoví úplný platný záznam a rozpracovaná transferová reference se při neúspěšném dokončení nesmí tiše ztratit. Tento kontrakt neznamená, že všechna ostatní NVS úložiště používají dvouslotový zápis; rozdíly popisuje audit REV77.

## 0. Stav balíčku REV83

REV72 upravilo izolovaný testovací sketch W5500 + servisní AP. REV73 ochránilo rozpracovanou transferovou referenci horní objemové kalibrace před ruční editací. REV74 ověřuje úplný obsah kalibrace po NVS zápisu a při nejistém výsledku blokuje další změny do restartu a ověřeného načtení. REV75 přidává samostatný model přerušení napájení při ukládání reference a při ukládání výsledného kalibračního bodu. REV76 přidává autentizovaný servisní reset pouze poškozeného záznamu. REV77 dokončuje audit elektrické kalibrace a ochran čerpadel. REV78 převádí horní limity z jediného blobu na transakční dvouslotový žurnál.

- Standardní nativní sada: 78/78 testovacích programů PASS; všechny statické kontroly PASS.
- Python sada: 17/17 PASS.
- ASan/UBSan: 76/76 testovacích programů PASS; statické kontroly obsažené v ASan runneru PASS.
- Arduino cílový překlad a fyzické testy nebyly provedeny.
- REV83 dokončuje audit ukládání domácí Wi-Fi STA jednotky ESP32 #1. Zjištěno, že `ssid` a `password` jsou stále dva samostatné klíče; dvouslotový `CredentialStore` se týká pouze AP/admin údajů. Firmware se v REV83 nemění. Detail: `docs/WIFI_CREDENTIAL_STORAGE_AUDIT_REV83.md`.
- Produkční firmware ESP32 #2 se nemění. Web zůstává dostupný pouze přes servisní AP; W5500 HTTP je stále pouze izolovaný laboratorní sketch, nikoli produkční funkce.
- Test přerušení napájení je hostitelský model atomického chování NVS, nikoli fyzická brownout zkouška ESP32. Přesnou verzi Arduino-ESP32/ESP-IDF je nutné potvrdit při cílovém překladu a následně provést zkoušku na zařízení.

REV77 audit NVS: `docs/NVS_SENSOR_AND_PUMP_PROTECTION_RECOVERY_AUDIT_REV77.md`. REV78 žurnál horních limitů: `docs/NVS_UPPER_TANK_LIMITS_JOURNAL_REV78.md`. REV79 souhrnný audit: `docs/NVS_STORAGE_AUDIT_REV79.md`. REV80 změna read-back: `docs/RELEASE_NOTES_1.00_REV80.md`. Autoritativní souhrn implementace: `docs/IMPLEMENTATION_STATUS.md`. Postup stolního W5500 testu: `docs/W5500_DUAL_INTERFACE_BENCH_TEST_REV74.md`. Historické záznamy REV63–REV75 zůstávají zachovány.

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
- Před transferem uživatel na ESP32 #2 uloží výchozí hladinu a odpovídající absolutní objem (musí ležet v již kalibrovaném rozsahu). Po transferu zadá skutečně přečerpané litry z výsledku ESP32 #1. ESP32 #2 vytvoří nový bod: nový absolutní objem = uložený výchozí objem − skutečně odebrané litry. Výchozí reference se spotřebuje až po úspěšném uložení nového bodu. Pokud zápis nebyl potvrzen jako commit, předchozí reference zůstává zachována; pokud API ohlásí chybu až po dokončeném commitu, běhový stav se zablokuje a po restartu se načte úplný nový záznam. Po nejistém výsledku se kalibrační operace nesmí automaticky opakovat bez kontroly stavu po restartu.
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
4. Přeložit a fyzicky ověřit izolovaný W5500 + servisní AP diagnostický sketch REV72; teprve po úspěšném testu rozhodnout o přenosu Ethernet HTTP do produkčního ESP32 #2. Zatím zůstává web pouze na servisním AP.
5. Přeložit oba sketch skutečným Arduino-ESP32 toolchainem odpovídajícím uživatelovu Arduino IDE 1.8.19 a přesné verzi ESP32 core.
6. Provést postupné fyzické testy senzorů, relé, ventilů, přítoku, nulového/přerušeného přítoku, restartů a ztráty sítě.

## 10. Stav ověření a pravidla označování

Každá funkce musí být označena jedním ze stavů: požadovaná, implementovaná, hostitelsky testovaná, přeložená cílovým Arduino toolchainem, testovaná na hardware, nebo neověřená. Tyto stavy se nesmějí zaměňovat.

REV70: standardní nativní sada 77/77 PASS; Python 17/17 PASS; ASan/UBSan 75/75 PASS. REV71: standardní nativní sada 77/77 PASS a všechny statické kontroly standardního runneru PASS včetně izolace W5500 bench sketch; Python 17/17 PASS; ASan/UBSan 75/75 PASS a statické kontroly obsažené v ASan runneru PASS. Testy ověřují model kalibrace a NVS adapter, nikoli skutečnou přesnost čidla, objemu nádrže ani fyzickou dostupnost W5500 HTTP. Arduino cílový překlad a fyzické testy nebyly provedeny.


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

## 15. Změny REV71 — izolovaný test W5500 + servisní AP

- Produkční firmware ESP32 #2 zůstává záměrně beze změny. W5500 stále zajišťuje DHCP/link recovery a web zůstává dostupný přes servisní AP.
- Přidán samostatný diagnostický sketch `tools/W5500_DUAL_INTERFACE_DIAGNOSTIC/W5500_DUAL_INTERFACE_DIAGNOSTIC.ino`. Používá stávající `Ethernet.h` s `EthernetServer` pro W5500 a `WebServer` pro servisní AP; cílem je ověřit současnou dostupnost jednoduchého read-only GET `/` přes oba síťové přístupy.
- Diagnostický sketch neobsahuje řízení čerpadel/ventilů, NVS, přihlášení, POST, OTA ani zápis konfigurace. Je určen pouze pro samostatnou stolní zkoušku; heslo testovacího AP je nutné před použitím změnit a W5500 připojit jen do důvěryhodné LAN.
- Přidán dokument `docs/W5500_DUAL_INTERFACE_BENCH_TEST_REV71.md` s postupem, kontrolními body a omezeními. Přidána statická kontrola izolace testovacího sketch.
- Tento krok neprokazuje kompatibilitu cílového Arduino core ani fyzickou dostupnost HTTP. Sketch nebyl v tomto prostředí přeložen ani spuštěn. Bez fyzického testu se žádná Ethernet HTTP vrstva nepřenáší do produkčního `.ino`.
- Další rozhodovací brána: uživatel nebo cílové Arduino IDE musí zjistit přesnou verzi Arduino-ESP32 core a knihovny Ethernet; poté přeložit a fyzicky vyzkoušet samostatný sketch přes AP i W5500 včetně odpojení Ethernet kabelu.


## 16. Změny REV72 — neblokující W5500 diagnostika a spustitelné testy

- Ethernet HTTP příjem v izolovaném diagnostickém sketchi byl změněn na neblokující stavový parser. V jednom průchodu smyčkou zpracuje nejvýše 64 bajtů a nečeká synchronně na dokončení HTTP hlaviček.
- Request line má pevný buffer 96 bajtů; hlavičky mají pevný buffer 128 bajtů; přijímá se nejvýše 15 neprázdných hlaviček a celkový limit příjmu je 500 ms.
- Podporované jsou pouze `GET / HTTP/1.0` a `GET / HTTP/1.1`; ostatní požadavky, překročení limitů nebo nedokončené požadavky se odmítnou.
- Aktualizována statická kontrola parseru a návod k laboratornímu ověření.
- Opraven executable bit `tests/run_native_tests.sh`; přímé spuštění nyní funguje.
- Produkční ESP32 #2 firmware a síťová architektura nebyly změněny. Fyzické ověření W5500/AP a Arduino cílový překlad zůstávají nutné.


## 17. Změny REV73 — ochrana otevřené kalibrační reference

- Při uložené výchozí referenci před transferem je nyní ruční editace tabulky výšek/objemů zakázána. Tím se zabrání tomu, aby běžná ruční editace bodu nečekaně spotřebovala a smazala rozpracovanou transferovou referenci.
- Ochrana je provedena ve třech vrstvách: model `addUpperTankVolumeCalibrationPoint()` odmítne změnu, webový POST handler `/volume-point` vrátí HTTP 409 a webová stránka skryje formulář ruční editace s vysvětlením.
- Reference se může dokončit standardním zadáním skutečných litrů z ESP32 #1, nebo výslovně zrušit samostatným formulářem.
- `addUpperTankPostTransferPoint()` zůstává funkční: vytvoří interní kopii bez reference a uloží nový bod až po ověření zadaných litrů a aktuální hladiny.
- Nativní test ověřuje, že ruční editace během otevřené reference vrátí konflikt a referenci ani výstupní kalibraci nezmění.
- Modelový zdroj je synchronizován v `firmware/common` a obou Arduino `src` stromech.


## 18. Změny REV74 — ověření úplného NVS read-back

- NVS úložiště horní objemové kalibrace po zápisu zpětně načítá a porovnává celý významový obsah: absolutní kotvu, všechny aktivní kalibrační body a obě hodnoty rozpracované transferové reference.
- Pokud zápis nebo zpětné ověření selže, běhový stav kalibrace se zneplatní a další kalibrační operace zůstanou blokované do restartu a ověřeného načtení NVS.
- Tato ochrana se týká pouze informativní objemové kalibrace; nemění elektrickou kalibraci snímače, ochranu čerpadla ani řízení ventilů.

## 19. Změny REV75 — odolnost transferové reference při výpadku napájení

- Přidán samostatný hostitelský test `test_nvs_upper_tank_volume_calibration_powercut.cpp` s modelem atomického nahrazení NVS záznamu.
- Test ověřuje čtyři kritické situace: výpadek před uložením reference; restart po úspěšném uložení reference; výpadek při ukládání výsledného bodu, kdy musí zůstat zachována původní reference; a případ, kdy se nový záznam uloží, ale API ohlásí chybu, takže běžící firmware zůstane zablokovaný a po restartu načte úplný nový záznam.
- Výsledek potvrzuje správné chování aplikačního úložiště při starém-nebo-novém úplném záznamu. Nejde o simulaci elektrického brownoutu ani o ověření konkrétní verze ESP-IDF na skutečném ESP32.
- NVS v ESP-IDF dokumentuje ochranu proti náhlému výpadku napájení a atomické aktualizace. Konkrétní cílový core/ESP-IDF však v projektu dosud není zjištěn; proto zůstává nutná cílová zkouška s fyzickým přerušením napájení.
- Firmware a hardware nebyly změněny. Testy pokrývají pouze persistenci a obnovu informativní objemové kalibrace.
- Standardní nativní sada: 78/78 PASS; Python: 17/17 PASS; ASan/UBSan: 76/76 PASS; statické kontroly obou runnerů PASS.


## 13. REV77 — audit NVS elektrické kalibrace a ochran čerpadel

- Elektrická kalibrace snímačů je bezpečnostní brána; chybějící/neplatný/NVS nedostupný záznam zůstává diagnostický a nepovoluje pumpu ani transfer. Servisní obnova vyžaduje známé měřítko, nezávislou fyzickou referenci, autentizaci a nové čerstvé měření po zápisu.
- Dvouslotové store studny, dolního čerpadla a horní bezpečnostní západky odmítají načíst celý záznam, pokud je přítomný slot poškozený nebo jsou generace nejednoznačné. Neprovádí automatický rollback ani reset latch.
- Horní nastavitelné limity jsou stále jediný CRC blob. Po poškození se nepřepisují a horní čerpadlo zůstává blokované. Další doporučená práce je migrace na dvouslotový žurnál s bezpečným převodem starého záznamu.
- Žádný servisní reset bezpečnostních limitů ani latch se v REV77 nepřidává. Nouzový režim není nástrojem obnovy NVS.
- Podrobná matice a servisní postup: `docs/NVS_SENSOR_AND_PUMP_PROTECTION_RECOVERY_AUDIT_REV77.md`.


## 14. REV78 — transakční NVS žurnál horních limitů

- `NvsUpperTankLimitsStore` používá dva datové sloty `slot0/slot1`, potvrzovací ukazatel `head` a transakční záznam `txn`. Veřejné API zůstává beze změny.
- Pořadí zápisu: transakční marker → neaktivní slot → úplné zpětné ověření → potvrzovací `head` → odstranění markeru. Při neúplném kandidátu se obnoví předchozí potvrzený slot; úplný ověřený kandidát se po restartu dokončí.
- Původní `state` se validuje podle původního formátu a migruje do slotu 0. Původní klíč se nemaže; přerušená migrace se opakuje z validního starého záznamu.
- Poškozený aktivní slot nebo `head` znamená fail-closed. Poškozený `txn` lze ignorovat jen tehdy, když platný `head` a odpovídající aktivní slot potvrzují poslední commit; bez platného `head` je stav fail-closed. Poškozený neaktivní slot bez transakce neblokuje ověřený aktivní slot.
- Kritická západka horního čerpadla zůstává v odděleném úložišti. REV78 nemění její stav, piny, relé, limity ani transferovou logiku.
- Testy: nativní C++ 78/78 PASS; Python 17/17 PASS; ASan/UBSan 76/76 PASS; statické kontroly obou runnerů PASS. Power-cut test je hostitelský model, nikoli fyzická brownout zkouška.
- Arduino cílový překlad a fyzické testy zůstávají neprovedené. Podrobnosti: `docs/NVS_UPPER_TANK_LIMITS_JOURNAL_REV78.md`.

## REV79 — audit NVS úložišť

- Proveden souhrnný audit produkčních NVS úložišť ESP32 #1 a ESP32 #2 včetně strategie zápisu, read-back, obnovy a reakce při chybě.
- Potvrzeno oddělení bezpečnostních permitů od objemových kalibrací, denní statistiky a historie transferů.
- Elektrická kalibrace snímačů, limity a kritické západky čerpadel, watchdog přítoku a transferový restart journal mají při chybě blokovat příslušnou běžnou funkci; výchozí diagnostické hodnoty se nepovyšují na ověřenou konfiguraci.
- Jednoslotová kritická úložiště nemají záložní starší kopii, ale jejich chyba vede do fail-closed. Jejich případná migrace na transakční žurnál je odložena do zjištění cílové verze Arduino-ESP32/ESP-IDF a fyzického NVS power-cut ověření.
- REV79 nemění běhový firmware, piny, relé, ventily, prahy ani nouzové režimy. Podrobná matice: `docs/NVS_STORAGE_AUDIT_REV79.md`.
- Přidána statická kontrola `tests/check_nvs_storage_audit.sh`.
- Arduino cílový překlad a fyzické testy nebyly provedeny.


## REV79 — audit NVS úložišť

- Proveden souhrnný audit NVS úložišť ESP32 #1 a ESP32 #2; podrobná matice je v `docs/NVS_STORAGE_AUDIT_REV79.md`.
- Potvrzeno fail-closed chování bezpečnostních nastavení při chybě NVS a oddělení objemových/statistických údajů od pumpových permitů.
- Bez změny běhového firmware, GPIO, relé, ventilů, ochranných limitů a nouzových režimů.
- Přidána statická kontrola `tests/check_nvs_storage_audit.sh`.
- Nativní C++ 78/78 PASS; Python 17/17 PASS; ASan/UBSan 76/76 PASS.
- Arduino cílový překlad, fyzické NVS power-cut testy a hardware testy nebyly provedeny.


## REV80 — přesné ověření elektrické kalibrace po NVS zápisu

- `NvsSensorConfigStore::save()` a explicitní `recover()` nyní po zápisu zpětně porovnávají všechny významové položky `SensorConfig` včetně metadat, elektrických parametrů, kalibračního měřítka, offsetu a kontrolního součtu.
- Předchozí kontrola `save()` ověřovala validitu načteného záznamu a shodu CRC; REV80 zpřesňuje kontrakt na úplnou shodu obsahu s právě ukládaným kandidátem.
- Změna je shodně zavedena v `firmware/common` i v obou Arduino `src` stromech. Bez změny formátu NVS, klíčů, migrace, pinů, limitů, relé, ventilů a ochranné logiky.
- Nativní test rozšířen o ověření všech položek po zápisu. Standardní sada: 78/78 PASS; Python: 17/17 PASS; ASan/UBSan: 76/76 PASS; statické kontroly PASS.
- Arduino cílový překlad, skutečná NVS a fyzické power-cut testy nebyly provedeny.


## REV81 — přesná kontrola read-back historie a denní statistiky (18. 11. 2026)

- `NvsTransferHistoryStore::save()` nyní po zápisu porovnává kompletní obsah načteného `TransferHistorySnapshot` s požadovaným kandidátem: počet, index, rezervovaná pole a všech 12 položek historie včetně všech jejich polí. Samotná validita CRC již nestačí k potvrzení úspěšného uložení.
- `NvsDailyInflowStore::save()` nyní porovnává přímo všechny významové položky `DailyInflowSnapshot` po read-backu místo odvozeného porovnání kontrolních součtů.
- Přidán fault-injection test historie: NVS backend nahlásí plný úspěšný zápis, ale ponechá předchozí platný blob. `save()` musí vrátit chybu a předchozí platná historie musí zůstat načitatelná.
- Změna se týká pouze statistiky a historie; nejde o pumpové permity ani o transferový restart journal. Nemění se NVS formát, klíče, bezpečnostní prahy, GPIO, relé, ventily ani nouzové režimy.
- Zdroje NVS úložišť jsou synchronizovány v `firmware/common` a obou Arduino `src` stromech.
- Arduino cílový překlad, skutečná NVS a fyzický power-cut test nebyly provedeny.


## REV82 — úplná shoda read-back objemové kalibrace studny a geometrie (18. 11. 2026)

- `NvsWellCalibrationStore::save()` po zápisu porovnává všechna významová metadata kalibrace, příznak absolutní kotvy, rezervovaná pole, kontrolní součet a všechny aktivní kalibrační body. `mergeAndSave()` používá stejnou úplnou kontrolu před předáním výsledku volajícímu.
- `NvsWellGeometryStore::save()` po zápisu porovnává identifikaci/verzi, průměr, využitelnou hloubku a kontrolní součet načtené geometrie s ukládaným kandidátem.
- Přidán fault-injection test pro obě úložiště: backend oznámí plný zápis, ale ponechá předchozí platný blob. Uložení musí být odmítnuto a původní data zůstanou načitatelná.
- Porovnávají se významová pole; nepoužívá se `memcmp` přes struktury s možným paddingem. Neaktivní položky pole kalibrace nejsou považovány za významová data.
- Bez změny NVS formátu, klíčů, geometrických výchozích hodnot, pumpových permitů, ochranných prahů, GPIO, relé, ventilů a nouzových režimů. Objemová kalibrace nadále nesmí ovlivnit pumpovou ani transferovou ochranu.
- Zdroje `NvsWellCalibrationStore.cpp` a `NvsWellGeometryStore.cpp` jsou synchronizovány v `firmware/common` a obou ESP32 `src` stromech.
- Standardní hostitelská sada: 78/78 PASS; ASan/UBSan: 76/76 PASS; statické kontroly obou runnerů PASS. Python sada a syntax/compileall kontrola budou znovu provedeny před vydáním.
- Arduino cílový překlad, skutečná ESP32 NVS a fyzické power-cut/hardware testy nebyly provedeny.


## REV83 — audit domácí Wi-Fi STA persistence (18. 11. 2026)

- Ověřeno přímo v `ESP32_1.ino`, že domácí SSID a heslo se ukládají dvěma samostatnými `putString()` do namespace `hwc_wifi`; následný read-back neřeší přerušení napájení mezi zápisy.
- Opraveno rozlišení proti `CredentialStore`: jeho dvouslotové `c0_*` / `c1_*` a `cred_active` patří AP/admin údajům, nikoli domácí STA dvojici.
- Zpracován návrh bezpečné migrace na dvouslotové STA úložiště se zachováním starých klíčů až do potvrzení nového záznamu.
- REV83 je auditní revize bez změny firmware, NVS formátu, GPIO, relé, ventilů, ochran a nouzových režimů.
