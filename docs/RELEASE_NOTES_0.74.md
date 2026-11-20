# Release notes — DEV KIT 0.74
Datum: 2026-11-04

## Pokračování P0 bezpečnostního auditu

- Doplněn nález P0-3: chybějící oba replay sloty jsou dnes nerozlišitelné od legitimního prvního spuštění.
- Popsán bezpečný směr další opravy pomocí inicializačního markeru a jeho omezení.
- Žádná síťová command integrace ani fyzické výstupy nebyly povoleny.
- Tato revize je auditní/dokumentační; firmware logika se nemění.

## Ověření

- Standardní nativní testy: zděděný stav 45/45; po této auditní dokumentační změně nebyla změněna firmware logika.
- ASan/UBSan: zděděný stav 45/45; po této auditní dokumentační změně nebyla změněna firmware logika.
- Arduino-ESP32 build, fyzické NVS/power-cut a hardware zůstávají neověřené.
