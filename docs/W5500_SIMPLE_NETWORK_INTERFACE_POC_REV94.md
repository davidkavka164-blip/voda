# REV94 – jednoduché řešení společného webu přes Wi‑Fi a W5500

## Závěr kontroly

Ve firmwaru REV93 jsou obě jednotky postavené na `WebServer` z ESP32 Arduino core a současně používají klasickou knihovnu `Ethernet.h` (W5500). Tyto dvě implementace síťového serveru nejsou automaticky propojené. Samotné `Ethernet.begin()` proto web přes W5500 nezpřístupní.

Byla ověřena jednodušší možnost: knihovna **EthernetESP32** (Networking-for-Arduino) používá na ESP32 core 3+ síťovou abstrakci `NetworkInterface`; její `EthernetClient`/`EthernetServer` jsou aliasy `NetworkClient`/`NetworkServer`. Dokumentace uvádí podporu W5500 a kompatibilitu s ESP32 core verze 3 nebo vyšší. To dává možnost ponechat jediný `WebServer` a vyhnout se přepisování 28 rout.

Zdroj: https://github.com/Networking-for-Arduino/EthernetESP32 (README, požadavek core >=3, W5500 driver a NetworkInterface).

## Důležité omezení

Není potvrzeno, jakou verzi ESP32 Arduino core má uživatel nainstalovanou. Pokud je to core 2.x, tato cesta není kompatibilní bez aktualizace core. Aktualizace core může ovlivnit další knihovny a API v projektu, proto se nesmí provést naslepo.

## Co je v REV94

- Přidán izolovaný experiment `experiments/W5500_WebServer_Core3_POC/`.
- POC používá stejné W5500 piny jako aktuální HWC mapování: CS 27, RESET 26, SPI 18/19/23.
- POC ověřuje jediný `WebServer` a základní odpověď HTTP.
- POC není součástí produkčního firmware a nesmí se nahrát do řídicích jednotek jako finální firmware.
- Produkční zdrojové kódy, ochrany čerpadel, ventilů, transfery, autentizace a OTA zůstávají beze změny.

## Co je nutné ověřit na stole

1. V Arduino IDE zjistit verzi balíčku `esp32 by Espressif Systems`.
2. Pouze pokud je core 3.x, nainstalovat `EthernetESP32` přes Library Manager.
3. Nahrát POC do jedné testovací ESP32 s odpojenými výstupy čerpadel a ventilů.
4. Ověřit HTTP stránku přes Wi-Fi i Ethernet současně.
5. Teprve po úspěšném ověření připravit malou změnu v HWC: nahradit Ethernet backend za EthernetESP32 a zachovat jeden WebServer, všechny existující routy a bezpečnostní kontroly.

## Bezpečnostní pravidla

- Síťový výpadek nesmí měnit stav relé čerpadel ani ventilů.
- Žádná síťová inicializace nesmí blokovat bezpečnostní smyčku.
- Neprovádět změny pinů ani logiky ochrany.
- OTA nepřevádět v této etapě.
- Nepřidávat komunikaci mezi ESP32; jednotky zůstávají nezávislé.

## Stav

- Zdrojová kontrola REV93: provedena.
- Externí knihovna / kompatibilita: dokumentačně ověřena.
- POC: připraven, cílový překlad a fyzický test neproveden.
- Produkční firmware: beze změny.
