# Release notes — HOME WATER CONTROLLER REV65

## Implementováno

- `TransferLogic` nyní používá `LowerTankInflowWatchdog` pro automatický transfer horní → dolní.
- Při přechodu PRECHECK → RUNNING se uloží výchozí objem dolní nádrže a neměnná kopie watchdog konfigurace.
- Při běhu se kontroluje první měřitelný přírůstek a následně pokračující přírůstek. Časové limity se berou z hodnot uložených a ověřených v NVS přes web ESP32 #1.
- Při vypršení rozběhového limitu nebo limitu bez dalšího přírůstku skončí transfer stavem ABORTED s důvodem `TRANSFER_REASON_NO_INFLOW`; výstupní ventilový povel je CLOSE_ALL.
- Neplatné měření způsobí bezpečné přerušení. Celkový timeout transferu zůstává samostatný.
- Přidány testy pro žádný počáteční přítok, potvrzený přítok a následné přerušení, neplatnou konfiguraci a fail-closed chování.
- `TransferLogic` je synchronizován do obou sketchů.

## Prozatímní objemový práh

- `minimumProgressLiters` je v této revizi nastaven na 12,32 l, což odpovídá 0,2 cm při známé ploše dolní nádrže 6,16 m².
- Jde o výchozí technický práh, nikoli o hodnotu ověřenou na fyzickém snímači. Před běžným provozem je nutné změřit šum a skutečné rozlišení dolního snímače. Pokud se ukáže nevhodný, upraví se samostatně; nesmí se zaměňovat s časovými limity.

## Omezení

- Horní transfer v provozním `.ino` stále obsahuje původní peer komunikaci, peer hladinu a peer valve permission. `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`, takže běžný horní automatický transfer je nadále blokován.
- REV65 tedy ověřuje integraci watchdogu v přenosné logice, ale **neuvádí horní transfer do provozu**. Dalším krokem je odpojení horního transferu od peer komunikace podle schválené samostatné architektury.
- Arduino cílový překlad a fyzické zkoušky nebyly provedeny.

## Testy

- Standardní nativní sada: 74/74 testovacích programů PASS; statické integrační kontroly PASS.
- ASan/UBSan: 72/72 testovacích programů PASS; integrační kontroly PASS.
- Python: 17/17 PASS.
