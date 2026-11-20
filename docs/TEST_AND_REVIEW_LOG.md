# Průběžný kontrolní deník

## 2026-10-29 — kontrola revize 0.11
- Rozbalen ZIP a zkontrolována struktura, hlavní sketch ESP32 #1/#2, požadavky MASTER 1.3 a dokumentace.
- Spuštěno `tests/run_native_tests.sh`: všech 8 testovacích programů prošlo.
- Nalezeno: web/Wi-Fi základ existoval, ale za běhu po ztrátě domácí Wi-Fi se nepokoušel o obnovení připojení; dokumentace měla staré verze a nejednotné souhrny.

## 2026-10-29 — změny revize 0.12
- Přidána periodická obnova uloženého Wi-Fi připojení.
- Při nedostupné Wi-Fi se po 30 sekundách spouští konfigurační AP souběžně s pokusy o připojení.
- Zpřísněna validace vstupních čísel v webu.
- Sjednocena dokumentace o skutečném stavu a bezpečnostních omezeních.
- Opakované testy a kontrola archivu jsou součástí dokončení revize; ESP32 překlad a fyzické ověření stále čekají na příslušné prostředí/hardware.

## Povinné další kontrolní body
- Po každé změně přenositelných modulů: spustit nativní testy.
- Po každé změně webu/sítě: statická kontrola zdrojů a dostupných testů; nepovažovat ji za fyzický test.
- Před vydáním ZIP: znovu spustit celou testovací sadu, ověřit synchronizaci společných zdrojů, zkontrolovat dokumentaci a spustit `zip -T`.
- Před finálním označením: vyžaduje skutečný překlad pro konkrétní Arduino-ESP32 core a test na fyzických zařízeních; bez nich musí výsledek zůstat označen jako neověřený.


## 2026-10-29 — historie přítoku a kontrola revize 0.12
- Přidáno `NvsDailyInflowStore`: verzovaný záznam 30denní historie, CRC, validace dat a odmítnutí přepsání poškozeného existujícího záznamu.
- ESP32 #1 obnoví historii po startu, ale záměrně resetuje měřicí baseline; nepozorovaný interval při vypnutí tak není přičten k dennímu přítoku.
- SNTP se vyžádá při dostupné Wi-Fi. Neplatný systémový čas znamená nedatované přírůstky, nikoli falešné datum.
- Nativní testy po rozšíření: 9/9 prošlo. Všechny prošly i s AddressSanitizer/UndefinedBehaviorSanitizer.
- Oba `.ino` soubory prošly `g++ -fsyntax-only` proti dočasným Arduino API stubům. To zachytí část syntaktických chyb, ale nepotvrzuje kompatibilitu se skutečnou verzí Arduino-ESP32 ani běh na hardware.


## 2026-10-29 — protokol peer zpráv, revize 0.13
- Přidán a otestován `PeerProtocol` včetně CRC32, verze, ID zařízení, boot ID, sekvence, rozsahů a stáří jednotlivých měření.
- Nativní testy po rozšíření: 10/10 prošlo.
- Fyzický transport (W5500/Wi-Fi) a aplikační řízení transferů nejsou tímto modulem dokončeny.

- Pro peer zprávy doplněna explicitní serializace/deserializace do 48bajtového little-endian rámce a test poškozeného rámce.
- Dashboard ESP32 #1 doplněn o aktuální měření a přehled denního/30denního přítoku; jde o statickou kontrolu kódu, nikoli fyzický test webu.
- Denní historie nyní odstraňuje data starší než posledních 30 kalendářních dní a odmítá neplatné datumové klíče.

- Doplněna webová elektrická kalibrace snímačů studny a dolní nádrže s validací, zápisem do NVS, načtením zpět a okamžitým použitím. Existující poškozený záznam se nepřepisuje automaticky.
- Přidán `test_nvs_sensor_config_store`; celková nativní sada v revizi 0.13 má 11 testovacích programů.


## 2026-10-29 — OTA základ pro ESP32 #1, revize 0.14
- Přidán multipart OTA endpoint s autentizací, kontrolou zápisu a restartem až po úspěšném dokončení. Při chybě se aktualizace neaktivuje.
- Provedena syntaktická kontrola obou sketch souborů proti dočasným Arduino API stubům. OTA upload, partition scheme a skutečný build nejsou ověřeny.


## 2026-10-29 — logická ochrana studničního čerpadla, revize 0.15
- Přidána verzovaná konfigurace transferového minima, kritické hladiny a obnovovací hladiny. Vstupy se kontrolují vůči využitelné hloubce studny.
- Přidáno NVS úložiště konfigurace a kritické blokace s CRC; poškozený existující záznam se automaticky nepřepisuje.
- ESP32 #1 nyní vyhodnocuje logickou ochranu, zobrazuje stav a nabízí nouzový režim pouze s výslovným potvrzením fyzické kontroly při neplatném snímači. Nouzový režim se po restartu neobnoví.
- Fyzické relé čerpadla není ovládáno; elektrické chování relé a bezpečnostní výstup nejsou ověřeny.
- Nativní sada rozšířena na 12 testovacích programů; všechny prošly i s ASan/UBSan. Oba sketch soubory prošly hostitelskou syntaktickou kontrolou.

## Závěrečná kontrola balíčku 0.15
- `tests/run_native_tests.sh`: 12/12 testovacích programů prošlo.
- ASan/UBSan: všech 12 testovacích programů prošlo.
- Hostitelská syntaktická kontrola obou `.ino` souborů proti dočasným Arduino API stubům prošla.
- Synchronizace všech `firmware/common/*` do obou `src` složek ověřena.
- Neprovedeno: skutečný Arduino-ESP32 build, W5500, Wi-Fi/AP na zařízení, NVS na zařízení, OTA upload na zařízení, čidla/relé/ventily/čerpadla a end-to-end transfery.


## 2026-10-29 — W5500 peer status, revize 0.16
- Přidána prozatímní DHCP/UDP broadcast vrstva přes W5500: ESP32 #2 vysílá stav každé 2 s, ESP32 #1 přijímá a zobrazuje jej ve webu.
- Neplatný objem/teplota a nepřipravená místní ochrana ESP32 #2 jsou explicitně označeny, nevyplňují se odhadovanými nulami jako platnými daty.
- Hostitelská syntaktická kontrola obou sketch souborů proti dočasným stubs prošla; skutečný Arduino build, DHCP, broadcast a hardware nebyly testovány.


## 2026-10-29 — revize 0.17: omezení zápisů historie do NVS
- Při opakované kontrole ESP32 #1 jsem našel, že `inflowDirty` bylo nastaveno při každém vzorku (každé 2 s) a historie se zapisovala každou minutu i bez změny.
- Nahrazeno samostatným `InflowPersistencePolicy`: NVS se označí jako změněné pouze při kladném přírůstku nebo při změně platného kalendářního dne; zápisy jsou omezené na nejvýše jednou za 5 minut. Neplatný čas nevytváří falešný den.
- Přidán `test_inflow_persistence_policy` pro stabilní vzorky, pozitivní změnu, přechod dne, neplatný čas, omezení frekvence a přetečení `millis()`.
- Celá nativní sada po opravě: 13/13 prošlo; nový modul prošel také ASan/UBSan. Oba `.ino` soubory prošly hostitelskou `g++ -fsyntax-only` kontrolou proti dostupným dočasným Arduino API stubům. Skutečný Arduino build a test na ESP32 stále nebyly provedeny.


## Revize 0.18 — 2026-10-29
- Změna: během transferu se hladina dolní nádrže mimo 0–121 cm odmítne jako neplatná; automat nehlásí falešné dokončení při přestřeleném/porouchaném měření.
- Přidán regresní případ do `test_transfer_logic.cpp`.
- Kompletní nativní sada: 13/13 prošla.
- Transferový test prošel ASan/UBSan.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům; nejde o skutečný Arduino-ESP32 build.
- Společné zdrojové soubory jsou byte-identické v obou sketch složkách.


## Revize 0.19 — lokální fail-closed ochrana ESP32 #2
- Do ESP32 #2 integrováno lokální vyhodnocení `WellPumpProtection`; není závislé na heartbeat ESP32 #1.
- Rozšířeno `NvsWellProtectionStore` o volitelný název namespace; výchozí `hwc_protect` zůstává zachován pro ESP32 #1, horní jednotka používá samostatný `hwc_upperprot`.
- Využitelná hloubka horní nádrže je záměrně nastavena jako neznámá (0), dokud nebude potvrzena geometrie. Ochranná konfigurace proto není platná a logické povolení čerpadla zůstává blokováno. Žádný fyzický výstup není aktivován.
- Nativní testy: 13/13 prošlo. Test `NvsWellProtectionStore` nově ověřuje, že horní jednotka používá samostatný namespace a nepřekračuje limit 15 znaků Preferences. Hostitelská syntax kontrola obou sketch souborů prošla proti Arduino API stubům. Skutečný Arduino-ESP32 build, NVS na zařízení a hardware nebyly testovány.


## Revize 0.20 — nastavitelné meze horní nádrže
- Přidán validovaný model `UpperTankLimits`: využitelná hloubka, kritická a obnovovací hladina čerpadla, domácí rezerva a hladina zahájení přepouštění.
- Přidáno NVS úložiště s verzí/CRC a ochranou proti automatickému přepsání poškozeného záznamu.
- Web ESP32 #1 umožňuje limity nastavit a uložit s kontrolním načtením. Logika přepouštěcího ventilu používá hysterezi a při neplatném/stárnoucím peer měření zůstává blokovaná.
- Nativní testy 15/15 prošly; oba sketch soubory prošly hostitelskou syntaktickou kontrolou.
- Otevřené: bezpečná synchronizace těchto limitů do ESP32 #2, místní aplikace kritických mezí na čerpadlo horní nádrže, fyzické výstupy, skutečný Arduino build a hardware.


## Revize 0.21 — lokální ochrana horního čerpadla
- Přidán samostatný `UpperTankProtection`, který vyhodnocuje `UpperTankLimits` přímo na ESP32 #2.
- Po startu i po neplatném snímači/výstupu z rozsahu je povolení blokováno do dosažení obnovovací hladiny; kritická hladina aktivuje blokaci s hysterezí.
- Záměrně nebyl přidán neautentizovaný UDP zápis limitů. Konfigurace ESP32 #1 a #2 je v oddělených NVS a zatím není bezpečně synchronizována.
- Nativní sada 16/16 prošla; nový test prošel ASan/UBSan; oba sketch soubory prošly hostitelskou syntaktickou kontrolou. Společné soubory jsou synchronizovány.
- Neprovedeno: Arduino-ESP32 build, fyzické NVS/senzory/W5500/relé a end-to-end test.


