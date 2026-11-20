# HOME WATER CONTROLLER 0.29

## Transfer: validace fyzického rozsahu zdrojové nádrže
- `TransferInputs` nově vyžaduje nakonfigurovanou fyzickou horní mez hladiny zdroje.
- Zdrojová hladina mimo rozsah 0 až `sourceMaximumLevelCm` je odmítnuta jako neplatné měření před startem i během běžícího transferu.
- Neplatná horní mez (nečíselná nebo nevyšší než transferové minimum) blokuje transfer jako nepřipravený systém.
- ESP32 #1 předává maximální hladinu horní nádrže z konfigurace horní nádrže a maximální hladinu studny z geometrie studny.
- Přidány regresní testy pro oba provozní okamžiky a chybnou konfiguraci.

## Ověření
- 17/17 nativních testovacích programů prošlo.
- Celá nativní sada prošla AddressSanitizer a UndefinedBehaviorSanitizer.
- Společné moduly jsou byte-identické v obou Arduino sketch `src` složkách.
- ZIP integrita bude ověřena při vytvoření archivu.

## Omezení
- Hostitelská syntaktická kontrola `.ino` v této revizi nebyla zopakována: dočasné Arduino API stuby nejsou v aktuálním prostředí dostupné.
- Skutečný Arduino-ESP32 build, NVS na fyzickém zařízení, síť, snímače, relé, ventily a čerpadla nebyly ověřeny.
- Fyzické výstupy zůstávají neaktivní. Balíček není provozní firmware.
