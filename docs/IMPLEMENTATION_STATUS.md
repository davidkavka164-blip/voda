# Stav implementace — REV86 / MASTER 3.10 (19. 11. 2026)

- **ESP32 #1 W5500:** implementováno SPI inicializování W5500, DHCP, detekce linku/ztráty IP, samostatný retry a zobrazení stavu/IP v dashboardu. Používá GPIO18/19/23/27/26 dle již existující mapy. Wi-Fi STA a konfigurační AP zůstávají zachovány.
- **Omezení ESP32 #1:** produkční `WebServer` routy jsou v REV85 dostupné pouze přes Wi-Fi STA/AP. Ethernetové HTTP ovládání, Basic Auth/Origin guard přes Ethernet, POST a OTA nejsou implementovány. Nesmí se tvrdit, že W5500 IP znamená dostupný ovládací web.
- **Nezávislost:** výpadek W5500/DHCP neblokuje lokální měření, pumpové ochrany, transferovou logiku ani Wi-Fi/AP. Bez inter-unit komunikace.
- **Testy REV85:** nativní C++ 79/79 PASS; statické integrační kontroly PASS. Python 17/17 a ASan/UBSan 77/77 jsou výsledky REV84 a v REV85 nebyly znovu spuštěny.
- **Omezení ověření:** Arduino cílový build, fyzický W5500 test, DHCP/link recovery na desce a fyzické bezpečnostní zkoušky nebyly provedeny.

- **REV86 – Ethernet HTTP:** audit obou jednotek dokončen. Kandidátní knihovny nejsou bezpečně ověřené pro současný stack a neznámou přesnou verzi ESP32 core; produkční firmware se nemění. Ovládací web zůstává přes Wi-Fi/AP.
- Podrobnosti: `docs/W5500_HTTP_DUAL_INTERFACE_AUDIT_REV86.md`.

Autoritativní specifikace: `MASTER_HOME_WATER_CONTROLLER_3.10_REV86.md`. Detail změny: `docs/RELEASE_NOTES_1.00_REV86.md`.

---

# Stav implementace — REV78 / MASTER 3.2 (16. 11. 2026)

## Aktuální stav
- **Architektura:** dvě nezávislé jednotky bez peer UDP/handshake/klíčů; žádná vzájemná aplikační komunikace.
- **ESP32 #1:** studna, dolní nádrž, ventily a transfery; horní → dolní používá místní měření dolní nádrže, kapacitu, watchdog a transferový žurnál.
- **ESP32 #2:** samostatná ochrana horního čerpadla, horní objemová kalibrace a DS18B20 GPIO4. Produkční web je stále pouze přes servisní AP; W5500 DHCP/link recovery neznamená ověřený web přes LAN.
- **Elektrická kalibrace:** jediný důvěryhodný stav je ověřený záznam NVS. Servisní obnova vyžaduje známé měřítko, nezávislou fyzickou referenci a autentizaci; po zápisu se starý vzorek zneplatní a čeká se na nové měření.
- **Ochrany čerpadel:** studna, dolní pumpa a horní safety latch používají vlastní dvouslotové NVS záznamy; jejich dosavadní fail-closed pravidla zůstávají beze změny.
- **Horní nastavitelné limity:** REV78 používá transakční dvouslotový žurnál `slot0/slot1` + `head` + `txn`; původní `state` se validuje a migruje bez smazání. Při neúplném kandidátu se obnoví předchozí potvrzená konfigurace; úplný ověřený kandidát se po restartu dokončí. Poškozený aktivní slot nebo `head` blokuje načtení; poškozený `txn` se ignoruje pouze při současně platném `head` a odpovídajícím aktivním slotu.
- **Kritická západka horního čerpadla:** zůstává oddělená od nastavitelných limitů a REV78 ji nemění.
- **Objemová kalibrace studny:** oddělena od ochrany čerpadla a transferu.
- **Testy REV78:** nativní C++ 78/78 PASS + statické kontroly PASS; Python 17/17 PASS; ASan/UBSan 76/76 PASS + statické kontroly obsažené v ASan runneru PASS. Arduino build a fyzické testy neprovedeny.

Autoritativní specifikace: `MASTER_HOME_WATER_CONTROLLER_3.2_REV78.md`. Detail žurnálu: `docs/NVS_UPPER_TANK_LIMITS_JOURNAL_REV78.md`. Historický audit NVS REV77: `docs/NVS_SENSOR_AND_PUMP_PROTECTION_RECOVERY_AUDIT_REV77.md`.

---



## Aktuální stav
- **Architektura:** dvě nezávislé jednotky bez peer UDP/handshake/klíčů. Produkční `.ino` sketchy již peer status nepřijímají ani nevysílají. Historické peer moduly a testy zůstávají pouze ve `firmware/common` a `tests`; z obou Arduino `src/` stromů i produkčních sketchů byly odstraněny.
- **ESP32 #1:** transfer horní → dolní nevyžaduje hladinu horní nádrže ani stav ESP32 #2. `TransferLogic` používá místní měření dolní nádrže, kapacitu, watchdog a transferový žurnál. Horní pumpa je chráněna lokálně ESP32 #2.
- **Watchdog:** dva časové limity nastavitelné přes web ESP32 #1, NVS + kontrolní načtení; watchdog integrován do transferového automatu. 12,32 l deadband je provizorní, ne fyzicky ověřen.
- **ESP32 #2:** W5500 síťová linka má vlastní DHCP/retry, ale dostupnost webu přes W5500/LAN/internet zatím není prokázána; servisní AP zachováno.
- **Horní objemová kalibrace:** model, NVS, webové zobrazení tabulky, ruční body, absolutní kotva a transferový referenční postup implementovány; nativně testováno. Fyzická přesnost a postup na zařízení neověřeny. **Teplota DS18B20:** REV69 implementována na ESP32 #2 / GPIO4; REV70 zpřesňuje diagnostiku a testuje rollover časovače; OneWire + DallasTemperature, jediný senzor 0x28, 12bit asynchronně, 10s interval, stale limit 15s, rescan 30s. Hostitelské a statické testy PASS; Arduino překlad a fyzické zapojení neověřeny.
- **Testy REV70:** C++ 77/77 a všechny statické kontroly PASS; Python 17/17 PASS. ASan/UBSan 75/75 testovacích programů PASS; statické kontroly PASS. Arduino build a fyzické testy neprovedeny.

Autoritativní specifikace: `MASTER_HOME_WATER_CONTROLLER_2.4_REV70.md`. Historické záznamy níže popisují předchozí revize a jsou ponechány pro auditní stopu; při rozporu má přednost MASTER 2.3 a tento souhrn.

---

# Stav implementace — aktualizace REV61

## Nový závazný směr MASTER 1.5
- Architektura: dvě nezávislé ESP32 bez vzájemné aplikační komunikace — požadováno, implementace ještě neprovedena.
- ESP32 #1: oba transfery přes web v litrech; měření přeneseného množství podle dolní nádrže — existující část logiky implementována, horní transfer stále závisí na peer datech a je aktuálně blokován.
- Ochrana horního transferu proti nulovému/přerušenému přítoku — požadováno, dosud neimplementováno.
- Horní objemová kalibrace s absolutním kotevním bodem a ručním zadáním skutečně odebraných litrů — implementováno v REV67; viz aktuální souhrn výše.
- Teplota horní nádrže DS18B20 — požadováno; REV59 má GPIO4 rezervované, ale akvizice není integrována.
- Peer UDP/HMAC/handshake — cílově odstranit; v REV59 jsou stále přítomné.
- Arduino cílový build a fyzické testy — neprovedeny.

Předchozí historické matice níže popisují starší revize a nesmějí přebít tento aktuální stav ani MASTER 1.5.

---

# Stav implementace — doplnění REV52 (13. 11. 2026)

`PeerAuthenticatedStatusReceiver` nyní poskytuje společnou bezpečnostní hranici pro budoucí autentizovaný UDP příjem: vyžaduje platný `PeerStatusSessionCodec`, ověřuje HMAC a vazbu na peer/boot epoch a teprve poté předá rámec do `PeerStatusMonitor` pro kontrolu sekvence a freshness. Nový modul je nativně testován a synchronizován do obou sketch `src/`.

**Důležité omezení:** modul zatím není volán z `.ino` UDP smyček. Produkční transport je stále CRC-only; handshake, produkční chráněné úložiště peer secretu a fyzický provisioning nejsou hotové. Horní automatický i běžný servisní transfer zůstává blokován. Arduino cílový build ani hardware nebyly ověřeny.

# Stav implementace — doplnění REV50 (13. 11. 2026)

`PeerDatagramEnvelope` nyní poskytuje 8B hlavičku s magic, verzí, typem a přesnou délkou payloadu. Tím odstraňuje kolizi 84B CHALLENGE a 84B autentizovaného STATUS na úrovni návrhu parseru. Modul je synchronizován do obou sketch `src/` a nativně testován.

**Důležité omezení:** obálka ani `PeerStatusSessionCodec` nejsou volány z `.ino` UDP smyček. Skutečný transport je stále 52B CRC-only; handshake ani bezpečné uložení/provisioning peer secretu nejsou hotové. Autentizované řízení horní cesty zůstává blokováno. Hostitelské testy neprokazují cílový Arduino build ani fyzický provoz.

# Stav implementace — doplnění REV49 (13. 11. 2026)

`PeerStatusSessionCodec` je nově implementován jako přenositelná vrstva: 52B status payload + 32B HMAC-SHA-256, směrový session klíč, vazba na device ID a obě boot epoch. Nativní test ověřuje autentizaci, integritu, směr, identitu a chybové případy. Modul je synchronizován do obou sketch `src/`.

**Důležité omezení:** codec není volán z `.ino` UDP smyček. Skutečný transport je stále 52B CRC-only; handshake ani bezpečné uložení/provisioning peer secretu nejsou hotové. Autentizované řízení horní cesty zůstává blokováno. Test codec na hostiteli neprokazuje cílový Arduino build ani fyzický provoz.

# Stav implementace — historická tabulka z revize 0.69, doplněno do 0.99

## Předchozí bezpečnostní doplnění REV48 (13. 11. 2026)

Síťový UDP přenos mezi ESP32 #2 (W5500) a ESP32 #1 (Wi-Fi STA) je implementován jako CRC-only telemetrie. Přenositelné HMAC/handshake/session/replay moduly jsou implementovány a nativně testovány, ale nejsou napojeny na hlavní UDP smyčky `.ino`. ESP32 #2 na portu 4212 zatím pouze vysílá a nepřijímá handshake. `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`, takže běžný i servisní transfer horní cestou zůstává blokován.

Produkční peer-secret backend s prokázanou důvěrností úložiště není implementován. `NvsPeerReplayStore` je dvouslotový journal nad Preferences s kontrolou zápisu a fail-closed poškození, nikoli šifrované úložiště ani ochrana proti fyzickému rollbacku. Podrobnosti a konkrétní pořadí implementace jsou v `PEER_AUTH_TRANSPORT_INTEGRATION_AUDIT_REV48.md`.


Stavy se rozlišují takto: **implementováno** = existuje kód; **nativně testováno** = prošel test na hostitelském počítači; **přeloženo pro ESP32** = skutečný Arduino-ESP32 build; **hardware ověřen** = ověřeno na fyzickém zařízení. Tyto úrovně nejsou zaměnitelné.

