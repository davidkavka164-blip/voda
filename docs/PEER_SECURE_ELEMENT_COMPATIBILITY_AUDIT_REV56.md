# HOME WATER CONTROLLER — posouzení secure elementu pro peer HMAC REV56

Datum: 14. 11. 2026  
Vychází z: REV52–REV55 a aktuálních zdrojů Microchip CryptoAuthentication  
Rozsah: statická analýza kompatibility existujícího handshake s externím secure elementem. Žádná změna firmware ani hardware.

## 1. Výsledek

Externí secure element s interním 32B HMAC klíčem je technicky vhodnější cesta než pokus o NVS Encryption na stock Arduino-ESP32 1.0.6.

Jako kandidátní rodinu pro další ověření označuji **Microchip ATECC608C v konfigurovatelné variantě**, nikoli automaticky konkrétní hotový Trust&GO profil. Microchip uvádí ATECC608C jako produkt v produkci a pro nové návrhy doporučuje řadu C; zařízení podporuje SHA-256/HMAC a chráněné sloty. Datový list ATECC608C-TFLXTLS popisuje 32B Secret Key slot použitelný pro SHA/HMAC, jehož obsah nelze běžně číst. Zápis klíče vyžaduje encrypted write s I/O protection key.

Toto je pouze kandidát. Konkrétní ordering code, přednastavená konfigurace slotů, možnost vlastního provisioningu a dostupnost u dodavatele musí být potvrzeny před nákupem.

## 2. Kompatibilita s existujícím handshake

Aktuální `PeerHandshake.cpp` používá dlouhodobý 32B shared secret pro tři HMAC výpočty nad zprávou `domain || transcript`:

| Operace | Doména | Délka vstupu |
|---|---:|---:|
| CHALLENGE tag | `HWC-HS-CHALLENGE-V1` | 66 B |
| FINISH tag | `HWC-HS-FINISH-V1` | 63 B |
| SESSION key | `HWC-SESSION-V1` | 61 B |

ATECC608C SHA/HMAC přijímá zprávu po blocích 1–64 B. Všechny tři vstupy se tedy vejdou do jedné operace; CHALLENGE se musí rozdělit na 64 B + 2 B. Secure element vrátí 32B HMAC výsledek.

Z toho plyne, že stávající protokol lze pravděpodobně zachovat, ale stávající implementaci nelze pouze obalit existujícím `PeerSecretVault`:

- `PeerSecretVault::load()` dnes vrací celý dlouhodobý secret do RAM ESP32.
- `PeerHandshake.cpp` předává secret jako pointer do softwarového `hmacSha256()`.
- Pro secure element je nutné nahradit tento přístup abstrakcí typu `IPeerMacProvider`, která přijme zprávu a vrátí HMAC, aniž by vydala klíč.
- Stejný provider musí obsloužit tvorbu CHALLENGE, ověření CHALLENGE, tvorbu FINISH a odvození SESSION key.
- Odvozený session key může zůstat v RAM jako krátkodobý klíč; následné směrové status HMAC klíče se odvozují z této session key stávající funkcí `derivePeerDirectionalKeys()`.

Host testy mohou nadále používat softwarový test provider s testovacím klíčem. Produkční backend musí být výhradně secure-element provider a při chybě zařízení musí selhat uzavřeně.

## 3. Provisioning a ochrana klíče

Obě ESP32 potřebují stejný 32B peer shared secret v odpovídajícím interním slotu secure elementu. Klíč nesmí být přenášen přes běžnou UDP/Wi-Fi síť ani ukládán do Preferences.

Před produkčním použitím je nutné definovat důvěryhodný provisioning postup pro oba čipy:

1. Zvolit přesný konfigurovatelný typ secure elementu a konfiguraci slotů.
2. Zvolit způsob bezpečného prvotního zápisu stejného klíče do obou čipů.
3. Ověřit HMAC challenge/response s testovacím klíčem před nevratným uzamčením slotů.
4. Ověřit, že firmware umí pouze vyžádat HMAC a nedokáže secret přečíst.
5. Ověřit obnovu po výpadku napájení a fail-closed chování při odpojeném/nefunkčním secure elementu.
6. Teprve po ověření uzamknout konfiguraci/sloty podle doporučeného provisioning postupu výrobce.

I/O protection pro I²C může dále chránit citlivé příkazy a odpovědi na fyzické sběrnici, ale vyžaduje vlastní párovací/provisioning krok. Nesmí se předpokládat, že je automaticky aktivní.

## 4. Elektrické a integrační neznámé

- V aktuálním firmware nebylo nalezeno existující použití `Wire` / I²C.
- Konkrétní GPIO pro SDA/SCL zatím nejsou určeny; nesmí kolidovat s již přidělenými signály, W5500 SPI ani budoucím rozšířením.
- Je nutné ověřit napájecí napětí modulu, I²C pull-up rezistory, logické úrovně, adresu a délku vedení.
- Není ověřen překlad CryptoAuthLib na uživatelově instalaci Arduino IDE 1.8.19 / Windows 7 ani její kompatibilita s přesnou verzí Arduino-ESP32 core.
- Není vybrán ani objednán žádný modul.

## 5. Doporučená implementační posloupnost

1. Potvrdit přesnou verzi Arduino-ESP32 core instalovanou uživatelem.
2. Vybrat konkrétní ATECC608C variantu a ověřit její slot policy a provisioning.
3. Samostatně ověřit knihovnu CryptoAuthLib a I²C komunikaci na testovací desce, bez připojení k čerpadlům/ventilům.
4. Navrhnout `IPeerMacProvider` a upravit handshake API tak, aby dlouhodobý klíč nikdy neopustil secure element.
5. Přidat testy provideru, chyb zařízení, nesprávného MAC, výpadků komunikace a restartů.
6. Teprve potom zapojit handshake do produkčních UDP smyček a autentizovaný receiver před `PeerStatusMonitor`.
7. Ověřit reálnou komunikaci, ztráty/duplikace/timeouty/restarty/link-loss. Až po úspěchu samostatně zvážit změnu bezpečnostní brány.

## 6. Bezpečnostní stav zůstává

- Žádný firmware ani hardware nebyl změněn.
- Produkční handshake a autentizovaný receiver nejsou zapojeny.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.
- Běžný automatický i servisní horní transfer zůstává blokován.
- Nouzové ruční otevření zůstává oddělené, výslovně potvrzované a obsluhou dozorované.

## 7. Zdroje

- Microchip ATECC608C – stav In Production, funkce secure key storage, SHA-256/HMAC: https://www.microchip.com/en-us/product/atecc608c
- Microchip ATECC608C-TFLXTLS data sheet – 32B Secret Key slot, SHA/HMAC a encrypted write: https://ww1.microchip.com/downloads/aemDocuments/documents/SCBU/ProductDocuments/DataSheets/ATECC608C-TFLXTLS-CryptoAuthentication-Data-Sheet-DS40002515.pdf
- Microchip ATECC608C migration note – doporučení pro nové návrhy: https://onlinedocs.microchip.com/oxy/GUID-EC688158-B182-4FB8-9DA1-4B9EFB5BDCE1-en-US-1/GUID-25909627-AD26-4235-BAE4-A5A592AC80D5.html
- Microchip CryptoAuthLib – podporovaná zařízení a HAL: https://github.com/MicrochipTech/cryptoauthlib
- Microchip ATECC608B/C SHA HMAC command description: https://ww1.microchip.com/downloads/aemDocuments/documents/SCBU/ProductDocuments/DataSheets/ATECC608C-TFLXTLS-CryptoAuthentication-Data-Sheet-DS40002515.pdf
