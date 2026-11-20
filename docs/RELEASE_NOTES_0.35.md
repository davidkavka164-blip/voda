# Revize 0.35 — HMAC-SHA-256 stavební blok

Datum: 2026-10-31

## Změny
- Přidán přenositelný `HmacSha256.*`: výpočet HMAC-SHA-256 a konstantně časové porovnání 32bajtových tagů.
- Přidán `test_hmac_sha256` s ověřením proti RFC 4231 vektoru 1, 2 a dlouhému klíči (vektor 6).
- Test je součástí `tests/run_native_tests.sh`; společné moduly jsou synchronizovány do obou firmware `src` adresářů.

## Bezpečnostní omezení
- HMAC modul je pouze kryptografický primitivní stavební blok. Není připojen k UDP, peer protokolu, `PeerCommandGuard`, transferům ani výstupům.
- Není definováno bezpečné generování, provisioning, ukládání, obnova ani rotace klíče.
- Není definováno kanonické kódování příkazového rámce. Podepisovat se musí přesně definované bajty; nikdy surová C++ struktura s paddingem nebo platformní endianitou.
- Dokud tyto části nebudou dokončeny a ověřeny, řídicí komunikace zůstává vypnutá.

## Ověření
- `tests/run_native_tests.sh`: 22/22 testovacích programů prošlo.
- Skutečný Arduino-ESP32 build, síťový test a fyzický hardware nebyly ověřeny.
