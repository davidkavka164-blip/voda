# Aplikační protokol mezi ESP32 #1 a ESP32 #2 — aktuální stav 0.69 (historie od 0.39)

Prozatímní transport stavových zpráv je implementován jako UDP broadcast přes W5500 na stejné DHCP LAN. Přenositelný základ zprávy a serializace je v `firmware/common/PeerProtocol.*`; fyzická síťová cesta zatím nebyla ověřena a závisí na topologii.

## Zásady
- Každá zpráva obsahuje `protocolVersion`, `deviceId`, `messageType`, `sequence`, `uptimeMs` a stav validních dat.
- Stavová zpráva obsahuje hladinu horní nádrže, její validitu, odhad objemu a stav kalibrace, teplotu a její validitu, místní stav ochrany čerpadla, aktivní alarmy a stáří měření.
- Hlavní jednotka sleduje heartbeat a stáří poslední zprávy. Samotná dosažitelnost sítě neznamená, že jsou měřená data aktuální.
- Každý řídicí požadavek má jedinečné `requestId`, časové omezení platnosti a explicitní výsledek ACK/REJECTED/FAILED. Opakování stejného `requestId` nesmí provést akci znovu.
- ESP32 #2 nikdy nevypne svou místní bezpečnost jen proto, že ESP32 #1 požádá o transfer.
- Při ztrátě spojení se žádný rozpracovaný transfer automaticky znovu nespustí po obnovení komunikace.
- Chybějící, stará nebo nekompatibilní data jsou považována za nedostupná; nesmí se nahrazovat nulou nebo poslední hodnotou bez označení stáří.

## Otevřená rozhodnutí
- Transport a fyzické propojení.
- Perioda heartbeat a timeouty podle reálné sítě.
- Kódování zpráv (JSON nebo kompaktní binární struktura) podle zvoleného transportu a dostupné RAM.
- Autentizace řídicích zpráv v rámci domácí sítě.

Status heartbeat je vysílán z ESP32 #2 každé 2 sekundy a ESP32 #1 označí peer data za zastaralá po 6 sekundách bez zprávy. Řídicí zprávy, autentizace a ACK/REJECTED/FAILED implementovány nejsou; protokol proto není připraven pro řízení transferů.


## Implementovaná část stavové zprávy
- `protocolVersion`, `deviceId`, `messageType`, `bootId`, `sequence`, `uptimeMs`, `measurementAgeMs`.
- Hladina, objem a teplota horní nádrže s oddělenými příznaky validity.
- Příznak připravenosti místní ochrany čerpadla a `alarmBits`.
- CRC32 kontrolní součet a kontrola očekávané verze/ID/rozsahů.
- `PeerStatusMonitor` odmítá opakované/staré sekvence v rámci stejného boot ID a vyhodnocuje stáří přijatých dat.
- Wire formát je 52 bajtů, little-endian; pole se serializují explicitně a neposílá se surová struktura s platformním paddingem. CRC32 pokrývá všechna pole kromě samotného kontrolního součtu.


## Prozatímní UDP transport v revizi 0.16
- ESP32 #2 vysílá na broadcast `255.255.255.255:4211`; ESP32 #1 poslouchá UDP port 4211.
- Obě zařízení získávají IP přes DHCP. Peer status nepoužívá Wi-Fi webový server; W5500 slouží pouze pro místní stavové zprávy.
- Vyžadována stejná L2/DHCP LAN bez broadcast isolation. Žádné statické IP ani autentizace peer zpráv nejsou zatím nastaveny. CRC32 detekuje poškození, nikoli úmyslné podvržení.
- Objem/teplota horní nádrže a místní připravenost čerpadla #2 jsou zatím označeny jako neplatné/nepřipravené. Transfer nesmí být povolen na základě těchto dat.


## Revize 0.34 — admission guard pro budoucí řídicí příkazy
- `PeerCommandGuard` je transportně nezávislý modul pro kontrolu metadat příkazu: `senderBootId`, `senderSequence`, `requestId` a `targetBootId`.
- Před přijetím vyžaduje `authenticated=true`; tuto hodnotu smí dodat pouze budoucí ověřená kryptografická vrstva. Guard sám žádný HMAC nepočítá a nelze jej považovat za autentizaci.
- Odmítá nulové identifikátory/sekvenci, cílový boot ID jiné než aktuální lokální boot ID, opakovanou nebo starou sekvenci stejného odesílacího bootu a duplicitní request ID z posledních 16 přijatých požadavků.
- Cache request ID je pouze v RAM a po restartu se smaže. Nejde tedy o trvalou idempotenci přes restart. Přepnutí sender boot ID zahájí novou sekvenční řadu; jeho bezpečné svázání s autentizovaným handshake je nutné doplnit před síťovou integrací.
- Modul není integrován do UDP, webu, transfer automatu ani výstupů. ACK/REJECTED/FAILED rámce nejsou implementovány.


