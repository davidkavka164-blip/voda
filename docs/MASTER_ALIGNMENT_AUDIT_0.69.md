# Kontrola návaznosti MASTER 1.3 a DEV KIT 0.66–0.71

Datum kontroly: 2026-11-04 (doplnění revizí 0.70–0.71)  
Závazný zdroj: `HOME_WATER_CONTROLLER_MASTER_1.3` (schválený obsahový návrh 28. 10. 2026)  
Porovnané balíčky: DEV KIT 0.66, 0.67 a 0.68; pracovní oprava a dokumentační sjednocení jsou označeny jako 0.69.

## 1. Výsledek návaznosti verzí

- 0.66 je základ s restartovacím žurnálem transferu.
- 0.67 přidává pouze testy okrajových stavů NVS žurnálu; firmware se nemění.
- 0.68 přidává diagnostiku výpadku peer komunikace do `TransferLogic`; společný soubor je kopírován do obou sketch `src`.
- Mezi verzemi nebyly zjištěny odstraněné soubory ani zpětné odebrání funkcí. Změny 0.67 a 0.68 navazují na 0.66.
- Při následné kontrole 0.68 byla nalezena chyba: ESP32 #2 generovalo `peerBootId` a nulovalo sekvenci uvnitř `startPeerEthernet()`. Funkce se volá i při opakovaném pokusu o DHCP/W5500, nikoli pouze při bootu. Tím se měnila epocha peeru při pouhém obnovení linky.
- Oprava 0.69 přesouvá inicializaci `peerBootId` do samostatné funkce volané jednou ze `setup()`. `startPeerEthernet()` již identitu ani sekvenci nemění. Při reconnectu tak zůstává stejné `bootId` a čítač pokračuje; po skutečném restartu se vytvoří nová epocha. `PeerStatusMonitor` navíc uchovává poslední čtyři opuštěné epochy a odmítá opožděné datagramy z těchto starších běhů.

## 2. Matice shody s MASTER 1.3

| Oblast Master 1.3 | Stav v DEV KIT 0.69 | Omezení / co zbývá |
|---|---|---|
| Dvě ESP32 a rozdělení odpovědností | Částečně implementováno | Dva samostatné sketch projekty existují; fyzická spolupráce není ověřena. ESP32 #2 má lokální bezpečnostní logiku. |
| Čerpadla podle vlastní tlakové automatiky, ESP32 jen bezpečnostně | Logika částečně implementována | Relé ani čerpadla nejsou fyzicky řízena. |
| Oddělené minimum ventilu a kritická hladina čerpadla, hystereze | Logika implementována a nativně testována | Skutečné meze musí být nastaveny podle instalace; hardware neověřen. |
| Nouzový režim při vadném čidle | Logika a explicitní UI implementovány | Fyzický provoz a chování na zařízení neověřeny. |
| Pouze směry studna→dolní a horní→dolní, ruční litry | Simulační automat implementován | Není to fyzický transfer; žádný ventil ani čerpadlo není připojeno. |
| Dolní nádrž 220×280 cm, 61,6 l/cm, měřicí rozsah 0–121 cm; 11 cm pouze informativní údaj | Matematika implementována | Výpočet a rozsah jsou nativně testované; instalace a snímač neověřeny. 11 cm je pouze informativní hranice běžného vyčerpání a neovlivňuje řízení; obecná matematická funkce dovoluje fyzicky validní měření 0–121 cm. |
| Studna, geometrie, kalibrace a lokální překalibrace | Přenositelné moduly implementovány a nativně testovány | Skutečná kalibrace, absolutní kotevní bod a NVS na desce nejsou ověřeny. |
| Snímače 4–20 mA, oddělená elektrická a objemová kalibrace | Matematika a konfigurace implementovány | Proudové čidlo, 150 Ω, ADC, ochrana vstupu a kabeláž nejsou fyzicky ověřeny. |
| Denní přítok a 30denní historie | Logika a NVS adaptér implementovány, nativně testovány | Reálný čas, skutečná měření a NVS na ESP32 neověřeny. |
| Teplota horní nádrže pouze informativně | Zatím neimplementováno | Rámec posílá `temperatureValid=0`; teplotní čidlo se nečte. |
| Stavová komunikace ESP32 #2→#1 | W5500/UDP status implementován, nativní protokol testován | CRC není autentizace. DHCP/broadcast a skutečný transport nebyly ověřeny. `bootId` nyní zůstává stabilní přes reconnect a opožděné rámce z posledních čtyř opuštěných epoch se odmítají (oprava 0.69). |
| Výpadek peer dat při transferu z horní nádrže | Simulační automat fail-safe ošetřuje | Výpadek odmítne PRECHECK nebo ukončí RUNNING; skutečný transport a ventil nejsou fyzicky ověřeny. |
| Autentizované handshake a COMMAND/RESPONSE/ACK | Přenositelné stavební moduly existují | Nejsou propojeny s W5500/UDP. Bezpečný provisioning a chráněný produkční klíč chybí. |
| Transfer po restartu | Restart guard + dvouslotový NVS žurnál připojen k simulaci | Hostitelský stub nenahrazuje power-cut test na ESP32. Výstupní GPIO není připojeno. |
| Web, Wi-Fi a AP bez internetu | Kód implementován | Fyzický test AP/Wi-Fi a celé UI na zařízení chybí. |
| Vzdálený přístup přes VPN | Pouze návrh | Router, WAN/CGNAT a WireGuard nejsou ověřeny ani nastaveny. |
| OTA obou jednotek | Upload handlery existují | Chybí build, partition/rollback a bezpečná koordinace obou jednotek. |
| Stav implementace / build / hardware | Dokumentace rozlišuje úrovně | Arduino-ESP32 build ani fyzické testy nebyly provedeny. |

