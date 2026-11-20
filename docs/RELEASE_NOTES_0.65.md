# HOME WATER CONTROLLER — revize 0.65

Datum: 2026-11-02

## Účel

Oprava ochrany proti opakovanému ID transferu po restartu ESP32 #1.

## Zjištění

`TransferLogic` si před revizí 0.65 pamatoval posledních osm ID pouze v RAM. Historie výsledků se ukládala do NVS, ale po restartu se její ID do ochrany automatu nevracela.

## Změny

- Kapacita RAM replay cache zvýšena z 8 na 12 ID, shodně s historií transferů.
- Přidána `restoreRecentRequestIds()`, která obnoví nejnovější ID z historie a zachová i novější ID přijatá v RAM.
- ESP32 #1 obnovuje ochranu po načtení NVS při startu i při pozdějším obnovení úložiště.
- Doplněny regresní testy restartu, kapacity okna a pozdního obnovení NVS.
- Změněné společné moduly synchronizovány do obou sketch `src/` složek.

## Ověření

- Nativní testy: 43/43.
- AddressSanitizer + UndefinedBehaviorSanitizer: 43/43.
- Arduino-ESP32 build a fyzické NVS/hardware: neověřeno.

## Bezpečnostní omezení

Transfer zůstává pouze SIMULACÍ. Žádné ventily ani čerpadla nejsou ovládány. Tato oprava není autentizací síťových příkazů a neodemyká peer command path.
