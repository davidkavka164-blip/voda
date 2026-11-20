# HOME WATER CONTROLLER 0.43

## Oddělené kryptografické klíče pro oba směry komunikace

- Přidán `PeerSessionKeys.*`, který z ověřeného 32B session key odvozuje dva nezávislé HMAC-SHA-256 klíče: initiator → responder a responder → initiator.
- Odvození používá oddělené doménové štítky a váže výsledek na obě identity zařízení i obě boot ID v kanonickém little-endian kontextu.
- Funkce odmítá chybějící/nesprávně dlouhý session key, nulové nebo stejné identity a nulové boot ID; výstupní klíče při každém odmítnutí vynuluje.
- Přidán `test_peer_session_keys`: ověřuje shodu odvozených klíčů obou stran, rozdílnost směrů, vazbu na identity/boot ID a fail-closed chování.
- Modul je synchronizován do obou sketch `src` složek.

## Ověření

- Nativní testovací programy: 27/27 prošlo běžným během.
- Celá sada: 27/27 prošla AddressSanitizer a UndefinedBehaviorSanitizer.
- Společné soubory v `firmware/common` a obou `src` složkách jsou byte-identické.
- Arduino-ESP32 build, W5500 síťová integrace a fyzický hardware nebyly ověřeny.
- Směrové klíče zatím nejsou napojeny na `PeerCommandFrame` ani UDP/W5500; žádné příkazové ani výkonové výstupy nejsou aktivovány.

## Omezení a další krok

Tato změna odděluje klíče pro oba směry a omezuje riziko reflexe zprávy mezi směry. Neřeší kompromitaci jedné z jednotek ani bezpečné uložení dlouhodobého společného tajemství. Další blokující úkol zůstává konkrétní ochrana/provisioning tajemství a síťová integrace handshake. Prosté NVS bez ověřené encryption se nadále nesmí používat pro tajemství.
