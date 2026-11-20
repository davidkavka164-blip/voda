# HOME WATER CONTROLLER — revize 0.62

Datum: 2026-11-02

## Účel

Revize 0.62 doplňuje technický návrh bezpečného párování a uložení společného tajemství pro ESP32 #1 a ESP32 #2. Firmware ani hardware se nemění.

## Změny
- Přidán `SECURITY_PROVISIONING_DESIGN.md`.
- Popsán model 32B párového tajemství, místní fyzický provisioning, rotace a obnova.
- Stanoven cílový směr: ESP-IDF Flash Encryption Release + NVS Encryption, po ověření přesného čipu, partition table a verze toolchainu.
- Výslovně odděleno šifrování NVS od ochrany proti návratu staršího anti-replay checkpointu.
- Stanoveny blokovací podmínky před W5500 handshake/command integrací.

## Ověření
- Nativní sada: 43/43 prošla.
- Firmware: beze změny.
- Synchronizace `firmware/common/` do obou `src/` složek: beze změny, shoda zachována.
- Skutečný Arduino-ESP32 build a fyzické testy: neprovedeny.
- Flash Encryption, NVS Encryption, Secure Boot a eFuse: nezapínány ani neměněny.

## Omezení
Tato revize není implementací bezpečného úložiště ani párování. Je to návrh a soupis ověřovacích bran. Síťové řídicí příkazy a výkonové výstupy zůstávají zakázané.
