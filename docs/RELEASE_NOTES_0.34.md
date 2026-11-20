# Revize 0.34 — základ kontroly budoucích řídicích příkazů

Datum: 2026-10-31

## Změny
- Přidán `PeerCommandGuard` jako transportně nezávislý admission/replay guard pro metadata budoucích řídicích příkazů.
- Příkaz je odmítnut, pokud nebyl označen jako autentizovaný externím ověřovačem, míří na jiné boot ID přijímače, obsahuje nulové ID/sekvenci, opakuje starou sekvenci v rámci stejného bootu odesílatele nebo opakuje některé z posledních 16 request ID.
- Přidán samostatný nativní test.
- Dokumentace protokolu opravena na aktuální 52bajtový stavový rámec.

## Bezpečnostní omezení
- Modul neimplementuje HMAC ani jinou kryptografickou autentizaci. Parametr `authenticated` je pouze důvěryhodný výsledek budoucí ověřovací vrstvy; nesmí být nastaven na základě CRC ani prosté UDP adresy.
- Cache request ID je volatilní a po restartu se smaže. Trvalá idempotence a autentizované znovunavázání po restartu peeru nejsou hotové.
- Modul není zapojen do UDP, transferů, ACK/REJECTED/FAILED ani fyzických výstupů. Žádné řízení ventilů/čerpadel nebylo aktivováno.

## Ověření
- `tests/run_native_tests.sh`: 21/21 testovacích programů prošlo.
- Skutečný Arduino-ESP32 build a fyzické testy nebyly provedeny.