## Revize 0.22 — nouzový režim horního čerpadla
- Rozšířena logika horní ochrany o potvrzený nouzový režim podobný ochraně studny. Bez explicitního potvrzení se režim neaktivuje; nouzové povolení neobchází kritickou blokaci, chybu konfigurace ani NVS. Po restartu je nouzový režim vypnutý.
- `tests/run_native_tests.sh`: 16/16 testovacích programů prošlo.
- Nový test horní ochrany prošel ASan/UBSan.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům.
- Společné zdrojové soubory jsou synchronizovány v obou sketch `src` složkách.
- Otevřeno: nouzové ovládání není napojeno na UI/komunikační vrstvu; fyzické výstupy, skutečný Arduino build a hardware nejsou ověřeny.


## 2026-10-29 — revize 0.23: simulační obrazovka transferu
- Integrovaný stavový automat `TransferLogic` do webového workflow ESP32 #1 v explicitním režimu SIMULACE. Uživatel volí zdroj, litry a timeout; lze spustit/zastavit a zobrazit stav, cílovou hladinu, odhad přírůstku a důvod ukončení.
- Vstupy zohledňují platnost a stáří snímačů, peer freshness, transferové minimum studny / domácí rezervu horní nádrže, kritickou ochranu a konfiguraci. Fyzické GPIO zůstává nedotčeno.
- `tests/run_native_tests.sh`: 16/16 prošlo. Oba sketch soubory prošly `g++ -fsyntax-only` proti dočasným Arduino API stubům.
- Omezení: UI handler není end-to-end testován na reálném WebServeru; žádný Arduino-ESP32 build, W5500, Wi-Fi, fyzický senzor ani výstup nebyl ověřen.


## 2026-10-29 — revize 0.24: místní UI horní jednotky
- ESP32 #2 dostala vlastní lokální Wi-Fi AP a Basic Auth web UI. UI umožňuje ukládat/ověřit limity horní nádrže a výslovně potvrdit nebo ukončit nouzový režim při poruše snímače.
- Nouzové povolení vyžaduje již zjištěnou neplatnost snímače, potvrzení fyzické kontroly, platné místní limity/NVS a nesmí obcházet kritickou blokaci. Po restartu je nouzový režim vypnutý.
- Změna limitů resetuje logickou ochranu do startovacího blokování do dosažení obnovovací hladiny. Fyzické relé zůstává neaktivní.
- Otevřené bezpečnostní omezení: limity v ESP32 #1 a ESP32 #2 se nesynchronizují automaticky; uživatel je musí zatím nastavit shodně.
- Nativní sada 16/16 a hostitelská syntaktická kontrola obou `.ino` souborů prošly. Skutečný Arduino build, Wi-Fi AP, NVS a fyzické ovládání nebyly ověřeny.


## 2026-10-29 — revize 0.25: diagnostika neshody horních limitů
- Peer status rozšířen z 48 na 52 bajtů o kontrolní součet lokálních limitů horní nádrže. ESP32 #2 jej vysílá pouze jako diagnostický údaj; žádné vzdálené nastavení nepřijímá.
- ESP32 #1 porovnává vlastní a peer checksum; neshoda/absence konfigurace blokuje simulaci transferu z horní nádrže a zobrazí varování.
- Protokol byl povýšen na verzi 2, aby se nový 52B rámec nezaměnil se starým 48B protokolem. `tests/run_native_tests.sh`: 16/16 prošlo; test PeerProtocol prošel ASan/UBSan; oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou; společné zdroje jsou synchronizované a ZIP integrity check prošel. Skutečný Arduino-ESP32 build a hardware nejsou k dispozici.


## 2026-10-29 — revize 0.26: základní OTA upload ESP32 #2
- Přidán místní upload `.bin` přes Arduino `Update`, Basic Auth, kontrola výsledku zápisu, abort při přerušení a restart až po odeslání potvrzení.
- Zachována diagnostika checksum limitů horní nádrže z protokolu peer v2.
- `tests/run_native_tests.sh`: 16/16 prošlo; test PeerProtocol prošel ASan/UBSan; oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou; společné zdroje jsou synchronizované. Skutečný Arduino build, partition a fyzické OTA nejsou ověřeny.


## 2026-10-29 — revize 0.27: omezení podmínek OTA
- ESP32 #2 odmítá upload bez `.bin` přípony; OTA se nezahájí při logickém povolení čerpadla nebo aktivním nouzovém režimu a zápis se při změně stavu přeruší.
- `tests/run_native_tests.sh`: 16/16 prošlo; oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou; společné zdroje jsou synchronizované. Skutečné OTA, partition a cílový build nejsou ověřeny.


## 2026-10-29 — revize 0.28: persistentní kritická blokace horního čerpadla
- Při bezpečnostní revizi bylo zjištěno, že `UpperTankProtection` měl kritickou blokaci pouze v RAM. Po restartu s neplatným snímačem by tak nouzové potvrzení mohlo ztratit informaci o předchozím kritickém poklesu.
- Přidán `NvsUpperTankProtectionStore` v samostatném namespace `hwc_up_safety`, s verzí, CRC, kontrolním načtením a odmítnutím automatického přepsání poškozeného záznamu.
- ESP32 #2 načítá kritickou blokaci při startu a ukládá její změny. Při chybějícím záznamu se bezpečnostní stav inicializuje až po platném měření na obnovovací hladině; nouzové povolení do té doby zůstává blokováno.
- `tests/run_native_tests.sh`: 17/17 testovacích programů prošlo. Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti Arduino API stubům. Společné soubory jsou synchronizované.
- Neprovedeno: skutečný Arduino-ESP32 build, NVS na fyzickém ESP32, snímače, W5500, relé, ventily, čerpadla a end-to-end test.

## 2026-10-31 — revize 0.29: fyzický rozsah zdrojové nádrže v transferu
- Kontrola revize 0.28 odhalila mezeru: `TransferLogic` ověřoval dolní nádrž, platnost/stáří zdroje a jeho transferové minimum, ale neověřoval horní fyzickou mez hladiny zdrojové nádrže. Konečná hodnota mimo skutečný rozsah mohla být přijata jako platný zdroj.
- `TransferInputs` nyní obsahuje `sourceMaximumLevelCm`. Neplatná/nelogická konfigurace horní meze transfer blokuje jako `SYSTEM_NOT_READY`; zdrojová hladina pod nulou nebo nad fyzickou mezí vyvolá `SENSOR_INVALID` při předběžné kontrole i během běhu.
- ESP32 #1 předává horní mez z `UpperTankLimits.usableDepthCm` nebo mez studny z `WellGeometry.usableDepthCm`. Žádný fyzický výstup nebyl připojen.
- Přidány tři regresní případy: zdroj nad maximem před startem, neplatná konfigurace maxima a překročení maxima během transferu.
- `tests/run_native_tests.sh`: 17/17 prošlo. Celá sada prošla také ASan/UBSan. Společné zdrojové soubory jsou byte-identické v obou sketch `src` složkách.
- Hostitelská syntaktická kontrola `.ino` nebyla v tomto běhu zopakována, protože dočasné Arduino API stuby nejsou v aktuálním pracovním prostředí dostupné. Skutečný Arduino-ESP32 build a hardware zůstávají neověřeny.


## 2026-10-31 — revize 0.30: nezávislá čerstvost transferových snímačů
- Při kontrole integrační logiky `updateTransferSimulation()` bylo zjištěno, že proměnná `samplesFresh` spojovala dostupnost a stáří snímače studny a dolní nádrže do jedné podmínky. Tím neplatný snímač studny blokoval i transfer z horní nádrže.
- Přidány samostatné časové značky `lastWellReadingMs` a `lastLowerTankReadingMs`; cílová dolní nádrž se validuje nezávisle a zdroj podle zvoleného směru.
- Přidán `SampleFreshness.h` s wrap-safe kontrolou `millis()` a `test_sample_freshness` včetně hranice stáří, nedostupného vzorku, nezávislosti čidel a přetečení čítače.
- `tests/run_native_tests.sh`: 18/18 prošlo. ASan/UBSan celá sada.
- Společné soubory jsou synchronizovány v obou sketch `src` složkách. Fyzické GPIO zůstává neaktivní.
- Skutečný Arduino-ESP32 build a fyzické testy nebyly provedeny. Hostitelská syntax kontrola `.ino` nebyla v této revizi provedena.


## 2026-10-31 — revize 0.31: trvalá historie transferové simulace
- Kontrola revize 0.30 ukázala, že terminální výsledek transferové simulace nebyl po restartu uchován. Přidán `TransferHistory` s kruhovou kapacitou 12 záznamů a NVS úložiště `hwc_xfer`.
- Záznam obsahuje request ID, zdroj, konečný stav, důvod, požadované litry, odhad přírůstku dolní nádrže, cílovou hladinu a uptime při ukončení. Dashboard ukazuje posledních pět záznamů. Jde výhradně o SIMULACE, nikoli skutečné transfery.
- NVS záznam má verzi a CRC32; neplatný existující záznam se nepřepisuje. Pokud NVS není dostupné, záznam se drží jen v RAM a je to uvedeno v Serial Monitoru.
- Nativní sada: 20/20 testovacích programů prošlo. ASan/UBSan: celá sada prošla. Společné soubory jsou synchronizované v obou sketch `src` složkách.
- Hostitelská syntax kontrola `.ino` a skutečný Arduino-ESP32 build nebyly provedeny. Fyzické GPIO, NVS, W5500, WebServer a senzory nebyly ověřeny na hardware.


## 2026-10-31 — revize 0.32: opakování NVS ukládání historie simulace
- Kontrola revize 0.31 odhalila chybu: request ID se označilo jako zpracované před úspěšným uložením, takže selhání NVS nemělo další pokus. Při nedostupném NVS při startu se úložiště zkoušelo otevřít jen jednou.
- ESP32 #1 nyní drží čekající historii jako dirty, ukládání opakuje nejdříve po 5 s a NVS zkouší znovu otevřít po 30 s. Po obnovení nejprve načte/ověří existující historii, sloučí RAM záznamy bez duplicit a odmítne přepsat poškozený záznam.
- `tests/run_native_tests.sh`: 20/20 prošlo. Celá sada prošla AddressSanitizer/UndefinedBehaviorSanitizer.
- `.ino` syntax kontrola nebyla provedena; skutečný Arduino-ESP32 build, NVS na zařízení a hardware nebyly ověřeny. Fyzické výstupy zůstávají neaktivní.


