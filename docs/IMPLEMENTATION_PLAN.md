# Implementační plán a akceptační kritéria

## Etapa 0 — zadání a zdroje (provedeno)
- [x] Získat MASTER 1.3 a oba ZIP archivy.
- [x] Rozbalit a inventarizovat moduly.
- [x] Identifikovat nesoulady mezi vzory a novým zadáním.
- [x] Zaznamenat chybějící hardwarové údaje.

## Etapa 1 — návrh architektury (rozpracováno)
- [x] Definovat přenositelné moduly pro převod snímače, objemové výpočty, transferový automat a ochranu čerpadla.
- [x] Navrhnout rozdělení modulů mezi ESP32 #1 a #2 a aplikační kontrakt komunikace.
- [x] Definovat a otestovat základní datovou strukturu peer stavové zprávy (verze, ID, boot ID, sekvence, stav horní nádrže, teplota, alarmy a validita).
- [ ] Doplnit jednotné aplikační datové struktury všech stavů snímačů, transferů a alarmů a napojit je na běžící aplikaci.
- [x] Navrhnout transportně nezávislý aplikační protokol ESP32 #1 ↔ ESP32 #2 s heartbeat, čítačem zpráv, kontrolou stáří a bezpečným timeoutem.
- [x] Prozatímní UDP broadcast stavových zpráv přes W5500/DHCP LAN implementován.
- [ ] Potvrdit skutečnou topologii, DHCP/broadcast průchodnost a ověřit fyzický transport; implementovat řídicí zprávy a ACK.
- [x] Připravit transportně nezávislé koordinátory handshake iniciátora i responderu včetně ověření READY, replay floor, timeoutu a cache retry zpráv.
- [ ] Definovat stavový automat transferu včetně předběžných kontrol, otevírání, měření, dokončení, STOP, timeoutu a logování.
- [ ] Definovat a na hardware ověřit bezpečné výstupní stavy podle skutečného typu relé/ventilu a zapojení.
- [x] Definovat verzi a kontrolní součet záznamu NVS pro elektrickou kalibraci snímače.
- [x] Rozšířit NVS o oddělené úložiště rozměrů studny a naměřené objemové kalibrace.
- [x] Rozšířit NVS o verzované uložení denní historie přítoku s CRC; stav kritické blokace do NVS ještě není integrován.
- [x] NVS: validované sloučení lokální kalibrace, zápis, opětovné načtení a kontrola uloženého záznamu (hostitelský stub test).
- [x] Omezit zápisy 30denní historie do NVS: zapisovat jen po změně historie nebo platného data, maximálně jednou za 5 minut; nativní regresní test.

## Etapa 2 — čistá kostra a překlad
- [x] Vytvořit dvě samostatné Arduino IDE složky, každou s názvem hlavního `.ino` odpovídajícím názvu složky (zatím bezpečné kostry bez GPIO).
- [x] Přidat konfiguraci elektrické kalibrace, ADC čtení a NVS úložiště snímačů bez výkonového řízení.
- [x] Přidat základní webové nastavení ESP32 #1 (rozměry studny, objemová kalibrace, elektrická kalibrace dvou snímačů, Wi-Fi/AP) a dashboard měření/historie.
- [ ] Doplnit plný dashboard, transferové ovládání, historii a diagnostiku.
- [ ] Přeložit prázdnou kostru v přesné verzi Arduino-ESP32 core a knihoven.
- [ ] Uložit verze nástrojů a reprodukovatelný postup sestavení.

## Etapa 3 — měření a datová logika
- [x] Základní validace 4–20 mA, detekce proudu mimo rozsah a čtení ADC s průměrováním (test na fyzickém čidle stále čeká).
- [x] Oddělit datový záznam elektrické kalibrace senzoru od mapování hladina→objem.
- [ ] Dolní nádrž: objem = hladina_cm × 61,6 l.
- [x] Studna: kalibrační body, interpolace a zřetelné rozlišení naměřených bodů od geometrického odhadu; rozměry jsou nastavitelné a ukládají se odděleně od křivky.
- [x] Studna: základní webové workflow lokální překalibrace se zachováním bodů mimo měřený interval.
- [ ] Doplnit plné kalibrační workflow s potvrzením podmínek měření, přehledem existujících bodů a zobrazením důvodu odmítnutí.
- [x] Denní přítok: pouze kladné změny objemu, 30denní historie, NVS persistence a korektní chování bez validního času (UI historie a fyzické ověření ještě chybí).

## Etapa 4 — komunikace a bezpečnost
- [x] Jednosměrný status heartbeat ESP32 #2 → ESP32 #1 s 6s timeoutem implementován.
- [ ] Obousměrná komunikace, řídicí požadavky/ACK a bezpečnost při ztrátě peeru.
- [ ] Lokální bezpečnost ESP32 #2 funkční bez hlavní jednotky.
- [ ] Kritická hladina čerpadla oddělena od minimální hladiny transferu.
- [ ] Nouzový režim pouze po explicitním potvrzení uživatelem.
- [ ] Výstupy zatím testovat s odpojenými výkonovými zařízeními.

## Etapa 5 — transfery a web
- [ ] Ručně požadovaný transfer podle litrů a směru.
- [ ] Cílová kapacita dolní nádrže a limity zdroje ověřeny před spuštěním.
- [ ] Ukončení podle hladiny/objemu, timeout pouze jako záložní ochrana.
- [x] STOP, chyby a důvody ukončení v simulačním automatu; posledních 12 výsledků simulace je od 0.31 ukládáno do NVS.
- [ ] Provozní historie skutečných transferů až po implementaci a ověření fyzického řízení.
- [ ] Celé webové rozhraní dostupné přes AP bez internetu (základní stránka nastavení už existuje; transfery a dashboard chybí).

## Etapa 6 — OTA a vzdálený přístup
- [x] Základní OTA upload pro ESP32 #1 s kontrolou zápisu a zachováním NVS je napsán; překlad a test zařízení čekají.
- [ ] OTA ESP32 #2, bezpečný plán aktualizace obou jednotek a ověřená obnova/rollback podle možností platformy.
- [x] Zvolit architekturu vzdáleného přístupu přes VPN na domácím routeru; návrh a podmínky jsou v `REMOTE_ACCESS_PLAN.md`.
- [ ] Ověřit konkrétní HW verzi/firmware TP-Link Archer AX12, WAN topologii a veřejnou IP/CGNAT.
- [ ] Nakonfigurovat a otestovat WireGuard VPN z mobilních dat do domácí LAN; ESP32 nevystavovat přímo internetu.
- [ ] Zajistit DHCP rezervaci ESP32 #1 a ověřit dosažitelnost hlavního webu přes VPN.
- [ ] Vyřešit bezpečné předání/změnu přihlašovacích údajů a odstranit citlivá hesla z diagnostických výpisů před ostrým vzdáleným provozem.

## Etapa 7 — ověření a předání
- [ ] Překlad obou firmware v Arduino IDE 1.8.19.
- [ ] Testovací matice dokončena a výsledky zaznamenány.
- [ ] Testy s odpojenými snímači, výpadkem komunikace, restartem a chybami transferu.
- [ ] Postup instalace, záloha, OTA a návratu na předchozí firmware.
- [ ] Každá funkce označena podle skutečné úrovně ověření.

## Akceptace
Za hotovo lze označit pouze funkce, které odpovídají MASTER 1.3 a mají doložený test. Funkce závislé na skutečném zapojení se neoznačí jako ověřené pouze na základě statické kontroly kódu.


