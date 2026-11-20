# HOME WATER CONTROLLER — audit skutečných míst použití přístupových údajů

Revize: 0.80  
Datum: 2026-11-04  
Stav: statický audit zdrojových sketchů; firmware beze změny.

## 1. Oprava chybného závěru revize 0.79

Revize 0.79 nesprávně uvedla, že v balíčku nejsou skutečné Wi-Fi/AP a webové call-sites. Kontrola souborů `firmware/ESP32_1/ESP32_1.ino` a `firmware/ESP32_2/ESP32_2.ino` prokazuje opak: oba sketch soubory obsahují inicializaci AP, webové routy, autentizaci i práci s `Preferences`.

Proto se blokátor „chybí síťové sketch soubory“ ruší. Není však pravda, že je bezpečné rovnou nasadit CredentialStore: skutečné call-sites jsou dostupné a současně obsahují níže uvedené nedostatky. Revize 0.79 zůstává v historii, její tvrzení o absenci call-sites je tímto dokumentem výslovně nahrazeno.

## 2. ESP32 #1 — doložená místa

Soubor: `firmware/ESP32_1/ESP32_1.ino`.

- Řádky 102–106: hesla jsou držena v objektech `String` (`apPassword`, `adminPassword`, `savedWifiPassword`).
- Řádky 235–239: webová autentizace používá `WebServer::authenticate("admin", adminPassword.c_str())`. Jde o HTTP Basic Auth; server je vytvořen jako běžný `WebServer` na portu 80, nikoli HTTPS.
- Řádek 512: hlavní HTML stránka po přihlášení přímo vypisuje `apPassword`.
- Řádky 1011–1017: SSID a heslo domácí Wi-Fi se ukládají přes `Preferences.putString`; návratové hodnoty se kontrolují, ale není provedeno zpětné načtení a porovnání obou hodnot.
- Řádky 1027–1044: heslo AP se načte z NVS, případně vygeneruje. Pokud `putString` selže, pokračuje se s heslem v RAM; při selhání `Preferences.begin` se rovněž vygeneruje RAM heslo.
- Řádky 1047–1058: AP se spustí i po výše uvedených chybách uložení. Heslo AP se vypisuje do Serial Monitoru.
- Řádky 1198–1211 (ověřeno statickým vyhledáním): administrátorské heslo se načítá/generuje a při úspěšném uložení vypisuje do Serial Monitoru. Při chybě otevření úložiště se autentizační údaj nastaví jako nedostupný.

## 3. ESP32 #2 — doložená místa

Soubor: `firmware/ESP32_2/ESP32_2.ino`.

- Řádky 87–93: webová autentizace používá `WebServer::authenticate` s administrátorským heslem.
- Řádky 269–302: `startLocalWeb()` načítá AP SSID, AP heslo a admin heslo z namespace `hwc_u2ui`.
- Řádky 276–288: při chybějících údajích generuje nové a zapisuje přes `putString`, ale ignoruje návratové hodnoty zápisů. Neprovádí read-back.
- Řádky 290–299: AP a web server se spouští bez ověření, že nově generované údaje byly trvale uloženy.
- Řádky 300–301: AP heslo i administrátorské heslo se vypisují do Serial Monitoru.
- HTML stavová stránka zobrazuje SSID a IP AP, nikoli však v kontrolovaném úseku heslo AP.

## 4. Společné a navazující bezpečnostní závěry

1. **P0: žádný start AP při neověřeném credential storage.** Obě jednotky musí odmítnout start AP, pokud NVS není dostupné, zápis selže, read-back selže nebo se hodnoty neshodují.
2. **P0: odstranit výpisy tajemství.** Serial Monitor není provisioning kanál. Nesmí obsahovat AP, admin ani domácí Wi-Fi hesla.
3. **P0: odstranit AP heslo z HTML ESP32 #1.** Běžná stavová stránka má zobrazovat pouze informaci, zda je AP aktivní a zda jsou credentials nakonfigurovány.
4. **P0: bezpečný provisioning/obnova.** Nové heslo se nesmí generovat automaticky při každém chybějícím/poškozeném záznamu. Stav EMPTY musí být odlišen od CORRUPT a IO_ERROR; obnova vyžaduje samostatné fyzicky potvrzené servisní oprávnění.
5. **P0: HTTP Basic Auth není šifrování.** HTTP na portu 80 neposkytuje důvěrnost ani integritu přihlašovacích údajů a obsahu. Nepovažovat přihlášení přes Basic Auth za bezpečné proti odposlechu na LAN. Dokud není vyřešen transportní threat model, webové rozhraní nesmí být vystaveno nedůvěryhodné síti ani internetu.
6. **NVS/Flash Encryption není potvrzeno.** Použití `Preferences` samo o sobě neprokazuje šifrování. Platformní audit 0.64 zůstává platný.

## 5. Bezpečné pořadí implementace

1. Nejprve vytvořit CredentialStore nad abstraktním backendem a otestovat EMPTY/VALID/CORRUPT/IO_ERROR, write/read-back/mismatch a přerušení mezi zápisy.
2. Připravit jeden společný call contract pro obě jednotky; AP start bude možný pouze po výsledku VALID.
3. Napojit ESP32 #1 a ESP32 #2 na skutečné call-sites; zatím ponechat všechny výkonové výstupy vypnuté.
4. Zavést fyzicky potvrzený servisní provisioning/obnovu. Konkrétní GPIO nevybírat bez potvrzení desek a jejich pinoutu.
5. Teprve po úspěšném ověření obnovy odstranit Serial výpisy a AP heslo z HTML.
6. Ověřit HTTP přístup pouze v rámci důvěryhodného lokálního segmentu, případně navrhnout bezpečnější transport. Router/VPN zůstává samostatná síťová vrstva.
7. Ověřit NVS Encryption a Flash Encryption na přesném toolchainu a cílových deskách.

## 6. Rozsah této revize

- Žádný firmware, pinout, partition table ani eFuse nebyly změněny.
- Žádné heslo nebylo vypsáno ani kopírováno do tohoto dokumentu.
- Statické nálezy vycházejí z uvedených řádků zdrojových souborů.
- Hostitelské nativní testy ověřují přenositelné moduly; samy o sobě neověřují Wi-Fi, WebServer, NVS šifrování ani chování na ESP32.
- Peer command path zůstává default-deny a fyzické výstupy neaktivní.