## 2026-10-31 — revize 0.33: fyzická horní mez ochrany čerpadla studny
- Bezpečnostní revize odhalila, že `WellPumpProtection` kontroloval nezápornou a konečnou hladinu, ale neměl vlastní limit fyzické hloubky. Měření nad hloubkou studny tak mohlo být přijato jako použitelné.
- `PumpProtectionInputs` nyní obsahuje `maximumLevelCm`; logika vyžaduje platnou mez větší než obnovovací hladina a hladinu v rozsahu `0..maximumLevelCm`. ESP32 #1 předává `wellGeometry.usableDepthCm` a měření mimo tento rozsah označí za neplatné.
- Nouzový režim lze výslovně potvrdit i při hodnotě mimo fyzický rozsah, ale jen s platnou konfigurací, dostupným bezpečnostním úložištěm a bez kritické blokace. Bez potvrzení zůstává čerpadlo logicky blokováno.
- Přidány regresní testy pro hladinu nad fyzickým maximem, odmítnutí bez potvrzení a odmítnutí neplatného maxima. `tests/run_native_tests.sh`: 20/20 prošlo.
- Společné soubory synchronizovány do obou sketch `src` složek. Skutečný Arduino-ESP32 build, `.ino` syntax kontrola a hardware nebyly provedeny; fyzické výstupy zůstávají neaktivní.


## 2026-10-31 — revize 0.34: admission guard příkazů peer
- Přidán `PeerCommandGuard` jako transportně nezávislá vstupní kontrola metadat budoucích příkazů: vyžaduje kladný výsledek externí autentizace, cílový boot ID aktuální jednotky, nenulové ID/sekvenci, monotónní sekvenci v rámci bootu odesílatele a deduplikaci posledních 16 request ID.
- Přidán `test_peer_command_guard` pro odmítnutí bez autentizace, duplicitní request ID, replay sekvence, chybný cílový boot ID, neplatné ID a změnu boot ID odesílatele.
- Důležité omezení: žádná HMAC/verifikační implementace není součástí modulu; `authenticated` musí být dodáno budoucím ověřovačem. Cache request ID je volatilní a není ochranou proti opakování přes restart. Modul není zapojen do sítě ani výstupů.
- Nativní sada: 21/21 testovacích programů prošlo.
- Závěrečná kontrola revize 0.34: všech 21 testovacích programů prošlo také při kompilaci a běhu s AddressSanitizer + UndefinedBehaviorSanitizer.
- Všechny soubory `firmware/common/*` jsou byte-identické v obou sketch `src` složkách. V obou `.ino` souborech zůstávají volání GPIO omezena na reset W5500; žádné čerpadlo ani ventil se neovládá.
- Arduino-ESP32 build, syntax kontrola `.ino`, W5500 síť a fyzické zkoušky nebyly provedeny.


## 2026-10-31 — revize 0.35: HMAC-SHA-256 primitivní modul
- Přidán samostatný HMAC-SHA-256 výpočet a konstantně časové porovnání 32bajtových tagů. Testy odpovídají známým RFC 4231 vektorům (krátké klíče, textové zprávy i klíč delší než 64 bajtů).
- Nativní sada: 22/22 testovacích programů prošlo.
- HMAC zatím není zapojen do peer transportu; bezpečná správa klíče a kanonický formát příkazu zůstávají otevřené.
- Arduino-ESP32 build, W5500 a hardware nebyly ověřeny.


## 2026-10-31 — revize 0.36: kanonický autentizovaný rámec
- Přidán 68bajtový little-endian `PeerCommandFrame` s HMAC-SHA-256 a ověřením před dekódováním.
- Nový test ověřuje round-trip, integritu podepsaných dat, délku, minimální klíč a povinná ID.
- Nativní sada: 23/23 testovacích programů prošlo.
- Síťová integrace, správa klíčů a fyzický provoz zůstávají mimo rozsah této revize.


## 2026-10-31 — revize 0.37: explicitní peer session a identita
- `PeerCommandGuard` nově kontroluje ID odesílatele i cíle, lokální boot ID, ustavené peer boot ID a sekvenci pouze v rámci explicitní session. Nový boot peeru se nepřijímá automaticky.
- Přidány regresní testy pro neustavenou session, odmítnutí neautentizovaného ustavení session, self-pairing, chybná ID zařízení, peer reboot, duplicity request ID a replay rámce určeného předchozímu lokálnímu bootu.
- Nativní testovací programy: 23/23; celá sada prošla AddressSanitizer a UndefinedBehaviorSanitizer.
- Skutečný Arduino-ESP32 build, síťový handshake, NVS persistence anti-replay a hardware nebyly provedeny.


## 2026-11-01 — revize 0.38: vzájemný challenge-response handshake
- Přidán `PeerHandshake.*`: HELLO nonce iniciátora, HMAC autentizovaný CHALLENGE responderu, HMAC FINISH iniciátora a odvození společného 32B session key z transcriptu s doménovým oddělením. Transcript zahrnuje identity a boot ID obou zařízení i oba 128bitové nonce.
- Testy ověřují shodný session key na obou stranách, odmítnutí nesprávného tajemství, změněného boot ID, poškozeného FINISH, nulových/stejných nonce, neplatných identit a slabého klíče.
- Nativní sada: 24/24 testovacích programů prošlo.
- Nonce/boot ID RNG, provisioning tajemství, wire encoding, timeout, síťová integrace, Arduino-ESP32 build a hardware nebyly provedeny. Žádné výstupy nejsou aktivovány.


## 2026-11-01 — revize 0.39: kanonické wire kódování handshake
- Přidán `PeerHandshakeWire.*`: HELLO 28 B, CHALLENGE 80 B, FINISH 38 B, hlavička `HWH1`, verze/typ a explicitní little-endian serializace.
- Dekodéry odmítají chybné délky, magic, verzi, typ a neplatné identity/boot ID/nonce; při chybě nulují výstup.
- Rozšířen test o kompletní challenge-response po wire encode/decode a ověření shodného session key na obou stranách.
- Nativní testovací programy: 24/24 prošly. Celá sada prošla AddressSanitizer + UndefinedBehaviorSanitizer.
- Společné moduly synchronizovány do obou sketch `src` složek. Arduino-ESP32 build, W5500 transport, timeouty, provisioning a fyzický hardware nebyly ověřeny. Žádné výkonové výstupy nejsou aktivní.


## 2026-11-01 — revize 0.40: fail-closed politika úložiště peer tajemství
- Přidán `PeerSecretVault.*`: 32B nenulový klíč, vazba na uspořádanou dvojici ID, odmítnutí nechráněného backendu, povinné fyzické potvrzení pro provisioning/mazání a explicitní nulování tajemství v RAM.
- Test `test_peer_secret_vault` ověřuje nechráněné úložiště, potvrzení, neplatné ID, vazbu klíče na dvojici, mazání a nulování.
- Nativní sada: 26/26 testovacích programů prošlo. Celá sada prošla AddressSanitizer + UndefinedBehaviorSanitizer.
- Nový modul synchronizován do obou sketch `src` složek; byte-identita všech common modulů ověřena následnou kontrolou.
- Důležité omezení: konkrétní ESP32 backend, NVS encryption/flash encryption, fyzický provisioning a rotace klíče nejsou implementovány ani ověřeny. Modul je pouze fail-closed kontrakt; žádné síťové příkazy ani výkonové výstupy nejsou povoleny. Arduino-ESP32 build a hardware nebyly ověřeny.


## 2026-11-01 — revize 0.41: odpovědi příkazového protokolu
- Přidán `PeerCommandResponse.*`: ACK/REJECTED/FAILED, enumerované důvody a kontrola korelace request ID, původního kódu příkazu, ID zařízení a boot ID.
- Testuje se platná/zakázaná kombinace statusu a důvodu, chybný cíl/boot ID, sekvence, změněné request ID, nesoulad příkazu, HMAC encode/decode a odmítnutí pozměněného autentizovaného rámce.
- Nativní testovací programy: 27/27. Skutečný Arduino-ESP32 build, síť ani hardware nebyly ověřeny.
- Nový modul synchronizován do obou sketch `src`; žádný síťový příkaz ani fyzický výstup není aktivován.


## 2026-11-01 — revize 0.42: timeout handshake
- Přidán wrap-safe `PeerHandshakeTimeout` (limity 1..INT32_MAX ms) a `TimedPeerHandshakeResponder`, který ruší pending challenge po timeoutu a odmítá FINISH přijatý na hranici/po lhůtě.
- Testy ověřují timeout, přesnou hranici, wrap-around `millis()`, validní FINISH před lhůtou, jednorázové přijetí, cancel a nulování session key při odmítnutí.
- Nativní sada: 26/26 testovacích programů prošlo běžným během i AddressSanitizer/UndefinedBehaviorSanitizer.
- Nové soubory synchronizovány do obou sketch `src` složek.
- Arduino-ESP32 build, UDP/W5500, NVS encryption, provisioning a hardware nebyly ověřeny; žádné výkonové výstupy nejsou aktivní.


## 2026-11-01 — revize 0.43: oddělené směrové klíče peer session
- Přidán `PeerSessionKeys.*`: z ověřeného session key odvozuje dva nezávislé HMAC-SHA-256 klíče s odlišnými doménovými štítky a kontextem device ID/boot ID obou stran.
- Test `test_peer_session_keys` ověřuje shodné odvození obou stran, rozdílnost klíčů pro oba směry, vazbu na kontext, odmítnutí neplatných parametrů a nulování výstupů. `test_peer_command_frame` navíc ověřuje, že COMMAND rámec podepsaný klíčem I→R nelze ověřit klíčem R→I.
- Nativní sada: 27/27 testovacích programů prošlo běžným během i ASan/UBSan.
- Společné soubory synchronizovány do obou sketch `src` složek a jejich byte-identita ověřena.
- Směrové klíče nejsou připojeny k command/response rámcům ani W5500; Arduino-ESP32 build a hardware nebyly ověřeny. Žádné výkonové výstupy nejsou aktivní.


## 2026-11-01 — revize 0.44: session-bound directional command codec
- Přidán `PeerSessionCodec.*`: výběr I→R/R→I klíče podle role, kontrola lokální/peer identity a boot ID při encode/decode, nulování výstupu při chybě. Interní klíče se nulují v destruktoru a třída zakazuje kopírování.
- `test_peer_session_codec` ověřuje initiator→responder COMMAND, responder→initiator RESPONSE, odmítnutí opačného směru, nesprávného peer boot ID a neplatného kontextu.
- Standardní nativní runner: 31/31 programů prošlo. Kompletní ASan/UBSan sada nebyla dokončena v časovém limitu; nový codec test byl samostatně spuštěn se sanitizéry.
- Common moduly synchronizovány do obou sketch `src` složek a byte-identita ověřena. Žádné síťové příkazy ani výkonové výstupy nejsou aktivní. Arduino-ESP32 build, W5500 a hardware nebyly ověřeny.


