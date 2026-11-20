# HOME WATER CONTROLLER — revize 0.55

Datum: 1. 11. 2026

## Změny
- Přidán `PeerHandshakeRetryPolicy` pro omezené opakování neměnných handshake datagramů.
- První odeslání je okamžité a počítá se jako pokus 1. Další pokusy jsou povoleny až po retry intervalu a nejvýše do nastaveného maxima.
- Po posledním pokusu se další retransmise nepovolí; `exhausted()` se aktivuje po uplynutí intervalu od posledního pokusu.
- Časování používá wrap-safe odečítání `uint32_t`.

## Ověření
- 41/41 nativních testovacích programů prošlo.
- Nový retry test prošel samostatně AddressSanitizer a UndefinedBehaviorSanitizer.
- 86 souborů `firmware/common` je byte-identických s odpovídajícími soubory v obou firmware `src` složkách.

## Omezení
- Retry politika není napojena na UDP/W5500; sama nic neposílá.
- Volající transport musí zachovat identické bajty opakované zprávy, volat `markSent()` pouze po úspěšném předání transportu a vynucovat celkový timeout handshake.
- Secret provisioning, chráněné úložiště tajemství, `esp_random()` boot ID/nonce, aktivace command session, Arduino-ESP32 build a fyzický hardware nebyly ověřeny. Výkonové výstupy zůstávají neaktivní.
