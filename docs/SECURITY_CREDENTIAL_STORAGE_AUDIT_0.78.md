# HOME WATER CONTROLLER — audit ukládání přístupových údajů

Revize: 0.78  
Datum: 2026-11-04  
Stav: audit zdrojového kódu; firmware se v této revizi nemění.

## 1. Rozsah

Kontrola inicializace AP a administrátorských hesel v `ESP32_1.ino` a `ESP32_2.ino`, zejména návratových hodnot NVS zápisů a chování při nedostupném úložišti. Navazuje na návrh `SECURITY_CREDENTIAL_PROVISIONING_DESIGN_0.77.md`.

## 2. Potvrzené nálezy

### CRED-01 — ESP32 #1: AP se může spustit s heslem, které není trvale uloženo

V `prepareConfigApCredentials()` se při neúspěšném `Preferences.begin()` vygeneruje heslo pouze do RAM a funkce pokračuje. `startConfigAp()` následně může AP spustit. Po restartu bude vygenerováno jiné heslo. To vytváří nekonzistentní a obtížně obnovitelný přístup.

Při neúspěšném `putString()` se navíc ponechá heslo v RAM a log výslovně žádá uživatele, aby si vzal hodnotu ze Serial Monitoru. Samotná kontrola návratové hodnoty neověřuje zpětné načtení a shodu.

### CRED-02 — ESP32 #1: administrátorské heslo se při prvním vytvoření vypisuje do Serial

`startWebAndNetwork()` generuje heslo, ukládá jej a následně jej vypisuje včetně hodnoty do Serial Monitoru. Při chybě `putString()` se hodnota v RAM vyprázdní, ale při úspěchu je tajemství zveřejněno v logu.

### CRED-03 — ESP32 #2: návratové hodnoty zápisů AP/admin údajů se ignorují

`startLocalWeb()` generuje `ap_name`, `ap_pass` a `admin`, ale návratové hodnoty `putString()` nekontroluje. Lokální AP se poté spouští s hodnotami v RAM bez ověření, že byly uloženy. Po restartu mohou být údaje jiné nebo chybět.

### CRED-04 — ESP32 #2: AP a administrátorské heslo se vypisují do Serial

`startLocalWeb()` vypisuje obě hesla v otevřeném textu. To odpovídá nálezům P0-5/P0-6 a musí být odstraněno v rámci bezpečné implementace provisioningu.

## 3. Bezpečný cílový stav

- Pokud nelze otevřít NVS, zapsat údaj, načíst jej zpět nebo ověřit shodu, lokální AP/web správa se nespustí s novým dočasným heslem.
- Žádné heslo se nevypisuje do Serial, HTML, HTTP odpovědi ani diagnostiky.
- Generování tajemství a jeho trvalé uložení tvoří jednu transakční operaci z pohledu aplikace: úspěch je až po read-back ověření.
- Částečně vytvořený záznam se při chybě zneplatní; chyba zneplatnění se hlásí obecně a stav zůstane blokovaný.
- Prvotní provisioning a obnova vyžadují fyzicky potvrzený servisní režim podle návrhu 0.77.

## 4. Co zatím nelze uzavřít

- Nejsou známy přesné typy/revize obou ESP32 desek, proto nelze bezpečně přidělit servisní GPIO.
- Není prokázána NVS Encryption ani Flash Encryption konfigurace cílového zařízení.
- Není ověřeno, že Arduino-ESP32 core v uživatelově prostředí podporuje zamýšlenou konfiguraci zabezpečeného flash.
- HTTP Basic Auth nad lokálním HTTP není šifrovaný transport; servisní provisioning proto nesmí být dostupný přes domácí LAN ani internet.

## 5. Požadované testy implementace

1. NVS begin fail: žádné AP/web UI, žádné heslo v logu.
2. Každý put fail: žádné AP/web UI, žádné heslo v logu.
3. Read-back fail a mismatch: fail-closed.
4. Částečný zápis a neúspěšná invalidace: trvalý blokovací stav.
5. Restart po každém kroku inicializace: žádná změna přihlašovacích údajů bez servisní obnovy.
6. Statická kontrola obou sketchů: žádný formátovaný log nesmí přijímat AP/admin heslo; dashboard nesmí vykreslit AP heslo.
7. Regresní test změny a obnovy po fyzickém servisním potvrzení.

## 6. Rozhodnutí pro další revizi

Nejdříve implementovat a hostitelsky otestovat bezpečný NVS helper s kontrolou write/read-back a fail-closed výsledkem. Teprve poté jej zapojit do obou sketchů a odstranit výpisy tajemství. GPIO servisního vstupu zůstává otevřenou hardwarovou závislostí; nesmí být domýšleno.

Peer command path zůstává default-deny. Fyzické výstupy zůstávají neaktivní.
