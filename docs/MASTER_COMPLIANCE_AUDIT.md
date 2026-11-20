> **Historický základní audit:** původní matice v tomto souboru byla vytvořena pro DEV KIT 0.60 dne 2026-11-02. Není samostatným aktuálním stavem revizí 0.66–0.69. Aktuální autoritativní porovnání s MASTER 1.3 je v `MASTER_ALIGNMENT_AUDIT_0.69.md`.

# Kontrola shody s HOME WATER CONTROLLER – MASTER 1.3

Datum kontroly: 2026-11-02  
Kontrolovaný firmware: DEV KIT 0.60 (zdrojový kód shodný s 0.59; 0.60 je dokumentační revize)  
Rozsah: statická kontrola obou sketchů, společných modulů, dokumentace a opakované spuštění nativních testů.

## Jak číst výsledky

- **Implementováno** – odpovídající kód je přítomen; samo o sobě to neznamená, že byl přeložen nebo vyzkoušen na ESP32.
- **Nativně testováno** – test proběhl na hostitelském počítači, nikoli na cílové desce.
- **Částečně** – existuje pouze část požadované funkce, simulace nebo samostatný modul.
- **Neověřeno** – chybí skutečný překlad Arduino-ESP32 nebo fyzický test.
- **Chybí** – požadovaná funkce není v aktuálním firmware dokončena.

## Souhrnná matice

| Požadavek Master 1.3 | Stav v 0.60 | Zjištění / omezení |
|---|---|---|
| Dvě ESP32 s rozdělením odpovědností (část 2) | Částečně | Existují dva samostatné Arduino sketch projekty. Fyzické nasazení a úplná spolupráce nejsou ověřeny. |
| Čerpadla řízená vlastní tlakovou automatikou; ESP32 zasahuje bezpečnostně (části 3–7) | Částečně | Logika ochrany studny a horní nádrže existuje a je nativně testována. Fyzické relé není inicializováno ani ovládáno; nouzové povolení proto zatím není provozní řízení čerpadla. |
| Povolené směry transferu a ruční požadavek v litrech (části 8–10) | Částečně | Webová SIMULACE nabízí studnu → dolní a horní → dolní. Nejde o skutečný transfer a automatické plnění není implementováno. |
| Dolní nádrž 220 × 280 cm, 61,6 l/cm, rozsah 11–121 cm (části 11–14) | Částečně | Výpočet objemu a ochrana fyzického rozsahu existují. Je nutné odlišit výpočet hladina × 61,6 od pracovního rozsahu 11–121 cm; reálné měření a hydraulická instalace nejsou ověřeny. |
| Kalibrační křivka studny, interpolace, lokální překalibrace (části 15–27) | Implementováno / nativně testováno | Moduly geometrie, křivky a editoru kalibrace existují; ukládání má testy se stubem Preferences. Skutečné NVS a kalibrace při reálném transferu nejsou ověřeny. |
| Transfer studna → dolní a ochrana zdroje (části 28–30) | Částečně | `TransferLogic` a ochranné limity existují. Web spouští pouze simulaci; ventil ani čerpadlo se fyzicky neovládají. |
| Horní nádrž Smart 7000, samostatná ochrana a transfer (části 31–35) | Částečně | Měření hladiny a logika lokální ochrany existují. Geometrie/objemová křivka horní nádrže není dokončena; lokální limity se nastavují samostatně na obou jednotkách a automatická synchronizace chybí. |
| Záznamy transferů a zdrojová měření (části 36–43) | Částečně | Historie ukládá výsledky SIMULACE. Není to historie skutečného průtoku. Kalendářní čas a záznam fyzických transferů nejsou dokončeny. |
| Snímače 4–20 mA, převod a oddělená kalibrace (části 44–47) | Implementováno / částečně ověřeno | Čtení ADC, matematika a konfigurace existují. Skutečný proudový snímač, 150 Ω shunt, ochrana vstupu, kabeláž a přesnost ADC nebyly ověřeny na deskách. |
| Hlavní web, grafika, transfery, kalibrace a nouzové ovládání (části 48–53) | Částečně | ESP32 #1 má dashboard, konfiguraci, kalibraci, SIMULACI a nouzové potvrzení. Kompletní provozní UI pro skutečné transfery není hotové. |
| Alarmy a komunikace ESP32 (části 54–56) | Částečně | UDP broadcast stavových zpráv ESP32 #2 → #1 existuje. Stavové rámce používají CRC, nikoli autentizaci. HMAC handshake koordinátory jsou samostatné moduly, ale nejsou napojeny na W5500/UDP. Řídicí příkazy a ACK/REJECTED/FAILED chybí. |
| Historie, NVS a zachování konfigurace (části 57–58) | Částečně | Několik NVS úložišť je verzovaných a chráněných kontrolním součtem/CRC; nativní testy pokrývají řadu chybových stavů. Skutečné NVS na deskách nebylo ověřeno. |
| Wi-Fi, AP a plné místní ovládání bez internetu (část 69) | Implementováno / neověřeno | ESP32 #1 spouští AP při chybějící/nefunkční Wi-Fi a registruje stejné hlavní webové cesty. Opakované připojování je v kódu. Chování na skutečném routeru a AP nebylo fyzicky ověřeno. ESP32 #2 má vlastní servisní AP/web pro lokální nastavení a bezpečnost. |
| Vzdálený přístup přes VPN (část 70) | Návrh | `REMOTE_ACCESS_PLAN.md` stanoví VPN na routeru a zákaz přímého vystavení ESP32. Archer AX12, jeho HW revize, firmware, WAN topologie a CGNAT nejsou ověřeny; VPN není nastavena. |
| Teplota horní nádrže (část 71) | Chybí | GPIO je pouze předběžně uvedeno. Není implementováno čtení DS18B20 ani jiného teplotního čidla; rámec správně posílá `temperatureValid = 0`. |
| OTA obou ESP32 (část 72) | Částečně | Upload handlery existují. Chybí skutečný Arduino build, ověření partition/rollbacku a fyzický test. Koordinovaná bezpečná aktualizace obou jednotek není dokončena. |
| Rozlišování implementace, testu, překladu a HW ověření (část 73) | Částečně | Dokumentace tyto stavy rozlišuje. Audit doplňuje další matici, aby se simulační a fyzické funkce nezaměňovaly. |

