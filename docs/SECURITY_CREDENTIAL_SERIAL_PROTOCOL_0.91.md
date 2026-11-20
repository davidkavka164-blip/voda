# HOME WATER CONTROLLER — lokální sériový provisioning protokol

Revize: 0.91  
Datum: 2026-11-04  
Stav: přenositelný parser a servisní wrapper implementovány a hostitelsky testovány; bez zapojení do `.ino`, fyzického vstupu nebo skutečného UART.

## 1. Rozsah

Revize 0.91 přidává binární protokol pro lokální USB/UART servisní spojení. Implementace je v:

- `firmware/common/CredentialProvisioningSerialProtocol.h`
- synchronizované kopie v `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`

Jde o parser a wrapper nad již existujícím `CredentialProvisioningController`. Není zde otevřen žádný port, nevolá se `Serial.begin()`, nepřiřazuje se GPIO a nemění se `.ino`.

## 2. Bezpečnostní hranice

- Sériový protokol neumí vytvořit ani prodloužit fyzickou autorizaci.
- Jediná budoucí cesta k `CredentialProvisioningController::openSession()` musí být deskový adapter, který průběžně vzorkuje skutečný fyzický servisní vstup přes `LocalServiceAuthorization::samplePhysicalInput()`.
- Serial/USB handler nesmí volat `samplePhysicalInput()` ani `openSession()`.
- `CredentialProvisioningSerialService` přijímá bajty pouze při otevřené a dosud platné servisní relaci. Bez relace parser resetuje a vstup ignoruje.
- Chybný rámec relaci uzavře. Úplný platný rámec předá jeden pokus controlleru; controller po pokusu relaci uzavře a vynuluje credential buffer.
- Neexistuje příkaz pro čtení nebo export uložených hesel. Odpověď obsahuje pouze stavový byte, nikdy credentials.

**Tento protokol není šifrovaný ani kryptograficky autentizovaný.** CRC-16 detekuje náhodné poškození přenosu, nikoli úmyslnou změnu nebo původ dat. Při budoucím použití musí jít o přímý místní kabel, bez Bluetooth/serial-over-IP/Wi-Fi bridge a bez přístupu třetích osob k vodičům. Tajemství jsou po dobu přenosu v plaintextu na fyzické lince. Tento model vychází z místního fyzického provisioningu, nikoli z ochrany proti útočníkovi s přístupem ke kabelu během servisního okna.

Před integrací do skutečného Serial portu se musí vyřešit jeho současné použití pro diagnostiku. Aktuální `.ino` stále obsahují výpisy hesel do Serial Monitoru; protokol proto **nesmí být zapnut**, dokud nebudou tyto výpisy odstraněny a servisní režim nebude mít řízené vlastnictví linky.

## 3. Rámec požadavku

Všechny vícebajtové délky a CRC jsou big-endian:

| Offset | Délka | Obsah |
|---:|---:|---|
| 0 | 1 | Magic `0x48` (`H`) |
| 1 | 1 | Magic `0x57` (`W`) |
| 2 | 1 | Verze `0x01` |
| 3 | 1 | Typ příkazu `0x01` = SET_CREDENTIALS |
| 4 | 2 | Délka payloadu, big-endian |
| 6 | proměnná | Payload |
| konec − 2 | 2 | CRC-16/CCITT-FALSE, big-endian |

Payload obsahuje přesně tři délkově prefixované ASCII hodnoty v tomto pořadí:

1. `apName`: 1 byte délka + 8–32 byte textu;
2. `apPassword`: 1 byte délka + 8–63 byte textu;
3. `adminPassword`: 1 byte délka + 12–64 byte textu.

