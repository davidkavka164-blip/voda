# Release notes — vývojová REV16

## Nouzové ruční ovládání ventilů — logická vrstva

- Přidán společný modul `ManualValveControl` a hostitelský test stavového automatu.
- ESP32 #1 dostalo samostatnou webovou sekci a endpointy `/manual-valve-mode` a `/manual-valve-command`.
- Ruční režim je oddělený od `/pump-emergency`; vyžaduje výslovné potvrzení fyzické kontroly hladin a relevantní poruchu/nedostupnost čidla.
- Aktivace je blokována při probíhajícím transferu a při nevyřešeném/poškozeném transferovém žurnálu.
- V ručním režimu je automatický transfer blokován. Povel CLOSE_ALL/STOP je kdykoli dostupný; změna směru vyžaduje nejprve CLOSE_ALL.
- Ruční režim je pouze v RAM. Po restartu je vždy neaktivní a logický povel je CLOSE_ALL. Události ručního režimu zatím nejsou ukládány do trvalé historie; to zůstává nedokončeným požadavkem Masteru §57.
- Dostupné ochrany zůstávají aktivní: dolní nádrž musí mít platné čerstvé měření a rezervní kapacitu; studna zachovává nakonfigurovanou ochranu, kritickou blokaci a transferové minimum, pokud je čidlo platné; horní směr vyžaduje čerstvá peer data a stávající `upperValveDecision` včetně hystereze.
- Pokud je čidlo studny neplatné, jeho chybějící údaj může v tomto nouzovém režimu nahradit pouze výslovné potvrzení fyzické kontroly; kritická blokace a chyba systému nadále blokují povel.
- Při neplatném měření dolní nádrže jsou oba směry blokovány. Při ztrátě peer dat je horní směr blokován podle Master 1.3 §56. Zda má fyzická kontrola uživatele v těchto případech nahradit automatické měření, zůstává otevřenou otázkou.
- **Fyzické výstupy zůstávají vypnuté.** Modul vydává pouze logický povel. Není připojen k `digitalWrite`, relé ani ventilům; tato revize není připravena k fyzickému ovládání.
- Nepřidělují se nové GPIO a nemění se provizorní pinout. Všechny hladinové hodnoty, rezervy, kritické limity a hystereze zůstávají beze změny.

## Ověření

- Nativní C++ testy: 60 testovacích programů.
- ASan/UBSan: 60 testovacích programů.
- Statické integrační kontroly: 12/12 PASS (včetně kontroly ručního režimu a peer boot epoch).
- Synchronizace všech modulů `firmware/common` do obou sketchů: PASS.
- Python unittest: 17/17 PASS; `compileall`: PASS.
- Arduino-ESP32 překlad, cílové NVS, fyzické ESP32/W5500, relé a ventily nejsou touto revizí ověřeny.

## Neměnné části

- Automatická transferová logika, limity, `transferValveOpenCm`, `householdReserveCm`, kritické limity čerpadel a hystereze nebyly změněny.
- `pumpProtectionReady` zůstává diagnostický údaj.
- ESP32 #2 zůstává lokální jednotkou horní nádrže; ruční ventilové příkazy zpracovává pouze hlavní ESP32 #1.