## 2026-11-01 — revize 0.45: command session admission pipeline
- Přidán `PeerCommandSession.*`: společný vstupní bod, který spojuje `PeerSessionCodec` a `PeerCommandGuard`. Aktivace vyžaduje volající vrstvou potvrzený čerstvý autentizovaný handshake; příchozí rámec projde HMAC a peer identity/boot ID kontrolou, musí být typu COMMAND a následně projde sekvencí/request-ID guardu. Data se aplikaci předají jen při přijetí. RESPONSE rámce se odmítají a čekají na samostatnou korelační vrstvu.
- `PeerSessionCodec` doplněn o `reset()`/`clear()`. `PeerCommandGuard` doplněn o `endSession()`, který ukončí session a resetuje sekvenci, ale zachová posledních 16 request ID v RAM pro reconnect v rámci stejného běhu. Po restartu se tato cache ztratí; persistentní anti-replay zůstává otevřený.
- `tests/run_native_tests.sh`: 32/32 testovacích programů prošlo standardním během. Nový `test_peer_command_session` samostatně prošel ASan/UBSan. Kompletní sanitizovaný runner byl přerušen časovým limitem po 24 testech; nelze tvrdit, že celá sada prošla sanitizéry.
- Všechny common soubory jsou synchronizovány do obou sketch `src` složek a byte-porovnány. ZIP integrity check prošel.
- Arduino-ESP32 build, handshake/W5500 integrace, chráněné úložiště klíče, persistentní anti-replay a fyzické testy nebyly provedeny. Výkonové výstupy zůstávají neaktivní.


## 2026-11-01 — revize 0.46: korelace peer požadavků a odpovědí
- Přidán `PeerRequestTracker.*` s pevnou kapacitou 8 slotů. Registruje validní COMMAND požadavky, hlídá duplicity request ID, ověřuje korelaci již autentizované RESPONSE podle request ID, původního command code, obou device ID a boot ID.
- Timeout je wrap-safe přes unsigned rozdíl `uint32_t`; interval 0 nebo nad INT32_MAX ms je odmítnut. Mismatched response neodebere pending request; pozdní response jej ukončí jako timeout. `takeExpired()` poskytuje aplikační vrstvě ID požadavku k vyřízení timeoutu.
- `tests/run_native_tests.sh`: 33/33 testovacích programů prošlo standardním během. Nový test samostatně prošel AddressSanitizer/UndefinedBehaviorSanitizer.
- Common soubory synchronizovány do obou Arduino `src` složek. Arduino-ESP32 build, kompletní sanitizovaný běh, W5500 integrace a hardware nebyly ověřeny. Tracker není připojen k síti ani transferovému automatu; cache je volatilní.


## 2026-11-01 — revize 0.47: integrace request/response do peer session
- `PeerCommandSession::prepareOutgoingRequest()` nyní kóduje autentizovaný COMMAND a registruje jej v `PeerRequestTracker` před předáním transportu. Při selhání registrace vynuluje vytvořený wire buffer a vrátí nulovou délku.
- `PeerCommandSession::receiveResponse()` ověří příchozí rámec přes session-bound HMAC codec, vyžaduje RESPONSE a teprve poté koreluje odpověď s pending requestem. Chybný HMAC/typ neuzavře čekající request.
- `PeerRequestTracker` uchovává 16 nedávných request ID; po ACK/REJECTED/FAILED nebo timeoutu ID nelze znovu použít. `cancelPendingPreserveHistory()` ruší pending požadavky při zavření session, ale ponechává recent ID pro reconnect v rámci stejného běhu.
- Nativní `tests/run_native_tests.sh`: 33/33 testovacích programů prošlo. `test_peer_command_session` i `test_peer_request_tracker` prošly samostatnými ASan/UBSan běhy.
- Společné soubory byly synchronizovány do obou firmware `src` adresářů; byte-identita a ZIP integrity se ověří před zabalením.
- Síťové napojení W5500, handshake orchestrace, persistentní anti-replay, secret provisioning/backend, retry politika, `TransferLogic` integrace, Arduino-ESP32 build a fyzické testy nejsou provedeny. Výkonové výstupy zůstávají neaktivní.


## Revize 0.49 — dvouslotový NVS replay žurnál
- Přidán `NvsPeerReplayStore` nad ESP32 `Preferences`, se dvěma nezávislými blob klíči, generačním čítačem, CRC32, výběrem novějšího platného slotu a read-back kontrolou.
- Simulace přerušení zápisu ověřuje zachování předchozího checkpointu; poškozený novější slot umožní fallback na starší platný záznam. Poškození obou slotů je fail-closed.
- Integrační test ověřuje skutečný adaptér proti `PersistentPeerReplayGuard`: po restartu přijímače se obnoví rezervovaná hranice a stará sekvence se odmítne.
- 36/36 nativních testů prošlo; nový test samostatně prošel ASan/UBSan. Nejde o fyzický test výpadku napájení ani Arduino-ESP32 překlad.
- Otevřeno: autentizovaná synchronizace sekvenční podlahy s odesílatelem, jeho monotónní allocator, W5500 handshake orchestrace a skutečný hardware test.


## Revize 0.50 — monotónní odesílací sekvence
- Přidán `PeerSequenceAllocator` s boot-scoped epochou, monotónním posunem nad persistentním replay floor, ochranou proti zpětnému posunu a fail-closed vyčerpáním rozsahu.
- `PeerCommandSession::prepareOutgoingRequest()` nově přidělí sekvenci interně a koreluje přesně zakódovaný požadavek.
- Integrační test ověřuje obnovení receiveru s rezervovaným floor 64 a přijetí prvního nového příkazu se sekvencí 65.
- První běh nového testu odhalil chybnou očekávanou návratovou hodnotu na hranici `MAX_SEQUENCE`; test byl opraven podle kontraktu allocatoru (hranice vyčerpá čítač a vrací false).
- 37/37 nativních testovacích programů prošlo; nové testy prošly ASan/UBSan.
- Neověřeno: autentizované předání floor přes síť, skutečný Arduino-ESP32 build, fyzické NVS a hardware.


## 2026-11-01 — revize 0.51: autentizovaná synchronizace replay floor
- `PeerHandshakeChallenge` nově nese `authenticatedReplayFloor`; hodnota je součástí HMAC transcriptu challenge, FINISH i session key derivation. Změna floor po vytvoření challenge je odmítnuta.
- `PeerHandshakeResponder::beginWithReplayStore()` načte floor přes `readPeerReplayFloorForHandshake()` z `IPeerReplayStore` podle identity a boot ID peeru a vytvoří challenge s touto hodnotou. Při poškozeném/chybovém úložišti selže zavedení handshake.
- Handshake wire protokol zvýšen na verzi 2; CHALLENGE narostl z 80 na 84 B. Starší verze je odmítnuta, aby se struktura nezaměnila.
- Nativní testy: 37/37. `test_peer_replay_store` a `test_peer_handshake_wire` prošly samostatně ASan/UBSan.
- Neprovedeno: W5500 síťová orchestrace, automatické volání `synchronizeOutgoingSequenceFloor()` v produkční session, Arduino-ESP32 build a fyzické testy.

## Revize 0.52 — handshake READY
- Přidán autentizovaný čtvrtý krok READY. Dřívější tříkrokový návrh neposkytoval iniciátorovi potvrzení, že responder FINISH skutečně přijal; po odeslání FINISH by tak strany mohly mít rozdílný názor na stav session.
- READY je HMAC nad samostatnou doménou se session key. Neobsahuje nové neautentizované parametry a jeho platnost je svázána s klíčem konkrétního handshake.
- Nativní sada 38/38 prošla; samostatný ASan/UBSan test nového modulu prošel.
- Otevřená rizika: ztráta READY a timeout musí být řešeny budoucím síťovým stavovým automatem; READY není zatím propojeno s aktivací `PeerCommandSession`.


## 2026-11-01 — revize 0.53: koordinátor handshake iniciátora
- Přidán `PeerHandshakeInitiator.*` se stavovým průběhem IDLE → WAIT_CHALLENGE → WAIT_READY → ESTABLISHED/FAILED.
- Iniciátor ověřuje CHALLENGE před použitím replay floor; test ověřuje, že pozměněná hranice nezmění čítač.
- FINISH je cacheován pro retry beze změny; session key není přístupný do ověření READY.
- Neplatný READY uzavře pokus a vymaže dočasné tajemství/session key.
- Nativní sada 39/39 prošla; nový test prošel samostatně ASan/UBSan.
- Síťový transport, responder orchestrátor, `esp_random()`, provisioning tajemství, Arduino build a fyzické testy zůstávají otevřené.


## 2026-11-01 — revize 0.54: koordinátor handshake responderu
- Přidán `PeerHandshakeResponderCoordinator` s časově omezeným lifecycle responderu. Replay floor se načítá přes `IPeerReplayStore` před vytvořením CHALLENGE.
- Po platném FINISH se vytvoří a cacheuje READY. `readyForRetry()` vrací identickou zprávu; session key lze získat až po `markReadyTransmitted()`. Iniciátor jej přesto nepoužije, dokud READY samostatně neověří.
- Timeout se kontroluje při FINISH, READY pending i poll; chybný FINISH, timeout a cancel mažou dočasný klíč a READY.
- Nativní sada 40/40 prošla. Nový test prošel samostatně ASan/UBSan.
- Zatím nejde o síťovou implementaci: chybí W5500/UDP orchestrace, retry timer transportu, secret provisioning, RNG integrace a napojení na `PeerCommandSession`. Arduino build a hardware nebyly ověřeny.


## 2026-11-01 — revize 0.55: bounded handshake retry policy
- Přidán `PeerHandshakeRetryPolicy` pro řízení časování opakovaných přenosů neměnného handshake datagramu. První odeslání je okamžité a započítá se jako pokus 1; další pokusy jsou omezeny intervalem a maximálním počtem.
- `due()`, `markSent()` a `exhausted()` oddělují rozhodnutí o retransmisi od samotného UDP transportu. Časové odečítání je wrap-safe přes přetečení `millis()`.
- Test pokrývá neplatné parametry, interval, limit pokusů, vyčerpání i přetečení `uint32_t`.
- Nativní sada 41/41 prošla. Nový test prošel samostatně ASan/UBSan.
- Retry politika zatím není napojena na W5500; transport musí sám zachovat přesné bajty zprávy, volat `markSent()` až po úspěšném předání a současně vynucovat celkový timeout handshake.


