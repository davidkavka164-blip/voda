# Testovací matice — první návrh

| ID | Scénář | Očekávaný výsledek | Stav |
|---|---|---|---|
| T01 | Start bez uložených Wi-Fi údajů | AP s plným webem; bezpečné výstupy | Neprovedeno |
| T02 | Start s dostupnou Wi-Fi | Připojení v limitu; AP se nespouští | Neprovedeno |
| T03 | Krátký výpadek Wi-Fi po startu | Opakované připojování; ne okamžité AP | Neprovedeno |
| T04 | Snímač otevřený/zkratovaný/mimo rozsah | Stav INVALID/ERROR, žádné falešně platné měření | Neprovedeno |
| T05 | Hladina studny dosáhne transfer minima | Transfer zablokovat/ukončit; samo o sobě nevypínat čerpadlo | Neprovedeno |
| T06 | Hladina dosáhne kritické hranice | Bezpečnostní zásah; hystereze do obnovy | Neprovedeno |
| T07 | Výpadek ESP32 #1 ↔ #2 | ESP32 #2 zachová lokální bezpečnost; bezpečný stav transferu | Neprovedeno |
| T08 | Cíl překročí kapacitu dolní nádrže | Transfer se nespustí | Neprovedeno |
| T09 | Uživatel stiskne STOP během transferu | Ventil do bezpečného stavu; log neúplného transferu | Neprovedeno |
| T10 | Transfer nedosáhne cíle v timeoutu | Bezpečné ukončení a alarm | Neprovedeno |
| T11 | Kalibrace studny mimo měřený úsek | Staré body zůstávají zachovány | Neprovedeno |
| T12 | Restart po uložení kalibrace | Body a nastavení přežijí restart | Neprovedeno |
| T13 | Denní přítok +35 L, +20 L, pokles, +15 L | Celkem 70 L | Neprovedeno |
| T14 | Restart s neplatným časem | Historie se nepřiřadí falešnému datu | Neprovedeno |
| T15 | OTA přerušeno / chybný soubor | Chyba je oznámena; nastavení zachováno; systém obnovitelný | Neprovedeno |
| T16 | Restart/boot ESP32 s připojenými výstupy | Žádné nechtěné otevření ventilu ani nebezpečné povolení čerpadla | Neprovedeno |
| T-PUMP-EMERG-01 | Nouzové povolení bez platného čidla po fyzické kontrole hladiny | Příslušné bezpečnostní relé se povolí; běžná tlaková automatika zůstává řídicím prvkem | Nativně testováno; hardware neproveden |
| T-PUMP-EMERG-02 | Nouzové povolení při aktivním kritickém latch | Po potvrzení se softwarový latch překoná, diagnostický příznak zůstane zachován | Nativně testováno; hardware neproveden |
| T-PUMP-EMERG-03 | Nouzové povolení při chybné konfiguraci nebo nedostupném/poškozeném NVS | Po potvrzení se softwarová blokace překoná; stav poruchy zůstane zobrazen | Nativně testováno; hardware neproveden |
| T-PUMP-EMERG-04 | Nouzové povolení při platném měření a jiné softwarové závadě | Nouzový režim lze aktivovat i bez poruchy čidla | Nativně testováno; hardware neproveden |
| T-PUMP-EMERG-05 | Nouzový režim bez fyzického potvrzení | Požadavek je odmítnut; relé zůstane odpojeno | Nativně testováno; staticky ověřeno |
| T-PUMP-EMERG-06 | Restart během nouzového režimu | GPIO startují LOW; nouzový stav se neobnoví | Nativně testováno; hardware neproveden |
| T-PUMP-EMERG-07 | Nouzový režim při neinitializovaném výstupním relé | Výstup zůstane fail-off; nouzový režim neobchází fyzickou připravenost relé | Nativně testováno; hardware neproveden |
| T-COMM-01 | Ztráta čerstvých dat ESP32 #2 v PRECHECK transferu z horní nádrže | Odmítnout transfer s důvodem COMMUNICATION; všechny ventily zavřené | Nativně testováno v 0.68 |
| T-COMM-02 | Ztráta čerstvých dat ESP32 #2 během RUNNING transferu z horní nádrže | Přerušit transfer, vrátit VALVES_CLOSE_ALL a důvod COMMUNICATION | Nativně testováno v 0.68 |
| T-COMM-03 | Opožděný UDP paket z předchozí boot epochy po přijetí nové epochy | Odmítnout paket z posledních čtyř opuštěných epoch; nepřepsat aktuální peer stav | Nativně testováno v 0.69 |

## Nové nativní testy v 0.8
- `test_well_geometry`: výchozí a upravené rozměry, válcový odhad, omezení hloubky, priorita měřených bodů, označení odhadu, checksum a validace uložené kalibrační křivky.
- `WellGeometry` ověřuje, že změna průměru mění geometrický odhad, ale nemění hodnoty měřené křivky.

