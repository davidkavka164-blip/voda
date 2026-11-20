# Otevřené hardwarové a integrační položky

## Hardware uvedený uživatelem

**ESP32 č. 1:** 2× elektrický ventil s 5V reléovým ovládáním; 2× bezpečnostní relé 24 V DC / 30 A: GPIO25 pro čerpadlo studny a GPIO32 pro čerpadlo dolní nádrže; 2× hladinové čidlo (studna a dolní nádrž); W5500 Ethernet (SPI SCK18/MISO19/MOSI23/CS27/RESET26); domácí Wi-Fi STA + konfigurační AP pro web; zdroj 5 V / 15 W; zdroj 24 V / 60 W. W5500 je od REV85 aktivováno pro DHCP síťovou cestu; HTTP ovládací web přes Ethernet zatím není implementován.

**ESP32 č. 2:** 1× teplotní čidlo; 1× hladinové čidlo; W5500 Ethernet připojený do domácí LAN; 1× bezpečnostní relé 24 V DC / 30 A pro místní ochranu čerpadla horní nádrže (GPIO25); zdroj 5 V / 15 W; zdroj 24 V / 15 W.

Z předchozího projektu jsou známa čtyři čidla QDY30A, 0–10 m, 4–20 mA, 24 V DC. Zatím není potvrzené přiřazení jednotlivých kusů a čtvrtý kus může zůstat rezervní.

## Pracovní návrh

Připraven je `GPIO_DRAFT.md` a společný `firmware/common/HardwarePins.h` s návrhem pinů pro klasickou desku ESP32 DevKit / ESP32-WROOM-32 a W5500 na SPI.

U pumpových relé je potvrzen přesný typ: Drátek.cz „Modul relé 24V 30A High-Low - 1 kanál na DIN lištu“, produkt 1476828250. Uživatel potvrdil, že stejné reléové zapojení je funkční v samostatném projektu studny. Přenáší se stejná konfigurace: napájení modulu 24 V DC, jumper HIGH, aktivní HIGH z GPIO ESP32 (3,3 V); HIGH přitáhne relé a povolí napájení motoru, LOW relé uvolní a motor odpojí. Není požadována nová změna elektrického principu ani další typ relé.

Pumpová GPIO mapa je potvrzena: ESP32 #1 GPIO25 = studna, GPIO32 = dolní nádrž; ESP32 #2 GPIO25 = horní nádrž. Uživatel potvrdil, že stejný typ 24V/30A relé a zapojení HIGH funguje v samostatném projektu studny; HOME WATER CONTROLLER přebírá toto řešení. Uživatel potvrdil také typ relé pro ventily: Drátek.cz „Modul relé 5V – 1 kanál s úrovňovým spínačem“, produkt 1517435132. Firmware řídí ventilové výstupy GPIO16/17, ale fyzický test 5V reléové desky, vstupu 3,3 V, bezpečného bootu a konkrétních cívek zůstává otevřený. Před připojením cívek je nutné ověřit jejich normální stav bez napájení a bezpečné zapojení.

## Nutno ověřit před zapojením

1. Přesný typ obou desek ESP32 a jejich revize.
2. U pumpových relé se přebírá funkční zapojení z projektu samostatné studny: Drátek 24V/30A HIGH–LOW, napájení 24 V DC, jumper HIGH a aktivní HIGH z GPIO ESP32. U 5V reléových modulů ventilů zůstává samostatné ověření vstupu a boot chování. GPIO nesmí přímo spínat cívku 5 V ani 24 V.
3. Typ ventilů a jejich stav při ztrátě napájení; monostabilní/bistabilní, normálně otevřené/zavřené. Firmware už řídí jejich reléové výstupy, ale fyzické cívky zatím nepřipojovat bez ověření.
4. U pumpových relé je potvrzeno přímé přerušení napájení motoru, dostatečné výkonové dimenzování a funkční zapojení stejného modulu v projektu samostatné studny. HOME WATER CONTROLLER přebírá tuto ověřenou konfiguraci; firmware nastaví GPIO do LOW hned na začátku setup(). Při montáži dalších dvou modulů se použije stejné zapojení a stejné nastavení HIGH.
5. Potvrdit mapování čidel: studna, dolní nádrž, horní nádrž; určit čtvrtý senzor jako rezervu nebo další měřicí bod.
6. Měřicí obvod 4–20 mA: předchozí projekt používal 150 Ω a GPIO34. Jde pouze o referenci. Ověřit konkrétní zapojení, polaritu, společnou zem a maximální napětí na ADC.
7. Přesný typ teplotního čidla. GPIO 4 v návrhu platí pouze pro DS18B20.
8. Přesný typ W5500 modulu, jeho napájení, reset a kompatibilita s 3,3V SPI logikou.
9. Ověřit fyzicky průchod UDP broadcastu mezi Wi-Fi klientem ESP32 #1 a Ethernet klientem ESP32 #2 přes domácí router/AP; obě rozhraní musí být ve stejné LAN/podsíti a nesmí být aktivní izolace Wi-Fi klientů nebo blokování broadcastu.
10. Parametry horní nádrže Smart 7000 a definice jejího pracovního minima/rezervy/kritické hladiny.
11. Verze Arduino-ESP32 core a knihoven v Arduino IDE 1.8.19; překlad v tomto prostředí zatím není možný.
12. Bezpečný testovací postup s odpojenými ventily a výkonovými obvody čerpadel.

