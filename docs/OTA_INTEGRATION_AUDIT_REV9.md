# HOME WATER CONTROLLER — audit OTA a aktualizační bezpečnosti

Revize balíčku: 1.00 AUDIT DRAFT REV10  
Datum: 7. 11. 2026  
Závazná reference: HOME WATER CONTROLLER MASTER 1.3  
Rozsah: statická kontrola skutečných OTA call-sites v `ESP32_1.ino` a `ESP32_2.ino`, autentizace HTTP, bezpečnostní podmínky a požadavky na koordinaci obou jednotek.  
Změny firmware: žádné. GPIO, relé, ventily a čerpadla zůstávají beze změny.

## 1. Shrnutí

V REV9 jsou dvě samostatné lokální OTA cesty: ESP32 #1 přes hlavní web a ESP32 #2 přes vlastní web/AP. Nejde o koordinovanou aktualizaci obou jednotek. Obě cesty zapisují aplikaci přes Arduino `Update`; fyzické OTA nebylo ověřeno.

Největší otevřené riziko není samotný zápis, ale ověření identity firmwaru, bezpečné přerušení provozních funkcí a důvěryhodnost uploadu. Samotná přípona `.bin`, HTTP Basic Auth a `Update.end(true)` nejsou důkazem, že jde o správný a důvěryhodný firmware pro konkrétní roli ESP32.

## 2. Zjištění

### OTA-REV9-01 — Chybí jednoznačné ověření cílové role firmware

**Závažnost: BLOKUJÍCÍ před reálným OTA nasazením.**

- ESP32 #1 přijímá upload přes `/update`; kontrola přípony `.bin` není v upload handleru provedena.
- ESP32 #2 kontroluje příponu `.bin`, ale přípona neurčuje, zda je image určena pro ESP32 #2.
- Obě jednotky používají stejnou rodinu ESP32. Obecná validace obrazu čipem sama o sobě nerozlišuje aplikační roli #1 a #2.
- Ve zdrojích nebyl nalezen aplikační manifest s identitou zařízení, kompatibilitou protokolu, verzí schématu NVS a kryptografickým podpisem, který by se ověřil před aktivací.

**Požadovaný návrh:** definovat podepsaný manifest pro každou image: `device_role`, `firmware_version`, `minimum_peer_protocol`, `nvs_schema_min/max`, délka obrazu a SHA-256. Podpis musí být ověřen důvěryhodným veřejným klíčem uloženým ve firmware/ROM-konfiguraci; hash bez podpisu nestačí. Odmítnout image pro nesprávnou roli ještě před `Update.begin()`.

### OTA-REV9-02 — Upload není kryptograficky autentizován

**Závažnost: BLOKUJÍCÍ před použitím v nedůvěryhodné síti.**

- Web používá HTTP na portu 80 a HTTP Basic Auth. Basic Auth přes nešifrované HTTP neposkytuje důvěrnost přihlašovacích údajů; přenos je možné na síti odposlechnout.
- Rate limiter omezuje opakované neúspěšné pokusy, ale nešifruje spojení ani nepotvrzuje integritu firmwaru.
- V OTA cestě nebylo nalezeno ověření digitálního podpisu image.

**Důsledek:** OTA lze považovat nanejvýš za lokální servisní funkci na důvěryhodné oddělené síti. Nesmí být vystavena internetu ani nedůvěryhodné Wi-Fi. Před vzdáleným OTA je nutné řešit autentizaci image a zabezpečení transportu; VPN sama nenahrazuje podpis firmwaru.

### OTA-REV9-03 — ESP32 #2 blokuje OTA při běžném `pumpPermit == true`

**Závažnost: VYSOKÁ — provozní/policy chyba k vyjasnění před nasazením.**

Handler ESP32 #2 odmítá OTA, pokud `upperPumpProtectionState.pumpPermit` nebo `emergencyModeActive` platí. `pumpPermit` znamená, že místní ochrana dovoluje běžný provoz čerpadla; neznamená, že čerpadlo právě běží. Po validním měření nad obnovovací hranicí se tedy OTA typicky zablokuje právě v normálním zdravém stavu. Naopak při neplatném snímači nebo aktivní kritické blokaci může být `pumpPermit == false` a samotná tato podmínka OTA povolí.

Toto není správně definovaná servisní podmínka. MASTER 1.3 vyžaduje, aby během OTA nedošlo k nebezpečnému otevření ventilu ani nekontrolovanému pokračování transferu; neříká, že OTA má být dostupná pouze při blokovaném povolení čerpadla.

**Doporučení:** později zavést explicitní servisní/OTA stav s potvrzením obsluhy a koordinací s ESP32 #1. Nepoužívat `pumpPermit` jako náhradu za informaci o skutečném běhu čerpadla. Dokud neexistuje ověřený hardware a definovaný servisní postup, ponechat výstupy neaktivní a nepřepisovat tuto logiku naslepo.

### OTA-REV9-04 — ESP32 #1 nemá provozní interlock pro budoucí transfer

