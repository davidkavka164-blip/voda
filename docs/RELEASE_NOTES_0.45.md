# HOME WATER CONTROLLER — revize 0.45

## Integrovaná command session a admission pipeline

- Přidán `PeerCommandSession.*`, který spojuje `PeerSessionCodec` s `PeerCommandGuard` do jednoho transportně nezávislého vstupního bodu.
- `activate()` odmítne aktivaci bez explicitního příznaku úspěšného čerstvého autentizovaného handshake. Volající musí tento příznak nastavit pouze po ověření FINISH a odvození směrových klíčů.
- `receiveCommand()` nejprve ověří HMAC, device ID a boot ID přes `PeerSessionCodec`; následně přijme pouze `PEER_FRAME_COMMAND` a předá metadata do replay/admission guardu. Dekódovaná data se aplikaci předají pouze při `PEER_COMMAND_ACCEPTED`.
- Odpovědi `PEER_FRAME_RESPONSE` jsou z command admission cesty výslovně odmítnuty. Jejich korelace s odchozími požadavky zůstává samostatnou další etapou.
- `close()` zneplatní codec a vymaže jeho interní směrové klíče. Replay guard zachová posledních 16 request ID v RAM při přerušení a obnovení session v rámci stejného běhu zařízení; při změně lokálního boot ID se RAM stav resetuje. Persistentní anti-replay po restartu zařízení není implementován.
- `PeerSessionCodec` nyní podporuje explicitní `reset()` a `clear()`; při neplatném kontextu se klíče i identita vynulují.
- `PeerCommandGuard` má `endSession()`: ukončí aktivní session, resetuje pořadí sekvencí a ponechá nedávná request ID pro ochranu proti opakovanému aplikačnímu požadavku po reconnectu.

## Ověření

- `tests/run_native_tests.sh`: **32/32 testovacích programů prošlo** standardním během.
- Nový `test_peer_command_session` ověřuje odmítnutí aktivace bez handshake, oba role-kontexty, autentizované přijetí COMMAND, duplicitní request ID, opakování ID s vyšší sekvencí, odmítnutí RESPONSE v command pipeline, uzavření session a zachování request-ID cache po nové session v rámci stejného běhu.
- Nový test `PeerCommandSession` samostatně prošel AddressSanitizer + UndefinedBehaviorSanitizer.
- Kompletní sanitizovaný runner nebyl dokončen v časovém limitu; prošel prvních 24 testovacích programů, poté byl běh ukončen limitem. Neoznačujeme tedy celou sadu za sanitizovaně ověřenou.
- Společné moduly jsou synchronizovány do obou sketch `src` složek a následně byte-porovnány.
- ZIP integrity check prošel.
- Neprovedeno: Arduino-ESP32 build, W5500/UDP integrace, reálný handshake, bezpečný secret backend/provisioning, persistentní anti-replay, NVS na fyzickém ESP32 a hardware test. Čerpadla ani ventily nejsou ovládány.

## Bezpečnostní omezení

Tato revize stále není síťovým řídicím protokolem. `activate()` přijímá výsledek handshake od volající vrstvy; sama handshake neprovádí ani neověřuje. Nesmí být připojena k síti, dokud nebude hotový chráněný provisioning společného tajemství, RNG/boot ID, síťový handshake přes W5500, persistentní ochrana proti replay a korelace odpovědí. ACK znamená přijetí příkazu, nikoli fyzický průtok vody.
