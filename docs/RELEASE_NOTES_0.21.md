# HOME WATER CONTROLLER — vývojová revize 0.21

## Lokální ochrana horního čerpadla
- Přidán samostatný modul `UpperTankProtection`; ESP32 #2 už nepoužívá obecnou ochranu studny s úmyslně nulovou hloubkou horní nádrže.
- Ochrana pracuje s `UpperTankLimits`: kritickou hladinou, obnovovací hladinou a využitelnou hloubkou.
- Po startu je povolení blokováno, dokud platný snímač nedosáhne obnovovací hladiny. Stejně tak po chybě snímače nebo hladině mimo platný rozsah musí hladina znovu dosáhnout obnovovací meze. Kritická hladina aktivuje blokaci s hysterezí.
- Chybějící/neplatné limity nebo nedostupné NVS znamenají fail-closed stav. Výstupní relé stále není aktivováno.

## Omezení, která zůstávají
- NVS je místní pro každé ESP32. Limity uložené na ESP32 #1 se automaticky neobjeví v ESP32 #2. Záměrně nebyl zaveden neautentizovaný UDP příkaz, který by mohl měnit bezpečnostní konfiguraci. Bezpečné nastavení/synchronizace mezi jednotkami je další etapa.
- Kritická blokace tohoto nového modulu není persistentně ukládána jako samostatný latch; bezpečnost po restartu zajišťuje povinné dosažení obnovovací hladiny.
- GPIO/relé zůstávají vypnuté. Skutečný Arduino-ESP32 překlad, NVS, senzory, W5500 a fyzické chování nebyly ověřeny.

## Ověření
- Nativní testy: 16/16 prošlo.
- Nový test `test_upper_tank_protection` prošel AddressSanitizer/UndefinedBehaviorSanitizer.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům; to není skutečný ESP32 build.
- Společné soubory v obou `src` složkách jsou byte-identické se `firmware/common`.