## Nativní testy v 0.7
- `test_sensor_config`: výchozí konfigurace, rozsahové validace, úprava kalibrace, detekce poškozeného kontrolního součtu.
- ADC průměrování a `Preferences` jsou platformní části; zatím nemají simulátor ani hardware test. Vyžadují samostatné ověření na ESP32.


## Nativní testy v 0.9
- `test_well_calibration_editor`: zachování bodů mimo přeměřený úsek, nahrazení bodů uvnitř úseku, odmítnutí nemonotónního výsledku, ochrana před smícháním absolutního a nepotvrzeného objemového základu a odmítnutí bodů za využitelnou hloubkou.


## Nativní testy v 0.9
- `test_well_calibration_editor`: zachování bodů mimo přeměřený úsek, nahrazení bodů uvnitř úseku, odmítnutí nemonotónního výsledku, ochrana před smícháním absolutního a nepotvrzeného objemového základu a odmítnutí bodů za využitelnou hloubkou.


## Nativní testy v 0.10
- `test_nvs_well_calibration_store`: NVS API přes testovací `Preferences` stub; zápis nové křivky, načtení po znovuotevření, merge lokálních bodů a odmítnutí poškozeného uloženého záznamu. Tento test nenahrazuje test na skutečném ESP32.


## Nativní testy v 0.12
- `test_nvs_daily_inflow_store`: uložení a opětovné načtení historie, zachování datovaného i nedatovaného přítoku, odmítnutí neplatných dat a ochrana před automatickým přepsáním poškozeného záznamu. Test používá hostitelský `Preferences` stub a nenahrazuje fyzické ověření ESP32.
- `test_water_math`: export/obnova snapshotu historie a validace příznaků snapshotu.
- Nativní testovací programy: 9; všechny prošly v poslední kontrole. Scénáře T01–T16 nadále čekají na integrační/hardwarové provedení.


## Nativní testy v 0.13
- `test_peer_protocol`: CRC, validace verze/ID/rozsahů, explicitní 52bajtová serializace, odmítnutí poškozených rámců, duplicit a starých sekvencí, boot ID a stáří měření.
- `test_nvs_sensor_config_store`: uložení/načtení elektrické kalibrace a odmítnutí přepsání poškozeného záznamu.
- `test_nvs_daily_inflow_store` a `test_water_math`: persistentní snapshot historie, nedatované přírůstky, validace data a 30denní klouzavé okno.
- Celkem 11 nativních testovacích programů; všechny prošly při poslední kontrole. Skutečný Arduino build a hardware test stále čekají.


## Nativní testy v 0.15
- `test_nvs_well_protection_store`: validace bezpečnostních mezí vůči hloubce, uložení/načtení konfigurace, persistence kritické blokace a odmítnutí přepsání poškozeného záznamu.
- Celkem 12 nativních testovacích programů; všechny prošly i s ASan/UBSan. Integrační a fyzické scénáře T01–T16 stále čekají na skutečné zařízení.

- `test_nvs_upper_tank_protection_store`: obnovení kritické blokace horního čerpadla, validace CRC a odmítnutí přepsání poškozeného záznamu.


## Nativní testy v 0.30
- `test_sample_freshness`: samostatná dostupnost vzorků, přesná hranice maximálního stáří, odmítnutí starého vzorku a wrap-safe chování při přetečení `millis()`.
- Integrační revize odhalila, že dřívější společná podmínka čerstvosti spojovala studnu a dolní nádrž; testovaná helper funkce se nyní používá samostatně pro oba snímače.
- Celkem 18 nativních testovacích programů; všechny prošly běžným během i ASan/UBSan. Fyzické scénáře T01–T16 zůstávají neprovedené.


## Nativní testy v 0.31
- `test_transfer_history`: validace záznamu, kruhové přepisování po dosažení kapacity 12, detekce duplicitních ID a odmítnutí poškozeného snapshotu.
- `test_nvs_transfer_history_store`: uložení a načtení historie přes hostitelský `Preferences` stub, CRC a zákaz přepsání poškozeného záznamu.
- Celkem 20 nativních testovacích programů; všechny prošly běžným během i ASan/UBSan. Fyzické scénáře T01–T16 zůstávají neprovedené.


## Revize 0.39 — handshake wire codec
| Test | Pokrytí |
|---|---|
| `test_peer_handshake_wire` | HELLO/CHALLENGE/FINISH round-trip; pevné délky; magic/verze/typ; strukturální validace; nulování výstupu při chybě; celý HMAC handshake po wire encode/decode a shoda session key |


## Revize 0.42 — handshake timeout
| Test | Pokrytí |
|---|---|
| `test_peer_handshake_timeout` | validace timeoutu, wrap-safe elapsed, přesná hranice, consume/cancel |
| `test_timed_peer_handshake_responder` | pending handshake timeout, FINISH na hranici/po lhůtě, nulování session key, úspěšný jednorázový FINISH před lhůtou a millis wrap-around |


