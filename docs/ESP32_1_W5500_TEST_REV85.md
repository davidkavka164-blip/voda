# ESP32 #1 — W5500 Ethernet test REV85

## Účel a rozsah

Ověřit fyzickou inicializaci W5500, DHCP a obnovu linky na ESP32 #1. Test není ověřením HTTP ovládání přes Ethernet: produkční ovládací web REV85 zůstává dostupný pouze přes Wi-Fi STA nebo konfigurační AP.

## Zapojení

| Signál | ESP32 GPIO |
|---|---:|
| SCK | 18 |
| MISO | 19 |
| MOSI | 23 |
| CS | 27 |
| RESET | 26 |

Před zapojením ověřit konkrétní modul W5500, napájení, společnou zem, resetovací vstup a 3,3V SPI logiku. Pinová mapa je již součástí `HardwarePins.h`; REV85 ji nemění.

## Postup

1. Nahrát sketch ESP32 #1 REV85; nejprve ponechat výkonové obvody čerpadel a ventily v bezpečném servisním stavu.
2. Připojit W5500 do důvěryhodného portu domácího routeru/switche.
3. V sériovém monitoru ověřit zprávu `ESP32 #1 W5500 DHCP ready` a přidělenou IP adresu.
4. Otevřít hlavní web přes Wi-Fi STA nebo konfigurační AP a ověřit, že dashboard ukazuje stav Ethernetu a stejnou IP adresu.
5. Odpojit Ethernet kabel. Dashboard/diagnostika musí ukázat odpojení; lokální měření, pumpové ochrany, ventily, transferový automat a Wi-Fi/AP musí pokračovat beze změny.
6. Kabel znovu připojit. Firmware má obnovit DHCP bez restartu ESP32; ověřit novou nebo obnovenou IP adresu.
7. Restartovat ESP32 s odpojeným kabelem. Jednotka musí dokončit start, zachovat Wi-Fi/AP a lokální bezpečnostní řízení; po připojení kabelu má W5500 získat DHCP.
8. Zopakovat test při nedostupném DHCP serveru (např. odpojený router) a ověřit, že síťová chyba nezablokuje místní funkce.

## Co tento test neověřuje

- Ovládací web přes Ethernet IP, včetně Basic Auth a Origin/Host ochrany.
- POST formuláře, změny NVS, OTA upload ani více souběžných klientů přes W5500.
- Vzdálený přístup z internetu nebo VPN.
- Elektrickou bezpečnost fyzického zapojení čerpadel a ventilů.

**Stav REV85:** statické kontroly prošly. Arduino cílový build a fyzický test W5500 nebyly provedeny.
