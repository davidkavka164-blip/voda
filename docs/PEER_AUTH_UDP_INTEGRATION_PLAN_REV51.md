# Peer autentizace – skutečný stav a plán UDP integrace (REV51)

## Výsledek přímého auditu REV50

Tento dokument opravuje možnou nejednoznačnost předchozích auditních poznámek. Přítomnost přenositelných kryptografických modulů v projektu **neznamená**, že je autentizace zapojena do síťové cesty firmware.

Přímá kontrola obou `.ino` potvrzuje:

- ESP32 #2 stále vytváří původní `PeerStatusFrame`, kóduje jej přes `encodePeerStatusFrame()` a vysílá CRC rámec jako broadcast na `255.255.255.255:4211`.
- ESP32 #2 má UDP socket otevřený na lokálním portu `4212`, ale jeho síťová smyčka zatím nevolá `parsePacket()` a nepřijímá HELLO/FINISH.
- ESP32 #1 naslouchá na lokálním UDP portu `4211`, vyžaduje přesně `PEER_STATUS_WIRE_SIZE` a volá `decodePeerStatusFrame()` + `PeerStatusMonitor::accept()`.
- ESP32 #1 zatím neodesílá HELLO a nepřijímá CHALLENGE/READY.
- `PeerDatagramEnvelope`, `PeerHandshakeWire`, timed handshake koordinátory a `PeerStatusSessionCodec` jsou přítomné v přenositelných modulech, ale nejsou volány z produkčních UDP smyček.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`; běžný automatický a servisní horní transfer proto zůstává blokován.

## Zvolená transportní role – bez změny topologie

- **Iniciátor:** ESP32 #1 (Wi-Fi STA), lokální UDP port `4211`.
- **Responder:** ESP32 #2 (W5500 Ethernet), lokální UDP port `4212`.
- **První HELLO:** ESP32 #1 odešle broadcast na `255.255.255.255:4212`, aby nemusela znát DHCP adresu ESP32 #2.
- **CHALLENGE / READY:** ESP32 #2 odpoví unicast na IP/port iniciátora získané z přijatého UDP datagramu (`remoteIP`, `remotePort`).
- **FINISH:** ESP32 #1 odešle unicast na zdrojovou IP ESP32 #2 a port `4212`, zjištěnou z přijatého CHALLENGE.
- **STATUS po navázání session:** doporučený cílový režim je unicast z ESP32 #2 na ověřenou adresu ESP32 #1, port `4211`; každý STATUS musí projít HMAC codec a anti-replay kontrolou. Broadcast STATUS se nesmí považovat za autentizovaný jen na základě CRC.

Tato role zachovává současné rozdělení Wi-Fi/Ethernet, router, W5500 i porty. Pokud síť nepřenáší broadcast mezi Wi-Fi a Ethernet segmentem, handshake selže bezpečně a horní transfer zůstane blokován. Pozdější přechod na známou/unicast adresu může být samostatná změna, nikoli důvod nyní měnit topologii.

## Nutné kroky implementace

1. Na ESP32 #1 vytvořit initiator lifecycle, HELLO retry a odesílání datagramů s `PeerDatagramEnvelope`.
2. Na ESP32 #2 začít obsluhovat příchozí UDP na portu `4212`, validovat envelope a HELLO, omezit počet souběžných handshake relací a použít responder coordinator.
3. Odpovědi CHALLENGE/READY cílit na zdrojový endpoint iniciátora; FINISH ověřovat pouze proti aktuální nevypršené výzvě.
4. Session key předat do `PeerStatusSessionCodec`; STATUS přijmout pouze s platným HMAC, správným směrem, peer ID, boot epoch a sekvencí.
5. Napojit persistentní anti-replay rezervaci a potvrzení na skutečné přijetí zpráv. Při restartu, změně boot epoch, timeoutu, chybě úložiště nebo ztrátě session okamžitě zrušit důvěryhodnost peer dat.
6. Teprve po end-to-end testech změnit horní transfer gate. Do té doby zůstává `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED = false`.

## Blokátor: společné tajemství

`PeerSecretVault` je pouze politika nad abstraktním `PeerSecretBackend`. V REV50 nebyla nalezena žádná produkční třída, která by `PeerSecretBackend` implementovala. `confidentialStorageAvailable()` se v produkčním firmware nikde neimplementuje. Plain Preferences/NVS není podle kontraktu vaultu přijatelné úložiště.

Před aktivací handshake je proto nutné vybrat a ověřit konkrétní ochranu tajemství v cílovém prostředí. Nelze pouze uložit 32 bajtů do běžného NVS a označit je za bezpečně chráněné. Zvlášť je třeba ověřit přesnou verzi Arduino-ESP32/ESP-IDF, NVS/Flash Encryption a postup fyzického provisioningu na obou deskách. Změny eFuse nebo zapnutí Flash Encryption jsou nevratné/provozní kroky a nesmí se provádět bez samostatného zálohovacího a obnovovacího postupu.

## Co REV51 mění

REV51 je auditní a plánovací revize. Nemění produkční UDP firmware, klíče, porty ani chování ventilů. Přidává statickou regresní kontrolu, která hlídá, že se stav neautentizované transportní cesty nezamění za hotovou autentizaci. Není to dokončení peer autentizace.
