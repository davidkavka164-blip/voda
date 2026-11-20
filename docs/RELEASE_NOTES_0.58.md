# HOME WATER CONTROLLER 0.58

## Změny
- Přidán `TimedPeerHandshakeResponderCoordinator` jako časovaná a omezeně opakovaná responder-side vrstva handshake.
- CHALLENGE se uchovává beze změny a lze jej opakovat po ztrátě datagramu; opakování má pevný interval a maximální počet pokusů.
- Po přijetí FINISH se READY uchovává beze změny. Stejný FINISH může získat totožné READY znovu, upravený FINISH je odmítnut.
- Session key zůstává nedostupný, dokud transport nepotvrdí první odeslání READY. Následné READY retry používají pouze cachovanou zprávu.
- Retry počítadla a časování jsou oddělena pro CHALLENGE a READY; časové výpočty používají wrap-safe politiku.

## Ověření
- `tests/run_native_tests.sh`: 43/43 testovacích programů prošlo.
- Nový test ověřuje identické opakování CHALLENGE/READY, limity pokusů, ztracené READY, změněný FINISH, dostupnost session key a přetečení `millis()`.
- Nový modul je zkopírován shodně do `firmware/common/`, `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`.

## Omezení
- Jde o transportně nezávislý modul; není připojen k W5500/UDP smyčce.
- Volající musí potvrzovat `markChallengeSent()`, `markReadySent()` a `markReadyRetrySent()` až po úspěšném předání datagramu transportu.
- Bezpečný ESP32 backend/provisioning tajemství, autentizované síťové řízení, integrace transferů, Arduino-ESP32 build a fyzické ověření zůstávají nedokončené.
- Výkonové výstupy čerpadel a ventilů zůstávají vypnuté.
