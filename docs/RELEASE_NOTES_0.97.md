# HOME WATER CONTROLLER — DEV KIT 0.97

Datum: 5. 11. 2026

## Ochrana HTTP Basic Auth proti opakovaným chybným pokusům

- Přidán společný `WebAuthRateLimiter.h`, synchronizovaný do `firmware/common/`, `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`.
- ESP32 #1 i ESP32 #2 nyní sledují neúspěšné přihlášení samostatně podle IPv4 klienta.
- Politika: 5 neúspěšných pokusů v 60s okně → blokace dané IP na 60 s; úspěšné přihlášení smaže záznam daného klienta.
- Časování používá wrap-safe rozdíly `uint32_t`; pevná tabulka 8 klientů nevyžaduje dynamickou alokaci.
- Aktivní blokace se nikdy nevytlačí kvůli nové IP. Pokud jsou všechny sloty blokované, neznámé IP jsou dočasně odmítány (fail-closed saturation), aby rotace zdrojových adres nevymazala existující blokace.
- Během blokace handler vrací HTTP 429; vlastní Basic Auth zůstává zachována.
- Přidán `test_web_auth_rate_limiter.cpp` a statická kontrola integrace v obou `.ino`.

## Testy

- Standardní nativní testy: 57/57 PASS.
- ASan/UBSan: 57/57 PASS.
- Python provisioning klient: 17/17 PASS.
- Python `py_compile`: PASS.
- Peer boot identity, W5500 recovery, credential module sync, AP startup/migration, RNG a HTTP auth integration: PASS.

## Omezení

- Jde o doplňkovou ochranu proti opakovaným pokusům, nikoli náhradu TLS, silných hesel nebo síťové segmentace.
- HTTP Basic Auth přes nešifrované HTTP stále přenáší znovupoužitelné přihlašovací údaje v zakódované, nikoli šifrované podobě. Provozujte web pouze v důvěryhodné lokální síti; vzdálený přístup má být přes VPN.
- Tabulka uchovává osm klientských IPv4 adres; při zaplnění všemi aktivními blokacemi odmítá nové zdroje po dobu 60 s. Změna IP může vytvořit nový záznam, takže nejde o úplnou ochranu proti distribuovanému hádání hesel.
- Arduino-ESP32 build, fyzický web server a test na hardware nebyly provedeny. Hostitelské testy ověřují logiku limitu, nikoli konkrétní chování síťové knihovny.
- Jednorázový Serial handoff nově generovaných hesel, plaintext NVS a nezapojený fyzický provisioning zůstávají otevřené.
- Peer command path zůstává default-deny; relé, ventily a čerpadla nejsou aktivovány.
