# Implementační audit — oddělení horního transferu od peer komunikace REV61

## Aktuální zjištění z REV59

Kontrola `firmware/ESP32_1/ESP32_1.ino` a `src/TransferLogic.*` potvrdila, že horní transfer je stále přímo svázán s peer vrstvou:

1. `TransferLogic::precheckFault()` odmítá horní zdroj, pokud `peerDataValid` není true.
2. `TransferLogic::runningFault()` znovu kontroluje peer data a `upperTransferValvePermitted`.
3. `ESP32_1.ino` získává horní hladinu z `PeerStatusMonitor` a `peerTransportReady`.
4. `systemReady` pro horní transfer vyžaduje `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED`, platné horní limity a shodu jejich checksumu s peerem.
5. Smyčka `loop()` zavírá horní ventil při ztrátě peer statusu.
6. Web ESP32 #1 zobrazuje horní hladinu/objem/teplotu z peer rámce; po oddělení musí tyto údaje patřit pouze webu ESP32 #2.

## Bezpečná cílová změna

- U horního transferu odstranit zdrojovou hladinu a horní rezervu z rozhodování ESP32 #1. ESP32 #1 zná pouze svůj dolní cíl, stav svého ventilu, požadované litry a lokální připravenost.
- Zachovat kontrolu zdrojové hladiny a hysterezi horního čerpadla uvnitř ESP32 #2. Tato jednotka nezávisle odpojí povolení horního čerpadla.
- Zachovat u ESP32 #1: validní/čerstvou dolní hladinu, kapacitu dolní nádrže, připravenost relé ventilu, STOP, timeout, žurnál, break-before-make a CLOSE_ALL.
- Po odstranění peer závislostí přidat detektor nulového/přerušeného přítoku. Ten bude pracovat pouze s čerstvými vzorky dolní nádrže, s rozběhovou dobou, prahem přírůstku a časem bez pokroku.
- Prahy watchdogu zůstanou konfigurační/konstantní až po změření skutečného chování čidla a průtoku; nesmí se zavést smyšlené hodnoty.

## Pozor na dvě fyzické skutečnosti

- Čerpadlo spíná jeho tlaková automatika; ESP32 #1 pouze otevírá ventil. Pokud horní čerpadlo není v dané hydraulické větvi skutečně aktivní, watchdog přítoku musí uzavřít ventil a ukončit požadavek.
- Přírůstek dolní nádrže je nepřímé měření. Souběžný odběr z dolní nádrže může přírůstek maskovat. Během transferu proto v první verzi nebude dovoleno současně odebírat vodu z dolní nádrže.

## Testy požadované před zapojením

- Horní transfer lze předběžně zahájit bez peer rámce, peer boot ID, peer freshness a zrcadlených horních limitů.
- Well transfer nadále vyžaduje platnou studniční hladinu a bezpečnostní meze.
- Oba transfery stále vyžadují validní a čerstvé měření dolní nádrže.
- Horní lokální ochrana ESP32 #2 vypne své bezpečnostní relé při minimu, vadném/starém čidle nebo neplatné konfiguraci.
- Watchdog: žádný počáteční přírůstek, přírůstek až po rozběhové prodlevě, běžný průběh, přerušení průtoku, šum bez skutečného přítoku, neplatný/starý vzorek, STOP, timeout, dosažení cíle a restart.
- Ve všech terminálních případech: CLOSE_ALL, záznam důvodu, žádné automatické pokračování po restartu.

## Omezení tohoto auditu

Tento dokument je plán implementace, nikoli tvrzení, že byly změny provedeny. REV61 nemění `.ino`, `TransferLogic`, piny ani elektrické zapojení.