## Revize 0.43 — směrové klíče peer session
| Test | Pokrytí |
|---|---|
| `test_peer_session_keys` | Shodné odvození na obou stranách, odlišné klíče I→R/R→I, vazba na device ID a boot ID, odmítnutí neplatného klíče/ID/boot ID a nulování výstupních klíčů při chybě |
| `test_peer_command_frame` | COMMAND rámec ověřen správným směrovým klíčem a odmítnut opačným směrovým klíčem |


## Revize 0.44 — PeerSessionCodec
- [x] Initiator→responder frame signed with I→R key and accepted by responder.
- [x] Responder→initiator frame signed with R→I key and accepted by initiator.
- [x] Reject frame when verified with the wrong directional key/context.
- [x] Reject mismatched peer boot ID and invalid local/peer identity context.
- [x] Internal directional key copy is wiped on destruction; codec is non-copyable.
- [ ] UDP/W5500 integration and live handshake/session lifecycle (not implemented).


## Revize 0.48 — persistentní anti-replay
- `test_persistent_peer_replay_guard`: checkpoint, CRC, atomický save contract, rezervace po blocích, fail-closed při chybě/corrupt záznamu, restart přijímače, peer boot rollover a sekvenční hranice.
- `test_peer_command_session_persistent`: HMAC command frame → persistent guard → RAM request-ID guard; simulovaný reboot receiveru a odmítnutí sequence z dříve rezervovaného bloku.
- Oba nové testy: AddressSanitizer + UndefinedBehaviorSanitizer.


## Revize 0.49 — ESP32 NVS replay žurnál
- `test_peer_replay_store`: dvouslotový Preferences/NVS backend, generace, kontrolní čtení, výpadek zápisu, fallback při poškození novějšího slotu, fail-closed při poškození obou slotů a restartová integrace s `PersistentPeerReplayGuard`.
- Test běží nad hostitelským stubem Preferences; nenahrazuje fyzický power-cut test ani build pro Arduino-ESP32.


## Revize 0.50 — odesílací sekvenční čítač
- `test_peer_sequence_allocator`: start bez boot ID, sekvence od 1, posun nad autentizovaný floor, odmítnutí zpětného posunu, zachování při reconnectu, reset při novém boot ID a vyčerpání rozsahu.
- `test_peer_command_session_persistent`: po restartu přijímače se synchronizuje uložený floor 64; `prepareOutgoingRequest()` automaticky vytvoří sekvenci 65 a přijímač ji přijme.
- Oba testy byly samostatně spuštěny s AddressSanitizer a UndefinedBehaviorSanitizer.
- Celá nativní sada: 37 testovacích programů, všechny prošly.
- Autentizovaná síťová výměna floor a W5500 transport zůstávají neprovedené.


## Revize 0.51 — autentizovaný replay floor v handshake
- `test_peer_replay_store`: responder načte floor 128 z dvouslotového NVS replay journalu, vloží jej do CHALLENGE; HMAC ověření uspěje a pozměněný floor je odmítnut. Nový peer boot epoch začíná floor 0; corrupt storage odmítne načtení.
- `test_peer_handshake`: po úspěšném ověření CHALLENGE s floor 128 se sender allocator posune a přidělí sekvenci 129; manipulace s floor zneplatní HMAC.
- `test_peer_handshake_wire`: CHALLENGE verze 2 / 84 B round-trip; stará verze 1 je odmítnuta.
- Nativní sada 37/37 prošla; nové NVS/handshake a wire testy prošly ASan/UBSan.

## Revize 0.52 — handshake READY
- [x] READY HMAC je platný pouze se session key odvozeným z aktuálního handshake.
- [x] Změněný READY tag a nesprávný session key jsou odmítnuty.
- [x] Wire READY kontroluje přesnou délku, verzi a typ zprávy.
- [x] Plná testovací sada: 38/38.
- [ ] READY přes skutečný W5500 UDP transport a výpadek/ztrátu READY rámce — čeká na síťový orchestrátor a hardware.


## Revize 0.53 — koordinátor handshake iniciátora
| Test | Pokrytí |
|---|---|
| `test_peer_handshake_initiator` | ověření CHALLENGE před replay floor; odmítnutí pozměněného floor bez posunu čítače; floor 64 → sequence 65; blokování session key před READY; identický FINISH retry; odmítnutí neplatného READY; shoda session key po READY; nulování klíče po cancel |

Nativní testovací programy: 39/39 prošlo. Nový test prošel samostatně ASan/UBSan.


## Revize 0.55 — bounded handshake retry policy
| Test | Pokrytí |
|---|---|
| `test_peer_handshake_retry_policy` | validace intervalu a limitu pokusů; okamžitý první pokus; časování dalších pokusů; zákaz pokusu nad limit; vyčerpání; wrap-safe přetečení `millis()` |