| Oblast | Stav implementace | Poznámka k aktuální revizi |
|---|---|---|
| Výpočet dolní nádrže 61,6 l/cm, rozsah 0–121 cm | Implementováno; nativně testováno | Objem se zobrazuje v dashboardu a používá se v simulačním transferu; fyzický průtok není řízen |
| Objemová křivka studny, interpolace a geometrický odhad | Implementováno; nativně testováno | Geometrie mimo měřený rozsah je pouze odhad |
| Lokální merge kalibrace a ochrana proti poškozenému NVS záznamu | Implementováno; testováno se stubem Preferences | Skutečné NVS na ESP32 neověřeno |
| Elektrická kalibrace 4–20 mA a ADC čtení | Implementováno; nativní matematika testována | Skutečný snímač, ADC, shunt a kabeláž neověřeny |
| ESP32 #1: web pro rozměry, objemovou křivku, elektrickou kalibraci a Wi-Fi STA/AP | Implementováno | Produkční HTTP routy pouze Wi-Fi STA/AP; W5500 HTTP není v REV85 implementováno |
| ESP32 #1: dashboard měření a 30denní přítok | Implementováno | Staticky zkontrolováno; web a reálné měření neověřeny na zařízení |
| ESP32 #1: Wi-Fi retry/fallback AP + W5500 DHCP/link recovery | Implementováno | Staticky ověřeno; Arduino build a fyzická síťová zkouška neprovedeny |
| ESP32 #2: čtení horní hladiny a lokální logika ochrany čerpadla | Implementováno; nativně testováno; reléový výstup integrován | Načítá `UpperTankLimits` z místního NVS `hwc_upperlimits`; po startu i chybě snímače vyžaduje platné ověření nebo výslovné nouzové potvrzení. Kritická blokace se obnovuje z NVS. GPIO25 ovládá místní bezpečnostní relé; fyzický test v tomto projektu neproveden. |
| Aplikační protokol stavových zpráv peeru | Implementováno a nativně testováno | Validace verze/ID/CRC, sekvence, boot ID, stáří a 52B serializace; monitor odmítá poslední čtyři opuštěné boot epochy; jednosměrný status UDP transport přes W5500 je integrován, ale neautentizován a fyzicky neověřen |
| Peer status přes W5500 UDP broadcast | Implementováno v obou sketch souborech; staticky zkontrolováno | Vyžaduje stejnou DHCP LAN a broadcast; fyzicky neověřeno; boot ID ESP32 #2 se od 0.69 mění pouze při restartu a monitor odmítá opožděné pakety z posledních čtyř starých epoch; přenos řídicích příkazů chybí |
| Časovaná responder-side handshake orchestrace | Přenositelné jádro implementováno a nativně testováno v 0.58 | `TimedPeerHandshakeResponderCoordinator` opakuje cachované CHALLENGE/READY s limitem pokusů; session key se uvolní až po potvrzení prvního odeslání READY. Zatím není napojen na W5500/UDP. |
| Peer command session / admission pipeline | Implementováno; nativně testováno | `PeerCommandSession` spojuje `PeerSessionCodec`, `PeerCommandGuard`, `PeerRequestTracker`, `PeerSequenceAllocator` a od 0.48 volitelně `PersistentPeerReplayGuard`. `NvsPeerReplayStore` je dvouslotový Preferences/NVS žurnál. Od 0.51 `PeerHandshakeResponder::beginWithReplayStore()` načítá floor pro přesný peer boot ID a přenáší jej v CHALLENGE jako HMAC autentizovanou součást transcriptu; CHALLENGE wire verze je 2 / 84 B. Iniciátor jej smí použít až po `verifyPeerHandshakeChallenge()` a následném `synchronizeOutgoingSequenceFloor()`. Automatická síťová orchestrace a W5500 transport zatím chybí |
| CredentialStore / provisioning hesel AP a admin | Přenositelné dvouslotové úložiště, NVS backend, fyzicky autorizovaný controller a sériový adapter; hostitelsky nativně + ASan/UBSan testováno. Od 0.94 legacy NVS startup gate; od 0.95 dvouslotový CredentialStore s jednorázovou migrací je autoritativní v AP call-sites obou sketchů | 58/58 C++ testovacích programů ve standardní sadě (0.98); ASan/UBSan a Python klient se potvrzují v závěrečném běhu; Python klient 17/17. Obě AP větve nyní odmítají start bez platné sady z `hwcCred`; legacy namespace se používá pouze při cílovém stavu EMPTY a úspěšné migraci; ESP32 #1 již nezobrazuje AP heslo v HTML. Prvotně vytvořená hesla jsou stále jednorázově předána přes Serial. Serial/GPIO board adapter a provisioning controller nejsou zapojeny do `.ino`; NVS/Flash Encryption, reálná NVS, Arduino build a hardware nejsou ověřeny. |
| HTTP přihlášení ESP32 #1/#2 | Basic Auth s per-IP rate limiterem od 0.97 | 5 neúspěšných pokusů / 60 s → 60 s blokace; HTTP 429; 8 pevných klientských slotů. Basic Auth stále není šifrován; VPN/TLS a fyzický test otevřené. |
| Peer klíč / úložiště | Fail-closed kontrakt `PeerSecretVault` nativně testován; provisioning ověřuje zápis zpětným čtením | Platformní audit 0.64 potvrdil závislost na přesné verzi core, inicializaci NVS a Flash Encryption; produkční backend není implementován |
| Lokální bezpečnost čerpadla na ESP32 #1 – studna | Implementováno; nativně testováno; reléový výstup integrován | GPIO25 řídí bezpečnostní relé. Nouzové povolení po fyzickém potvrzení hladiny překonává všechny softwarové blokace ochrany včetně kritického latch, chybné konfigurace a NVS; po restartu se zruší. Fyzický test v tomto projektu neproveden. |
| Lokální bezpečnost čerpadla na ESP32 #1 – dolní nádrž | Implementováno; nativně testováno; reléový výstup integrován v REV28, nouzový režim doplněn v REV30 | GPIO32 řídí vlastní bezpečnostní relé. Minimum a hystereze jsou samostatně nastavitelné; nouzové povolení po fyzickém potvrzení hladiny překonává softwarové blokace včetně kritického latch, chybné konfigurace a NVS; po restartu se zruší. Fyzický test v tomto projektu neproveden. |
| Horní nádrž: nastavitelné limity a logika rezervy/přepouštění | Implementováno; nativně testováno | ESP32 #1 i #2 ukládají lokální limity do vlastního NVS. Automatická synchronizace mezi jednotkami zatím chybí; konfigurace musí být ručně shodná. Fyzický ventil se neovládá. |
| Kritická hladina versus transferové minimum | Konfigurovatelné a vyhodnocované v ESP32 #1; kritická blokace se ukládá do NVS | Ochrana čerpadla je samostatná od transferového minima; bezpečnostní relé GPIO25/GPIO32 se řídí lokálním permit stavem. |
| Nouzový režim čerpadel na ESP32 #1 | Webové potvrzení pro studnu i dolní nádrž; logika a výstupy integrovány | Po fyzickém potvrzení překoná softwarové blokace čidla, kritického latch, konfigurace i NVS. Každé čerpadlo samostatně; RAM-only, po restartu zrušeno. |
| Nouzový režim horního čerpadla na ESP32 #2 | Logika + místní autentizované UI implementovány | Po fyzickém potvrzení překoná softwarové blokace čidla, kritického latch, konfigurace i NVS. RAM-only, po restartu zrušeno. GPIO25 řídí místní relé; fyzický test a UI na zařízení neprovedeny. |
| Transferový automat ESP32 #1 | Logika a fyzický výstup ventilových relé integrovány; restart journal PREPARED/ACTIVE/INTERRUPTED/RESULT_PENDING | Řídí ventilové relé GPIO16/17; automatický transfer po restartu nepokračuje bez nového požadavku/obnovy podle journalu. Fyzický průtok a relé na zařízení neověřeny. Čerpadla spouští vlastní tlaková automatika. |
| Ventily a bezpečnostní relé čerpadel | Výstupní vrstvy integrovány; pump safety GPIO povolena od REV28 | ESP32 #1 řídí ventilové relé GPIO16/17 a pump relé GPIO25/32; ESP32 #2 řídí pump relé GPIO25. Funkce a zapojení všech relé v tomto společném projektu nebyly fyzicky ověřeny. Běžné spouštění čerpadel zůstává v tlakové automatice. |
| Historie transferů | Implementována na ESP32 #1; posledních 12 výsledků, CRC32, NVS kontrolní načtení a ochrana proti přepsání poškozeného záznamu | Historie zachycuje výsledky řídicí logiky; sama o sobě neměří skutečný průtok. Čas je uptime, nikoli kalendářní čas; skutečné NVS a web nejsou ověřeny na zařízení. |
| Denní přítok a 30denní historie | Implementováno v ESP32 #1 včetně verzovaného NVS a ochrany CRC; nativně testováno | Zápis do NVS je v 0.18 vyvolán změnou historie/datumu a omezen na nejvýše jednou za 5 minut; stabilní vzorky nezapisují. Při neplatném čase jsou přírůstky nedatované; hardware čas a měření neověřeny |
| OTA ESP32 #1 | Základní upload handler implementován; hostitelsky syntakticky zkontrolován | OTA partition, skutečný build a zařízení neověřeny |
| Vzdálený přístup přes internet | Návrh zdokumentován v `REMOTE_ACCESS_PLAN.md`; implementace a ověření čekají | Preferován WireGuard na domácím routeru; konkrétní Archer AX12 zatím nelze konfigurovat bez ověření HW verze, firmware a WAN/CGNAT. ESP32 se nemají přímo vystavovat internetu. |
| OTA ESP32 #2 | Základní lokální upload handler implementován v 0.26; hostitelsky syntakticky zkontrolován | Přes místní AP + Basic Auth a `Update`; partition, binární soubor, rollback a zařízení nejsou ověřeny |
| Překlad Arduino-ESP32 | Neproveden | V prostředí chybí Arduino IDE/toolchain |
| Fyzické testy | Neprovedeny | Nutné před uvedením do provozu |

## Kontrolní doplnění revize 0.61
- Provedena kontrola shody s Master 1.3; podrobnosti a matice jsou v `MASTER_COMPLIANCE_AUDIT.md`.
- Znovu ověřeno 43/43 nativních testů a 43/43 testů ASan/UBSan.
- Kopie `firmware/common/` v obou sketch `src/` jsou bajtově shodné.
- Stavový UDP broadcast je neautentizovaný (CRC není autentizace); síťové řídicí příkazy nejsou implementovány.
- Horní objem a teplota se neposílají jako platné hodnoty; fyzické výstupy zůstávají neaktivní.
- Bezpečný provisioning a backend dlouhodobého tajemství zůstávají neimplementované. Revize 0.62 doplňuje návrh v `SECURITY_PROVISIONING_DESIGN.md`; Flash Encryption, NVS Encryption, Secure Boot ani eFuse nebyly zapnuty.
- Síťová handshake orchestrace přes W5500, skutečné transfery, teplotní měření, Arduino build a hardware testy zůstávají otevřené.