Pravidlo: dokud položka ovlivňující elektrickou bezpečnost nebo výstupní logiku není potvrzena, použít rozhraní/configuration placeholder, nikoli domněnku.


## Prozatímní síťový předpoklad revize 0.16
- Firmware používá UDP broadcast na port 4211. Aktuální potvrzená topologie REV45: ESP32 #2 vysílá přes W5500 Ethernet; ESP32 #1 přijímá přes domácí Wi-Fi STA. Obě rozhraní musí být ve stejné LAN a router/AP musí propouštět broadcast mezi Ethernetem a Wi-Fi. Podrobnosti viz aktualizace REV45 níže.
- UDP status neobsahuje autentizaci a nesmí se používat v nedůvěryhodné síti. Zatím nepřenáší řídicí příkazy.


## Aktualizace REV24 – potvrzené přiřazení čerpadel

Uživatel upřesnil, že ESP32 #1 hlídá dvě samostatná čerpadla: ve studni a v dolní nádrži. ESP32 #2 místně hlídá čerpadlo horní nádrže. Pracovní přiřazení podle dosavadní pinové mapy je GPIO25 = studna, GPIO32 = dolní nádrž na ESP32 #1; GPIO25 = horní nádrž na ESP32 #2. Rozdělení čerpadel mezi jednotky je potvrzené, konkrétní GPIO zůstávají předběžná. Uživatel potvrdil, že 24V/30A relé přerušují napájení motoru a jejich výkonové dimenzování je dostačující. Logika samostatné ochrany čerpadla dolní nádrže, její vlastní kritická hladina a hystereze jsou doplněny v REV25 a jsou uživatelsky nastavitelné; nesmí se odvozovat z pracovního minima dolní nádrže 11 cm. Fyzické výstupy zůstávají vypnuté do ověření vstupní úrovně relé a bezpečného boot/reset stavu. Podrobnosti: `PUMP_SAFETY_RELAY_INTEGRATION_AUDIT_REV24.md`.

## Aktualizace REV25 – ochrana čerpadla dolní nádrže

Logická ochrana čerpadla dolní nádrže je nyní implementována na ESP32 #1. Webové rozhraní umožňuje samostatně nastavit minimální hladinu pro vypnutí a hysterezi pro opětovné povolení; obě hodnoty jsou uloženy v NVS. Pracovní minimum 11 cm zůstává nezávislé. Rozsah čidla pro tuto ochranu je 0–121 cm. Dokud uživatel nenastaví vlastní hodnoty, ochrana čerpadlo logicky nepovolí. Podrobnosti: `LOWER_TANK_PUMP_PROTECTION_REV25.md`.

Uživatel potvrdil, že stejné reléové zapojení je funkční v projektu samostatné studny. Pro všechny tři výstupy se proto přebírá totožný modul, 24V napájení, jumper HIGH a ovládání aktivní úrovní HIGH z GPIO. REV28 povoluje fyzické výstupy; při nasazení dalších modulů se použije stejné zapojení jako u funkční samostatné studny.


## Aktualizace REV27 – výstupní adaptér bezpečnostních relé čerpadel

