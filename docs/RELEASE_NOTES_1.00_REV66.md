# Release notes — HOME WATER CONTROLLER REV66

Datum: 14. 11. 2026
Základ: DEV KIT REV65

## Hlavní změna

Produkční firmware obou jednotek byl upraven pro samostatný provoz bez vzájemné komunikace.

### ESP32 #1
- Odstraněn peer UDP listener a peer status monitor z provozního sketch.
- Horní transfer již nevyžaduje horní hladinu, peer status, peer boot epoch ani vzdálenou hysterézi.
- Odstraněna duplicitní konfigurace horních bezpečnostních limitů z ESP32 #1; tyto limity zůstávají lokální na ESP32 #2.
- Servisní otevření horní cesty kontroluje místní validitu dolní nádrže a připravenost ventilového výstupu.
- `TransferLogic` již neobsahuje runtime peer vstupy `peerDataValid` a `upperTransferValvePermitted`. Historické číselné enum kódy zůstaly zachovány kvůli záznamům historie.
- Watchdog odmítá i první přírůstek pozorovaný až po překročení startup grace; vzorek přesně na hranici je ještě přípustný.

### ESP32 #2
- Odstraněno peer UDP vysílání a boot identity.
- Zachováno W5500 DHCP připojení a doplněna/pojmenována samostatná obnova Ethernet linky pro síťovou konektivitu.
- Lokální ochrana horního čerpadla a jeho bezpečnostní relé nebyly změněny.

## Testy
- `tests/run_native_tests.sh`: 74/74 nativních C++ testovacích programů PASS; statické integrační kontroly PASS.
- `tests/.run_native_tests_asan.sh`: 72/72 ASan/UBSan testovacích programů PASS; integrační kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- ZIP integrity: PASS.

## Omezení
- Arduino IDE / Arduino-ESP32 cílový překlad nebyl proveden.
- Fyzické testy na ESP32, čidlech, relé, ventilech a čerpadlech nebyly provedeny.
- Deadband 12,32 l zůstává provizorní; musí se doladit podle skutečného šumu a rozlišení dolního čidla.
- W5500 DHCP linka ESP32 #2 neprokazuje, že webové rozhraní je dosažitelné z LAN/internetu. To vyžaduje samostatné ověření/implementaci.
- Staré peer moduly a jejich nativní testy zůstávají v archivu, ale nejsou volány produkčními sketchi. Jejich odstranění je další úklidová práce.
