# Release notes – REV24

## Zaměření
Oprava rozdělení bezpečnostních ochran tří čerpadel podle upřesnění uživatele.

## Změny
- Pracovní pinová mapa: ESP32 #1 GPIO25 pro čerpadlo ve studni, ESP32 #1 GPIO32 pro čerpadlo v dolní nádrži a ESP32 #2 GPIO25 pro čerpadlo v horní nádrži. Uživatel potvrdil rozdělení čerpadel mezi jednotky; konkrétní GPIO zůstávají předběžná.
- Uživatel potvrdil, že 24V/30A relé odpojují napájení motoru a jejich výkonové dimenzování je dostačující.
- V pinových hlavičkách byly obecné názvy nahrazeny názvy odpovídajícími konkrétním čerpadlům.
- Doplněn audit `PUMP_SAFETY_RELAY_INTEGRATION_AUDIT_REV24.md`.

## Otevřené
- Samostatná ochrana čerpadla dolní nádrže ještě není implementována.
- Kritická hladina a obnovovací hystereze dolního čerpadla nejsou určeny; pracovní minimum nádrže 11 cm se nesmí použít jako náhrada.
- Fyzické pump safety výstupy zůstávají vypnuté do dokončení tří ochranných logik a bezpečného boot/reset řešení.
- Arduino build a fyzické testy nebyly provedeny.

## Beze změny
- Žádné limity ani hystereze studniční nebo horní ochrany.
- Žádné nastavení transferů, ventilů, tlakového řízení nebo nouzového režimu.
