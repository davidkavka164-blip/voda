# REV66 — audit odstranění peer závislosti

## Cíl
ESP32 #1 a ESP32 #2 pracují samostatně. ESP32 #1 nečte horní hladinu a nevyžaduje žádné potvrzení ESP32 #2. ESP32 #2 místně chrání horní čerpadlo podle svého snímače, limitu a hystereze.

## Provedené změny
1. `TransferLogic::precheckFault()` pro zdroj UPPER vyžaduje platné místní měření dolní nádrže, ale nevyžaduje `sourceLevelValid`, horní minimum/maximum, peer data ani remote valve permit.
2. `TransferLogic::runningFault()` u zdroje UPPER sleduje místní dolní nádrž, kapacitu, systémovou připravenost a watchdog; nevyžaduje vzdálený upper sample.
3. `TransferInputs` již neobsahuje `peerDataValid` a `upperTransferValvePermitted`. Historické enum hodnoty důvodů komunikace/ventilového povolení se zachovaly, aby nedošlo k přemapování uložených číselných kódů v historii.
4. ESP32 #1 skládá horní transfer input bez horního peer snímače. `systemReady` pro horní zdroj kontroluje místní dolní nádrž, reléový výstup a uložené nastavení watchdogu.
5. Servisní otevření horního ventilu na ESP32 #1 vyžaduje místní validní dolní nádrž. Horní pump safety relay zůstává pod lokální ochranou ESP32 #2.
6. ESP32 #1 peer UDP receiver/monitor byl odstraněn z produkčního sketch.
7. ESP32 #2 peer UDP vysílání, boot ID a sequence byly odstraněny. W5500 Ethernet DHCP/retry zůstává jako samostatná síťová linka.
8. ESP32 #1 již neukládá ani nekontroluje kopii horních bezpečnostních limitů; konfigurace horního čerpadla zůstává lokálně v ESP32 #2.

## Bezpečnostní chování horního transferu
- ESP32 #1 otevře horní cestu jen po ověření vlastní dolní nádrže, kapacity, ventilového výstupu, watchdog nastavení a transferového žurnálu. První přírůstek musí být pozorován nejpozději na hranici startup grace; opožděný přírůstek už požadavek nezachrání.
- ESP32 #2 při nízké hladině nebo poruše místního snímače odebere povolení hornímu čerpadlu. ESP32 #1 se o tom nedozví síťovou zprávou; rozpozná pouze chybějící přírůstek v dolní nádrži a zavře ventil podle watchdogu.
- Pokud ESP32 #2 zcela ztratí napájení, místní pump relay je navrženo fail-off; ESP32 #1 nadále hlídá nulový přítok a zavře ventil. Tato kombinace vyžaduje fyzické ověření.
- Není to důkaz, že mechanický ventil fyzicky zavřel; chybí koncová zpětná vazba.

## Co zůstává
- Historické peer knihovny/testy zůstávají v repozitářovém balíčku, ale nejsou v produkčních sketch include/volání.
- ESP32 #2 má W5500 DHCP, ale lokální `WebServer` je zatím na servisním AP; dosažitelnost přes W5500 LAN/internet není ověřena.
- Objemová kalibrace horní nádrže a DS18B20 ještě nejsou implementovány.
- Časové limity watchdogu jsou nastavitelné přes web a NVS; deadband 12,32 l je jen provizorní.
- Arduino target build a fyzické zkoušky nebyly provedeny.
