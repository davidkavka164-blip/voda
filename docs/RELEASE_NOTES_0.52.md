# HOME WATER CONTROLLER 0.52

## Změny
- Doplněn čtvrtý handshake krok READY: responder jej vytvoří až po úspěšném ověření FINISH a initiator jej ověří session key.
- Iniciátor tak získá kryptografické potvrzení, že responder FINISH skutečně přijal. Samotné odeslání FINISH již nesmí být považováno za dokončené ustavení session.
- READY používá HMAC-SHA-256 se samostatnou doménou `HWC-HS-READY-V1` a 32B session key.
- Wire codec rozšířen o typ READY (38 B); stávající HELLO/CHALLENGE/FINISH zůstávají beze změny, wire verze zůstává 2.
- Přidán test plné výměny, správného ověření READY, změněného tagu, nesprávného session key, délky a typu rámce.

## Ověření
- 38/38 nativních testovacích programů prošlo.
- Nový test prošel samostatně AddressSanitizer a UndefinedBehaviorSanitizer.
- Společné moduly jsou synchronizovány do obou Arduino `src` složek; kopie byly porovnány.
- ZIP prošel `zip -T`.

## Omezení
- READY zatím není napojeno na W5500 UDP orchestrace ani na automatickou aktivaci `PeerCommandSession`.
- Bezpečný provisioning a konkrétní chráněné úložiště dlouhodobého tajemství, `esp_random()` boot ID/nonce, skutečný Arduino-ESP32 build a hardwarové testy zůstávají otevřené.
- Výkonové výstupy čerpadel a ventilů zůstávají neaktivní.