## Zásadní omezení
Tento balíček je vývojový firmware, nikoli certifikované bezpečnostní zařízení. Pumpová relé přebírají potvrzené zapojení ze samostatného projektu studny, ale fyzická funkce všech tří relé v tomto společném projektu, motorové obvody, skutečný stav při bootu/resetu a celé řízení musí být ověřeny na cílovém hardware před běžným provozem.


## Revize 0.21
- ESP32 #2 nově používá samostatný `UpperTankProtection` místo obecné ochrany studny s nulovou hloubkou.
- Ochrana vyžaduje validní lokální `UpperTankLimits`, funkční NVS, validní hladinu v rozsahu a dosažení obnovovací hladiny po startu i po poruše snímače. Kritická blokace má hysterezi.
- Limity na ESP32 #1 a ESP32 #2 zůstávají v oddělených fyzických NVS; jejich bezpečné předání/konfigurace mezi jednotkami není implementováno. Výstupní GPIO zůstává neaktivní.
- Nativní sada 16/16 prošla; nový test prošel ASan/UBSan; oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou. Skutečný Arduino build a hardware neověřeny.

## Revize 0.20
- Nastavitelné limity horní nádrže a logika hystereze přepouštěcího ventilu přidány; pouze logické vyhodnocení.
- NVS úložiště limitů má CRC a ochranu před přepsáním poškozeného záznamu.
- Nativní sada 15/15 prošla; oba sketch soubory prošly hostitelskou syntaktickou kontrolou.
- Limity zatím nejsou předávány ESP32 #2; fyzické výstupy zůstávají vypnuté.

## Revize 0.18
- Transferový automat nově odmítá fyzicky nemožnou hladinu dolní nádrže během běžícího transferu (mimo 0–121 cm); regresní test přidán.
- Výstupy zůstávají softwarově neaktivní; jde o ochranu logiky, nikoliv potvrzení funkce na hardware.


## Revize 0.22
- Horní čerpadlo má nyní logický nouzový režim s výslovným potvrzením fyzické kontroly, možností výslovného ukončení a zákazem obejít kritickou blokaci či chybu NVS/konfigurace. Nouzový režim se po restartu neobnovuje. Ovládací UI/transport zatím chybí.
- `UpperTankProtection` nyní podporuje výslovně potvrzený nouzový režim při neplatném měření, který lze explicitně ukončit. Nouzový režim neobchází kritickou blokaci, neplatnou konfiguraci ani chybu NVS a po restartu se neobnovuje.
- Logika je nativně testovaná; ovládací rozhraní nouzového režimu a fyzický výstup horního čerpadla nejsou implementovány.

## Revize 0.23
- ESP32 #1 web nyní nabízí výslovně označenou SIMULACI transferu studna → dolní nádrž nebo horní nádrž → dolní nádrž. Uživatel nastaví objem a časový limit, může simulaci zastavit a vidí cílovou hladinu, odhad změny objemu a důvod ukončení.
- Simulace volá `TransferLogic` s čerstvostí místních měření, stavem peer komunikace, minimální hladinou zdroje, ochranou studny a kapacitou dolní nádrže. Jakákoli chyba vede do terminálního stavu s příkazem `VALVES_CLOSE_ALL`.
- Fyzické výstupy nejsou propojeny; označení SIMULACE je zobrazeno přímo u formuláře. Nejde o provozní přenos vody.
- Nativní sada: 16/16. Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou. Skutečný Arduino-ESP32 build a hardware nebyly testovány.

## Revize 0.24
- ESP32 #2 má vlastní lokální AP/web pro nastavení `UpperTankLimits` a nouzový režim horního čerpadla. Změna limitů vyžaduje znovu dosažení obnovovací hladiny.
- Přihlašovací údaje se generují při prvním spuštění a ukládají do NVS; při startu se vypíší do Serial Monitoru. Web je přístupný přes místní AP na 192.168.4.1.
- Limity mezi ESP32 #1 a #2 zatím nejsou automaticky synchronizovány. Uživatelská konfigurace musí být ručně shodná.
- Nativní sada 16/16 prošla; oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou. Skutečný Arduino-ESP32 build a hardware nebyly testovány.


## Revize 0.25
- Peer status protokol verze 2 rozšířen na 52 bajtů o checksum lokálních limitů horní nádrže. ESP32 #2 jej pouze oznamuje, ESP32 #1 porovnává s vlastní konfigurací.
- Neshoda, chybějící konfigurace nebo nulový checksum blokuje simulaci transferu z horní nádrže; žádné nastavení se nepřenáší ani automaticky nepřepisuje.
- CRC není autentizace. Peer síť musí zůstat důvěryhodná; fyzické výstupy zůstávají neaktivní.


## Revize 0.26
- ESP32 #2 má místní autentizovanou OTA upload stránku přes `Update`; při chybě zápisu upload přeruší, při úspěchu odešle potvrzení a restartuje jednotku.
- Peer konfigurace checksum diagnostika z 0.25 zůstává zachována.
- Skutečný Arduino-ESP32 build, OTA partition a fyzická aktualizace nejsou ověřeny.


## Revize 0.27
- OTA ESP32 #2 odmítá soubory bez přípony `.bin` a blokuje update při logickém povolení čerpadla či nouzovém režimu. Při změně stavu během zápisu update abortuje.
- Jde o dodatečnou softwarovou pojistku; správnost binárního souboru, partition a skutečné OTA nejsou ověřeny.



## Revize 0.28 — persistentní kritická blokace horního čerpadla
- Přidán `NvsUpperTankProtectionStore` v odděleném namespace `hwc_up_safety`, který ukládá kritickou blokaci a platnost bezpečnostního stavu s CRC.
- ESP32 #2 obnovuje blokaci po restartu. Chybějící záznam se nevydává za ověřený bezpečný stav: inicializuje se až po platném měření na obnovovací hladině. Poškozený existující záznam se automaticky nepřepisuje.
- Při změně kritické blokace se změna ukládá do NVS; při selhání zápisu je logické povolení blokováno. Nouzové povolení je zakázáno, dokud není bezpečnostní záznam platný.
- Nativní testy: 17/17 prošlo. Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou. Skutečný Arduino-ESP32 build, NVS na zařízení a fyzické testy nebyly provedeny.


## Revize 0.29
- `TransferLogic` nově kontroluje fyzickou horní mez zdrojové hladiny před startem i během transferu; horní mez je odvozena z nastavené využitelné hloubky konkrétní nádrže/studny.
- Neplatná konfigurace maximální hladiny blokuje transfer; zdrojová hladina mimo fyzický rozsah vyvolá `SENSOR_INVALID`.
- Nativní sada 17/17 prošla včetně ASan/UBSan.
- Hostitelská syntax kontrola `.ino` nebyla v této revizi provedena, protože stuby nejsou dostupné. Skutečný Arduino build a hardware zůstávají neověřeny.


## Revize 0.30
- Opraveno nezávislé vyhodnocení čerstvosti snímačů pro transfer. Dolní nádrž má vlastní časovou značku a validitu; studna má vlastní časovou značku. Při transferu z horní nádrže není vyžadováno měření studny.
- Přidán `SampleFreshness.h` a test pro wrap-safe kontrolu stáří. Nativní testovací programy 18/18 prošly.
- Fyzické výstupy zůstávají vypnuté; skutečný Arduino build a hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.32
- Opravena ztráta příležitosti k trvalému uložení historie simulace: záznam přidaný do RAM zůstává označen jako čekající, dokud NVS nepotvrdí zápis kontrolním načtením.
- Nedostupné NVS se zkouší znovu každých 30 sekund; neúspěšné ukládání čekající historie se opakuje nejdříve po 5 sekundách.
- Při pozdějším obnovení NVS se nejprve ověří existující záznam, následně se sloučí čekající RAM záznamy bez duplicit. Poškozený existující záznam zůstává chráněn před přepsáním.
- Nativní sada 20/20 prošla; celá sada prošla ASan/UBSan. Hostitelská syntax kontrola `.ino`, skutečný Arduino build a hardware nebyly provedeny.


## Kontrolní stav po revizi 0.33
- Ochrana čerpadla studny odmítá hladinu mimo fyzický rozsah `0..wellGeometry.usableDepthCm`; samotný stav snímače `OK` již nestačí.
- Nouzový režim lze po fyzické kontrole potvrdit také při hodnotě mimo rozsah, ale ne při chybné konfiguraci, kritické blokaci nebo chybě bezpečnostního úložiště.
- Nativní sada 20/20 prošla. Skutečný Arduino-ESP32 build a fyzické testy nebyly provedeny.

## Kontrolní stav po revizi 0.34
- Revize 0.34 přidává `PeerCommandGuard`: kontrola výsledku autentizace předávaného externím ověřovačem, cílového boot ID, sekvence a posledních 16 ID požadavků. Modul není zapojen do UDP, autentizace HMAC ani ACK/REJECTED/FAILED zatím nejsou implementovány.


## Kontrolní stav po revizi 0.36
- Přidán přenositelný HMAC-SHA-256 modul a konstantně časové porovnání 32bajtových tagů. Ověřeno proti známým testovacím vektorům RFC 4231 včetně klíče delšího než blok SHA-256.
- HMAC zatím není integrován do UDP příkazů. Chybí bezpečné vytvoření/uložení klíče, rotace, identita peerů, přesná kanonická serializace podepisovaných polí a ochrana proti restart/replay.
- Nativní sada 23/23 prošla; skutečný Arduino-ESP32 build a fyzické testy nebyly provedeny.


## Kontrolní stav po revizi 0.36
- K předchozímu HMAC primitivu přibyl kanonický 68bajtový rámec `PeerCommandFrame` s little-endian serializací a ověřením HMAC před dekódováním.
- Nativní sada 23/23 prošla, včetně nového testu rámce; celá sada prošla také ASan/UBSan.
- Stále chybí bezpečné párování a provisioning klíčů, jejich rotace, autentizovaný handshake, síťové napojení, persistentní anti-replay a ACK/REJECTED/FAILED. Příkazová cesta proto není aktivní.
- Arduino-ESP32 build a fyzický hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.37
- `PeerCommandGuard` nově vyžaduje identitu lokálního a vzdáleného zařízení a explicitně ustavenou peer session; změna boot ID peeru již automaticky neotevírá novou sekvenční řadu.
- Testy pokrývají neustavenou session, chybná ID obou stran, neautentizovaný handshake, změnu peer boot ID, duplicity request ID a staré cílové boot ID.
- `recordAuthenticatedPeerSession()` zatím pouze přijímá výsledek budoucí autentizované handshake vrstvy jako boolean; samo o sobě nic kryptograficky neověřuje. Handshake, klíče, persistentní anti-replay, UDP a ACK/REJECTED/FAILED zůstávají otevřené.
- Nativní testovací programy 23/23 prošly; celá sada prošla ASan/UBSan. Arduino-ESP32 build a hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.38
- `PeerHandshake.*` implementuje transportně nezávislý vzájemný HMAC challenge-response: HELLO s nonce iniciátora, CHALLENGE s nonce responderu a autentizací celého přepisu, FINISH s důkazem iniciátora a doménově oddělené odvození session key.
- Handshake váže identitu obou jednotek a boot ID obou stran. Nulové a shodné nonce, nulové identity/boot ID a klíč kratší než 32 bajtů jsou odmítnuty.
- Generování nonce/boot ID, timeout pending handshake, provisioning/ochrana tajemství, síťové kódování a integrace s `PeerCommandGuard` nejsou implementovány. `PeerHandshakeResponder` drží jeden pending transcript a po prvním FINISH pokusu jej spotřebuje; časový limit musí vynutit volající vrstva. Na ESP32 musí volající dodat nonce z `esp_random()` a session povolit až po dokončeném FINISH.
- Nativní sada 24/24 prošla. Skutečný Arduino-ESP32 build, síť a hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.39
- `PeerHandshakeWire.*` doplňuje pevné explicitní kódování HELLO 28 B, CHALLENGE 80 B a FINISH 38 B; nepřenáší se surové struktury. Dekodéry kontrolují přesnou délku, hlavičku, verzi, typ a strukturální pole a při chybě nulují výstup.
- Nativní test ověřuje celý handshake po wire encode/decode a shodný session key obou stran. Nativní sada 24/24 prošla; celá sada prošla ASan/UBSan.
- Zůstává otevřené: skutečné `esp_random()`/boot ID, timeout a správa pending handshake, bezpečný provisioning a ochrana tajemství, UDP/W5500 transport, propojení s `PeerCommandGuard`, persistentní anti-replay a ACK/REJECTED/FAILED.
- Arduino-ESP32 build, fyzická síť a hardware nebyly ověřeny; fyzické výstupy zůstávají neaktivní.


