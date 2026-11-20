# HOME WATER CONTROLLER 0.27

## Zpřísnění OTA ESP32 #2
- Upload přijímá pouze soubory s příponou `.bin` (jde o základní kontrolu, nikoli důkaz správného cílového zařízení).
- Zahájení OTA je blokováno, pokud je logicky povolené čerpadlo horní nádrže nebo aktivní nouzový režim.
- Pokud se tento stav změní během uploadu, zápis se přeruší.
- Zachován protokol peer v2 s kontrolou shody limitů horní nádrže.

## Omezení
- Přípona `.bin` nezaručuje, že soubor patří ESP32 #2. Aktualizaci používejte jen s ověřeným buildem a po ověření partition tabulky.
- Skutečný Arduino-ESP32 build, OTA partition, rollback a fyzická aktualizace nejsou ověřeny.
- Výkonové výstupy zůstávají neaktivní.
