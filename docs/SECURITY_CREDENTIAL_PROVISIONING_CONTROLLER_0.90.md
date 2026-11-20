# HOME WATER CONTROLLER — transportně nezávislý provisioning controller

Revize: 0.90  
Datum: 2026-11-04  
Stav: přenositelné jádro implementováno a hostitelsky testováno; bez fyzického vstupu, servisního transportu a AP integrace.

## 1. Rozsah revize

Revize 0.90 doplňuje společný workflow pro prvotní nastavení a fyzicky autorizovanou obnovu AP SSID/AP hesla/admin hesla. Změny jsou ve společné složce a synchronizované ve zdrojových stromech ESP32 #1 a ESP32 #2:

- `firmware/common/CredentialProvisioningController.h`
- aktualizovaný `firmware/common/CredentialStore.h`
- kopie obou souborů v `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`

Nový controller je transportně nezávislý. Není připojen k `.ino`, GPIO, HTTP, Serial ani Wi-Fi. Záměrně tedy ještě nejde o funkční provisionovací rozhraní. Zápisová metoda `CredentialStore::saveInternal()` je privátní a přístupná pouze controlleru; tím nelze první sadu založit přímým voláním veřejné metody úložiště. V této revizi tedy veškeré změny credentials vyžadují fyzickou servisní autorizaci; běžná změna z již přihlášeného dashboardu zatím není implementována.

## 2. Nový workflow

1. Deskový adapter musí pravidelně číst skutečný fyzický servisní vstup a jako jediný volat `LocalServiceAuthorization::samplePhysicalInput()`.
2. Po nepřetržitém podržení po dobu 3 s autorizační politika vytvoří jednorázové okno 60 s.
3. `CredentialProvisioningController::openSession(now)` sám spotřebuje jednorázové oprávnění. HTTP/UDP/Serial handler nesmí autorizaci vytvářet ani simulovat.
4. Otevřená servisní relace používá tentýž deadline jako původní fyzická autorizace: od okamžiku dokončení 3s podržení uplyne nejvýše 60 s celkem. Pozdní otevření relaci neprodlouží. Časování je wrap-safe a při dosažení hranice se relace odmítne.
5. `provision(now, Credentials&)` přijme právě jeden pokus. Po každém výsledku relaci uzavře a vstupní `Credentials` bezpečně vynuluje.
6. Vstup se validuje před zápisem. Následně se použije dvouslotový staging/read-back/commit a finální načtení aktivní sady.
7. Úspěch se vrací jen tehdy, když aktivní uložená sada přesně odpovídá předaným údajům. Controller žádné heslo nevrací ani neloguje.

## 3. Obnova po neúplném prvním zápisu

Audit odhalil případ: výpadek napájení při úplně prvním provisioningu může zanechat část staging klíčů bez `cred_active`. `CredentialStore::load()` správně hlásí `CORRUPT`, protože stav nelze bezpečně považovat za prázdný. Bez obnovovací cesty by však zůstala jednotka trvale zablokovaná.

Revize 0.90 přidává `CredentialStore::eraseAllForPhysicalRecovery()` jako privátní metodu přístupnou pouze `CredentialProvisioningController`:

- nejprve odstraní a ověří nepřítomnost selektoru `cred_active`;
- potom idempotentně odstraní všech šest slotových hodnot;
- každé odstranění ověří následným čtením;
- nakonec ověří, že úložiště vrací pouze `CREDENTIAL_EMPTY`.

Při přerušení obnovy může zůstat jen stav `CORRUPT`; další pokus vyžaduje nové fyzické oprávnění. Nikdy se nesmí zvolit neúplný slot jako platný. Pokud backend vrací `IO_ERROR` nebo mazání/ověření selže, controller operaci ukončí a přístup zůstává fail-closed.

Obnova `CORRUPT` znamená fyzicky autorizované odstranění celé sady AP/admin credentials a následné založení nové sady. Nejde o získání ani zobrazení původního hesla. Platná sada (`VALID`) se při změně nahrazuje přes neaktivní slot; poslední aktivní sada zůstává do commitu nedotčená. V této revizi jsou všechny zápisy záměrně dostupné jen přes fyzicky autorizovaný controller; běžná změna přes webové přihlášení zatím není implementována.

## 4. Ochrana RAM

`CredentialStore::clear()` nyní používá zápisy přes `volatile` byte pointer, aby kompilátor nemohl běžné nulování odstranit jako mrtvý zápis. Controller vyžaduje mutable dočasný objekt `Credentials&` a po každém výsledku jej vynuluje; interní kopie pro ověření jsou rovněž vynulovány.

Budoucí servisní transport musí parsovat hodnoty do pevně dimenzovaného dočasného bufferu. Nesmí vytvářet logovací řetězce, HTTP odpovědi nebo dlouhodobé `String` kopie s hesly. Toto revize 0.90 zatím nemůže ověřit, protože transport není implementován.

## 5. Výsledky testů

Nový test `tests/test_credential_provisioning_controller.cpp` pokrývá:

- odmítnutí provisioningu bez fyzické autorizace;
- úspěšné prvotní nastavení a vynulování vstupního bufferu;
- jednorázovost autorizace a požadavek na fyzické uvolnění před novým pokusem;
- odmítnutí neplatného vstupu bez zápisu;
- obnovu osiřelých staging záznamů po přerušeném prvním zápisu;
- fail-closed chování při chybě mazání;
- bezpečné pokračování obnovy po přerušení již probíhající obnovy;
- neprovádění automatického mazání při `IO_ERROR`;
- změnu již platných credentials přes neaktivní slot;
- expiraci servisní relace přesně na 60 s včetně přetečení časovače.

Výsledky ověřeného hostitelského běhu:

- `./tests/run_native_tests.sh`: **49/49 testovacích programů PASS**.
- `sh tests/.run_native_tests_asan.sh`: **49/49 testovacích programů PASS**.
- kontrola stability peer boot identity: PASS.
- kontrola W5500 LinkOFF/LinkON obnovy: PASS.
- hlavičky CredentialStore a provisioning controlleru jsou bajtově shodné ve `common/`, `ESP32_1/src/` a `ESP32_2/src/`.

## 6. Co zůstává otevřené

- Konkrétní typ a revize obou desek, servisní vstup a elektrické zapojení nejsou ověřeny. GPIO nebylo přiděleno.
- Není implementován deskový adapter, který vzorkuje fyzický vstup.
- Není zvolen ani implementován servisní transport. USB/Serial samotné není fyzickou autorizací.
- Controller není zapojen do `.ino` ani do AP/web inicializace. Start AP tedy zatím není tímto modulem blokován.
- Stávající výpisy hesel do Serial a zobrazení AP hesla v HTML ESP32 #1 zatím zůstávají otevřeným bezpečnostním nálezem; nebyly odstraněny.
- `NvsCredentialBackend` byl testován s hostitelským Preferences stubem, nikoli na reálné NVS.
- NVS Encryption/Flash Encryption, Arduino-ESP32 build, skutečný power-cut, servisní transport a hardware nebyly ověřeny.
- HTTP Basic Auth přes prosté HTTP zůstává nešifrovaný; běžné webové rozhraní nelze považovat za bezpečný provisioning transport.

**Závěr:** Přenositelné jádro fyzicky autorizovaného provisioningu a obnovy je připravené a hostitelsky testované. Nesmí se však označit za produkční bezpečnostní opravu, dokud nebudou doplněny a ověřeny skutečný fyzický vstup, servisní transport, AP boot gate a ochrana uložených dat.
