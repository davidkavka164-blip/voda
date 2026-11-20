# HOME WATER CONTROLLER DEV KIT 0.91

Datum: 2026-11-04

## Lokální sériový provisioning protokol — přenositelná vrstva

- Přidán `CredentialProvisioningSerialProtocol.h` do `firmware/common/` a synchronizované kopie pro ESP32 #1 a ESP32 #2.
- Binární framing `HW`, verze 1, command `SET_CREDENTIALS`, délkově prefixovaná pole a CRC-16/CCITT-FALSE.
- Parser používá pevný 170B buffer, nepoužívá `String` ani heap, hlídá maximální délku a 2s inter-byte timeout.
- Přijímá pouze printable ASCII; AP heslo je omezeno na 8–63 znaků, administrátorské heslo na 12–64. Neznámá verze/command, oversized payload, chybný CRC, řídicí znaky a špatná struktura jsou odmítnuty.
- `CredentialProvisioningSerialService` přijímá vstup pouze během již otevřené fyzicky autorizované relace. Nemá API k vytvoření ani prodloužení autorizace. Chybný rámec relaci uzavře; validní rámec spotřebuje jeden controller attempt. `CredentialStore::validate()` nově omezuje AP heslo na 8–63 znaků, aby se vyloučilo uložení 64znakového PSK, který může být při SoftAP použití zkrácen.
- Odpovědi obsahují jen stavový byte; credentials se nikdy neecho ani nevrací.
- Protokol není šifrovaný ani autentizovaný. CRC není kryptografická ochrana; budoucí použití je omezeno na přímý místní kabel bez vzdáleného bridge.
- Protokol není integrován do `.ino`/Serial/GPIO a není aktivní. Stávající Serial/HTML úniky hesel se v této revizi nemění.

## Výsledky

- Standardní nativní testy: **50/50 PASS**.
- ASan/UBSan: **50/50 PASS**.
- Peer boot identity: PASS.
- W5500 link recovery: PASS.
- Moduly CredentialProvisioningSerialProtocol jsou bajtově shodné ve `common/`, `ESP32_1/src/` a `ESP32_2/src/`.

## Omezení

- Arduino překlad, skutečný UART/USB, desky, GPIO, fyzický servisní vstup a NVS nebyly testovány.
- AP boot gate, odstranění výpisů hesel, NVS/Flash Encryption a peer command path zůstávají mimo rozsah.
- Peer command path zůstává default-deny; fyzické výstupy jsou neaktivní.
