# Revize 0.39 — kanonické wire kódování handshake

- Přidán `PeerHandshakeWire.*` pro explicitní serializaci HELLO (28 B), CHALLENGE (80 B) a FINISH (38 B). Kódování má pevnou hlavičku `HWH1`, verzi a typ zprávy.
- Vícebajtová čísla se kódují little-endian; nepřenášejí se surové C++ struktury a jejich padding.
- Dekodéry vyžadují přesnou délku, magic, verzi, typ a strukturálně platné identity, boot ID a nonce. Při chybě vynulují výstupní strukturu.
- Test navíc provádí celý handshake přes wire encode/decode a ověřuje, že obě strany odvodí shodný session key.
- Nativní sada: 24/24 testovacích programů prošlo; celá sada prošla také AddressSanitizer a UndefinedBehaviorSanitizer.
- Wire codec zatím není připojen k UDP/W5500 ani k řízení session; chybí timeout, RNG/boot ID vrstva, provisioning a bezpečné uložení tajemství. Arduino-ESP32 build a fyzické síťové testy nebyly provedeny. Žádné čerpadlo ani ventil se neovládá.
