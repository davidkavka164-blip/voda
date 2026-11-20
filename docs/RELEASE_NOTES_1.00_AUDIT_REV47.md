# HOME WATER CONTROLLER – auditní revize REV47

Datum: 2026-11-13

## Bezpečnostní brána peer komunikace

- Audit odhalil, že CRC-only UDP status ESP32 #2 byl používán jako podmínka fyzického otevření ventilu horní → dolní nádrž. CRC neautentizuje odesílatele.
- Běžný automatický transfer horní → dolní je nyní blokován, dokud není autentizovaná peer session skutečně integrována do UDP transportu.
- Servisní ruční otevření horní cesty je rovněž blokováno.
- Nouzové ruční otevření zůstává samostatné a vyžaduje explicitní fyzické potvrzení obsluhy; nejde o běžný síťový řídicí kanál.
- CRC-only peer zprávy zůstávají dostupné pouze jako diagnostická telemetrie. Dashboard tuto hranici výslovně zobrazuje.
- Opraveny zavádějící komentáře/texty o tom, zda bezpečnostní relé čerpadel jsou řízena.

## Testy

- 67/67 standardních nativních testovacích programů PASS; všechny připojené integrační kontroly PASS.
- 65/65 testovacích programů v rozsahu samostatného ASan/UBSan runneru PASS.
- Python 17/17 PASS; compileall PASS; shell syntax PASS; synchronizace společných a credential modulů PASS.

## Omezení

- Autentizovaný peer handshake/session, bezpečný provisioning tajemství a důvěrné ESP32 úložiště stále nejsou integrovány. Horní automatický transfer proto zůstává záměrně blokován.
- Arduino-ESP32 cílový build a fyzické testy nebyly provedeny.
- W5500 broadcast přes skutečný router/AP, VPN a vzdálený přístup nejsou ověřeny.
- Horní objemová kalibrace a měření teploty zůstávají nedokončené.
