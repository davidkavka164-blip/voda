# Bezpečnostní audit přihlašovacích údajů — revize 0.76

Datum: 2026-11-04  
Rozsah: statická kontrola obou hlavních sketchů ESP32 #1 a ESP32 #2. Firmware se v této revizi nemění.

## Nálezy

### P0-4 — ESP32 #1 zobrazuje heslo konfiguračního AP v běžném webovém rozhraní

`firmware/ESP32_1/ESP32_1.ino`, funkce generující hlavní HTML stránku (řádek v okolí původního 512): při aktivním AP přidá do stránky hodnotu `apPassword`.

Důsledek: každý, kdo získá přístup k webové stránce zařízení, může získat heslo AP. Heslo AP není vhodné zobrazovat v dashboardu ani v běžné autentizované či neautentizované HTML odpovědi.

### P0-5 — hesla ESP32 #1 se vypisují do Serial Monitoru

`firmware/ESP32_1/ESP32_1.ino`:
- start konfiguračního AP vypisuje `apPassword`;
- vytvoření webového administrátorského hesla vypisuje heslo včetně uživatelského jména.

Důsledek: přístup k sériovému logu nebo jeho kopii znamená kompromitaci přístupových údajů.

### P0-6 — hesla ESP32 #2 se vypisují do Serial Monitoru

`firmware/ESP32_2/ESP32_2.ino`, funkce lokálního AP: vypisuje heslo AP i administrátorské heslo.

Důsledek: stejný jako u ESP32 #1.

## Proč se výpisy v této revizi nemažou

Pouhé odstranění výpisu by mohlo uživatele uzamknout mimo lokální rozhraní, protože bezpečný a otestovaný způsob prvotního předání, obnovy a změny hesel zatím není implementován. Současně by bez alternativního workflow nebylo možné spolehlivě obnovit přístup po ztrátě hesla.

Do implementace bezpečného provisioningu se proto nesmí firmware nasazovat jako bezpečnostně dokončený. Nález je otevřený a současné výpisy jsou výslovně považovány za dočasné riziko, nikoli za přijatelnou produkční vlastnost.

## Požadovaná oprava

1. Navrhnout lokální provisioning a obnovu přístupu bez zobrazení existujícího hesla.
2. Zajistit fyzické potvrzení servisního režimu a jeho časové omezení.
3. Nové heslo předat pouze jednorázově v lokální servisní relaci; nikdy jej neukládat do Serial logu ani běžné webové stránky.
4. Po úspěšném nastavení odstranit všechny výpisy hesel z obou sketchů a odstranit vykreslení `apPassword` z dashboardu.
5. Ověřit obnovu přístupu po restartu, ztrátě NVS a neúspěšném zápisu, aby oprava nezpůsobila uzamčení zařízení.
6. Ověřit obě jednotky na cílovém hardwaru. Do té doby ponechat peer command path default-deny a fyzické výstupy neaktivní.

## Stav

- Statická kontrola zdrojů: nálezy potvrzeny.
- Firmware změny: žádné.
- Arduino build a hardware: neprovedeny.
- Bezpečnostní stav: P0 otevřeno; nepovažovat přístupové údaje za důvěrně chráněné.