## Kontrolní stav po revizi 0.12
- Nativní sada: 12/12 testovacích programů prochází; oba sketch soubory také prošly hostitelskou syntaktickou kontrolou proti dočasným stubům.
- Základní web a Wi-Fi/AP existují; retry po ztrátě Wi-Fi doplněno v 0.12.
- Řídicí komunikace ESP32 #1 ↔ #2, plné ovládání transferů, bezpečné řízení výstupů, OTA ESP32 #2 a fyzické ověření zůstávají otevřené. Základní dashboard a historie přítoku už jsou implementovány.
- Arduino-ESP32 překlad a hardware test nejsou doloženy; podle akceptačních kritérií projekt není finální.


## Kontrolní stav po revizi 0.18
- Transferový automat má dodatečnou kontrolu fyzického rozsahu hladiny dolní nádrže i během transferu.
- Další větší etapa: lokální ochrana čerpadla horní nádrže s vlastními bezpečnostními mezemi a uloženým blokováním; limity horní nádrže nejsou v zadání potvrzené, proto se nesmí aktivovat podle odhadnutých hodnot.


## Kontrolní stav po revizi 0.19
- ESP32 #2 nyní lokálně vyhodnocuje stejnou fail-closed logiku ochrany čerpadla jako ESP32 #1 a používá oddělený NVS namespace `hwc_upperprot`, aby se ochranné záznamy jednotek nemohly přepisovat.
- Horní nádrž nemá potvrzenou využitelnou hloubku ani trip/recovery hladiny. Konfigurace je proto úmyslně neplatná a povolení čerpadla zůstává blokováno; relé GPIO není aktivováno.
- Stále chybí uživatelské nastavení ochranných mezí horní nádrže, integrace teplotního čidla, transferové UI a obousměrná řídicí komunikace.


## Kontrolní stav po revizi 0.31
- ESP32 #1 ukládá posledních 12 terminálních výsledků SIMULACE transferu do NVS; dashboard zobrazuje posledních pět. Záznam nese ID, směr, stav, důvod, požadovaný objem, odhad přírůstku a uptime při dokončení.
- NVS záznam je verzovaný, chráněný CRC32 a poškozený existující záznam se automaticky nepřepisuje.
- Nativní sada 20/20 prošla; ASan/UBSan celá sada; společné moduly synchronizovány. Skutečný Arduino build a hardware nebyly ověřeny.


## Kontrolní stav po revizi 0.32
- Opraveno opakování NVS zápisu a pozdější obnovení historie simulace.
- Hlavní nedokončené etapy zůstávají: autentizovaná obousměrná komunikace ESP32, synchronizace horních limitů, ACK/REJECTED/FAILED, skutečný transferový automat bez fyzických výstupů, kompletní diagnostika a fyzické ověření.


## Kontrolní stav po revizi 0.33
- Ochrana čerpadla studny nyní kontroluje i maximální fyzickou hloubku; regresní test ověřuje přetečení rozsahu a omezení nouzového režimu.
- Další priorita: návrh autentizovaného obousměrného protokolu a ACK/REJECTED/FAILED bez použití samotného CRC jako autentizace.


## Kontrolní stav po revizi 0.34
- Přidán přenositelný `PeerCommandGuard` pro odmítnutí neautentizovaných požadavků (výsledek externího ověřovače), nesprávného cílového boot ID, nulových identifikátorů, opakovaných sekvencí a nedávno použitých request ID.
- Modul je zatím samostatný a není napojen na UDP ani transfery. Samotný CRC ani parametr `authenticated` nejsou kryptografická autentizace; před integrací musí vzniknout a být otestována skutečná HMAC/verifikační vrstva a bezpečná správa klíče.


## Kontrolní stav po revizi 0.35
- Přidán a RFC 4231 vektory ověřen HMAC-SHA-256 primitivní modul.
- [ ] Definovat kanonický wire formát řídicích rámců a přesný rozsah autentizovaných polí.
- [ ] Navrhnout provisioning/rotaci tajemství a bezpečnou vazbu klíče na identitu obou jednotek.
- [ ] Integrovat ověřování před `PeerCommandGuard`; přidat autentizované ACK/REJECTED/FAILED a testy chyb/timeoutů/replay.
- [ ] Až poté uvažovat o síťovém řízení transferů; fyzické výstupy zůstávají poslední etapou.


## Kontrolní stav po revizi 0.36
- Kanonický HMAC rámec je definován a nativně testován. Další blokující etapa: bezpečné spárování/provisioning tajemství, autentizovaný handshake a teprve poté integrace request/ACK do UDP.


## Kontrolní stav po revizi 0.37
- Zpřísněn `PeerCommandGuard`: zařízení musí mít očekávanou identitu a příkazy jsou přijímány pouze v explicitně ustavené peer session s připnutým boot ID peeru. Automatické přijetí libovolného nového sender boot ID bylo odstraněno.
- Další blokující krok je skutečný fresh challenge-response handshake s HMAC, definovanými nonce a ochranou proti replay; teprve po něm smí síťová vrstva volat `recordAuthenticatedPeerSession()`.


## Kontrolní stav po revizi 0.38
- [x] Přidán a nativně otestován transportně nezávislý vzájemný HMAC challenge-response handshake s nonce obou stran, vazbou na device ID/boot ID, FINISH potvrzením a odvozením session key.
- [ ] Implementovat bezpečný provisioning a ochranu společného tajemství, volání `esp_random()` pro nonce/boot ID, timeout pending handshake a wire encoding.
- [ ] Napojit dokončený handshake na `PeerCommandGuard` a HMAC rámce; teprve pak implementovat UDP request/ACK/REJECTED/FAILED.


## Kontrolní stav po revizi 0.39
- [x] Definováno kanonické wire kódování HELLO/CHALLENGE/FINISH s pevnou délkou a explicitním little-endian zápisem.
- [x] Nativně otestována kompletní kryptografická výměna po zakódování a dekódování zpráv.
- [ ] Doplnit bezpečný provisioning společného tajemství a určit reálný model jeho ochrany na ESP32 (NVS encryption/flash encryption nebo fyzický provisioning); prosté NVS bez ochrany nepovažovat za bezpečné.
- [ ] Implementovat síťový stavový automat s timeouty, omezením počtu pokusů a RNG/boot ID z `esp_random()`.
- [ ] Po úspěšném FINISH explicitně ustavit `PeerCommandGuard`; následně integrovat autentizované request/ACK/REJECTED/FAILED a persistentní ochranu proti replay.


## Kontrolní stav po revizi 0.40
- [x] Přidat fail-closed kontrakt pro úložiště sdíleného peer tajemství a test odmítnutí nechráněného backendu.
- [ ] Implementovat konkrétní ESP32 backend pouze po ověření a zapnutí ESP-IDF NVS encryption/flash encryption v cílové partition konfiguraci; nesmí existovat fallback na plaintext.
- [ ] Definovat fyzický provisioning obou zařízení, párovací identitu, bezpečnou rotaci a postup obnovy/ztráty klíče.
- [ ] Teprve po ověření backendu napojit `esp_random()`, timeouty handshake, `PeerCommandGuard` a síťové ACK/REJECTED/FAILED.


## Doplnění po revizi 0.41
- [x] Definovat transportně nezávislé odpovědi ACK/REJECTED/FAILED a korelaci s původním příkazem.
- [ ] Doplnit autentizovaný UDP transport, timeouty, opakování a persistentní ochranu proti replay.
- [ ] Dokončit bezpečný provisioning a konkrétní ESP32 backend chráněného tajemství.
- [ ] Teprve po splnění předchozích bodů propojit příkazy a odpovědi s transferovým automatem; fyzické výstupy až po samostatné revizi bezpečnosti.


