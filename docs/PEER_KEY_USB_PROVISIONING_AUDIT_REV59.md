# HOME WATER CONTROLLER — jednoduché zavedení společného peer klíče přes USB

Revize: 1.00 AUDIT REV59  
Datum: 2026-11-14  
Stav: audit zdrojů REV58; návrh minimálního lokálního provisioningu; bez změny provozního firmware

## 1. Závěr

Pro zavedení jednoho společného HMAC klíče do ESP32 #1 a ESP32 #2 není potřeba další hardware, samostatný autorizační GPIO vstup ani síťový provisioning.

Nejjednodušší přiměřená varianta je použít již existující USB–UART sériové připojení desek a jednoduché textové příkazy. Obě jednotky mají v `.ino` inicializaci `Serial.begin(115200)` a sériový port se nyní používá pro diagnostické výpisy. V aktuálních sketchech ale není implementován příjem příkazů pro peer klíč.

## 2. Co už ve zdrojích existuje

- `PeerStatusHmacCodec` ověřuje původní 52bytový status rámec pomocí HMAC-SHA256; výsledný rámec má 84 bytů.
- `NvsPeerSharedKeyStore` ukládá 32bytový klíč do namespace `hwc_peer`, položka `shared`, a při zápisu ověřuje zpětné načtení.
- Oba moduly jsou ve `firmware/common/` a synchronizované ve zdrojích obou ESP32.
- ESP32 #1 i ESP32 #2 používají `Serial.begin(115200)` a vypisují diagnostiku. V provozních `.ino` však zatím není příjem peer klíče přes Serial.
- Stávající `CredentialProvisioningSerialProtocol` patří k jinému workflow: nastavuje Wi-Fi a administrátorské přihlašovací údaje, používá fyzickou GPIO autorizaci a jeho adapter není připojen k provozním sketchům. Pro peer klíč jej nebudeme rozšiřovat.

## 3. Navržené minimální příkazy

Příkazy se přijímají pouze po místním USB–UART připojení. Parser musí mít pevně omezený vstupní buffer, odmítnout neúplné/příliš dlouhé řádky a nikdy nevypisovat přijatý klíč.

- `PEERKEY STATUS` — vrátí pouze `SET` nebo `MISSING`.
- `PEERKEY SET <64 hex znaků>` — dekóduje přesně 32 bajtů, uloží je přes `NvsPeerSharedKeyStore::save()` a potvrdí pouze úspěch/neúspěch.
- `PEERKEY ERASE` — smaže uložený klíč; bez klíče zůstává autentizovaná peer komunikace a horní transfer zablokovaný.

Stejný klíč se zadá nejprve do jedné a potom do druhé jednotky. Přijatý text ani klíč se nesmí echo-vat do Serial Monitoru, logovat ani vracet v odpovědi. Neplatný vstup nesmí změnit již uložený klíč.

Příkazy jsou lokální servisní rozhraní, nikoli kryptografická autentizace operátora. Jejich ochrana vychází z nutnosti fyzicky připojit USB kabel k desce. To odpovídá zvolenému domácímu modelu hrozeb; není to ochrana proti osobě s fyzickým přístupem k zařízení.

## 4. Vygenerování a zacházení s klíčem

Klíč bude 32 náhodných bajtů (256 bitů), zobrazených jako 64 hex znaků. Vygeneruje se lokálně na počítači uživatele; nebude pevně zapsán ve firmware, posílán přes domácí LAN ani ukládán do projektového ZIP archivu.

Při zavádění se použije Arduino IDE Serial Monitor, rychlost 115200 baud a ukončení řádku Newline. Klíč se vloží do obou zařízení. Firmware jej nesmí vypsat ani ukládat do diagnostických logů.

NVS úložiště je v této variantě nešifrované. Osoba s fyzickým přístupem k flash paměti může klíč získat. To je známé a přijaté omezení; nesmí se tvrdit, že klíč je chráněn proti fyzickému přečtení.

## 5. Dopady a provozní opatrnost

- Otevření Serial Monitoru může podle USB–UART převodníku a nastavení DTR/RTS vyvolat reset desky.
- Obě jednotky nastavují bezpečnostní relé čerpadel do LOW již na začátku `setup()`. Při resetu tedy povolení motoru odpadne.
- Provisioning se proto provede při odstaveném systému, nikoli během běžícího čerpadla nebo transferu.
- Klíč se ukládá samostatně v NVS každé desky; zápis do ESP32 #1 neznamená zápis do ESP32 #2.
- Pokud klíč chybí nebo je neplatný, autentizovaná komunikace nesmí být považována za dostupnou. Stávající `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.

## 6. Omezení anti-replay ochrany

Existující `PeerStatusMonitor` kontroluje pořadí `sequence` a změny `bootId` během běhu přijímače. Po restartu ESP32 #1 se jeho RAM historie vynuluje. Bez ukládání každého přijatého paketu do NVS proto nelze tvrdit, že je replay ochrana úplná i přes restart přijímače.

Pro jednoduchou jednosměrnou telemetrii je nyní přiměřené ponechat HMAC, pořadové číslo, náhodné `bootId`, časovou čerstvost a fail-closed chování. Nepřidávat zápis do flash každé dvě sekundy jen kvůli trvalému čítači. Krátké reziduální okno po restartu přijímače musí být zdokumentováno a ověřeno v testu; nesmí se řešit dalším handshake systémem.

## 7. Co se v REV59 nemění

- Žádný zásah do provozních `.ino`, UDP transportu, pinů, relé, NVS partition ani eFuse.
- ESP32 #2 zatím dál vysílá CRC-only rámec 52 B a ESP32 #1 jej přijímá pouze jako diagnostickou telemetrii.
- Horní automatický i běžný servisní transfer zůstává blokován, protože `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED == false`.
- Nouzové ruční ovládání ventilů zůstává samostatnou, uživatelem potvrzenou výjimkou podle stávající logiky.

## 8. Ověření zdrojů REV58

- `./tests/run_native_tests.sh`: 72/72 nativních testovacích programů PASS; všechny přítomné statické integrační kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 70/70 nativních testovacích programů PASS; statické integrační kontroly PASS.
- ZIP REV58 byl před kontrolou rozbalen bez chyby.
- Nebyl proveden Arduino-ESP32 cílový překlad, nahrání do desek ani fyzická zkouška USB–UART/NVS.

## 9. Další krok

Připravit malý, hostitelsky testovatelný parser tří příkazů a jeho bezpečné začlenění do obou `.ino`. Teprve po kontrole parseru doplnit USB obsluhu. Do té doby se peer klíč fyzicky neprovisionuje a autentizované řízení se nezapíná.