## Kontrolní stav po revizi 0.40
- `PeerSecretVault.*` zavádí fail-closed kontrakt pro úložiště sdíleného 32B tajemství. Backend musí výslovně deklarovat důvěrné úložiště; prosté NVS/Preferences bez NVS encryption je odmítnuto. Provisioning a mazání vyžadují explicitní fyzické potvrzení.
- Modul neimplementuje šifrování, ESP32 NVS encryption ani bezpečný přenos klíče. Žádný produkční backend není připojen; klíč se proto zatím nesmí používat pro síťový handshake.
- Testy pokrývají odmítnutí nechráněného úložiště, neplatné identity, chybějící fyzické potvrzení, vazbu na dvojici zařízení, mazání a nulování výstupní paměti. Nativní sada 26/26 prošla včetně ASan/UBSan.
- Společné moduly jsou synchronizovány v obou firmware. Arduino-ESP32 build, skutečné NVS encryption, fyzický provisioning, W5500 a hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.41
- `PeerCommandResponse.*` definuje ACK, REJECTED a FAILED, důvody odmítnutí/chyby a kontrolu korelace s původním request ID, identitou i boot ID obou zařízení. Odpověď se přenáší stávajícím HMAC rámcem; testuje se round-trip a odmítnutí pozměněného podpisu.
- Modul zatím není napojen na W5500/UDP ani `TransferLogic`; ACK není důkaz fyzického průtoku. Bezpečný ESP32 backend pro tajemství, provisioning, síťový handshake, persistentní anti-replay a výstupy zůstávají otevřené.
- Nativní sada: 27/27 testovacích programů. Skutečný Arduino-ESP32 build a fyzické testy nebyly provedeny.


## Kontrolní stav po revizi 0.42
- Přidán `PeerHandshakeTimeout.*` s wrap-safe timeoutem a `TimedPeerHandshakeResponder.*`, který váže pending challenge na lhůtu, po vypršení ruší handshake a odmítá FINISH na hranici timeoutu i po ní. Při odmítnutí nulování výstupního session key.
- Testy pokrývají běžný timeout, přesnou hranici, přetečení `millis()`, úspěšné přijetí před lhůtou, nulovou/nepřípustně dlouhou lhůtu, zrušení a jednorázovost FINISH.
- Nativní testovací programy: 26/26; celá sada prošla AddressSanitizer a UndefinedBehaviorSanitizer.
- Nové moduly synchronizovány do obou sketch `src` složek.
- Wrapper není zatím napojen na UDP/W5500; stále chybí bezpečný ESP32 backend pro tajemství, provisioning, skutečné `esp_random()`, síťová integrace, persistentní anti-replay, Arduino-ESP32 build a fyzické testy. Žádné výkonové výstupy nejsou aktivní.


## Kontrolní stav po revizi 0.43
- Přidán `PeerSessionKeys.*`: z 32B session key odvozuje oddělené HMAC-SHA-256 klíče pro initiator → responder a responder → initiator. KDF používá doménové oddělení a zahrnuje obě device ID i boot ID.
- `test_peer_session_keys` ověřuje shodu odvození na obou stranách, rozdílnost směrových klíčů, vazbu na identitu/boot ID a nulování výstupu při chybě.
- Nativní runner: 31/31 testovacích programů prošlo standardním během; kompletní ASan/UBSan sada nebyla v revizi 0.44 dokončena v časovém limitu. Nový codec test byl samostatně spuštěn se sanitizéry. Společné moduly jsou synchronizovány v obou firmware.
- Směrové klíče nejsou zatím napojeny na příkazové rámce ani W5500. Chybí konkrétní chráněné úložiště/provisioning, síťový handshake, persistentní anti-replay a integrace ACK/REJECTED/FAILED s transfery. Arduino-ESP32 build a hardware nebyly ověřeny; výstupy zůstávají neaktivní.


## Kontrolní stav po revizi 0.44
- Přidán `PeerSessionCodec.*`: odchozí rámce se podepisují směrovým klíčem podle role lokální jednotky; příchozí rámce se ověřují opačným směrovým klíčem. Codec navíc pevně kontroluje lokální/peer device ID a boot ID.
- Test pokrývá oba směry komunikace, odmítnutí rámce v nesprávném směru, nesoulad peer boot ID a neplatný kontext. Vnitřní kopie klíčů se při destrukci nulují; třída je necopyable.
- Standardní nativní runner: 31/31 testovacích programů prošlo. Kompletní ASan/UBSan sada v této revizi nebyla dokončena v časovém limitu; nový codec test prošel samostatným sanitizovaným během.
- Codec zatím není připojen k handshake, `PeerCommandGuard`, UDP/W5500 ani transferům. Chybí bezpečný ESP32 secret backend/provisioning, persistentní anti-replay, síťový stavový automat, Arduino-ESP32 build a fyzické ověření. Výstupy zůstávají neaktivní.


## Kontrolní stav po revizi 0.45
- Přidán `PeerCommandSession.*`, který spojuje `PeerSessionCodec` a `PeerCommandGuard`. Aktivace vyžaduje explicitní výsledek čerstvého autentizovaného handshake od volající vrstvy; samotná třída handshake neprovádí.
- `receiveCommand()` ověří HMAC a vazbu device ID/boot ID, odmítne jiné typy než COMMAND a poté provede admission/replay kontrolu. Dekódovaný příkaz se předá aplikaci pouze při přijetí. RESPONSE rámce se záměrně ponechávají samostatné request-correlation vrstvě.
- `close()` maže interní klíče codec. `PeerCommandGuard::endSession()` zachovává posledních 16 request ID v RAM při reconnectu stejného běhu; po restartu zařízení se cache ztratí. Persistentní anti-replay není implementován.
- `PeerSessionCodec` získal explicitní `reset()`/`clear()`. Neplatný kontext vymaže klíče i identitu.
- Nativní runner: 32/32 testovacích programů prošlo. Nový session pipeline test prošel samostatně ASan/UBSan. Kompletní sanitizovaný runner byl přerušen časovým limitem po prvních 24 testech; celá sada tedy není označena jako sanitizovaně ověřená.
- Síťové napojení W5500/UDP, handshake orchestrace, bezpečný secret backend/provisioning, persistentní anti-replay, ACK/REJECTED/FAILED correlation, Arduino-ESP32 build a hardware testy zůstávají otevřené. Výkonové výstupy nejsou aktivní.


## Kontrolní stav po revizi 0.46
- Přidán `PeerRequestTracker.*` s kapacitou 8 souběžných odchozích příkazů. Kontroluje validitu požadavku, duplicitní request ID, korelaci již autentizované odpovědi (request ID, command code, device ID a boot ID obou stran) a wrap-safe timeout do INT32_MAX ms.
- Nesoulad odpovědi neodstraní čekající záznam; odpověď po timeoutu je odmítnuta. Vypršené ID lze vyzvednout přes `takeExpired()`. Tabulka je pouze RAM, nechrání proti replay po restartu.
- Nativní runner: 33/33 testovacích programů prošlo. Nový tracker test samostatně prošel ASan/UBSan; kompletní sanitizovaná sada nebyla spuštěna.
- Tracker není připojen k `PeerSessionCodec`/`PeerCommandSession`, W5500 ani `TransferLogic`. Před použitím musí být odpověď ověřena aktivním session codec. Neřeší retry ani dokončení transferu; ACK je pouze přijetí příkazu.
- Společné moduly jsou synchronizovány do obou firmware `src` složek. Arduino-ESP32 build, síť a hardware nebyly ověřeny; fyzické výstupy zůstávají neaktivní.


## Kontrolní stav po revizi 0.47
- `PeerCommandSession` nyní integruje odchozí request tracking a příjem autentizovaných odpovědí. Rámec je určen k odeslání až po úspěšné registraci požadavku; při selhání registrace se výstupní data nulují.
- `receiveResponse()` nejprve provede HMAC a session binding decode, potom ověří RESPONSE a předá ji trackeru. Testuje se platná ACK, pozměněný podpis, korelace a timeout.
- `PeerRequestTracker` drží 16 posledních použitých request ID a odmítá jejich opětovné použití; při zavření peer session se pending sloty zruší, ale historie zůstane v RAM pro reconnect stejného běhu. Po restartu se ztratí.
- Nativní runner: 33/33 testovacích programů prošlo. Oba nové/změněné testy `test_peer_command_session` a `test_peer_request_tracker` prošly samostatně ASan/UBSan.
- Stále chybí W5500/UDP napojení, handshake orchestrace, bezpečný ESP32 secret backend/provisioning, persistentní anti-replay, retry politika a integrace s transferovým automatem. Arduino-ESP32 build a fyzické testy nebyly provedeny; výkonové výstupy zůstávají neaktivní.


## Kontrolní stav po revizi 0.49
- Přidán `NvsPeerReplayStore` s dvouslotovým žurnálem `slot0`/`slot1`, generačním čítačem a CRC32. Nový checkpoint se ukládá do druhého slotu a po zápisu se znovu načte a ověří; starší platný slot zůstává nedotčen.
- Při čtení se vybere novější platný záznam; pokud je jeden slot poškozený, druhý platný se použije. Pokud jsou oba poškozené, backend vrátí CORRUPT a guard odmítne aktivaci. Chyba čtení/zápisu se nepovažuje za prázdné úložiště.
- Důvod návrhu: dokumentace ESP-IDF uvádí, že NVS při aktualizaci zapíše novou hodnotu před odstraněním staré; po výpadku napájení může být ztracena nová key-value položka. Dva samostatné klíče tak poskytují fallback na předchozí checkpoint. `Preferences::putBytes()` ve zdrojovém kódu Arduino-ESP32 volá `nvs_set_blob()` a `nvs_commit()`.
- Nativní sada 36/36 prošla. NVS adaptér včetně propojení s replay guardem prošel samostatným ASan/UBSan testem. Simulovaný test výpadku napájení není fyzický power-cut test.
- **Stále blokuje:** sekvenční allocator odesílatele a autentizované předání `persistentReplayFloor()` po novém handshake; bez toho se odesílatel nemusí umět posunout nad rezervovaný rozsah. Chybí také ověření přesnou verzí Arduino-ESP32 core na cílové desce, W5500 orchestrace, `esp_random()` boot ID/nonce, retry a integrace transferu.

