# HOME WATER CONTROLLER – aktuální audit shody s MASTER 1.3 – REV47

Datum: 2026-11-13  
Základ: zdrojový balíček REV46 + bezpečnostní oprava REV47  
Rozsah: statická kontrola obou sketchů, společných modulů, dokumentace a hostitelských testů.

## 1. Zásadní závěr REV47

Při kontrole bylo potvrzeno, že ESP32 #1 přijímá stavové UDP rámce ESP32 #2 chráněné pouze CRC32. Přijatá hladina a checksum limitů se přesto používaly jako podmínka fyzického otevření ventilu horní → dolní nádrž. CRC odhalí náhodné poškození, ale neověřuje identitu odesílatele a nebrání úmyslnému podvržení rámce na stejné LAN.

To odporovalo dřívějšímu bezpečnostnímu pravidlu v `OPEN_DESIGN_ITEMS.md` a `COMMUNICATION_PROTOCOL.md`, podle kterého se CRC-only status nesmí použít pro fyzické síťové řízení.

### Opatření REV47

- Běžný automatický transfer horní → dolní nádrž je nyní fail-closed, dokud není skutečně integrována autentizovaná peer session.
- Servisní ruční otevření horní cesty je ze stejného důvodu blokováno.
- Nouzové ruční otevření zůstává samostatnou cestou: vyžaduje explicitní nouzový režim a fyzické potvrzení/ověření obsluhou. Tato výjimka odpovídá uživatelem potvrzenému nouzovému postupu; není založena na důvěře v UDP telemetrii.
- CRC-only stavová telemetrie může zůstat zobrazena pro diagnostiku, ale dashboard výslovně uvádí, že nejde o autentizaci a běžný horní transfer je blokován.
- Opraveny také zastaralé komentáře a text dashboardu, které tvrdily, že relé čerpadel nejsou řízena, přestože bezpečnostní relé již v aktuálním sketchi řízena jsou.

Toto opatření neimplementuje autentizovanou komunikaci. Je to dočasná bezpečnostní brána, která brání tomu, aby se neautentizovaná síťová data používala k povolení fyzického ventilu.

## 2. Stav hlavních oblastí MASTER 1.3

| Oblast | Stav REV47 | Poznámka |
|---|---|---|
| Dvě ESP32 a rozdělení odpovědností | Implementováno v kódu / HW neověřeno | ESP32 #1: studna, dolní nádrž, ventily a hlavní web. ESP32 #2: horní nádrž a lokální ochrana. |
| Běžné řízení čerpadel tlakovou automatikou | Implementováno jako bezpečnostní povolení | ESP32 čerpadla běžně nespíná; bezpečnostní relé povoluje/odpojuje napájení. |
| Tři bezpečnostní relé čerpadel | Kódově integrováno / částečně HW ověřeno | GPIO25/32 na ESP32 #1 a GPIO25 na ESP32 #2. Samostatný studniční výstup byl uživatelem ověřen; kombinované zapojení všech tří relé ne. |
| Nouzové povolení čerpadel | Implementováno / hostitelsky testováno | Vyžaduje výslovné potvrzení fyzické kontroly; přechodné, po restartu se ruší. Nepřemosťuje elektrické ochrany motoru. |
| Ventily a ruční režimy | Implementováno v kódu / HW ověření otevřené | ESP32 #1 ovládá oba ventily. Stav je povel relé, nikoli potvrzená poloha ventilu. |
| Automatický transfer studna → dolní | Implementováno v kódu / HW ověření otevřené | Vyžaduje platné snímače, ochranu zdroje, kapacitu, journal a připravený výstup. |
| Automatický transfer horní → dolní | Záměrně blokován REV47 | Současná UDP telemetrie není autentizována. Znovu povolit až po autentizované peer session a testech. |
| Nouzový ruční transfer horní → dolní | Samostatná obslužná výjimka | Jen po fyzickém ověření hladiny a explicitním nouzovém potvrzení. |
| Dolní nádrž – geometrie 220 × 280 cm | Implementováno | 61,6 l/cm; 11 cm je pouze informativní, nikoli ochranný limit čerpadla. |
| Studna – geometrie a objemová křivka | Implementováno / kalibrace neúplná | Geometrie Ø80/800 cm je minimální základ; absolutní objem vyžaduje známý kotevní bod. |
| Oddělení `SensorConfig` a `WellCalibration` | Implementováno / testováno | Elektrická konfigurace je povinná pro důvěryhodnou výšku; objemová křivka sama neblokuje ochrany ani transfer. |
| Servisní obnova `SensorConfig` | Implementováno v kódu / fyzicky neověřeno | Vyžaduje skutečný referenční bod, známé měřítko a nový čerstvý vzorek po obnově. |
| Horní objemová kalibrace | Chybí | ESP32 #2 odesílá `upperVolumeValid = 0`. |
| Teplota horní nádrže | Chybí | GPIO4 je pouze plánované; DS18B20 se zatím nečte, `temperatureValid = 0`. |
| Peer telemetrie ESP32 #2 → #1 | Implementováno, ale pouze CRC | W5500 Ethernet na #2, Wi-Fi STA UDP příjem na #1; fyzické routerové přemostění broadcastu neověřeno. Telemetrie není autentizována. |
| Autentizovaný peer handshake/session | Moduly existují, integrace chybí | HELLO/CHALLENGE/FINISH/READY, session keys a anti-replay jsou hostitelsky testované přenositelné moduly; nejsou připojeny k UDP/W5500 transportu ani k aktuálnímu řízení. |
| Bezpečné úložiště peer tajemství | Chybí produkční backend | `PeerSecretVault` má fail-closed kontrakt, ale není dodán a ověřen důvěrný ESP32 backend ani fyzický provisioning. |
| Webová autentizace | Částečně implementováno | HTTP Basic Auth, rate limiter a same-origin POST kontrola. HTTP zůstává nešifrované. |
| Vzdálený přístup | Návrh / neověřeno | Pouze VPN přes router; router a VPN nejsou nakonfigurovány. Přímé vystavení HTTP portů je zakázáno. |
| OTA | Implementováno v kódu / cílově neověřeno | .bin upload a bezpečnostní interlock; chybí Arduino build a fyzický test aktualizace/obnovy. |
| NVS a transfer journal | Hostitelsky testováno | Skutečné ESP32 NVS, fyzický power-cut a chování přesného Arduino core nebyly ověřeny. |

