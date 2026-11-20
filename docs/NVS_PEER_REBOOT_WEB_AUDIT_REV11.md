# HOME WATER CONTROLLER — NVS, peer reboot a web audit

Revize balíčku: 1.00 AUDIT REV12  
Kontrolovaný zdroj: DEV KIT 0.99 obsažený v REV11  
Závazná reference: HOME WATER CONTROLLER – MASTER 1.3 (28. 10. 2026)  
Rozsah: statická kontrola NVS adaptérů, obnovy po restartu, peer komunikace a webových cest.  
Změny firmwaru: žádné.

## 1. NVS – inventura a úroveň ochrany

### Dvouslotové / žurnálové zápisy

| Úložiště | Princip | Poznámka |
|---|---|---|
| Přihlašovací údaje (`CredentialStore` + `NvsCredentialBackend`) | Dva datové sloty a commit/selector postup s ověřením | Hostitelské testy pokrývají přerušení a poškození. Běžné Preferences/NVS není samo o sobě šifrované. |
| Transfer restart journal (`NvsTransferJournalStore`) | `slot0`/`slot1`, generace, CRC32, read-back, při nejasnosti fail-closed | Poškozený slot nevede k návratu na starší stav; nový transfer se zablokuje. |
| Peer replay checkpoint (`NvsPeerReplayStore`) | `slot0`/`slot1`, generace, CRC32 a init marker | Při poškození checkpointu se bezpečnostní stav nepřepisuje ani nesnižuje. |
| Ochrana studny (`NvsWellProtectionStore`) | Dva sloty s generací a CRC; migrace starého záznamu až po ověření nového | Uchovává konfiguraci, kritickou západku a platnost bezpečnostního stavu. |
| Ochrana horní nádrže (`NvsUpperTankProtectionStore`) | Dva sloty s generací a CRC; migrace starého záznamu až po ověření nového | Místní kritická západka ESP32 #2 se obnovuje samostatně od nastavitelných limitů. |

### Jednozáznamová úložiště

Následující adaptéry používají jeden blob na záznam (u senzorů samostatný klíč pro každý snímač):

- `NvsSensorConfigStore` – elektrická kalibrace snímačů;
- `NvsWellGeometryStore` – geometrie studny;
- `NvsWellCalibrationStore` – objemová kalibrační křivka studny;
- `NvsUpperTankLimitsStore` – limity horní nádrže;
- `NvsTransferHistoryStore` – historie transferů;
- `NvsDailyInflowStore` – denní přítok.

Tyto adaptéry ověřují velikost, formát a kontrolní součet/CRC; zápisy navíc ověřují načtený obsah. Předchozí platný záznam však není uložen v druhém aplikačním slotu. Read-back prokazuje, že lze přečíst právě zapsanou hodnotu; sám o sobě nezaručuje zachování předchozí hodnoty při ztrátě nebo poškození NVS položky.

### Závěr k výpadku napájení

- Hostitelské testy se stubem `Preferences` ověřují logiku validace, obnovy, chyb zápisu a některé scénáře přerušení.
- Nejde o fyzické přerušení napájení ESP32 během skutečného zápisu flash.
- Bez potvrzené verze Arduino-ESP32/ESP-IDF a skutečné partition table nelze tvrdit, že byly ověřeny konkrétní vlastnosti NVS na cílové desce.
- CRC/checksum detekuje náhodné poškození; není kryptografickou autentizací ani ochranou proti rollbacku.
- NVS Encryption/Flash Encryption nejsou potvrzeny. Přístupové údaje a další citlivé hodnoty v běžném Preferences nelze považovat za šifrované.

**Doporučení:** zachovat současné fail-closed chování. Před fyzickým provozem provést na samostatné testovací desce opakované power-cut testy všech jednozáznamových i dvouslotových úložišť, včetně migrace starších záznamů. Neprovádět nyní změnu partition table ani eFuse.

## 2. Peer komunikace a bezpečnostní handshake

Ve zdrojích existují samostatné, nativně testované moduly pro HELLO/CHALLENGE/FINISH/READY, odvození session klíčů, HMAC rámce, command session, korelaci odpovědí, retry a persistentní anti-replay checkpoint.