**Závažnost: BLOKUJÍCÍ před fyzickým napojením ventilů.**

ESP32 #1 zahajuje `Update.begin()` po webové autentizaci. V handleru není podmínka, která by odmítla OTA při aktivním transferu nebo požadovala bezpečné potvrzení stavu ventilů. V aktuální revizi jde pouze o simulaci a fyzické výstupy jsou vypnuté, takže nejde o současné fyzické nebezpečí. Před aktivací ventilů však musí být OTA vstup blokován při transferu a musí být vyžadováno bezpečné ukončení/uzavření trasy.

Protože všechny elektrické ventily ovládá ESP32 #1, právě hlavní jednotka musí zajistit, že update ESP32 #2 nemůže vést k nebezpečnému otevření trasy. Ztráta peer komunikace během aktualizace #2 musí být vyhodnocena jako komunikační porucha a případný běžící transfer musí přejít do bezpečného ukončení.

### OTA-REV9-05 — Aktualizace obou jednotek není koordinována

**Závažnost: VYSOKÁ — neúplnost vůči MASTER 1.3.**

- ESP32 #1 má vlastní `/update` a výslovně oznamuje, že OTA ESP32 #2 zatím není implementována z hlavního webu.
- ESP32 #2 má samostatnou lokální `/update` cestu.
- Není implementováno řízení pořadí aktualizace, čekání na restart peeru, ověření nové verze po restartu, kompatibility protokolu ani bezpečný návrat při neúspěchu.

**Doporučený postup:** nejprve bezpečně aktualizovat ESP32 #2, ověřit její restart a kompatibilní status/handshake; teprve poté případně aktualizovat ESP32 #1. Přesné pořadí musí být potvrzeno podle kompatibility protokolu. Při nekompatibilní verzi nebo ztrátě peeru musí být transfer zablokován, nikoli automaticky obnoven.

### OTA-REV9-06 — Zachování NVS a návrat po chybě nejsou ověřeny na cílové platformě

**Závažnost: BLOKUJÍCÍ před fyzickým nasazením.**

`Update` zapisuje aplikační image do OTA partition. Ve zdrojích není prokázán reálný test zachování všech používaných namespaces, kalibrací, ochranných limitů, transfer journalu a credentials po OTA. Není rovněž doložena funkční rollback/boot-confirmation politika cílového bootloaderu a partition table.

**Požadované testy na přesné verzi core a testovací desce:** zachování všech NVS namespaces; přerušení uploadu; chybný/truncated image; image pro nesprávnou roli; výpadek napájení při zápisu; první boot nové image; záměrné selhání bootu; rollback/servisní obnova; peer kompatibilita po aktualizaci. Žádný test nesmí měnit produkční eFuse.

## 3. Shoda s MASTER 1.3

| Požadavek MASTER 1.3 | Stav REV9 |
|---|---|
| Webový upload `.bin` | Implementován odděleně na obou jednotkách; neověřen na HW |
| Ověření vhodnosti firmware | Nedostatečné: chybí role/manifest/podpis |
| Průběh a výsledek | Základní upload status existuje; fyzicky neověřen |
| Zachování NVS | Předpokládáno, nikoli prokázáno na cílovém HW |
| Bezpečné řešení obou ESP32 | Nesplněno; dvě oddělené OTA cesty bez koordinace |
| Během OTA #2 žádné nebezpečné otevření/nekontrolovaný transfer | Fyzické výstupy vypnuté; budoucí interlock není integrován |
| Lokální OTA přes AP bez internetu | Architektonicky dostupná cesta; fyzicky neověřena |

## 4. Doporučené pořadí další práce bez znalosti verze Arduino core

1. Navrhnout formát podepsaného firmware manifestu a kompatibilitní pravidla, bez závislosti na ESP-IDF API.
2. Navrhnout obecný OTA state machine: `IDLE → AUTHORIZED → PRECHECK → RECEIVING → VERIFYING → READY_TO_REBOOT → BOOT_VERIFY → COMPLETE/ROLLBACK`.
3. Definovat bezpečnostní kontrakt mezi OTA a transferem: zákaz nového transferu, bezpečné ukončení existujícího transferu, ověření zavření ventilu (nebo výslovné omezení bez zpětné vazby), žádný automatický restart transferu po návratu.
4. Definovat kompatibilitu peer protokolu a aktualizační pořadí obou jednotek.
5. Teprve po potvrzení core, partition table a skutečného HW zvolit platformní podpis/boot verification a rollback implementaci.

## 5. Co se v této revizi změnilo

- Přidán tento auditní dokument a aktualizovány release notes/README.
- Žádný firmware, pinout, NVS, partition table, eFuse, OTA handler ani výstupní hardware nebyl změněn.
- Nejsou tvrzeny testy OTA na zařízení.

## 6. Ověřovací hranice

Statická kontrola zdrojových call-sites a opakované hostitelské testy nejsou náhradou za Arduino-ESP32 build ani fyzický OTA test. Tento balíček zůstává auditním návrhem, nikoli firmwarem pro řízení reálných ventilů/čerpadel.