## 3. Bezpečnostní blokátory

1. Produkční backend a fyzický provisioning společného tajemství nejsou hotové. `PeerSecretVault` je přenositelný kontrakt, nikoli hotové zabezpečené NVS úložiště.
2. Přihlašovací údaje AP a webového administrátora se stále vypisují do Serial Monitoru. Před odstraněním výpisu je nutné dodat bezpečnou cestu prvotního získání a obnovy hesel.
3. Handshake koordinátory a `PeerCommandSession` nejsou napojeny na UDP/W5500 smyčku. Neexistuje aktivní autentizovaný řídicí transport.
4. Stavové UDP zprávy jsou chráněny CRC, nikoli HMAC; smějí sloužit pouze pro stavový dashboard a simulační logiku na důvěryhodné LAN.
5. Fyzické výstupy čerpadel a ventilů zůstávají odpojené a zakázané. Žádný simulační výstup `VALVE_OPEN_*` není povolením pro fyzické GPIO.

## 4. Testy a ověření

- `tests/run_native_tests.sh`: 45/45 testovacích programů prošlo.
- Kompletní sada s AddressSanitizer + UndefinedBehaviorSanitizer: 45/45 prošlo.
- `firmware/common/` musí být bajtově shodné s oběma sketch `src`; tato kontrola je součástí vydání.
- Arduino-ESP32 build, fyzické NVS, power-cut, W5500, senzory, relé, ventily a síť nejsou ověřeny.

### Dodatek po peer protokolovém auditu 0.70

- Sekvenční porovnání používá wrap-safe serial arithmetic bez převodu `uint32_t` na `int32_t`; přetečení `0xFFFFFFFF → 0` je přijato.
- Rozdíl přesně `0x80000000` je považován za nejednoznačný a odmítnut.
- Testována je také kapacita čtyř retired boot ID. Historie je záměrně omezená: po pěti přechodech epochy je nejstarší ID vyřazeno. Tato krátká ochrana není persistentní anti-replay a nesmí být považována za autentizaci.
- Standardní sada a ASan/UBSan prošly 45/45; statický invariant boot identity PASS.

### Dodatek po kontrole DHCP recovery 0.71

- ESP32 #2 po `Ethernet.maintain()` kontroluje ztrátu DHCP adresy; při `localIP()[0] == 0` zavře UDP socket a vrací transport do retry režimu.
- ESP32 #1 při stejné detekci rovněž zavře UDP socket před opakováním inicializace.
- Statická kontrola obou cest PASS; nativní a ASan/UBSan sady 45/45.
- Jde pouze o detekci ztráty DHCP adresy. Samotný fyzický výpadek kabelu, při kterém knihovna může stále vracet dříve přidělenou IP, není tímto ověřen ani nutně detekován.

## 5. Další bezpečný postup

1. Pokračovat transportním auditem detekce fyzického link-down, znovuzískání DHCP a UDP socketu po reconnectu; statické DHCP recovery z 0.71 ještě není hardware ověřená.
2. Poté připravit transportní orchestraci handshake a COMMAND/RESPONSE pouze jako neaktivní/testovatelnou vrstvu; fyzické výstupy zůstanou zakázané.
3. Před síťovým řízením uzavřít provisioning a chráněné úložiště tajemství, bezpečné heslo/AP workflow a persistentní anti-replay.
4. Až následně provést cílový Arduino build a testy na deskách bez připojených výkonových zařízení.
5. Fyzické řízení navrhovat až po potvrzení skutečného zapojení ventilů, relé a jejich bezpečného stavu při resetu a ztrátě napájení.

**Závěr:** DEV KIT 0.66 → 0.67 → 0.68 tvoří návaznou vývojovou řadu a odpovídá směru MASTER 1.3. Nejde však o hotový systém podle Masteru. Revize 0.69 opravuje nalezenou chybu boot epochy a sjednocuje dokumentaci; neaktivuje síťové řízení ani fyzické výstupy.
