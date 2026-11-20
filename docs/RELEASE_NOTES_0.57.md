# HOME WATER CONTROLLER 0.57

## Změny
- Přidán `TimedPeerHandshakeInitiator` pro časované navazování spojení z iniciátorské strany.
- Celkový timeout pokrývá čekání na CHALLENGE i READY.
- HELLO a FINISH mají omezené opakování; opakované zprávy jsou obsahově identické s původním datagramem.
- Po ověření CHALLENGE se retry politika znovu spustí pro FINISH.
- Neplatný CHALLENGE/READY vede do FAILED a vymaže dočasné klíče; session key je dostupný až po úspěšném READY.

## Ověření
- `tests/run_native_tests.sh`: 42/42 testovacích programů prošlo.
- `test_timed_peer_handshake_initiator`: samostatně prošel AddressSanitizer a UndefinedBehaviorSanitizer.
- Nové soubory jsou shodné v `firmware/common/`, `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`.

## Omezení
- Modul není připojen k UDP/W5500 síťové smyčce. Volající musí volat `markHelloSent()` / `markFinishSent()` až po úspěšném odeslání datagramu.
- Neexistuje dokončený chráněný ESP32 backend pro secret ani fyzický provisioning.
- Boot ID a nonce nejsou napojeny na `esp_random()`.
- Arduino-ESP32 build, síť a fyzický hardware nejsou ověřeny. Výkonové výstupy zůstávají vypnuté.
