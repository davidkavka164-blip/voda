# HOME WATER CONTROLLER — vývojová revize 0.20

## Nastavitelné meze horní nádrže
- Přidán `UpperTankLimits`: využitelná hloubka, kritická hladina čerpadla, obnovovací hladina čerpadla, minimální rezerva pro domácnost a hladina zahájení přepouštění.
- Validace vyžaduje: kritická hladina < obnova; kritická hladina < rezerva < hladina zahájení přepouštění ≤ využitelná hloubka; všechny hodnoty jsou konečné a v rozsahu.
- Přidáno samostatné NVS úložiště `hwc_upperlimits` s verzí a CRC. Poškozený existující záznam nelze bez vyřešení automaticky přepsat.
- Web ESP32 #1 nyní nabízí formulář pro nastavení všech pěti limitů. Uložení se ověří zpětným načtením.
- Přidána čistá logika přepouštěcího ventilu s hysterezí: otevření až od nastavené horní hladiny, zavření při poklesu na domácí rezervu; při neplatném/stárnoucím měření nebo neplatné konfiguraci je logické povolení blokováno.
- Logický stav je pouze diagnostický. Žádný fyzický výstup se neovládá.

## Ověření
- Nativní sada: 15/15 testovacích programů prošlo.
- Nový test limitů pokrývá pořadí mezí, hysterezi, dosažení rezervy, neplatný snímač a hladinu mimo využitelnou hloubku.
- Nový test NVS pokrývá uložení/načtení, CRC a zákaz přepsání poškozeného záznamu.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům; nejde o skutečný překlad pro ESP32.
- Společné zdrojové soubory jsou synchronizovány do obou Arduino složek.

## Omezení
- Konfigurace limitů je nyní uložena a vyhodnocována na ESP32 #1. Bezpečné předání konfigurace ESP32 #2 a zapojení těchto limitů do její lokální ochrany čerpadla ještě není implementováno.
- Horní hloubku a limity musí uživatel zadat podle skutečné nádrže; nejsou předvyplněny odhadem.
- Fyzické relé a ventil zůstávají vypnuté. Skutečný Arduino-ESP32 build a test na zařízení nebyly provedeny.