## Kontrolní stav po revizi 0.42
- [x] Přidán wrap-safe timeout pending handshake a časově omezený responder wrapper; timeout a hranice FINISH jsou nativně otestovány.
- [ ] Napojit timeout wrapper do síťové UDP/W5500 stavové smyčky; zatím nepovolovat command session.
- [ ] Implementovat konkrétní chráněné úložiště a fyzický provisioning společného tajemství.
- [ ] Generovat boot ID a nonce na ESP32 pomocí `esp_random()` a ověřit dostupnost/semantiku v konkrétním Arduino-ESP32 core.
- [ ] Integrovat kompletní HELLO/CHALLENGE/FINISH přes W5500, následně ustavit `PeerCommandGuard` a teprve poté příkazy/odpovědi.


## Kontrolní stav po revizi 0.43
- [x] Odvodit samostatné klíče pro oba směry z ověřeného session key s doménovým oddělením a vazbou na identity/boot ID.
- [ ] Napojit směrové klíče do HMAC command/response rámců a doplnit testy, že rámec podepsaný klíčem opačného směru je odmítnut.
- [ ] Dokončit konkrétní bezpečný provisioning a chráněné uložení dlouhodobého tajemství; bez ověřené ochrany flash nepoužívat síťové příkazy.
- [ ] Implementovat HELLO/CHALLENGE/FINISH přes W5500, volání `esp_random()`, limity pokusů a explicitní ustavení `PeerCommandGuard`.
- [ ] Teprve poté připojit request/ACK/REJECTED/FAILED k transferovému automatu; fyzické výstupy až po samostatné bezpečnostní a hardwarové akceptaci.


## Doplnění po revizi 0.44
- [x] Použít oddělené směrové HMAC klíče přímo v session-bound command/response codec a svázat rámce s ID/boot ID obou peerů.
- [ ] Napojit codec až po úspěšném handshake a explicitním ustavení `PeerCommandGuard`; při timeoutu, restartu nebo změně boot ID zahodit codec i session klíče.
- [ ] Dokončit konkrétní ESP32 chráněné úložiště/provisioning, persistentní anti-replay a UDP/W5500 handshake/request/response stavový automat.
- [ ] Před fyzickým řízením transferu dokončit lokální fail-safe výstupy, ztrátu komunikace, ACK vs. potvrzení průtoku a nezávislé nouzové ochrany všech čerpadel.


## Kontrolní stav po revizi 0.45
- [x] Propojit session-bound HMAC codec s command admission guardem; ověřit HMAC/identity/boot ID/typ COMMAND před předáním příkazu.
- [x] Při ukončení session vymazat interní směrové klíče; zachovat krátkou request-ID cache přes reconnect v rámci stejného běhu zařízení.
- [x] Navrhnout a nativně otestovat persistentní sekvenční anti-replay přes atomické úložiště; [ ] doplnit skutečný ESP32 backend a ověřit power-loss chování.
- [ ] Implementovat request-response korelaci ACK/REJECTED/FAILED a timeout/retry politiku; ACK nesmí být považováno za potvrzení průtoku.
- [ ] Napojit handshake, session pipeline a odpovědi do UDP/W5500 stavového automatu až po bezpečném secret backendu a provisioningu.


## Kontrolní stav po revizi 0.46
- Korelační tabulka odchozích příkazů a timeout odpovědí je implementována a nativně otestována; síťové napojení, persistentní stav, retry politika a integrace do transferu zůstávají otevřené.


## Kontrolní stav po revizi 0.47
- [x] Propojit `PeerCommandSession` s `PeerRequestTracker`: odchozí požadavek je zaregistrován před předáním rámce transportu; příchozí odpověď se nejprve autentizuje a teprve poté koreluje.
- [x] Omezit opakované použití posledních 16 request ID v RAM a zachovat tuto historii při reconnectu stejného běhu.
- [ ] Persistentní anti-replay a bezpečné generování unikátních request ID po restartu.
- [ ] Implementovat retry/timeout politiku v síťové vrstvě tak, aby opakování nepřineslo duplicitní fyzický transfer.
- [ ] Napojit handshake + command session + response korelaci na W5500/UDP a teprve následně na transferový automat.


## Kontrolní stav po revizi 0.49
- [x] Implementovat `NvsPeerReplayStore` jako dvouslotový Preferences/NVS žurnál s generačním čítačem, CRC a ověřením zápisu.
- [x] Otestovat obnovu z předchozího platného slotu při poškození/selhání zápisu a propojení s `PersistentPeerReplayGuard` po restartu přijímače.
- [ ] Ověřit NVS adaptér skutečným výpadkem napájení na cílové desce a přesnou verzí Arduino-ESP32 core.
- [ ] Doplnit sekvenční allocator odesílatele a autentizovanou synchronizaci `persistentReplayFloor()` v handshake/session establishment.
- [ ] Teprve poté aktivovat persistentní replay ochranu ve W5500 command transportu.

## Kontrolní stav po revizi 0.48
- [x] Navrhnout persistentní anti-replay checkpoint vázaný na identitu peeru a jeho boot ID.
- [x] Rezervovat sekvenční rozsahy předem a odmítnout příkaz, pokud atomický zápis rezervace selže.
- [x] Integrovat persistentní guard do přijímací cesty `PeerCommandSession` za HMAC ověření a před předání COMMAND aplikaci.
- [x] Otestovat restart přijímače, skip rezervovaného rozsahu, poškozený checkpoint, chybu úložiště, změnu peer boot ID a ASan/UBSan.
- [ ] Implementovat skutečný ESP32 `IPeerReplayStore` s ověřenou atomickou persistencí; hostitelský fake není důkazem chování NVS při výpadku napájení.
- [ ] Doplnit monotónní sekvenční čítač odesílatele, jeho vazbu na boot ID a síťovou orchestrace W5500.


## Kontrolní stav po revizi 0.50
- [x] Přidat monotónní odesílací sekvenční allocator a automatické přidělení sequence při přípravě COMMAND požadavku.
- [x] Přidat lokální API pro posun odesílatele nad persistentní floor přijímače; nativně otestovat restart přijímače a sekvenci nad rezervovaným rozsahem.
- [ ] Rozšířit autentizovaný session-sync/handshake o přenos persistentního floor. Volat `synchronizeOutgoingSequenceFloor()` pouze po ověření HMAC a vazby na aktuální peer boot ID.
- [ ] Doplnit obousměrnou W5500 UDP orchestraci HELLO/CHALLENGE/FINISH, COMMAND/RESPONSE, timeouty a bezpečné opakování stejného requestu se stejnou sekvencí a request ID.
- [ ] Propojit přijaté a potvrzené transferové příkazy s `TransferLogic`; výstupy zůstanou neaktivní do samostatné hardwarové bezpečnostní akceptace.


## Kontrolní stav po revizi 0.51
- [x] Přenést persistentní replay floor v CHALLENGE a zahrnout jej do HMAC transcriptu/session key derivation.
- [x] Na straně responderu načíst floor z persistentního úložiště podle peer identity a boot ID; chybný NVS záznam odmítnout.
- [x] Změnit handshake wire verzi na 2 a CHALLENGE na 84 B; testovat round-trip a odmítnutí manipulace.
- [x] Ověřit návaznost ověřeného floor na odesílací allocator (128 → 129).
- [ ] Implementovat W5500 UDP handshake orchestrátor, který po ověření CHALLENGE synchronizuje `PeerCommandSession` a teprve poté povolí COMMAND.
- [ ] Bezpečně provisionovat a uchovávat sdílené tajemství; dodat boot ID a nonce z `esp_random()` na ESP32.

