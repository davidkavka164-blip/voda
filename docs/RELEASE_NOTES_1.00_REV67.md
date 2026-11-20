# Release notes — HOME WATER CONTROLLER REV67

Datum: 14. 11. 2026
Základ: DEV KIT REV66

## Horní nádrž — objemová kalibrace
- Přidán samostatný datový model `UpperTankVolumeCalibration` pro tabulku výška [cm] → absolutní objem [l].
- Model kontroluje verzi, rozsah, monotónní pořadí bodů a CRC. Podporuje přesný odečet, lineární interpolaci a výslovný stav mimo kalibrovaný rozsah; mimo rozsah neextrapoluje.
- První bod musí být potvrzen jako absolutní objem. Další body lze ručně přidávat nebo upravit při stejné výšce, pouze pokud zůstane křivka monotónní.
- Přidáno NVS úložiště `hwc_up_vol`, kontrola CRC a ověření zpětným načtením. Poškozený záznam se nesmí automaticky přepsat.
- ESP32 #2 web zobrazuje tabulku a objem; nabízí zadání absolutního bodu, ruční přidání/úpravu bodu, uložení reference před transferem a vytvoření nového bodu po transferu podle skutečných litrů z ESP32 #1.
- Výchozí reference se po úspěšném uložení nového bodu spotřebuje. Pokud se uložení nepodaří, zůstává původní reference zachována. Nepoužitou nebo neplatnou referenci lze výslovně zrušit. Každý bod lze ručně přidat nebo upravit zadáním výšky a absolutního objemu.
- Kalibrace je informační a neovlivňuje ochranu čerpadla ani řízení ventilů.

## Síť ESP32 #2
- W5500 webový přístup nebyl implementován. Prozkoumaná knihovna `WebServer_ESP32_W5500` vyžaduje ESP32 core 2.0.6+; instalovaná verze uživatele není potvrzena. Změna síťového stacku se proto odkládá do ověření konkrétního core.

## Testy a omezení
- `tests/run_native_tests.sh`: 76/76 nativních testovacích programů PASS; statické kontroly včetně nové kontroly horní objemové kalibrace PASS.
- Arduino cílový překlad nebyl proveden.
- Fyzické testy ESP32, čidel, relé, ventilů a kalibračního postupu nebyly provedeny.
- První absolutní bod a každý ruční bod vyžadují skutečně ověřený objem. Před transferem je nutné uložit výchozí referenci; mezi referencí a výsledným měřením nesmí být jiný přítok ani odběr.
