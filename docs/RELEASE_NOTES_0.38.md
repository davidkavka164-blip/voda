# Revize 0.38 — challenge-response handshake

- Přidán transportně nezávislý modul `PeerHandshake.*` pro vzájemné ověření obou jednotek pomocí challenge-response.
- Iniciátor posílá identitu zařízení, vlastní boot ID a 128bitový nonce. Odpovídající jednotka vrací vlastní boot ID, nový 128bitový nonce a HMAC-SHA-256 podpis celého přepisu handshake.
- Iniciátor ověří odpověď, vytvoří FINISH HMAC; responder přijme handshake až po ověření FINISH. Tím se ověřuje znalost společného tajemství na obou stranách.
- Z obou nonce, obou identit a obou boot ID se doménově odděleným HMAC odvozuje 32bajtový session key pro budoucí autentizaci příkazových rámců.
- Odmítnuty jsou nulové/stejné identity, nulová boot ID, nulové nebo stejné nonce a tajemství kratší než 32 bajtů. Při chybě ověření se session key vynuluje.
- Nonce a boot ID musí na ESP32 dodat volající vrstva z kryptograficky bezpečného zdroje (`esp_random()`); modul sám RNG neimplementuje. `PeerHandshakeResponder` drží jeden pending transcript a spotřebuje jej při prvním FINISH pokusu; volající musí navíc vynutit timeout. Session se nesmí povolit před přijetím FINISH.
- Nativní testovací sada: 24/24 programů. Překlad Arduino-ESP32, skutečné RNG, provisioning klíče, síťový přenos, NVS a hardware nejsou ověřeny.
- HMAC handshake zatím není napojen na UDP/W5500 ani `PeerCommandGuard`; žádný řídicí příkaz ani fyzický výstup se neaktivuje.