## 2026-11-01 — revize 0.57: časovaný handshake iniciátoru
- Přidán `TimedPeerHandshakeInitiator` s celkovým timeoutem a omezenými opakováními HELLO/FINISH.
- HELLO/FINISH se opakují beze změny obsahu; retry interval a počet pokusů se aplikují samostatně na obě fáze.
- Neplatný CHALLENGE/READY okamžitě ukončí iniciátor do FAILED; citlivý session key se nevydá a dočasná data se mažou.
- Regresní test pokrývá wrap-safe časování, přesné opakování, dokončení výměny, vyčerpání pokusů, celkový timeout a neplatný READY.
- `tests/run_native_tests.sh`: 42/42 testovacích programů prošlo. Nový test prošel samostatně ASan/UBSan.
- Skutečný Arduino-ESP32 build, síťový W5500 transport a fyzický hardware nebyly ověřeny.


## 2026-11-01 — revize 0.58, responder-side handshake retry
- Přidán transportně nezávislý `TimedPeerHandshakeResponderCoordinator`. CHALLENGE a READY mají oddělené retry politiky a vracejí přesně stejné datové struktury.
- READY lze opakovat po ztrátě; upravený FINISH nedostane cachované READY. Session key se zpřístupní až po potvrzení prvního odeslání READY.
- Nativní sada: 43/43 testovacích programů prošlo. Nový test pokrývá retry exhaustion, duplicity, změněný FINISH a wrap-around `millis()`.
- Modul je synchronizován do obou Arduino `src/` stromů. Arduino-ESP32 build, W5500 transport a fyzické zařízení nebyly ověřeny.


## 2026-11-01 — revize 0.59: responder retry při chybě UDP send
- Revize odhalila hraniční chybu v 0.58: retry timer READY začínal až po úspěšném `endPacket()`. Při selhání prvního send tak READY nemělo časovanou retransmisi.
- Timer se nyní aktivuje při vytvoření READY; první úspěšný send (nebo úspěšná retransmise) teprve zpřístupní session key.
- Duplicitní FINISH vrací stejné READY bez restartu retry timeru.
- Každý retry pokus včetně lokálního selhání transportu se započítá; vyčerpání v READY_PENDING vede do FAILED a odstranění session key. CHALLENGE má obdobný hook pro započítání lokálních pokusů.
- 43/43 nativních testovacích programů prošlo; celá sada prošla s AddressSanitizer a UndefinedBehaviorSanitizer.
- Stále neprovedeno: skutečný UDP transport, Arduino-ESP32 build, provisioning/secret backend a hardware.


## 2026-11-01 — revize 0.60: vzdálený přístup a bezpečnostní audit
- Přidán `REMOTE_ACCESS_PLAN.md`: vzdálený přístup má být přes WireGuard VPN na domácím routeru; ESP32 se nesmí přímo vystavit veřejnému internetu.
- Oficiální podpora TP-Link potvrzuje WireGuard VPN Server pro Archer AX12(EU) V1 od firmware 1.1.28 Build 20250523. Konkrétní HW verze, firmware, WAN topologie a CGNAT uživatele nejsou ověřeny.
- Zdrojový audit odhalil, že firmware vypisuje některá generovaná hesla AP a webového administrátora do Serial Monitoru. Před ostrým vzdáleným přístupem je nutné vyřešit bezpečné předání/reset hesel a odstranit tajemství z diagnostických výpisů.
- Revize je dokumentační; firmware se nemění. Nativní testy znovu spuštěny: 43/43 prošlo.
- Router nebyl konfigurován, firmware routeru nebyl aktualizován a VPN nebyla testována.


## 2026-11-02 — revize 0.61: audit shody s Master 1.3
- Provedena statická kontrola požadavků Master 1.3 proti oběma sketchům, společným modulům a dokumentaci. Přidána matice `MASTER_COMPLIANCE_AUDIT.md`.
- Potvrzeno: horní objem a teplota jsou ve stavovém rámci neplatné; fyzické GPIO výstupy nejsou řízeny; transferové UI spouští pouze SIMULACI.
- Potvrzeno: W5500/UDP stavové rámce nejsou autentizované; handshake a command session moduly nejsou integrovány do síťové smyčky.
- Nalezeny výpisy hesel AP a webového administrátora do Serial Monitoru. Odstranění je odloženo do návrhu použitelného a bezpečného provisioningu, aby uživatel neztratil přístup.
- `tests/run_native_tests.sh`: 43/43 prošlo; stejná sada s AddressSanitizer/UndefinedBehaviorSanitizer: 43/43 prošlo. Runner kompiluje v C++11 s `-Wall -Wextra -Werror -pedantic`.
- Společné moduly jsou bajtově shodné ve `firmware/common/` a obou sketch `src/`.
- Firmware se nemění. Arduino-ESP32 build, fyzické NVS, senzory, W5500, Wi-Fi/AP a hardware zůstávají neověřené.


## 2026-11-02 — revize 0.62: návrh bezpečného párování
- Provedena kontrola `PeerSecretVault`, handshake koordinátorů, `PeerCommandSession`, `NvsPeerReplayStore` a současné W5500 UDP vrstvy.
- Potvrzeno: 32B fail-closed vault a časované handshake moduly existují, ale ESP32 backend chráněného klíče ani síťová handshake/command orchestrace nejsou připojeny.
- Přidán `SECURITY_PROVISIONING_DESIGN.md`; žádný firmware ani hardware se nemění.
- `./tests/run_native_tests.sh`: 43/43 testovacích programů prošlo.
- Synchronizace common modulů: `firmware/common/` je bajtově shodný s oběma `src/` složkami.
- Neprovedeno: skutečný Arduino build, fyzické testy, NVS Encryption, Flash Encryption, Secure Boot, eFuse, provisioning a síťové řídicí příkazy.


## 2026-11-02 — revize 0.63: ověření zápisu peer tajemství
- `PeerSecretVault::provision()` nyní po úspěšném `write()` načte 32B tajemství zpět a provede konstantně časové porovnání; dočasný buffer je vždy vynulován.
- Pokud zpětné čtení selže nebo se hodnota liší, vault se pokusí neověřený záznam zneplatnit a vrátí chybu. Backendový kontrakt výslovně požaduje trvalý zápis a trvalé zneplatnění při úspěšném `erase()`.
- Test rozšířen o selhání zápisu, zápis poškozené hodnoty a neúspěšné zpětné čtení; ověřuje také pokus o odstranění neověřeného záznamu.
- `tests/run_native_tests.sh`: 43/43 testovacích programů prošlo. Změněný `test_peer_secret_vault` prošel samostatně s AddressSanitizer/UndefinedBehaviorSanitizer. Kompletní sanitizovaná sada nebyla dokončena v časovém limitu; neoznačuje se tedy jako 43/43 sanitizovaně ověřená.
- Změněné soubory `PeerSecretVault.h/.cpp` jsou bajtově shodné v `firmware/common/` a obou Arduino `src/` stromech.
- Arduino-ESP32 build, NVS Encryption, skutečný backend, fyzický provisioning a hardware nebyly ověřeny. Síťová command path a výkonové výstupy zůstávají zakázané.


## 2026-11-02 — revize 0.64: platformní audit NVS Encryption
- Ověřeny oficiální zdrojové soubory Arduino-ESP32 1.0.6 a 2.0.17: `initArduino()` volá `nvs_flash_init()` před `setup()`; `Preferences` používá pro default NVS `nvs_open()` a pro vlastní partition `nvs_flash_init_partition()` + `nvs_open_from_partition()`.
- ESP-IDF v3.3 již poskytuje `nvs_flash_secure_init()` a související API; NVS Encryption vyžaduje chráněnou NVS key partition a Flash Encryption.
- Oficiální Arduino-ESP32 instalační dokumentace uvádí, že core 2.x.x není podporován na Windows 7 a starším; 1.0.6 je poslední uváděná funkční řada pro Windows 7.
- Přesná verze uživatelova core není známa. Proto nebyl vytvořen platformní backend ani upravena partition table/eFuse. Přidán `NVS_ENCRYPTION_PLATFORM_AUDIT.md` a aktualizován implementační plán.
- Webové zdroje: Arduino-ESP32 installation guide; raw source tags 1.0.6/2.0.17; ESP-IDF v3.3 NVS docs a ESP-IDF v4.4 NVS docs.
- Žádný firmware se v revizi 0.64 nemění. Nativní testy 43/43 z revize 0.63 zůstávají platné; Arduino build, NVS Encryption a hardware nebyly testovány.


## 2026-11-02 — revize 0.65: persistentní ochrana ID transferů
- Audit `TransferLogic` zjistil, že duplicate-ID cache byla pouze osmipoložková RAM a po restartu se neobnovovala z již persistentní historie transferů.
- Cache rozšířena na 12 položek, shodně s kapacitou `TransferHistory`; přidána `restoreRecentRequestIds()` s bezpečným sloučením historických a nově přijatých RAM ID.
- ESP32 #1 obnovuje replay ochranu při načtení historie v `setup()` i při pozdějším zotavení NVS. Chronologické pořadí historie je zachováno.
- Regresní testy pokrývají duplicitní ID po restartu, zachování nejnovějšího okna 12 ID, přijetí ID mimo toto okno a zachování ID přijatého v RAM před pozdním zotavením NVS.
- `tests/run_native_tests.sh`: 43/43; kompletní běh s AddressSanitizer a UndefinedBehaviorSanitizer: 43/43.
- `TransferLogic.h/.cpp` bajtově shodné v `firmware/common/` a obou ESP32 `src/`; ZIP kontrola bude provedena při zabalení.
- Změna nemění fyzické výstupy. Transfery zůstávají SIMULACÍ; Arduino build, skutečné NVS a hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.66 — 2026-11-02
- Přidán `TransferRestartGuard` a dvouslotový `NvsTransferJournalStore`; obojí synchronizováno do obou ESP32 `src/` stromů.
- ESP32 #1 nyní před přijetím simulačního požadavku zapisuje PREPARED, před RUNNING ACTIVE a při koncovém výsledku RESULT_PENDING. Po restartu PREPARED/ACTIVE přechází na INTERRUPTED; není možné automatické obnovení.
- Obnova vyžaduje potvrzení obsluhy a úspěšné uložení záznamu do transferové historie. Poškozené nebo nedostupné úložiště blokuje nové požadavky.
- `TransferLogic::forceAbort()` vrací koncový ABORTED stav s `VALVES_CLOSE_ALL`, pokud nelze trvale uložit ACTIVE.
- Nativní sada: 45/45. Kompletní AddressSanitizer + UndefinedBehaviorSanitizer: 45/45.
- Změněn runner: oddělil stubované Preferences buildy pro replay journal a transfer journal, aby sanitizer běh nemíchal nekompatibilní testovací backendy.
- Arduino-ESP32 build nebyl proveden; fyzické NVS a výpadek napájení nebyly testovány. Výstupy zůstávají neaktivní.