## 3. Otevřené blokátory před ostrým provozem

1. **Autentizovaná peer komunikace:** doplnit bezpečný fyzický provisioning sdíleného tajemství a důvěrné úložiště; integrovat handshake HELLO/CHALLENGE/FINISH/READY, session keys, autentizované rámce a persistentní anti-replay do skutečného UDP transportu. Teprve potom lze odstranit `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED = false`.
2. **Síťové ověření:** ověřit broadcast W5500 → router → Wi-Fi STA, klientskou izolaci, VLAN a výpadek/reconnect na skutečném Archer AX12 nebo switchi. Bez routeru nelze tuto část uzavřít.
3. **Cílová kompilace:** sestavit oba sketch projekty pro skutečně používanou desku a Arduino-ESP32 core. V tomto prostředí nebyl nalezen `arduino-cli` ani PlatformIO a cílový build nebyl proveden.
4. **Fyzické ověření:** ověřit všechny tři pumpové výstupy, oba ventilové výstupy, výchozí stav při resetu/výpadku napájení, elektrickou kompatibilitu 3,3V GPIO a skutečné chování ventilů.
5. **Snímače:** provést referenční kalibraci všech tří 4–20mA snímačů v instalaci; ověřit ADC, shunt 150 Ω, vedení, rozsahy a obnovu po výpadku.
6. **Horní nádrž:** doplnit objemový model/kalibraci a teplotní čidlo DS18B20, až budou potvrzeny fyzické parametry a zapojení.
7. **Webové zabezpečení:** doplnit bezpečný provisioning bez úniku hesel přes Serial a posoudit NVS/Flash Encryption. HTTP Basic Auth není šifrování.
8. **Pevné nouzové vypínače:** samostatně navrhnout a ověřit fyzické hardwarové obejití ESP32; toto zapojení není součástí REV47.

## 4. Ověření REV47

- `tests/run_native_tests.sh`: **67/67 nativních testovacích programů PASS**; všechny připojené integrační kontroly včetně nové brány peer autentizace PASS.
- `bash tests/.run_native_tests_asan.sh`: **65/65** testovacích programů v rozsahu tohoto samostatného ASan/UBSan runneru PASS; runner nepokrývá úplně všechny programy standardní sady.
- `python3 -m unittest discover -s tests/python -v`: **17/17 PASS**.
- `python3 -m compileall -q tools tests/python`: PASS.
- `bash -n` všech `tests/*.sh`: PASS.
- Synchronizace společných modulů a credential modulů: PASS.

### Co tyto testy nedokazují

Hostitelské testy a statické kontroly nejsou Arduino-ESP32 cílová kompilace ani test na skutečných deskách. Neověřují skutečnou NVS, fyzické relé, proudové snímače, Wi-Fi/Ethernet broadcast přes router, VPN, OTA ani elektrické bezpečnostní zapojení.