**Tyto moduly nejsou připojeny k síťové smyčce obou sketchů.** V `ESP32_1.ino` a `ESP32_2.ino` je aktuálně zapojeno pouze UDP vysílání/příjem stavového rámce horní nádrže. Stavový rámec používá CRC32, nikoli HMAC. `PeerSecretVault` nemá dokončený produkční backend, který by prokazoval důvěrné uložení sdíleného tajemství.

Důsledky:

- aktuální UDP status slouží pro zobrazování a simulační rozhodování, nikoli jako autentizovaný kanál řídicích povelů;
- nejsou implementovány vzdálené COMMAND/RESPONSE povely pro fyzické ovládání;
- fyzické výstupy zůstávají vypnuté;
- handshake, command session ani replay guard se nesmějí označit za produkčně integrované jen proto, že jejich samostatné moduly a hostitelské testy existují.

**Doporučení:** ponechat command path default-deny. Další bezpečný krok je transportně nezávislá integrační specifikace a testovací orchestrátor, následně až W5500 integrace po vyřešení bezpečného provisioningu a úložiště tajemství.

## 3. Chování při restartu ESP32 #2 během transferu horní → dolní

### Místní ochrana ESP32 #2

- Po startu se načítají limity horní nádrže z místního NVS.
- Kritická západka se obnovuje z odděleného místního úložiště.
- Konstruktor ochrany vyžaduje po startu nové ověření hladiny; povolení čerpadla se neobnoví jen na základě předchozího stavu RAM.
- Pokud bezpečnostní záznam chybí, vytvoří se až po platném měření na obnovovací hladině. Poškozený záznam se automaticky nenahrazuje.
- Nouzový režim čerpadla je stav RAM a po restartu se automaticky neaktivuje; vyžaduje nové vědomé potvrzení.

Toto se týká nouzového provozu čerpadla. Není to nouzové ruční ovládání ventilů hlavní jednotky.

### Chování hlavní jednotky ESP32 #1

- ESP32 #2 vytváří při každém bootu nové `bootId` a začíná sekvenci znovu.
- `PeerStatusMonitor` přijme nový boot epoch a uchovává omezenou historii nedávno ukončených epoch, aby opožděný starý UDP paket nevrátil stav zpět.
- Horní hladina se považuje za použitelnou pouze při čerstvém rámci a platném měření (limit 6000 ms).
- Pokud data zestárnou, `TransferLogic` obdrží `peerDataValid=false` a běžící transfer horní → dolní ukončí komunikační chybou. Obnovení komunikace samo nevytvoří nový transfer.
- Restart ESP32 #1 je navíc chráněn persistentním transferovým žurnálem PREPARED/ACTIVE/INTERRUPTED/RESULT_PENDING; po restartu se aktivní transfer nikdy automaticky neobnoví.

### Zjištěná mezera – rychlý restart peeru

Při přijetí nového `bootId` se aktuálně pouze vymění peer epoch v monitoru. Pokud ESP32 #2 naběhne a odešle nový platný rámec dříve, než šestisekundový limit způsobí zastarání dat, `TransferLogic` nedostane informaci, že se peer restartoval. Rozběhnutá simulace tak může pokračovat s novou peer epochou bez nového požadavku uživatele.

To je v rozporu s dříve stanoveným pravidlem projektu, že po výpadku/restartu peeru se transfer nemá automaticky obnovovat. Doporučené řešení: při změně `bootId` během aktivního transferu horní → dolní vyvolat bezpečné ukončení s důvodem COMMUNICATION/peer restart; obnovení dat pak dovolí až nový uživatelský požadavek. Tato podmínka se nesmí opírat o `pumpProtectionReady` – místní ochrana čerpadla je samostatná funkce a zůstává diagnostickým stavem peeru.

**V této revizi nebyl firmware změněn.** Zjištění je zdokumentováno před implementací.

## 4. Web a shoda s MASTER 1.3

### Již přítomné

