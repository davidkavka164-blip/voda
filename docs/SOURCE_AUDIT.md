# Audit zdrojových projektů

Audit vstupů:
- `TUV_Controller_3_senzor2.12_newWifi_WIFI_RECONNECT_AP_FINAL.zip` — 34 položek, přibližně 980 kB rozbaleného obsahu, zdrojové moduly pro web, Wi-Fi/AP, W5500, nastavení/NVS, OTA, historii a diagnostiku.
- `WaterLevelController_2.6.1.zip` — 18 položek, přibližně 57 kB rozbaleného obsahu, zdrojové moduly pro snímač 4–20 mA, přepočty, historii, ochranu čerpadla, web, Wi-Fi/AP a W5500.
- `HOME_WATER_CONTROLLER_MASTER_1.3.docx` — zadání se 74 oddíly.

## Kandidáti k opětovnému použití (po úpravě)

### Z TUV Controller
- `wifi.cpp/.h`: struktura opakovaného Wi-Fi připojování, konfigurační AP a ukládání údajů do NVS. Je vhodnější výchozí předloha pro požadavek, aby krátký výpadek Wi-Fi nespouštěl AP.
- `webserver.cpp/.h`: struktura obsluhy webu a více síťových rozhraní; ne kopírovat celé HTML/obslužné routy bez přestavby pro nový systém.
- `settings.cpp/.h`: vzor práce s Preferences/NVS; nově navrhnout oddělené a verzované klíče pro konfiguraci, kalibrace, transfery a denní historii.
- `ethernet.cpp/.h`: pouze pokud bude W5500 potvrzen jako součást finálního hardwaru.
- OTA implementace je inspirace, ne důkaz, že je bezpečně vyřešena aktualizace dvou ESP32.

### Z Water Level Controller
- `water_sensor.cpp/.h`: užitečný vzor pro ADC, proud 4–20 mA a diagnostiku signálu.
- `pump.cpp/.h`: pouze jako reference pro stavovou logiku ochrany. Nelze převzít beze změn, protože nový systém běžně nechává čerpadlo řídit tlakovou automatikou a rozlišuje ochranný výstup od běžného spínání.
- `history.cpp/.h`: vzor pro periodický sběr; výpočet denního přítoku je nutné napsat podle MASTER 1.3.
- `settings.cpp/.h`: vzor NVS, nikoli hotová datová struktura pro nový systém.

## Zjištěné nesoulady / rizika
1. WaterLevelController počítá objem studny jako objem válce podle průměru. To je v rozporu s MASTER 1.3; tento výpočet se nesmí použít pro spolehlivý objem studny.
2. Wi-Fi/AP implementace vzorů se liší. Pro nový projekt je nutné zachovat požadované chování: AP při chybějících údajích nebo neúspěšném startovním připojení; během pozdějšího výpadku opakovat připojování bez okamžitého přepnutí do AP.
3. Stávající WLC `pumpBegin()` vypíná relé a `pumpUpdate()` vykonává běžnou automatiku čerpadla. To není automaticky správné pro nový projekt, kde čerpadla normálně řídí tlaková automatika.
4. Pinouty vzorových projektů nejsou závazné pro nový hardware. Zejména piny W5500 a výstupů nelze převzít bez kontroly konfliktů s konkrétními deskami a zapojením.
5. Ve zdrojových souborech TUV jsou pevně zapsané lokální síťové údaje. Při přenosu kódu se nesmějí přenášet osobní přihlašovací údaje ani pevná konfigurace domácí sítě; nové zařízení má používat konfigurovatelné hodnoty a bezpečné výchozí nastavení.
6. Přiložený binární soubor TUV nepovažovat za ověřený výstup nového zdrojového stavu. Pro nový firmware vždy sestavovat `.bin` z přesně identifikovaných zdrojů.
7. K dispozici není Arduino IDE ani `arduino-cli` v tomto pracovním prostředí; kompilace zatím nebyla provedena.

## Doporučení
Nevytvářet nový projekt prostým sloučením obou ZIPů. Použít je jako zdokumentované reference a navrhnout čistou architekturu se společnými bezpečnostními pravidly. Přenášet pouze malé, jasně ohraničené moduly po kontrole závislostí a po každé etapě ověřovat překlad.
