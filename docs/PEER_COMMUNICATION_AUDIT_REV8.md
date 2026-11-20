# Audit komunikace ESP32 #1 ↔ ESP32 #2 — REV8

Datum: 7. 11. 2026  
Rozsah: skutečné call-sites obou `.ino`, společné moduly `PeerProtocol`, stavový UDP transport, vstup transferové simulace a související dokumentace.  
Závazná reference: HOME WATER CONTROLLER MASTER 1.3.  
Výsledek: **auditní zjištění; žádná změna firmware ani aktivace výstupů**.

## 1. Shrnutí

Komunikace v aktuálním firmware je pouze jednosměrné vysílání stavových dat ESP32 #2 → ESP32 #1 přes UDP broadcast. ESP32 #1 používá přijatou hladinu horní nádrže v dashboardu a v transferové simulaci. Nejde o autentizovanou komunikaci a nejde o hotovou bezpečnou řídicí cestu.

Důležitá pozitivní zjištění:
- Příjem kontroluje délku rámce, magic, verzi, identifikátor zařízení, typ zprávy, rozsahy hodnot a CRC.
- Monitor odmítá duplicitní, staré a mimo pořadí doručené sekvence; hlídá stáří přijetí i stáří měření.
- Při ztrátě čerstvých dat horní nádrže transferová simulace vrací důvod `COMMUNICATION` a ukončuje běžící simulaci.
- ESP32 #2 si limity horní nádrže načítá z vlastního NVS; nepřebírá je přes UDP.
- Žádná fyzická výstupní cesta pro ventily ani čerpadla není v tomto stavu aktivní.

## 2. Zjištění

### PEER-REV8-01 — Stavový UDP rámec není autentizovaný

**Závažnost: BLOKUJÍCÍ před jakýmkoli použitím peer dat pro fyzické řízení.**

`PeerProtocol` používá CRC32. CRC odhalí náhodné poškození, ale není kryptografickým důkazem původu ani integrity vůči útočníkovi. Přijímač přijme rámec od libovolného uzlu, který dokáže doručit UDP datagram na port 4211 a sestavit veřejně známý formát s platným CRC, boot ID a novou sekvencí. Zdrojová IP/MAC adresa ani HMAC podpis se neověřují.

Důsledek: hodnoty hladiny, alarmů a checksumu limitů mohou být v místní LAN podvrženy. Čerstvost a CRC tuto slabinu neřeší. Dokument `OPEN_HARDWARE_ITEMS.md` správně uvádí, že neautentizovaný UDP status nesmí být použit v nedůvěryhodné síti.

**Požadavek před budoucím fyzickým řízením:** status musí být svázán s autentizovanou peer session a ověřeným identitou/boot epochou, včetně ochrany proti replay. Při chybě nebo neexistenci session musí být data považována za nedůvěryhodná a transfer nesmí pokračovat.

### PEER-REV8-02 — Kryptografické moduly existují, ale nejsou napojeny do `.ino`

**Závažnost: BLOKUJÍCÍ pro autentizovanou síťovou komunikaci.**

V obou sketch souborech nebyly nalezeny call-sites pro `PeerHandshake`, `PeerCommandSession`, `PeerSessionCodec`, `PeerSecretVault`, `NvsPeerReplayStore` ani `PersistentPeerReplayGuard`. Tyto moduly jsou přenositelné a hostitelsky testované, ale jejich existence sama o sobě nechrání UDP status transport.

Produkční bezpečné úložiště dlouhodobého peer tajemství a jeho fyzicky autorizované provisioning zapojení také nejsou dokončeny. `NvsPeerReplayStore` není instanciován ani používán ve sketchech.

Platformní audit `NVS_ENCRYPTION_PLATFORM_AUDIT.md` navíc potvrzuje, že `PeerSecretVault` je pouze fail-closed politika/rozhraní, nikoli šifrovací backend. `Preferences` samo NVS Encryption nezapíná. Vzhledem k vývojovému prostředí Windows 7 / Arduino IDE 1.8.19 a dosud neznámé přesné verzi ESP32 core nelze bezpečný produkční backend nyní implementovat odpovědně bez ověření platformy. Nesmí se předstírat, že běžné NVS je důvěrné.

**Další implementační brána:** navrhnout a integrovat životní cyklus session pro oba směry, provisioning tajemství a důvěryhodné úložiště, perzistentní anti-replay a timeout/reconnect chování. Teprve potom smí být status považován za autentizovaný.

### PEER-REV8-03 — Transferová simulace spoléhá na neautentizovanou hladinu horní nádrže