## Kontrolní stav po revizi 0.67 — 2026-11-03
- Rozšířen `test_nvs_transfer_journal_store.cpp`: přesný namespace `hwc_xfer_jrnl`, zápis po poškození novějšího slotu při zachování platného fallbacku a přetečení generačního čítače `0xFFFFFFFF → 0`.
- `tests/run_native_tests.sh`: 45/45 prošlo.
- Kompletní sada spuštěna také s `-fsanitize=address,undefined -fno-omit-frame-pointer`: 45/45 prošlo.
- `firmware/common/` zůstává bajtově shodný s oběma ESP32 `src/`; delimiter check `ESP32_1.ino` prošel.
- Firmware se nemění. Arduino build, fyzické NVS a skutečný power-cut test nebyly provedeny.


## Kontrolní stav po revizi 0.68 — 2026-11-03
- Audit transferového automatu zjistil nesprávnou prioritu diagnostiky: ztráta peer dat byla kvůli `sourceLevelValid=false` klasifikována jako chyba čidla.
- `TransferLogic::precheckFault()` a `runningFault()` nyní nejprve rozlišují ztrátu peer komunikace u zdroje UPPER_TANK.
- Přidány testy: výpadek peer dat v PRECHECK odmítne start; výpadek v RUNNING přeruší transfer, vrátí `VALVES_CLOSE_ALL` a `TRANSFER_REASON_COMMUNICATION`.
- Standardní nativní sada: 45/45. Kompletní ASan/UBSan: 45/45.
- `TransferLogic.cpp` je bajtově shodný ve `firmware/common/` a obou ESP32 `src/` složkách.
- Arduino-ESP32 build, fyzická síť, hardware a výkonové výstupy nebyly testovány; transfer zůstává SIMULACÍ.

## Kontrola návaznosti MASTER 1.3 a revize 0.66–0.68 — 2026-11-03
- Porovnány archivy 0.66, 0.67 a 0.68: verze navazují; 0.67 rozšiřuje test NVS journalu bez změny firmware, 0.68 mění pouze transferovou logiku a její testy/dokumentaci.
- Zjištěna chyba v ESP32 #2: `peerBootId` a `peerSequence` se obnovovaly při každém volání `startPeerEthernet()`, tedy i při DHCP/W5500 reconnectu.
- Oprava 0.69: boot identita se inicializuje jednou v `setup()`; síťové reconnecty zachovají epochu a monotónní sekvenci.
- Přidán aktuální audit shody `docs/MASTER_ALIGNMENT_AUDIT_0.69.md`; stará matice 0.60 je výslovně označena jako historická.
- Nativní sada: 45/45. Kompletní ASan/UBSan: 45/45.
- Arduino-ESP32 build, fyzická síť, NVS, power-cut a hardware nebyly ověřeny. Fyzické výstupy zůstávají zakázané.

- Navazující peer audit 0.69: `PeerStatusMonitor` nyní odmítá opožděné UDP rámce z posledních čtyř opuštěných boot epoch; přidán regresní případ do `test_peer_protocol`.
- Přidán statický invariant `tests/check_peer_boot_identity.sh`; ověřuje jednorázové generování boot ID a stabilní sekvenci přes W5500 reconnect.
- Po doplnění: 45/45 nativních testů, 45/45 ASan/UBSan a statický invariant PASS.


## Kontrolní stav po revizi 0.70 — 2026-11-04
- `PeerStatusMonitor` přešel z převodu `uint32_t` rozdílu na `int32_t` na explicitní serial arithmetic; přetečení `0xFFFFFFFF → 0` je platné, rozdíl `0x80000000` se odmítá jako nejednoznačný.
- `test_peer_protocol` pokrývá rollover, opožděnou starou sekvenci, half-range a čtyřpoložkový retired boot ID ring včetně jeho eviction limitu.
- `firmware/common/PeerProtocol.h/.cpp` je bajtově shodný s oběma ESP32 `src/`.
- Standardní nativní sada: 45/45; AddressSanitizer + UndefinedBehaviorSanitizer: 45/45; `check_peer_boot_identity.sh`: PASS.
- Arduino-ESP32 build, fyzický W5500/DHCP, NVS a hardware nebyly testovány. Fyzické výstupy zůstávají zakázané.


## Kontrolní stav po revizi 0.71 — 2026-11-04
- ESP32 #2 nyní kontroluje DHCP adresu po `Ethernet.maintain()`; při ztrátě zavře UDP socket, označí transport jako nedostupný a časovaně opakuje inicializaci.
- ESP32 #1 při zjištěné ztrátě DHCP adresy rovněž zavře UDP socket před retry.
- Přidán `tests/check_peer_ethernet_recovery.sh` do standardního test runneru.
- Standardní nativní sada: 45/45; AddressSanitizer + UndefinedBehaviorSanitizer: 45/45; boot identity PASS; DHCP recovery invariant PASS.
- Fyzický link-down, W5500, DHCP server/lease a skutečný reconnect nebyly ověřeny. Samotná kontrola nulové IP nemusí detekovat fyzické odpojení kabelu při zachované DHCP konfiguraci.

## Kontrolní stav po revizi 0.72 — 2026-11-04
- Obě ESP32 kontrolují fyzický stav W5500 přes `Ethernet.linkStatus()`. Při `LinkOFF` zavřou UDP socket a čekají na návrat linky; po `LinkON` bezprostředně zahájí DHCP inicializaci.
- Zachována fallback cesta pro `Unknown` a samostatná reakce na ztrátu DHCP adresy při zachované fyzické lince.
- Statická regresní kontrola obou endpointů: PASS.
- Opraven `.run_native_tests_asan.sh`: speciální linkovací větve nyní používají sanitizer flags a správné pořadí testovacích Preferences stubů; sanitizer běh obsahuje i kontrolu Ethernet recovery.
- Standardní nativní sada: 45/45; AddressSanitizer + UndefinedBehaviorSanitizer: 45/45; boot identity PASS; Ethernet recovery PASS.
- Arduino Ethernet Library 2.0.0+ je požadována pro `linkStatus()`; fyzický W5500 link test ani Arduino-ESP32 build nebyl proveden.
- Fyzické výstupy zůstávají zakázané.


## Kontrolní stav po revizi 0.73 — P0 peer security
- Nalezen rollback anti-replay floor: `NvsPeerReplayStore::load()` mohl při korupci novějšího slotu použít starší platný slot. Opraveno fail-closed pravidlem: korupce kteréhokoli slotu blokuje načtení i zápis.
- `PeerSecretVault::provision()` nyní při chybě backend write zkusí záznam zneplatnit; test simuluje částečný zápis.
- Standardní nativní sada: 45/45; ASan/UBSan: 45/45; boot identity PASS; Ethernet recovery PASS.
- Změněné společné moduly synchronizovány do obou ESP32 `src/` stromů.
- Skutečné NVS/power-cut, Arduino build, produkční šifrovaný backend, fyzický provisioning a W5500 nebyly ověřeny. Výkonové výstupy zůstávají zakázané.

## Dodatečný audit po revizi 0.73 — P0-3
- Statická kontrola odhalila, že dva chybějící NVS sloty jsou klasifikovány jako EMPTY. To je správné pro první použití, ale nerozlišuje úplnou ztrátu již inicializovaného žurnálu.
- Nález je zaznamenán jako otevřený P0 blokátor; v této revizi nebyla provedena neověřená změna úložiště.
- NVS marker může detekovat ztrátu slotů při zachovaném markeru, nikoli smazání celé partition nebo rollback celé flash.

## 2026-11-04 — revize 0.76: audit vystavení přihlašovacích údajů
- Statická kontrola odhalila, že ESP32 #1 zobrazuje heslo konfiguračního AP v běžné HTML stránce a vypisuje AP/admin hesla do Serial Monitoru; ESP32 #2 vypisuje AP/admin hesla do Serial Monitoru.
- Firmware se záměrně nemění: odstranění výpisů bez bezpečného předání a obnovy hesel by mohlo uživatele uzamknout mimo zařízení. Přidán auditní dokument s požadovaným pořadím opravy.
- P0-4 až P0-6 zůstávají otevřené. Peer command path zůstává zakázaná a fyzické výstupy neaktivní.
- Arduino build, skutečné NVS, provisioning ani hardware nebyly v této revizi ověřeny.

## 2026-11-04 — revize 0.77: návrh bezpečného provisioningu přístupových údajů
- Zpracován návrh prvotního nastavení, změny a obnovy AP/admin hesel pro ESP32 #1 a #2.
- Návrh vyžaduje fyzicky potvrzený a časově omezený servisní režim; USB/Serial samotné není považováno za fyzické oprávnění.
- Výslovně zakázáno zobrazování existujících hesel v dashboardu/HTTP a jejich výpis do Serial logu; změna/obnova nahrazuje heslo novým.
- GPIO servisního vstupu nebylo přiděleno, protože konkrétní desky a dostupné piny nejsou ověřeny.
- Firmware zůstává beze změny; P0-4 až P0-6 otevřené. Nativní testy nejsou změnou kódu dotčeny, budou znovu spuštěny při balení.
- Po dokumentační změně znovu spuštěno: standardní nativní testy 45/45 PASS; ASan/UBSan 45/45 PASS; boot identity PASS; Ethernet recovery PASS.
- Změna nezasahuje firmware; Arduino překlad a fyzický hardware nebyly ověřeny.

## 2026-11-04 — revize 0.79: integrační brána credential storage
- Ověřena struktura zdrojového stromu 0.78; AP/Wi-Fi/webová aplikační vrstva chybí, takže nelze provést skutečné napojení helperu ani ověřit zákaz startu AP při chybě NVS.
- Záměrně nebyl přidán izolovaný helper bez produkčního call-site. Přidána specifikace integračního kontraktu a povinných testů.
- Změna je dokumentační; před vydáním znovu provést celý nativní test suite a kontrolu archivu.

## Revize 0.80 — 2026-11-04
- Statická kontrola skutečných síťových call-sites v `ESP32_1.ino` a `ESP32_2.ino`.
- Opraven chybný auditní závěr 0.79 o absenci AP/Wi-Fi/web implementace.
- Nálezy a přesné odkazy na řádky jsou v `SECURITY_CREDENTIAL_CALLSITE_AUDIT_0.80.md`.
- `./tests/run_native_tests.sh`: PASS, 45/45 native test programs.
- `sh tests/.run_native_tests_asan.sh`: PASS, 45/45 native test programs.
- `tests/check_peer_boot_identity.sh`: PASS.
- `tests/check_peer_ethernet_recovery.sh`: PASS.
- Firmware beze změny; žádný Arduino build ani hardware test.

