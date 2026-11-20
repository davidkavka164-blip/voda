# HOME WATER CONTROLLER — revize 0.67

Datum: 2026-11-03

## Rozšíření testů NVS transfer journalu

- Rozšířen `tests/test_nvs_transfer_journal_store.cpp`.
- Ověřen přesný namespace `hwc_xfer_jrnl` (13 znaků, limit Preferences je 15).
- Ověřeno, že při poškození novějšího slotu zůstane starší platný záznam dostupný a další zápis nahradí poškozený slot.
- Ověřeno generační pořadí přes přetečení `uint32_t`: generace 0 je novější než `0xFFFFFFFF`.
- Produkční firmware se nemění.

## Ověření

- Nativní testovací programy: 45/45.
- AddressSanitizer + UndefinedBehaviorSanitizer: 45/45.
- `firmware/common/` je bajtově shodný s `firmware/ESP32_1/src/` i `firmware/ESP32_2/src/`.
- Kontrola závorek v `ESP32_1.ino`: prošla.
- Arduino-ESP32 build: neproveden.
- Fyzické NVS a skutečný power-cut test: neprovedeny.

## Bezpečnostní hranice

Transfery zůstávají pouze simulací. Žádný fyzický ventil ani čerpadlo není ovládáno.