## Kontrolní stav po revizi 0.52
- [x] Doplnit čtvrtý handshake krok READY, aby iniciátor získal autentizované potvrzení přijetí FINISH responderem.
- [x] READY ověřovat HMAC-SHA-256 se samostatnou doménou a session key; přidat pevné wire kódování a testy manipulace.
- [ ] Implementovat transportní orchestrátor W5500 UDP HELLO/CHALLENGE/FINISH/READY včetně timeoutů, opakování a rušení session.
- [ ] Teprve po READY aktivovat `PeerCommandSession`, aplikovat autentizovaný replay floor a zpřístupnit COMMAND/RESPONSE.
- [ ] Dokončit bezpečný provisioning/chráněné uložení sdíleného tajemství a `esp_random()` pro boot ID/nonce.


## Kontrolní stav po revizi 0.53
- [x] Implementovat transportně nezávislý stavový koordinátor handshake iniciátora.
- [x] Ověřit CHALLENGE před aplikací autentizovaného replay floor na odesílací sekvenční allocator.
- [x] Zachovat přesný FINISH pro retry a nepovolit session key před ověřením READY.
- [ ] Doplnit odpovídající responder orchestrace v síťové smyčce a propojit obě strany přes UDP/W5500.
- [ ] Napojit `esp_random()` pro boot ID/nonce a dokončit bezpečný provisioning i chráněné úložiště dlouhodobého tajemství.
- [ ] Po úspěšném READY automaticky sestavit directional keys a aktivovat `PeerCommandSession`; teprve potom povolit COMMAND/RESPONSE.


## Kontrolní stav po revizi 0.54
- [x] Přidat transportně nezávislý koordinátor responderu s replay-store-backed CHALLENGE, ověřením FINISH, cache READY pro retry a časovým omezením.
- [ ] Doplnit síťový orchestrátor, který bude bezpečně řídit čekání na skutečné potvrzení protistrany; samotné předání READY UDP socketu není důkazem, že jej iniciátor přijal.

## Kontrolní stav po revizi 0.55
- [x] Přidat bounded retry policy pro neměnné handshake datagramy, včetně wrap-safe časování a limitu pokusů.
- [ ] Navázat retry policy na UDP/W5500 transport: HELLO, CHALLENGE, FINISH a READY se opakují jako přesně stejné serializované bajty; každá fáze má svůj limit a podléhá celkovému handshake timeoutu.
- [ ] Ošetřit ztracené/duplicitní HELLO a FINISH tak, aby opakovaný datagram nevyvolal nové nonce, nový challenge ani opakované přijetí stejného FINISH.
- [ ] Přidat omezení četnosti nových handshake pokusů a vazbu na očekávanou IP/port/identitu peeru; nepovažovat CRC ani UDP adresu za autentizaci.
- [ ] Teprve po kompletním READY a synchronizaci replay floor sestavit directional keys a aktivovat `PeerCommandSession`.
- [ ] Před povolením síťových příkazů dokončit bezpečný secret provisioning/úložiště a `esp_random()` pro boot ID i nonce.


## Kontrolní stav po revizi 0.57
- [x] Přidat časovaný iniciátor handshake s bounded retry pro HELLO a FINISH.
- [x] Ověřit přesné opakování datagramů, přetečení `millis()`, timeout, vyčerpání pokusů a fail-closed chování při neplatném READY.
- [ ] Navrhnout a implementovat konkrétní chráněný secret backend a bezpečný provisioning obou jednotek.
- [ ] Integrovat generování boot ID/nonces pomocí `esp_random()` a ověřit konkrétní Arduino-ESP32 core.
- [ ] Teprve poté připojit HELLO/CHALLENGE/FINISH/READY a autentizované COMMAND/RESPONSE do UDP/W5500 stavové smyčky.


## Platformní brána po revizi 0.64 — NVS Encryption
- [x] Ověřit oficiální Arduino-ESP32 Windows 7 omezení a API `Preferences`/NVS inicializace pro core 1.0.6 a 2.0.17.
- [x] Zaznamenat, že `Preferences` samo nezapíná NVS Encryption a že bezpečnost závisí na inicializaci default NVS, build konfiguraci a chráněné key partition.
- [ ] Zjistit přesnou instalovanou verzi Arduino-ESP32 core v uživatelově Arduino IDE 1.8.19.
- [ ] Podle skutečné verze ověřit dostupné `nvs_flash_secure_init()` API, Flash Encryption bootloader a partition table na testovací desce.
- [ ] Až po úspěšném testu navrhnout produkční `PeerSecretBackend`; nikdy nepřijmout plaintext fallback.
- [ ] Pokud stávající Arduino workflow neumožní bezpečnou inicializaci, samostatně posoudit externí secure element a změnu HMAC rozhraní.


## Doplňující dokončená práce — revize 0.65
- Ochrana transferového automatu proti duplicitnímu ID se nyní obnovuje z posledních 12 persistentních výsledků po restartu ESP32 #1.
- Pokud se NVS zpřístupní až po startu, obnovená historie se sloučí s ID již přijatými v RAM.
- Oprava je pouze pro simulační automat; skutečné transfery zůstávají blokované do samostatné implementace a hardwarového ověření.

## Kontrolní stav po revizi 0.66
- [x] Přidat trvalý restart journal transferu se stavy PREPARED/ACTIVE/INTERRUPTED/RESULT_PENDING.
- [x] Před přijetím simulačního požadavku uložit PREPARED; před přechodem do RUNNING uložit ACTIVE.
- [x] Po restartu nikdy automaticky neobnovit transfer; vyžadovat explicitní potvrzení obsluhy.
- [x] Výsledek uložit do historie před uvolněním journalu; při chybě NVS zůstat fail-closed.
- [ ] Ověřit dvouslotový journal fyzickým power-cut testem na cílové desce a přesné verzi Arduino-ESP32 core.
- [ ] Před jakýmkoli fyzickým řízením prokázat, že každý výstupní zápis je podmíněn platným `canOpenValve()` a nezávislými hardwarovými ochranami.
- Nativní testy: 45/45; kompletní ASan/UBSan: 45/45. Fyzické výstupy zůstávají zakázané.


## Kontrolní stav po revizi 0.67
- [x] Ověřit NVS namespace transfer journalu a jeho limit délky.
- [x] Ověřit zápis do poškozeného slotu při zachování posledního platného fallback záznamu.
- [x] Ověřit generační čítač při přetečení `uint32_t`.
- [ ] Provést skutečný power-cut test na cílovém ESP32 a ověřit chování NVS při přerušení napájení během zápisu.
- Nativní testy: 45/45; ASan/UBSan: 45/45. Firmware se v této revizi nemění.


## Dodatek po revizi 0.68 — transferový automat

- Přenositelný `TransferLogic` pro SIMULACI má předběžnou kontrolu, běh, STOP, timeout, dosažení cíle, důvody ukončení a bezpečný příkaz `VALVES_CLOSE_ALL` v koncových stavech.
- `TransferRestartGuard` doplňuje persistentní PREPARED/ACTIVE/INTERRUPTED/RESULT_PENDING a blokaci po restartu.
- Revize 0.68 doplnila jednoznačnou reakci na ztrátu peer dat u zdroje horní nádrže: COMMUNICATION a abort.
- Stále chybí autentizované síťové COMMAND/RESPONSE/ACK/REJECTED/FAILED propojení a jeho integrace do aplikačního automatu. To zůstává blokováno P0 bezpečnostními kroky.
- Fyzické výstupy zůstávají zakázané; stavové příkazy automatu nejsou připojeny na GPIO.