## Revize 0.81 — 2026-11-04
- Dokumentační implementační brána CredentialStore: stavový kontrakt, migrační pořadí, AP start gate a fault-injection testovací matice.
- Firmware a testovací zdrojové soubory beze změny.
- `./tests/run_native_tests.sh`: PASS, 45/45 native test programs; peer boot identity PASS; W5500 recovery PASS.
- `sh tests/.run_native_tests_asan.sh`: PASS, 45/45 native test programs.
- Arduino build, NVS encryption, fyzický provisioning a hardware testy neprovedeny.


## 2026-11-04 — revize 0.85: CredentialStore commit read-back
- Opravena klasifikace commit výsledku při `put(cred_active)` returning false after write; rozhodující je ověřený read-back markeru.
- Přidán test backendu, který marker uloží, ale vrátí chybu; `save()` musí rozpoznat skutečný commit.
- `./tests/run_native_tests.sh`: PASS, 47/47 native test programs.
- `sh tests/.run_native_tests_asan.sh`: PASS, 47/47 native test programs.
- `tests/check_peer_boot_identity.sh`: PASS.
- `tests/check_peer_ethernet_recovery.sh`: PASS.
- Arduino build a fyzická NVS/hardware nebyly ověřeny; AP integrace a provisioning zůstávají otevřené.

## Revize 0.88 — přesná expirace servisní autorizace
- Opravena hranice 60s autorizačního okna: autorizace se nyní odmítne již při `elapsed >= WINDOW_MS`, nikoli až po překročení limitu.
- Doplněn regresní test přesného okamžiku expirace pro `consume()` a `isArmed()`.
- Modul synchronizován pro ESP32 #1/#2.
- `./tests/run_native_tests.sh`: PASS, 48/48 native test programs; peer boot identity PASS; W5500 recovery PASS.
- `sh tests/.run_native_tests_asan.sh`: PASS, 48/48 native test programs; peer boot identity PASS; W5500 recovery PASS.
- Arduino build, fyzický vstup, provisioning, NVS/Flash Encryption a hardware testy nebyly provedeny.


## Revize 0.90 — 2026-11-04
- Implementován transportně nezávislý provisioning controller s jednorázovou fyzickou autorizací a 60s session timeoutem.
- Ošetřen recovery postup pro osiřelý staging a přerušené mazání: selector se odstraní a ověří před čištěním slotů; po chybě zůstává stav fail-closed.
- Credential buffer se vynuluje na všech return cestách; store clear používá volatile nulování.
- Nový fault-injection test zahrnuje počáteční provisioning, CORRUPT recovery, přerušení recovery a opakování po nové fyzické autorizaci, IO_ERROR, chybné údaje, rotaci a expiry přes wrap.
- `./tests/run_native_tests.sh`: PASS, 49/49 native test programs.
- `sh tests/.run_native_tests_asan.sh`: PASS, 49/49 native test programs.
- Peer boot identity PASS; W5500 recovery PASS.
- Moduly bajtově shodné ve common a obou ESP32 src stromů.
- Arduino build, fyzický servisní input/transport, skutečná NVS, power-cut a NVS/Flash Encryption neověřeny. AP call-sites a log/HTML úniky zůstávají neopravené.


## 2026-11-04 — revize 0.92: hostitelský klient provisioningu
- Přidán `tools/credential_provisioning_client.py` pro sestavení SET_CREDENTIALS v1 rámce a validaci pevné response.
- Hesla se zadávají pomocí getpass; výchozí režim je náhled, klient nevypisuje obsah rámce ani credentials.
- Skutečné odesílání je podmíněno `--send` a explicitním `--firmware-integrated`; u revize 0.92 se tento přepínač nesmí použít, protože Serial/GPIO adapter není zapojen do firmware.
- `python3 -m unittest discover -s tests/python -v`: PASS, 15/15.
- `python3 -m py_compile tools/credential_provisioning_client.py tests/python/test_credential_provisioning_client.py`: PASS.
- `./tests/run_native_tests.sh`: PASS, 50/50 C++ test programs; peer boot identity PASS; W5500 recovery PASS.
- `sh tests/.run_native_tests_asan.sh`: PASS, 50/50 C++ test programs.
- PySerial/COM zařízení, DTR/RTS reset chování a skutečný provisioning nebyly testovány; Arduino build, NVS/Flash Encryption a hardware zůstávají neověřené.


## 2026-11-04 — revize 0.93: board adapter + READY handshake
- Přidán přenositelný `CredentialProvisioningBoardAdapter.h`: board-specific fyzický vstup a UART-like stream se propojí bez výběru GPIO/UART pinů.
- Před autorizací jsou RX bajty aktivně zahazovány; pro otevření relace se vyžaduje prázdný RX a další klidný poll. READY se odešle až po spotřebování jednorázové fyzické autorizace.
- Host klient čeká na ověřený READY status 0x01, a teprve poté odešle credentials. Neplatný/timeout READY znamená žádné odeslání.
- Testy: stale/early request, 3s fyzické podržení, READY-before-request, fragmentovaný rámec, one-shot, přesná 60s expirace, neúspěšný TX, 600B backlog a dávkové vyprazdňování.
- `./tests/run_native_tests.sh`: PASS, 51/51; `sh tests/.run_native_tests_asan.sh`: PASS, 51/51.
- `python3 -m unittest discover -s tests/python -v`: PASS, 17/17; `py_compile`: PASS.
- `tests/check_credential_module_sync.sh`: PASS; šest modulů identických ve common a obou src stromech.
- Adapter zůstává nezapojený do `.ino`; servisní GPIO/UART a DTR/RTS nejsou potvrzeny. Stávající Serial/HTML úniky hesel zůstávají; aktuální sketch firmware nesmí použít `--firmware-integrated`.
- Arduino build, fyzické NVS/power-cut, NVS/Flash Encryption ani hardware testy nebyly provedeny.


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


## 2026-11-04 — revize 0.95: jednotné dvouslotové CredentialStore
- Přidán `CredentialLegacyMigrator.h`, který migruje staré namespace do `hwcCred` pouze při prokazatelném stavu EMPTY. VALID čte jen nový store; CORRUPT/IO_ERROR jsou fail-closed a legacy fallback se zakazuje.
- `CredentialStore::validate()` zpřísněna na tisknutelné ASCII (bez řídicích a ne-ASCII znaků); přidány regresní případy do `test_credential_store`.
- ESP32 #1: připojení nového store pro `apName`, `apPassword`, `adminPassword`; první migrace ze `hwc_ap/password` a `hwc_auth/admin`.
- ESP32 #2: připojení nového store; první migrace z `hwc_u2ui/ap_name`, `ap_pass`, `admin`.
- Legacy záznamy zůstávají nedotčené; po migraci nejsou používány jako záložní zdroj. Jednorázové zobrazení nových hesel nastává až po potvrzeném commitu celé sady.
- Přidán `test_credential_legacy_migrator.cpp` a `test_nvs_credential_legacy_migrator.cpp`: prázdný cíl, úspěšná migrace, autorita již existujícího store, zákaz migrace z CORRUPT/IO_ERROR, odmítnutí nevalidního zdroje, reboot read-back a poškození vybraného slotu přes emulovaný `Preferences` backend.
- `./tests/run_native_tests.sh`: PASS, 55/55 native test programs. `sh tests/.run_native_tests_asan.sh`: PASS, 55/55. Peer boot identity PASS; W5500 recovery PASS; credential modul sync PASS; AP startup/migration static integration PASS.
- `python3 -m unittest discover -s tests/python -v`: PASS, 17/17; `py_compile`: PASS.
- Zatím neprovedeno: Arduino build, skutečná NVS, power-cut na hardware, NVS/Flash Encryption, fyzický UART/GPIO provisioning. Peer command path default-deny; fyzické výstupy neaktivní.


## 2026-11-04 — revize 0.96: RF-gated credential entropy
- Audit zjistil, že v revizi 0.95 se při prvním generování AP/admin hesel volalo `esp_random()` ještě před startem Wi-Fi driveru. ESP-IDF 3.3.x uvádí true-random výstup při aktivním Wi-Fi/Bluetooth; bez RF je výstup pseudonáhodný.
- Přidán `CredentialRandom.h`: 4×32bit random words → 32 uppercase hex characters (128 bitů), pevný buffer, nulování při chybném vstupu.
- Obě sketch jednotky aktivují Wi-Fi STA (`WiFi.persistent(false)` + `WiFi.mode(WIFI_STA)`) před každým generováním chybějícího hesla; chyba inicializace znamená fail-closed.
- `./tests/run_native_tests.sh`: PASS, 56/56; ASan/UBSan PASS, 56/56; Python klient 17/17. Peer boot identity, W5500 recovery, credential module sync, AP startup/migration integration a RNG integration PASS.
- Host testy neověřují fyzickou kvalitu RNG ani Arduino build. Skutečná NVS, power-cut, NVS/Flash Encryption, UART/GPIO provisioning a jednorázový Serial handoff zůstávají otevřené.


## 2026-11-05 — revize 0.97: HTTP auth rate limiting
- Nález: obě webová rozhraní používala HTTP Basic Auth bez aplikačního omezení opakovaných chybných pokusů.
- Přidán `WebAuthRateLimiter.h`: 5 neúspěšných pokusů/IP v 60 s, blokace 60 s, úspěch vyčistí stav IP; pevná tabulka 8 klientů, wrap-safe časování a fail-closed saturace bez vytlačení aktivních blokací.
- Obě autentizační brány vrací během blokace HTTP 429 a nevolají `authenticate()`.
- `test_web_auth_rate_limiter.cpp` pokrývá limit, přesnou expiraci, izolaci IP, reset po úspěchu, reset okna, wrap `uint32_t` a saturaci tabulky.
- Standardní C++ testy 57/57 PASS; ASan/UBSan 57/57 PASS; Python klient 17/17 PASS; py_compile PASS; peer boot identity, W5500 recovery, credential module sync, AP startup/migration, entropy a HTTP auth integration PASS.
- Omezení: HTTP Basic Auth je stále nešifrované; TLS/VPN, fyzické NVS, Arduino build a hardware zůstávají otevřené.


