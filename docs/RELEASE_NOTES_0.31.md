# HOME WATER CONTROLLER 0.31

## Trvalá historie výsledků transferové simulace
- Při kontrole revize 0.30 bylo zjištěno, že transferová obrazovka zobrazovala pouze aktuální/poslední výsledek v RAM. Po restartu se ztratil a nebylo možné zpětně dohledat důvod ukončení simulace.
- Přidán přenositelný modul `TransferHistory.*` s kruhovou historií posledních 12 záznamů. Každý záznam obsahuje jedinečné ID požadavku, zdroj, konečný stav, důvod ukončení, požadovaný objem, odhad změny objemu dolní nádrže, cílovou hladinu a čas dokončení v uptime ESP32.
- Přidáno `NvsTransferHistoryStore.*` v odděleném namespace `hwc_xfer`, s verzí, CRC32, kontrolou záznamu a odmítnutím automatického přepsání existujícího poškozeného záznamu.
- ESP32 #1 načte historii při startu, uloží každý terminální výsledek právě jednou a na dashboardu zobrazí posledních pět záznamů.
- Záznamy jsou výslovně označeny jako SIMULACE. Neprokazují skutečný průtok a nesmějí být zaměňovány s provozní historií fyzických transferů.
- Pokud NVS není dostupné nebo je záznam poškozen, aplikace nesmí poškozený záznam přepsat; výsledek může zůstat pouze v RAM do restartu.

## Ověření
- Nativní testy: 20/20 prošlo, včetně testu kruhového bufferu, validace, duplicitních ID a NVS CRC/ochrany proti přepsání.
- ASan/UBSan: celá nativní sada ověřena samostatným během.
- Společné zdrojové soubory synchronizovány do obou Arduino sketch `src` adresářů.
- Skutečný Arduino-ESP32 build, hostitelská syntax kontrola `.ino`, fyzické NVS, WebServer, W5500 a hardware nebyly v této revizi ověřeny.
- Fyzické výstupy zůstávají zcela neaktivní; transfer zůstává simulací.
