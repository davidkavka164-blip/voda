# HOME WATER CONTROLLER 0.28

## Persistentní kritická blokace horního čerpadla
- Přidán `NvsUpperTankProtectionStore` v odděleném NVS namespace `hwc_up_safety`.
- Kritická blokace horního čerpadla se načítá po restartu a ukládá při změně. Záznam má verzi a CRC; poškozený záznam se automaticky nepřepisuje.
- Chybějící bezpečnostní záznam se inicializuje až po platném měření, které dosáhne obnovovací hladiny. Do té doby není povolen běžný ani nouzový režim čerpadla.
- Při chybě ukládání kritické blokace se logické povolení zablokuje.

## Ověření
- 17/17 nativních testovacích programů prošlo.
- Nový test pokrývá uložení/obnovení blokace, CRC a odmítnutí přepsání poškozeného záznamu.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti pomocným Arduino API stubům.
- Společné zdrojové soubory jsou synchronizované; ZIP integrity check se provede při zabalení.

## Omezení
- Neproveden skutečný Arduino-ESP32 build ani test NVS na fyzickém ESP32.
- Relé, čerpadla a ventily zůstávají v tomto vývojovém balíčku neaktivní.
