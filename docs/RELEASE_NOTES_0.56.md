# HOME WATER CONTROLLER — revize 0.56

Datum: 1. 11. 2026

## Změny
- Opravena retransmise handshake FINISH na straně responderu.
- Po prvním platném FINISH responder uloží přesný důkaz a READY. Opakovaný identický FINISH vrátí stejnou READY, aniž by znovu spouštěl ověření, měnil replay floor nebo odvozoval nový session key.
- Odlišný FINISH se odmítne. V pending stavu se současně kontroluje timeout i při retransmisi.
- Doplněny testy ztracené READY, identické retransmise FINISH, pozměněného FINISH a retransmise po přechodu do ESTABLISHED.

## Ověření
- Kompletní nativní testovací sada: 41/41 testovacích programů prošlo.
- Koordinátor handshake včetně duplicit FINISH a timeoutu READY_PENDING samostatně prošel AddressSanitizer a UndefinedBehaviorSanitizer.
- Všechny soubory `firmware/common` jsou kopírovány do obou sketch `src` složek; shoda ověřena před zabalením.
- ZIP archiv ověřen příkazem `unzip -t`.

## Omezení
- Tato oprava neznamená, že UDP/W5500 transport je implementován. Síťová orchestraci handshake, retry scheduler, validace zdrojové adresy/portu, rate limiting, provisioning klíče, ESP32 RNG, chráněné úložiště, Arduino-ESP32 build a hardware zůstávají otevřené.
- Po přechodu do ESTABLISHED vrací responder READY pouze pro přesně stejný FINISH. Vyšší vrstva musí určit životnost session a po jejím ukončení zavolat `cancel()`.
- Výkonové výstupy zůstávají neaktivní.
