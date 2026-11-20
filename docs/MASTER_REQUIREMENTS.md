# HOME WATER CONTROLLER — MASTER 1.5 (REV61)

Datum: 14. 11. 2026  
Základ: uživatelem dodaný DEV KIT REV59, audit REV60 a následně výslovně potvrzená samostatná architektura bez vzájemné komunikace.

## 1. Závazná architektura

Projekt tvoří dvě samostatné jednotky ESP32. Mezi jednotkami nebude aplikační komunikace, synchronizace konfigurace, společný klíč, vzájemné povolování ani závislost provozu jedné jednotky na druhé.

- **ESP32 #1 – hlavní vodní hospodářství:** studna, dolní dešťová nádrž, všechny elektrické ventily umístěné u dolní nádrže, hlavní webové rozhraní, zadávání a řízení transferů studna → dolní a horní → dolní. Měří přenesené množství podle změny objemu dolní nádrže.
- **ESP32 #2 – horní nádrž:** samostatné měření hladiny, místní ochrana horního čerpadla podle minima a hystereze, objemová kalibrace horní nádrže a měření teploty. Má vlastní webové rozhraní.
- Každá jednotka zachovává své lokální bezpečnostní funkce nezávisle na druhé jednotce, domácí síti a internetu.
- Obě jednotky mají samostatný přístup do domácí sítě/internetu. Od REV85 používá ESP32 #1 W5500 Ethernet s DHCP a současně zachovává Wi-Fi STA + konfigurační AP. ESP32 #2 používá W5500 Ethernet a servisní AP. Vzdálený přístup přednostně přes VPN; ESP32 se přímo nevystavují veřejnému internetu.
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
2. Oddělit horní transfer v ESP32 #1 od peer telemetrie a vzdálených horních limitů; zachovat místní podmínky dolní nádrže a ventilů.
3. Doplnit a otestovat nulový/přerušený přítok podle dolní nádrže. Prahy nastavit až po měření skutečného hardware.
4. Doplnit horní objemovou kalibraci v ESP32 #2: absolutní kotevní bod, ruční zadání skutečně odebraných litrů, interpolace, NVS a webové zobrazení.
5. Doplnit DS18B20 a webové zobrazení teploty.
6. Odpojit a odstranit peer UDP a bezpečnostní moduly po kontrole referencí.
7. Ověřit samostatný LAN/internet přístup obou jednotek, Arduino build obou sketchů a postupné fyzické testy.

## 10. Stav ověření a pravidla označování

Každá funkce musí být označena jedním ze stavů: požadovaná, implementovaná, hostitelsky testovaná, přeložená cílovým Arduino toolchainem, testovaná na hardware, nebo neověřená. Tyto stavy se nesmějí zaměňovat.

Výchozí REV59 podle auditu REV60: nativní C++ 72/72 PASS a Python 17/17 PASS; Arduino cílový překlad a fyzické testy nebyly provedeny. Tyto výsledky neprokazují správnost nové samostatné architektury ani nových funkcí.


## Aktualizace REV85 — síť ESP32 #1

ESP32 #1 nově inicializuje W5500 přes SPI (SCK18/MISO19/MOSI23/CS27/RESET26), získává adresu přes DHCP a obnovuje spojení po výpadku linky nebo IP. Wi-Fi STA a konfigurační AP zůstávají zachovány. V REV85 je produkční ovládací web stále pouze na Wi-Fi STA/AP; HTTP přes W5500 se nepovažuje za implementované.
