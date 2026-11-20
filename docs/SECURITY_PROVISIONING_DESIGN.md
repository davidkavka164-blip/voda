# HOME WATER CONTROLLER — návrh bezpečného párování a uložení klíče

Revize návrhu: 0.64  
Datum: 2026-11-02  
Stav: technický návrh; není implementováno ani schváleno pro fyzické zapnutí bezpečnostních eFuse.

## 1. Účel a hranice

Tento dokument uzavírá návrhovou část nejbližšího bezpečnostního blokátoru z auditu Master 1.3: jak vytvořit, předat, uložit, obnovit a změnit společné tajemství pro autentizaci ESP32 #1 a ESP32 #2.

Dokument sám o sobě:
- nezapíná Secure Boot ani Flash Encryption;
- nemění eFuse, partition table, NVS ani firmware;
- nepovoluje síťové řídicí příkazy ani výkonové výstupy;
- nepovažuje běžné Preferences/NVS bez prokázaného NVS encryption za chráněné úložiště;
- neurčuje konkrétní UART/GPIO zapojení, protože skutečné desky, revize a zapojení nejsou potvrzeny.

## 2. Co je nyní v kódu

- `PeerSecretVault` očekává 32bajtové tajemství a backend, který výslovně potvrdí důvěrné úložiště.
- `PeerSecretVault` před čtením vynuluje výstupní buffer a při chybě jej znovu vynuluje.
- Provisioning i mazání vyžadují explicitní příznak fyzického potvrzení.
- Prostý backend bez důvěrného úložiště musí operaci odmítnout.
- Produkční ESP32 backend, bezpečný fyzický provisioning a rotace klíče zatím neexistují.
- `NvsPeerReplayStore` je samostatný dvouslotový NVS žurnál pro anti-replay checkpointy; není to úložiště tajemství.

## 3. Doporučený model tajemství

1. Použít jedno nezávislé 256bitové (32 B) náhodné tajemství pro konkrétní dvojici ESP32 #1 ↔ ESP32 #2.
2. Tajemství nesmí být odvozeno z MAC adresy, názvu AP, hesla Wi-Fi, sériového čísla ani veřejného ID jednotky.
3. Generovat je kryptograficky bezpečným generátorem náhodných čísel. V produkčním firmwaru musí být zdroj náhodnosti na ESP32 ověřen; pouhé `random()` nestačí. `esp_random()` je v současném kódu používán pro boot/request identifikátory, ale zatím není integrován do handshake orchestrace.
4. Tajemství nesmí být vypsáno do Serial Monitoru, webu, UDP rámce, diagnostického logu ani uloženého exportu projektu.
5. Každá dvojice zařízení má mít vlastní tajemství. Tajemství se nesmí sdílet mezi více instalacemi.

## 4. Doporučené fyzické párování

Párování má proběhnout pouze při místní instalaci a při fyzickém přístupu k oběma jednotkám. Preferovaný směr implementace:

- servisní provisioning přes fyzicky připojené USB/seriové rozhraní;
- žádný přenos klíče přes Wi-Fi AP, běžný web, UDP broadcast ani internet;
- servisní režim musí být explicitní a časově omezený;
- provisioning rozhraní musí přijmout pouze příkaz pro vytvoření/instalaci klíče po fyzickém potvrzení místního operátora;
- nástroj nesmí tajemství zobrazit v konzoli ani ukládat do logu; buffer na hostiteli i zařízení musí být po dokončení smazán;
- obě jednotky musí po zápisu provést načtení zpět a ověřit shodu pomocí autentizovaného testu handshake, nikoli vypsáním klíče.

Přesný fyzický postup (USB-UART, přímé propojení jednotek nebo jiná servisní metoda) zůstává otevřený do potvrzení konkrétní desky, volných pinů a dostupných konektorů. V tomto kroku se žádné GPIO nepřiděluje.

## 5. Chráněné úložiště — doporučená cílová varianta

Pro klasický ESP32 je cílovou variantou ESP-IDF Flash Encryption v režimu Release společně s NVS Encryption. NVS Encryption používá samostatnou NVS key partition; klíčová partition musí být chráněna Flash Encryption. Samotné zapnutí Flash Encryption neznamená, že běžná NVS data jsou automaticky šifrována.

Podmínky před implementací:

- přesně zjistit typ čipu, revizi, flash a aktuální Arduino-ESP32/ESP-IDF verzi;
- připravit a zkontrolovat vlastní partition table včetně `nvs_keys` partition;
- ověřit konfiguraci NVS Encryption v konkrétním buildu a skutečné inicializaci NVS, kterou používá Arduino `Preferences`; platformní audit 0.64 ukazuje, že `Preferences` samo šifrování nezapíná a Windows 7 omezuje dostupnou řadu Arduino-ESP32;
- nejprve provést celý postup na náhradní/testovací desce stejného typu;
- otestovat výpadek napájení při prvním šifrování, obnovu, OTA a sériové servisní postupy;
- teprve po úspěšné zkoušce navrhnout produkční postup pro obě jednotky.

