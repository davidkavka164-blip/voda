# REV39 – oddělení výšky hladiny a objemové kalibrace

## Závazné rozlišení

Projekt obsahuje dvě nezávislé věci, které dříve používaly v názvech podobné slovo „kalibrace“:

1. `SensorConfig` – elektrické nastavení převodu snímače 4–20 mA na výšku hladiny. Obsahuje odpor bočníku, rozsah proudu, rozsah snímače a korekci měřítka/posunu. Je součástí samotného výpočtu výšky.
2. `WellCalibration` – objemová křivka studny, která převádí naměřenou výšku na objem v litrech. Neovlivňuje výšku ani povolení čerpadel a transferu.

## Pravidlo pro provoz

- Čerpadla a transfer se rozhodují podle platné, čerstvé a fyzikálně přípustné **výšky hladiny** a podle samostatných ochranných mezí.
- Chybějící, neúplná nebo neplatná objemová křivka `WellCalibration` sama o sobě nikdy nesmí blokovat čerpadlo ani transfer. V takovém případě je objem pouze odhadnutý z geometrie nebo označen jako nedostupný podle možností výpočtu.
- Stav elektrického nastavení `SensorConfig` je odlišný: při chybějícím záznamu, poškozeném záznamu nebo nedostupném NVS se výška získaná z náhradních diagnostických parametrů nepovažuje za důvěryhodnou pro běžnou ochranu. Provizorní výchozí parametry slouží pouze k diagnostice a uvedení zařízení do provozu; samy o sobě nikdy nepovolují běžný chod čerpadla ani transferu.
- Běžné bezpečnostní použití výšky je povoleno až po uložení a úspěšném ověření elektrického nastavení konkrétního snímače. Tím se brání tomu, aby elektricky živý, ale výškově nekalibrovaný snímač poskytl zdánlivě platnou hodnotu a způsobil například chod čerpadla na sucho.
- Nouzové režimy čerpadel zůstávají řízeny dříve schváleným explicitním fyzickým potvrzením a pravidly nezávislými na této změně.

## Provedené zpřesnění v REV39

- `SensorCalibrationState` bylo přejmenováno na `SensorConfigLoadState`.
- Stavy `SENSOR_CALIBRATION_*` byly přejmenovány na `SENSOR_CONFIG_*`.
- Pomocná funkce `sensorCalibrationTrustedForSafety` byla přejmenována na `sensorConfigUsableForLevelMeasurement`.
- Proměnné `wellCalibrationState`, `lowerCalibrationState` a `upperCalibrationState` byly přejmenovány na `wellSensorConfigState`, `lowerSensorConfigState` a `upperSensorConfigState`.
- Dashboard výslovně zobrazuje „Elektrické nastavení snímače“, aby jej nebylo možné zaměnit s objemovou kalibrací.
- Chování řízení a ochran se v této revizi nemění; pouze se zpřesňuje názvosloví a dokumentace.

## Omezení ověření

Hostitelské testy a statické kontroly mohou potvrdit propojení zdrojového kódu. Neověřují fyzickou přesnost převodu 4–20 mA na výšku, skutečné NVS ani chování čerpadel na zařízení. Arduino cílový build a hardwarové testy zůstávají otevřené.


## REV40 – blokace při chybějícím elektrickém nastavení

- Upřesnění provozního pravidla: `SENSOR_CONFIG_MISSING_PROVISIONAL` není použitelný stav pro bezpečnostní rozhodování.
- `sensorConfigUsableForLevelMeasurement()` nyní povoluje pouze `SENSOR_CONFIG_VALID_STORED`.
- Chybějící záznam, poškozený záznam i nedostupné NVS blokují běžné použití výšky pro ochranu čerpadel a transfer. Výchozí parametry zůstávají pouze diagnostické.
- Po úspěšném uložení a kontrolním načtení konfigurace se příslušný snímač stává použitelným.
- Objemová kalibrace `WellCalibration` zůstává zcela oddělena a tato změna ji neovlivňuje.