Povoleny jsou pouze tisknutelné ASCII bajty `0x20–0x7E`; NUL, řídicí znaky, neplatná délka, chybějící pole a koncové přebytečné bajty se odmítají. Současné limity odpovídají `CredentialStore::validate()`. AP heslo je omezeno na 63 znaků kvůli kompatibilitě s Wi-Fi SoftAP API; zdrojový kód Arduino-ESP32 pracuje s limitem SSID a komunitní issue dokumentuje zkrácení 64znakového PSK na 63 znaků. Před cílovým buildem je nutné ověřit přesné chování instalované starší verze Arduino-ESP32. [Arduino-ESP32 WiFiAP.cpp](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiAP.cpp), [issue #5367](https://github.com/espressif/arduino-esp32/issues/5367).

CRC se počítá od pole Version (offset 2) po poslední payload byte. Parametry: polynomial `0x1021`, initial value `0xFFFF`, bez reflektování, xorout `0x0000` (CRC-16/CCITT-FALSE).

Maximální payload je 162 byte a maximální celý rámec 170 byte. Parser používá pevný buffer; nepoužívá `String`, heap alokace ani dynamické délky.

## 4. Timeout a chování parseru

- Mezi dvěma přijatými bajty nesmí uplynout 2 000 ms. Na přesné hranici 2 000 ms se rozpracovaný rámec vynuluje.
- Časová aritmetika je wrap-safe pro `millis()`.
- Šum před magic sekvencí se ignoruje.
- Neplatná verze, typ, délka, CRC nebo payload vrátí FRAME_REJECTED; wrapper následně zavře servisní relaci.
- Po úspěšném dekódování se interní rámcový buffer vynuluje a hodnoty se předají controlleru v dočasném `Credentials` bufferu.
- Na konci operace controller vstupní buffer vynuluje.

## 5. Odpověď

Odpověď má stejnou hlavičku, typ `0x81` = RESULT, payload délky 1 byte a CRC-16/CCITT-FALSE. Payload je pouze stavový kód:

- `0x12` — chybný rámec;
- `0x20 + enum CredentialProvisioningResult` — výsledek provisioningu (0x20 = PROVISIONED, 0x21 = SESSION_CLOSED, 0x22 = SESSION_EXPIRED, 0x23 = INVALID_INPUT, 0x24 = STORAGE_IO_ERROR, 0x25 = RECOVERY_FAILED, 0x26 = SAVE_FAILED).

Při chybějící servisní relaci a při čekání na další bajty se odpověď neposílá; tím se zabrání odpovědi na každý přijatý byte. Adaptér má odpověď odeslat pouze pro FRAME_REJECTED nebo PROVISION_RESULT.

## 6. Testování revize 0.91

Nový `tests/test_credential_provisioning_serial_protocol.cpp` pokrývá:

- odmítnutí sériového provisioningu bez fyzické relace;
- úspěšný přenos po jednotlivých bajtech a spotřebování jednorázové relace;
- ignorování šumu před magic sekvencí;
- odmítnutí chybného CRC a uzavření relace;
- odmítnutí neznámé verze, typu a příliš dlouhého payloadu;
- odmítnutí řídicích znaků i při správném CRC;
- timeout neúplného rámce na přesné 2 000ms hranici;
- maximální legální délku rámce;
- pevnou odpověď bez echo credentials a odmítnutí malého/null output bufferu.

Výsledky hostitelských kontrol:

- standardní nativní sada: **50/50 PASS**;
- ASan/UBSan: **50/50 PASS**;
- peer boot identity: PASS;
- W5500 link recovery: PASS.

## 7. Co zůstává otevřené

- Neznáme přesnou revizi/desku ani dostupný servisní konektor; žádné GPIO/UART piny nebyly přiděleny.
- Parser ani servisní wrapper nejsou připojeny k reálnému `Serial`/UART, fyzickému vstupu nebo `.ino`.
- Není připraven hostitelský klient ani instalační postup.
- V aktuálních `.ino` stále existují výpisy AP/admin hesel do Serial; ESP32 #1 navíc zobrazuje AP heslo v HTML. Před integrací se musí odstranit a staticky testovat všechny tyto úniky.
- CredentialStore/NVS adapter stále není zapojen do startu AP/web obou jednotek.
- USB/UART plaintext přenos, skutečná NVS, NVS/Flash Encryption, Arduino build a hardware nebyly ověřeny.

**Závěr:** Protokol je připraven jako hostitelsky testované přenositelné jádro. Není to funkční nebo produkčně aktivní servisní port. Bez fyzického autorizačního vstupu a bezpečného vlastnictví sériové linky jej nelze zapnout.

## Doplnění revize 0.92 — hostitelský klient

Hostitelský klient `tools/credential_provisioning_client.py` je doplněn v revizi 0.92. Umí sestavit request, validovat response a volitelně použít PySerial. Výchozí režim pouze kontroluje vstupy a rámec. Odesílání je chráněno přepínačem `--firmware-integrated`; v revizi 0.92 se nesmí použít, protože device-side Serial/GPIO adapter není zapojen do `.ino`. Podrobnosti a omezení jsou v `SECURITY_CREDENTIAL_HOST_CLIENT_0.92.md`.
