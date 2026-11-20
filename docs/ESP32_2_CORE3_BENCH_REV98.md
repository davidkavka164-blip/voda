# REV98 – stolní zkouška ESP32 #2 s Arduino-ESP32 3.3.7

Výchozí stav je [master 3.18 / REV97](../MASTER_HOME_WATER_CONTROLLER_3.18_REV97.md).
U ESP32 #2 už uživatel ověřil domácí Wi-Fi a Ethernet v REV97 včetně obnovy po odpojení kabelu.

V REV98 připojuje nativní ovladač `ETH` W5500 jako síťové rozhraní ESP32.
Tentýž `WebServer` obsluhuje domácí Wi-Fi, servisní AP a Ethernet. Stránky `/`
a `/status` vedou na přihlášený ovládací web horní nádrže včetně OTA.
Ověřování správce, kontrola Origin/Host, omezení pokusů o přihlášení a
blokace OTA při nouzovém režimu zůstávají ve stávajících handlerech.
Zapojení W5500 zůstává CS 27, RESET 26, SCK 18, MISO 19 a MOSI 23.
Pin INT zůstává nezapojený. Nativní ovladač při `irq = -1` používá polling.
ESP32 #1 na větvi `main` dál používá REV97 a core 1.0.6.

## Sestavení

ESP32 Dev Module, Arduino-ESP32 3.3.7, QIO / 80 MHz, OneWire 2.3.8 a
DallasTemperature 4.0.6. Sketch obsahuje `partitions.csv` shodný se 4MB
tabulkou REV97 včetně dvou OTA oddílů. Ovladač
W5500 je součástí core 3.3.7. Pro nahrání připraveného aplikačního souboru
přes OTA není potřeba měnit Arduino IDE ani core na počítači.

## Zkouška na stole

1. Odpoj výstup čerpadla. Měj k dispozici USB a odpovídající sketch **ESP32 #2
   REV97** pro návrat, pokud se síť po aktualizaci neozve.
2. V ovládacím webu **ESP32 #2 REV97** nahraj přes Wi-Fi aplikační soubor
   `ESP32_2_REV98_CORE3_BENCH.bin`. Během aktualizace nevypínej napájení.
3. Po restartu vyhledej případnou novou IP adresu W5500 v sériovém monitoru
   na řádku `Ethernet management:` nebo v přehledu sítě v ovládacím webu.
   Ethernet může dostat jinou adresu než v REV97.
4. Otevři `http://<IP W5500>/` a přihlas se dosavadním účtem `admin`.
   Ověř, že jde o správu horní nádrže a že stránka uvádí REV98 Core 3.
   Domácí Wi-Fi na jednotce nemusí být dostupná. Přístup přes servisní AP
   otestuj zvlášť.
5. Ověř návrat webu po odpojení kabelu a aktualizační formulář přes kabel.
   Skutečná zkouška OTA přes Ethernet vyžaduje druhé nahrání testovacího
   aplikačního souboru a kontrolu restartu.

## Výsledek stolní zkoušky

Uživatel potvrdil, že se na ESP32 #2 s REV98 zobrazuje stejný ovládací web
přes Wi-Fi i W5500. Způsob prvního nahrání REV98 nebyl výslovně potvrzen.

Uživatel dále potvrdil funkční ovládací web i nahrání OTA přes Ethernet,
když ESP32 nebyla připojená k domácí Wi-Fi. Samostatné potvrzení návratu
REV98 po odpojení kabelu zatím chybí.

## REV99

Po úspěšném zápisu OTA používá ESP32 #2 stejnou stránku s výsledkem jako
ostatní formuláře. Nabízí odkaz **Zpět na přehled** na `/`, který po
restartu otevře základní stránku bez opakování nahrávání. Výsledek prvního
nahrání REV99 ještě zobrazí firmware REV98. Odkaz lze ověřit po další
aktualizaci spuštěné z běžící REV99.

## REV100

Servisní AP ESP32 #2 se spouští po 30 sekundách bez domácí Wi-Fi i
Ethernetu s IP adresou. Po startu běží stejná ochranná lhůta. Jakmile je
alespoň jedno z těchto připojení opět dostupné, servisní AP se vypne.
Přihlašovací údaje domácí Wi-Fi lze uložit z ovládacího webu přes Ethernet.
Jednotka nejprve požádá o ukončení předchozího spojení nebo pokusu a poté
spustí připojení s novými údaji. Skutečné připojení a IP ukazuje přehled.

Stolní zkouška REV100 s odpojeným výstupem čerpadla:
1. Přes Ethernet ulož správné údaje domácí Wi-Fi. Na přehledu ověř stav
   „Připojeno“ a otevři zobrazenou Wi-Fi IP. Servisní AP má být vypnutý.
2. Zkus výpadek jen jednoho rozhraní. Druhé rozhraní má obsloužit web a
   servisní AP má zůstat vypnutý.
3. Ponech obě připojení nedostupná přes 30 sekund. Ověř přihlášení přes
   servisní AP, potom obnov kabel. Po získání ethernetové IP má AP zhasnout.
4. Ověř stejný postup při startu bez obou připojení.

REV100 je stolní sestava pro ověření těchto změn na desce.

## REV101

Na jedné stolní jednotce REV100 po restartu ohlásila neúspěšnou inicializaci
W5500. REV101 spouští ovladač mimo řídicí smyčku, po krátké prodlevě a
řízeném hardwarovém resetu. SPI používá 10 MHz. Při chybě se pokus zopakuje
nejvýše dvakrát, pokud předchozí ovladač šel uklidit. Web a sériový monitor
rozlišují inicializaci modulu, fyzický link a čekání na DHCP. Síťové
podmínky servisního AP zůstávají stejné jako v REV100.

Při zkoušce s odpojeným výstupem čerpadla nahraj sestavu přes funkční
ovládací web, zkontroluj IP adresu W5500 a ověř studený start s již
zapojeným síťovým kabelem. Jestli inicializace dál selhává, zachyť výpis
na 115200 baud od prvního řádku „W5500 initialization attempt“.

## REV102

Přehled ESP32 #2 má samostatné ovládání domácí Wi-Fi. Volba **Vypnout
domácí Wi-Fi** vypne režim STA a uloží nastavení přes restart. Uložený
SSID a heslo zůstanou k dispozici pro **Zapnout domácí Wi-Fi**. Oba režimy
lze změnit z přihlášeného webu přes W5500. Servisní AP zůstává vázaný na
výpadek obou hlavních připojení po 30 sekundách.

Při stolní zkoušce použij ovládací web na IP adrese W5500. Vypni domácí
Wi-Fi, obnov stránku a zkontroluj stav „Vypnuto“. Ověř zachování volby po
restartu. S odpojeným výstupem čerpadla pak na více než 30 sekund odpoj
Ethernet a zkontroluj servisní AP. Po návratu Ethernetu lze domácí Wi-Fi
zapnout bez opětovného zadávání hesla.

### Výsledek zkoušky REV102

Uživatel potvrdil funkční stolní zkoušku ESP32 #2 podle výše uvedeného
postupu, včetně přepínání domácí Wi-Fi a záložního servisního AP.
Provozní ověření čidla a výstupu čerpadla zůstává samostatným krokem.
