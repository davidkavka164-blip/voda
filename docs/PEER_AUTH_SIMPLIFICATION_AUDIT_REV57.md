# HOME WATER CONTROLLER — zjednodušení autentizace peer komunikace REV57

Datum: 14. 11. 2026  
Vychází z: `HOME_WATER_CONTROLLER_DEV_KIT_1.00_AUDIT_DRAFT_REV56.zip`  
Rozsah: návrhový audit, bez změny firmware, pinů, topologie, NVS layoutu nebo hardware.

## 1. Rozhodnutí

Pro současnou komunikaci ESP32 #2 → ESP32 #1 se nebude implementovat plný systém handshake a session managementu. Je to jednosměrná periodická telemetrie v domácí LAN; pro její účel je přiměřenější jednoduché ověření každého datagramu.

Cílový návrh:

1. Zachovat stávající UDP broadcast, porty 4211/4212 a stávající datový rámec 52 B.
2. Zachovat `HmacSha256.cpp/.h` a použít jeden dlouhodobý společný 32B klíč.
3. K 52B statusu připojit 32B HMAC-SHA256 tag vypočtený nad přesnými 52B wire daty. Výsledný datagram má 84 B.
4. ESP32 #1 ověří HMAC v konstantním čase ještě před dekódováním/předáním rámce do `PeerStatusMonitor`.
5. Stávající `bootId` a `sequence` se zachovají. `PeerStatusMonitor` už odmítá duplicitní, starší a mimo pořadí přijaté sekvence během běhu přijímače a sleduje změny boot epoch.
6. Po výpadku komunikace, chybě HMAC nebo restartu se běžný horní transfer nepovolí, dokud nepřijde platná aktuální telemetrie a nejsou splněny všechny ostatní bezpečnostní podmínky.
7. Nouzový ruční ventilový režim zůstává samostatnou, obsluhou potvrzovanou výjimkou podle stávajících pravidel.

## 2. Co se pro tento účel nepoužije

Není potřeba:

- HELLO / CHALLENGE / FINISH / READY handshake;
- odvozování session key a směrových session klíčů;
- časované retry/timeout orchestrace handshake;
- `PeerCommandSession`, `PeerCommandGuard` a command-response protokol;
- trvalý replay checkpoint s rezervací sekvenčních rozsahů;
- secure element ATECC608C ani úprava bootloaderu, partition table nebo eFuse.

Tyto moduly mohou zůstat v archivu pro případné budoucí rozšíření, ale nemají být závislostí současné peer status komunikace.

## 3. Klíč a jeho uložení

Pro první jednoduchou implementaci se počítá s jedním 32B klíčem uloženým v samostatném Preferences/NVS namespace na obou jednotkách. Zápis musí ověřit přesné zpětné načtení; při chybě nebo chybějícím klíči se autentizovaný peer stav nepovažuje za dostupný.

Důležité omezení: běžné Preferences/NVS v aktuálně předpokládaném Arduino-ESP32 1.0.6 není šifrované úložiště. Klíč tedy není chráněn proti fyzickému přečtení flash paměti nebo získání firmware/flash dumpu. Toto riziko se přijímá a zdokumentuje; nesmí se tvrdit, že klíč je uložen bezpečně či šifrovaně.

Klíč se nesmí vypisovat do Serial logu ani přenášet běžnou UDP komunikací. Způsob jednorázového fyzického vložení klíče do obou jednotek se navrhne co nejjednodušeji až před implementací provisioning části; nesmí se kvůli tomu znovu zavádět rozsáhlý provisioning framework.

## 4. Přesný rozsah ochrany proti opakování

`bootId` a `sequence` už jsou součástí autentizovaných 52 B. Během běhu ESP32 #1 tak lze odmítat opakované a starší rámce a po změně boot ID přijmout novou epochu.

Současný `PeerStatusMonitor` drží historii epoch pouze v RAM. Po restartu ESP32 #1 se tato historie ztratí. Jednoduchá varianta proto neposkytuje kryptografickou ochranu proti cílenému přehrání dříve zachyceného platného datagramu bezprostředně po restartu přijímače. Plná ochrana přes restart by vyžadovala trvalý replay stav nebo nový handshake; to se v této fázi nepřidává.

Proti běžným náhodným duplikátům, zpožděným UDP paketům a podvržení rámce bez znalosti klíče poskytne jednoduchý HMAC + stávající sequence/bootId přiměřenou ochranu. Po restartu a výpadku se navíc zachová fail-closed pravidlo pro řízení transferu.

## 5. Co se nesmí změnit

- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`, dokud nebude nová cesta integrována a ověřena.
- CRC-only rámec nesmí obnovit důvěryhodný peer stav ani povolit běžný/servisní horní transfer.
- HMAC se musí ověřit před jakýmkoli předáním dat do `PeerStatusMonitor`.
- Nouzové ruční ovládání zůstává oddělené.
- Ochrany čerpadel, senzory, kalibrace, NVS záznamy, síťová topologie a výstupy relé se tímto návrhem nemění.

## 6. Změny zdrojů

Žádné. REV57 je pouze návrhový audit. Produkční UDP smyčky zůstávají ve stavu REV56; handshake není zapojen a autentizovaný receiver není použit. Bezpečnostní brána horní cesty zůstává vypnutá.

## 7. Doporučený další postup

1. Zkontrolovat konkrétní existující HMAC implementaci a doplnit malý status codec pro `52 B + 32 B tag`.
2. Přidat jednoduché NVS úložiště klíče s ověřením readback; bez tvrzení o šifrování.
3. Integrovat odeslání autentizovaného statusu do ESP32 #2 a ověření na ESP32 #1 před `PeerStatusMonitor`.
4. Přidat testy správného klíče, špatného klíče, změněného payloadu, zkráceného/přebytečného paketu, duplikátu, staré sekvence, změny boot ID a výpadku komunikace.
5. Ověřit Arduino cílový překlad a skutečnou UDP komunikaci na obou deskách.
6. Teprve po úspěchu samostatně posoudit změnu `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED`.

## 8. Co zatím není ověřeno

- Přesná verze Arduino-ESP32 core instalovaná uživatelem.
- Cílový překlad REV56/REV57 v jeho Arduino IDE.
- Skutečný zápis a čtení peer klíče na deskách.
- Skutečný UDP HMAC provoz a chování při restartu/výpadku.
- Fyzické chování čerpadel a ventilů po změně firmware.