## Konkrétní zjištění při kontrole zdrojů

1. `ESP32_1.ino` přijímá stavové UDP rámce horní nádrže přes W5500. Dekódování a kontrola CRC/sekvence/stáří nejsou kryptografickým ověřením odesílatele. Použití je zatím pro stavový dashboard a SIMULACI; nesmí být považováno za bezpečný kanál řídicích povelů.
2. `ESP32_2.ino` vysílá `upperVolumeValid = 0` a `temperatureValid = 0`. Objem horní nádrže a teplota tedy nejsou aktuálně měřené hodnoty.
3. Výkonové GPIO pro čerpadla a ventily nejsou v hlavních sketchech inicializována ani spínána. To je záměrná bezpečnostní blokace; žádný skutečný transfer se nesmí očekávat.
4. Horní limity se ukládají v oddělených NVS obou jednotek. ESP32 #1 porovnává checksum přijatý od ESP32 #2, ale automatické bezpečné předání konfigurace není implementováno.
5. Generovaná hesla AP a webového administrátora se vypisují do Serial Monitoru. Je to bezpečnostní nedostatek pro ostrý provoz. Nelze výpis pouze odstranit bez náhrady, protože uživatel potřebuje bezpečně získat prvotní přístupové údaje. Nejprve je nutné navrhnout fyzický provisioning, obnovu/změnu hesel a ochranu uložených tajemství.
6. `PeerSecretVault` definuje a testuje fail-closed kontrakt, ale konkrétní ESP32 backend s odpovídající ochranou dlouhodobého klíče a fyzický provisioning nejsou dokončeny.
7. OTA ESP32 #1 a #2 mají upload handlery, ale to není důkaz bezpečného upgradu. Je třeba ověřit partition, velikost obrazu, autentizaci, stav ochrany během upgradu, restart, zachování NVS a obnovu při neúspěchu.
8. Společné moduly ve `firmware/common/` jsou v revizi 0.60 byte-identické s kopiemi v `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`.

