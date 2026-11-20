# HOME WATER CONTROLLER 0.47

## Integrace autentizovaných požadavků a odpovědí
- `PeerCommandSession::prepareOutgoingRequest()` spojuje vytvoření session-bound HMAC rámce s registrací request ID, command code, peer identity/boot ID a timeoutu v `PeerRequestTracker`. Transport smí odeslat rámec pouze při výsledku `PEER_REQUEST_TRACKED`.
- Při neúspěšné registraci se zakódované bajty vynulují a `written` zůstane 0; plná tabulka ani duplicitní ID tedy nevytvoří nechtěně odesílatelný příkaz.
- `PeerCommandSession::receiveResponse()` dekóduje a autentizuje příchozí rámec přes aktivní `PeerSessionCodec`, vyžaduje typ RESPONSE a následně koreluje odpověď. Chybný HMAC, chybný směr nebo chybný typ vrací `PEER_REQUEST_INVALID` a čekající požadavek zůstává otevřen.
- `PeerRequestTracker` si pamatuje posledních 16 request ID. ID po ACK/REJECTED/FAILED nebo timeoutu nelze znovu použít v rámci zachované historie. `cancelPendingPreserveHistory()` zruší čekající sloty při zániku session, ale zachová nedávná ID pro reconnect během stejného běhu zařízení.
- Testy pokrývají plný round-trip COMMAND → autentizovaná ACK, pozměněný HMAC, korelaci a timeout, odmítnutí opětovného použití ID, zrušení čekajících požadavků a zachování historie při reconnectu.

## Ověření
- Nativní runner: 33/33 testovacích programů prošlo.
- Nový `test_peer_command_session` prošel samostatným AddressSanitizer + UndefinedBehaviorSanitizer během.
- `test_peer_request_tracker` prošel samostatným AddressSanitizer + UndefinedBehaviorSanitizer během.
- Společné moduly byly zkopírovány do `firmware/ESP32_1/src` a `firmware/ESP32_2/src`; následně bude ověřena byte-identita.

## Omezení
- Žádná síťová integrace W5500/UDP, žádný handshake orchestrator a žádné propojení s `TransferLogic`.
- Tracker i 16-ID historie jsou volatilní; po restartu se ztratí. Persistentní anti-replay a bezpečné generování request ID zůstávají otevřené.
- Neexistuje retry mechanismus. Timeout musí řešit aplikační vrstva; opakované odeslání nesmí znamenat nové fyzické provedení.
- ACK znamená pouze přijetí/akceptaci příkazu, nikoli dokončení transferu ani potvrzení průtoku vody.
- Skutečný Arduino-ESP32 build, W5500, NVS encryption/provisioning a fyzický hardware nebyly ověřeny. Výkonové výstupy zůstávají neaktivní.