## Aktuální kontrolní snímek po revizi 0.69 — podle MASTER 1.3

Starší etapy výše zachovávají historii původního plánu; pro aktuální stav platí tento souhrn:

- [x] Dolní nádrž: matematika 61,6 l/cm a horní limit 121 cm implementovány a nativně testovány. Upřesnění uživatele REV26: 11 cm je pouze informativní údaj a nikdy se neuplatňuje jako provozní blokace. Ochranu čerpadla řídí výhradně uživatelsky nastavitelné minimum a hystereze.
- [x] Studna: geometrie, kalibrační křivka, interpolace, lokální merge a NVS adaptéry implementovány; hostitelsky testováno.
- [x] Denní přítok: kladné změny, nedatovaný součet a 30denní historie implementovány; fyzická měření a reálný čas neověřeny.
- [x] Lokální ochranné logiky studny a horní nádrže implementovány a nativně testovány; fyzická relé nejsou připojena.
- [x] Transferový automat, STOP, timeout, historie simulací, persistentní ID a restart journal implementovány pro SIMULACI.
- [x] Ztráta peer dat při transferu z horní nádrže je v simulaci klasifikována jako COMMUNICATION a končí abortem s příkazem `VALVES_CLOSE_ALL`.
- [x] Peer status ESP32 #2→#1 přes W5500/UDP existuje; boot ID je od 0.69 stabilní přes reconnect a mění se jen po restartu.
- [ ] Autentizovaná handshake/COMMAND/RESPONSE orchestrace přes W5500 není implementována.
- [ ] Produkční chráněné úložiště peer tajemství a bezpečný provisioning nejsou hotové.
- [ ] Bezpečné prvotní získání/obnova hesel bez výpisu tajemství do Serial Monitoru není hotové.
- [ ] Horní nádrž: objemová křivka a teplota nejsou dokončeny.
- [ ] OTA koordinace obou ESP32, partition/rollback a cílové ověření nejsou dokončeny.
- [ ] Arduino-ESP32 build, fyzické NVS/power-cut a testy desek nebyly provedeny.
- [ ] Fyzické řízení ventilů a čerpadel zůstává zakázáno.

Aktuální podrobná matice: `MASTER_ALIGNMENT_AUDIT_0.69.md`. Nativní sada 45/45 a kompletní ASan/UBSan 45/45.

## Navazující krok po revizi 0.72
- [x] Doplnit fyzickou detekci `LinkOFF` na obou W5500 peer jednotkách.
- [x] Po obnovení linky zahájit DHCP reconnect bez dlouhého pevného čekání.
- [x] Opravit sanitizer runner a zahrnout statickou kontrolu link recovery.
- [ ] Ověřit přesnou verzi Arduino Ethernet Library a provést skutečný Arduino-ESP32 build na cílovém prostředí.
- [ ] Na stole otestovat odpojení/připojení kabelu, výpadek DHCP, restart routeru/switche a návrat peer statusu; zatím pouze bez fyzických výstupů.
- [ ] Poté pokračovat v P0 bezpečnostním auditu: chráněný provisioning tajemství, bezpečné zacházení s hesly, handshake orchestrace a persistentní anti-replay.


## Navazující krok po revizi 0.73 — zbývající P0
- [x] Zabránit rollbacku replay floor při poškození kteréhokoli NVS slotu; fail-closed místo fallbacku.
- [x] Při neúspěšném backend write tajemství zkusit zneplatnit i případný částečný záznam.
- [ ] Ověřit konkrétní Arduino-ESP32 core a Arduino Ethernet Library 2.0.0+ a provést skutečný překlad.
- [ ] Na testovací desce ověřit NVS journal při skutečném power-cut; nesmí být zaměněno za hostitelský stub test.
- [ ] Navrhnout a ověřit produkční chráněné úložiště tajemství a fyzický provisioning/rotaci.
- [ ] Integrovat a ověřit `esp_random()` boot ID/nonce v přesném targetu.
- [ ] Definovat explicitní servisní obnovu korupčního replay journalu bez automatického vymazání.
- [ ] Teprve po uzavření všech P0 propojit handshake a command orchestrace s W5500; fyzické výstupy zůstanou odpojené do samostatné HW validace.

## Dodatečný P0 nález po revizi 0.73 — úplná ztráta replay journalu
- [ ] P0-3: rozlišit legitimní první inicializaci od ztráty obou NVS replay slotů pomocí inicializačního markeru a fail-closed obnovy.
- [ ] Otestovat pořadí zápisů marker/checkpoint při výpadku napájení; marker nesmí způsobit falešné přijetí starého replay floor.
- [ ] Výslovně zachovat omezení: marker v běžné NVS neřeší smazání celé partition ani rollback celé flash; tento threat model vyžaduje samostatnou hardwarovou ochranu.
- [ ] Do uzavření P0-3 ponechat peer command path default-deny.


## Stav P0-3 po revizi 0.75
- [x] Doplnit NVS marker zapisovaný až po ověřeném prvním checkpointu.
- [x] Pokud marker existuje a oba sloty chybí, odmítnout stav jako CORRUPT.
- [x] Otestovat úplnou ztrátu obou slotů v nativním stubu.
- [ ] Otestovat skutečný power-cut na cílovém ESP32 včetně přerušení mezi checkpointem a markerem.
- [ ] Vyřešit threat model smazání celé partition / rollbacku celé flash hardwarově; běžný NVS marker tuto ochranu neposkytuje.
- [ ] Ponechat peer command path default-deny do uzavření všech P0.

## Dodatečné P0 nálezy po revizi 0.75 — vystavení přihlašovacích údajů
- [ ] P0-4: odstranit zobrazení `apPassword` v běžné HTML stránce ESP32 #1 až po zavedení bezpečného obnovovacího/provisioning workflow.
- [ ] P0-5: odstranit výpis AP a webového administrátorského hesla do Serial Monitoru na ESP32 #1.
- [ ] P0-6: odstranit výpis AP a webového administrátorského hesla do Serial Monitoru na ESP32 #2.
- [ ] Před odstraněním výpisů implementovat a otestovat bezpečné prvotní předání, obnovu a změnu přístupových údajů, aby uživatel neztratil přístup.
- [ ] Zachovat default-deny peer command path a neaktivní fyzické výstupy.

Podrobnosti: `SECURITY_CREDENTIAL_EXPOSURE_AUDIT_0.76.md`.

## Navazující krok po revizi 0.76 — návrh bezpečného přístupu (0.77)
- [x] Popsat životní cyklus prvotního nastavení, změny a obnovy AP/admin hesel pro obě jednotky.
- [x] Stanovit zákaz výpisu hesel do Serial Monitoru a jejich zobrazení v dashboardu; hesla se nikdy znovu nečtou/zobrazují uživateli.
- [x] Stanovit fail-closed chování při chybě NVS a regresní testovací matici.
- [ ] Ověřit fyzický servisní vstup na konkrétních deskách ESP32 #1 a #2; nepřidělovat GPIO bez ověření.
- [ ] Implementovat provisioning/obnovu až po ověření fyzického vstupu a bezpečné servisní cesty.
- [ ] Poté odstranit všechny výpisy AP/admin hesel z obou sketchů a odstranit `apPassword` z dashboardu.
- [ ] Prokázat ochranu uložených hesel konkrétní NVS Encryption + Flash Encryption konfigurací na testovací desce.

