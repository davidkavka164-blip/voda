# HOME WATER CONTROLLER DEV KIT 0.93

Datum: 2026-11-04

## Board adapter pro fyzicky autorizovaný UART provisioning

- Přidán `CredentialProvisioningBoardAdapter.h`, synchronizovaný do `firmware/common/` a obou `ESP32_x/src/` stromů.
- Adapter propojuje existující fyzickou autorizační politiku, provisioning controller, UART-like `Stream` a board-specific fyzický vstup bez předpokladu konkrétního GPIO nebo polarity.
- Před otevřením relace průběžně zahazuje nevyžádané RX bajty. Po vyprázdnění RX vyžaduje ještě jeden klidný poll; teprve potom spotřebuje fyzickou autorizaci a otevře servisní relaci.
- Po otevření odešle pevný 9B `READY` rámec (response type `0x81`, status `0x01`). Host klient odešle SET_CREDENTIALS až po ověření tohoto rámce.
- Vstup bez relace nikdy nevytvoří oprávnění. Síťový/HTTP/UDP handler nemá rozhraní k fyzickému vstupu.
- RX drain je omezen na 256 B/poll a servisní čtení na 64 B/poll, aby adapter nezablokoval hlavní smyčku.
- Po jednom výsledku, chybě rámce, expiraci nebo chybě TX se parser resetuje a další vstup se zahodí do další fyzické autorizace.
- Host klient nyní skenuje omezený šum/boot logy, čeká na ověřený `READY`, a teprve pak pošle credentials. Pokud READY nepřijde nebo přijde jiný stav, požadavek neposílá.

## Testy

- `./tests/run_native_tests.sh`: **51/51 PASS**.
- `sh tests/.run_native_tests_asan.sh`: **51/51 PASS**.
- `python3 -m unittest discover -s tests/python -v`: **17/17 PASS**.
- `python3 -m py_compile ...`: PASS.
- `tests/check_credential_module_sync.sh`: PASS; šest bezpečnostních modulů je bajtově totožných ve common a obou ESP32 src stromích.
- `tests/check_peer_boot_identity.sh`: PASS.
- `tests/check_peer_ethernet_recovery.sh`: PASS.

Nové adapter testy zahrnují předčasné/stale RX bajty, fyzické podržení, READY-before-request, fragmentovaný request, one-shot provisioning, expiraci relace, selhání TX a dávkované vyprazdňování 600 nevyžádaných bajtů.

## Omezení

- Adapter je hostitelsky testovaná přenosná vrstva; **není připojen do `ESP32_1.ino` ani `ESP32_2.ino`**.
- Není zvolen servisní GPIO, pin polarity, UART port, konektor ani baud rate pro konkrétní desky.
- V současných `.ino` je Serial používán pro diagnostiku a stále obsahuje výpisy hesel; ESP32 #1 navíc zobrazuje AP heslo v HTML. Do skutečného Serial portu se adapter nesmí připojit, dokud nebude vyřešeno vlastnictví linky a odstraněny úniky tajemství.
- Host klient s aktuálním firmware stále nesmí být spuštěn s `--firmware-integrated`.
- Protokol přenáší credentials v plaintextu; pouze přímý lokální kabel, bez bridge. CRC není autentizace ani šifrování.
- CredentialStore stále není napojen na boot/AP gate. Arduino build, fyzická NVS, power-cut, DTR/RTS, NVS/Flash Encryption a fyzický hardware nebyly ověřeny.
- Peer command path zůstává default-deny; fyzické výstupy zůstávají neaktivní.
