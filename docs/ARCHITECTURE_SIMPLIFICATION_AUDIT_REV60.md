# HOME WATER CONTROLLER — audit zjednodušení architektury REV60

Datum: 2026-11-14  
Východisko: uživatelem dodaný archiv REV59  
Stav této revize: dokumentační audit a potvrzení cílové architektury; provozní firmware se nemění.

## 1. Potvrzené cílové rozhodnutí

ESP32 #1 a ESP32 #2 budou dvě samostatné jednotky. Nebude mezi nimi aplikační komunikace, synchronizace konfigurace, společný klíč, UDP peer protokol ani vzájemné povolování funkcí.

- **ESP32 #1:** studna, dolní nádrž, všechny transferové ventily, hlavní web a řízení transferů. Uživatel zadá požadovaný objem v litrech. Transfer horní → dolní se ovládá z webu ESP32 #1 stejně jako transfer ze studny.
- **ESP32 #2:** horní nádrž, místní ochrana horního čerpadla podle minimální hladiny a hystereze, zobrazení hladiny a objemu v litrech, teplota vody a vlastní web.
- Obě jednotky mají samostatný přístup do domácí sítě/internetu. ESP32 #2 již používá W5500 Ethernet; ten lze zachovat pro připojení do routeru/switche. Jeho UDP peer vysílání se odstraní. ESP32 #1 zůstává na Wi-Fi STA. Vzdálený přístup bude přes router/VPN, ne přímým vystavením ESP32 do internetu.
- Výpadek nebo restart jedné jednotky neovlivní lokální ochranu druhé. Po restartu se žádný transfer automaticky neobnoví.

## 2. Přesná hranice odpovědnosti při transferu horní → dolní

ESP32 #1 nezná hladinu horní nádrže a nepotřebuje ji znát. Předpokladem bezpečného provozu je, že ESP32 #2 samostatně řídí bezpečnostní relé horního čerpadla a při dosažení svého minima / chybě vlastního měření odebere povolení čerpadlu.

ESP32 #1 při požadavku:

1. ověří vlastní čerstvé a platné měření dolní nádrže, požadované litry, volnou kapacitu, připravenost ventilového výstupu a stav transferového žurnálu;
2. otevře pouze ventil horní → dolní; čerpadlo běžně spíná jeho vlastní tlaková automatika, ESP32 jej nereguluje;
3. sleduje přírůstek objemu dolní nádrže podle místního čidla ESP32 #1;
4. ukončí transfer po dosažení zadaného objemu, při STOP, chybě měření, dosažení limitu dolní nádrže, timeoutu nebo při nedostatečném/žádném přírůstku hladiny;
5. při každém ukončení přikáže bezpečný stav ventilů (CLOSE_ALL), uloží výsledek a neobnoví transfer automaticky.

### Hlídání nulového nebo přerušeného přítoku

Toto hlídání je výslovně potvrzený požadavek. Není potřeba další průtokoměr: primárním indikátorem je změna hladiny/objemu dolní nádrže.

- Po otevření ventilu se použije omezená doba pro rozběh průtoku.
- Po jejím uplynutí musí být zjištěn měřitelný kladný přírůstek nad šum/rozlišení snímače.
- Během transferu se sleduje, zda přírůstek pokračuje. Pokud se po stanovenou dobu neobjeví dostatečný pokrok, transfer se přeruší.
- Časy a minimální přírůstek se neurčují odhadem v této revizi. Musí vycházet z reálné rychlosti čerpání, intervalu vzorkování a šumu instalovaného snímače; po oživení se ověří na zařízení.
- Neplatné nebo zastaralé měření dolní nádrže znamená okamžité bezpečné ukončení transferu.
- Tato kontrola nedokazuje mechanické otevření ventilu; bez koncového spínače zůstává poloha ventilu pouze povelovým stavem.
- Odběr vody z dolní nádrže během transferu může maskovat přírůstek a zkreslit vypočtené množství. V první verzi se souběžný odběr nekompenzuje; při kalibraci i přesném transferu se nemá z dolní nádrže současně odebírat.

## 3. Měření množství transferu