## Navazující krok po revizi 0.77 — audit NVS přístupových údajů (0.78)
- [x] Prověřit, zda obě jednotky kontrolují úspěch zápisu AP/admin údajů.
- [x] Identifikovat start AP s heslem pouze v RAM a ignorované návratové hodnoty `putString()`.
- [x] Definovat fail-closed cílové chování a testy write/read-back/mismatch.
- [ ] Implementovat přenositelný bezpečný NVS credential helper s ověřením zpětného načtení.
- [ ] Zapojit helper do obou sketchů; odstranit výpisy hesel ze Serial a AP heslo z dashboardu.
- [ ] Ověřit servisní vstup na konkrétních deskách; GPIO nepřidělovat odhadem.
- [ ] Prokázat NVS Encryption + Flash Encryption na cílovém hardware.
- Peer command path zůstává default-deny; fyzické výstupy neaktivní.

## Navazující krok po revizi 0.78 — integrační brána přístupových údajů (0.79)
- [x] Ověřit, zda balíček obsahuje skutečné AP/Wi-Fi/web call-sites obou jednotek.
- [x] Zastavit implementaci izolovaného helperu, který by nebyl zapojen do startu AP a jehož testy by neprokazovaly fail-closed chování aplikace.
- [x] Definovat společné rozhraní CredentialStore, stavové výsledky a povinnou testovací matici.
- [ ] Získat/obnovit skutečné síťové sketch soubory ESP32 #1 a ESP32 #2.
- [ ] Implementovat společný CredentialStore a napojit jej na skutečný provisioning i start AP.
- [ ] Odstranit výpisy AP/admin hesel ze Serial a zobrazení AP hesla z dashboardu až po ověření bezpečné obnovy.
- [ ] Ověřit servisní vstup a NVS/Flash Encryption na přesných cílových deskách.
- Peer command path zůstává default-deny; fyzické výstupy neaktivní.

## Navazující revize 0.80 — oprava integračního auditu credentials
- [x] Ověřit skutečné AP/Wi-Fi/web call-sites v obou hlavních sketch souborech.
- [x] Opravit chybný závěr revize 0.79, že síťové sketch call-sites chybí; revize 0.79 je v tomto bodě superseded.
- [x] Zmapovat přesná místa načítání, generování, ukládání, zobrazování a logování credentials na ESP32 #1 a #2.
- [x] Potvrdit HTTP Basic Auth na HTTP portu 80 jako nešifrovaný transport; webové rozhraní nesmí být považováno za bezpečné proti odposlechu.
- [ ] Implementovat společný CredentialStore a napojit jej na skutečné call-sites obou jednotek; AP start pouze po validním read-back.
- [ ] Přidat integrační testy, které ověří, že AP/web server nezačne při NVS begin/write/read/mismatch chybě.
- [ ] Zavést fyzicky potvrzený provisioning/obnovu; GPIO až po ověření konkrétních desek.
- [ ] Odstranit výpisy tajemství do Serial a heslo AP z HTML ESP32 #1 až po ověření obnovovací cesty.
- [ ] Ověřit NVS/Flash Encryption a cílový Arduino build.
- Peer command path zůstává default-deny; fyzické výstupy neaktivní.

## Revize 0.81 — implementační brána CredentialStore
- [x] Přesně rozlišit `VALID`, `EMPTY`, `CORRUPT`, `IO_ERROR`; částečná sada credentials nesmí být považována za EMPTY.
- [x] Definovat fail-closed podmínky pro start AP/web na ESP32 #1 a #2.
- [x] Definovat bezpečnou migraci přes staging/read-back/commit se zachováním poslední ověřené sady.
- [x] Doplnit testovací matici pro NVS chyby a výpadek napájení v každém kroku.
- [ ] Vyřešit a ověřit fyzicky autorizovaný prvotní provisioning/obnovu pro skutečné desky.
- [ ] Implementovat společný CredentialStore a NVS adapter; nejprve hostitelské testy všech stavů a fault injection.
- [ ] Zapojit AP/web call-sites obou jednotek a integračně prokázat, že při nevalidním úložišti AP/web neběží.
- [ ] Odstranit výpisy/zobrazení tajemství až po ověření obnovy.
- [ ] Ověřit NVS/Flash Encryption a Arduino build na přesné cílové konfiguraci.
- Peer command path default-deny; fyzické výstupy neaktivní.

## Revize 0.82 — audit pořadí inicializace credentials

- [x] Znovu ověřit pořadí načítání/generování credentials a startu AP v obou skutečných `.ino` souborech.
- [x] Potvrdit, že ESP32 #1 může spustit AP s heslem pouze v RAM po chybě NVS/zápisu.
- [x] Potvrdit, že ESP32 #2 nekontroluje výsledky zápisu ani read-back před spuštěním AP a web serveru.
- [x] Popsat bezpečné pořadí migrace tak, aby `EMPTY` neznamenalo nefunkční prvotní nastavení a současně nedocházelo k automatickému neověřenému AP.
- [ ] Před implementací určit fyzicky autorizovaný provisioning/obnovu; GPIO nepřidělovat bez potvrzeného hardware.
- [ ] Implementovat hostitelsky testovaný CredentialStore se staging/commit a fault injection.
- [ ] Integrovat do ESP32 #1/#2; nevalidní NVS stav musí blokovat AP i web server.
- [ ] Po ověření provisioning workflow odstranit výpisy tajemství a AP heslo z HTML.
- [ ] Ověřit NVS/Flash Encryption na cílovém hardware/toolchainu.

## Revize 0.83 — přenositelné jádro CredentialStore
- [x] Přidat přenositelné rozhraní backendu a CredentialStore pro AP/admin credentials.
- [x] Implementovat dvouslotový staging/read-back/commit; aktivní slot zůstává nedotčen při chybě přípravy nového slotu.
- [x] Rozlišit chybějící klíč od I/O chyby a `EMPTY` od `CORRUPT`.
- [x] Přidat hostitelský fault-injection test.
- [ ] Implementovat skutečný Preferences/NVS adapter s přesnými návratovými kódy a ověřením chování `putString/getString/remove`.
- [ ] Integrovat CredentialStore do obou `.ino`; blokovat AP i web při `EMPTY/CORRUPT/IO_ERROR`, dokud neproběhne fyzicky autorizovaný provisioning.
- [ ] Ověřit provisioning/obnovu a konkrétní servisní vstup na cílových deskách.
- [ ] Odstranit Serial/HTML úniky až po ověření obnovovací cesty.
- [ ] Ověřit NVS/Flash Encryption a cílový Arduino build.

## Revize 0.84 — NVS backend credentials
- [x] Implementovat Preferences backend s odděleným namespace a přesnou kontrolou délky/ukončení hodnot.
- [x] Rozlišit MISSING od ERROR; kontrolovat výsledky put/get/remove.
- [x] Přidat host test pro NVS adapter a zapojit do standardní i ASan/UBSan sady.
- [ ] Přeložit proti přesné Arduino-ESP32 a Preferences verzi cílového prostředí.
- [ ] Integrovat adapter/CredentialStore do boot a AP call-sites obou `.ino` až po uzavření fyzicky autorizovaného provisioning/obnovovacího postupu.
- [ ] Ověřit NVS chování při skutečném power-cut a NVS/Flash Encryption.


