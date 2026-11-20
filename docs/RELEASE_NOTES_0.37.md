# Revize 0.37 — explicitní peer session a pevná identita

- Zpřísněn `PeerCommandGuard`: `reset()` nyní nastavuje lokální device ID a boot ID.
- Přidána explicitní operace `recordAuthenticatedPeerSession()`, která připne očekávané ID peeru a jeho boot ID pouze po předání výsledku čerstvého autentizovaného handshake.
- Příkaz bez ustavené session, od jiného device ID nebo s jiným peer boot ID je odmítnut. Automatické přepnutí na nový sender boot ID bylo odstraněno.
- Deduplikace posledních 16 request ID zůstává zachována přes změnu peer session v rámci jednoho běhu. Po restartu se RAM cache vymaže; staré rámce pro předchozí lokální boot odmítne kontrola target boot ID.
- Důležité: parametr `freshHandshakeAuthenticated` není autentizace. Skutečný challenge-response handshake, bezpečné uložení/rotace klíčů, persistentní anti-replay, UDP integrace a ACK/REJECTED/FAILED nejsou implementovány. Řídicí příkazy zůstávají vypnuté.
- Nativní testy: 23/23; celá sada prošla ASan/UBSan. Arduino-ESP32 build a fyzický hardware nebyly ověřeny.