Dolní nádrž zůstává primárním měřidlem množství přenesené vody. Její aktuální známý převod je 61,6 l/cm (vnitřní půdorys 220 × 280 cm). ESP32 #1 vypočítá rozdíl objemu mezi začátkem a průběhem transferu. Čas je pouze záložní timeout, nikoli hlavní měření.

Pro transfer ze studny zůstává zachována místní validace hladiny zdroje a oddělení elektrické kalibrace snímače od objemové kalibrace studny. Pro transfer z horní nádrže se zdrojová hladina nekontroluje na ESP32 #1; ochranu zdroje zajišťuje samostatně ESP32 #2.

## 4. Horní nádrž — objemová kalibrace v ESP32 #2

V REV59 není objemová kalibrace horní nádrže integrována. ESP32 #2 nyní posílá `upperVolumeLiters = 0` a `upperVolumeValid = 0`; teplotní akvizice rovněž není integrována. To je důležitá skutečná mezera, nikoli hotová funkce.

Cílová funkce:

- Oddělit elektrickou kalibraci snímače (4–20 mA → výška v cm) od objemové kalibrace nádrže (výška v cm → litry).
- Použít uživatelsky editovatelnou tabulku kalibračních bodů `výška cm → objem litrů`, s interpolací mezi sousedními body a jasným označením odhadu mimo kalibrovaný rozsah.
- Ukládat body do samostatného NVS záznamu ESP32 #2, s verzí, kontrolním součtem a ověřením zápisu zpětným načtením.
- Kalibrační body se nemění automaticky při každém transferu. Změna proběhne jen po výslovném úkonu uživatele v kalibrační části webu ESP32 #2.

### Ruční postup kalibrace podle množství přečerpaného do dolní nádrže

1. Uživatel jednorázově zadá známý výchozí absolutní objem při aktuálně změřené hladině (referenční kotevní bod). Bez tohoto bodu nelze zobrazovat důvěryhodnou absolutní zásobu v litrech.
2. Po transferu uživatel převezme skutečně přečerpané litry z výsledku ESP32 #1 a zadá je do kalibračního formuláře ESP32 #2.
3. ESP32 #2 načte aktuální hladinu horní nádrže a vypočítá nový objem jako předchozí známý objem minus zadané odebrané litry.
4. Uloží nový bod (aktuální výška, vypočtený absolutní objem) a zachová ostatní platné body. Při odběru mimo tento kalibrační postup se křivka automaticky nemění.
5. Uživatel musí zajistit, aby mezi referenčním bodem a zadávaným transferem nedošlo k jinému nezapočítanému přítoku/odběru. Přesnost je omezena také přesností měření přírůstku dolní nádrže.

Pokud není známý absolutní kotevní objem nebo je k dispozici méně než dva vhodné body pro interpolaci, UI musí objem označit jako neověřený / kalibrace nedostatečná. Nesmí zobrazovat falešně přesné litry. Mimo kalibrovaný rozsah může zobrazit pouze zřetelně označený odhad, nikoli měřenou hodnotu.

## 5. Horní nádrž — ochrana a teplota

- ESP32 #2 ponechá vlastní čidlo hladiny, vlastní elektrickou kalibraci a vlastní bezpečnostní relé GPIO25.
- Ochrana čerpadla zůstane místní: nastavitelné minimum (kritická vypínací hladina) a hystereze/obnovovací hladina. Ztráta ESP32 #1, domácí Wi-Fi nebo internetu nesmí tuto ochranu ovlivnit.
- Odstraní se horní transferová rezerva a ventilová hystereze, pokud po oddělení jednotek nemají jiný místní účel. Přesné zjednodušení datové struktury limitů se provede až při implementaci a migraci NVS.
- Teplota je informativní údaj a sama neovládá čerpadla ani ventily. V REV59 je GPIO4 pouze rezervované pro DS18B20; vlastní čtení teploty není integrováno.

## 6. Co z REV59 zachovat

