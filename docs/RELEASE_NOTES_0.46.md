# HOME WATER CONTROLLER — revize 0.46

## Korelace odchozích příkazů a odpovědí

- Přidán `PeerRequestTracker.*` s pevnou kapacitou 8 souběžných požadavků bez dynamické alokace.
- `track()` přijímá pouze platný COMMAND rámec, nenulový timeout v rozsahu 1..INT32_MAX ms a odmítá duplicitní request ID, dokud je požadavek evidován.
- `acceptResponse()` očekává rámec, který již autentizovala aktivní `PeerSessionCodec`; ověří přesnou korelaci request ID, původní command code, obě device ID a obě boot ID. Nesouhlasná odpověď neodebere čekající požadavek.
- Odpověď po timeoutu vrací `PEER_REQUEST_TIMED_OUT` a požadavek odstraní. `takeExpired()` umožňuje aplikační vrstvě postupně vyzvednout vypršené request ID.
- Timeout používá wrap-safe odečítání `uint32_t` pro `millis()`; intervaly delší než INT32_MAX ms jsou odmítnuty.
- Tracker pouze sleduje korelaci a timeout odpovědi. Neodesílá opakování, neprovádí příkazy a neprokazuje fyzický průtok vody. Po restartu se jeho tabulka ztratí; nejde o persistentní anti-replay.

## Ověření

- `tests/run_native_tests.sh`: **33/33 testovacích programů prošlo** standardním během.
- Nový `test_peer_request_tracker` samostatně prošel AddressSanitizer + UndefinedBehaviorSanitizer.
- Společné moduly synchronizovány do obou sketch `src` složek a byte-porovnány.
- Neprovedeno: kompletní ASan/UBSan běh všech testů, Arduino-ESP32 build, W5500/UDP integrace, skutečný handshake/provisioning a fyzické testy.
- Čerpadla a ventily zůstávají softwarově neaktivní.

## Bezpečnostní omezení

Tracker musí dostat pouze již autentizovanou a session-bound RESPONSE zprávu. Není připojen do `PeerCommandSession`, W5500 ani `TransferLogic`. ACK znamená pouze potvrzení přijetí příkazu, nikoli dokončení transferu nebo průtok. Před síťovým řízením nadále chybí produkční chráněné úložiště tajemství, provisioning, handshake orchestrace, persistentní anti-replay, autentizovaná síťová integrace a bezpečné zacházení s výpadkem komunikace.