Nativní testovací programy: 41/41 prošlo. Nový test prošel samostatně ASan/UBSan.


## Revize 0.57 — timed handshake initiator
- `TimedPeerHandshakeInitiator`: počáteční HELLO, neměnná retransmise HELLO/FINISH, samostatné bounded retry fáze, wrap-safe timeout, vyčerpání pokusů, úspěšné ověření READY a okamžité fail-closed při neplatném READY.
- Nativní testovací programy: 42/42 prošlo. Nový test prošel ASan/UBSan.

## Revize 0.66 — obnova transferu po restartu
- `test_transfer_restart_guard`: přechody PREPARED/ACTIVE/RESULT_PENDING, restart do INTERRUPTED, zákaz automatického obnovení, explicitní potvrzení, fail-closed při poškození a selhání zápisu, vynucený stav READY až po potvrzení.
- `test_nvs_transfer_journal_store`: dvouslotový NVS journal, generace, CRC32, read-back, fallback na předchozí platný slot, detekce poškození obou slotů, kombinace skutečného přenositelného guardu s NVS stubem, opakovaný boot bez zbytečného přepisu flash a selhání zápisu.
- `test_transfer_logic`: doplněn `forceAbort()` a ověření `VALVES_CLOSE_ALL`.
- Celá sada: 45/45 standardně a 45/45 s AddressSanitizer + UndefinedBehaviorSanitizer.
- Hostitelský NVS stub nenahrazuje fyzický výpadek napájení ani cílový Arduino-ESP32 build.


## Revize 0.67 — hraniční testy NVS transfer journalu
- Ověřen namespace `hwc_xfer_jrnl` (13 znaků).
- Poškozený novější slot je nahrazen až po zachování platného fallback slotu; následné načtení vrací nový záznam.
- Ověřeno generační pořadí při přetečení `uint32_t`: generace 0 je novější než `0xFFFFFFFF`.
- Celá sada: 45/45 standardně a 45/45 s AddressSanitizer + UndefinedBehaviorSanitizer.



## Revize 0.69 — peer boot epoch a UDP pořadí
| Test | Pokrytí |
|---|---|
| `test_peer_protocol` | Přijetí nové boot epochy; odmítnutí opožděného datagramu z nedávno opuštěné epochy; stávající kontrola duplicitních/starých sekvencí a freshness. |
| `check_peer_boot_identity.sh` | Statická kontrola, že `esp_random()` boot ID je voláno pouze v jednorázové inicializaci ze `setup()` a `startPeerEthernet()` nemění boot ID ani sekvenci. |
| `check_peer_boot_epoch_transfer_abort.sh` | Kontroluje jednorázovou detekci nové peer boot epochy a bezpečné ukončení aktivního transferu horní → dolní s důvodem COMMUNICATION; nový požadavek je povinný. |

Celkem: 45/45 nativních testovacích programů; statická kontrola boot identity navíc. Kompletní ASan/UBSan: 45/45.

| LocalServiceAuthorization | Physical-service authorization policy: hold duration, debounce/bounce resistance, one-shot token, release-before-rearm, expiry, millis rollover | `test_local_service_authorization.cpp` |
| T-CRED-SERIAL-01 | Provisioning serial frame bez otevřené fyzické relace | Ignorovat vstup, úložiště zůstane EMPTY | Nativně testováno v 0.91 |
| T-CRED-SERIAL-02 | Fragmentovaný validní rámec v platné fyzické relaci | Jediný provisioning attempt proběhne a relace se uzavře | Nativně testováno v 0.91 |
| T-CRED-SERIAL-03 | Šum před magic, chybný CRC, verze, command nebo délka | Šum ignorovat; chybný rámec odmítnout a relaci zavřít | Nativně testováno v 0.91 |
| T-CRED-SERIAL-04 | Control/NUL znaky nebo neúplná/extra pole | Odmítnout bez zápisu credentials | Nativně testováno v 0.91 |
| T-CRED-SERIAL-05 | Inter-byte timeout přesně 2 000 ms / max payload | Neúplný frame vynulovat; maximální legální rámec bezpečně přijmout | Nativně testováno v 0.91 |
| T-CRED-SERIAL-06 | Odpověď servisního protokolu | Přenést pouze status byte, nikdy původní hesla | Nativně testováno v 0.91 |


Aktuální revize DEV KIT 0.91: **50/50 nativních testovacích programů** a **50/50 ASan/UBSan**; peer boot identity a W5500 recovery statické kontroly PASS.