## Opakované testy při auditu 2026-11-02

- `tests/run_native_tests.sh`: **43/43** testovacích programů prošlo.
- Stejná sada byla spuštěna s AddressSanitizer a UndefinedBehaviorSanitizer: **43/43** prošlo.
- Runner používá C++11, `-Wall -Wextra -Werror -pedantic`.
- Společné zdrojové soubory byly porovnány bajt po bajtu: obě kopie jsou shodné.
- ZIP archivu byla ověřena integrita.
- Skutečný Arduino-ESP32 build nebyl proveden; fyzické ESP32, W5500, senzory, relé, ventily, NVS a síť nebyly testovány.

## Priority další práce

### P0 – bezpečnostní blokátory před síťovým řízením
1. Navrhnout a otestovat provisioning společného tajemství, identitu jednotek, změnu/obnovu klíče a konkrétní chráněné úložiště. Prosté nechráněné Preferences/NVS nesmí být vydáváno za bezpečné úložiště klíče.
2. Navrhnout bezpečné prvotní nastavení, zobrazení a obnovu hesel AP a webu; odstranit tajemství z běžných Serial výpisů až po zajištění použitelné náhradní cesty.
3. Teprve po splnění bodu 1 integrovat HELLO/CHALLENGE/FINISH/READY přes W5500, navázat `PeerCommandSession`, persistentní anti-replay a COMMAND/RESPONSE.

### P1 – dokončení řídicího systému bez aktivace výkonových výstupů
4. Propojit autentizovanou obousměrnou komunikaci s bezpečnostními daty a přenosem horních limitů.
5. Dokončit aplikační propojení transferu s autentizovaným příkazem/ACK a bezpečným transportem. Přenositelný simulační automat již řeší předběžné kontroly, čerstvost měření, cíl, STOP, timeout a historii; fyzické řízení zůstává blokováno.
6. Dokončit model horní nádrže a teplotní měření až podle potvrzených fyzických parametrů a zvoleného čidla.
7. Dokončit hlavní webové rozhraní a diagnostiku; jasně oddělit SIMULACI od provozního transferu.

### P2 – ověření na cílové platformě a provozní síť
8. Přeložit oba sketch projekty v uživatelově Arduino IDE 1.8.19 s přesnou verzí ESP32 core a knihoven.
9. Ověřit senzory, ADC, W5500, Wi-Fi/AP, NVS, výpadky komunikace a boot/reset bez připojených výkonových zařízení.
10. Teprve po samostatné hardwarové bezpečnostní revizi navrhnout fyzické výstupy a jejich fail-safe zapojení.
11. Po ověření routeru Archer AX12 dokončit WireGuard VPN a vzdálený přístup. VPN nesmí být podmínkou místního řízení.

## Závěr

DEV KIT 0.60 má rozsáhlý a dobře testovaný základ přenositelné logiky. Není však shodný s dokončeným systémem požadovaným Master 1.3. Největší mezera není v matematických modulech, ale v bezpečném provisioning/uložení klíčů, skutečné obousměrné komunikaci, integraci transferového automatu, chybějícím měření horního objemu/teploty a v ověření na cílovém hardware. Výkonové výstupy musí zůstat neaktivní, dokud tyto blokátory a hardwarová bezpečnost nebudou samostatně uzavřeny.


## Dodatečná kontrola 2026-11-02 — revize 0.63

- Při následné revizi `PeerSecretVault` bylo zjištěno, že původní `provision()` spoléhal pouze na návratovou hodnotu backendového `write()` a neověřoval uloženou hodnotu.
- Oprava: po zápisu se klíč načte zpět a všech 32 bajtů se porovná konstantním časem. Při neúspěšném ověření se vault pokusí záznam zneplatnit a provisioning skončí chybou.
- Testy pokrývají odmítnutí write failure, chybnou read-back hodnotu a read-back failure. Celá standardní nativní sada: 43/43; nový vault test samostatně ASan/UBSan.
- Tato oprava posiluje přenositelný kontrakt, ale **neřeší** produkční backend, skutečnou trvalost zápisu, ochranu flash, provisioning ani síťovou integraci.


