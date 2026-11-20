# HOME WATER CONTROLLER — audit integrace autentizované peer komunikace REV48

Datum: 2026-11-13  
Rozsah: kontrola skutečného propojení připravených bezpečnostních modulů s UDP transportem ESP32 #1 (Wi-Fi STA) a ESP32 #2 (W5500 Ethernet). Firmware se v této revizi nemění.

## 1. Shrnutí nálezu

Síťová komunikace ve smyslu přenosu stavových dat je implementována. Kryptografická autentizace této stavové cesty implementována není.

To není rozpor s předchozím návrhem: dříve byly dokončeny síťová topologie, UDP status rámec, CRC, boot ID, sekvence, freshness a fail-closed chování při výpadku. Samostatně vznikaly přenositelné kryptografické moduly. Jejich přítomnost a nativní testy však neznamenají, že jsou volány z hlavních sketchů.

## 2. Ověřený stav ve zdrojovém kódu REV47

### ESP32 #1

- `firmware/ESP32_1/ESP32_1.ino::pollPeerWifi()` otevře `WiFiUDP` na portu 4211, přijímá přesně `PEER_STATUS_WIRE_SIZE`, dekóduje CRC status rámec a předá jej `PeerStatusMonitor`.
- Tato funkce nepřijímá ani nezpracovává HELLO, CHALLENGE, FINISH nebo READY.
- Sketch nevolá `TimedPeerHandshakeInitiator`, `PeerHandshakeWire`, `PeerSecretVault` ani `PeerSessionCodec`.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` je explicitně `false`; běžné a servisní použití horní cesty proto zůstává blokováno.

### ESP32 #2

- `startPeerEthernet()` otevře `EthernetUDP` na portu 4212.
- `sendPeerStatus()` sestaví stavový rámec a vysílá jej broadcastem na `255.255.255.255:4211` každé dvě sekundy.
- V hlavním sketchi není volání `peerUdp.parsePacket()`, `remoteIP()` ani příjem handshake zpráv. UDP socket je tedy v aktuálním firmware vysílací, nikoli plnohodnotný responder.
- Sketch nevolá `TimedPeerHandshakeResponderCoordinator`, `PeerSecretVault` ani `PeerSessionCodec`.

### Přenositelné moduly

V obou sketch `src/` existují a jsou testovány moduly HMAC-SHA-256, kanonické handshake wire zprávy, HELLO/CHALLENGE/FINISH/READY, časované retry orchestrace, derivace směrových session klíčů, session codec, command session a replay guard. Jde o stavební bloky; hlavní UDP smyčky je dosud nepoužívají.

## 3. Přesné rozdělení bezpečnostních závislostí

| Vrstva | Stav REV47 |
|---|---|
| UDP přenos ESP32 #2 → ESP32 #1 | Implementován, CRC-only |
| CRC, ID zařízení, boot ID, sekvence, freshness | Implementováno a nativně testováno |
| HMAC a handshake wire formát | Přenositelné moduly implementovány a testovány |
| Časovaná handshake orchestrace | Přenositelné moduly implementovány a testovány |
| Session-bound směrové klíče a codec | Přenositelné moduly implementovány a testovány |
| Skutečné volání handshake z `.ino` a příjem na obou UDP socketech | Neimplementováno |
| Produkční `PeerSecretBackend` s prokázanou ochranou tajemství v klidu | Neimplementováno |
| Fyzický provisioning společného peer tajemství | Přenositelné části existují; board adapter a reálné servisní vstupy nejsou zapojeny do `.ino` |
| Cílový Arduino build a fyzický síťový test | Neprovedeno |

`PeerSecretVault` záměrně odmítá přístup, pokud backend nepotvrdí důvěrné úložiště. Obyčejné `Preferences`/NVS bez ověřené NVS Encryption/Flash Encryption nesmí vracet `confidentialStorageAvailable() == true`.

`NvsPeerReplayStore` je dvouslotový žurnál nad `Preferences` s read-back kontrolou a fail-closed chováním při poškození. CRC a dva sloty ale nešifrují data a samy nebrání fyzickému rollbacku flash. Jeho přítomnost proto neřeší bezpečné uložení dlouhodobého tajemství.

## 4. Zachování schválené topologie — návrh transportního zapojení

Není důvod měnit topologii ani přidávat W5500 do ESP32 #1. Při implementaci lze zachovat stávající porty a role:

1. ESP32 #2 dál vysílá discovery/status broadcast na UDP 4211.
2. ESP32 #1 přijme první validní datagram a zná jeho zdrojovou IP adresu.
3. ESP32 #1 jako handshake iniciátor odešle HELLO unicastem na zdrojovou IP ESP32 #2, port 4212.
4. ESP32 #2 na již otevřeném UDP portu 4212 začne zpracovávat příchozí handshake zprávy a odpoví unicastem na zdrojovou IP/port ESP32 #1 (4211).
5. ESP32 #1 ověří CHALLENGE, odešle FINISH a přijme READY. Session se aktivuje pouze po úplném ověření READY.
6. Teprve po aktivní session se stavové zprávy ESP32 #2 opatří autentizačním tagem odvozeným pro směr responder → initiator. ESP32 #1 přijímá řídicím účelům použitelný stav pouze po ověření tagu, identity, obou boot epoch, sekvence a freshness.
7. Při restartu kterékoliv jednotky, ztrátě linky, timeoutu, chybě replay úložiště nebo zrušení session se autentizovaný stav invaliduje. Opětovné přijetí čerstvého CRC-only rámce samo o sobě nesmí obnovit povolení horní cesty.

Toto je návrh návaznosti na existující transport, nikoli hotová implementace. Před kódováním je nutné navrhnout a otestovat samostatný kanonický autentizovaný status rámec: současný `PeerSessionCodec` podepisuje pevný command frame 68 B a nelze jej bez změny významu použít jako plnohodnotný serializátor všech polí status rámce.

## 5. Pořadí další práce

### Fáze A — platformní a klíčová brána

- Potvrdit přesnou instalovanou verzi `esp32 by Espressif Systems` v Arduino IDE 1.8.19.
- Pro tuto verzi ověřit dostupné API NVS Encryption a kompatibilitu Arduino startupu, `Preferences`, Wi-Fi a OTA.
- Zvolit pouze prokazatelně bezpečné řešení dlouhodobého tajemství. Pokud stávající Windows 7 / Arduino core neumožní bezpečné a reprodukovatelné nastavení, nepředstírat ochranu a samostatně vyhodnotit externí secure element nebo změnu vývojového prostředí.
- Navrhnout fyzický provisioning obou jednotek bez výpisu tajemství do Serial, HTML nebo logů.

### Fáze B — transportní handshake

- Přidat oboustranné UDP zpracování na již otevřených socketech.
- Implementovat omezený stavový automat iniciátora/responderu s timeoutem, retry, kontrolou zdrojové IP/portu a zrušením session při změně boot ID.
- Nikdy neaktivovat session před ověřením READY.
- Přidat testy ztracených, duplicitních, opožděných a pozměněných handshake paketů, restartu obou peerů a výpadku Ethernet/Wi-Fi.

### Fáze C — autentizovaný status a řídicí gate

- Navrhnout pevný wire formát pro kompletní status telemetry s HMAC a session kontextem.
- Ověřovat tag před publikací dat do `PeerStatusMonitor`.
- Zajistit anti-replay a persistenci sequence floor; při chybě NVS fail-closed.
- Až po úspěšném testu celé cesty odstranit dočasnou konstantní blokaci `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED`.

## 6. Bezpečnostní rozhodnutí REV48

- Zachovat REV47 fail-closed blokaci běžného a servisního transferu horní cestou.
- Nouzový ruční režim zůstává samostatný a vyžaduje fyzické ověření obsluhou; tato revize jej nemění.
- Nezměnit UDP topologii, porty, pinout, NVS layout ani eFuse.
- Nezavádět provizorní plaintext klíč, pevný klíč v kódu ani prosté uložení tajemství v `Preferences`.
- Neoznačovat autentizaci za hotovou na základě samotných nativních testů transportně nezávislých tříd.

## 7. Testy provedené při auditu REV48

- Standardní nativní sada: 67/67 testovacích programů PASS.
- ASan/UBSan sada: 65/65 testovacích programů PASS.
- Python klient provisioningu: 17/17 testů PASS.
- Statické integrační kontroly spuštěné v rámci standardní nativní sady: PASS.
- Arduino-ESP32 cílový překlad, NVS Encryption na cílovém čipu, skutečný UDP handshake a fyzické testy: NEPROVEDENO.

Poznámka: nativní testy ověřují přenositelné moduly a simulované/stubované NVS. Neprokazují fyzickou ochranu flash, kvalitu cílového RNG ani chování skutečného routeru/W5500.
