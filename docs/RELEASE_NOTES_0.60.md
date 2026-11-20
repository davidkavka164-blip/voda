# HOME WATER CONTROLLER 0.60 — dokumentační revize

## Změny
- Přidán `REMOTE_ACCESS_PLAN.md` s návrhem vzdáleného přístupu přes WireGuard VPN na domácím routeru.
- Zaznamenán router TP-Link Archer AX12 podle informace uživatele.
- Ověřena oficiální podpora WireGuard pro konkrétní větev Archer AX12(EU) V1; konkrétní HW revize a firmware uživatelova zařízení zůstávají neznámé.
- Doplněny podmínky WAN/CGNAT, DHCP rezervace, bezpečnostní požadavky a akceptační testy.
- Aktualizován stav implementace a plán vývoje.

## Bezpečnostní a funkční omezení
- Router nebyl konfigurován a firmware nebyl aktualizován.
- Vzdálený přístup není implementován ani otestován.
- Firmware ESP32 se touto dokumentační revizí nemění.
- Přímé vystavení webu ESP32 do internetu je zakázáno.

## Ověření
- Nativní testy revize 0.59: 43/43 prošlo.
- Revize 0.60 nemění zdrojový kód firmware; testy jsou uvedeny jako výchozí stav, nikoli jako nový test této dokumentační změny.
