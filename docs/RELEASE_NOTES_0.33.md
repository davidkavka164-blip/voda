# Revize 0.33 — fyzický rozsah ochrany čerpadla studny

Datum: 2026-10-31

## Změny
- Ochrana čerpadla studny nově kontroluje maximální fyzickou hloubku z `WellGeometry.usableDepthCm`.
- Hladina pod nulou, nad fyzickým maximem nebo neplatná maximální mez znamená, že měření není použitelné pro povolení čerpadla.
- Nouzový režim lze aktivovat po výslovném potvrzení fyzické kontroly i při měření mimo rozsah; nesmí obejít chybu konfigurace, nedostupné bezpečnostní úložiště ani uloženou kritickou blokaci.
- Přidány regresní testy.

## Ověření
- `tests/run_native_tests.sh`: 20/20 testovacích programů prošlo.
- Skutečný Arduino-ESP32 build, hostitelská syntax kontrola `.ino` a fyzické testy nebyly provedeny.
- Výstupy čerpadel a ventilů zůstávají neaktivní.