## Revize 0.85 — CredentialStore commit fault handling
- [x] Při commit zápisu `cred_active` určit výsledek podle zpětného načtení, nikoli pouze podle návratové hodnoty `put()`.
- [x] Doplnit fault-injection test pro zápis, který se fyzicky provede, ale backend vrátí chybu.
- [x] Synchronizovat CredentialStore do obou ESP32 zdrojových stromů.
- [ ] Doplnit integrační AP gate a provisioning/obnovu; do té doby CredentialStore nepovažovat za produkčně zapojený.
- [ ] Ověřit přesný Arduino-ESP32/Preferences build a skutečné chování NVS na testovací desce.

## Další krok po revizi 0.86
- [x] Rozšířit hostitelské testy NVS credential adapteru o malformed payloady a hraniční délky.
- [ ] Uzavřít návrh fyzicky autorizovaného prvotního provisioningu a obnovy přístupu bez přiřazení neověřeného GPIO.
- [ ] Po uzavření provisioning workflow integrovat CredentialStore do startu AP/web obou jednotek a přidat integrační testy fail-closed chování.
- [ ] Teprve po ověření obnovy odstranit hesla ze Serial a dashboardu.
- [ ] Ověřit přesnou verzi Arduino-ESP32/Preferences a skutečnou NVS na testovací desce.

## Revize 0.87 — přenositelná politika fyzické servisní autorizace
- [x] Přidat politiku fyzického podržení 3 s a jednorázové autorizační okno 60 s.
- [x] Vyžadovat uvolnění fyzického vstupu před dalším autorizováním.
- [x] Ošetřit přetečení `millis()` unsigned časovou aritmetikou.
- [x] Přidat host test krátkého stisku, zákmitu, jednorázového použití, expirace a wrap-around.
- [x] Synchronizovat hlavičku do common a obou ESP32 stromů.
- [ ] Vybrat a ověřit fyzický servisní vstup podle konkrétní revize desek; nepřidělovat GPIO odhadem.
- [ ] Napsat deskový adapter, který jako jediný smí volat `samplePhysicalInput()`.
- [ ] Teprve po fyzickém ověření integrovat autorizaci do provisioning/obnovy credentials.
- [ ] Integrovat CredentialStore do AP/web call-sites a ověřit fail-closed boot obou jednotek.

## Navazující revize 0.89 — servisní autorizace
- [x] Otestovat podržení kratší než 3 s a přesné dosažení limitu.
- [x] Otestovat zákaz opakované autorizace při nepřetržitě aktivním vstupu.
- [x] Otestovat nové měření po fyzickém uvolnění vstupu.
- [x] Otestovat expiraci přesně na 60 s přes přetečení `millis()`.
- [ ] Ověřit skutečný typ/revizi desek a zvolit fyzický servisní vstup bez konfliktu s již použitými piny.
- [ ] Navrhnout a otestovat servisní transport pro provisioning; žádné tajemství nesmí být logováno.
- [ ] Po potvrzení hardwaru připojit fyzický adapter k autorizační politice a integračně ověřit jednorázové oprávnění.
- [ ] Teprve poté napojit CredentialStore na AP/web boot obou ESP32 a ověřit fail-closed chování.


## Revize 0.90 — provisioning workflow a obnova úložiště credentials
- [x] Přidat transportně nezávislý `CredentialProvisioningController`, který sám spotřebuje jednorázovou fyzickou autorizaci.
- [x] Omezit provisioning relaci na původní 60s deadline fyzické autorizace (nezačínat nové okno při openSession) a jeden pokus; po každém výsledku ji uzavřít.
- [x] Vynulovat předaný pevně dimenzovaný credential buffer na všech návratových cestách.
- [x] Přidat fyzicky autorizovanou obnovu `CORRUPT`: nejprve odstranit/ověřit selector, potom odstranit/ověřit sloty, následně ověřit `EMPTY`.
- [x] Otestovat orphan staging data, přerušení obnovy, chybu mazání, IO_ERROR, první provisioning a rotaci.
- [ ] Určit typ/revizi desek a ověřit vhodný fyzický servisní vstup bez konfliktu s boot/W5500/senzory.
- [ ] Implementovat deskový adapter, který jako jediný volá `samplePhysicalInput()`; žádný síťový/serial handler nesmí autorizační vstup simulovat.
- [ ] Vybrat fyzický servisní transport pro vstup nových hesel; preferovat přímé lokální servisní rozhraní, ne běžné HTTP přes LAN.
- [ ] Připojit controller do skutečného provisioning endpointu obou jednotek a otestovat, že endpoint neexistuje mimo otevřenou relaci.
- [ ] Napojit CredentialStore na boot/AP gate: AP a web mohou startovat pouze při `VALID`; `EMPTY/CORRUPT/IO_ERROR` zůstávají fail-closed, dokud nedojde k autorizovanému provisioningu.
- [ ] Teprve po fyzicky ověřené obnově odstranit tajemství ze Serial a HTML; přidat statické kontroly logů a odpovědí.
- [ ] Ověřit konkrétní Arduino-ESP32/Preferences build, skutečnou NVS a power-cut na testovací desce.
- [ ] Ověřit NVS Encryption + Flash Encryption na přesné cílové konfiguraci; eFuse/Release změny pouze po samostatném schválení.
- Peer command path zůstává default-deny; fyzické výstupy neaktivní.

## Revize 0.91 — lokální sériový transport provisioningu
- [x] Definovat binární rámec SET_CREDENTIALS s pevnými limity, verzí, délkou a CRC-16/CCITT-FALSE.
- [x] Implementovat pevně dimenzovaný parser bez `String`/heap a s 2s inter-byte timeoutem.
- [x] Zajistit, že serial wrapper pouze spotřebuje již otevřenou fyzickou relaci; nemůže sám volat ani vytvářet fyzickou autorizaci.
- [x] Chybný rámec fail-closed uzavře servisní relaci; validní rámec spotřebuje jeden controller attempt; odpovědi nikdy neobsahují credentials.
- [x] Doplnit host testy šumu, fragmentace, CRC, chybné verze/command, oversized payload, neplatné textové znaky, timeout včetně wrap-around, extra payload, max délku a absence fyzické relace.
- [x] Sjednotit AP heslo na 8–63 znaků v CredentialStore i serial protokolu kvůli kompatibilitě SoftAP.
- [ ] Ověřit konkrétní desky/konektor a určit, zda použít USB-UART/Serial0 nebo samostatný UART bez konfliktu s W5500, boot logy a servisním přístupem.
- [ ] Odstranit všechna hesla ze Serial Monitoru a HTML ESP32 #1; přidat statické kontroly proti regresi.
- [x] Připravit host klient bez echo/logování credentials a přidat hostitelské testy (revize 0.92); [ ] fyzicky ověřit přímý kabel až po integraci device-side adaptéru.
- [ ] Připojit serial service pouze v otevřené fyzické servisní relaci a otestovat skutečný GPIO adapter.
- [ ] Napojit CredentialStore na boot/AP gate obou jednotek a ověřit, že AP/web neběží při EMPTY/CORRUPT/IO_ERROR.
- [ ] Ověřit Arduino build, skutečnou NVS, výpadky napájení a platformní NVS/Flash Encryption.
- Peer command path zůstává default-deny; fyzické výstupy neaktivní.