## Revize 0.35 — kryptografický stavební blok
- `HmacSha256.*` implementuje HMAC-SHA-256 podle RFC 2104/4231 a konstantně časové porovnání tagu. Testy ověřují známé vektory RFC 4231 včetně dlouhého klíče.
- Modul přijímá libovolné bajty; zatím není definováno kanonické binární kódování řídicího rámce, takže jej nelze bezpečně volat nad surovou C++ strukturou.
- HMAC není dosud integrován do UDP ani `PeerCommandGuard`. Není definována bezpečná distribuce, uložení či rotace sdíleného tajemství. Bez těchto částí se žádný řídicí příkaz nesmí přijímat.


## Revize 0.36 — kanonický autentizovaný rámec
- `PeerCommandFrame.*` definuje pevný 68bajtový formát: 36 bajtů hlavička/payload + 32 bajtů HMAC-SHA-256. Všechna vícebajtová pole jsou little-endian; nepodepisuje se surová C++ struktura.
- Podepisovaná oblast zahrnuje magic, verzi, typ, kód příkazu, ID obou zařízení, boot ID obou stran, sekvenci, request ID a dva signed 32bitové argumenty. Rezervované bajty musí být nula.
- Decoder ověřuje HMAC před publikováním dekódovaných hodnot. Minimální délka klíče je 32 bajtů.
- Zatím není definováno bezpečné prvotní spárování, uložení/rotace klíče ani autentizovaný handshake. Rámec není integrován do UDP ani admission guardu. Příkazy zůstávají zakázané.


## Revize 0.37 — explicitní peer session a svázání identity
- `PeerCommandGuard` nově zná lokální ID zařízení, očekávané ID peeru, lokální boot ID a boot ID peeru ustavené samostatnou session operací.
- Příkaz je odmítnut, pokud ID odesílatele/cíle nesouhlasí, cílový boot ID není aktuální, session nebyla ustavena nebo boot ID odesílatele neodpovídá právě ustavené session.
- Změna boot ID peeru již nikdy automaticky neresetuje sekvenci. Vyžaduje explicitní `recordAuthenticatedPeerSession()`; její boolean parametr je pouze kontrakt pro budoucí ověřovací vrstvu, nikoli autentizace.
- Cache posledních 16 request ID přežívá změnu peer session v RAM, ale nikoli restart. Cílový boot ID brání přijetí zachycených rámců adresovaných předchozímu startu lokální jednotky.
- Skutečný fresh challenge-response handshake, bezpečné generování/uložení/rotace klíče, perzistentní anti-replay a UDP integrace nejsou implementovány. Příkazy proto zůstávají zakázány.


## Revize 0.38 — transportně nezávislý vzájemný handshake
- `PeerHandshake.*` definuje tříkrokový postup HELLO → CHALLENGE → FINISH. HELLO nese identitu iniciátora, cílovou identitu, boot ID iniciátora a jeho 128bitový nonce.
- CHALLENGE obsahuje obě identity, obě boot ID, oba nonce a HMAC-SHA-256 přes kanonický transcript s doménovým oddělením. Iniciátor odmítne změněné nebo neautentizované údaje.
- FINISH je HMAC důkaz iniciátora nad stejným transcriptem. Responder session key odvodí a vrátí až po ověření FINISH. Obě strany odvodí shodný 32B session key přes samostatnou doménu.
- `PeerHandshakeResponder` přijímá pouze očekávanou identitu peeru, drží jeden pending transcript a spotřebuje jej při prvním FINISH pokusu. Volající musí přidat timeout a odmítnout další HELLO, dokud pending handshake nevyprší nebo není zrušen.
- Volající musí generovat oba 128bitové nonce i boot ID z kryptograficky bezpečného zdroje (na ESP32 `esp_random()`). Nesmí se použít `random()`, čas ani MAC adresa jako náhrada náhodnosti.
- Modul zatím nedefinuje wire encoding ani síťové timeouty, neukládá společné tajemství, není napojen na UDP/W5500 a nevolá `PeerCommandGuard`. Session nesmí být použita pro příkazy, dokud není handshake dokončen na obou stranách.
- Nativní testy ověřují shodný session key, nesprávný klíč, pozměněný transcript/FINISH, neplatné nonce/identity a jednorázové přijetí FINISH ve stateful responderu.


## Revize 0.39 — wire kódování handshake
- `PeerHandshakeWire.*` definuje tři pevné typy rámců s hlavičkou `HWH1`, verzí 1 a typem zprávy: HELLO 28 B, CHALLENGE 80 B, FINISH 38 B.
- Pole jsou serializována explicitně; 32bitová čísla little-endian. Surové C++ struktury se neposílají.
- Dekodér kontroluje přesnou délku, magic, verzi, typ, identity, boot ID a nonce. Při odmítnutí vynuluje výstup. Kryptografické ověření HMAC následuje až po dekódování CHALLENGE/FINISH pomocí `PeerHandshake.*`.
- Wire codec zatím není napojen na UDP/W5500. Časové limity, generování nonce/boot ID z `esp_random()`, bezpečný provisioning tajemství a aktivace session přes `PeerCommandGuard` nejsou hotové.