## Kontrolní stav po revizi 0.48
- Přidán `PersistentPeerReplayGuard` a `IPeerReplayStore`. Checkpoint váže rezervované sekvenční okno na identitu lokální/peer jednotky a peer boot ID; CRC32 odhaluje náhodné poškození. CRC není ochrana proti úmyslnému pozměnění ani rollbacku flash.
- Při zachovaném peer boot ID po restartu přijímače se jako sekvenční podlaha použije dříve rezervované maximum. Přijímač tak po rebootu odmítne všechny sekvence z rezervovaného rozsahu, i když některé nebyly před výpadkem použity. Nová sekvenční epocha je povolena pouze po čerstvém autentizovaném handshake s novým peer boot ID.
- Rezervace se ukládá po blocích; selhání atomického zápisu odmítne příkaz jako `PEER_COMMAND_REJECT_PERSISTENCE_FAILURE`. `PeerCommandSession::activateWithPersistentReplay()` používá persistentní guard před původním RAM guardem.
- Nativní sada 36/36 prošla. `PersistentPeerReplayGuard` a integrační `PeerCommandSession` test prošly ASan/UBSan. Testy používají simulované úložiště, nikoli skutečné NVS.
- **Blokující omezení:** konkrétní ESP32 `NvsPeerReplayStore` backend je implementován jako dvouslotový Preferences/NVS žurnál, ale zatím není připojen do běžící command session. Backend je připraven, ale persistentní ochrana zatím není aktivní v běžícím firmware, protože chybí autentizované předání sekvenční podlahy odesílateli a W5500 orchestrace. Sekvenční čítač odesílatele, autentizované předání `persistentReplayFloor()` odesílateli, bezpečné boot ID/nonce z `esp_random()`, W5500 orchestrace, retry a transfer integrace zůstávají otevřené.
- Při restartu přijímače se přeskočí celý rezervovaný blok; odesílatel musí sekvence přidělovat monotónně v rámci svého boot ID. Opakování stejného požadavku musí zachovat původní sequence/request ID, nesmí vytvořit nový příkaz.


## Kontrolní stav po revizi 0.50
- Přidán `PeerSequenceAllocator`: čítač sekvencí COMMAND začíná od 1 pro nové lokální boot ID, zachovává se při reconnectu v rámci stejného boot ID, nikdy se neposouvá zpět a odmítne přidělit sekvenci nad `0x7FFFFFFF`.
- `PeerCommandSession::prepareOutgoingRequest()` přiděluje sekvenci automaticky a do `PeerRequestTracker` ukládá stejný rámec, který zakóduje. Při plné tabulce nebo chybě kódování může vzniknout mezera v čítači; mezery jsou bezpečné a sekvence se nikdy nerecyklují.
- `synchronizeOutgoingSequenceFloor()` posune čítač nad rezervovanou hranici přijímače. Metoda sama neověřuje autentizaci parametru: smí být volána pouze s hodnotou získanou z budoucího autentizovaného session-sync. HELLO/CHALLENGE/FINISH zatím sekvenční hranici nepřenáší a W5500 orchestraci neobsahuje.
- Integrační test po restartu přijímače synchronizuje floor 64 a ověřuje, že první COMMAND dostane sekvenci 65 a přijímač jej přijme.
- Nativní sada 37/37 prošla. Nový allocator a upravený integrační test byly samostatně ověřeny ASan/UBSan.
- **Stále blokuje:** autentizované přenesení floor v handshake/session-sync, síťová orchestrace W5500, provisioning a bezpečné uložení dlouhodobého tajemství, `esp_random()` boot ID/nonce, retry/response transport a integrace s transferovým automatem. Žádný fyzický výstup není aktivován.


## Kontrolní stav po revizi 0.51
- Handshake CHALLENGE přenáší `authenticatedReplayFloor`; pole je zahrnuto do HMAC challenge/finish a odvození session key. Manipulace s hodnotou proto zneplatní ověření.
- `PeerHandshakeResponder::beginWithReplayStore()` načte floor z persistentního replay úložiště podle místní identity, identity peeru a peer boot ID z HELLO. Nový peer boot ID dostane floor 0; poškozený záznam, chyba úložiště nebo neshoda identity handshake odmítnou.
- CHALLENGE wire formát se změnil z 80 na 84 bajtů a verze handshake wire z 1 na 2; staré CHALLENGE rámce jsou záměrně odmítnuty.
- Odesílatel po úspěšném ověření CHALLENGE může posunout allocator nad floor; test ověřuje 128 → 129. Není však ještě vytvořen síťový orchestrátor, který by handshake automaticky provedl a zavolal synchronizaci v aktivní `PeerCommandSession`.
- Nativní sada 37/37 prošla. Nový NVS/handshake test a wire test prošly samostatně ASan/UBSan. Arduino-ESP32 build, W5500 a fyzický hardware zůstávají neověřené.
- **Další blokátor:** síťová orchestrace HELLO/CHALLENGE/FINISH na W5500, bezpečný provisioning tajemství a skutečné `esp_random()`/boot ID. Výkonové výstupy zůstávají neaktivní.

## Revize 0.52 — handshake READY
- Doplněn čtvrtý krok READY. Responder jej může vytvořit po úspěšném ověření FINISH; iniciátor jej ověří session key a má tak důkaz přijetí FINISH druhou stranou.
- READY wire rámec je 38 B, typ 4, protokol verze 2. HMAC používá oddělenou doménu.
- Nativní testy: 38/38 prošlo. Nový test ověřuje platný READY, změněný tag, jiný klíč a chybné wire rámce.
- READY zatím není propojeno s UDP/W5500 ani automatickým lifecycle `PeerCommandSession`; příkazová komunikace proto zůstává vypnutá.


## Kontrolní stav po revizi 0.53
- `PeerHandshakeInitiator` nově řídí lokální stav iniciátora: IDLE → WAIT_CHALLENGE → WAIT_READY → ESTABLISHED, případně FAILED.
- CHALLENGE se nejprve ověří HMAC/transcriptem; teprve potom se použije replay floor na `PeerSequenceAllocator`. Při neplatném CHALLENGE se sekvenční čítač neposune.
- FINISH se uchovává beze změny pro retry; opakovaný přenos tak používá identický FINISH. Session key není dostupný před ověřením READY.
- Při chybě se mažou dočasná kopie sdíleného tajemství a session klíče. Test ověřuje i sekvenční posun floor 64 → první další sekvence 65.
- Nativní sada: 39/39 testovacích programů prošlo. Nový test prošel samostatně ASan/UBSan.
- **Stále chybí:** responder-side síťový orchestrátor, UDP/W5500 transport, `esp_random()` pro boot ID/nonce, bezpečný provisioning a chráněný backend tajemství, automatická aktivace `PeerCommandSession`, retry/timeout síťové smyčky a propojení s transfery. Arduino-ESP32 build a fyzické testy nebyly provedeny; výstupy čerpadel/ventilů zůstávají neaktivní.


## Kontrolní stav po revizi 0.54
- Přidán `PeerHandshakeResponderCoordinator` pro responder stranu: WAIT_FINISH → READY_PENDING → ESTABLISHED, případně FAILED. Zahájení používá `beginWithReplayStore()`, takže replay floor je načten podle identity a boot ID peeru.
- Po ověření FINISH se vytvoří READY a uloží se jeho přesná kopie pro retry. Opakované vydání READY vrací identické bajty; session key není dostupný před potvrzením předání READY transportní vrstvě k odeslání. Iniciátor nadále samostatně ověřuje READY, takže samotné odeslání responderem neznamená, že protistrana session přijala.
- Timeout pokrývá čekání na FINISH i READY_PENDING. Timeout, neplatný FINISH a explicitní cancel mažou READY i session key.
- Nativní sada: 40/40 testovacích programů prošlo. Nový test responder koordinátoru prošel samostatně ASan/UBSan.
- Neimplementováno: UDP/W5500 transportní orchestrace, opakování HELLO/CHALLENGE/FINISH/READY v síti, provisioning tajemství, `esp_random()` integrace, aktivace `PeerCommandSession`. Příkazová komunikace zůstává vypnutá. Arduino-ESP32 build a hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.55
- Přidán `PeerHandshakeRetryPolicy` pro bounded retransmission handshake zpráv. První pokus se počítá jako okamžité odeslání; další pokusy jsou povoleny po pevném intervalu do maximálního počtu. Po posledním pokusu se retransmise zastaví a `exhausted()` signalizuje vyčerpání až po dalším intervalu.
- Časování je wrap-safe přes přetečení `millis()`. Politika je pouze rozhodovací; transport musí opakovaně odesílat přesně stejný serializovaný datagram, kontrolovat celkový handshake timeout a volat `markSent()` až po úspěšném předání datagramu transportu.
- Nativní sada: 41/41 testovacích programů prošlo. Nový retry test prošel samostatně ASan/UBSan.
- 86 souborů `firmware/common` je synchronizováno s oběma firmware `src` složkami.
- UDP/W5500 orchestrace, síťové řízení retry, bezpečný secret backend/provisioning, `esp_random()` integrace, aktivace `PeerCommandSession`, Arduino-ESP32 build a fyzické testy zůstávají otevřené. Výkonové výstupy zůstávají neaktivní.

## Kontrolní stav po revizi 0.56
- `PeerHandshakeResponderCoordinator` nyní uchovává přesný přijatý FINISH a při jeho identické retransmisi vrací cachovanou READY, a to jak ve stavu READY_PENDING, tak po přechodu do ESTABLISHED. Tím je pokryt případ, kdy se READY ztratí a iniciátor zopakuje FINISH.
- Pozměněný FINISH není přijat jako retransmise. Při vypršení lhůty ve stavu READY_PENDING je retransmise odmítnuta a citlivý stav vymazán.
- Změna je transportně nezávislá; UDP/W5500 ji zatím nevolá. Životnost již ustavené session a bezpečné zavření session musí dále řídit transportní/orchestrační vrstva.
- Nativní sada a samostatný ASan/UBSan test koordinátoru jsou uvedeny v release notes 0.56. Arduino-ESP32 build a fyzické zařízení nebyly ověřeny.


## Kontrolní stav po revizi 0.57
- [x] Přidán `TimedPeerHandshakeInitiator`: časově omezený HELLO/CHALLENGE/FINISH/READY životní cyklus s omezenými retransmisemi HELLO a FINISH.
- [x] Opakování používá původní neměnný HELLO/FINISH; po přijetí CHALLENGE se retry politika restartuje pro FINISH.
- [x] Neplatný CHALLENGE/READY, celkový timeout a vyčerpání pokusů ukončí handshake fail-closed; session key je dostupný až po ověření READY.
- [x] Nativní sada: 42/42 testovacích programů prošlo; nový test prošel ASan/UBSan.
- [x] Nové společné soubory jsou synchronizovány v obou firmware `src` složkách.
- [ ] Propojit oba handshake koordinátory s reálným UDP/W5500 transportem; stále chybí secret provisioning, chráněný ESP32 backend a `esp_random()` integrace.
- [ ] Arduino-ESP32 build a fyzické testy nebyly provedeny.


