# Release notes — DEV KIT 1.00 REV78

Datum: 16. 11. 2026 | Základ: REV77

## Hlavní změna

`NvsUpperTankLimitsStore` byl převeden z jediného CRC blobu `state` na transakční dvouslotový žurnál.

- Datové sloty `slot0` a `slot1` se střídají; aktivní záznam určuje ověřený `head`.
- `txn` se zapíše a ověří před změnou neaktivního slotu.
- Přerušený zápis kandidáta se po restartu vrátí k předchozí potvrzené konfiguraci.
- Kompletní ověřený kandidát se po restartu dokončí, i když výpadek nastal při zápisu `head`.
- Původní `state` se validuje a migruje bez smazání; přerušená migrace se opakuje z původního záznamu.
- Poškozený aktivní slot nebo `head` zůstává fail-closed. Poškozený `txn` lze ignorovat jen s platným `head` a odpovídajícím aktivním slotem; bez nich je stav fail-closed. Poškozený neaktivní slot bez transakce neblokuje aktivní záznam.

## Bezpečnostní rozsah

Změna se týká pouze horních nastavitelných limitů. Kritická západka horního čerpadla zůstává v odděleném úložišti. Bez změny pinů, zapojení, relé, ventilů, limitů a transferové logiky. API store zůstává stejné.

## Testy

- `./tests/run_native_tests.sh`: 78/78 nativních programů PASS; všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 76/76 nativních programů PASS; statické kontroly runneru PASS.
- Arduino cílový překlad a fyzické testy nebyly provedeny. Hostitelský model nenahrazuje fyzický brownout test ESP32.
