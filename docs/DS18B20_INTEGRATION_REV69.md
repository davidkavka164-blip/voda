# DS18B20 – integrace ESP32 #2 (REV69; audit REV70)

## Zapojení
- Datový vodič DS18B20: GPIO4 ESP32 #2 (`WATER_TEMPERATURE_DATA`).
- Napájení čidla: 3,3 V a GND.
- Nutný pull-up rezistor přibližně 4,7 kΩ mezi DATA a 3,3 V. Nepřipojovat pull-up na 5 V; ESP32 GPIO není 5V tolerantní.
- Zapojení zatím není fyzicky ověřeno. Před připojením ověřit pinout konkrétního pouzdra/sondy a společnou zem.

## Knihovny
- OneWire (Paul Stoffregen).
- DallasTemperature (Miles Burton).
- Nainstalovat přes Arduino IDE 1.8.19 → Sketch → Include Library → Manage Libraries. Pokud Library Manager na Windows 7 nefunguje, použít ZIP balíčky knihoven a Sketch → Include Library → Add .ZIP Library.
- Přesné verze knihoven a kompatibilita se skutečně instalovaným Arduino-ESP32 core musí být ověřeny v uživatelově IDE. Přesná verze core zatím není známa; cílový překlad nebyl proveden.

## Chování firmwaru
- Vyžaduje právě jedno zařízení na 1-Wire sběrnici a rodinný kód 0x28. Nula nebo více než jedno zařízení se vyhodnotí jako nedostupné/nejednoznačné; zařízení se znovu hledá každých 30 s.
- Rozlišení je nastaveno na 12 bitů. Převod je asynchronní; smyčka čeká nejméně 750 ms, takže se neblokuje běh ochrany čerpadla po dobu konverze.
- Nový požadavek na měření se vydává přibližně každých 10 s. Vzorek starší než 15 s se nezobrazuje jako platný.
- Hodnoty mimo fyzický rozsah DS18B20 (-55 až +125 °C), včetně knihovních chybových sentinelů -127/-85 °C, jsou odmítnuty.
- Chybějící čidlo, chyba čtení, hodnota mimo rozsah a zastaralý vzorek se zobrazují jako nedostupná teplota s diagnostickým důvodem.
- Teplota je pouze informativní. Neovlivňuje pump permit, bezpečnostní relé ani ventily. Při poruše teplotního čidla pokračuje nezávislá ochrana hladiny podle vlastního snímače.

## Audit REV70
- Startovní diagnostika již správně označuje GPIO4 jako DS18B20 s informativním měřením; odstraněno redundantní druhé volání `begin()` sběrnice.
- Host test nyní ověřuje wrap-around `millis()` a to, že neúspěšné čtení neposune čas posledního dobrého vzorku.

## Ověření
- Přidán host-test `test_water_temperature` pro hraniční rozsah, NaN, chybové sentinel hodnoty, chybějící čidlo, chybu čtení a stáří vzorku.
- Statická integrační kontrola ověřuje GPIO4, knihovní rozhraní, freshness gate a oddělení od pump permit.
- Host testy nenahrazují Arduino překlad ani fyzické ověření čidla, pull-up rezistoru a vedení.
