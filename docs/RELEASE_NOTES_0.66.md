# HOME WATER CONTROLLER — revize 0.66

Datum: 2026-11-02

## Obnova transferu po restartu

- Přidán `TransferRestartGuard` s trvalými stavy CLEAR, PREPARED, ACTIVE, INTERRUPTED a RESULT_PENDING.
- Přidán `NvsTransferJournalStore` s dvouslotovým NVS žurnálem, generací, CRC32, kontrolním načtením a fallbackem na starší platný slot.
- ESP32 #1 vyžaduje dostupný journal i historii NVS a před přijetím simulace uloží PREPARED; před přechodem do RUNNING musí uložit ACTIVE.
- Selhání zápisu žurnálu vyvolá fail-closed blokaci; automat lze násilně ukončit do stavu ABORTED s příkazem VALVES_CLOSE_ALL.
- Po restartu se PREPARED/ACTIVE mění na INTERRUPTED. Automat se nikdy automaticky neobnovuje.
- Výsledek se nejprve uloží do historie a až potom se žurnál uvolní. Při přerušení je nutné explicitní potvrzení obsluhy.
- Přidána dokumentace `docs/TRANSFER_RESTART_RECOVERY.md`.
- Nové moduly jsou synchronizovány v `firmware/common/` a obou ESP32 `src/` složkách.

## Ověření

- Nativní testy: 45/45.
- Kompletní AddressSanitizer + UndefinedBehaviorSanitizer: 45/45.
- NVS journal test zahrnuje skutečný přenosný guard + NVS adaptér se stubem Preferences, restart, fallback a selhání zápisu.
- Arduino-ESP32 build: neproveden.
- Fyzické NVS, skutečný power-cut test, desky a výstupy: neověřeno; výstupy zůstávají zakázané.

## Bezpečnostní hranice

Tato revize zajišťuje bezpečné obnovení simulačního stavu a připravuje povinnou bránu pro budoucí fyzickou výstupní vrstvu. Není to povolení k aktivaci ventilů ani čerpadel.
