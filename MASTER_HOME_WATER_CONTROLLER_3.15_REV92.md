# HOME WATER CONTROLLER — MASTER 3.15 / REV92

Datum: 19. 11. 2026

## Revize

REV92 navazuje na skutečný zdroj REV91. Přidává sdílenou, transportně nezávislou kontrolu Origin/Host a navazuje na návrh transportního adaptéru pro společnou obsluhu Wi-Fi STA, konfiguračního AP a W5500. Zpřesňuje závislosti stávajících webových callbacků, autentizace, rate limiteru, Origin/Host ochrany a streamovaného OTA.

**Produkční firmware beze změny.** Ethernetové HTTP ovládání není v této revizi implementováno. Důvodem je přímá vazba callbacků na Arduino `WebServer` a nutnost zachovat bezpečnostní i OTA chování bez paralelní, méně otestované implementace.

Podrobnosti: `docs/HTTP_TRANSPORT_ADAPTER_DESIGN_REV91.md`, `docs/HTTP_ORIGIN_POLICY_EXTRACTION_REV92.md`.

## Zachované systémové rozhodnutí

Obě jednotky zůstávají autonomní, bez ESP32↔ESP32 komunikace. Síťové rozhraní nesmí ovlivnit ochrany čerpadel, ventily, watchdog přepouštění ani fail-off výstupy.
