# P0 peer security audit — revize 0.73

Datum: 2026-11-04
Rozsah: přenositelný peer secret vault, persistentní anti-replay journal a bezpečnostní brány před síťovým řízením.

## Nálezy a opravy

### P0-1 — rollback replay floor při poškození slotu (opraveno v 0.73)
Předchozí `NvsPeerReplayStore::load()` v některých případech vrátil platný slot, i když druhý slot byl poškozen. Pokud poškozený slot obsahoval novější checkpoint, výběr staršího platného slotu mohl snížit `reservedThrough` a otevřít prostor pro replay již použitých sekvencí.

Oprava: korupce kteréhokoli slotu znamená `PEER_REPLAY_STORE_CORRUPT`. Není povolen fallback na starší checkpoint. `readPeerReplayFloorForHandshake()` i `PersistentPeerReplayGuard` následně odmítnou pokračovat. Test pokrývá poškození nejnovějšího slotu a ověřuje fail-closed výsledek.

### P0-2 — částečný zápis tajemství při chybě backendu (opraveno v 0.73)
Předchozí `PeerSecretVault::provision()` při `backend.write()==false` ihned skončil. Rozhraní backendu nemůže zaručit, že chyba nastala před jakoukoli změnou persistentního záznamu.

Oprava: při neúspěšném zápisu vault zavolá `backend.erase()` a vrátí chybu. Test simuluje částečný zápis a potvrzuje, že záznam nezůstane dostupný.

## Zbývající P0 blokátory

1. **Produkční chráněné úložiště tajemství neexistuje.** `PeerSecretVault` je pouze fail-closed politika a abstrakce; žádný ESP32 backend neprokazuje NVS Encryption.
2. **Bezpečný fyzický provisioning/rotace/obnova není implementována.** Příznak `physicalProvisioningConfirmed` je pouze vstupní kontrakt, nikoli detekce fyzického servisního režimu.
3. **Zdroj boot ID a nonce není integrován do kompletní orchestrace.** `esp_random()` musí být ověřen v přesném Arduino-ESP32/ESP-IDF targetu a před použitím RNG musí být zajištěna dostupnost entropie.
4. **Anti-replay rollback celé flash není řešen.** Dvouslotový žurnál s CRC řeší náhodné poškození a omezuje ztrátu při přerušení zápisu; nebrání útočníkovi obnovit starší kompletní kopii flash.
5. **Peer handshake/command orchestrace není napojena na W5500 UDP aplikační smyčku.** Stavový UDP broadcast zůstává neautentizovaný a nesmí být považován za řídicí kanál.
6. **Produkční command path musí zůstat default-deny.** Žádný COMMAND/RESPONSE/ACK nesmí řídit ventily nebo čerpadla.

## Rozhodnutí
- Opravy 0.73 jsou pouze přenositelné a hostitelsky testované.
- Při korupci replay journalu je preferována ztráta dostupnosti před oslabením anti-replay ochrany. Servisní obnova musí být explicitní a autentizovaná; automatické vymazání žurnálu není povoleno.
- Flash Encryption Release, NVS Encryption, Secure Boot, partition table a eFuse zůstávají nedotčené.
- Žádné fyzické výstupy nejsou povoleny.

## Dodatečný nález při pokračujícím auditu — úplná ztráta obou slotů

### P0-3 — prázdný replay journal nelze odlišit od prvního spuštění (otevřeno)

`NvsPeerReplayStore::load()` vrací `PEER_REPLAY_STORE_EMPTY`, pokud klíče `slot0` i `slot1` chybí. Stejný výsledek nastane při legitimním prvním použití i po smazání obou slotů. `PersistentPeerReplayGuard::beginAuthenticatedSession()` pak dovolí inicializaci nového checkpointu s floor 0.

Důsledek: samotná fail-closed reakce na poškození jednoho slotu neřeší odstranění obou záznamů. Pokud dojde ke ztrátě obou slotů, anti-replay stav se může znovu inicializovat. V současné verzi proto nelze považovat persistentní anti-replay za uzavřený P0 požadavek.

Bezpečný další návrh:
- doplnit samostatný inicializační marker, který se trvale zapíše až po prvním úspěšném checkpointu;
- pokud marker existuje a oba sloty chybí, vrátit CORRUPT, nikoli EMPTY;
- otestovat výpadek napájení mezi zápisem checkpointu a markeru a při obnově;
- explicitní servisní reset musí být oddělen od běžného bootu a vyžadovat autentizovaný fyzický servisní postup.

Omezení: marker ve stejné běžné NVS partition není ochranou proti úmyslnému smazání celé partition ani proti obnovení starší kopie celé flash. Odolnost proti takovému útoku vyžaduje důvěryhodný monotónní prvek nebo jinou hardwarově vynucenou ochranu; samotné CRC, další NVS klíč ani NVS Encryption neprokazují anti-rollback celé flash.

Do uzavření P0-3 zůstává peer command path zakázaná. Nález nebyl v této revizi maskován kosmetickou změnou kódu.
