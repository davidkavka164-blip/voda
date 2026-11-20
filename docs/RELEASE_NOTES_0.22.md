# HOME WATER CONTROLLER — vývojová revize 0.22

## Nouzové povolení čerpadla horní nádrže
- Rozšířen `UpperTankProtection` o explicitní příkaz nouzového režimu, potvrzení fyzické kontroly hladiny a samostatný příkaz pro jeho ukončení.
- Chování odpovídá zásadě použité u studny: nouzový režim není aktivní bez potvrzení; nelze jím obejít dříve aktivovanou kritickou blokaci, neplatnou konfiguraci ani chybu NVS.
- Při nouzovém povolení je měření stále označeno jako neplatné/upozornění; uživatel tak nesmí zaměnit nouzový provoz za normální ochranu.
- Po restartu se nouzový režim neobnovuje. Po návratu snímače je nutné nouzový režim výslovně ukončit; pokud snímač zůstává neplatný, normální povolení je blokováno.
- Doplněny regresní testy: chybějící potvrzení, správné potvrzení, výpadek NVS, již aktivní kritická blokace a ukončení nouzového režimu.

## Důležité omezení
- Toto vydání doplňuje a testuje logiku, nikoli uživatelské ovládání nouzového režimu. ESP32 #2 zatím nemá dokončené lokální ovládací rozhraní a ESP32 #1 nemá bezpečně ověřený příkaz pro předání této volby. Nouzový režim proto není v této revizi dostupný uživateli přes hotové UI a nelze jej považovat za provozně dokončenou funkci.
- Fyzický výstup čerpadla zůstává deaktivovaný. Nouzové povolení nesmí být implementováno jako automatické zapnutí výstupu bez nezávislého fyzického bezpečnostního řetězce.

## Ověření
- `tests/run_native_tests.sh`: 16/16 nativních testovacích programů prošlo.
- `test_upper_tank_protection` dále ověřen samostatně s ASan/UBSan.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům; nejde o skutečný překlad pro ESP32.
- Společné zdrojové soubory synchronizovány do obou sketch `src` adresářů.
- Neprovedeno: Arduino-ESP32 build, hardware, NVS na zařízení, skutečný snímač/relé/čerpadlo a end-to-end komunikace.
