# Revize 0.41 — autentizované odpovědi na příkazy

- Přidán `PeerCommandResponse.*` s přesně definovanými výsledky ACK, REJECTED a FAILED.
- Odpověď kopíruje request ID a obrací identitu zařízení i boot ID; obsahuje vlastní sekvenci odpovídající jednotky a kód původního příkazu.
- Definovány strojově čitelné důvody odmítnutí/chyby: neplatný požadavek, nebezpečná hladina, prázdný zdroj, plný cíl, kritická ochrana, neplatný snímač/konfigurace, chyba úložiště, obsazeno, ztráta komunikace, timeout, nepřipravený výstup a interní chyba.
- Neplatné kombinace jsou odmítnuty: ACK musí mít důvod NONE; REJECTED a FAILED musí mít konkrétní důvod.
- Test ověřuje korelaci odpovědi s požadavkem, chybná ID/boot ID, request ID, sekvenci, kombinace stavů a HMAC wire round-trip včetně odmítnutí pozměněného rámce.
- Odpovědní modul je transportně nezávislý; není připojen k UDP, transfer automatu ani fyzickým výstupům. ACK znamená potvrzení přijetí/rozhodnutí, nikoli důkaz fyzického průtoku.
