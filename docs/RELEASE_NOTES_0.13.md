# HOME WATER CONTROLLER — vývojová revize 0.13

## Změny proti 0.12
- Přidán transportně nezávislý modul `PeerProtocol` pro stavové zprávy ESP32 #2 → ESP32 #1.
- Stavová zpráva obsahuje verzi protokolu, ID zařízení, ID startu (boot ID), sekvenci, uptime, stáří měření, hladinu/objem horní nádrže, teplotu, připravenost místní ochrany a alarmové bity.
- Zprávy se kontrolují pomocí CRC32, očekávané verze/ID, rozsahů čísel a příznaků validity.
- `PeerStatusMonitor` odmítá duplicitní/staré sekvence v rámci stejného boot ID, přijme novou sekvenci po restartu peeru a odděluje čerstvost spojení od použitelnosti jednotlivých měření.
- Přidán explicitní 48bajtový little-endian wire formát; přenos nepoužívá surovou strukturu C++ s paddingem.
- ESP32 #1 web nyní zobrazuje aktuální měření studny/dolní nádrže a 30denní přítokovou historii včetně nedatovaných přírůstků.
- Přidány webové formuláře elektrické kalibrace scale/offset pro snímač studny a dolní nádrže; změny se zapisují do NVS, znovu načítají a aplikují za běhu.
- `NvsSensorConfigStore` nyní odmítá automaticky přepsat existující poškozený kalibrační záznam.
- Denní historie se čistí podle skutečného 30denního kalendářního okna; neplatný kalendářní klíč se nepřiřadí falešnému datu.
- Přidán test protokolu pro validní/poškozené zprávy, serializaci, duplicity, staré sekvence, nový boot ID a stáří měření.

## Kontrola
- Nativní testy: 11/11 testovacích programů prošlo.
- `PeerProtocol` je přenositelný logický modul; zatím není napojen na W5500, Wi-Fi ani běžící aplikace.
- Skutečný Arduino-ESP32 překlad a fyzické ověření stále nebyly provedeny.
- Výkonové výstupy zůstávají vypnuté.
