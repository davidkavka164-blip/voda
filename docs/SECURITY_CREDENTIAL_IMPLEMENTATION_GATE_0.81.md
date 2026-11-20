# HOME WATER CONTROLLER — implementační brána CredentialStore

Revize: 0.82  
Datum: 2026-11-04  
Stav: statická analýza a integrační plán; firmware beze změny.

## 1. Rozhodnutí

CredentialStore zatím není bezpečné zapojit do startu AP. Důvodem není absence call-sites (ty jsou přítomné), ale chybějící ověřená cesta prvotního provisioningu a obnovy. Prosté odmítnutí startu AP při prázdném NVS by na nové jednotce znemožnilo konfiguraci. Automatické vygenerování a použití hesla při neověřeném zápisu by zachovalo současnou zranitelnost.

Tato revize proto nepřidává izolovaný helper ani neoznačuje návrh za implementaci. Nejprve se stanoví přesný stavový kontrakt a pořadí migrace.

## 2. Povinné stavy úložiště

- `VALID`: všechny povinné údaje existují, splňují délkové/znakové limity a úspěšně prošly read-back porovnáním.
- `EMPTY`: úložiště je dostupné a všechny požadované položky skutečně chybí. To je jediný stav, ve kterém smí být nabídnut prvotní provisioning.
- `CORRUPT`: část položek chybí, záznam je neplatný nebo hodnoty neprojdou validací. Nesmí se automaticky generovat náhrada ani spustit AP.
- `IO_ERROR`: NVS nelze otevřít, číst, zapisovat nebo ověřit. Nesmí se spustit AP.

Částečný stav (např. AP heslo existuje, admin heslo chybí) není `EMPTY`; je `CORRUPT`.

## 3. Rozdělení údajů

ESP32 #1 používá oddělené namespaces `hwc_ap`, `hwc_auth` a `hwc_wifi`. Implementace musí ověřit AP a admin credentials samostatně, ale výsledek pro start webového AP je validní pouze tehdy, jsou-li validní oba povinné záznamy. Domácí Wi-Fi údaje jsou samostatná konfigurace a nesmějí být podmínkou lokálního servisního AP.

ESP32 #2 ukládá AP SSID, AP heslo a admin heslo do `hwc_u2ui`. Všechny povinné položky musí být validní a read-back ověřené; částečný záznam je `CORRUPT`.

## 4. Bezpečný migrační postup

1. Zjistit existující stav NVS bez vypisování hodnot.
2. Pokud jsou všechna povinná data validní, ověřit je read-backem a ponechat dosavadní přístup funkční.
3. Pokud je stav `EMPTY`, spustit pouze explicitní prvotní provisioning, jehož fyzické oprávnění bude samostatně ověřeno.
4. Pokud je stav `CORRUPT` nebo `IO_ERROR`, nespouštět AP s náhradními údaji; vyžadovat servisní obnovu.
5. Nové hodnoty zapsat do staging klíčů, znovu načíst a porovnat; až potom atomicky přepnout aktivní verzi. Pokud platformní NVS API neposkytne atomický commit, použít verzované dva sloty s generací a kontrolním součtem a testovat každý bod výpadku.
6. Po ověření nové aktivní sady lze odstranit starou sadu. Při chybě musí být zachována poslední ověřená sada.
7. Teprve po úspěšném ověření provisioning/obnovy odstranit tajemství ze Serial výpisů a HTML.

Kontrolní součet chrání proti náhodnému poškození, nikoli proti úmyslné změně. Ochrana důvěrnosti uložených hesel zůstává samostatným blokátorem NVS/Flash Encryption.

## 5. Startovací podmínky

- ESP32 #1: `startConfigAp()` nesmí zavolat `WiFi.softAP()`, dokud AP password i admin password nejsou `VALID`.
- ESP32 #2: `startLocalWeb()` nesmí zavolat `WiFi.softAP()` ani `localWebServer.begin()`, dokud AP SSID, AP password a admin password nejsou `VALID`.
- Při `CORRUPT`/`IO_ERROR` se nesmí použít heslo pouze v RAM.
- Při `EMPTY` se nesmí automaticky vygenerovat heslo a následně jej vypisovat do Serial; musí následovat samostatně autorizovaný provisioning.
- Žádné heslo, token ani obsah staging záznamu nesmí být součástí Serial logu, HTML, HTTP odpovědi ani chybové diagnostiky.

## 6. Povinné testy před integrací

1. Úplně prázdný NVS → `EMPTY`, AP zůstane vypnuté do autorizovaného provisioningu.
2. Všechny validní záznamy → `VALID`, AP může pokračovat.
3. Chybějící jediný údaj z jinak existující sady → `CORRUPT`, AP vypnuté.
4. Neplatná délka/znaky → `CORRUPT`.
5. NVS begin/read/write/read-back chyba → `IO_ERROR`, AP vypnuté.
6. Read-back mismatch → `CORRUPT`/chyba operace, stará aktivní sada zůstane zachována.
7. Výpadek napájení po každém jednotlivém zápisu staging/commit → po restartu buď stará `VALID` sada, nová `VALID` sada, nebo fail-closed; nikdy částečná sada.
8. ESP32 #1 i #2: ověřit, že při všech nevalidních stavech nedojde k `WiFi.softAP()` ani startu web serveru.
9. Staticky ověřit, že logy a HTML neobsahují formátovací cesty pro tajemství.
10. Ověřit zachování domácí Wi-Fi nastavení ESP32 #1 a všech ostatních NVS namespaces.

## 7. Otevřená rozhodnutí / blokátory

- Přesná deska a fyzický servisní vstup pro obě jednotky nejsou potvrzeny; GPIO se nepřiděluje.
- USB/Serial připojení samo o sobě není fyzické potvrzení servisního oprávnění.
- Produkční NVS Encryption + Flash Encryption nejsou ověřeny na cílovém toolchainu.
- HTTP Basic Auth přes HTTP port 80 neposkytuje šifrování; lokální web nesmí být vystaven nedůvěryhodné síti ani internetu.
- `String` objekty s hesly nelze považovat za bezpečně nulované; po implementaci je třeba posoudit jejich životnost a možnost použití pevně dimenzovaných bufferů. Bez ověření chování Arduino `String` nelze tvrdit bezpečné vymazání RAM.

## 8. Rozsah revize

- Firmware a pinout beze změny.
- Žádné NVS, partition table, eFuse ani hesla nebyly změněny.
- Nativní testy ověřují stávající přenositelné moduly; tato revize nepřidává CredentialStore testy.
- Peer command path zůstává default-deny a fyzické výstupy neaktivní.