## Hostitelský klient provisioningu — revize 0.92
| Test | Pokrytí | Stav |
|---|---|---|
| Python CRC vector | CRC-16/CCITT-FALSE referenční vektor `123456789` → `0x29B1` | PASS |
| Request frame layout | HW magic, verze, command, big-endian payload length a CRC nad přesným rozsahem | PASS |
| Credential bounds | Minimum/maximum délek, max. 170B rámec, odmítnutí krátkých/dlouhých hodnot | PASS |
| ASCII validation | Ne-ASCII, NUL/řídicí znaky a neplatné hodnoty jsou odmítnuty | PASS |
| Result response | Pevná 9B odpověď, magic/verze/typ/délka/CRC | PASS |
| Fake serial exchange | Jeden request write, flush a parse status response bez skutečného COM portu | PASS |
| Physical serial integration | Skutečný UART/USB, DTR/RTS, fyzický servisní GPIO, adapter v `.ino` | Neprovedeno |


## Rozšířené klientské testy revize 0.92
| Test | Pokrytí | Stav |
|---|---|---|
| Cross-language golden frame | Python host client and C++ device parser test build the exact same 59-byte SET_CREDENTIALS frame | PASS |
| Preview confidentiality | CLI preview output does not contain AP or administrator password | PASS |
| Send integration gate | `--send` without `--firmware-integrated` fails before prompting or opening a serial port | PASS |


## Nativní testy v 0.94 — bezpečný start AP
- `test_credential_startup_gate`: načtení existujících údajů, první vytvoření, přesný read-back, write-then-fail, chybějící/poškozený read-back, odmítnutí neplatných existujících údajů, IO_ERROR, ASCII a hranice AP/admin hesel.
- `test_nvs_credential_text_storage`: Preferences text adapter, validace a životní cyklus open/close. Používá hostitelský stub; nenahrazuje skutečný ESP32 NVS test.
- `check_credential_startup_gate_integration.sh`: statická kontrola, že oba `.ino` ověřují přihlašovací údaje před `WiFi.softAP` a HTML dashboard ESP32 #1 neobsahuje AP password.
- Celkem 53 nativních testovacích programů, standardní i ASan/UBSan; Python provisioning klient 17 testů.

## Jednotné credential úložiště a migrace — revize 0.95
| Test | Pokrytí | Stav |
|---|---|---|
| `test_credential_legacy_migrator` | Migrace pouze z EMPTY, zachování validního cíle jako autority, zákaz migrace při CORRUPT/IO_ERROR, odmítnutí nevalidního legacy zdroje, fail commit | PASS |
| `test_credential_store` | Délkové limity a odmítnutí řídicích/non-ASCII znaků ve všech credential polích | PASS |
| `test_nvs_credential_legacy_migrator` | End-to-end migrace přes Preferences NVS backend, čerstvý backend po rebootu, načtení aktivního selectoru/slotu, fail-closed po poškození vybraného slotu | PASS |
| `check_credential_startup_gate_integration.sh` | Oba `.ino` nejprve načítají unified store, migrují legacy pouze při EMPTY a neprovádějí legacy fallback při CORRUPT/IO_ERROR | PASS |
| Fyzická migrace na ESP32 | Reálná NVS, výpadek napájení při každé fázi, Arduino-ESP32 build | Neprovedeno |


## HTTP autentizace — revize 0.97
| Test | Pokrytí | Stav |
|---|---|---|
| `test_web_auth_rate_limiter` | Per-IP threshold, 60s window/block, exact expiry, success reset, wrap-safe millis, bounded-table saturation and preservation of active blocks | PASS |
| `check_web_auth_rate_limiter_integration.sh` | Obě `.ino` kontrolují blokaci před `authenticate()`, zaznamenávají neúspěch i úspěch a používají společný limiter | PASS |
| Standardní nativní testy | 57/57 programů | PASS |
| ASan/UBSan | 57/57 programů | PASS |
| Python provisioning klient | 17/17 | PASS |
| Arduino build / fyzický HTTP test | Neprovedeno | Otevřeno |


## REV16 — nouzové ruční ovládání ventilů (logická vrstva)

- `test_manual_valve_control`: nouzová aktivace bez skutečné relevantní poruchy nebo bez fyzického potvrzení je odmítnuta; při poruše a potvrzení je povolena; blokace při aktivním transferu/nevyřešeném žurnálu; ON zůstává aktivní do STOP bez objemového cíle; pouze jeden směr; změna směru až po CLOSE_ALL; nový objekt po restartu je neaktivní a CLOSE_ALL.
- `check_manual_valve_control_integration.sh`: endpointy a volání jsou přítomny pouze v hlavní jednotce; automatický transfer je blokován během ručního režimu; fyzické relé GPIO nejsou řízeny; společný modul je synchronizován.
- Fyzické scénáře ventilů zůstávají neprovedené, protože GPIO/relé záměrně nejsou aktivní.

## REV17 — fyzické relé nouzového ručního ovládání ventilů

- `test_valve_relay_output`: výchozí OFF, neplatná konfigurace, active-HIGH/active-LOW polarita, mapování studna/horní cesta, break-before-make, opakovaný povel bez cvakání relé, CLOSE_ALL a fail-safe neplatný příkaz.
- `check_manual_valve_control_integration.sh`: relé GPIO16/17 inicializována na LOW před startup delay; pouze ESP32 #1 ovládá ventilová relé; relé čerpadel a výstupy ESP32 #2 zůstávají neaktivní.
- Automatický transfer je stále simulace; fyzické relé/ventily nebyly na desce ověřeny.
- Standardní nativní testy: 61/61; ASan/UBSan: 61/61; Python: 17/17.


