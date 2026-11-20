# HOME WATER CONTROLLER — release notes REV53

Datum: 14. 11. 2026

## Návazný audit REV52

- Ověřen skutečný stav UDP call-sites obou `.ino` souborů.
- Potvrzeno, že `PeerAuthenticatedStatusReceiver` je zatím samostatný modul a není volán z produkční UDP smyčky.
- Potvrzeno, že ESP32 #2 stále vysílá 52B CRC-only status broadcast na UDP 4211 a ESP32 #1 jej přijímá na portu 4211.
- Potvrzeno, že `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` je `false` a blokuje běžný automatický i servisní horní transfer.
- Zpřesněn další blokátor: produkční důvěrný backend peer tajemství a cílově ověřené NVS/Flash Encryption.
- Bez změny firmware, pinoutu, UDP topologie, NVS layoutu, partition table nebo eFuse.

## Testy

- Nativní: 70/70 testovacích programů PASS.
- ASan/UBSan: 68/68 testovacích programů PASS.
- Python provisioning klient: 17/17 PASS.
- Peer auth transport non-integration guard: PASS.
- Peer status control gate: PASS.
- Common module synchronization: PASS.
- Mixed Wi-Fi/Ethernet transport check: PASS.
- ZIP integrity: PASS.

Arduino-ESP32 cílový build, skutečná NVS/Flash Encryption, fyzický provisioning, UDP handshake a hardware testy nebyly provedeny.

REV53 je auditní/dokumentační revize. Neoznačuje autentizovanou komunikaci za dokončenou.
