# Release notes — DEV KIT 1.00 REV79

Datum: 17. 11. 2026 | Základ: REV78

## Hlavní změna

Proveden úplný zdrojový audit NVS úložišť produkčních jednotek ESP32 #1 a ESP32 #2. Výsledkem je matice strategií ukládání, kontrol CRC/délky, read-back ověření, obnovy a chování při chybě v `docs/NVS_STORAGE_AUDIT_REV79.md`.

## Závěry

- Bezpečnostní konfigurace a stavové západky zůstávají při chybě NVS blokované; nebyla nalezena cesta, která by běžnou činnost čerpadla povolila automatickým dosazením výchozí konfigurace.
- Horní limity používají od REV78 transakční dvouslotový žurnál.
- Některá další jednoslotová úložiště (zejména elektrická kalibrace snímačů a watchdog přítoku) nemají rollback na předchozí kopii. Jejich neplatný/chybějící stav ale blokuje příslušnou běžnou funkci.
- Objemové kalibrace a statistické historie jsou odděleny od pumpových permitů.
- Přímý zápis Wi-Fi údajů je ověřován přesným zpětným načtením obou řetězců.
- Přidána statická integrační kontrola `check_nvs_storage_audit.sh`.

## Rozhodnutí

REV79 nemění běhovou logiku ani bezpečnostní chování, protože audit neodhalil jednoznačnou chybu vyžadující zásah bez cílového testování. Další krok je ověřit přesnou verzi Arduino-ESP32/ESP-IDF a provést skutečné NVS testy s fyzickým přerušením napájení. Teprve poté rozhodnout o rozšíření transakčních žurnálů na další úložiště.

## Omezení

- Arduino cílový překlad nebyl proveden.
- Fyzické testy NVS/power-cut ani pumpových relé nebyly provedeny.
- Hostitelské testy a statická kontrola nenahrazují test na skutečné desce.
