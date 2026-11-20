# DEV KIT 1.00 — REV92

Datum: 19. 11. 2026

## Změny
- Extrahována kontrola Origin/Host do `HWC::isSameHttpOriginAuthority()` v `firmware/common/HttpOriginPolicy.h`.
- Funkce synchronizována do zdrojových stromů ESP32 #1 a ESP32 #2.
- Obě autentizační brány používají stejnou funkci před zpracováním POST.
- Přidán nativní test pozitivních a negativních případů.
- Aktualizován kontrolní skript CSRF/Origin.

## Ověření
- 81 nativních testovacích programů PASS.
- Integrační a statické kontroly v `tests/run_native_tests.sh` PASS.
- Arduino cílový překlad nebyl proveden; fyzický Ethernet test nebyl proveden.

## Omezení
REV92 stále nepřidává HTTP server přes W5500. `WebServer` a všechny routy zůstávají zachovány; změna se týká pouze společné politiky ověření Origin/Host.
