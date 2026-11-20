> Historical snapshot: the dual-W5500 topology described here is superseded by the mixed Wi-Fi/Ethernet transport in REV45. See `NETWORK_TOPOLOGY_AUDIT_REV45.md`.

# Audit obnovy komunikace ESP32 #1 / ESP32 #2 – REV33

## Rozsah

Kontrola chování při ztrátě Ethernet/W5500 spojení, ztrátě DHCP adresy, obnovení linky, restartu jedné jednotky a obnovení stavových UDP zpráv. Zvláštní důraz je na to, aby přerušený transfer z horní nádrže nebyl po návratu komunikace automaticky obnoven.

## Zjištění

1. ESP32 #1 i ESP32 #2 detekují fyzický stav W5500 linky. Po obnovení fyzického spojení znovu inicializují Ethernet/DHCP; při ztrátě adresy ukončí UDP socket a opakují inicializaci.
2. ESP32 #2 vytvoří `peerBootId` pouze jednou při startu zařízení. Opakované inicializace W5500 ani DHCP nemění boot ID ani čítač sekvence.
3. ESP32 #1 přijímá pouze validní stavové rámce se správným CRC, očekávaným ID zařízení a novou sekvencí. Duplicitní, staré a mimo pořadí doručené rámce odmítá.
4. Použitelnost hladiny horní nádrže závisí na čerstvosti přijatého rámce i stáří samotného měření. Po překročení limitu 6000 ms je peer hodnota nepoužitelná.
5. Ztráta peer dat při PRECHECK nebo RUNNING vyvolá abort transferu s důvodem COMMUNICATION; výstupní příkaz přejde na CLOSE_ALL.
6. Změna boot ID ESP32 #2 během PRECHECK/RUNNING horního transferu rovněž vyvolá abort. Nová stavová data po restartu peer jednotky nesmějí obnovit původní požadavek.
7. `TransferLogic` drží terminální stav ABORTED až do potvrzení výsledku. Po obnovení peer dat zůstává ventil zavřený; nový transfer vyžaduje nový požadavek s novým ID.
8. Žurnál transferu ukládá PREPARED před otevřením ventilu a ACTIVE před přechodem do RUNNING. Po restartu se PREPARED/ACTIVE převedou na INTERRUPTED; automatické pokračování není povoleno.

## Omezení

- Nativní a statické testy ověřují logiku a integrační vazby, nikoli skutečný výpadek kabelu, DHCP serveru nebo napájení na fyzickém zařízení.
- Detekce peer nečerstvosti používá limit 6000 ms. Krátké výpadky, během nichž zůstávají data stále v limitu čerstvosti, nejsou považovány za přerušený transfer; pokud čerstvost vyprší, transfer se ukončí.
- Boot ID je náhodná 32bitová hodnota vytvořená při startu ESP32 #2. Kolize je velmi nepravděpodobná, ale tento identifikátor není kryptograficky garantovaně unikátní.

## Závěr

Při ztrátě čerstvých peer dat nebo při detekci nového boot epochu ESP32 #2 je rozpracovaný horní transfer ukončen a ventil se zavře. Obnovení Ethernetu ani příchod nových stavových rámců samy o sobě znovu nevydají požadavek na transfer. Obsluha musí zadat nový požadavek.
