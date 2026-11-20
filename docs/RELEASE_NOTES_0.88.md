# HOME WATER CONTROLLER DEV KIT 0.88

Datum: 2026-11-04

## Oprava hranice servisní autorizace

- Jednorázové autorizační okno nyní končí přesně po 60 000 ms. Dříve bylo možné autorizaci spotřebovat ještě v přesném okamžiku hranice, protože kontrola používala `>` místo `>=`.
- Stejná hranice platí pro `consume()` i `isArmed()`.
- Doplněn regresní test přesného okamžiku expirace.
- Oprava je synchronizována v `firmware/common` a ve zdrojových stromech ESP32 #1 a #2.

## Ověření

- Standardní nativní testy: 48/48 programů PASS.
- ASan/UBSan: 48/48 programů PASS.
- Peer boot identity: PASS.
- W5500 link recovery: PASS.
- Arduino-ESP32 build, fyzický servisní vstup, provisioning a NVS/Flash Encryption nebyly ověřeny.
- Servisní autorizační modul není připojen do `.ino` ani k GPIO. Peer command path zůstává default-deny; fyzické výstupy zůstávají neaktivní.
