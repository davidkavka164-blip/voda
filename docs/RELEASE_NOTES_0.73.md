# Release notes — DEV KIT 0.73

Datum: 2026-11-04
Základ: DEV KIT 0.72

## Bezpečnostní opravy P0

### 1. Persistentní anti-replay journal
- `NvsPeerReplayStore::load()` nyní vrací `PEER_REPLAY_STORE_CORRUPT`, pokud je poškozen kterýkoli z obou slotů, i když druhý slot obsahuje platný starší checkpoint.
- Předchozí chování mohlo po poškození novějšího slotu vrátit starší `reservedThrough`. To představovalo rollback anti-replay ochrany a mohlo umožnit opětovné přijetí zpráv ze již rezervovaného rozsahu.
- Při chybě zápisu, kdy předchozí slot zůstane neporušený a nový záznam nebyl potvrzen, zůstává zachována možnost načíst předchozí checkpoint.
- Při zjištěné korupci se handshake floor nenačte, nový checkpoint se nezapíše a peer handshake musí zůstat nedostupný (fail-closed).

### 2. Bezpečné selhání provisioningu tajemství
- `PeerSecretVault::provision()` nyní při neúspěšném `backend.write()` zavolá `erase()`. Backend může vrátit chybu až poté, co částečně změnil persistentní obsah.
- Testovací backend simuluje částečný zápis 8 bajtů a následné selhání; vault záznam zneplatní.

## Ověření
- `tests/run_native_tests.sh`: 45/45 programů PASS.
- `sh tests/.run_native_tests_asan.sh`: 45/45 programů PASS (AddressSanitizer + UndefinedBehaviorSanitizer).
- `check_peer_boot_identity.sh`: PASS.
- `check_peer_ethernet_recovery.sh`: PASS.
- Společné `NvsPeerReplayStore.cpp` a `PeerSecretVault.cpp` synchronizovány do obou ESP32 `src/` stromů.
- Žádný GPIO, relé, ventil ani čerpadlo nebyl aktivován.

## Omezení
- Jde o nativní hostitelské testy se stubem Preferences, nikoli o důkaz chování skutečného ESP32 NVS při power-cut.
- Arduino-ESP32 build, konkrétní verze Arduino Ethernet Library, skutečné W5500, NVS Encryption, produkční `PeerSecretBackend`, fyzický provisioning a eFuse/Flash Encryption nebyly ověřeny ani změněny.
- CRC32 není ochrana proti úmyslné úpravě nebo obnovení starší kopie celé flash.
- Při korupci jednoho replay slotu se systém nyní záměrně zablokuje a vyžaduje servisní obnovu; automaticky neobnovuje komunikaci.

## Bezpečnostní rozhodnutí
**Peer command path zůstává zakázaná.** Tato revize opravuje dvě chyby v přenositelných bezpečnostních komponentách, ale neuzavírá P0 provisioning ani platformní ochranu tajemství a anti-rollback.
