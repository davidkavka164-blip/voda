# HOME WATER CONTROLLER 0.48

## Persistentní anti-replay sekvenční okno
- Přidán `PersistentPeerReplayGuard` a rozhraní `IPeerReplayStore` s výsledky EMPTY/VALID/CORRUPT/ERROR. Poškozený záznam ani chyba backendu se nikdy nepovažují za prázdné úložiště.
- Checkpoint obsahuje magic, verzi, lokální a peer device ID, peer boot ID, rezervované sekvenční maximum a CRC32. CRC slouží pouze k detekci náhodného poškození.
- Sekvence se rezervují po blocích; přijetí první zprávy z dalšího bloku je podmíněno úspěšným atomickým uložením vyššího maxima. Při selhání se příkaz odmítne a čítač se neposune.
- Po restartu přijímače a nezměněném peer boot ID se nepoužitá část posledního bloku přeskočí. Nový peer boot ID smí resetovat sekvenční prostor pouze po čerstvém autentizovaném handshake.
- `PeerCommandSession::activateWithPersistentReplay()` zapojuje persistentní guard do příjmu COMMAND; HMAC/identity/boot validace předchází persistentnímu guardu a RAM cache request ID zůstává jako druhá kontrola.
- Sekvenční epocha je omezena na hodnoty pod 2^31; před dosažením limitu musí vzniknout nová autentizovaná peer boot epocha.

## Testy
- Nativní sada: 35/35.
- Nový `test_persistent_peer_replay_guard`: restart, rezervace po blocích, selhání zápisu, poškození, změna peer boot ID a hranice sekvence.
- Nový `test_peer_command_session_persistent`: integrace přes skutečný HMAC codec, simulovaný restart přijímače a odmítnutí staré sekvence.
- Oba nové testy prošly AddressSanitizer + UndefinedBehaviorSanitizer.

## Omezení
- `IPeerReplayStore` je pouze abstraktní rozhraní. Konkrétní ESP32/NVS backend není implementován ani ověřen; persistentní anti-replay proto zatím není zapnutý v běžícím firmware. Implementace backendu musí splnit atomický zápis při výpadku napájení.
- CRC32 nebrání úmyslnému rollbacku flash. Skutečné zabezpečení proti fyzickému útočníkovi by vyžadovalo platformní ochrany.
- Chybí monotónní sequence allocator na straně odesílatele a autentizované předání `persistentReplayFloor()` po restartu přijímače; bez toho se odesílatel může zaseknout na sekvenci nižší než rezervovaná podlaha. Dále chybí `esp_random()` boot ID/nonce, W5500 orchestrace, retry politika a napojení na transferový automat.
- Skutečný Arduino-ESP32 build ani hardware test nebyly provedeny. Čerpadla a ventily zůstávají neaktivní.