- Hlavní web ESP32 #1 zobrazuje stav studny, dolní a horní nádrže, konfiguraci limitů, kalibraci, historii a SIMULACI transferu.
- Běžný transfer vyžaduje výslovný požadavek uživatele se zdrojem a litry.
- Horní limity rozlišují `pumpCriticalTripCm`, `pumpRecoveryCm`, `householdReserveCm` a `transferValveOpenCm`.
- Pomocná funkce `evaluateUpperTankTransferValve()` uplatňuje otevírací práh `transferValveOpenCm` a hysteretické držení povolení do dosažení rezervy `householdReserveCm`.
- ESP32 #2 má vlastní lokální servisní web a samostatnou nouzovou funkci pro čerpadlo.

### Chybějící / nespojené

1. **Nouzové ruční ovládání ventilů:** v hlavním webu není samostatná cesta ani uživatelské rozhraní pro ruční povel ventilům. Existující `/pump-emergency` se týká nouzového provozu čerpadla studny; `/pump-emergency` na ESP32 #2 se týká místní ochrany čerpadla horní nádrže. Ani jedna cesta není ručním ovládáním ventilů podle částí 40 a 52 Master 1.3.
2. **Ruční režim a fyzická kontrola hladin:** před implementací je nutné zachovat požadavek vědomé aktivace, potvrzení a výrazného označení nouzového ručního režimu. Při vadném čidle musí být zobrazena výzva k fyzické kontrole hladin. Režim nesmí obcházet ochrany, které lze zachovat. Nesmí se zaměnit s automatickým transferem ani s nouzovým povolením čerpadla.
3. **Horní ventilová hystereze není zapojena do transferového automatu:** web a hlavní smyčka vyhodnocují `upperValveDecision`, který používá práh otevření a rezervní hysterezi. `updateTransferSimulation()` však předává `TransferLogic` jako minimum pouze `householdReserveCm`; `TransferLogic::precheckFault()` ani `runningFault()` nepřebírají `upperValveDecision`. Proto může simulace horního transferu projít precheckem nad rezervou i v situaci, kdy pomocná ventilová logika při zavřeném ventilu ještě nepovolila otevření pod `transferValveOpenCm`. Doporučená oprava je propojit existující rozhodnutí/hysterezi s precheckem transferu, nikoli měnit hodnoty nebo význam uložených limitů.
4. **`pumpProtectionReady`:** zůstává diagnostickým údajem místní ochrany čerpadla na ESP32 #2. Nezavádět jej jako automatickou blokaci ventilu, pokud nejde o výslovně definovanou hydraulickou nebo bezpečnostní závislost. Kritická hladina čerpadla, rezerva pro objekt a hranice otevření ventilu zůstávají tři oddělené veličiny.
5. **Fyzické výstupy:** v obou sketchech je výslovně uvedeno, že fyzické výstupy jsou vypnuté. Současná stránka transferu je označena jako SIMULACE. Stav ventilu je pouze logický povel, nikoli potvrzená fyzická poloha.

## 5. Doporučené pořadí další práce

1. Přidat do `PeerStatusMonitor` jednoznačnou detekci změny boot epoch a nativní test; aktivní transfer horní → dolní při změně boot ID bezpečně ukončit. Zachovat oddělenost pump protection a transferové rezervy.
2. Propojit již existující `upperValveDecision` s precheckem/running logikou `TransferLogic`, včetně testů pod prahem, na prahu, v hysterezním pásmu a na rezervě. Nezměnit nastavené hodnoty ani jejich význam.
3. Navrhnout samostatný stavový automat a webové rozhraní pro nouzové ruční ovládání ventilů podle Master 1.3; před fyzickým výstupem provést bezpečnostní revizi. Ruční režim nesmí být implementován jako pouhé vypnutí všech kontrol.
4. Dokončit transportně nezávislé propojení handshake/session/replay modulů a otestovat jej hostitelsky; W5500 a tajemství zůstanou vypnuté do dokončení provisioning/NVS security.
5. Na skutečném hardware provést power-cut testy NVS a až po ověření platformy řešit případné zdvojení jednozáznamových úložišť.

## 6. Ověření

Tento dokument je statický audit zdrojů v REV11. Nejde o Arduino-ESP32 překlad ani test na cílové desce. Předchozí hostitelské testy zůstávají relevantní, protože firmware ani testovací zdroje nebyly touto dokumentační revizí změněny.
