# REV38 – propagace důvěryhodnosti elektrické elektrické nastavení snímače snímačů

**Základ:** REV37 auditní nález  
**Rozsah:** ESP32 #1 (studna + dolní nádrž), ESP32 #2 (horní nádrž), pumpové ochrany, transfer a peer status.

## Důležité rozlišení pojmů

Tento dokument řeší výhradně **elektrické nastavení snímače** (`SensorConfig`): převod proudu 4–20 mA na výšku hladiny. Neřeší objemovou kalibraci studny (`WellCalibration`), která převádí již naměřenou výšku na litry.

Objemová kalibrace studny se používá pouze při výpočtu objemu. Její chybění, poškození nebo nedostupnost sama o sobě nesmí blokovat čerpadlo ani přečerpávání. Toto pravidlo zůstává zachováno.

## Implementovaný stavový model

Firmware nyní rozlišuje čtyři stavy `SensorConfigLoadState`:

- `SENSOR_CONFIG_MISSING_PROVISIONAL`: NVS namespace je dostupný, klíč elektrické nastavení snímače neexistuje. Výchozí hodnoty zůstávají použitelné pro první zprovoznění podle dosavadního návrhu; nejsou automaticky zapisovány. Webové rozhraní je označí jako provizorní.
- `SENSOR_CONFIG_VALID_STORED`: uložený záznam prošel kontrolou verze, rozsahů a checksumu.
- `SENSOR_CONFIG_INVALID_PRESENT`: klíč existuje, ale záznam nelze ověřit. Výchozí hodnoty jsou ponechány pouze pro diagnostický výpočet; nejsou považovány za důvěryhodné pro běžné bezpečnostní rozhodování.
- `SENSOR_CONFIG_NVS_UNAVAILABLE`: NVS namespace se nepodařilo otevřít; nelze určit, zda záznam chybí nebo je poškozený. Výchozí hodnoty jsou pouze diagnostické.

## Bezpečnostní propojení

- Studniční pumpa: použitelnost elektrického nastavení snímače je součástí `sensorValid` předaného do `WellPumpProtection`.
- Dolní nádrž: použitelnost elektrického nastavení snímače je součástí `sensorValid` předaného do `LowerTankPumpProtection`.
- Horní nádrž: použitelnost elektrického nastavení snímače je součástí lokální platnosti čidla předané do `UpperTankProtection`.
- Automatický transfer: důvěryhodnost dolní nádrže a studny je součástí `lowerLevelValid` a `sourceLevelValid`. Horní zdroj se blokuje nepřímo přes peer status.
- ESP32 #2 neoznačí horní hladinu jako použitelnou v UDP peer rámci, pokud elektrické nastavení snímače není `MISSING_PROVISIONAL` nebo `VALID_STORED`. Tím se zavře i navazující horní transfer na ESP32 #1.
- Servisní ruční ventilový režim vyžaduje důvěryhodnou elektrické nastavení snímače relevantního snímače. Nouzový ruční režim nadále zachovává předchozí výslovný fyzický override; pokud elektrické nastavení snímače nelze považovat za důvěryhodnou, studniční snímač se pro tento guard vyhodnotí jako nedostupný.
- Výslovně potvrzený nouzový režim každého čerpadla zůstává podle uživatelova rozhodnutí schopen obejít softwarové chyby čidla/elektrické nastavení snímače/NVS. Neobchází fyzickou připravenost relé ani nezávislé elektrické a motorové ochrany. Po restartu se nouzový režim neobnovuje.

## Diagnostika

Webová rozhraní zobrazují stav elektrické nastavení snímače odděleně od elektrické/math platnosti měření. Rozlišují uloženou elektrické nastavení snímače, provizorní výchozí hodnoty, poškozený záznam a nedostupné NVS. Při nedůvěryhodném stavu může být naměřená hodnota stále zobrazena pro diagnostiku, ale běžná pumpová ochrana ji nepoužije.

Úspěšné ověřené uložení nové elektrické nastavení snímače na ESP32 #1 přepne příslušný stav na `VALID_STORED` až po kontrolním načtení z NVS. Již existující poškozený záznam nelze běžným webovým formulářem přepsat; ochrana proti tichému přepsání zůstává zachována. Samostatný servisní postup pro vědomou opravu takového záznamu zůstává otevřeným bodem.

## Testy a limity

- `test_sensor_config`: ověřuje politiku důvěryhodnosti všech čtyř stavů.
- `check_sensor_calibration_trust_integration.sh`: kontroluje rozlišení stavů při startu, jejich použití ve všech třech pumpových ochranách a v horním peer statusu.
- `tests/run_native_tests.sh`: 65 nativních programů a integrační kontroly.
- Hostitelské testy neověřují fyzické NVS, skutečné výpadky napájení, chování relé na motorových obvodech ani síťový provoz skutečných ESP32.
- Arduino-ESP32 cílová kompilace a fyzické testy zůstávají neprovedené.

## Záměrné omezení

Chybějící záznam je stále povolen jako provizorní stav pro první uvedení do provozu; dashboard jej výrazně označuje. Tato revize nemění dohodnutou politiku prvního zprovoznění. Pokud bude později požadováno, aby i první instalace vyžadovala explicitní kalibrační commissioning, půjde o samostatné rozhodnutí a změnu bezpečnostní politiky.
