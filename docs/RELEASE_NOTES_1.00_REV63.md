# HOME WATER CONTROLLER — REV63

- MASTER aktualizován na 1.7.
- Potvrzeno uživatelské nastavení rozběhového limitu a limitu bez pokroku přes web ESP32 #1.
- Stanoveno ukládání do NVS, kontrola rozsahů, ověření zápisu a fail-safe blokování horního transferu při neplatné konfiguraci.
- Změny konfigurace během transferu se použijí až při dalším transferu.
- Rozlišen celkový timeout transferu a parametry watchdogu; objemový práh proti šumu zůstává samostatný.
- Jde o aktualizaci specifikace; provozní firmware REV62 nebyl změněn.