## REV18 — automatický transfer přes fyzická relé
- `check_automatic_transfer_relay_integration.sh`: explicitní mapování příkazů TransferLogic na relé driver, blokace při neinicializovaném driveru, vazba na restart journal a pořadí update → sync; kontrola, že pump relays a ESP32 #2 zůstávají bez ventilových výstupů.
- `test_valve_relay_output`: společný výstupní driver nadále ověřuje OFF při startu, obě trasy, break-before-make, idempotenci, aktivní HIGH/LOW a fail-close při neplatném příkazu.
- Fyzické sepnutí 5V reléové desky, kompatibilita 3,3V vstupů, COM–NO zapojení, chování ventilů a skutečný průtok vyžadují zkoušku na hardware; nejsou pokryty hostitelskými testy.

## Revize 1.00 – nouzový ventil a zachování ochrany čerpadla
- `test_emergency_valve_pump_guard`: při platném měření studny musí nouzové otevření studniční trasy respektovat stávající `pumpPermit`; při neplatném/nepoužitelném měření může obsluha po fyzické kontrole použít nouzový ventilový režim.
- Integrační kontrola ověřuje, že ochrana horního čerpadla zůstává lokálně vyhodnocována na ESP32 #2 a není zaměněna za vzdálenou ventilovou blokaci.
- Testy neprokazují fyzické odpojení čerpadla: výstupy pump relé nejsou v této revizi aktivovány.

## REV23 — audit fyzického povolení čerpadel
- `check_pump_safety_output_gate.sh`: ověřuje, že pump safety GPIO nejsou inicializována ani spínána před potvrzením elektrického zapojení; současně kontroluje dokumentaci nevyřešeného GPIO32.
- Audit porovnává GPIO návrh s hlavním dokumentem 1.3: ESP32 #1 / GPIO25 pro studniční čerpadlo, ESP32 #2 / GPIO25 pro čerpadlo horní nádrže; ESP32 #1 / GPIO32 zůstává bez potvrzené funkce.
- Zvlášť otevřené zůstává, zda relé přerušuje nízkonapěťový povolovací obvod nebo výkonový obvod motoru a který kontakt COM/NO/NC vytváří bezpečný stav.
- Fyzické pump safety výstupy zůstávají deaktivované. Test neprokazuje elektrickou funkci relé ani vypnutí čerpadla.

## REV25 — nastavitelná ochrana čerpadla dolní nádrže
- `test_lower_tank_pump_protection`: samostatné nastavitelné minimum a hystereze, startovní blokace do obnovovací hladiny, kritická blokace, hysterezní pásmo, obnova, neplatné měření, chyba NVS a obnovený kritický latch.
- `test_nvs_lower_tank_pump_protection_store`: dvouslotové CRC NVS, načtení konfigurace, změna kritického latch, ověřený zápis a fail-closed po přerušeném zápisu.
- `check_lower_tank_pump_protection_integration.sh`: webové nastavení, měření dolní nádrže, uložení latch a přiřazení GPIO32; kontroluje, že fyzický výstup zůstává vypnutý.
- Standardní nativní testy: 64/64 PASS; ASan/UBSan: 64/64 PASS; Python: 17/17 PASS; integrační kontroly PASS.
- Arduino cílový build a fyzické testy: neprovedeny.


## REV27 – výstupní adaptér bezpečnostních relé čerpadel
- `test_pump_safety_relay_output`: výchozí OFF, commissioning gate, povolení pouze při gate ON, okamžité vypnutí při odebrání permitu, active-HIGH/active-LOW, idempotence.
- `check_pump_safety_output_gate.sh`: tři samostatné lokální výstupy, jejich vazba na vlastní `pumpPermit`, čerstvost měření a aktivní fyzické výstupy na základě funkčního zapojení ze samostatného projektu studny.
- Fyzické odpojení motorů, COM–NO, 3,3V trigger, pull-down a motorový proud: neprovedeno.


## REV29 – audit nezávislosti pumpových ochran
- `check_pump_protection_independence.sh`: kontroluje, že studna a dolní nádrž používají na ESP32 #1 vlastní permit a vlastní čerstvost měření; horní pumpové relé na ESP32 #2 závisí pouze na místním měření a permitu; nouzový ventilový režim neobchází platnou ochranu zdrojového čerpadla.
- Logické testy potvrzují, že nouzový režim pumpové ochrany studny/horní nádrže nepřekoná kritický latch, chybnou konfiguraci ani chybu úložiště. (Dolní čerpadlo dostalo vlastní nouzový režim v REV30.)
- Nativní testy 65/65 PASS; ASan/UBSan 65/65 PASS; Python 17/17 PASS; všechny integrační kontroly PASS. Arduino build a fyzické testy všech tří výstupů zůstávají neprovedené.