## Kontrolní stav po revizi 0.58
- Přidán `TimedPeerHandshakeResponderCoordinator`: omezené retry CHALLENGE a READY, přesně cachované zprávy, odmítnutí změněného FINISH a oddělení potvrzení odeslání od samotného vytvoření zprávy.
- Nový test ověřuje ztrátu CHALLENGE/READY, identické opakování, retry limity, dostupnost session key až po prvním READY a wrap-safe časování.
- `tests/run_native_tests.sh`: 43/43 testovacích programů prošlo.
- Síťová orchestrace W5500/UDP, provisioning tajemství, příkazové zprávy, skutečné transfery a aktivace výstupů zůstávají otevřené.


## Revize 0.59 — doplnění retry odolnosti
- Responder READY retry timer se spouští při prvním platném FINISH, takže ztracený/selhaný první UDP send nevyřadí retry mechanismus.
- Opakovaný identický FINISH nezahajuje nový timer a nemění ESTABLISHED zpět na READY_PENDING.
- `markReadyRetryAttempted(nowMs, accepted)` počítá každý pokus; session key se zpřístupní až po úspěšně přijatém odeslání READY. Při vyčerpání pokusů bez úspěšného odeslání se klíč vymaže.
- `markChallengeAttempted(nowMs)` dovoluje započítat neúspěšné lokální pokusy CHALLENGE.
- Nativní sada zůstává 43/43; celá sada prošla i ASan/UBSan. Síťová orchestrace a fyzické testy nejsou součástí této revize.


## Revize 0.60 — dokumentace vzdáleného přístupu
- Doplněn samostatný návrh vzdáleného přístupu přes VPN v `REMOTE_ACCESS_PLAN.md`.
- Zaznamenán uživatelem uvedený router TP-Link Archer AX12 a oficiálně dohledaná podpora WireGuard pro Archer AX12(EU) V1; konkrétní kus zůstává neověřen.
- Žádná síťová konfigurace ani firmware routeru nebyly změněny.
- Nativní testy zdrojového kódu zůstávají 43/43; tato revize je dokumentační a nemění firmware.


## Doplnění revize 0.62 — bezpečný provisioning (2026-11-02)
- Přidán návrh fyzického provisioningu 32B párového tajemství, cílového chráněného úložiště, rotace/obnovy a podmínek před síťovou integrací.
- Cílová varianta: Flash Encryption Release + NVS Encryption na ověřeném ESP32 targetu. Samotné Preferences/NVS bez prokázané NVS Encryption není považováno za důvěrné úložiště.
- Rozlišeno šifrování dat v NVS od ochrany proti návratu starší kopie anti-replay žurnálu; CRC není kryptografická autentizace.
- Neprovedeno: změna partition table, eFuse, Flash Encryption, NVS Encryption, Secure Boot, produkční backend, fyzický provisioning ani síťová orchestrace.
- Nativní testy 43/43 prošly; firmware se neměnil.

## Kontrolní doplnění revize 0.65
- `TransferLogic` měl ochranu proti duplicitním ID pouze v RAM (8 záznamů); po restartu se persistentní historie transferů nevyužívala k obnovení této ochrany.
- Ochrana byla rozšířena na 12 ID a doplněna metoda `restoreRecentRequestIds()`. ESP32 #1 ji volá po obnovení historie z NVS, včetně pozdějšího zotavení NVS.
- Při pozdním zotavení NVS se persistentní ID sloučí s ID požadavků přijatých od startu; běžící/již zaznamenané požadavky se nezapomenou.
- Regresní testy: obnovení po restartu, ořez na posledních 12 ID, odmítnutí duplicit a zachování ID při pozdním načtení historie.
- `tests/run_native_tests.sh`: 43/43; kompletní AddressSanitizer/UndefinedBehaviorSanitizer: 43/43.
- Změněné `TransferLogic.h/.cpp` jsou bajtově shodné v `firmware/common/` a obou sketch `src/`.
- Skutečný Arduino-ESP32 build a fyzické NVS nebyly ověřeny; transfer zůstává SIMULACÍ a výkonové GPIO jsou neaktivní.


## Kontrolní stav po revizi 0.66
- `TransferRestartGuard` a `NvsTransferJournalStore` jsou implementovány a připojeny k ESP32 #1 transferové SIMULACI.
- PREPARED/ACTIVE se po restartu převádí na INTERRUPTED; RESULT_PENDING zůstává blokující, dokud se výsledek nepotvrdí v historii. Automatické pokračování není možné.
- Nový požadavek se odmítne, pokud žurnál není dostupný, je poškozený nebo čeká na potvrzení. Selhání zápisu při aktivaci vyvolá fail-safe abort a `VALVES_CLOSE_ALL`.
- Žurnál používá dvouslotový NVS záznam s CRC32, generačním čítačem a zpětným ověřením. CRC není ochrana proti úmyslnému rollbacku flash.
- Nativní sada: 45/45; kompletní ASan/UBSan: 45/45.
- Arduino-ESP32 build, fyzické NVS, power-cut test a hardware nebyly ověřeny. Výkonové GPIO zůstávají neaktivní.


## Kontrolní doplnění revize 0.67
- Dvouslotový transfer journal má rozšířené nativní testy pro namespace, zápis po poškození jednoho slotu a přetečení generačního čítače.
- Firmware logika se nemění; Arduino build, fyzické NVS a skutečný power-cut nejsou ověřeny.


## Kontrolní doplnění revize 0.69
- Nalezena a opravena inicializace `peerBootId` uvnitř `startPeerEthernet()`, která měnila boot epochu při reconnectu. Nyní se identita inicializuje jednou ze `setup()` a W5500 retry ji nemění.
- Aktuální shoda s MASTER 1.3 je rozepsána v `MASTER_ALIGNMENT_AUDIT_0.69.md`; starší matice 0.60 je historická.
- Nativní testy 45/45 a kompletní ASan/UBSan 45/45. Arduino build a fyzické ověření zůstávají neprovedené.

## Kontrolní doplnění revize 0.72 — W5500 link recovery
- Obě peer jednotky rozlišují `LinkOFF` od pouhé ztráty DHCP adresy; UDP socket se zavře při fyzickém výpadku linky.
- Po obnovení `LinkON` se DHCP inicializace spustí bez 60sekundového čekání. `Unknown` zachovává původní časovanou retry cestu.
- Vyžaduje Arduino Ethernet Library 2.0.0+ pro W5500/W5200 `linkStatus()`.
- Standardní nativní testy 45/45 a ASan/UBSan 45/45; statické kontroly boot identity a link recovery PASS.
- Arduino-ESP32 build a fyzický W5500 test zůstávají neprovedené.


## Kontrolní doplnění revize 0.73 — bezpečnostní audit peer vrstvy
- Opraven fail-open fallback anti-replay journalu při poškození jednoho slotu; nově se jakákoli korupce projeví jako chyba a handshake floor se neposkytne.
- Opraveno zneplatnění případného částečně zapsaného tajemství při chybě backendu.
- Hostitelské testy 45/45 standard + 45/45 ASan/UBSan.
- Produkční backend, fyzický provisioning, NVS Encryption, anti-rollback celé flash a síťová orchestrace zůstávají blokátory P0.

## Dodatečný bezpečnostní nález po revizi 0.73
- Otevřen P0-3: pokud chybí oba NVS replay sloty, aktuální adaptér vrací EMPTY a nelze rozlišit první spuštění od úplné ztráty žurnálu.
- Peer command path zůstává zakázaná. Další krok je návrh a test inicializačního markeru; marker v běžné NVS nebude vydáván za ochranu proti smazání celé partition nebo rollbacku celé flash.


## Kontrolní doplnění revize 0.75 — anti-replay initialization marker
- NVS replay store zapisuje marker `init` až po prvním ověřeném checkpointu. Marker + dva chybějící sloty znamenají CORRUPT; marker s neznámou hodnotou také.
- Přerušení po checkpointu a před markerem ponechá checkpoint čitelný; následný úspěšný save marker doplní.
- Nativní test pokrývá úplnou ztrátu obou slotů po inicializaci.
- Standardní testy 45/45; ASan/UBSan 45/45; boot identity a W5500 recovery PASS.
- Otevřené omezení: marker ve stejné NVS partition nechrání před smazáním celé partition ani rollbackem celé flash. Fyzický power-cut a Arduino build nebyly ověřeny.

## Revize 0.79 — integrační brána přístupových údajů
- Byla ověřena struktura DEV KIT 0.78: skutečné síťové/AP/webové call-sites nejsou v dodaném firmware stromu obsaženy.
- Z tohoto důvodu nebyl vytvořen izolovaný helper vydávaný za produkční opravu. Bez call-site by nešlo otestovat, že AP skutečně zůstane vypnuté při chybě NVS.
- Doplněna specifikace společného CredentialStore, rozlišení EMPTY/CORRUPT/IO_ERROR, read-back ověření, fail-closed chování a integrační testy.
- P0-4 až P0-6 zůstávají otevřené do získání skutečných síťových sketch souborů.
- Peer command path default-deny; fyzické výstupy neaktivní. Arduino build a hardware neověřeny.

## Revize 0.80 — bezpečnostní audit skutečných credential call-sites
- Auditní zjištění: oba hlavní `.ino` soubory obsahují reálné AP/Wi-Fi/web call-sites. Původní tvrzení revize 0.79 o jejich absenci bylo chybné a je opraveno.
- ESP32 #1: AP heslo se zobrazuje v HTML a vypisuje do Serial; AP se může spustit i po selhání trvalého uložení. Admin heslo se rovněž vypisuje do Serial. Domácí Wi-Fi credentials se zapisují bez read-back ověření.
- ESP32 #2: zápisy AP/admin údajů ignorují návratové hodnoty; AP/web se spouští bez ověření persistence; AP i admin hesla se vypisují do Serial.
- Obě jednotky používají HTTP Basic Auth nad HTTP; přenos není šifrovaný.
- Firmware v 0.80 beze změny. P0 provisioning a CredentialStore zůstávají otevřené.
- Nativní testy a ASan/UBSan 45/45; peer identity a W5500 recovery statické kontroly PASS.
- Arduino build, NVS/Flash Encryption a fyzické testy neprovedeny. Výkonové výstupy zůstávají neaktivní.

## Revize 0.81 — CredentialStore implementation gate
- Stav: návrh integrační smlouvy; firmware beze změny.
- Definovány stavy VALID/EMPTY/CORRUPT/IO_ERROR a zákaz automatického fallbacku na RAM hesla.
- Definován migrační postup staging → read-back → commit, zachovávající poslední ověřenou sadu.
- Otevřeno: fyzicky autorizovaný provisioning/obnova, implementace helperu a NVS adaptéru, integrace obou AP call-sites, NVS/Flash Encryption.
- Žádný Arduino build ani hardware test nebyl proveden.

