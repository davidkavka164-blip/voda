# HOME WATER CONTROLLER — DEV KIT 1.00 REV86

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV85

## Změna

REV86 je auditní revize síťové vrstvy W5500 a požadavků na plnohodnotný HTTP přístup přes Ethernet na obou jednotkách.

- Znovu ověřeno, že ESP32 #1 i ESP32 #2 používají `Ethernet.h` pro W5500 a oddělený Arduino `WebServer` pro Wi-Fi/AP.
- Ověřeny kandidátní knihovny `WebServer_ESP32_W5500` a `SparkFun_WebServer_ESP32_W5500`.
- První kandidát vyžaduje ESP32 core 2.0.6+ a používá jinou LwIP integraci W5500.
- Druhý kandidát používá nízkoúrovňový ESP-IDF síťový/SPI/interrupt stack a výslovně varuje před souběžným použitím Arduino SPI/interrupt metod. Jeho příklad přepíná mezi W5500 režimem a standardním Arduino Ethernet režimem, nikoli jejich souběhem.
- Žádná z těchto knihoven nebyla přidána do produkčního firmware.

## Rozhodnutí

Nedělat neověřenou výměnu Ethernet stacku. Ponechat REV85 provozní síťovou implementaci a nejprve potvrdit přesnou verzi Arduino-ESP32 core. Plnohodnotné HTTP přes W5500 zůstává otevřeným úkolem pro ESP32 #1 i ESP32 #2.

## Rozsah a bezpečnost

- Produkční `.ino` firmware, GPIO, SPI pin map, NVS, relé čerpadel, relé ventilů, ochranné prahy, transfery a nouzové režimy se nemění.
- DHCP/link recovery W5500 zůstává nezávislé na lokálním řízení.
- Žádné tvrzení o dostupnosti HTTP ovládání přes Ethernet se nepřidává.

## Testy

- Audit zdrojových souborů a dokumentace: dokončen.
- Produkční firmware: beze změny proti REV85.
- Arduino cílový překlad a fyzické testy: neprovedeny.
