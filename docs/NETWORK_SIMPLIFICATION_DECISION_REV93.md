# REV93 — zjednodušení přístupu k webu přes W5500

## Proč měníme přístup

Předchozí návrh směřoval k vlastnímu HTTP parseru a přepisu stávajících handlerů. To je pro tento projekt zbytečně rozsáhlé a zvyšuje riziko rozdílného chování ovládání vody.

Kontrola skutečného REV92 ukazuje, že firmware již používá knihovnu `Ethernet.h` pro W5500, ale webové routy obsluhuje Arduino `WebServer`. Tyto dvě vrstvy nejsou automaticky propojené.

## Preferované řešení

Prověřit nativní W5500/lwIP podporu konkrétní verze ESP32 Arduino core. Pokud lze W5500 provozovat jako síťové rozhraní lwIP, ponechat jedinou instanci `WebServer` a všechny stávající routy. Tím se vyhneme druhému HTTP serveru, kopírování rout, duplicitní autentizaci i vlastnímu OTA parseru.

## Záložní varianta

Pokud to používaný core nepodporuje, nezavádět narychlo druhý HTTP server. Zachovat funkční Wi-Fi webové ovládání a Ethernet používat pouze pro síťové funkce, které již firmware podporuje. Ethernetový přístup k webu odložit do doby, než bude jasná kompatibilní knihovna/core.

## Bezpečnostní hranice

- Neměnit autentizaci, CSRF/Origin kontrolu ani OTA.
- Neměnit pump safety relays, valve relays, transfer watchdog ani ochrany senzorů.
- Výpadek Ethernetu/Wi-Fi nesmí blokovat hlavní smyčku ani měnit stav výstupů.
- Neprovádět automatickou aktualizaci ESP32 core bez ověření kompatibility s Arduino IDE 1.8.19 a Windows 7.

## Stav

REV93 je dokumentační revize. Firmware se neměnil. Arduino build a fyzický test W5500 nebyly provedeny.