## REV30 — nouzové povolení všech tří čerpadel při poruše čidla
- `test_lower_tank_pump_protection`: výslovné fyzické potvrzení umožní nouzové povolení dolního čerpadla pouze při neplatném čidle; bez potvrzení, při kritickém latch, neplatné konfiguraci nebo chybě NVS zůstává výstup blokován.
- Web ESP32 #1 má samostatný endpoint `/lower-pump-emergency` a tlačítko pro aktivaci/ukončení nouzového režimu dolního čerpadla.
- Nouzový stav dolního čerpadla je pouze v RAM a po restartu se neobnovuje.
- Všechna tři čerpadla nyní mají shodný princip nouzového povolení při poruše vlastního čidla; jejich ochranné stavy a relé zůstávají nezávislé.

- Restart audit REV30: unit tests for well, lower and upper pump protections verify that an active emergency override is not restored by constructing a fresh controller after reboot. `check_pump_safety_output_gate.sh` verifies all three GPIO outputs are driven LOW in setup before the startup delay. Transfer journal and manual valve controller retain their existing no-auto-resume behavior.

## REV34 — audit Wi-Fi/AP/W5500 obnovy
- `check_network_recovery_safety.sh`: ověřuje Wi-Fi STA retry ESP32 #1, start fallback AP po 30 s, souběžný AP+STA režim, lokální AP ESP32 #2 a skutečnost, že W5500 je UDP peer transport, nikoli web server.
- Stejný test kontroluje, že počáteční DHCP může trvat až 5 s, ale retry z hlavní smyčky obou jednotek je omezen na 1 s (response timeout 250 ms).
- Nativní testy: 65/65 PASS; Python: 17/17 PASS; integrační kontroly včetně nové síťové kontroly PASS; `compileall` a shell syntax PASS.
- Reálná Wi-Fi/AP koexistence, DHCP, W5500 kabel/link recovery, UDP broadcast a cílový Arduino build nebyly fyzicky ověřeny.

## REV35 web security

- `check_web_csrf_origin_guard.sh`: checks Origin/Host collection and same-origin POST enforcement on both ESP32 web interfaces.

## REV36 – OTA finalization guard
- `check_ota_safety_interlock.sh`: verifies ESP32 #2 rechecks the upper pump emergency override immediately before OTA finalization and aborts the write if active.
- This is a source-level integration check; actual interrupted OTA behavior must still be tested on the physical ESP32 #2.

## REV37 – sensor calibration startup-state audit

- Audit confirms the NVS sensor-config store rejects corrupt records and refuses silent overwrite.
- Audit identifies that ESP32 #1 and #2 currently substitute provisional defaults for missing, corrupt and NVS-unavailable states without propagating calibration trust to pump/transfer gates.
- Required implementation tests for `MISSING_PROVISIONAL`, `VALID_STORED`, `INVALID_PRESENT` and `NVS_UNAVAILABLE` are listed in `docs/SENSOR_CALIBRATION_STARTUP_AUDIT_REV37.md`.
- No firmware change is included in REV37; this item remains open.

## REV38 – použitelnost elektrického nastavení snímačů
- `test_sensor_config`: ověřuje stavy elektrického nastavení snímače; objemová kalibrace studny je samostatná.
- `check_sensor_calibration_trust_integration.sh`: kontroluje rozlišení všech čtyř stavů při startu obou jednotek, propojení se třemi lokálními pumpovými ochranami a neplatnost horní peer hladiny při nepoužitelném elektrickém nastavení snímače.
- REV38 uzavírá zdrojový nález REV37. Arduino cílový build, fyzické NVS testy a testy skutečných čerpadel/ventilů zůstávají neprovedené.


## REV39 – oddělení výšky hladiny a objemové kalibrace
- Přejmenované typy, stavy a proměnné výslovně označují elektrické nastavení snímače (`SensorConfig`), nikoli objemovou křivku studny (`WellCalibration`).
- `WellCalibration` zůstává pouze ve výpočtu objemu; její stav není vstupem do pumpových ochran ani transfer logiky.
- `check_sensor_calibration_trust_integration.sh` ověřuje použitelnost elektrického nastavení pro výpočet výšky a návazné ochrany.
- Jde o zpřesnění názvosloví a dokumentace bez změny dosavadního bezpečnostního chování REV38.


## REV40 – missing electrical sensor setup fails closed

- `SENSOR_CONFIG_MISSING_PROVISIONAL` is diagnostic/commissioning only and is not accepted by `sensorConfigUsableForLevelMeasurement()`.
- Only `SENSOR_CONFIG_VALID_STORED` permits safety use of the measured height. Corrupt, missing and unavailable NVS states all block normal pump/transfer decisions.
- `test_sensor_config` verifies all four states; `check_sensor_calibration_trust_integration.sh` verifies the shared policy and all three sensor-to-protection paths.
- `WellCalibration` remains limited to volume estimation and cannot block pump or transfer operation.

