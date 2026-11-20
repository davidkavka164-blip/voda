# HOME WATER CONTROLLER — návrh bezpečného provisioningu přístupových údajů

Revize návrhu: 0.77  
Datum: 2026-11-04  
Stav: návrh pro implementaci; firmware se v této revizi nemění.

## 1. Cíl a rozsah

Vyřešit nálezy P0-4 až P0-6 z revize 0.76: zobrazení hesla AP ESP32 #1 v dashboardu a výpisy hesel AP/admin obou jednotek do Serial Monitoru. Návrh musí zachovat možnost prvotního přístupu, změny hesel i obnovy po ztrátě přihlašovacích údajů.

Tento dokument neurčuje neověřené GPIO ani nepovoluje síťové peer příkazy nebo fyzické výstupy.

## 2. Závazná pravidla

- Žádné heslo se nikdy nevypisuje do běžného Serial logu, webové stránky, UDP ani diagnostického exportu.
- Existující heslo se nikdy znovu nezobrazuje. Web nabízí pouze změnu hesla, nikoli jeho čtení.
- Heslo se do NVS zapisuje až po validaci vstupu; úspěch se potvrzuje zpětným načtením a kontrolou shody.
- Při chybě NVS se nesmí vytvořit dočasné nebo známé výchozí heslo. Přístup zůstane fail-closed a zařízení ohlásí pouze obecnou chybu.
- Obnova přístupu znamená nastavení nových údajů po fyzicky potvrzeném servisním vstupu; nikdy neodhalení starého hesla.
- AP heslo, webové admin heslo a domácí Wi-Fi heslo jsou tři oddělené údaje. Nesmí se vzájemně odvozovat ani sdílet.

## 3. Navržený životní cyklus

### 3.1 První spuštění

1. Jednotka zjistí, že chybí platné AP nebo admin heslo.
2. Nevygeneruje produkční přihlašovací údaje, které by následně vypsala do logu.
3. Spustí omezený servisní stav a indikuje jej pouze neobsahujícím tajemství stavovým hlášením.
4. Servisní konfigurace bude povolena pouze po fyzickém potvrzení servisního režimu.
5. Operátor zadá nové AP a admin heslo přímo v lokální servisní relaci; hesla se nevracejí v HTTP odpovědi ani se neukládají do logu.
6. Firmware ověří délku/validitu, zapíše hodnoty, načte je zpět a porovná. Při jakémkoli selhání zůstane servisní režim aktivní a přístupové údaje se nepovažují za nastavené.
7. Po úspěšném zápisu se servisní okno uzavře a jednotka restartuje síťovou službu.

### 3.2 Běžný provoz a změna hesla

- Běžné AP se spouští jen podle stávající síťové logiky; dashboard zobrazuje SSID a adresu, nikdy heslo.
- Změna admin hesla vyžaduje platnou aktuální webovou autentizaci a zadání nového hesla dvakrát.
- Změna AP hesla vyžaduje platnou aktuální autentizaci; po úspěšném zápisu se AP restartuje. UI oznámí, že klient musí použít nové heslo, ale nové heslo znovu nevypíše.
- Změna hesla se nesmí provést metodou GET; citlivá pole nesmí být předvyplněna.

### 3.3 Ztráta hesla / servisní obnova

- Obnova je možná pouze fyzicky potvrzeným servisním režimem.
- Servisní režim musí být časově omezený a po vypršení se uzavře; přesná doba bude konfigurační konstanta a otestuje se.
- Servisní režim nesmí sám o sobě povolit peer command path, OTA ani výkonové výstupy.
- Obnova nahradí staré přihlašovací údaje novými; staré hodnoty se nesmějí zobrazit ani exportovat.
- Pokud nelze fyzické potvrzení bezpečně detekovat na konkrétní desce, obnova se neimplementuje přes webový endpoint dostupný běžnou sítí.

## 4. Fyzické potvrzení — otevřená hardwarová závislost

