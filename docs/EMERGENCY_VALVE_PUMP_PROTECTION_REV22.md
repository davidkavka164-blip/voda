# Nouzové ruční ventily a ochrana čerpadel – REV22

## Dohodnuté chování

Nouzový ruční režim je určen pouze pro skutečnou poruchu relevantního čidla nebo komunikace. Ovládá ventily přímo ON/OFF; nepožaduje objem ani časový cíl a obsluha sleduje hladiny a vydává STOP.

## Studna → dolní nádrž

- Při platném a čerstvém měření studny musí být `pumpProtectionState.pumpPermit == true`. Pokud stávající ochrana čerpadla vyhodnotí nízkou hladinu, kritickou blokaci, neplatnou konfiguraci nebo systémovou chybu, nouzové otevření studničního ventilu je odmítnuto.
- Nouzový režim nadále obchází běžné transferové prahy a minimální transferovou hladinu; zachovává se však samostatné povolení ochrany čerpadla proti chodu nasucho.
- Pokud je měření studny neplatné, nouzový ventilový povel může být povolen po explicitním potvrzení fyzické kontroly. Čidlovou ochranu v takovém stavu nelze vyhodnotit a odpovědnost za kontrolu hladiny přebírá obsluha.

## Horní nádrž → dolní nádrž

- Nouzový ventilový povel nevyžaduje peer komunikaci s ESP32 #2.
- Ochrana horního čerpadla se vyhodnocuje lokálně na ESP32 #2 v jeho hlavní smyčce, nezávisle na ručním ovládání ventilu z ESP32 #1.
- Pokud je ESP32 #2 skutečně vypnuté nebo nefunkční, jeho lokální ochrana nemůže pracovat. Ruční nouzové ovládání ventilu tuto skutečnost neumí ověřit.
- `pumpProtectionReady` zůstává diagnostickým údajem; nepoužívá se jako nová blokace nouzového ventilu ani automatického transferu.

## Důležitá hranice aktuální implementace

V této revizi jsou ochrany čerpadel vyhodnocovány softwarově a jejich logické povolení/blokace je dostupné ve stavu systému. Fyzické výstupy `PUMP_1_SAFETY_RELAY`, `PUMP_2_SAFETY_RELAY` a ESP32 #2 pump safety relay nejsou inicializovány ani spínány firmwarem. REV22 proto sama o sobě ještě nedokládá fyzické odpojení čerpadla při chodu nasucho. Zapojení a aktivace bezpečnostních relé je samostatná hardwarová etapa a musí být ověřena na konkrétním zapojení, včetně bezpečného stavu při restartu a ztrátě napájení ESP32.

## Zachované prvky

- Dolní nádrž, horní peer komunikace a běžné transferové limity nejsou v nouzovém režimu požadavkem pro otevření.
- Pouze jedno ventilové relé může být aktivní; změna směru vyžaduje STOP.
- STOP a deaktivace režimu dávají oběma ventilovým relé povel OFF.
- Čerpadla nejsou spouštěna povelem ventilového ovládání; jejich běžné tlakové automatiky se nemění.