- Doplněn společný `PumpSafetyRelayOutput` a jeho kopie do obou Arduino sketch `src` adresářů. Jeden výstup odpovídá jednomu čerpadlu; logické `pumpPermit` aktivuje relé pouze při explicitně povoleném commissioning gate.
- ESP32 #1 připravuje samostatné výstupy pro studniční čerpadlo GPIO25 a čerpadlo dolní nádrže GPIO32. ESP32 #2 připravuje místní výstup horního čerpadla GPIO25.
- Každý výstup je při startu nastaven na LOW (aktivní HIGH = relé odpadlé). V běžném režimu se synchronizuje s lokálním `pumpPermit` a čerstvostí posledního měření (max. 5 s). V nouzovém režimu po výslovném potvrzení fyzické kontroly hladiny se pro příslušné čerpadlo ignoruje softwarová ochrana hladiny, kritický latch, neplatná konfigurace i chyba/nedostupnost NVS; nouzový stav obejde i podmínku čerstvosti měření. Nouzový stav je pouze v RAM a po restartu se neobnoví. Výstup stále vyžaduje inicializované a připravené relé; nouzový režim neobchází nezávislé jištění ani ochranu motoru.
- **REV28 přebírá funkční elektrické řešení z projektu samostatné studny** a nastavuje `PUMP_SAFETY_OUTPUTS_ENABLED = true` pro všechny tři výstupy.
- Aktivní HIGH znamená povolení motoru; LOW odpojí relé a přeruší napájení motoru. Stejné zapojení a nastavení HIGH se použije na všech třech modulech.
- Hostitelské testy ověřují logickou vazbu permit → výstup, výchozí OFF, active-HIGH/active-LOW mapování, idempotenci a fail-off. Nenahrazují fyzické ověření každého nově osazeného modulu v jeho konkrétním motorovém obvodu.


## Aktualizace REV28 – převzetí funkčního reléového zapojení ze samostatné studny

- Uživatel upřesnil, že zapojení bezpečnostního relé čerpadla bylo vyřešeno a je funkční v samostatném projektu studny. HOME WATER CONTROLLER proto přebírá stejný modul Drátek 24 V DC / 30 A, napájení 24 V DC, jumper HIGH a ovládání aktivní úrovní HIGH z GPIO ESP32.
- HIGH = relé přitažené / motor povolen; LOW = relé odpadlé / napájení motoru přerušeno.
- Fyzické výstupy všech tří čerpadel jsou v REV28 povoleny. GPIO se v setup() nastavují do LOW bezprostředně na začátku inicializace; výstup se následně řídí pouze vlastním lokálním `pumpPermit` a čerstvostí příslušného měření.
- ESP32 #1 GPIO25 = studna; ESP32 #1 GPIO32 = dolní nádrž; ESP32 #2 GPIO25 = horní nádrž.
- Při montáži dalších modulů se použije totožné zapojení jako u funkčního modulu v samostatné studně. Arduino cílový build a fyzické testy všech tří instalovaných výstupů zůstávají samostatnými ověřovacími kroky.

## Nouzové obejití ESP32 – samostatný hardwarový stupeň

Uživatel plánuje instalovat fyzické přepínače pro nejvyšší nouzi, které umožní obejít řídicí ESP32. Tento stupeň je záměrně mimo firmware a není součástí webového nouzového režimu čerpadel. Konkrétní elektrické schéma a typ přepínačů zatím nejsou určeny; nesmí se domýšlet ani považovat za již ověřené. Při samostatném návrhu musí být zachována tlaková automatika a nezávislé elektrické jištění a ochrana motoru.

## Aktualizace REV34 – síťové rozhraní a obnova spojení

- ESP32 #1 poskytuje webové rozhraní přes Wi-Fi STA; při nedostupné domácí Wi-Fi spouští po 30 s fallback AP a dál zkouší STA připojení. Při neúspěšném startovním připojení spouští AP již po 15s startovacím pokusu.
- ESP32 #2 poskytuje vlastní lokální Wi-Fi AP; v této revizi se nepřipojuje jako STA do domácí Wi-Fi.
- W5500 na ESP32 #2 slouží pro DHCP/UDP peer vysílání, nikoli pro webový server. ESP32 #1 používá Wi-Fi STA pro web i peer UDP příjem; její fallback AP není mostem do Ethernetové LAN.
- Firmware sám nezajišťuje veřejný internetový přístup. Vzdálený přístup vyžaduje samostatně nakonfigurovanou bezpečnou síťovou cestu, například VPN routeru.
- DHCP timeout při startu W5500 zůstává 5 s; opakované pokusy z hlavní smyčky jsou omezeny na 1 s, aby síťová obnova zbytečně nezdržovala čtení čidel a synchronizaci bezpečnostních relé.
- Podrobnosti a limity ověření: `NETWORK_RECOVERY_AUDIT_REV34.md`.

## REV38–REV41 – elektrické nastavení snímačů a objemová kalibrace

