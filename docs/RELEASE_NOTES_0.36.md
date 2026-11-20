# Revize 0.36 — autentizovaný binární rámec příkazů

- Přidán `PeerCommandFrame.*`: pevný kanonický little-endian rámec o délce 68 bajtů. Prvních 36 bajtů tvoří přesně definovaná podepisovaná oblast; posledních 32 bajtů je HMAC-SHA-256.
- Rámec nese magic, verzi, typ COMMAND/RESPONSE, kód příkazu, ID odesílatele/cíle, boot ID obou stran, sekvenci, request ID a dva celočíselné argumenty.
- Encoder odmítá neplatná metadata, nulové identifikátory a klíč kratší než 32 bajtů. Decoder ověří délku, magic/verzi, rezervované bajty, HMAC konstantně časovým porovnáním a až poté zpřístupní dekódovaná pole.
- Test ověřuje round-trip, záporný argument, změnu podepsaných dat, chybnou délku, krátký klíč a neplatné request ID.
- Modul zatím není připojen k UDP, `PeerCommandGuard`, transferovému automatu ani webu. Neexistuje správa/rotace klíčů ani handshake; samotný rámec proto zatím neopravňuje k přijetí síťového příkazu.
- Nativní testovací programy: 23/23.
