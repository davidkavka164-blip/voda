# HOME WATER CONTROLLER — MASTER 1.8 (REV64)

Datum: 14. 11. 2026  
Základ: uživatelem dodaný DEV KIT REV59, samostatná architektura potvrzená uživatelem a implementační audit REV62.

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
- Pokud po rozběhovém okně přírůstek nenastane, transfer se ukončí jako nulový přítok.
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

### 3.3 Stav implementace watchdogu (REV64)

- Přenosný `LowerTankInflowWatchdog` je v projektu od REV62, zatím není připojen k `TransferLogic` ani k provoznímu řízení.
- REV64 přidává `NvsLowerTankInflowWatchdogStore`: verzovaný záznam, CRC, validaci rozsahu a ověření zpětným načtením.
- REV64 přidává do webu ESP32 #1 formulář `/inflow-watchdog-config` se dvěma časy v sekundách: čekání na první přírůstek a maximální dobu bez dalšího přírůstku.
- Povolený rozsah obou časů je 1–3600 sekund. Počáteční hodnoty jsou 30 s / 30 s; jsou pouze orientační a uživatel je musí doladit při provozu.
- Nastavení se načítá při startu a ukládá do NVS. Změna se přijímá jen mimo aktivní transfer a platí od dalšího transferu.
- Při chybě NVS nebo neplatném záznamu je na webu zobrazen stav chyby. Samotné nastavení však zatím není připojeno jako povinná podmínka `TransferLogic`, protože watchdog ještě není integrován.
- `minimumProgressLiters` zůstává samostatným prahem proti šumu a není součástí časového formuláře.
- Horní transfer v aktuálním `.ino` stále závisí na staré peer komunikaci. Cílová samostatná architektura tedy zatím není implementována.
- Do odstranění peer závislostí, integrace watchdogu a fyzických zkoušek se horní transfer nesmí označit za připravený pro běžný automatický provoz.

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
- Použije se uživatelsky editovatelná tabulka bodů výška [cm] → absolutní objem [l], interpolace mezi sousedními body a zřetelné označení odhadu mimo kalibrovaný rozsah.
- Kalibrační bod se přidá pouze v uživatelem aktivovaném kalibračním režimu.
- První bod musí být známý absolutní kotevní objem při aktuálně změřené hladině. Bez něj lze sledovat pouze relativní změny, nikoli důvěryhodnou absolutní zásobu.
- Po transferu horní → dolní uživatel převezme skutečně přečerpané litry z výsledku ESP32 #1 a ručně je zadá do kalibračního formuláře ESP32 #2. ESP32 #2 zaznamená aktuální hladinu a vytvoří nový bod: nový absolutní objem = předchozí známý objem − skutečně odebrané litry.
- Mezi referenčním bodem a kalibračním transferem nesmí být nezapočítaný přítok/odběr. Přesnost závisí na přesnosti přírůstku dolní nádrže.
- Kalibrace se nesmí automaticky měnit při běžném transferu, pokud uživatel výslovně nepotvrdí kalibrační úkon.
- Teplota horní nádrže je informativní; sama neovládá čerpadla ani ventily. Předpokládané čidlo DS18B20 na GPIO4; fyzické zapojení a knihovnu ověřit.

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

## 8. Co odstranit z původní peer architektury

Po kontrole referencí a regresních testech odstranit z provozních sketchů a následně z balíčku: UDP peer status broadcast/listener, `PeerStatusMonitor`, peer status rámce, handshake HELLO/CHALLENGE/FINISH/READY, session keys, peer command session, replay store/guard, peer secret vault, HMAC status codec, peer-key provisioning, peer boot/freshness gating, zrcadlené horní transferové limity na ESP32 #1 a všechny UI/diagnostické prvky, které předpokládají vzájemnou komunikaci.

Před smazáním každého modulu ověřit všechny reference. Obecné HMAC/SHA moduly odstranit pouze pokud nemají jiného konzumenta. Zachovat pumpové ochrany, webové přihlášení, OTA, senzorovou matematiku, NVS integritu, transferový žurnál a historii.

## 9. Pořadí implementace

1. Aktualizovat master a všechny aktuální stavové dokumenty podle této architektury.
2. **Hotovo v REV64:** NVS konfigurace a webová stránka pro nastavitelné časové limity; hostitelsky ověřit validaci, ukládání, načítání a obnovu poškozeného záznamu.
3. Oddělit horní transfer v ESP32 #1 od peer telemetrie a vzdálených horních limitů; zachovat místní podmínky dolní nádrže a ventilů.
4. Integrovat připravený `LowerTankInflowWatchdog` do transferu horní → dolní, doplnit testy návaznosti a prahy nastavit až po měření skutečného hardware.
5. Doplnit horní objemovou kalibraci v ESP32 #2: absolutní kotevní bod, ruční zadání skutečně odebraných litrů, interpolace, NVS a webové zobrazení.
6. Doplnit DS18B20 a webové zobrazení teploty.
7. Odpojit a odstranit peer UDP a bezpečnostní moduly po kontrole referencí.
8. Ověřit samostatný LAN/internet přístup obou jednotek, Arduino build obou sketchů a postupné fyzické testy.

## 10. Stav ověření a pravidla označování

Každá funkce musí být označena jedním ze stavů: požadovaná, implementovaná, hostitelsky testovaná, přeložená cílovým Arduino toolchainem, testovaná na hardware, nebo neověřená. Tyto stavy se nesmějí zaměňovat.

REV64: nativní C++ 74/74 PASS; ASan/UBSan 72/72 PASS; Python 17/17 PASS. Nový NVS store časových parametrů má hostitelské testy zápisu, načtení, kontroly integrity a opravy explicitním uložením. Statická kontrola potvrzuje připojení formuláře a NVS store do ESP32 #1. Watchdog stále není připojen k běžícímu transferu; horní transfer stále používá peer závislosti. Arduino cílový překlad a fyzické testy nebyly provedeny.


## 11. Změny REV63

- Potvrzen požadavek uživatele na nastavitelnost obou časových limitů watchdogu z webového rozhraní ESP32 #1.
- Stanoveno ukládání do NVS, ověření zpětným načtením, validace rozsahu a blokování horního transferu při chybějící/neplatné konfiguraci.
- Změny nastavení se projeví až při dalším transferu; aktivní transfer používá kopii konfigurace načtenou při startu.
- Výslovně oddělen celkový timeout transferu od časů watchdogu a od minimálního objemového přírůstku.
- REV63 je aktualizace specifikace; provozní firmware zatím nebyl změněn.


## 12. Změny REV64

- Implementován NVS store `NvsLowerTankInflowWatchdogStore` ve společné části a zrcadlen do obou sketchů.
- Záznam obsahuje verzi, kontrolní součet a dva časové parametry v sekundách; rozsah každého je 1–3600 s.
- ESP32 #1 načítá nastavení při startu a zobrazuje jeho stav na stránce automatických transferů.
- Přidán autentizovaný POST `/inflow-watchdog-config`; ukládání je odmítnuto během aktivního transferu. Po uložení se provádí kontrolní načtení.
- Výchozí hodnoty 30 s / 30 s jsou výslovně pouze orientační; nejsou prezentovány jako ověřené provozní limity.
- Doplněn nativní test store a statická integrační kontrola.
- Provozní watchdog zůstává nezapojený; horní transfer stále závisí na peer komunikaci. Nebyla provedena Arduino kompilace ani hardwarové testy.
