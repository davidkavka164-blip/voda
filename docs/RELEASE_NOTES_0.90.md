# HOME WATER CONTROLLER DEV KIT 0.90

Datum: 2026-11-04

## Transportně nezávislý provisioning a fyzicky autorizovaná obnova credentials

- Přidán `CredentialProvisioningController.h`: spotřebuje jednorázovou fyzickou autorizaci, otevře servisní relaci pouze do původního 60s deadlinu od dokončení fyzického podržení (okno se neprodlužuje) a přijme jeden provisioning pokus.
- Všechny zápisy jsou v této revizi dostupné pouze přes fyzicky autorizovaný controller; všechny výsledky uzavřou relaci; vstupní mutable `Credentials` buffer se vynuluje. Controller nevrací ani neloguje hesla.
- `CredentialStore::eraseAllForPhysicalRecovery()` je privátní a dostupný pouze provisionovacímu controlleru. Obnova nejprve odstraní/ověří selektor, potom odstraní a ověří všech šest slotových klíčů a nakonec vyžaduje stav `EMPTY`.
- Doplněno bezpečné zotavení po osiřelém staging záznamu z přerušeného prvního zápisu nebo přerušené obnovy. Při `IO_ERROR` a chybě mazání zůstává systém fail-closed.
- Zápis `CredentialStore::saveInternal()` je privátní a přístupný pouze provisionovacímu controlleru; první založení ani rotaci nelze obejít přímou veřejnou metodou úložiště.
- `CredentialStore::clear()` používá nulování přes `volatile` byte pointer. Backend kontrakt výslovně vyžaduje idempotentní mazání chybějícího klíče.
- Přidán rozsáhlý `test_credential_provisioning_controller.cpp` včetně fault injection a přesné expirace relace.
- Moduly jsou synchronizované v common i obou ESP32 zdrojových stromech.

## Výsledky

- Standardní nativní testy: **49/49 PASS**.
- ASan/UBSan: **49/49 PASS**.
- Peer boot identity: PASS.
- W5500 link recovery: PASS.
- ZIP integrita: PASS; po rozbalení archivu standardní i ASan/UBSan testy znovu 49/49 PASS.

## Omezení

- Controller není zapojen do `.ino`, fyzického GPIO ani servisního transportu.
- AP/web boot gate není dokončen; výpisy hesel v Serial a HTML nebyly odstraněny.
- NVS/Flash Encryption, Arduino build a fyzické testy nebyly provedeny.
- Peer command path zůstává default-deny a fyzické výstupy zůstávají neaktivní.