### DEV KIT 0.83 — CredentialStore portable core
- Přidáno přenositelné jádro `CredentialStore.h` s dvouslotovým staging/read-back/commit.
- Přidán test `test_credential_store.cpp`; testuje EMPTY, write failure, read-back mismatch, úspěšný commit, zachování aktivního slotu při neúspěšné změně, CORRUPT a IO_ERROR.
- Standardní a ASan/UBSan test suite: 46/46.
- Peer boot identity a W5500 recovery testy: PASS.
- Není implementován NVS adapter ani integrace do skutečných AP call-sites. Není Arduino build ani hardware test. Nepovažovat credentials část za uzavřenou.

## Revize 0.98 — model výpadku napájení credential NVS
- Přidán hostitelský test přerušení při každém mutačním bodu změny přihlašovacích údajů (před/po zápisu). Po restartu je přijatelná pouze úplná stará nebo nová sada.
- Test modeluje perzistentní key/value úložiště; neověřuje skutečné chování NVS flash, brownout ani fyzický výpadek napájení.
- Firmware se nemění. Arduino build, fyzická NVS, NVS/Flash Encryption a hardware zůstávají neověřené.

## Revize 0.84 — Preferences/NVS adapter pro credentials
- Přidán `NvsCredentialBackend.h` s Arduino `Preferences` backendem v odděleném namespace `hwcCred`.
- Čtení kontroluje existenci klíče, délku, přesný počet bajtů, koncový NUL a nepřítomnost embedded NUL; zápis kontroluje vrácený počet bajtů; mazání kontroluje `remove()`.
- Adapter synchronizován ve společné složce a ve zdrojích ESP32 #1/#2.
- Přidán `test_nvs_credential_backend.cpp`; standardní testy 47/47, ASan/UBSan 47/47; peer boot identity a W5500 recovery PASS.
- Test Preferences adapteru používá host stub. Arduino-ESP32 build a fyzická NVS nebyly ověřeny.
- Adapter zatím není zapojen do `.ino` boot/AP call-sites; provisioning, odstranění úniků hesel a NVS/Flash Encryption zůstávají otevřené.


## Revize 0.85 — ověření výsledku commit credentials
- Opravena nejednoznačnost `CredentialStore::save()` při chybovém návratu backendu po fyzicky dokončeném zápisu `cred_active`: výsledek commit kroku se nyní určuje zpětným načtením přesného markeru.
- Přidán fault-injection test write-then-fail pro commit marker.
- Standardní testy 47/47; ASan/UBSan 47/47; peer boot identity PASS; W5500 recovery PASS.
- Změna je synchronizována pro obě ESP32. Není integrována do AP call-sites; provisioning, NVS/Flash Encryption, Arduino build a hardware test zůstávají otevřené.

## Revize 0.86 — rozšíření NVS credential adapter testů
- Rozšířeny testy Preferences adapteru o embedded NUL, chybějící terminátor, oversized payload, prázdný řetězec, limity zápisu, null argument, chování před/po lifecycle a prázdný namespace.
- Adapter správně odmítá poškozené payloady a délky mimo buffer; prázdný C řetězec je na úrovni adapteru syntakticky validní, ale CredentialStore jej odmítá validační politikou.
- Standardní testy 47/47; ASan/UBSan 47/47; peer boot identity PASS; W5500 recovery PASS.
- Produkční integrace do AP call-sites, provisioning/obnova, fyzická NVS, Arduino build a NVS/Flash Encryption zůstávají otevřené.
- Firmware funkčně nezměněn; peer command path default-deny; fyzické výstupy neaktivní.

## Revize 0.87 — servisní autorizační politika
- Přidána přenositelná jednorázová fyzická autorizace: 3 s podržení, 60 s okno, jednorázové spotřebování a nutnost uvolnění před dalším použitím.
- Časování je wrap-safe přes přetečení `millis()`.
- Modul je synchronizován pro ESP32 #1/#2, ale není připojen k žádnému GPIO ani `.ino` call-site.
- Host testy: standard 48/48, ASan/UBSan 48/48; peer boot identity PASS; W5500 recovery PASS.
- Fyzický servisní vstup, provisioning, AP integrace, Arduino build a NVS/Flash Encryption zůstávají neověřené.
- Peer command path default-deny; fyzické výstupy neaktivní.

## Revize 0.88 — servisní autorizace
- Opravena přesná hranice expirace 60s jednorázové servisní autorizace; autorizace je neplatná od okamžiku dosažení limitu.
- Testováno standardní sadou 48/48 a ASan/UBSan 48/48; peer boot identity a W5500 recovery PASS.
- Modul zůstává přenositelnou politikou bez GPIO/`.ino` integrace. Fyzický servisní vstup, provisioning a ochrana NVS/Flash nejsou ověřeny.

## Revize 0.89 — regresní audit servisní autorizace
- Rozšířeny testy o podržení pod limitem, přesné dosažení 3 s, nemožnost opakovaného získání autorizace při nepřetržitě aktivním vstupu, nové měření po uvolnění a expiraci přes přetečení `millis()`.
- Standardní testy 48/48 PASS; ASan/UBSan 48/48 PASS; peer boot identity a W5500 recovery PASS.
- Bez změny produkční politiky; hlavička je synchronizována pro obě ESP32.
- Fyzický GPIO vstup, provisioning/obnova credentials, AP integrace, Arduino build, skutečná NVS a NVS/Flash Encryption zůstávají neověřené.
- Peer command path default-deny; fyzické výstupy neaktivní.


## Revize 0.90 — transportně nezávislý provisioning controller
- Přidán `CredentialProvisioningController`: fyzická one-shot autorizace, max. 60s servisní relace, jeden pokus, fail-closed výsledky.
- Přidána privátní `eraseAllForPhysicalRecovery()` pro obnovu po přerušeném staging nebo poškození; selector se maže a ověřuje jako první, potom sloty, následně se ověřuje EMPTY. Metodu může volat pouze provisioning controller.
- `CredentialStore::clear()` používá `volatile` nulování; controller vynuluje předaný buffer na každé návratové cestě.
- Testy pokrývají první provisioning, obnovu částečného záznamu, přerušení obnovy, chybu mazání, IO_ERROR, změnu platných údajů, neplatný vstup a expiraci/wrap.
- Standardní nativní testy 49/49 PASS; ASan/UBSan 49/49 PASS; peer boot identity PASS; W5500 recovery PASS.
- Moduly jsou bajtově shodné v `firmware/common/`, `ESP32_1/src/` a `ESP32_2/src/`.
- Controller není zapojen do GPIO, `.ino`, HTTP, Serial ani AP boot call-sites. Fyzický servisní vstup a transport nejsou implementovány; stávající úniky hesel nejsou odstraněny.
- Arduino build, skutečná NVS, NVS/Flash Encryption a hardware nebyly ověřeny. Peer command path default-deny; fyzické výstupy neaktivní.

## Revize 0.91 — lokální sériový provisioning protokol
- Přidán `CredentialProvisioningSerialProtocol.h` do common a synchronizované kopie pro ESP32 #1/#2.
- Protokol používá pevný binární rámec s magic `HW`, verzí, SET_CREDENTIALS, délkou, třemi délkově prefixovanými ASCII fields a CRC-16/CCITT-FALSE.
- Parser bez heap/String, pevný max rámec 170 B, AP passphrase 8–63 znaků, 2s inter-byte timeout, kontrola struktury a ASCII; frame errors zavírají aktuální servisní session.
- Wrapper přijímá data pouze během dříve fyzicky autorizované controller session; neumí tuto autorizaci vytvořit. Jeden validní rámec je jeden provisionovací pokus. Odpovědi nesou pouze status, nikdy credentials.
- Host nativní testy: 50/50 PASS; ASan/UBSan 50/50 PASS; peer boot identity PASS; W5500 recovery PASS.
- Protokol není šifrovaný ani autentizovaný; CRC je pouze detekce náhodné chyby. Budoucí použití pouze přímý místní kabel, bez vzdáleného bridge.
- Není připojen do `.ino`, Serial, GPIO ani AP boot gate. Arduino build, UART/USB hardware a NVS nebyly ověřeny. Stávající úniky hesel na Serial a HTML zůstávají.
- Peer command path default-deny; fyzické výstupy neaktivní.


## Revize 0.92 — hostitelský klient provisioningu
- Hostitelský nástroj umí vytvořit SET_CREDENTIALS v1 rámec, ověřit vstupy/CRC a dekódovat odpověď zařízení.
- Hesla se zadávají skrytě přes `getpass`; nejsou v argumentech CLI ani ve výstupu. Výchozí režim je náhled bez odesílání.
- Odesílání vyžaduje výslovné `--firmware-integrated`; tento přepínač se v 0.92 nesmí použít, protože Serial/GPIO adapter není integrován do `.ino`.
- Python klient má 15/15 unit testů; Python bytecode compilation PASS. C++ testy standard i ASan/UBSan 50/50.
- PySerial/COM transport, DTR/RTS chování, fyzický servisní vstup, AP boot gate, skutečná NVS, Arduino build a hardware nejsou ověřeny.
- Stávající log/HTML úniky credentials zůstávají do ověřené provozní obnovy přístupu; peer command path default-deny a fyzické výstupy neaktivní.


## Revize 0.93 — board adapter a READY handshake pro provisioning
- Přidán obecný `CredentialProvisioningBoardAdapter.h`; zajišťuje fyzické vzorkování vstupu přes board-specific `readActive()`, one-shot controller, serial service a UART-like stream. Nepředpokládá konkrétní GPIO ani UART.
- Před otevřením relace zahazuje všechny předchozí RX bajty (max. 256 B/poll) a vyžaduje jeden klidný poll. Poté spotřebuje autorizaci a odešle stavový READY rámec (`RESULT 0x81`, status `0x01`).
- Aktivní request zpracovává v dávkách max. 64 B/poll; po výsledku, chybě nebo expiraci parser resetuje a zbylý vstup zahodí.
- Host klient čeká na READY a neposílá credentials bez platného READY. Čte přes omezený šum diagnostických logů.
- `./tests/run_native_tests.sh`: PASS, 51/51 C++ test programs; peer boot identity PASS; W5500 recovery PASS; credential module sync PASS.
- `sh tests/.run_native_tests_asan.sh`: PASS, 51/51 C++ test programs; peer boot identity PASS; W5500 recovery PASS; credential module sync PASS.
- Python klient tests: 17/17 PASS; Python bytecode compilation PASS.
- Adapter není zapojen do `.ino`. GPIO, UART port, baud rate a DTR/RTS nejsou ověřeny. Stávající Serial/HTML úniky hesel trvají; `--firmware-integrated` je nadále zakázáno.
- Arduino build, fyzická NVS, power-cut, NVS/Flash Encryption a hardware nebyly provedeny/ověřeny.


