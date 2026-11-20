# HOME WATER CONTROLLER 0.53

## Změny
- Přidán `PeerHandshakeInitiator.*`, stavový koordinátor iniciátora handshake.
- `begin()` ověřuje lokální boot ID sekvenčního allocatoru, identity, nonce a rozsah tajemství.
- CHALLENGE se kryptograficky ověřuje před aplikací `authenticatedReplayFloor`.
- Po ověření floor se vytvoří FINISH a uloží se pro opakované odeslání beze změny bajtů.
- Session key nelze získat a handshake nelze označit jako ustavený před ověřením READY.
- Chybný CHALLENGE/READY přepne stav do FAILED a vymaže dočasné tajemství; READY chyba navíc vymaže session key a FINISH.
- Nový test pokrývá manipulaci s floor, posun sekvence 1 → 65, blokování session key před READY, identický FINISH retry, neplatný READY a shodu klíčů po úspěchu.
- Opraven prázdný kořenový README, který v předchozím balíčku 0.52 nebyl vyplněn.

## Ověření
- 39/39 nativních testovacích programů prošlo.
- `test_peer_handshake_initiator` prošel samostatně AddressSanitizer a UndefinedBehaviorSanitizer.
- Společné moduly byly synchronizovány do obou firmware `src/` složek a porovnány.
- ZIP prošel `zip -T`.

## Omezení
- Koordinátor je transportně nezávislý; UDP/W5500 orchestrace stále není implementována.
- Volající musí dodat nonce vytvořený bezpečným generátorem; `esp_random()` není napojen.
- Bezpečný provisioning/chráněné úložiště sdíleného tajemství zůstává blokátorem.
- Arduino-ESP32 build a fyzické testy nebyly provedeny. Výkonové výstupy zůstávají vypnuté.
