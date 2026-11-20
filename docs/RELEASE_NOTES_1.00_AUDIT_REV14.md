# DEV KIT 1.00 AUDIT — REV14

Datum: 2026-11-10

## Změna

Opraveno chybějící propojení stávajícího `upperValveDecision` s transferovou logikou horní → dolní:

- `TransferInputs` přebírá platné/povolené rozhodnutí horního ventilu.
- `TransferLogic` jej vyžaduje při zahájení i během transferu.
- Nad rezervou domácnosti, ale pod prahem otevření ventilu, je nový transfer blokován.
- V hysterezním pásmu pokračuje transfer pouze tehdy, pokud předchozí stav udržel ventil logicky otevřený.
- Na rezervě domácnosti se povolení ruší a transfer se ukončí stávající ochranou zdrojového minima.
- Přidán test kombinující reálný evaluator `evaluateUpperTankTransferValve()` s `TransferLogic` a statická integrační kontrola.

## Zachované parametry a principy

Žádné hodnoty ani významy limitů nebyly změněny. Kritická hladina ochrany čerpadla, obnovovací hladina, rezerva domácnosti a práh otevření ventilu zůstávají samostatné. `pumpProtectionReady` zůstává diagnostický. Nouzové ruční ovládání ventilů se nemění. Fyzické GPIO výstupy zůstávají vypnuté.

## Ověření

- Nativní C++: 59/59 testovacích programů PASS (včetně nových hraničních případů v `test_transfer_logic`).
- ASan/UBSan: 59/59 PASS.
- Statické integrační kontroly: 10/10 PASS, včetně nové `check_upper_transfer_valve_gate.sh`.
- Python unittest: 17/17 PASS.
- `compileall`: PASS.
- Synchronizace všech společných modulů v obou sketchích: PASS.
- Arduino-ESP32 build a fyzický test nejsou součástí tohoto hostitelského ověření.
