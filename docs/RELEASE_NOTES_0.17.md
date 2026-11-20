# HOME WATER CONTROLLER — vývojová revize 0.17

## Změny proti 0.16
- Opraveno zbytečné zapisování historie přítoku do NVS: předchozí kód označoval historii jako změněnou při každém vzorku a zapisoval ji přibližně každou minutu i bez změny.
- Nový modul `InflowPersistencePolicy` označí historii ke zápisu při kladném přírůstku nebo při změně platného kalendářního dne; stabilní vzorky zápis nespustí.
- Změněné snapshoty se ukládají nejvýše jednou za 5 minut. Při restartu se měřicí baseline nadále záměrně resetuje, aby se neznámý interval nepřičetl k přítoku.
- Přidán nativní regresní test pro změny, stabilní vzorky, změnu data, neplatný čas, throttling a přetečení `millis()`.

## Kontrola
- Nativní testy: 13/13 prošlo.
- Nový modul prošel ASan/UBSan; oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům (nejde o skutečný target build).
- NVS úložiště stále testováno pouze přes hostitelský stub `Preferences`.
- Skutečný Arduino-ESP32 překlad, fyzické NVS, snímače, W5500 a ostatní hardware nebyly ověřeny.
- Výkonové výstupy ventilů a čerpadel zůstávají vypnuté.
