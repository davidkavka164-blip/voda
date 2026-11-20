# Audit obnovy elektrického nastavení snímačů – REV41

Datum: 13. 11. 2026

## Rozsah

Kontrola servisní obnovy `SensorConfig` pro tři snímače:
- studna (`well`) na ESP32 #1,
- dolní nádrž (`lower`) na ESP32 #1,
- horní nádrž (`upper`) na ESP32 #2.

Tato elektrická konfigurace převádí signál 4–20 mA na výšku hladiny. Není to objemová kalibrace studny `WellCalibration`.

## Zjištěný stav REV40

### ESP32 #1 – studna a dolní nádrž

- Webový formulář obsahuje `/sensor-calibration` a handler vyžaduje administrátorské ověření.
- Handler načte stávající konfiguraci, upraví `calibrationScale` a `calibrationOffsetCm`, validuje rozsahy a uloží ji přes `NvsSensorConfigStore::save()`.
- Zápis se ověřuje opětovným načtením a kontrolou checksumu.
- Pokud záznam chybí, handler může vytvořit nový záznam.
- Pokud záznam existuje, ale je poškozený, handler jeho přepsání výslovně odmítne. To chrání před tichým přepsáním původního nastavení, ale neposkytuje servisní obnovu.
- Formulář nemá samostatné fyzické potvrzení, že obsluha ověřila skutečnou výšku hladiny vůči referenci.

### ESP32 #2 – horní nádrž

- Místní webové rozhraní má formulář pro místní limity a nouzový režim čerpadla.
- V REV40 není nalezen formulář ani POST handler pro změnu/uložení `SensorConfig` horního snímače.
- Chybějící nebo poškozené nastavení horního snímače tedy nelze obnovit běžným místním webovým rozhraním.

### Nedostupné NVS

- Pokud `Preferences::begin()` / otevření namespace selže, webová obnova nemůže bezpečně zapsat konfiguraci.
- NVS nedostupné se nesmí zaměnit za chybějící klíč. Nesmí se povolit uložení přes provizorní RAM hodnoty.

## Bezpečnostní závěr

REV40 správně blokuje běžné použití výšky, pokud `SensorConfig` není uložená a ověřená. Servisní obnova však není úplná a není vhodné ji obcházet automatickým návratem k výchozím parametrům.

Tato revize záměrně nemění firmware ani pravidla ochrany. Nezavádí žádné tlačítko pro bezpodmínečné přepsání poškozeného záznamu.

## Požadavky pro budoucí implementaci obnovy

1. Samostatný servisní postup pro každý snímač, včetně horního snímače na ESP32 #2.
2. Administrátorské ověření a stejnopůvodový POST; ESP32 #2 musí zůstat dostupné pouze přes své místní AP.
3. Výslovné potvrzení obsluhy, že ověřila skutečnou výšku hladiny nezávislou referenční metodou a že zadává hodnoty pro správně přiřazený snímač.
4. Obsluha musí zadat nebo potvrdit `scale` a `offset` podle skutečného měření; pouhé potvrzení checkboxem bez referenčního měření není kalibrace.
5. Speciální cesta pro obnovu smí nahradit pouze chybějící nebo prokazatelně neplatný záznam. Platný záznam nesmí tato cesta tiše přepsat.
6. Při zápisu musí zůstat běžná ochrana blokována až do úspěšného zápisu, read-backu, validace rozsahů a checksumu.
7. Při selhání zápisu/read-backu zůstává stav neplatný a pump permit musí být LOW v běžném režimu.
8. Nedostupné NVS vyžaduje samostatnou diagnostiku/opravu úložiště; webová servisní cesta nesmí předstírat úspěch.
9. Obnova `SensorConfig` nesmí měnit `WellCalibration`, geometrii, ochranné limity, transfer journal ani ostatní NVS klíče.
10. Po obnově musí být zobrazen stav „uloženo a ověřeno“ až po opětovném načtení. Před návratem do automatiky má obsluha zkontrolovat zobrazenou výšku proti fyzické referenci.

## Testy a omezení

V REV41 byl proveden audit zdrojového kódu a hostitelské regresní testy REV40:
- `./tests/run_native_tests.sh`: 65/65 nativních testovacích programů PASS; všechny připojené statické/integration checks PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `python3 -m compileall -q tools tests/python`: PASS.
- `bash -n tests/*.sh`: PASS.
- ZIP integrity REV40: PASS.

Neprovedeno: Arduino-ESP32 cílová kompilace, skutečné NVS testy, fyzická kalibrace snímačů, webové testy na deskách a testy relé/čerpadel. REV41 je auditní dokumentační revize, nikoli nová verze firmware.
