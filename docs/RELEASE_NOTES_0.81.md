# Release notes — DEV KIT 0.81

Datum: 2026-11-04

## CredentialStore implementation gate

- Doplněn přesný stavový kontrakt `VALID`, `EMPTY`, `CORRUPT`, `IO_ERROR`.
- Rozlišeno úplně prázdné NVS od částečného/poškozeného záznamu.
- Definován bezpečný migrační postup se staging záznamy, read-back a zachováním poslední ověřené sady.
- Stanoveny konkrétní podmínky, za kterých smějí ESP32 #1 a #2 spustit AP a lokální web.
- Doplněna integrační a power-cut testovací matice.
- Zaznamenáno, že `String` neprokazuje bezpečné nulování RAM a HTTP Basic Auth na HTTP neposkytuje šifrování.

Firmware se nemění. CredentialStore není implementován ani zapojen do AP call-sites. Důvodem je dosud nevyřešený bezpečný prvotní provisioning/obnova a neověřený fyzický servisní vstup.
