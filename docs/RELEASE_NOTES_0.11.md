# HOME WATER CONTROLLER — vývojová revize 0.11

## Změny
- První webová konfigurace na ESP32 #1.
- Uložení a ověření geometrie studny.
- Zadání objemové kalibrační křivky přes textové pole `hladina_cm,objem_l`, bezpečné sloučení a opětovné ověření NVS.
- Uložení SSID/hesla domácí Wi-Fi a konfigurační AP při neúspěšném připojení.
- Basic Auth pro webové stránky; jedinečné správcovské heslo se vytvoří při prvním startu a vypíše do Serial Monitoru.
- Výkonové výstupy zůstávají vypnuté.

## Ověření a omezení
- 8/8 nativních testů prošlo.
- ZIP integrity check prošel při zabalení.
- Nebyl proveden skutečný překlad Arduino-ESP32, test Wi-Fi/AP, NVS na ESP32 ani test hardware.
- Tato verze není hotový řídicí firmware a nemá být použita pro řízení čerpadel/ventilů.