## REV43 – sensor recovery re-arm
- [x] Missing/corrupt/NVS-unavailable electrical sensor configuration blocks normal permit for all three pump protections.
- [x] After recovery, cached pre-recovery samples are invalidated before the trust state is changed.
- [x] Fresh valid reading below recovery threshold remains blocked for well/lower/upper pump.
- [x] Fresh valid reading at recovery threshold can clear startup validation, subject to existing critical latch and storage/configuration gates.

## REV44 – sensor recovery power interruption

- `test_nvs_sensor_config_powercut`: models a write failure before storage change, a short/partial blob, and a complete new blob with an error returned to the caller. A fresh store instance simulates reboot and verifies that only a complete checksum-valid record can load.
- The test uses a deterministic host stub. It is not a physical flash power-cut test and does not replace a bench test with the exact Arduino-ESP32/ESP-IDF version used for deployment.
- Source audit confirms all three recovery handlers promote the trust state only after write/read-back validation and invalidate the old cached reading before that promotion.

## REV45 – smíšená Wi-Fi/Ethernet peer topologie

- `check_mixed_wifi_ethernet_peer_transport.sh`: ověřuje ESP32 #1 jako Wi-Fi UDP přijímač a ESP32 #2 jako W5500 Ethernet UDP vysílač, včetně fail-closed chování při ztrátě STA.
- `check_network_recovery_safety.sh`: ověřuje Wi-Fi STA reconnect a fallback AP ESP32 #1, Wi-FiUDP listener a W5500/DHCP sender ESP32 #2.
- `check_peer_ethernet_recovery.sh`: ověřuje fyzickou W5500 link recovery pouze na ESP32 #2; ESP32 #1 již na W5500 nezávisí.
- Požadavek sítě: obě zařízení ve stejné LAN/podsíti; router/AP musí propouštět broadcast mezi Ethernetem a Wi-Fi. Guest Wi-Fi, VLAN separation a client isolation nejsou podporované bez další konfigurace.
- Nativní testy: 67/67 PASS; Python: 17/17 PASS; statické integrační kontroly, `compileall` a shell syntax PASS.
- Arduino-ESP32 target build a fyzický broadcast test přes konkrétní router/AP zůstávají neprovedené.


## REV81 — přesné read-back ověření historie/statistiky
- `test_nvs_transfer_history_store`: simulovaný backend oznámí plný zápis, ale zachová starý validní záznam; `save()` musí vrátit false a stará historie zůstává čitelná.
- `test_nvs_daily_inflow_store`: ověřuje standardní persistenci a odmítnutí poškozeného záznamu; `save()` nyní navíc porovnává všechna pole snapshotu po read-backu.
- Standard: 78/78; ASan/UBSan: 76/76; Python: 17/17.


## REV82 — přesný read-back geometrie a kalibrace studny
- `test_nvs_well_calibration_store`: backend ignoruje jeden zápis, přesto vrátí plný počet bajtů; `mergeAndSave()` odmítne úspěch a předchozí kalibrace zůstane načitatelná.
- `test_nvs_well_geometry_store`: stejný scénář pro geometrii; `save()` odmítne falešný úspěch a zachová původní geometrii.
- Standardní nativní sada 78/78 PASS; ASan/UBSan 76/76 PASS.


## REV84 — transakční domácí Wi-Fi STA persistence
- `test_wifi_sta_credential_store`: dvouslotový zápis, exact read-back, zachování předchozí aktivní dvojice při selhání neaktivního slotu, opakování neúplné migrace, falešně úspěšný zápis, legacy partial-pair fail-closed, otevřená síť a neplatný selector.
- `check_wifi_sta_credential_integration.sh`: kontroluje použití úložiště v ESP32 #1 a synchronizaci hlavičky v obou ESP32 `src` stromech.
- Standardní nativní sada 79/79 PASS; ASan/UBSan 77/77 PASS; Python 17/17 PASS. Arduino build a fyzické NVS/brownout testy neprovedeny.


## REV85 — W5500 Ethernet ESP32 #1
- `check_esp32_1_w5500_integration.sh`: statická kontrola SPI pinů, W5500 resetu, DHCP inicializace, link/IP obnovy, dashboardu a explicitního omezení HTTP přístupu na Wi-Fi/AP.
- Fyzický scénář: start s připojeným Ethernetem, DHCP lease, odpojení/připojení kabelu, restart bez kabelu a následné obnovení DHCP. Při všech stavech musí lokální měření a ochrany čerpadel pokračovat. **Na fyzickém ESP32 zatím neprovedeno.**
- Ethernetové HTTP routy, Basic Auth, POST a OTA nejsou v REV85 implementovány ani testovány.
