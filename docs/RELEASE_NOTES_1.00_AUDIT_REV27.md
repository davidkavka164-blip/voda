# Release notes – REV27

## Výstupní adaptér bezpečnostních relé čerpadel

- Přidán společný `PumpSafetyRelayOutput` s aktivní HIGH/LOW polaritou, výchozím OFF, idempotentním přepínáním a explicitním commissioning gate.
- ESP32 #1 propojuje logickou ochranu studny s GPIO25 a dolní nádrže s GPIO32.
- ESP32 #2 propojuje místní ochranu horní nádrže s GPIO25.
- Všechny tři výstupy jsou při startu nastaveny na LOW před startup delay.
- Povolení je podmíněno `pumpPermit` a čerstvým měřením; stáří nad 5 s vynutí OFF.
- Nouzové režimy čerpadel zůstávají řízeny původními ochrannými třídami; adaptér je nepřepisuje.
- `PUMP_SAFETY_OUTPUTS_ENABLED` zůstává `false` ve všech pinových mapách, dokud není fyzicky ověřen COM–NO fail-safe, 3,3V trigger a externí pull-down.

## Ověření

- `test_pump_safety_relay_output`: commissioning gate OFF/ON, active-HIGH/active-LOW, permit/stop, idempotence.
- Statická integrační kontrola ověřuje tři samostatné výstupy a freshness gate.
- Hostitelské testy nenahrazují Arduino cílový build ani fyzickou zkoušku.
