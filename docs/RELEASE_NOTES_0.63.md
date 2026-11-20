# HOME WATER CONTROLLER — revize 0.63

Datum: 2026-11-02

## Účel

Revize 0.63 posiluje fail-closed provisioning kontrakt `PeerSecretVault`. Firmware aplikace, síťová vrstva a výkonové výstupy se nemění.

## Změny
- Po úspěšném backendovém zápisu se 32B klíč načte zpět.
- Porovnání zpětně načtené hodnoty probíhá konstantním časem.
- Dočasný buffer se po ověření vynuluje.
- Při neúspěšném nebo odlišném zpětném čtení se vault pokusí neověřený záznam zneplatnit a vrátí chybu.
- Backendový kontrakt výslovně stanoví požadavek trvalého zápisu a trvalého zneplatnění při úspěšném erase.
- Testy pokrývají selhání zápisu, poškozený read-back a selhání čtení zpět.

## Ověření
- Nativní sada: 43/43 prošla.
- Změněný test `PeerSecretVault`: samostatně prošel AddressSanitizer + UndefinedBehaviorSanitizer.
- Kompletní sanitizovaná sada nebyla v tomto kroku dokončena; výsledek se neuvádí jako úplné sanitizované ověření.
- Kopie `PeerSecretVault.h/.cpp` jsou synchronizovány ve společné složce i obou ESP32 `src/` stromech.
- ZIP integrita bude ověřena při zabalení.

## Omezení

Produkční ESP32 backend, NVS Encryption, fyzický provisioning, Arduino-ESP32 build a hardware nejsou implementovány/ověřeny. Flash Encryption, Secure Boot a eFuse nebyly zapínány ani měněny. Síťová řídicí cesta a fyzické výstupy zůstávají zakázané.
