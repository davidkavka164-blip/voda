# HOME WATER CONTROLLER — REV64

- MASTER aktualizován na 1.8.
- Přidán `NvsLowerTankInflowWatchdogStore` s verzovaným záznamem, CRC, validací rozsahu a kontrolním načtením.
- Dva časové limity jsou zadávány v sekundách; rozsah 1–3600 s. Výchozí 30 s / 30 s jsou pouze orientační.
- ESP32 #1 načítá uložené hodnoty při startu a zobrazuje stav konfigurace na stránce transferů.
- Přidán autentizovaný formulář `/inflow-watchdog-config`; změna během aktivního transferu je odmítnuta.
- Nastavení se ukládá do NVS a po zápisu se ověřuje načtením.
- Přidány nativní testy store a statická integrační kontrola.

## Ověření

- Nativní C++: 74/74 PASS.
- ASan/UBSan: 72/72 PASS.
- Python: 17/17 PASS.
- ZIP integrity: ověřit při vytvoření archivu.
- Arduino cílový překlad: neproveden.
- Fyzický hardware: netestováno.

## Omezení

Watchdog není zapojen do provozního `TransferLogic`. Horní transfer stále závisí na peer komunikaci REV59. Tato revize implementuje pouze uložení a webovou editaci časových parametrů, nikoli ochrannou logiku v provozu.