Elektrické nastavení snímače (`SensorConfig`) převádí signál 4–20 mA na výšku hladiny. Běžné ochranné použití výšky je povoleno pouze tehdy, když je elektrické nastavení uložené a ověřené. Chybějící, poškozený nebo nedostupný záznam znamená neplatné měření pro běžnou ochranu; výchozí hodnoty jsou pouze diagnostické. Objemová křivka studny (`WellCalibration`) převádí výšku na litry a sama nikdy neblokuje čerpadla ani transfer.

REV41 zjistila, že servisní obnova není dokončena: ESP32 #1 může uložit chybějící konfiguraci studny/dolní nádrže, ale odmítá přepsat poškozený záznam; ESP32 #2 nemá webový formulář pro elektrické nastavení horního snímače. Nedostupné NVS nelze opravit běžným webovým zápisem. Firmware se v REV41 nemění. Podrobnosti: `LEVEL_VS_VOLUME_CALIBRATION_AUDIT_REV39.md` a `SENSOR_CONFIG_RECOVERY_AUDIT_REV41.md`. Arduino build a fyzické testy jsou stále otevřené.


## Aktualizace REV41 – audit servisní obnovy elektrického nastavení snímačů

- Ověřeno, že bezpečnostní blokace při chybějícím/neplatném `SensorConfig` zůstává zachována.
- Zjištěna neúplná servisní obnova: ESP32 #1 umí uložit chybějící záznam studny/dolní nádrže, ale odmítá přepsat poškozený; ESP32 #2 nemá webovou cestu pro nastavení horního snímače.
- Nedostupné NVS zůstává mimo možnosti webové opravy.
- REV41 nemění firmware ani ochrany; definuje požadavky na bezpečný servisní postup s fyzickou referenční kontrolou, autentizací a read-backem.
- Viz `SENSOR_CONFIG_RECOVERY_AUDIT_REV41.md`.


## Aktualizace REV45 – potvrzená smíšená síťová topologie

Uživatel upřesnil zapojení: **ESP32 #1 bude připojena k domácí Wi-Fi; ESP32 #2 bude připojena do domácí LAN přes W5500 Ethernet.** Dřívější předpoklad dvou W5500 peer rozhraní je tímto nahrazen.

- ESP32 #1 používá Wi-Fi STA pro webové rozhraní a příjem stavových UDP rámců na portu 4211 (`WiFiUDP`). Na ESP32 #1 se pro peer komunikaci W5500 nepoužívá.
- ESP32 #2 používá W5500 Ethernet/DHCP a vysílá stavové UDP broadcasty na port 4211. Její lokální servisní web/AP zůstává nezávislý.
- Obě jednotky musí být ve stejné domácí LAN/podsíti. Router/AP musí propouštět broadcast z Ethernetu do Wi-Fi klientů; nesmí být zapnutá izolace klientů, guest Wi-Fi ani oddělení VLAN.
- Fallback AP ESP32 #1 není síťovým mostem do Ethernetové LAN. Pokud ESP32 #1 není připojena jako STA k domácí Wi-Fi, peer stav je nedostupný a horní transfer se bezpečně zablokuje. Po obnovení STA se UDP listener znovu otevře; čerstvý peer rámec je nutný před obnovením použití horní hladiny.
- Ztráta Ethernetu ESP32 #2 ponechává její místní snímání a místní ochranu horního čerpadla v chodu, ale ESP32 #1 přestane dostávat aktuální horní hladinu a horní transfer se zablokuje.
- UDP status zůstává stavovým kanálem, nikoli webovým serverem. Tato revize nemění dosavadní bezpečnostní omezení peer protokolu.
- Skutečný průchod broadcastu přes konkrétní router/AP a Arduino-ESP32 kompilace zatím nejsou fyzicky ověřeny.


## Aktualizace REV85 — W5500 na ESP32 #1

- Firmware ESP32 #1 nyní inicializuje W5500 na GPIO18/19/23/27/26, získává DHCP a sleduje link/IP; Wi-Fi STA a konfigurační AP zůstávají aktivní podle dosavadní logiky.
- Pinová mapa již W5500 obsahovala; REV85 ji nemění. Před fyzickým zapojením je stále nutné ověřit konkrétní modul, napájení, reset a 3,3V SPI kompatibilitu.
- Ethernetové HTTP ovládání není součástí REV85. Přes W5500 se zatím nesmí považovat za dostupné formuláře, autentizace ani OTA.
- Výpadek W5500 nesmí ovlivnit pumpové ochrany, měření, ventily, transfery ani Wi-Fi/AP.