## Revize 0.92 — hostitelský klient provisioningu
- [x] Implementovat klientské sestavení a validaci rámce SET_CREDENTIALS v1 podle device parseru.
- [x] Skryté interaktivní zadávání hesel; bez vypisování hodnot, frame dumpu nebo ukládání credentials do souboru.
- [x] Výchozí náhled bez odesílání; skutečný send blokovat bez explicitního potvrzení integrace firmware.
- [x] Otestovat CRC, limity polí, max. 170B rámec, response parser a fake serial exchange.
- [ ] Ověřit přesný hardware, fyzický servisní vstup a sériový port; žádný GPIO/UART pin nevybírat odhadem.
- [ ] Implementovat board adapter pro fyzické vzorkování vstupu, řízení servisní session a vlastnictví Serial linky; síťové handlery nesmí otevřít session.
- [ ] Integrovat Serial service do `.ino`, nastavit a ověřit baud rate, DTR/RTS/auto-reset, boot logy a buffer/timeout.
- [ ] Teprve po fyzickém otestování obnovy připojit CredentialStore k boot/AP gate a odstranit stávající AP/admin hesla ze Serial i AP heslo z HTML.
- [ ] Ověřit přesnou Arduino-ESP32 verzi, skutečné NVS, power-cut a NVS/Flash Encryption.
- Peer command path zůstává default-deny; fyzické výstupy neaktivní.


## Revize 0.93 — board adapter a READY handshake
- [x] Přidat obecný board adapter pro fyzický input + UART-like stream, bez výběru GPIO/UART pinů.
- [x] Před relací zahodit RX backlog a vyžadovat klidný poll před jejím otevřením.
- [x] Odeslat READY až po spotřebování fyzické autorizace; klient musí READY ověřit před zasláním credentials.
- [x] Omezit práci na poll: RX drain 256 B, servisní parsování 64 B.
- [x] Testovat stale RX, READY gate, fragmentaci, one-shot, timeout, TX failure a 600B flood.
- [x] Synchronizovat credential security moduly mezi common a oběma ESP32 src stromky.
- [ ] Ověřit přesný model/revizi desek, servisní vstup, elektrickou polaritu a boot-safe zapojení.
- [ ] Vybrat UART/USB rozhraní, baud rate, konektor a ověřit DTR/RTS/auto-reset.
- [ ] Vyřešit výhradní vlastnictví Serial linky; odstranit všechna hesla ze Serial/HTML a přidat statické regresní kontroly.
- [ ] Zapojit board adapter do `.ino` obou jednotek a provést Arduino-ESP32 build.
- [ ] Napojit CredentialStore na AP/web boot gate; AP a web smí běžet pouze při `VALID`; otestovat `EMPTY/CORRUPT/IO_ERROR` fail-closed.
- [ ] Ověřit fyzické provisionování, skutečnou NVS, power-cut a NVS/Flash Encryption.
- Peer příkazy zůstávají zakázané; fyzické výstupy zůstávají neaktivní.


## Bezpečnostní stav po revizi 0.94
- AP startup obou jednotek nyní čeká na validní a zpětně ověřené přístupové údaje ve stávajících NVS namespace.
- ESP32 #1 neprovozuje fallback AP s pouze RAM heslem; ESP32 #2 při neplatném NVS credential setu vypne lokální AP/web.
- Heslo AP ESP32 #1 již není součástí HTML dashboardu.
- Následující bezpečnostní úkoly zůstávají otevřené: fyzický service GPIO/UART, integrace provisioning board adapteru do `.ino`, bezpečné odstranění všech jednorázových Serial handoff výpisů, migrace starých namespace do dvouslotového CredentialStore (pokud bude schválena), NVS/Flash Encryption a fyzické testy.
- Funkce AP gate je zatím ověřena hostitelskými testy a statickou kontrolou call-sites; nikoli Arduino-ESP32 buildem ani fyzickou NVS.


## Revize 0.95 — integrace CredentialStore a legacy migrace
- [x] Zavést `CredentialLegacyMigrator` s povolením pouze při stavu cíle `EMPTY`; nikdy nepřepisovat VALID/CORRUPT/IO_ERROR.
- [x] ESP32 #1: použít `hwcCred` jako autoritativní zdroj AP SSID, AP hesla a administrátorského hesla; legacy namespace pouze pro první migraci.
- [x] ESP32 #2: použít stejné `hwcCred` schéma jako autoritativní zdroj; legacy `hwc_u2ui` pouze pro první migraci.
- [x] Při `CORRUPT`/`IO_ERROR` zakázat legacy fallback a zůstat fail-closed.
- [x] Nově vytvořená hesla předat jednorázově až po potvrzeném commit/read-back celého credential setu; běžný boot již hesla nevypisuje.
- [x] Přidat host test migrace, synchronizaci nového modulu a statickou kontrolu obou `.ino` call-sites.
- [ ] Ověřit Arduino-ESP32/Preferences build a reálné chování NVS na obou ESP32.
- [ ] Provést power-cut test v každé fázi migrace na testovací desce; ověřit EMPTY/CORRUPT/VALID chování.
- [ ] Dokončit fyzicky autorizovaný UART/GPIO provisioning a odstranit dočasný jednorázový Serial handoff.
- [ ] Ověřit NVS Encryption/Flash Encryption na přesné revizi čipu a konfiguraci; eFuse měnit až po samostatném bezpečnostním postupu.
- Peer command path zůstává default-deny; fyzické výstupy zůstávají neaktivní.


## Revize 0.96 — entropy při generování výchozích hesel
- [x] Ověřit podmínky `esp_random()` v ESP-IDF 3.3.x a zjistit, že generování probíhalo před aktivací Wi-Fi RF.
- [x] Přidat společný hex generátor používající čtyři 32bitová slova (128 bitů) s pevnými buffery a nulováním výstupu při chybě.
- [x] Před generováním chybějících hesel aktivovat Wi-Fi STA driver (`WiFi.persistent(false)`, `WiFi.mode(WIFI_STA)`) a při chybě fail-closed.
- [x] Ponechat existující hesla beze změny, aby automatická rotace nezablokovala přístup.
- [x] Přidat hostitelské a statické integrační testy obou sketchů.
- [ ] Provést skutečný Arduino-ESP32 build a ověřit `WiFi.mode(WIFI_STA)`/RNG na cílových deskách.
- [ ] Dokončit fyzicky autorizovaný provisioning a nahradit dočasný jednorázový Serial handoff.
- [ ] Ověřit NVS Encryption/Flash Encryption a odstranit nebo řízeně vyčistit legacy plaintext credential kopie až po úspěšně ověřeném provisioning workflow.
- Peer command path zůstává default-deny; fyzické výstupy zůstávají neaktivní.

## Další kroky po DEV KIT 0.97
1. Ověřit Arduino-ESP32 1.0.6 build obou sketchů; zejména `WebServer::client().remoteIP()` a HTTP 429 call-sites. Hostitelské testy nejsou náhradou target buildu.
2. Otestovat rate limiter na fyzických AP obou ESP32: pět chybných pokusů, blokace, přesná expirace, úspěšné přihlášení a více klientských IP.
3. Vyřešit HTTP Basic Auth přes prosté HTTP. Před vzdáleným přístupem požadovat WireGuard VPN; nikdy nevystavovat port 80 ESP32 přímo internetu. Posoudit TLS jen pokud je pro lokální provoz skutečně nutné.
4. Dokončit fyzicky autorizovaný provisioning a odstranit jednorázový Serial handoff nových hesel.
5. Vyřešit NVS/Flash Encryption a ověřit skutečné NVS/power-cut na cílových deskách.
6. Peer příkazy i fyzické výstupy ponechat zakázané, dokud nejsou uzavřeny P0 security blokátory a hardware testy.
