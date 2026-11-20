# HOME WATER CONTROLLER — vývojová revize 0.16

## Změny proti 0.15
- W5500 je nyní zapojen do softwarové vrstvy obou jednotek pro místní UDP stavové zprávy.
- Obě jednotky používají SPI piny z pracovní mapy GPIO revize C, reset W5500 a DHCP s omezeným timeoutem.
- ESP32 #2 každé 2 sekundy vysílá 48bajtovou stavovou zprávu broadcastem do portu UDP 4211. ESP32 #1 přijímá, ověřuje CRC/verzi/ID/sekvenci a stáří zprávy a zobrazuje stav horní nádrže ve webu.
- Při chybě DHCP zůstává místní snímání ESP32 #2 aktivní a Wi-Fi/AP web ESP32 #1 zůstává dostupný; peer data jsou označena jako nedostupná. Obě jednotky opakují inicializaci DHCP přibližně po 60 sekundách, pokud start selže.
- Záměrně se neposílají falešná data: objem a teplota horní nádrže jsou označeny jako nedostupné a místní ochrana čerpadla #2 hlásí nepřipraveno.

## Prozatímní síťový předpoklad
Obě desky W5500 musí být připojeny do stejné místní Ethernetové sítě, která poskytuje DHCP a dovoluje broadcast mezi klienty. Pokud jsou na oddělených VLAN, používají statické IP nebo je aktivní client/broadcast isolation, tato varianta peer komunikace fungovat nebude a transport bude nutné upravit.

## Kontrola
- Nativní testy logiky: 12/12 prošlo.
- Všechny společné C++ moduly a oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům.
- W5500, DHCP, broadcast, skutečný Arduino-ESP32 build a komunikace mezi fyzickými deskami nebyly ověřeny.
- Transferové příkazy a ovládání fyzických výstupů nejsou součástí této revize.
