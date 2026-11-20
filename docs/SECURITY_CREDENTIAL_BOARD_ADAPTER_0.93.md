# HOME WATER CONTROLLER — servisní board adapter

Revize: 0.93  
Datum: 2026-11-04  
Stav: přenositelný board adapter implementován a hostitelsky testován; bez mapování na GPIO/UART a bez zapojení do `.ino`.

## 1. Účel

`CredentialProvisioningBoardAdapter<StreamT, InputT>` v `firmware/common/CredentialProvisioningBoardAdapter.h` propojuje:

- `LocalServiceAuthorization` — politiku 3s fyzického podržení a 60s jednorázového okna;
- `CredentialProvisioningController` — jednorázový zápis/obnovu přístupových údajů;
- `CredentialProvisioningSerialService` — parser a response wrapper;
- `StreamT` — UART-like transport (`available/read/write/flush`);
- `InputT` — jediný board-specific vstup (`readActive()`).

Modul nepředpokládá GPIO číslo, elektrickou polaritu, pull-up ani konkrétní `HardwareSerial`. Tyto parametry musí dodat až ověřený adapter cílové desky.

## 2. Životní cyklus

1. Každý `poll(nowMs)` vzorkuje fyzický vstup přes `readActive()` a posílá výsledek do `samplePhysicalInput()`.
2. Pokud neexistuje platná servisní relace, parser se resetuje a RX bajty se zahazují. V jednom pollu nejvýše 256 B; při zbylém backlogu se další práce odloží do příštího pollu.
3. Pokud byly nějaké nevyžádané bajty odstraněny, adapter vyžaduje ještě jeden klidný poll bez RX dat. Tím se oddělí stará data od nově otevřené servisní relace.
4. Jestliže je fyzická autorizace stále platná, controller ji spotřebuje a otevře relaci s původním 60s deadline.
5. Zařízení odešle devítibajtový rámec READY: magic `HW`, verze 1, response type `0x81`, délka 1, status `0x01`, CRC-16/CCITT-FALSE.
6. Host klient čeká na platný READY. Teprve poté odešle jeden request SET_CREDENTIALS.
7. Adapter zpracuje maximálně 64 RX bajtů na poll. Po kompletním validním requestu nebo chybě odešle pevnou výsledkovou odpověď bez credentials.
8. Controller relaci spotřebuje a uzavře. Zbylé RX bajty se zahodí; další pokus vyžaduje release a nové fyzické podržení.

## 3. Fail-closed chování

- Bez fyzického vstupu není možné otevřít session.
- Serial request nemůže volat `samplePhysicalInput()` ani `openSession()`.
- Data přijatá před relací jsou zahozena, nikoli podržena pro pozdější zpracování.
- Zbylý RX backlog blokuje otevření session, dokud nebude odstraněn.
- Neúplný/poškozený request, expirace nebo transportní TX chyba resetuje parser a zavře relaci.
- Selhání odeslání READY okamžitě uzavře právě spotřebovanou session. Je vyžadována nová fyzická autorizace.
- Výsledkové rámce neobsahují SSID ani hesla.

## 4. Protokol READY

Aby host neposílal tajemství dříve, než zařízení dokončí fyzickou autorizaci, byl zaveden stavový kód `0x01` v rámci RESULT (`0x81`). READY není samostatné oprávnění: potvrzuje pouze to, že board adapter již spotřeboval platnou fyzickou autorizaci a má otevřenou časově omezenou session.

Host klient:

- čte a ověřuje devítibajtové rámce včetně CRC;
- před rozpoznáním `HW` dovolí omezený počet diagnostických/boot bajtů;
- při chybě, timeoutu nebo jiném stavovém kódu request neodešle;
- credentials odešle právě jednou až po READY;
- následně čeká na výsledkový status.

## 5. Sériová linka a logy

Tento modul sám neřeší vlastnictví fyzického UART portu. Současné sketch `.ino` používají `Serial` pro diagnostické výpisy. V budoucí integraci se musí vybrat jedna z možností:

- přímý Serial0/USB-UART s výslovným režimem, který po dobu služby potlačí všechny běžné logy a webové/OTA callback výpisy; nebo
- samostatný UART a konkrétní konektor/piny ověřené pro danou revizi desky.

Žádná volba nebyla v 0.93 provedena. Současné výpisy AP/admin hesel a HTML únik ESP32 #1 zůstávají otevřené; proto nesmí být adapter zapnut v reálném firmware.

Protokol přenáší hesla v plaintextu. Přímý kabel a fyzický dohled jsou povinné; žádný Bluetooth, Wi-Fi, TCP/UDP bridge ani sériový server. CRC-16 chrání před náhodnou chybou, nikoli proti aktivnímu odposlechu/manipulaci.

## 6. Testy

Nový `tests/test_credential_provisioning_board_adapter.cpp` ověřuje:

- vstupní rámec před autorizací se zahodí a neprovisionuje;
- READY se odešle pouze po fyzickém 3s podržení;
- validní request se přijme až po READY a spotřebuje session;
- po výsledku se další request bez nového fyzického podržení zahodí;
- fragmentovaný request se zpracuje v jedné relaci;
- expirace přesného 60s deadline zavře relaci a resetuje parser;
- chyba TX při READY zavře session;
- 600 nevyžádaných bajtů se odstraní po dávkách do 256 B/poll a session se otevře až po klidném pollu.

Výsledky 0.93: C++ standard 51/51, ASan/UBSan 51/51, Python klient 17/17, syntax check PASS, synchronizace modulů PASS.

## 7. Otevřené blokátory před integrací

1. Potvrdit přesný model/revizi obou ESP32 desek a servisní konektor.
2. Zvolit fyzický vstup, elektrickou polaritu, pull-up/down, debounce a boot-safe stav; GPIO nepřidělovat odhadem.
3. Zvolit UART/USB port, baud rate a potvrdit DTR/RTS/auto-reset chování na fyzickém hardware.
4. Vyřešit výhradní vlastnictví Serial linky a odstranit stávající log/HTML úniky hesel.
5. Připojit adapter do obou `.ino`, bez jakéhokoli přístupu k relé/ventilům/čerpadlům.
6. Připojit CredentialStore do boot/AP gate a provést skutečné fail-closed a recovery testy.
7. Ověřit Arduino-ESP32 build, NVS a power-cut na testovacím hardware.