## Revize 0.94 — ověřená NVS brána pro start AP
- Přidán `CredentialStartupGate.h`: validace tisknutelného ASCII, kontrola délky, rozlišení read error/missing, exact write/read-back a fail-closed při chybě.
- Přidán `NvsCredentialTextStorage.h` pro stávající Preferences namespace; po uzavření adapteru se další read/write klasifikuje jako IO error.
- ESP32 #1 nyní spustí konfigurační AP pouze při ověřeném AP hesle z `hwc_ap/password` a ověřeném administrátorském hesle z `hwc_auth/admin`. Dočasné AP heslo pouze v RAM bylo odstraněno.
- ESP32 #2 nyní ověřuje `hwc_u2ui/ap_name`, `ap_pass` a `admin` před `WiFi.softAP`; při neplatném záznamu/NVS failu zůstane lokální AP/web vypnutý.
- Existující neplatné údaje se automaticky nepřepisují. Prvotně vytvořené údaje se zatím jednorázově zobrazí po úspěšném read-back; to zůstává přechodným Serial handoff do fyzicky ověřené provisioning integrace.
- ESP32 #1 dashboard již nezobrazuje AP heslo v HTML; zápis domácí Wi-Fi ověřuje read-backem SSID i hesla.
- Přidány `test_credential_startup_gate.cpp`, `test_nvs_credential_text_storage.cpp` a statická kontrola obou sketch AP gate call-sites.
- Standardní testy 53/53 PASS; ASan/UBSan 53/53 PASS; Python klient 17/17 PASS; py_compile PASS; peer boot identity PASS; W5500 recovery PASS; common/ESP32 credential modules sync PASS; AP gate static integration PASS.
- Omezení: hostitelské stub testy nejsou testem skutečné ESP32 NVS. Arduino překlad, fyzická NVS, power-cut, NVS/Flash Encryption, UART/GPIO a hardware nebyly ověřeny.
- Serial provisioning adapter zatím není integrován do `.ino`; peer command path zůstává default-deny a fyzické výstupy zůstávají neaktivní.


## Stav po revizi 0.94
- Credential startup gate je implementována ve skutečných AP call-sites obou `.ino` souborů a hostitelsky testována.
- ESP32 #1: bez ověřeného AP/admin NVS se konfigurační AP nespustí; AP heslo není v HTML. ESP32 #2: bez ověření tří credential keys se AP/web nespustí.
- Jednorázový Serial handoff nově generovaných hesel zůstává do dokončení fyzicky ověřeného provisioning workflow; nejde o úplné odstranění secret logů.
- Arduino build, fyzická NVS/power-cut, encryption a servisní vstup/transport nejsou ověřeny.


## Revize 0.95 — jednotné dvouslotové credential úložiště
- Přidán `CredentialLegacyMigrator.h`; jeho migrace je povolena pouze při `hwcCred == EMPTY`, odmítá `CORRUPT`/`IO_ERROR` a po commit/read-back ověřuje kompletní sadu.
- ESP32 #1 i ESP32 #2 nyní nejprve načtou `hwcCred`; po `VALID` legacy namespace vůbec nepoužívají. Při `CORRUPT` nebo `IO_ERROR` je legacy fallback zakázán.
- První migrace přebírá staré rozdělené klíče pouze při validní kompletní sadě. Staré klíče se nemažou, ale po migraci nejsou autoritativní.
- Jednorázový handoff nových hesel nastává až po dokončeném commitu do nového store; běžné bootování již hesla z nového store nevypisuje.
- Přidán host test migrátoru a statická kontrola skutečných `.ino` call-sites.
- Hostitelské testy ověřují politiku migrace; Arduino překlad, fyzická NVS, power-cut, encryption, UART/GPIO a hardware zůstávají neověřené.


## Revize 0.96 — entropy a síla nově generovaných přístupových údajů
- Nalezen problém v revizi 0.95: výchozí hesla se generovala přes `esp_random()` před startem Wi-Fi driveru; ESP-IDF 3.3.x negarantuje v tomto stavu true-random výstupy.
- Nový `CredentialRandom.h` vytváří 32 hex znaků ze čtyř 32bitových slov (128 bitů) a kontroluje kapacitu bufferu.
- Oba sketchy nyní před generováním chybějícího hesla vyžadují úspěšný start Wi-Fi STA driveru; pokud selže, nový credential set se nezaloží a AP zůstává fail-closed.
- Existující hesla se automaticky nemění. Jednorázový Serial handoff, plaintext NVS a nezapojený fyzický provisioning zůstávají otevřené.
- `./tests/run_native_tests.sh`: PASS, 56/56; `sh tests/.run_native_tests_asan.sh`: PASS, 56/56; Python klient 17/17; peer/W5500/credential sync/startup/entropy statické testy PASS.
- Arduino build, skutečný RNG, NVS, NVS/Flash Encryption a hardware nebyly ověřeny.


## Revize 0.97 — omezení opakovaných chybných HTTP přihlášení
- Přidán společný `WebAuthRateLimiter.h` a zapojen do `requireWebAuth()` ESP32 #1 a `requireLocalAuth()` ESP32 #2.
- 5 chybných pokusů/IP v 60s okně znamená 60s blokaci; během blokace HTTP 429. Úspěšné přihlášení vyčistí stav klienta.
- Osm slotů; aktivní blokace se nevytlačují. Při saturaci se nové IP dočasně odmítají, aby rotace zdrojů nemazala aktivní blokace.
- Standardní testy 57/57; ASan/UBSan 57/57; Python klient 17/17; py_compile, peer/W5500, credential sync/startup/entropy a auth integration PASS.
- Omezení: Basic Auth stále běží přes nešifrované HTTP; ochrana je pouze rate limit, nikoli TLS nebo úplná ochrana proti distribuovaným pokusům. Arduino build a hardware nebyly ověřeny.


## Aktuální stav po REV31 (13. 11. 2026)

Nouzové povolení všech tří čerpadel je po výslovném fyzickém potvrzení dostupné bez ohledu na softwarový stav hladinové automatiky (čidlo, stáří měření, kritický latch, konfigurace nebo NVS). V nouzovém režimu se u příslušného čerpadla obejde i freshness gate. Běžný režim zůstává beze změny. Nouzový stav je RAM-only a po restartu se ruší. Fyzická připravenost výstupního relé a nezávislé elektrické/mechanické ochrany motoru zůstávají zachovány.

## Revize 1.00 audit REV32
- Sjednocena bezpečnostní pravidla OTA: ESP32 #1 nyní přijímá pouze soubory `.bin`, blokuje aktualizaci při povolení běžného/nouzového provozu kteréhokoli místního čerpadla a během aktivního transferu nebo ručního režimu ventilů.
- Stavové podmínky se znovu kontrolují při každém datovém bloku a před finalizací OTA; při změně stavu se zápis přeruší.
- ESP32 #2 blokuje OTA během nouzového režimu horního čerpadla. Web ESP32 #1 upozorňuje na nutnost před aktualizací zastavit čerpání a zavřít ventily; restart dočasně odpojí bezpečnostní relé.
- Uživatel potvrdil, že fyzické přepínače pro úplné obejití ESP32 budou samostatnou hardwarovou nouzovou vrstvou. Firmware tuto funkci neimplementuje; konkrétní schéma zůstává otevřené.
- Nativní testy, ASan/UBSan, Python testy a statické integrační kontroly prošly. Arduino cílový build, skutečné OTA a fyzické testy zůstávají neprovedené.


## REV79 — souhrnný audit NVS (17. 11. 2026)
- Zmapována produkční NVS úložiště obou ESP32 včetně strategie zápisu, CRC/délkové validace, read-back a reakce na poškození.
- Bezpečnostní konfigurace, kritické západky, watchdog a transferový restart journal zůstávají při chybě fail-closed.
- Objemové kalibrace, denní přítok a historie transferů zůstávají odděleny od pumpových permitů.
- Jednoslotová úložiště bez rollbacku byla zdokumentována; jejich migrace na žurnály čeká na zjištění cílového Arduino-ESP32/ESP-IDF a fyzické power-cut testy.
- Nativní C++ 78/78 PASS; Python 17/17 PASS; ASan/UBSan 76/76 PASS; statické kontroly PASS.
- Arduino cílový překlad a fyzické testy nebyly provedeny.


## REV80 — úplný read-back elektrické kalibrace snímačů (17. 11. 2026)

- `NvsSensorConfigStore::save()` a `recover()` ověřují po zápisu úplnou významovou shodu všech polí `SensorConfig`.
- Zdroj je synchronizován ve společné knihovně a obou ESP32 sketchech; test pokrývá všechna pole.
- Nativní C++ 78/78 PASS; Python 17/17 PASS; ASan/UBSan 76/76 PASS; statické kontroly PASS.
- Bez změny NVS formátu, bezpečnostních prahů, výstupů a hardwaru. Arduino build a fyzické NVS testy zůstávají neprovedené.


## REV81 — přesné read-back ověření statistiky a historie (18. 11. 2026)
- `NvsTransferHistoryStore::save()` porovnává po zápisu všechna pole načteného snapshotu s kandidátem. Přidán test backendu, který vrátí plný počet zapsaných bajtů, ale ponechá starý platný blob; metoda správně odmítne úspěch a původní historie zůstane zachována.
- `NvsDailyInflowStore::save()` porovnává všechna pole snapshotu přímo, nikoli pouze odvozené CRC.
- Změny jsou synchronizovány ve společném zdroji a obou ESP32 `src` stromech. Bez změny formátu NVS, klíčů, pumpových permitů, ochranných prahů, GPIO, relé, ventilů a nouzových režimů.
- Hostitelské testy ověřují logiku aplikační vrstvy; nejde o skutečný test fyzické NVS ani brownoutu. Arduino cílový překlad a fyzické testy nebyly provedeny.


## REV82 — přesný read-back geometrie a objemové kalibrace studny (18. 11. 2026)
- `NvsWellCalibrationStore` po zápisu porovnává všechna významová metadata a všechny aktivní body; `mergeAndSave()` ověřuje stejnou úplnou shodu.
- `NvsWellGeometryStore` porovnává po read-backu magic/verzi, průměr, hloubku a checksum.
- Fault-injection testy ověřují případ, kdy NVS backend nahlásí plný zápis, ale ponechá starý validní záznam; starý záznam zůstává čitelný a operace hlásí chybu.
- Změny synchronizovány v `firmware/common` a obou ESP32 `src` stromech. Bez změny pumpových ochran, transferů, NVS formátu, GPIO, relé, ventilů nebo nouzových režimů.
- Nativní C++ 78/78 PASS; ASan/UBSan 76/76 PASS; statické kontroly PASS. Python/compileall/shell syntax kontrola se opakuje před vydáním.
- Arduino cílový překlad a fyzické NVS/hardware testy nebyly provedeny.


## REV83 — audit domácí Wi-Fi STA persistence (18. 11. 2026)
- Ověřeno přímo v `firmware/ESP32_1/ESP32_1.ino`: domácí SSID a heslo se ukládají samostatně do `hwc_wifi/ssid` a `hwc_wifi/password`. Read-back probíhá až po obou zápisech, takže neřeší výpadek napájení mezi nimi.
- Dvouslotový `CredentialStore` se týká AP/admin údajů, nikoli domácí STA dvojice. Rozlišení a doporučený postup opravy jsou v `docs/WIFI_CREDENTIAL_STORAGE_AUDIT_REV83.md`.
- REV83 nemění firmware ani NVS data. Nativní testy: 78/78 PASS; Python: 17/17 PASS; compileall a syntaxe obou shell runnerů PASS.
- Arduino cílový překlad, ASan/UBSan opakování v REV83 a fyzické NVS/brownout testy nebyly provedeny.
