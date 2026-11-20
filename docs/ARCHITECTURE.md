# Cílová architektura firmware — HOME WATER CONTROLLER MASTER 2.1 / REV67

Tento dokument nahrazuje starší návrh s `PeerLink`. ESP32 #1 a ESP32 #2 jsou samostatná zařízení bez vzájemné aplikační komunikace. Podrobný audit, migrační plán a hranice bezpečnosti jsou v `ARCHITECTURE_SIMPLIFICATION_AUDIT_REV60.md`.

## ESP32 #1 — studna, dolní nádrž a transfery

- Čte a validuje hladinu studny a dolní nádrže.
- Zobrazuje objem studny podle geometrického odhadu/objemové kalibrace a objem dolní nádrže podle známé geometrie 61,6 l/cm.
- Řídí místní bezpečnostní relé studničního a dolního čerpadla; běžné spínání motorů zůstává vlastní tlakové automatice.
- Řídí všechny transferové ventily. Uživatel zadává zdroj a požadované litry přes web.
- Transfer ze studny používá místní bezpečnostní podmínky studny. Transfer z horní nádrže nečte vzdálenou hladinu; horní čerpadlo chrání samostatně ESP32 #2.
- U transferu horní → dolní sleduje přírůstek objemu dolní nádrže. Po rozběhové době vyžaduje měřitelný přítok a během běhu detekuje jeho přerušení. Při nulovém přítoku, neplatném/stárnoucím čidle, dosažení cíle, STOP, limitu dolní nádrže nebo timeoutu uzavře ventily.
- Vede transferový žurnál a historii. Po restartu nikdy automaticky nepokračuje.
- Má vlastní Wi-Fi STA a webové rozhraní; vzdálený přístup je přes domácí router/VPN.

## ESP32 #2 — horní nádrž

- Čte a validuje vlastní hladinový snímač.
- Místně řídí bezpečnostní relé horního čerpadla podle minimální hladiny a hystereze/obnovovací hladiny. Ochrana není závislá na ESP32 #1 ani na dostupnosti sítě.
- Zobrazuje hladinu a objem v litrech. Objem se určuje samostatnou kalibrační křivkou `hladina cm → objem l` s absolutním kotevním bodem, ručním přidáváním/úpravou absolutních bodů, uložením výchozí reference před transferem, ručním zadáním skutečně odebraných litrů po transferu a interpolací uvnitř kalibrovaného rozsahu. Mimo rozsah se objem neextrapoluje.
- Zobrazuje teplotu vody z DS18B20 na GPIO4; REV69 používá asynchronní 12bitové čtení přes OneWire/DallasTemperature s kontrolou validity a stáří. Teplota je informativní a nemá vliv na ochranu čerpadla ani ventily. Fyzické zapojení a cílový překlad zatím nejsou ověřeny.
- Má vlastní webové rozhraní. W5500 Ethernet se používá jako samostatná síťová linka do routeru/switche a má DHCP obnovu; webová dostupnost přes W5500/LAN zatím není implementována. Kandidátní knihovna WebServer_ESP32_W5500 vyžaduje ESP32 core 2.0.6+; přesná instalovaná verze core zatím není potvrzena. Z důvodu kompatibility s Windows 7 a konfliktů nízkoúrovcového SPI se W5500 web zatím neimplementuje. Servisní AP zůstává pro místní přístup.

## Společné zásady

1. Žádná aplikační komunikace mezi ESP32; žádný peer UDP, handshake, společný klíč ani synchronizace konfigurace.
2. Každá jednotka má vlastní NVS a vlastní web. Změna konfigurace jedné jednotky se nepřenáší do druhé.
3. Žádné dlouhé blokující čekání v hlavní smyčce; měření, web, síť a bezpečnost musí zůstat obsluhované.
4. Každá naměřená hodnota má stav validity a stáří. Neplatné nebo zastaralé měření nesmí být bezpečnostním vstupem.
5. Při startu, restartu, chybě nebo STOP se výstupy nastaví do bezpečného stavu. Nouzové režimy jsou pouze v RAM a po restartu se ruší.
6. Kalibrace, nastavení a historie mají verzované NVS záznamy, kontrolní součty a ověření zápisu zpětným načtením.
7. Vzdálený přístup přednostně přes VPN; ESP32 se nevystavují přímo veřejnému internetu.
8. Piny, elektrické parametry a fyzické zpětné vazby se nesmějí domýšlet. Výstupní povel ventilu není důkaz jeho fyzické polohy.

## Stav transferu

`IDLE → PRECHECK → RUNNING → COMPLETED` s terminálními stavy `STOPPED` a `ABORTED`.

- Uživatel musí zadat nový požadavek.
- Dolní nádrž je primárním měřidlem přeneseného množství.
- U transferu horní → dolní se po rozběhovém okně hlídá kladný přírůstek hladiny a následně pokračující přítok.
- Časový limit je záložní ochrana, ne měření litrů.
- Při odběru z dolní nádrže během transferu může být přenesené množství podhodnoceno; první verze tento souběh nekompenzuje.
- STOP nebo chyba znamená CLOSE_ALL; automatické obnovení po restartu není povoleno.