Flash Encryption Release a související eFuse operace jsou zásadní bezpečnostní změny s omezením dalších servisních možností. Nesmí být zapnuty automaticky ani bez samostatného výslovného souhlasu uživatele. Secure Boot je samostatná vrstva pro ověřování firmwaru; jeho zapnutí a dopady na OTA/servis se musí posoudit odděleně.

## 6. Co NVS Encryption neřeší

Šifrování NVS je ochrana dat uložených ve flash, nikoli kompletní bezpečnostní architektura. Zejména samo o sobě nedokazuje odolnost proti návratu celé starší kopie flash ani nenahrazuje autentizaci zpráv.

`NvsPeerReplayStore` musí být dále posouzen z hlediska:
- zachování checkpointu při výpadku napájení;
- odmítnutí poškozených nebo neplatných záznamů;
- chování při obnově staršího obsahu NVS;
- vazby na přesné ID peeru a boot ID;
- toho, zda fyzický útočník s přístupem k flash patří do požadovaného modelu hrozeb.

CRC chrání před náhodným poškozením, samo o sobě však není kryptografickým důkazem původu dat.

## 7. Rotace a obnova

- Rotace se provádí pouze v místním servisním režimu, když jsou obě jednotky dostupné.
- Během rotace musí být síťové příkazy zakázány.
- Nový klíč se zapíše do obou jednotek, načte zpět a ověří vzájemným handshake.
- Dokud ověření neprojde na obou stranách, systém zůstává ve stavu bez autentizovaného peer spojení.
- Při ztrátě jedné jednotky se starý klíč považuje za kompromitovatelný; po výměně se provede nové párování.
- Nesmí existovat skrytý výchozí klíč, servisní univerzální heslo ani automatický fallback na neautentizované příkazy.

## 8. Požadavky pro produkční backend `PeerSecretVault`

Backend smí vrátit `confidentialStorageAvailable() == true` pouze tehdy, pokud ověří skutečnou konfiguraci NVS Encryption a její ochranné závislosti. Pouhá přítomnost hodnoty v Preferences nestačí.

`read()` a `write()` musí:
- pracovat pouze s přesnou dvojicí ID zařízení;
- odmítnout neplatnou délku/verzi nebo poškozený záznam;
- mít trvalý zápis před návratem úspěchu;
- po zápisu ověřit přesnou hodnotu opětovným načtením; `PeerSecretVault` toto ověření nyní provádí a porovnává 32 bajtů konstantním časem;
- při neúspěšném ověření se pokusit neověřený záznam zneplatnit;
- nikdy neposkytovat tajemství do logu;
- vracet chybu při jakékoli nejistotě.

`erase()` musí být explicitní servisní akce. Backend smí vrátit úspěch pouze tehdy, když je záznam trvale nedostupný. Změna klíče nesmí automaticky obnovit starou hodnotu z nechráněného úložiště. Samotný přenositelný vault nemůže zaručit atomickou transakci ani trvalost backendu; to musí být ověřeno v konkrétní implementaci.

## 9. Brány před síťovou integrací

Síťová orchestrace HELLO/CHALLENGE/FINISH/READY a `PeerCommandSession` přes W5500 se nesmí aktivovat, dokud nejsou splněny všechny následující podmínky:

1. potvrzený typ a revize hardwaru obou ESP32;
2. skutečný chráněný backend `PeerSecretVault` na obou jednotkách;
3. ověřený fyzický provisioning a rotace klíče;
4. ověřený zdroj nonce a boot ID;
5. ověřená perzistence anti-replay po restartu a výpadku napájení;
6. úplné testy transportní orchestrace včetně ztrát, duplicit, timeoutů, odpojení a restartů;
7. produkční command path ve výchozím stavu uzavřená.

I po dokončení těchto bodů zůstávají výkonové výstupy fyzicky neaktivní, dokud neproběhne samostatná hardwarová bezpečnostní validace.

## 10. Aktuální rozhodnutí projektu

**Přijato jako předběžný návrhový směr:** fyzický provisioning + chráněné úložiště NVS Encryption založené na Flash Encryption Release. Platformní audit 0.64 ale ukázal, že kompatibilita s uživatelovým Windows 7 / Arduino core zatím není prokázána; cílová varianta proto není schválena pro implementaci.

**Zatím neschváleno / neprovedeno:** změna partition table na deskách, generování a zápis produkčního klíče, vypalování eFuse, Flash Encryption Release, Secure Boot, UART lock-down, produkční NVS backend a síťová command orchestrace.

**Následující bezpečný krok:** zjistit přesnou verzi Arduino-ESP32 core a podle ní vybrat testovací platformní postup. Revize 0.63 ověřuje zápis tajemství zpětným čtením v přenositelném vaultu; produkční backend tím nevzniká. Do vyřešení platformní kompatibility zůstává vault bez produkčního backendu a peer command path zakázaná. Do té doby zůstává `PeerSecretVault` bez produkčního backendu a peer command path zakázaná.
