# HOME WATER CONTROLLER — hostitelský klient provisioningu

Revize: 0.92  
Datum: 2026-11-04  
Stav: klientská tvorba/validace rámce implementována a hostitelsky testována; transportní část připravena, ale zařízení-side Serial/GPIO adapter není integrován.

## 1. Co je implementováno

Soubor `tools/credential_provisioning_client.py` implementuje klientskou stranu wire formátu z revize 0.91:

- ověření délek AP názvu (8–32), AP hesla (8–63) a admin hesla (12–64);
- striktní printable ASCII bez NUL a řídicích znaků;
- sestavení SET_CREDENTIALS v1 rámce, big-endian délka a CRC-16/CCITT-FALSE;
- kontrolu pevné 9B odpovědi zařízení, včetně magic/verze/typu/délky/CRC;
- překlad stavových kódů, které nikdy neobsahují uložené údaje;
- volitelný PySerial transport s jedním zápisem a jednou odpovědí.

Hesla se zadávají přes `getpass`, nikoli jako argument příkazové řádky. Výchozí režim je náhled: rámec se pouze sestaví a ověří, nic se neodesílá. Klient nikdy nevypisuje hodnoty ani hex obsah rámce.

## 2. Bezpečnostní pojistka odesílání

Odesílání vyžaduje současně `--send`, `--port` a explicitní `--firmware-integrated`. Poslední přepínač je vědomé potvrzení operátora, že cílový firmware již obsahuje skutečný a otestovaný Serial/GPIO adapter. Revize 0.92 takový adapter **neobsahuje**; proto se do zařízení této revize nesmí odesílat a přepínač `--firmware-integrated` se nesmí používat.

Před odesláním klient požaduje druhé potvrzení přesné hodnoty `SEND`. Fyzické podržení servisního vstupu musí proběhnout na zařízení; klient ho neumí vytvořit ani prodloužit.

## 3. Omezení sériové linky

Sériový protokol přenáší hesla v plaintextu. CRC-16 chrání jen před náhodným poškozením, nikoli proti aktivní manipulaci ani odposlechu. Budoucí použití je dovoleno pouze přímým lokálním USB/UART kabelem, pod dohledem obsluhy, bez Bluetooth, sériového serveru, síťového převodníku nebo jiného bridge.

Klient před otevřením portu nastavuje DTR/RTS na neaktivní hodnoty; to může omezit automatický reset některých desek, ale nezaručuje jej. Otevírání COM portu, boot/reset chování a zvolený baud rate musí být ověřeny na přesném hardware.

Python nedává spolehlivou záruku bezpečného nulování immutable řetězců. Klient proto hesla nikdy nezapisuje do souboru ani logu, ale nelze tvrdit, že po skončení procesu zůstala v paměti nulová každá kopie.

## 4. Použití

Požadavky pro lokální testování (Python 3.6+):

```text
python tests/python/test_credential_provisioning_client.py
```

Výchozí náhled (zadá hesla skrytě, vytvoří rámec, ale nic neposílá):

```text
python tools/credential_provisioning_client.py
```

Testy na Windows lze spustit dávkovým souborem `tests\run_provisioning_client_tests.cmd`; vyžaduje instalovaný Python 3.6+ a dostupný příkaz `python`.

Pro budoucí integrovaný firmware je PySerial 3.5 volitelnou závislostí:

```text
python -m pip install -r tools/requirements.txt
```

Příklad příkazu pro budoucnost — **nepoužívat s aktuálním firmware 0.92**:

```text
python tools/credential_provisioning_client.py --send --port COM4 --baud 115200 --firmware-integrated
```

Název portu je příklad, nikoli doporučení pro konkrétní zařízení. Port, baud rate, reset linky a fyzický servisní vstup musí být nejprve potvrzeny.

## 5. Testování revize 0.92

Nové `tests/python/test_credential_provisioning_client.py` obsahuje 15 testů:

- referenční CRC vektor `123456789` → `0x29B1`;
- layout a CRC request rámce;
- společný golden wire vector, bajtově shodný s C++ parser testem;
- minimální a maximální délky (max. rámec 170 B);
- odmítnutí příliš krátkých/dlouhých hodnot;
- odmítnutí ne-ASCII a řídicích znaků;
- validaci správné i poškozené response;
- odmítnutí neplatné délky, typu a formátu response;
- test jednorázového serial write/flush/read přes fake transport;
- kontrola, že náhled nevypíše žádné heslo;
- kontrola, že reálné odeslání je blokováno bez explicitního potvrzení integrovaného firmware;
- ověření stavových textů bez credentials.

Výsledky:

- C++ nativní sada: 50/50 PASS;
- C++ ASan/UBSan: 50/50 PASS;
- Python klient: 15/15 PASS;
- Python bytecode compilation: PASS;
- peer boot identity: PASS;
- W5500 recovery: PASS.

## 6. Co zůstává otevřené

- Serial parser/service není připojen k `Serial`/UART v `.ino`.
- Není zvolen ani ověřen fyzický servisní GPIO a konektor.
- CredentialStore/NVS není zapojen do startu AP/web obou jednotek.
- Stávající výpisy AP/admin hesel do Serial a AP heslo v HTML ESP32 #1 zůstávají; nelze je odstranit před dokončením funkční obnovy přístupu.
- Host klient nebyl zkoušen na fyzickém COM portu a nedokáže komunikovat s aktuálním firmware.
- Arduino build, skutečná NVS, DTR/RTS reset chování, NVS/Flash Encryption a hardware nebyly ověřeny.

**Závěr:** Hostitelský klient je připraven pro další integrační etapu, ne pro provoz s aktuálním firmware. Stav není vydáván za funkční provisioning celé jednotky.

## Doplnění revize 0.93 — čekání na READY

Host klient nově před odesláním čeká na platný 9B RESULT frame se stavem `0x01` (`SESSION_READY`). Request SET_CREDENTIALS odešle až poté, co ověří magic, verzi, typ, délku a CRC READY. Před READY dovolí omezený šum/boot logy, které může `Serial` poslat. Pokud READY nepřijde nebo vrátí jiný status, credentials se neodešlou.

Tento postup odpovídá `CredentialProvisioningBoardAdapter.h`, který před otevřením session zahazuje data z RX, spotřebuje fyzickou autorizaci a teprve potom vyšle READY. Adapter zatím není připojen k `.ino` ani ke konkrétnímu GPIO/UART. Proto se `--firmware-integrated` stále nesmí použít s žádným současným sketch firmware 0.93.
