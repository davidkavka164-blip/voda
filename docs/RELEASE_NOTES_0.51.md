# HOME WATER CONTROLLER 0.51

## Změny
- Persistentní replay floor přijímače je součástí HMAC autentizovaného handshake CHALLENGE.
- `PeerHandshakeResponder::beginWithReplayStore()` načte uloženou hranici z NVS backendu podle identity peeru a jeho boot ID; poškozené úložiště selže bezpečně.
- Iniciátor po úspěšném ověření CHALLENGE získá autentizovaný floor a může jej předat `PeerCommandSession::synchronizeOutgoingSequenceFloor()`. Test ověřuje hranici 128 a první sekvenci 129.
- Handshake wire protokol je nyní verze 2; CHALLENGE má 84 B místo 80 B. Verze 1 je záměrně odmítnuta.

## Ověření
- 37/37 nativních testovacích programů prošlo.
- NVS/handshake a wire testy prošly samostatně AddressSanitizer + UndefinedBehaviorSanitizer.
- Společné soubory jsou synchronizovány do obou Arduino `src` složek.

## Omezení
- Není hotová W5500 UDP orchestrace, automatické propojení handshake s aktivací `PeerCommandSession`, provisioning bezpečného tajemství ani generování boot ID/nonce přes `esp_random()`.
- Arduino-ESP32 překlad a fyzické NVS/power-cut testy nebyly provedeny.
- Čerpadla a ventily zůstávají softwarově neaktivní.