## Dodatečná kontrola 2026-11-02 — revize 0.65

- V transferové logice byla nalezena mezera v ochraně proti opakovanému ID: RAM cache se po restartu ztratila, přestože posledních 12 výsledků transferů je uloženo v NVS.
- Revize 0.65 obnovuje posledních 12 ID z persistentní historie a při pozdním obnovení NVS slučuje uložená ID s požadavky přijatými od startu.
- Oprava je nativně testována; 43/43 testů standardně i s ASan/UBSan.
- Tím se zlepšuje ochrana simulačního automatu proti opakovanému požadavku. Nejde o autentizaci síťového příkazu ani o náhradu persistentního anti-replay protokolu pro peer komunikaci.
- Skutečné transfery a výkonové GPIO zůstávají neimplementované; Arduino build a fyzické NVS nebyly ověřeny.

## Dodatečná kontrola 2026-11-02 — revize 0.66

- Přidán dvouslotový persistentní transfer journal (`NvsTransferJournalStore`) a přenositelný fail-closed automat (`TransferRestartGuard`).
- ESP32 #1 zapisuje PREPARED před přijetím simulace a ACTIVE při přechodu do RUNNING. Koncový výsledek se drží v RESULT_PENDING do úspěšného uložení historie.
- Po bootu PREPARED/ACTIVE nikdy nepokračují; mění se na INTERRUPTED a vyžadují explicitní potvrzení. Poškozený/nedostupný žurnál blokuje nové transfery.
- Potvrzení přerušení nejprve uloží záznam do historie; teprve pak se žurnál vymaže. Při chybě NVS zůstává transfer zablokovaný.
- `TransferLogic::forceAbort()` umožňuje přejít do ABORTED s `VALVES_CLOSE_ALL`, pokud nelze trvale uložit ACTIVE.
- Nativní testy: 45/45. Kompletní AddressSanitizer/UndefinedBehaviorSanitizer: 45/45. Runner odděluje stubované Preferences backendy.
- Změněné společné moduly jsou byte-identické v `firmware/common/` a obou ESP32 `src/` složkách.
- Stále neprovedeno: Arduino-ESP32 build, skutečné NVS na cílové desce, fyzický power-cut test, výstupní GPIO/hardware. Transfer zůstává SIMULACÍ.


## Dodatečná kontrola 2026-11-03 — revize 0.68: výpadek peer komunikace během transferu

- Při statickém auditu bylo zjištěno, že `TransferLogic::precheckFault()` a `runningFault()` nejprve kontrolovaly `sourceLevelValid`. ESP32 #1 přitom správně zneplatňuje vzorek horní nádrže po překročení freshness limitu; příčina se proto zaznamenala jako `SENSOR_INVALID` ještě před kontrolou `peerDataValid`.
- Oprava: u požadavku z horní nádrže se nejprve vyhodnotí `peerDataValid`. Výpadek nebo zastarání peer dat se jednoznačně klasifikuje jako `TRANSFER_REASON_COMMUNICATION`.
- Při ztrátě komunikace v PRECHECK se transfer nespustí. Při ztrátě během RUNNING přechází automat do `TRANSFER_ABORTED` a vrací `VALVES_CLOSE_ALL`.
- Regresní testy pokrývají výpadek v PRECHECK i RUNNING; zároveň zůstává transfer ze studny nezávislý na peer komunikaci.
- `TransferLogic.cpp` je bajtově synchronizován do `firmware/common/` a obou ESP32 `src/` stromů.
- Nativní testy: 45/45; kompletní ASan/UBSan: 45/45.
- Toto ověřuje pouze přenositelný simulační automat. W5500/UDP status zůstává neautentizovaný a fyzické výstupy nejsou připojeny. Řídicí příkazy přes síť se nesmí aktivovat před splněním P0 bezpečnostních blokátorů. Arduino build a hardware nebyly ověřeny.
