# HOME WATER CONTROLLER — audit pořadí inicializace přístupových údajů

Revize: 0.82  
Datum: 2026-11-04  
Rozsah: statická kontrola ESP32_1.ino a ESP32_2.ino; bez změny firmware

## 1. Zjištění

Při kontrole skutečných call-sites ve verzi 0.81 byly potvrzeny následující rozdíly:

### ESP32 #1

1. `startWebAndNetwork()` načítá/generuje administrátorské heslo v namespace `hwc_auth`.
2. `prepareConfigApCredentials()` načítá/generuje AP heslo v namespace `hwc_ap`.
3. Při selhání `Preferences.begin()` pro AP namespace se přesto vygeneruje heslo pouze v RAM.
4. Při selhání `putString()` se heslo ponechá v RAM a `startConfigAp()` může následně spustit AP.
5. Před spuštěním AP není ověřeno zpětné načtení obou údajů ani jejich společná platnost.
6. Admin heslo se při prvním vytvoření vypisuje do Serial; AP heslo se vypisuje při každém startu AP.
7. `handleRoot()` v AP režimu zobrazuje AP heslo v HTML.

### ESP32 #2

1. `startLocalWeb()` načte tři hodnoty z `hwc_u2ui`.
2. Chybějící/neplatné hodnoty automaticky generuje a zapisuje, ale návratové hodnoty `putString()` nekontroluje.
3. Neprovádí read-back ověření.
4. I po neúspěšném zápisu může pokračovat přes `WiFi.softAP()` a `localWebServer.begin()`.
5. AP i admin heslo vypisuje do Serial.

## 2. Kritická nuance migrace

Prosté vložení kontroly `VALID` a odmítnutí AP při `EMPTY` by zablokovalo nové zařízení bez předchozí konfigurace. Zachování automatického generování by naopak ponechalo tajemství bez spolehlivého předání uživateli. Nesmí se proto provést izolovaná změna `if (!valid) return;` bez hotové provisioning cesty.

Stejně tak nelze odstranit výpisy hesel a současně ponechat automatické generování bez uživatelsky dostupného, autorizovaného předání nových přihlašovacích údajů.

## 3. Požadované pořadí budoucí integrace

1. CredentialStore načte úplný záznam a vrátí jeden ze stavů `VALID`, `EMPTY`, `CORRUPT`, `IO_ERROR`.
2. Pro `VALID` provede validaci formátu a read-back porovnání.
3. Pro `EMPTY` nabídne pouze explicitní provisioning po nezávisle ověřeném fyzickém oprávnění.
4. Provisioning vytvoří kompletní novou sadu ve staging slotu; po read-back ověření zapíše commit marker/generaci.
5. Při přerušení napájení se po restartu přijme pouze poslední kompletní ověřená sada. Částečná sada nesmí být aktivní.
6. Pro `CORRUPT` a `IO_ERROR` se AP ani web server nespustí; zachová se bezpečný lokální servisní stav bez síťového zpřístupnění.
7. Teprve po otestování prvotního nastavení, změny a obnovy se odstraní Serial výpisy a HTML zobrazení hesla.

## 4. NVS implementační poznámky

- `Preferences.putString()` vrací počet zapsaných bajtů; nenulový výsledek sám o sobě není důkazem, že se celý záznam bezpečně aktivoval.
- Zápis tří položek ESP32 #2 do jednoho namespace není automatická transakce. Je nutný staging/commit protokol a testy výpadku po každém zápisu.
- U ESP32 #1 jsou AP, admin a domácí Wi-Fi ve třech oddělených namespaces. Je nutné rozlišit kritické AP/admin údaje od nepovinné domácí Wi-Fi konfigurace.
- `EMPTY` smí být vráceno pouze tehdy, pokud jsou všechny povinné položky pro danou jednotku nepřítomné a NVS čtení proběhlo bez chyby.
- Částečný záznam je `CORRUPT`, nikoli `EMPTY`.
- Kontrolní součet řeší náhodné poškození, ne důvěrnost ani úmyslnou manipulaci. NVS/Flash Encryption zůstává samostatně neověřená.

## 5. Blokátory

- Není potvrzen fyzický servisní vstup a přesná deska/pinout obou jednotek. Žádný GPIO se nepřiděluje.
- USB/Serial přístup sám o sobě není fyzické potvrzení autorizace.
- Není ověřeno NVS Encryption ani Flash Encryption na skutečném cílovém hardware/toolchainu.
- HTTP Basic Auth na HTTP neposkytuje šifrování přenosu; webové rozhraní musí zůstat v důvěryhodné lokální síti.
- Arduino `String` neposkytuje prokázané bezpečné nulování kopie hesla v RAM.

## 6. Rozsah a ověření

- Firmware, piny, partition table, NVS a eFuse beze změny.
- Neproběhl Arduino build ani test na fyzickém ESP32.
- Stávající nativní testy ověřují přenositelné moduly, nikoli provisioning call-sites.
- Peer command path zůstává default-deny; fyzické výstupy jsou neaktivní.