**Závažnost: BLOKUJÍCÍ před fyzickým napojením ventilů. Aktuálně bez přímého fyzického účinku, protože jde o simulaci.**

`updateTransferSimulation()` považuje peer hladinu za použitelnou, pokud je W5500 připraven a `upperLevelUsable()` potvrdí platný a čerstvý vzorek. Dále kontroluje lokální limity a shodu jejich checksumu. Nevyžaduje autentizovanou peer session, protože ta zatím není integrována.

To je přijatelné pouze pro současnou diagnostickou/simulační fázi. Před fyzickým řízením musí být podmínka `upperFresh` rozšířena o stav autentizované session a platnost jejího peer kontextu. Samotný checksum limitů není autentizace.

### PEER-REV8-04 — Stav místní ochrany čerpadla ESP32 #2 je pouze diagnostický

Pole `pumpProtectionReady` se posílá ve status rámci a zobrazuje se na webu ESP32 #1. Nenašel jsem jeho použití v `TransferInputs` ani v rozhodování `TransferLogic`; aktuálně tedy transferovou simulaci neblokuje.

Neoznačuji to automaticky za chybu: místní ochrana čerpadla na ESP32 #2 je samostatná funkce a nemusí být totožná s podmínkou pro otevření gravitačního/transferového ventilu. Před fyzickou integrací je však nutné výslovně určit, zda a kdy musí být `pumpProtectionReady` předpokladem konkrétního transferu. Pokud ano, musí se přenášet autentizovaně a musí být součástí prechecku i průběžné kontroly.

### PEER-REV8-05 — Objem horní nádrže a teplota se zatím nepřenášejí jako platná měření

ESP32 #2 explicitně nastavuje `upperVolumeValid = 0` a `temperatureValid = 0`; kód uvádí, že kalibrace objemu a měření teploty nejsou integrovány. To je správné fail-safe označení a nesmí se změnit na platné hodnoty bez skutečné implementace a ověření. MASTER 1.3 počítá s objemovým modelem horní nádrže a informativní teplotou.

### PEER-REV8-06 — Síťová topologie a fyzická vrstva nejsou ověřeny

UDP broadcast vyžaduje dosažitelnost obou W5500 ve stejné vhodné LAN a průchod broadcast/client-to-client komunikace. Přesná topologie, moduly W5500, napájení, SPI a reálný link/DHCP provoz nejsou fyzicky ověřeny. Automatická obnova linku/DHCP je implementována a hostitelsky/staticky testována, nikoli ověřena na hardware.

## 3. Shoda s MASTER 1.3

- Výpadek/zastarání peer dat je detekován; transferová simulace je ukončena jako komunikační chyba.
- Neplatná nebo nedostupná horní hladina neumožní zahájit transfer ze zdroje horní nádrže.
- ESP32 #2 zachovává lokální logiku ochrany i bez komunikace; fyzické relé však není ovládáno.
- Přijatá data zatím nejsou kryptograficky důvěryhodná. To je otevřená bezpečnostní brána, nikoli hotová funkce.
- Žádný automatický transfer ani fyzické otevření ventilu nebylo v tomto auditu povoleno.

## 4. Doporučené pořadí další práce

1. Zjistit přesnou verzi Arduino-ESP32 core v uživatelově IDE a ověřit na testovací desce dostupný postup NVS Encryption / Flash Encryption; do té doby nepřidávat produkční backend.
2. Dokončit návrh bezpečného peer provisioning a platformně ověřitelného úložiště dlouhodobého tajemství pro skutečný cílový ESP32 core.
3. Integrovat handshake/session do obou `.ino` a UDP transportu; přidat autentizaci každého status rámce a ochranu proti replay.
4. Doplnit integrační testy skutečných call-sites: chybějící tajemství, poškozený NVS, špatný HMAC, replay, restart jedné strany, ztráta linku, opožděný paket, změna boot ID, timeout handshake a obnova komunikace.
5. Teprve po tom znovu auditovat `TransferLogic` a jeho vstupy pro budoucí fyzické ventily. Vyjasnit význam `pumpProtectionReady` pro jednotlivé trasy.
6. Samostatně doplnit objemový model horní nádrže a teplotní čidlo podle skutečného hardware.
7. Ověřit topologii W5500 a Arduino-ESP32 build na cílové verzi prostředí.

## 5. Ověřovací hranice

Tento audit vychází z REV8 zdrojových souborů a hostitelských testů. Neprokazuje skutečné chování W5500, ESP32, NVS/Flash Encryption, relé, čerpadel ani ventilů. Výstupy zůstávají neaktivní. Balíček zůstává auditním návrhem a není určen pro nasazení do fyzického řízení vody.