- Veškeré tři samostatné ochrany čerpadel a jejich fyzická bezpečnostní relé/polaritu; žádná změna tlakových automatik.
- Elektrickou kalibraci a validaci snímačů, ochranu proti neplatným/stárnoucím měřením, recovery pravidla a NVS kontrolní součty.
- Řízení ventilů ESP32 #1, break-before-make, CLOSE_ALL při STOP/chybě, žurnál transferu, historii, ochranu proti opakovanému požadavku a zákaz automatického obnovení po restartu.
- Webové přihlášení, rate limiter, CSRF/Origin kontrolu, OTA bezpečnostní interlock, lokální AP fallback a samostatné weby obou jednotek.
- Studniční objemovou kalibraci a geometrii; dolní nádrž zůstává lineární podle známé geometrie.
- W5500 na ESP32 #2 jako jeho síťové připojení k routeru/switchi (pokud bude fyzicky připojeno). Odstraní se pouze peer UDP broadcast; Ethernet se nepoužívá pro vzájemnou komunikaci jednotek.

## 7. Co odstranit nebo zjednodušit

Po kontrole všech referencí a po oddělených testech odstranit z provozních sketchů a následně z balíčku:

- ESP32 #2 UDP status broadcast a ESP32 #1 UDP peer listener;
- `PeerStatusMonitor`, peer status rámce a jejich CRC/boot epoch/freshness logiku;
- handshake HELLO/CHALLENGE/FINISH/READY, session keys, peer command session, replay store/guard, peer secret vault, HMAC status codec a peer-key USB provisioning;
- ESP32 #1 závislost transferu horní → dolní na peer freshness, peer boot ID, peer checksum a zrcadlené horní limity;
- duplikovanou konfiguraci horní transferové rezervy/ventilové hystereze na ESP32 #1.

Před smazáním souborů je nutné ověřit, že nemají jiného konzumenta. `HmacSha256` a obecné datové typy se odstraní pouze tehdy, pokud po odpojení peer vrstvy nezůstane jiná reference. Nesmí se mazat ochrana čerpadel, webové přihlašování, OTA, obecná senzorová matematika ani transferový journal.

## 8. Síťová architektura po zjednodušení

- ESP32 #1: Wi-Fi STA do domácího routeru; lokální web a internet/VPN.
- ESP32 #2: W5500 Ethernet do stejného routeru nebo switche; lokální web dostupný v LAN a přes VPN. Stávající servisní AP lze zachovat pro místní servis, ale není mostem do internetu.
- Není potřeba, aby jednotky navzájem znaly IP adresu, port nebo stav.
- Pokud fyzické připojení W5500 do LAN nebude možné, musí se samostatně rozhodnout o Wi-Fi STA pro ESP32 #2; nesmí se kvůli tomu obnovovat aplikační peer komunikace.

## 9. Pořadí implementace

1. Upravit master dokumentaci a odstranit rozpory s peer architekturou.
2. Oddělit transfer horní → dolní v ESP32 #1 od všech peer dat; přidat testy, že upper transfer vyžaduje platné měření dolní nádrže, nikoli vzdálený peer.
3. Doplnit detekci nulového/přerušeného přítoku podle dolní nádrže; otestovat rozběhové okno, šum, stárnutí čidla, ztrátu přítoku, STOP, timeout a restart.
4. Doplnit objemovou kalibraci horní nádrže v ESP32 #2 včetně absolutního kotevního bodu, ručního zadání odebraných litrů, interpolace, NVS a webového zobrazení.
5. Integrovat teplotní čidlo DS18B20 na již rezervovaném GPIO4; ověřit knihovnu/verzi a fyzické zapojení.
6. Odpojit a odstranit peer UDP a peer security moduly po ověření, že už nejsou použity.
7. Ověřit samostatný internet/LAN přístup obou jednotek, poté Arduino build obou sketchů a nakonec hardware testy.

## 10. Stav ověření REV59 při auditu

- ZIP: 737 položek; `ZipFile.testzip()` bez chyby.
- Nativní C++ sada: 72/72 programů PASS; statické integrační kontroly v `tests/run_native_tests.sh` PASS po obnovení spustitelného bitu shell skriptů v pracovní kopii (ZIP je při rozbalení v tomto prostředí předal bez spustitelného bitu).
- Python: 17/17 PASS.
- Arduino-ESP32 cílový překlad: NEPROVEDEN.
- Fyzické testy obou ESP32, čidel, ventilů, relé, W5500, DS18B20 a internetu: NEPROVEDENY.
- Tyto výsledky ověřují původní REV59, nikoli novou zjednodušenou architekturu. REV60 nemění firmware.