## Revize 0.40 — politika úložiště společného tajemství
- `PeerSecretVault.*` definuje rozhraní backendu, který musí deklarovat `confidentialStorageAvailable()`. Bez této vlastnosti se žádné čtení, zápis ani mazání tajemství nepovolí.
- Rozhraní vyžaduje 32B nenulové tajemství a váže ho na uspořádanou dvojici ID (lokální ID, peer ID). Na každém zařízení jde o opačné pořadí identit, ale provisionovaný obsah tajemství musí být stejný.
- Provisioning a mazání vyžadují explicitní fyzické potvrzení; žádná webová nebo síťová cesta k provisioning zatím neexistuje.
- Toto není implementace šifrování. Konkrétní backend smí vrátit `confidentialStorageAvailable() == true` pouze po ověření, že cílová konfigurace ESP-IDF skutečně používá NVS encryption nebo jiný odpovídající chráněný mechanismus. Prosté `Preferences`/NVS bez šifrování musí zůstat odmítnuto.


## Revize 0.44 — session-bound directional frame codec

`PeerSessionCodec.*` je transportně nezávislá vrstva nad 68B `PeerCommandFrame`. Vytváří se až po úspěšném handshake a obdrží směrové klíče a pevný kontext lokální/peer identity a boot ID.

- Lokální iniciátor podepisuje odchozí rámce klíčem I→R a ověřuje příchozí klíčem R→I.
- Lokální responder podepisuje odchozí rámce klíčem R→I a ověřuje příchozí klíčem I→R.
- Odesílaný rámec musí obsahovat lokální sender ID/boot ID a peer target ID/boot ID. Přijatý rámec musí mít opačné hodnoty.
- Neplatný kontext nebo dekódování je odmítnuto; výstupní rámec se před dekódováním vynuluje. Vnitřní kopie směrových klíčů se při destrukci nulují.
- Třída je necopyable, aby nedocházelo k nekontrolovanému množení kopií klíčů.

Tento modul neprovádí handshake ani správu session, neukládá replay stav a neposílá UDP/W5500 pakety. Je nutné jej vytvořit pouze po ověřeném FINISH a zahodit při změně boot ID nebo ztrátě session. Síťová integrace zůstává otevřená.

## Handshake revize 0.52 — potvrzení READY
- Wire handshake verze 2 nyní definuje HELLO (28 B), CHALLENGE (84 B), FINISH (38 B) a READY (38 B).
- Po přijetí a kryptografickém ověření FINISH responder vytvoří READY: HMAC-SHA-256 nad doménou `HWC-HS-READY-V1`, klíčem je session key odvozený z autentizovaného transcriptu.
- Iniciátor musí ověřit READY stejným session key; teprve poté smí považovat handshake za dokončený a uvažovat o aktivaci command session.
- Ztracený READY znamená, že iniciátor session nepovažuje za aktivní. Responder musí při timeoutu/novém handshake starou session zrušit; síťová orchestrace a životní cyklus session zatím nejsou implementovány.
- READY dokazuje držení session key, ale samo o sobě nenahrazuje bezpečný provisioning, ochranu dlouhodobého tajemství, timeouty, limity pokusů ani transportní orchestraci.


## Handshake wire v2 — stav k revizi 0.53
Handshake používá pevné zprávy HELLO (28 B), CHALLENGE (84 B), FINISH (38 B) a READY (38 B), s hlavičkou `HWH1`, verzí 2 a explicitní little-endian serializací. CHALLENGE obsahuje autentizovanou persistentní replay floor. READY je HMAC potvrzení responderu po ověření FINISH.

`PeerHandshakeInitiator` nyní implementuje transportně nezávislý lifecycle iniciátora a chrání pravidlo, že sequence floor se aplikuje až po ověření CHALLENGE a session key se zpřístupní až po READY. FINISH lze opakovat pouze jako identický uložený rámec. Tato vrstva není dosud napojena na UDP/W5500; síťový stavový automat musí zajistit adresní vazbu, timeouty, opakování a zrušení obou stran.


## Revize 0.69 — stabilita boot epochy a opožděné UDP rámce
- ESP32 #2 inicializuje `peerBootId` jednou při startu v `setup()`. Opakovaný DHCP/W5500 reconnect nesmí měnit boot ID ani resetovat `peerSequence`.
- `PeerStatusMonitor` si v RAM uchovává poslední čtyři opuštěné boot ID. Po přijetí nové boot epochy odmítá opožděné datagramy z nedávných předchozích epoch, které by jinak mohly vrátit monitor zpět ke starým datům.
- Test `test_peer_protocol` ověřuje přijetí nové epochy a odmítnutí následně doručeného rámce z předchozí epochy.
- Tato ochrana je omezená na poslední čtyři epochy a aktuální běh přijímače; není kryptografickou autentizací. Po restartu přijímače ani proti úmyslnému podvržení CRC rámce nenahrazuje HMAC handshake a persistentní anti-replay.
- Stavové UDP zůstává pouze pro dashboard/simulační dohled; řídicí příkazy a fyzické výstupy jsou zakázané.
