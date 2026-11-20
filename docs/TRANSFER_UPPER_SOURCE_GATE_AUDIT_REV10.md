# HOME WATER CONTROLLER — audit vstupních podmínek transferu horní nádrže

Revize balíčku: 1.00 AUDIT DRAFT REV11  
Datum: 7. 11. 2026  
Závazná reference: HOME WATER CONTROLLER MASTER 1.3  
Rozsah: statická kontrola `TransferLogic`, `UpperTankLimits`, `ESP32_1.ino` a návaznosti na stavové zprávy ESP32 #2.  
Změny firmware: žádné.

## 1. Shrnutí

Základní bezpečnostní chování transferové simulace je fail-closed: bez platné/čerstvé hladiny horní nádrže, při ztrátě peer dat, neplatných limitech nebo neshodě limitů mezi jednotkami se transfer horní → dolní nespustí nebo se běžící simulace ukončí. Přenesené množství se vyhodnocuje podle změny hladiny dolní nádrže, nikoli podle času.

Byla však nalezena jedna důležitá nesrovnalost mezi samostatným vyhodnocením přepouštěcího ventilu a vlastní transferovou logikou.

## 2. TRANSFER-REV11-01 — práh `transferValveOpenCm` není součástí prechecku transferu

**Závažnost: VYSOKÁ před aktivací fyzického ventilu; návrhová nejednoznačnost, nikoli potvrzené porušení MASTER 1.3.**

Aktuální implementace:

- `UpperTankLimits::evaluateUpperTankTransferValve()` používá `transferValveOpenCm` jako práh pro otevření logického povolení a `householdReserveCm` jako spodní mez pro jeho zavření (hystereze).
- Dashboard zobrazuje „Hladina pro zahájení přepouštění“ a „Vyhodnocení přepouštění“.
- `TransferLogic::precheckFault()` však jako minimum zdroje horní nádrže kontroluje pouze `householdReserveCm`.
- `updateTransferSimulation()` předává do `TransferLogic` `householdReserveCm`, ale nepředává výsledek `upperValveDecision` ani `transferValveOpenCm`.
- Příkaz `VALVE_OPEN_UPPER_TO_LOWER` vzniká při stavu `TRANSFER_RUNNING`, nezávisle na `upperValveDecision`.

Praktický důsledek: pokud je hladina horní nádrže vyšší než rezerva domácnosti, ale nižší než `transferValveOpenCm`, dashboard může ukazovat logické „zakázáno“, zatímco ručně vyžádaná transferová simulace může přejít do `RUNNING` a požadovat otevření horní trasy.

### Vztah k MASTER 1.3

MASTER 1.3 vyžaduje uživatelsky spuštěný transfer, bezpečnou minimální úroveň/rezervu horní nádrže a blokaci při nespolehlivých datech. Výslovně ale nepopisuje další parametr `transferValveOpenCm` ani to, zda jde o samostatný provozní práh, nebo pouze o doporučující/hysterezní stavovou indikaci.

Proto se tato revize nerozhoduje, zda je správná hodnota `householdReserveCm`, nebo `transferValveOpenCm`. To by znamenalo změnu schválené logiky bez dostatečného rozhodnutí. Před fyzickým výstupem musí být jednoznačně stanoveno:

1. zda uživatelský transfer smí začít při hladině nad rezervou domácnosti i pod `transferValveOpenCm`;
2. zda `transferValveOpenCm` je pouze indikace „dostatečná zásoba pro zahájení“, nebo tvrdá bezpečnostní podmínka;
3. zda při poklesu hladiny pod práh během transferu stačí pokračovat až k `householdReserveCm`, nebo se má transfer zastavit dříve.

### Doporučené technické řešení po potvrzení pravidla

Přenést jednoznačný výsledek horního zdrojového prechecku do `TransferInputs` a používat stejnou podmínku při startu i během transferu. Nepoužívat pouze dashboardový `upperValveDecision` jako volně sdílenou globální proměnnou. Přidat hraniční testy těsně pod/na/nad oběma prahy a test přechodu prahu během běžícího transferu.

## 3. TRANSFER-REV11-02 — `pumpProtectionReady` je pouze diagnostika

**Závažnost: OTEVŘENÉ ROZHODNUTÍ; neblokovat bez rozlišení hydraulické role.**

ESP32 #2 přenáší `pumpProtectionReady` a alarmy. ESP32 #1 zobrazuje tento stav, ale `TransferLogic` jej nepoužívá. MASTER 1.3 požaduje zachování místní ochrany čerpadla horní nádrže při výpadku komunikace a předávání stavu ochrany mezi jednotkami.

Není automaticky správné blokovat transfer horní → dolní při `pumpProtectionReady == false`: horní transfer může být hydraulicky samostatná ventilová cesta, zatímco `pumpProtectionReady` popisuje ochranu čerpadla pro zásobování domu. Tyto funkce se nesmějí sloučit bez ověření skutečné hydrauliky.

Před fyzickou integrací je nutné definovat, zda je transfer horní → dolní nezávislý na čerpadle horní nádrže. Pokud ano, `pumpProtectionReady` zůstane diagnostický stav a lokální ochrana čerpadla běží samostatně na ESP32 #2. Pokud ne, musí být autentizovaný stav ochrany součástí startovacího i průběžného prechecku.

## 4. Ověřené pozitivní vlastnosti současné simulace

- Uživatel musí zadat směr, litry a výslovně spustit transfer.
- Směry jsou omezené na studna → dolní a horní → dolní.
- Zastaralá/neplatná peer hladina způsobí komunikační chybu.
- Neshoda checksumu horních limitů blokuje horní transfer.
- Pokles zdroje na/z pod nastavenou rezervu během běhu způsobí bezpečnostní ukončení.
- Neplatná nebo zastaralá hladina dolní nádrže transfer blokuje/ukončí.
- Cílové množství se odvozuje ze změny hladiny dolní nádrže.
- Po restartu se přerušený transfer automaticky neobnoví; vyžaduje obslužné potvrzení.
- Všechny tyto vlastnosti jsou v této revizi pouze logikou/simulací; fyzické výstupy nejsou aktivní.

## 5. Další pořadí

1. Zachovat obě zjištění jako otevřené brány, dokud nebude ověřena hydraulická funkce horní nádrže a ventilu.
2. Dokončit bezpečnou autentizaci peer dat podle předchozího auditu.
3. Před fyzickým řízením sjednotit rozhodnutí o horním transferu do jednoho prechecku a jedné průběžné ochrany.
4. Přidat testy hraničních hodnot a regresní testy stavového automatu.
5. Teprve poté navrhnout a testovat fyzický výstup ventilu.

## 6. Ověřovací hranice

Tento audit je statická analýza zdrojů. Neprokazuje průtok, chování ventilu, skutečnou hydraulickou vazbu čerpadla a horního transferu ani chování na ESP32. Firmware ani hardware nebyly změněny.