## 2026-11-05 — revize 0.98, model výpadku napájení credentials
- Přidán `test_credential_powercut.cpp`: simulované přerušení při všech čtyřech mutačních bodech aktualizace credentials, před i po zápisu (8 scénářů).
- Po simulovaném restartu je povolena pouze kompletní předchozí nebo nová sada; částečná sada nesmí být přijata jako VALID.
- `tests/run_native_tests.sh`: 58/58 testovacích programů PASS; integrační kontroly synchronizace, AP startupu, entropy a HTTP auth PASS.
- Simulace není fyzický power-cut a není důkazem chování skutečné ESP32 NVS. Arduino-ESP32 1.0.6 build a hardware zůstávají neověřené.


## 2026-11-05 — revize 0.99, read-back NVS kalibračních zápisů
- `NvsSensorConfigStore::save()`, `NvsWellGeometryStore::save()` a `NvsWellCalibrationStore::save()` nyní po zápisu načtou data zpět přes validační cestu a ověří checksum; kalibrace ověřuje také počet bodů.
- Kopie zdrojů v common a obou ESP32 src jsou synchronizované.
- `./tests/run_native_tests.sh`: 58/58 PASS; `sh tests/.run_native_tests_asan.sh`: 58/58 PASS; Python 17/17 PASS; `py_compile` PASS; credential module sync PASS.
- Read-back není náhradou za dvouslotovou transakci ani fyzický power-cut. Skutečná NVS, Arduino target build, NVS/Flash Encryption a hardware zůstávají neověřené.


## REV31 – nouzové povolení čerpadel při libovolné softwarové poruše (13. 11. 2026)

- Uživatel upřesnil, že nouzový režim nemá být omezen pouze na poruchu čidla. Pokud je čerpadlo samo provozuschopné, má být možné překonat libovolnou softwarovou blokaci hladinové automatiky po fyzické kontrole hladiny.
- Všechny tři ochranné třídy nyní dovolují nouzový režim po explicitním potvrzení bez ohledu na stav čidla, kritický latch, platnost konfiguračních mezí nebo dostupnost NVS. Diagnostické příznaky závady zůstávají zobrazené; nouzový režim je nemaže.
- Nouzový stav zůstává pouze v RAM a po restartu se zruší. Po jeho vypnutí se znovu uplatní běžná ochrana; při přetrvávající poruše zůstane čerpadlo blokováno.
- Webová ovládání všech tří jednotek jsou dostupná i při platném měření; aktivace vyžaduje fyzické potvrzení hladiny.
- Výstupní synchronizace všech tří relé v nouzovém režimu nepoužívá freshness gate měření; v běžném režimu zůstává limit 5 s zachován. Fyzická připravenost a commissioning gate výstupního relé zůstávají povinné.
- Nouzový režim nepřemosťuje jističe, tepelnou ochranu, ochranu motoru ani případnou skutečnou mechanickou/elektrickou závadu. ESP32 nemá přímou zpětnou vazbu, která by sama diagnostikovala poruchu motoru.
- Rozšířeny nativní testy všech tří ochran o kritický latch, chybnou konfiguraci/NVS, potvrzení obsluhy, odmítnutí bez potvrzení a reset nouzového režimu po restartu.


## 2026-11-13 — REV41: audit servisní obnovy elektrického nastavení snímačů
- Zdrojový audit odhalil neúplnou servisní obnovu `SensorConfig`: ESP32 #1 umí uložit chybějící záznam, ale poškozený záznam odmítá přepsat; ESP32 #2 nemá lokální webový formulář/POST handler pro konfiguraci horního snímače.
- Formulář ESP32 #1 zatím nevyžaduje samostatné potvrzení fyzické referenční kontroly výšky. Nedostupné NVS nelze opravit webovým zápisem.
- Firmware nebyl změněn; doplněn auditní dokument s požadavky na bezpečný servisní recovery workflow.
- REV40 regresní testy zopakovány: 65/65 nativních testovacích programů PASS a všechny integrační kontroly PASS; Python 17/17 PASS; `compileall` PASS; `bash -n tests/*.sh` PASS.
- Arduino target build, skutečná NVS, fyzická referenční kalibrace a hardware nejsou ověřeny.


## REV53 — návazný audit peer autentizace (14. 11. 2026)

- Ověřen uživatelem dodaný REV52 ZIP a přímo prohlédnuty hlavní UDP call-sites obou `.ino` jednotek.
- Potvrzeno: 52B CRC-only status transport stále běží; handshake a `PeerAuthenticatedStatusReceiver` nejsou volány z produkční UDP smyčky.
- Potvrzeno: `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED=false` blokuje běžný automatický i servisní horní transfer; nouzová ruční cesta zůstává samostatnou obslužnou výjimkou.
- Přidán `PEER_AUTH_NEXT_STEP_AUDIT_REV53.md` s konkrétním pořadím další práce a limity platformního NVS Encryption.
- `tests/run_native_tests.sh`: 70/70 PASS.
- `sh tests/.run_native_tests_asan.sh`: 68/68 PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `check_peer_auth_transport_not_integrated.sh`, `check_peer_status_auth_control_gate.sh`, `check_common_module_sync.sh`, `check_mixed_wifi_ethernet_peer_transport.sh`: PASS.
- Arduino target build, NVS/Flash Encryption na čipu, fyzický provisioning, skutečný UDP handshake a hardware nebyly testovány.

## REV54 — 14. 11. 2026 — NVS secure-init startup audit

- Zkontrolován Arduino-ESP32 1.0.6 `initArduino()` a `Preferences::begin()` proti upstream zdrojům.
- Zkontrolována ESP-IDF v3.3 implementace `nvs_flash_secure_init_partition()`, `nvs_flash_deinit_partition()` a default wrappers.
- Zjištěno, že secure-init vrací `ESP_OK` bez změny kontextu, pokud NVS partition už byla inicializována; kandidátní postup proto vyžaduje úspěšný deinit před secure-init.
- Potvrzeno, že secure-init API je podmíněno `CONFIG_NVS_ENCRYPTION`.
- Zkontrolováno pořadí prvního `Preferences.begin()` v obou `.ino`; před ním jsou pump safety výstupy nastaveny LOW.
- Přidán dokument `docs/NVS_SECURE_INIT_STARTUP_AUDIT_REV54.md` a release notes REV54.
- Bez změny firmware, partition table, eFuse, síťové komunikace a bezpečnostních bran.
- Arduino build, flash/NVS encryption, hardware a OTA testy nebyly provedeny.


## REV55 — 14. 11. 2026 — ověření stock Arduino core NVS Encryption

- V oficiálním Arduino-ESP32 1.0.6 `sdkconfig.h` (402 řádků) nebylo nalezeno `CONFIG_NVS_ENCRYPTION` ani `CONFIG_SECURE_FLASH_ENC_ENABLED`.
- Výchozí `default.csv` a `min_spiffs.csv` obsahují pouze standardní `nvs` partition, nikoli `nvs_keys` nebo peer-secret partition.
- Stock core 1.0.6 proto nelze považovat za připravený pro `nvs_flash_secure_init()`; tento API je podmíněn `CONFIG_NVS_ENCRYPTION`.
- Nedoporučeno volat `nvs_flash_deinit()` na výchozí NVS v běžící aplikaci ani měnit partition layout provozních jednotek.
- Přidán audit `docs/NVS_ENCRYPTION_STOCK_CORE_AUDIT_REV55.md` a release notes REV55.
- Firmware, NVS data, partition table, eFuse, UDP transport a bezpečnostní brány beze změny.
- Kontrola se vztahuje ke stock tagu 1.0.6; skutečná instalovaná verze core, Arduino target build, NVS/Flash Encryption a hardware zůstávají neověřené.


## REV56 — 14. 11. 2026 — posouzení externího secure elementu

- Prohlédnut `PeerHandshake.cpp`, `PeerSecretVault` a `PeerSessionKeys.cpp`. Dlouhodobý secret je dnes předáván jako pointer do softwarového HMAC; prosté úložiště v secure elementu proto nestačí.
- Změřeny vstupy HMAC: CHALLENGE 66 B, FINISH 63 B, SESSION 61 B. Kandidátní ATECC608C SHA/HMAC podporuje vstup po blocích do 64 B.
- Popsán nutný refaktor na `IPeerMacProvider`, aby dlouhodobý klíč neopouštěl secure element; krátkodobá session key může být odvozena a použita v RAM.
- ATECC608C zůstává pouze kandidátem; provisioning, varianta, CryptoAuthLib, I²C piny a hardware nebyly ověřeny.
- Přidán audit `docs/PEER_SECURE_ELEMENT_COMPATIBILITY_AUDIT_REV56.md` a release notes REV56.
- Firmware a bezpečnostní brána beze změny; horní automatický i servisní transfer zůstává blokován.


## REV69 — 15. 11. 2026 — DS18B20 ESP32 #2

- Ověřen přesný uživatelem nahraný archiv REV68; ZIP měl 660 položek a kontrola integrity prošla. Vnitřní kořenová složka byla chybně pojmenována REV67; v REV69 je sjednocena.
- Potvrzeno, že REV68 nemělo akvizici DS18B20; GPIO4 bylo pouze rezervované.
- Přidán `WaterTemperature` validační modul a jeho kopie do obou sketch `src/`; ESP32 #2 používá OneWire + DallasTemperature, vyhledává právě jedno čidlo rodiny 0x28, čte asynchronně při 12 bitech a označuje neplatné/zastaralé vzorky jako nedostupné.
- Teplota je pouze informativní; není vstupem pump permit, relé ani řízení ventilů.
- Přidán hostitelský test chybových stavů, rozsahu a freshness a statická kontrola integrace.
- W5500 web zůstává odložen: kandidátní WebServer_ESP32_W5500 vyžaduje ESP32 core 2.0.6+; alternativní SparkFun řešení zasahuje nízkoúrovňově do SPI/interruptů a může kolidovat se současnou Ethernet/SPI obsluhou. Přesná instalovaná verze core není známa.
- Nativní C++ 77/77 PASS; Python 17/17 PASS; statické integrační kontroly PASS.
- Arduino target build, fyzické DS18B20 zapojení, pull-up rezistor a W5500 LAN web nebyly ověřeny.

## REV89 — bounded HTTP request parser

- Added `firmware/common/HttpRequestParser.h`, mirrored to both ESP32 sketch `src/` trees.
- Added C++11 host tests for GET/POST, query/form decoding, duplicate parameters/headers, malformed escapes, unsupported transfer encoding, incomplete/oversized bodies, and a 300-byte calibration-form value.
- The parser request context is 6120 bytes on the host ABI used for the check; future integration must allocate it statically or in another long-lived bounded context, not as a large local callback variable.
- `tests/run_native_tests.sh`: 80 native test programs and all static integration checks PASS.
- Parser test also passed AddressSanitizer + UndefinedBehaviorSanitizer.
- Production `.ino` files are unchanged. No Arduino target build, live HTTP test, Ethernet test or physical hardware test was performed.