Master ani dostupný pinový plán zatím nepotvrzuje samostatné servisní tlačítko/jumper pro obě desky. GPIO se proto v revizi 0.77 nepřiděluje.

Před implementací musí být ověřeno:
- přesný typ/revize obou vývojových desek;
- zda je k dispozici nezávislý vstup servisního tlačítka/jumperu bez konfliktu s W5500, senzory, boot režimem a budoucím hardwarem;
- elektrické zapojení, aktivní úroveň, pull-up/pull-down a chování při resetu;
- zda servisní potvrzení vyžaduje podržení při bootu nebo fyzické potvrzení po bootu.

Nepoužívat automaticky GPIO0/BOOT: jeho funkce při startu může kolidovat s bootloaderem a bez ověření desky nelze bezpečně určit chování. USB/Serial přítomnost sama o sobě není dostatečným důkazem fyzického oprávnění.

## 5. Zabezpečení servisní relace

- Servisní endpoint je dostupný pouze v servisním režimu a pouze z lokální AP sítě.
- Po úspěšném provisioningu se endpoint deaktivuje; po timeoutu také.
- Vstupní hesla se zpracují pouze v RAM, nesmí se připojit k diagnostickým Stringům ani chybovým textům.
- HTTP odpovědi obsahují pouze výsledek operace, nikdy přijaté hodnoty.
- Omezit počet neúspěšných pokusů a přidat prodlevu; konkrétní limity musí být testovány proti zablokování legitimního servisu.
- Použít POST a vypnout cache odpovědí pro provisioning endpointy.
- Lokální HTTP bez TLS neposkytuje ochranu proti pasivnímu odposlechu na stejné Wi-Fi. Proto je tento postup přijatelný pouze v časově omezeném AP servisním režimu, nikoli přes domácí LAN nebo internet. Preferovanou alternativou pro finální implementaci je přímé fyzické servisní rozhraní, pokud bude potvrzeno.

## 6. Úložiště

Tento návrh neprohlašuje běžné Preferences/NVS za důvěrné úložiště. AP/admin hesla zůstávají citlivými údaji a jejich ochrana ve flash musí být řešena samostatně platformním plánem `SECURITY_PROVISIONING_DESIGN.md`.

Před produkčním nasazením je nutné prokázat konkrétní NVS Encryption + Flash Encryption konfiguraci na přesném targetu. Bez toho se nesmí tvrdit, že hesla jsou chráněna proti fyzickému čtení flash.

## 7. Požadované regresní testy před odstraněním starých výpisů

1. Čisté první spuštění bez přihlašovacích údajů.
2. Chybějící/poškozený záznam AP a admin hesla.
3. Neúspěšný NVS begin, write, read-back a mismatch.
4. Přerušení napájení před zápisem, během zápisu a po ověření.
5. Úspěšná změna hesla a odmítnutí starého hesla.
6. Ztráta hesla a fyzicky potvrzená obnova.
7. Servisní timeout a opětovné uzavření endpointu.
8. HTTP GET nesmí měnit údaje; žádná odpověď nesmí obsahovat heslo.
9. Statická kontrola zdrojů: žádný `Serial.*` formát nesmí obsahovat hodnoty `apPassword`, `localApPassword`, `adminPassword` nebo `localAdminPassword`; dashboard nesmí vykreslit `apPassword`.
10. Testy obou ESP32 samostatně; žádné sdílení hesla mezi jednotkami.

## 8. Rozhodnutí a další brána

Návrhový směr: fyzicky potvrzený, časově omezený lokální provisioning; žádné zobrazování existujících hesel; změna přes autentizované rozhraní; obnova pouze fyzickým servisním vstupem.

Před implementací je nutné rozhodnout/ověřit pouze jednu hardwarovou věc: dostupnost bezpečného fyzického servisního vstupu na obou konkrétních deskách. Do té doby se firmware nemění a staré výpisy jsou evidovány jako otevřené P0 riziko, nikoli jako bezpečné řešení.
