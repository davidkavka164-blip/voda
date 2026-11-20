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
