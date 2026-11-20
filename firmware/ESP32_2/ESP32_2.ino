/*
 * HOME WATER CONTROLLER — ESP32 #2 (upper tank unit)
 * Independent upper-tank unit: local level sensing, pump protection, configuration UI, and W5500 network interface.
 * Upper pump safety relay is enabled and follows the local pump protection permit.
 */
#include "src/HardwarePins.h"
#include "src/HttpOriginPolicy.h"
#include "src/SensorMath.h"
#include "src/SensorConfig.h"
#include "src/NvsSensorConfigStore.h"
#include "src/LevelSensorReader.h"
#include "src/WaterMath.h"
#include "src/TransferLogic.h"
#include "src/WellPumpProtection.h"
#include "src/UpperTankLimits.h"
#include "src/NvsUpperTankLimitsStore.h"
#include "src/NvsUpperTankProtectionStore.h"
#include "src/UpperTankProtection.h"
#include "src/PumpSafetyRelayOutput.h"
#include <ETH.h>
#include <atomic>
#include <Network.h>
#include "src/WifiStaCredentialStore.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>
#include "src/NvsCredentialTextStorage.h"
#include "src/CredentialStore.h"
#include "src/NvsCredentialBackend.h"
#include "src/CredentialLegacyMigrator.h"
#include "src/CredentialRandom.h"
#include "src/WebAuthRateLimiter.h"
#include "src/WaterTemperature.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <cmath>
#include <Esp.h>
#include <esp_system.h>

namespace {
// ESP-IDF 3.3 documents esp_random() as true-random only while Wi-Fi or
// Bluetooth RF is enabled. Start the STA interface (without connecting) before
// creating first-install credentials; if Wi-Fi cannot start, fail closed.
bool enableCredentialRandomSource() {
    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_STA)) {
        Serial.println("ERROR: Wi-Fi RF could not be initialized; new credentials will not be generated.");
        return false;
    }
    return true;
}

uint32_t credentialRandomWordSource() {
    return esp_random();
}
HWC::NvsSensorConfigStore configStore;
HWC::SensorConfig upperTankConfig;
HWC::SensorConfigLoadState upperSensorConfigState = HWC::SENSOR_CONFIG_NVS_UNAVAILABLE;
HWC::LevelSensorReader* upperTankSensor = nullptr;
WebServer localWebServer(80);
HWC::WebAuthRateLimiter webAuthRateLimiter;
Preferences uiPreferences;
String localApName;
String localApPassword;
String localAdminPassword;
bool localWebReady = false;
bool localOtaAuthorized = false;
bool localOtaStarted = false;
bool localOtaSucceeded = false;
String localOtaError;
bool haveUpperReading = false;
HWC::LevelSensorReading lastUpperReading = {};
bool upperEmergencyEnableRequested = false;
bool upperEmergencyConfirmationRequested = false;
bool upperEmergencyDisableRequested = false;
String savedWifiSsid;
String savedWifiPassword;
uint32_t lastWifiAttemptMs = 0;
bool wifiReconnectRequested = false;
bool wifiAttemptPending = false;
bool wifiStaDisabled = false;
uint32_t lastManagementLinkMs = 0;
uint32_t lastServiceApAttemptMs = 0;
bool serviceApActive = false;
constexpr uint32_t MANAGEMENT_LINK_GRACE_MS = 30000U;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 30000U;
enum class EthernetStartup { Pending, Ready, Failed };
std::atomic<EthernetStartup> ethernetStartup{EthernetStartup::Pending};
IPAddress reportedEthernetIp;
HWC::NvsUpperTankLimitsStore upperLimitsStore("hwc_upperlimits");
HWC::NvsUpperTankProtectionStore upperSafetyStore("hwc_up_safety");
HWC::UpperTankSafetyState upperSafetyState = {false, false};
bool upperSafetyStorageReady = false;
bool upperSafetyRecordPresent = false;
HWC::UpperTankLimits upperTankLimits = HWC::emptyUpperTankLimits();
HWC::UpperTankProtection upperPumpProtection;
HWC::UpperTankProtectionState upperPumpProtectionState = {false, false, true, true, true, false};
HWC::PumpSafetyRelayOutput upperPumpSafetyRelayOutput;
bool upperPumpSafetyRelayReady = false;
bool upperProtectionConfigValid = false;
bool upperProtectionStorageReady = false;
uint32_t lastReportMs = 0;
constexpr uint32_t WATER_TEMPERATURE_SAMPLE_INTERVAL_MS = 10000U;
constexpr uint32_t WATER_TEMPERATURE_CONVERSION_MS = 750U; // DS18B20 at 12-bit resolution
constexpr uint32_t WATER_TEMPERATURE_MAX_AGE_MS = 15000U;
constexpr uint32_t WATER_TEMPERATURE_RESCAN_MS = 30000U;
OneWire waterTemperatureOneWire(HwcPins::Esp32_2::WATER_TEMPERATURE_DATA);
DallasTemperature waterTemperatureBus(&waterTemperatureOneWire);
DeviceAddress waterTemperatureAddress;
HWC::WaterTemperatureReading waterTemperatureReading = HWC::emptyWaterTemperatureReading();
bool waterTemperatureDeviceReady = false;
bool waterTemperatureConversionPending = false;
uint32_t waterTemperatureConversionStartedMs = 0U;
uint32_t waterTemperatureLastRequestMs = 0U;
uint32_t waterTemperatureLastDiscoveryMs = 0U;

void discoverWaterTemperatureSensor(uint32_t nowMs) {
    waterTemperatureDeviceReady = false;
    waterTemperatureConversionPending = false;
    waterTemperatureBus.begin();
    waterTemperatureBus.setWaitForConversion(false);
    const uint8_t count = waterTemperatureBus.getDeviceCount();
    if (count != 1U || !waterTemperatureBus.getAddress(waterTemperatureAddress, 0U)) {
        HWC::updateWaterTemperatureReading(waterTemperatureReading, false, false, 0.0f, nowMs);
        Serial.printf("WATER_TEMPERATURE: sensor missing or ambiguous (devices=%u); GPIO=%u\n",
                      static_cast<unsigned>(count), HwcPins::Esp32_2::WATER_TEMPERATURE_DATA);
        waterTemperatureLastDiscoveryMs = nowMs;
        return;
    }
    // DS18B20 family code is 0x28. Do not silently accept another 1-Wire device.
    if (waterTemperatureAddress[0] != 0x28U || !waterTemperatureBus.isConnected(waterTemperatureAddress)) {
        HWC::updateWaterTemperatureReading(waterTemperatureReading, false, false, 0.0f, nowMs);
        Serial.println("WATER_TEMPERATURE: device is not a connected DS18B20; measurement disabled.");
        waterTemperatureLastDiscoveryMs = nowMs;
        return;
    }
    waterTemperatureBus.setResolution(waterTemperatureAddress, 12U);
    waterTemperatureDeviceReady = true;
    waterTemperatureLastDiscoveryMs = nowMs;
    waterTemperatureLastRequestMs = nowMs - WATER_TEMPERATURE_SAMPLE_INTERVAL_MS;
    Serial.println("WATER_TEMPERATURE: one DS18B20 found; 12-bit asynchronous conversion enabled.");
}

void maintainWaterTemperature(uint32_t nowMs) {
    if (!waterTemperatureDeviceReady) {
        if (static_cast<uint32_t>(nowMs - waterTemperatureLastDiscoveryMs) >= WATER_TEMPERATURE_RESCAN_MS ||
            waterTemperatureLastDiscoveryMs == 0U) {
            discoverWaterTemperatureSensor(nowMs);
        }
        return;
    }
    if (waterTemperatureConversionPending &&
        static_cast<uint32_t>(nowMs - waterTemperatureConversionStartedMs) >= WATER_TEMPERATURE_CONVERSION_MS) {
        const float value = waterTemperatureBus.getTempC(waterTemperatureAddress);
        const bool valid = HWC::isWaterTemperatureValueValid(value);
        HWC::updateWaterTemperatureReading(waterTemperatureReading, true, valid, value, nowMs);
        waterTemperatureConversionPending = false;
        if (valid) Serial.printf("WATER_TEMPERATURE: %.2f C\n", value);
        else {
            Serial.printf("WATER_TEMPERATURE: invalid reading %.2f C; value marked unavailable.\n", value);
            waterTemperatureDeviceReady = false;
            waterTemperatureLastDiscoveryMs = nowMs;
        }
    }
    if (waterTemperatureDeviceReady && !waterTemperatureConversionPending &&
        static_cast<uint32_t>(nowMs - waterTemperatureLastRequestMs) >= WATER_TEMPERATURE_SAMPLE_INTERVAL_MS) {
        waterTemperatureBus.requestTemperaturesByAddress(waterTemperatureAddress);
        waterTemperatureConversionStartedMs = nowMs;
        waterTemperatureLastRequestMs = nowMs;
        waterTemperatureConversionPending = true;
    }
    if (waterTemperatureReading.status == HWC::WATER_TEMPERATURE_VALID &&
        !HWC::isWaterTemperatureFresh(waterTemperatureReading, nowMs, WATER_TEMPERATURE_MAX_AGE_MS)) {
        waterTemperatureReading.status = HWC::WATER_TEMPERATURE_READ_ERROR;
    }
}

void writePumpSafetyRelayLevel(uint8_t pin, bool high) {
    digitalWrite(pin, high ? HIGH : LOW);
}

void syncUpperPumpSafetyRelay(uint32_t nowMs) {
    const bool readingFresh = haveUpperReading && static_cast<uint32_t>(nowMs - lastReportMs) <= 5000U;
    const bool permit = upperPumpProtectionState.emergencyModeActive ||
        (readingFresh && upperPumpProtectionState.pumpPermit);
    if (!upperPumpSafetyRelayReady || !upperPumpSafetyRelayOutput.applyPermit(permit)) {
        if (upperPumpSafetyRelayReady) upperPumpSafetyRelayOutput.forceOff();
    }
}

bool hasBinExtension(const String& filename) {
    const char* name = filename.c_str();
    const size_t length = strlen(name);
    if (length < 4U) return false;
    return name[length - 4U] == '.' &&
           tolower(static_cast<unsigned char>(name[length - 3U])) == 'b' &&
           tolower(static_cast<unsigned char>(name[length - 2U])) == 'i' &&
           tolower(static_cast<unsigned char>(name[length - 1U])) == 'n';
}

bool parseFloatStrictLocal(const String& text, float& value) {
    if (!text.length() || text.length() > 24) return false;
    char* end = nullptr;
    value = strtof(text.c_str(), &end);
    return end != text.c_str() && end && *end == '\0' && std::isfinite(value);
}

bool isSameOriginPost(WebServer& server) {
    // HTTP Basic Auth is browser-cached. Require explicit same-origin headers
    // on every state-changing POST, independent of the network transport.
    if (server.method() != HTTP_POST || !server.hasHeader("Origin") || !server.hasHeader("Host"))
        return false;
    return HWC::isSameHttpOriginAuthority(server.header("Origin").c_str(),
                                          server.header("Host").c_str());
}

bool requireLocalAuth() {
    if (localWebServer.method() == HTTP_POST && !isSameOriginPost(localWebServer)) {
        localWebServer.send(403, "text/plain; charset=utf-8", "Cross-origin or unverifiable request rejected");
        return false;
    }
    if (localAdminPassword.length() < 12) {
        localWebServer.send(503, "text/plain; charset=utf-8", "Web administrator credentials unavailable");
        return false;
    }
    const IPAddress remoteIp = localWebServer.client().remoteIP();
    const uint32_t clientKey = (static_cast<uint32_t>(remoteIp[0]) << 24) |
                               (static_cast<uint32_t>(remoteIp[1]) << 16) |
                               (static_cast<uint32_t>(remoteIp[2]) << 8) |
                               static_cast<uint32_t>(remoteIp[3]);
    const uint32_t now = millis();
    if (webAuthRateLimiter.isBlocked(clientKey, now)) {
        localWebServer.send(429, "text/plain; charset=utf-8", "Too many failed login attempts; wait 60 seconds.");
        return false;
    }
    if (localWebServer.authenticate("admin", localAdminPassword.c_str())) {
        webAuthRateLimiter.recordSuccess(clientKey);
        return true;
    }
    webAuthRateLimiter.recordFailure(clientKey, now);
    localWebServer.requestAuthentication(BASIC_AUTH, "HWC Upper Tank", "Místní správa horní nádrže");
    return false;
}

void sendLocalResult(const String& message) {
    String h = "<!doctype html><html lang='cs'><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    h += "<title>HWC horní nádrž</title><body style='font:16px Arial;max-width:700px;margin:24px auto;padding:0 14px'><h1>Horní nádrž – výsledek</h1><p>";
    h += message;
    h += "</p><p><a href='/'>Zpět na přehled</a></p></body></html>";
    localWebServer.send(200, "text/html; charset=utf-8", h);
}

void handleLocalRoot() {
    if (!requireLocalAuth()) return;
    String h = "<!doctype html><html lang='cs'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    h += "<title>HWC – horní nádrž</title><style>body{font:16px Arial;max-width:800px;margin:20px auto;padding:0 14px;background:#f4f6f8;color:#17202a}section{background:#fff;padding:16px;margin:14px 0;border-radius:10px}input{box-sizing:border-box;width:100%;padding:9px;margin:5px 0 12px}button{padding:10px 16px}.warn{color:#9b1c1c}</style></head><body><h1>HOME WATER CONTROLLER</h1><h2>ESP32 #2 – ochrana horního čerpadla</h2>";
    h += "<section><h3>Teplota vody – DS18B20</h3>";
    if (HWC::isWaterTemperatureFresh(waterTemperatureReading, millis(), WATER_TEMPERATURE_MAX_AGE_MS)) {
        h += "<p>Aktuální teplota: <b>" + String(waterTemperatureReading.temperatureC, 2) + " °C</b></p>";
    } else {
        h += "<p class='warn'>Teplota není dostupná nebo je měření zastaralé.</p>";
        if (waterTemperatureReading.status == HWC::WATER_TEMPERATURE_SENSOR_MISSING)
            h += "<p class='warn'>Čidlo nebylo nalezeno nebo je na sběrnici více než jedno zařízení.</p>";
        else if (waterTemperatureReading.status == HWC::WATER_TEMPERATURE_OUT_OF_RANGE)
            h += "<p class='warn'>Čidlo vrátilo hodnotu mimo rozsah DS18B20.</p>";
        else if (waterTemperatureReading.status == HWC::WATER_TEMPERATURE_READ_ERROR)
            h += "<p class='warn'>Chyba čtení nebo zastaralý vzorek.</p>";
        else h += "<p class='warn'>Čidlo zatím nebylo inicializováno.</p>";
    }
    h += "<p>Teplota je pouze informativní. Nemá vliv na ochranu čerpadla ani řízení ventilů.</p></section>";
    h += "<section><h3>Ochrana čerpadla proti chodu nasucho</h3><p>Hladina pro ochranu: ";
    if (HWC::sensorConfigUsableForLevelMeasurement(upperSensorConfigState) &&
        haveUpperReading && lastUpperReading.result.status == HWC::SENSOR_MATH_OK) {
        h += String(lastUpperReading.result.calibratedLevelCm, 1) + " cm (elektrický přepočet platný)";
    } else if (haveUpperReading && lastUpperReading.result.status == HWC::SENSOR_MATH_OK) {
        h += "<b class='warn'>NEOVĚŘENÝ DIAGNOSTICKÝ PŘEPOČET: " + String(lastUpperReading.result.rawLevelCm, 1) + " cm surově; nepoužívat jako výšku pro ochranu</b>";
    } else h += "<b class='warn'>NEPLATNÁ / NEDOSTUPNÁ</b>";
    h += "</p><p>Elektrická kalibrace: ";
    h += upperSensorConfigState == HWC::SENSOR_CONFIG_VALID_STORED ? "ULOŽENÁ A OVĚŘENÁ" :
         (upperSensorConfigState == HWC::SENSOR_CONFIG_MISSING_PROVISIONAL ? "CHYBÍ OVĚŘENÉ NASTAVENÍ – výška není použitelná pro ochranu" :
          (upperSensorConfigState == HWC::SENSOR_CONFIG_INVALID_PRESENT ? "POŠKOZENÉ ELEKTRICKÉ NASTAVENÍ – výška není použitelná pro ochranu" : "NVS NEDOSTUPNÉ – elektrické nastavení není ověřeno"));
    h += "</p><p>Povolení čerpadla: "; h += upperPumpProtectionState.pumpPermit ? "POVOLENO" : "BLOKOVÁNO";
    h += "</p><p>Povel bezpečnostnímu relé: ";
    h += !upperPumpSafetyRelayReady ? "CHYBA / NEPŘIPRAVENO" : (upperPumpSafetyRelayOutput.energized() ? "POVOLIT MOTOR" : "ODPOJIT MOTOR");
    h += "</p><p>Nouzový režim: "; h += upperPumpProtectionState.emergencyModeActive ? "AKTIVNÍ" : "vypnutý";
    h += "</p><p>Kritická blokace: "; h += upperPumpProtectionState.criticalLevelLatched ? "AKTIVNÍ" : "neaktivní";
    h += "</p><p>Konfigurace/NVS: "; h += (upperProtectionConfigValid && upperProtectionStorageReady && upperSafetyStorageReady) ? "platná" : "NEPLATNÁ / nedostupná";
    h += "</p><p class='warn'><b>Nouzový režim po fyzickém ověření hladiny obchází softwarovou ochranu hladiny, kritickou blokaci i chybu konfigurace/NVS. Obsluha přebírá průběžnou kontrolu hladiny. Neobchází jistič, tepelnou/elektrickou ochranu motoru ani fyzickou připravenost bezpečnostního relé. Režim je místní a po restartu se vypne.</b></p></section>";
    h += "<section><h3>Meze ochrany čerpadla</h3>";
    h += "<form method='post' action='/local-limits'><label>Využitelná hloubka nádrže (cm)</label><input name='depth' type='number' min='20' max='2000' step='0.1' required value='";
    if (upperProtectionConfigValid) h += String(upperTankLimits.usableDepthCm, 1);
    h += "'><label>Kritická hladina čerpadla (cm)</label><input name='trip' type='number' min='0' max='2000' step='0.1' required value='";
    if (upperProtectionConfigValid) h += String(upperTankLimits.pumpCriticalTripCm, 1);
    h += "'><label>Hladina obnovení čerpadla (cm)</label><input name='recovery' type='number' min='0' max='2000' step='0.1' required value='";
    if (upperProtectionConfigValid) h += String(upperTankLimits.pumpRecoveryCm, 1);
    h += "'><button type='submit'>Uložit meze čerpadla</button></form>";
    if (upperSensorConfigState != HWC::SENSOR_CONFIG_VALID_STORED) {
        h += "<h3>Servisní obnova elektrického nastavení horního snímače</h3><p class='warn'>Použijte pouze po nezávislém fyzickém změření skutečné hladiny. Měřítko musí být určeno z ověřené kalibrace; posun se vypočítá z aktuálního surového měření a referenční hladiny.</p><form method='post' action='/sensor-recovery'><label>Ověřené měřítko (0,5–1,5)</label><input name='scale' type='number' min='0.5' max='1.5' step='0.001' required><label>Skutečná hladina změřená nezávislou metodou (cm)</label><input name='reference' type='number' min='0' max='2000' step='0.1' required><label><input style='width:auto' type='checkbox' name='physical' value='yes' required> Potvrzuji fyzické změření hladiny nezávislou referencí a správné přiřazení horního snímače.</label><button type='submit'>Obnovit elektrické nastavení horního snímače</button></form>";
    }
    h += "</section>";
    h += "<section><h3>Nouzový režim horního čerpadla</h3><p>Nouzové povolení se po restartu neobnovuje. Před potvrzením fyzicky ověřte, že je v nádrži dostatek vody pro bezpečný provoz; během provozu hladinu průběžně kontrolujte.</p>";
    if (!upperPumpProtectionState.emergencyModeActive) h += "<form method='post' action='/pump-emergency'><input type='hidden' name='mode' value='enable'><label><input style='width:auto' type='checkbox' name='physical' value='yes' required> Potvrzuji, že jsem fyzicky zkontroloval hladinu horní nádrže a je bezpečná pro provoz čerpadla. Přebírám průběžnou kontrolu hladiny.</label><button type='submit'>NOUZOVĚ POVOLIT HORNÍ ČERPADLO</button></form>";
    else h += "<form method='post' action='/pump-emergency'><input type='hidden' name='mode' value='disable'><button type='submit'>Ukončit nouzový režim</button></form>";
    h += "</section>";
    h += "<section><h3>Síťové připojení W5500</h3><p>Stav Ethernetu: ";
    const EthernetStartup startup = ethernetStartup.load();
    if (startup == EthernetStartup::Ready && ETH.linkUp() && ETH.hasIP()) {
        h += "DHCP adresa přidělena</p><p>IP adresa: <b>" + ETH.localIP().toString() + "</b></p>";
    } else {
        if (startup == EthernetStartup::Pending) h += "Probíhá inicializace W5500";
        else if (startup == EthernetStartup::Failed) h += "Inicializace W5500 selhala";
        else h += ETH.linkUp() ? "Čeká se na DHCP" : "Fyzický link je odpojen";
        h += "</p>";
    }
    h += "<p>Ovládací web je dostupný přes Ethernet i Wi-Fi. REV104 Core 3 – stolní zkouška.</p></section>";
    h += serviceApActive ? "<p>Servisní AP je zapnutý.</p>" : "<p>Servisní AP je vypnutý.</p>";
    h += "<section><h3>Domácí Wi-Fi</h3><p>";
    if (wifiStaDisabled) {
        h += (WiFi.getMode() & WIFI_MODE_STA) ? "Vypíná se." : "Vypnuto.";
    } else if (WiFi.STA.connected() && WiFi.STA.hasIP()) {
        h += "Připojeno, IP adresa: " + WiFi.localIP().toString();
    } else {
        h += savedWifiSsid.length() ? "Připojení se obnovuje." : "Připojení není nastavené.";
    }
    h += "</p><form method='post' action='/wifi-mode'>";
    if (wifiStaDisabled) {
        h += "<button name='mode' value='on' type='submit'>Zapnout domácí Wi-Fi</button>";
    } else {
        h += "<button name='mode' value='off' type='submit'>Vypnout domácí Wi-Fi</button>";
    }
    h += "</form><form method='post' action='/wifi'><label>Název domácí Wi-Fi (SSID)</label>"
         "<input name='ssid' maxlength='32' required><label>Heslo Wi-Fi</label>"
         "<input name='password' type='password' maxlength='64'>"
         "<button type='submit'>Uložit nastavení Wi-Fi</button></form></section>";
    h += "<section><h3>Aktualizace firmwaru ESP32 #2</h3><p class='warn'>Nahrávejte pouze ověřený firmware určený pro ESP32 #2. Během aktualizace neodpojujte napájení.</p><form method='POST' action='/update' enctype='multipart/form-data'><label>Soubor firmwaru .bin</label><input type='file' name='firmware' accept='.bin,application/octet-stream' required><button type='submit'>Nahrát firmware do ESP32 #2</button></form></section>";
    h += "<section><p>AP pro místní správu: <b>" + localApName + "</b>, adresa <b>192.168.4.1</b>. Bezpečnostní relé horního čerpadla je řízeno místní ochranou; ESP32 #2 neovládá ventily.</p></section></body></html>";
    localWebServer.send(200, "text/html; charset=utf-8", h);
}

void handleLocalSensorRecovery() {
    if (!requireLocalAuth()) return;
    if (!localWebServer.hasArg("scale") || !localWebServer.hasArg("reference") ||
        !localWebServer.hasArg("physical") || localWebServer.arg("physical") != "yes") {
        localWebServer.send(400, "text/plain; charset=utf-8", "Obnova vyžaduje měřítko, skutečnou referenční hladinu a fyzické potvrzení."); return;
    }
    if (upperSensorConfigState == HWC::SENSOR_CONFIG_VALID_STORED) {
        localWebServer.send(409, "text/plain; charset=utf-8", "Záznam je již platný; servisní obnova jej nesmí přepsat."); return;
    }
    if (upperSensorConfigState == HWC::SENSOR_CONFIG_NVS_UNAVAILABLE || !configStore.begin()) {
        localWebServer.send(503, "text/plain; charset=utf-8", "NVS není dostupné. Obnova nemůže pokračovat."); return;
    }
    float scale = 0.0f, reference = 0.0f;
    if (!parseFloatStrictLocal(localWebServer.arg("scale"), scale) ||
        !parseFloatStrictLocal(localWebServer.arg("reference"), reference)) {
        localWebServer.send(400, "text/plain; charset=utf-8", "Měřítko a referenční hladina musí být platná čísla."); return;
    }
    if (upperTankSensor == nullptr) {
        localWebServer.send(503, "text/plain; charset=utf-8", "Snímač není inicializován."); return;
    }
    const HWC::LevelSensorReading live = upperTankSensor->read();
    if (!live.adcReadOk || live.result.status != HWC::SENSOR_MATH_OK) {
        localWebServer.send(409, "text/plain; charset=utf-8", "Aktuální elektrický signál není platný. Obnovu nelze provést."); return;
    }
    HWC::SensorConfig candidate = HWC::defaultSensorConfig();
    if (!HWC::buildSensorConfigRecoveryCandidate(scale, live.result.rawLevelCm, reference, candidate)) {
        localWebServer.send(400, "text/plain; charset=utf-8", "Zadané hodnoty vedou k neplatnému elektrickému nastavení."); return;
    }
    if (!configStore.recover("upper", candidate)) {
        localWebServer.send(500, "text/plain; charset=utf-8", "Obnova selhala. Záznam nebyl bezpečně uložen; ochrana zůstává blokována."); return;
    }
    HWC::SensorConfig verify = {};
    if (!configStore.load("upper", verify) || !HWC::sensorConfigChecksumMatches(verify)) {
        localWebServer.send(500, "text/plain; charset=utf-8", "Kontrolní načtení po obnově selhalo. Ochrana zůstává blokována."); return;
    }
    upperTankConfig = verify;
    upperTankSensor->setConfig(verify);
    // The previous cached reading may have been converted with provisional defaults.
    // Keep the local pump relay fail-off and do not advertise a trusted peer level
    // until the next scheduled acquisition uses the newly verified configuration.
    haveUpperReading = false;
    upperSensorConfigState = HWC::SENSOR_CONFIG_VALID_STORED;
    Serial.printf("SENSOR_RECOVERY upper: reference=%.1f cm raw=%.1f cm scale=%.3f offset=%.1f cm; NVS read-back PASS\n",
                  reference, live.result.rawLevelCm, verify.calibrationScale, verify.calibrationOffsetCm);
    sendLocalResult("Elektrické nastavení horního snímače bylo obnoveno, zapsáno a ověřeno. Zkontrolujte zobrazenou výšku proti fyzické referenci.");
}

void handleLocalLimits() {
    if (!requireLocalAuth()) return;
    const char* names[] = {"depth", "trip", "recovery"};
    float values[3] = {};
    for (size_t i = 0; i < 3; ++i) {
        if (!localWebServer.hasArg(names[i]) || !parseFloatStrictLocal(localWebServer.arg(names[i]), values[i])) {
            localWebServer.send(400, "text/plain; charset=utf-8", "Všechny limity musí být platná čísla."); return;
        }
    }
    const float depth = values[0];
    const float trip = values[1];
    const float recovery = values[2];
    if (depth < 20.0f || depth > 2000.0f || trip < 0.0f || recovery <= trip || recovery > depth) {
        localWebServer.send(400, "text/plain; charset=utf-8",
                            "Kritická hladina musí být pod obnovovací hladinou a obnovovací hladina nejvýše ve využitelné hloubce.");
        return;
    }
    // Keep the existing NVS record format for rollback. The old transfer fields
    // are unused by the local pump; retain them if they still fit the record.
    HWC::UpperTankLimits candidate = upperTankLimits;
    candidate.usableDepthCm = depth;
    candidate.pumpCriticalTripCm = trip;
    candidate.pumpRecoveryCm = recovery;
    if (!(candidate.householdReserveCm > trip &&
          candidate.transferValveOpenCm > candidate.householdReserveCm &&
          candidate.transferValveOpenCm <= depth)) {
        candidate.householdReserveCm = trip + (depth - trip) / 2.0f;
        candidate.transferValveOpenCm = depth;
    }
    HWC::updateUpperTankLimitsChecksum(candidate);
    if (!HWC::isUpperTankLimitsValid(candidate)) {
        localWebServer.send(400, "text/plain; charset=utf-8", "Zkontrolujte rozestup hladinových mezí.");
        return;
    }
    if (!upperProtectionStorageReady || !upperLimitsStore.begin()) {
        localWebServer.send(503, "text/plain; charset=utf-8", "NVS není dostupné; limity nebyly uloženy."); return;
    }
    if (upperLimitsStore.hasRecord()) {
        HWC::UpperTankLimits old = HWC::emptyUpperTankLimits();
        if (!upperLimitsStore.load(old)) {
            localWebServer.send(409, "text/plain; charset=utf-8", "Stávající záznam je poškozený; automatické přepsání je zakázáno."); return;
        }
    }
    if (!upperLimitsStore.save(candidate)) {
        localWebServer.send(500, "text/plain; charset=utf-8", "Uložení nebo kontrolní načtení selhalo."); return;
    }
    upperTankLimits = candidate; upperProtectionConfigValid = true;
    // Require fresh recovery validation after any limit change.
    upperPumpProtection = HWC::UpperTankProtection(upperSafetyState.criticalLatch, upperSafetyStorageReady && upperSafetyState.stateValid);
    upperPumpProtectionState = {false, upperSafetyState.criticalLatch, true, true, !upperSafetyStorageReady, false};
    sendLocalResult("Limity byly uloženy a znovu načteny. Po změně limitů je povolení čerpadla blokováno do dosažení obnovovací hladiny.");
}

void handleLocalUpdateUpload() {
    HTTPUpload& upload = localWebServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        localOtaAuthorized = requireLocalAuth();
        localOtaStarted = false;
        localOtaSucceeded = false;
        localOtaError = "";
        if (!localOtaAuthorized) return;
        if (!hasBinExtension(upload.filename)) {
            localOtaError = "Odmítnuto: vyberte firmware soubor s příponou .bin.";
            Serial.println("OTA #2 rejected: filename is not .bin");
            return;
        }
        if (upperPumpProtectionState.emergencyModeActive) {
            localOtaError = "Aktualizace je blokována během nouzového režimu horního čerpadla.";
            Serial.println("OTA #2 rejected: pump emergency mode active");
            return;
        }
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            localOtaError = "Nelze zahájit OTA zápis. Ověřte OTA partition a volnou velikost firmwaru.";
            Update.printError(Serial);
            return;
        }
        localOtaStarted = true;
        Serial.printf("OTA #2 started: %s\n", upload.filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (!localOtaAuthorized || !localOtaStarted) return;
        if (upperPumpProtectionState.emergencyModeActive) {
            localOtaError = "Aktualizace byla přerušena, protože byl aktivován nouzový režim horního čerpadla.";
            Update.abort();
            localOtaStarted = false;
            return;
        }
        const size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
            localOtaError = "Zápis firmwaru byl neúplný.";
            Update.printError(Serial);
            Update.abort();
            localOtaStarted = false;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!localOtaAuthorized || !localOtaStarted) return;
        // Recheck immediately before selecting the newly written image.
        // The emergency override may have become active between upload chunks.
        if (upperPumpProtectionState.emergencyModeActive) {
            localOtaError = "Aktualizace byla přerušena, protože je aktivní nouzový režim horního čerpadla.";
            Update.abort();
            localOtaStarted = false;
            Serial.println("OTA #2 aborted before finalization: pump emergency mode active");
            return;
        }
        if (Update.end(true)) {
            localOtaSucceeded = true;
            Serial.printf("OTA #2 finished: %u bytes\n", static_cast<unsigned>(upload.totalSize));
        } else {
            localOtaError = "Firmware nebyl dokončen nebo ověřen.";
            Update.printError(Serial);
            Update.abort();
        }
        localOtaStarted = false;
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (localOtaStarted) Update.abort();
        localOtaStarted = false;
        localOtaSucceeded = false;
        localOtaError = "Nahrávání firmwaru bylo přerušeno.";
    }
}

void handleLocalUpdateComplete() {
    if (!requireLocalAuth()) return;
    if (localOtaSucceeded) {
        sendLocalResult("Aktualizace ESP32 #2 byla dokončena. Zařízení se restartuje. "
                        "Za několik sekund klikněte na Zpět na přehled.");
        delay(1000);
        ESP.restart();
        return;
    }
    if (localOtaError.length() == 0) localOtaError = "Aktualizace se nezdařila. Firmware nebyl aktivován.";
    sendLocalResult(localOtaError);
}

void handleLocalPumpEmergency() {
    if (!requireLocalAuth()) return;
    const String mode = localWebServer.arg("mode");
    if (mode == "disable") {
        upperEmergencyDisableRequested = true;
        upperEmergencyEnableRequested = false;
        upperEmergencyConfirmationRequested = false;
        sendLocalResult("Požadavek na ukončení nouzového režimu byl přijat."); return;
    }
    if (!(mode == "enable") || !localWebServer.hasArg("physical") || !(localWebServer.arg("physical") == "yes")) {
        localWebServer.send(400, "text/plain; charset=utf-8", "Nouzové povolení vyžaduje výslovné potvrzení fyzické kontroly."); return;
    }
    upperEmergencyEnableRequested = true;
    upperEmergencyConfirmationRequested = true;
    upperEmergencyDisableRequested = false;
    sendLocalResult("Požadavek na nouzové povolení horního čerpadla byl přijat. Aktivace proběhne v nejbližším cyklu ochrany; ověřte stav AKTIVNÍ a povel relé v přehledu. Po fyzické kontrole hladiny ji obsluha musí průběžně sledovat.");
}

bool copyCStringToCredentialField(const char* source, char* target, size_t capacity) {
    if (source == nullptr || target == nullptr || capacity == 0U) return false;
    size_t length = 0U;
    while (length < capacity && source[length] != '\0') ++length;
    if (length >= capacity) return false;
    memset(target, 0, capacity);
    if (length != 0U) memcpy(target, source, length);
    target[length] = '\0';
    return true;
}

void loadLocalWifi() {
    Preferences settings;
    if (settings.begin("hwc_wifi", true)) {
        wifiStaDisabled = settings.getBool("sta_off", false);
        settings.end();
    }
    HWC::NvsCredentialBackend backend("hwc_wifi");
    if (!backend.begin()) return;
    HWC::WifiStaCredentialStore store(backend);
    HWC::WifiStaCredentials stored = {};
    if (store.load(stored) == HWC::WIFI_STA_CREDENTIAL_VALID) {
        savedWifiSsid = stored.ssid;
        savedWifiPassword = stored.password;
        wifiReconnectRequested = true;
    }
    HWC::WifiStaCredentialStore::clear(stored);
    backend.end();
}

void handleLocalWifi() {
    if (!requireLocalAuth()) return;
    String ssid = localWebServer.arg("ssid");
    const String password = localWebServer.arg("password");
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 64 ||
        (password.length() != 0 && password.length() < 8)) {
        localWebServer.send(400, "text/plain; charset=utf-8", "Neplatný název nebo heslo Wi-Fi.");
        return;
    }

    HWC::NvsCredentialBackend backend("hwc_wifi");
    if (!backend.begin()) {
        localWebServer.send(503, "text/plain; charset=utf-8", "Úložiště Wi-Fi není dostupné.");
        return;
    }
    HWC::WifiStaCredentialStore store(backend);
    HWC::WifiStaCredentials candidate = {};
    ssid.toCharArray(candidate.ssid, sizeof(candidate.ssid));
    password.toCharArray(candidate.password, sizeof(candidate.password));
    const bool saved = store.save(candidate);
    HWC::WifiStaCredentialStore::clear(candidate);
    backend.end();
    if (!saved) {
        localWebServer.send(500, "text/plain; charset=utf-8", "Uložení Wi-Fi údajů nebylo potvrzené.");
        return;
    }
    savedWifiSsid = ssid;
    savedWifiPassword = password;
    wifiReconnectRequested = true;
    sendLocalResult(wifiStaDisabled
        ? "Wi-Fi údaje byly uloženy. Domácí Wi-Fi je vypnutá. Zapneš ji na přehledu."
        : "Wi-Fi údaje byly uloženy. Jednotka zkusí nové připojení. Přes kabel můžeš výsledek zkontrolovat na přehledu.");
}

void handleLocalWifiMode() {
    if (!requireLocalAuth()) return;
    const String mode = localWebServer.arg("mode");
    if (mode != "on" && mode != "off") {
        localWebServer.send(400, "text/plain; charset=utf-8", "Neplatný režim domácí Wi-Fi.");
        return;
    }

    const bool disable = mode == "off";
    Preferences settings;
    if (!settings.begin("hwc_wifi", false)) {
        localWebServer.send(503, "text/plain; charset=utf-8", "Úložiště Wi-Fi není dostupné.");
        return;
    }
    settings.putBool("sta_off", disable);
    const bool saved = settings.isKey("sta_off") && settings.getBool("sta_off", !disable) == disable;
    settings.end();
    if (!saved) {
        localWebServer.send(500, "text/plain; charset=utf-8", "Uložení režimu Wi-Fi nebylo potvrzené.");
        return;
    }
    wifiStaDisabled = disable;
    wifiAttemptPending = false;
    wifiReconnectRequested = true;
    sendLocalResult(disable
        ? "Domácí Wi-Fi se vypne. Údaje sítě zůstávají uložené."
        : "Domácí Wi-Fi je povolená. Pokud jsou uložené údaje, zařízení se zkusí připojit.");
}

void startLocalWeb() {
    HWC::NvsCredentialBackend initialBackend("hwcCred");
    if (!initialBackend.begin()) {
        localApName = ""; localApPassword = ""; localAdminPassword = "";
        Serial.println("Unified credential NVS unavailable; local AP/web UI disabled.");
        return;
    }
    HWC::CredentialStore initialStore(initialBackend);
    HWC::Credentials active;
    HWC::CredentialStore::clear(active);
    const HWC::CredentialStoreStatus initialStatus = initialStore.load(active);
    initialBackend.end();

    bool credentialsReady = false;
    bool migrated = false;
    bool apPasswordGenerated = false;
    bool adminPasswordGenerated = false;

    if (initialStatus == HWC::CREDENTIAL_VALID) {
        localApName = active.apName;
        localApPassword = active.apPassword;
        localAdminPassword = active.adminPassword;
        credentialsReady = true;
    } else if (initialStatus == HWC::CREDENTIAL_EMPTY) {
        if (!uiPreferences.begin("hwc_u2ui", false)) {
            Serial.println("Legacy local UI NVS unavailable; local AP/web UI disabled.");
            HWC::CredentialStore::clear(active);
            return;
        }

        const bool apPasswordExisted = uiPreferences.isKey("ap_pass");
        const bool adminPasswordExisted = uiPreferences.isKey("admin");
        const uint64_t mac = ESP.getEfuseMac();
        char generatedApName[24] = {0};
        char generatedApPassword[33] = {0};
        char generatedAdminPassword[33] = {0};
        char verifiedApName[33] = {0};
        char verifiedApPassword[64] = {0};
        char verifiedAdminPassword[65] = {0};
        snprintf(generatedApName, sizeof(generatedApName), "HWC-UPPER-%04X", static_cast<unsigned>(mac & 0xFFFFU));
        if ((!apPasswordExisted || !adminPasswordExisted) && !enableCredentialRandomSource()) {
            uiPreferences.end();
            HWC::CredentialStore::clear(active);
            return;
        }
        if ((!apPasswordExisted &&
             !HWC::generateHexCredential(generatedApPassword, sizeof(generatedApPassword), credentialRandomWordSource, 4U)) ||
            (!adminPasswordExisted &&
             !HWC::generateHexCredential(generatedAdminPassword, sizeof(generatedAdminPassword), credentialRandomWordSource, 4U))) {
            uiPreferences.end();
            HWC::CredentialStore::clear(active);
            return;
        }

        HWC::NvsCredentialTextStorage credentialStorage(uiPreferences);
        const HWC::CredentialStartupResult apNameResult = HWC::ensureCredentialText(
            credentialStorage, "ap_name", generatedApName, sizeof(generatedApName),
            8U, 32U, verifiedApName, sizeof(verifiedApName));
        const HWC::CredentialStartupResult apPasswordResult = HWC::ensureCredentialText(
            credentialStorage, "ap_pass", generatedApPassword, sizeof(generatedApPassword),
            8U, 63U, verifiedApPassword, sizeof(verifiedApPassword));
        const HWC::CredentialStartupResult adminPasswordResult = HWC::ensureCredentialText(
            credentialStorage, "admin", generatedAdminPassword, sizeof(generatedAdminPassword),
            12U, 64U, verifiedAdminPassword, sizeof(verifiedAdminPassword));
        credentialStorage.close();
        uiPreferences.end();

        HWC::Credentials legacy;
        HWC::CredentialStore::clear(legacy);
        const bool legacyReady = apNameResult == HWC::CREDENTIAL_STARTUP_READY &&
                                 apPasswordResult == HWC::CREDENTIAL_STARTUP_READY &&
                                 adminPasswordResult == HWC::CREDENTIAL_STARTUP_READY &&
                                 copyCStringToCredentialField(verifiedApName, legacy.apName, sizeof(legacy.apName)) &&
                                 copyCStringToCredentialField(verifiedApPassword, legacy.apPassword, sizeof(legacy.apPassword)) &&
                                 copyCStringToCredentialField(verifiedAdminPassword, legacy.adminPassword, sizeof(legacy.adminPassword)) &&
                                 HWC::CredentialStore::validate(legacy);
        apPasswordGenerated = !apPasswordExisted && apPasswordResult == HWC::CREDENTIAL_STARTUP_READY;
        adminPasswordGenerated = !adminPasswordExisted && adminPasswordResult == HWC::CREDENTIAL_STARTUP_READY;

        HWC::clearCredentialText(generatedApName, sizeof(generatedApName));
        HWC::clearCredentialText(generatedApPassword, sizeof(generatedApPassword));
        HWC::clearCredentialText(generatedAdminPassword, sizeof(generatedAdminPassword));
        HWC::clearCredentialText(verifiedApName, sizeof(verifiedApName));
        HWC::clearCredentialText(verifiedApPassword, sizeof(verifiedApPassword));
        HWC::clearCredentialText(verifiedAdminPassword, sizeof(verifiedAdminPassword));

        if (legacyReady) {
            HWC::NvsCredentialBackend migrationBackend("hwcCred");
            if (migrationBackend.begin()) {
                HWC::CredentialStore migrationStore(migrationBackend);
                const HWC::CredentialLegacyMigrationResult migration =
                    HWC::CredentialLegacyMigrator::loadOrMigrate(migrationStore, legacy, active);
                migrated = migration == HWC::CREDENTIAL_MIGRATION_DONE;
                credentialsReady = (migrated || migration == HWC::CREDENTIAL_MIGRATION_ALREADY_PRESENT) &&
                                   HWC::CredentialStore::validate(active);
                migrationBackend.end();
            }
        }
        HWC::CredentialStore::clear(legacy);
        if (credentialsReady && migrated) {
            if (apPasswordGenerated)
                Serial.printf("Initial upper-tank AP password (one-time display): %s\n", active.apPassword);
            if (adminPasswordGenerated)
                Serial.printf("Initial upper-tank web admin password (one-time display): %s\n", active.adminPassword);
        }
        if (credentialsReady) {
            localApName = active.apName;
            localApPassword = active.apPassword;
            localAdminPassword = active.adminPassword;
        }
    } else {
        Serial.printf("Unified credential store is not usable (state=%u); legacy fallback blocked.\n",
                      static_cast<unsigned>(initialStatus));
    }

    HWC::CredentialStore::clear(active);
    if (!credentialsReady) {
        localApName = ""; localApPassword = ""; localAdminPassword = "";
        Serial.println("Local AP/web UI disabled; unified credentials are not valid.");
        return;
    }

    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    static const char* requestHeaders[] = {"Origin", "Host"};
    localWebServer.collectHeaders(requestHeaders, 2);
    localWebServer.on("/", HTTP_GET, handleLocalRoot);
    localWebServer.on("/status", HTTP_GET, handleLocalRoot);
    localWebServer.on("/wifi", HTTP_POST, handleLocalWifi);
    localWebServer.on("/wifi-mode", HTTP_POST, handleLocalWifiMode);
    localWebServer.on("/local-limits", HTTP_POST, handleLocalLimits);
    localWebServer.on("/sensor-recovery", HTTP_POST, handleLocalSensorRecovery);
    localWebServer.on("/pump-emergency", HTTP_POST, handleLocalPumpEmergency);
    localWebServer.on("/update", HTTP_POST, handleLocalUpdateComplete, handleLocalUpdateUpload);
    localWebServer.onNotFound([]() { if (!requireLocalAuth()) return; localWebServer.send(404, "text/plain", "Not found"); });
    localWebServer.begin();
    lastManagementLinkMs = millis();
    lastServiceApAttemptMs = lastManagementLinkMs;
    localWebReady = true;
}

void maintainLocalWifi(uint32_t now) {
    if (!localWebReady) return;
    if (wifiStaDisabled) {
        wifiAttemptPending = false;
        if ((WiFi.getMode() & WIFI_MODE_STA) &&
            (wifiReconnectRequested || now - lastWifiAttemptMs >= WIFI_RETRY_INTERVAL_MS)) {
            lastWifiAttemptMs = now;
            if (!WiFi.enableSTA(false)) Serial.println("Upper tank Wi-Fi STA could not be disabled; will retry.");
        }
        wifiReconnectRequested = false;
        return;
    }
    if (savedWifiSsid.length() == 0) return;

    if (wifiReconnectRequested || (!wifiAttemptPending &&
        !(WiFi.STA.connected() && WiFi.STA.hasIP()) &&
        now - lastWifiAttemptMs >= WIFI_RETRY_INTERVAL_MS)) {
        wifiReconnectRequested = false;
        lastWifiAttemptMs = now;
        wifiAttemptPending = WiFi.enableSTA(true);
        if (wifiAttemptPending) {
            WiFi.disconnectAsync(false, false);
        } else {
            Serial.println("Upper tank Wi-Fi STA initialization failed; will retry.");
        }
        return;
    }

    if (!wifiAttemptPending || now - lastWifiAttemptMs < 250U) return;
    if (!WiFi.STA.started() || WiFi.STA.connected()) {
        if (now - lastWifiAttemptMs >= 2000U) {
            wifiAttemptPending = false;
            wifiReconnectRequested = true;
            Serial.println("Upper tank Wi-Fi STA not ready for new credentials; will retry.");
        }
        return;
    }

    wifiAttemptPending = false;
    lastWifiAttemptMs = now;
    if (!WiFi.STA.connect(savedWifiSsid.c_str(), savedWifiPassword.c_str())) {
        Serial.println("Upper tank Wi-Fi connection could not start; will retry.");
    }
}

void maintainServiceAp(uint32_t now) {
    if (!localWebReady) return;
    const bool wifiReady = WiFi.STA.connected() && WiFi.STA.hasIP();
    const bool ethernetReady = ethernetStartup.load() == EthernetStartup::Ready && ETH.linkUp() && ETH.hasIP();
    if (wifiReady || ethernetReady) {
        lastManagementLinkMs = now;
        if (serviceApActive && WiFi.enableAP(false)) {
            serviceApActive = false;
            Serial.println("Upper tank service AP stopped: management link restored.");
        }
        return;
    }

    if (serviceApActive || now - lastManagementLinkMs < MANAGEMENT_LINK_GRACE_MS ||
        now - lastServiceApAttemptMs < 5000U) return;
    lastServiceApAttemptMs = now;
    serviceApActive = WiFi.softAP(localApName.c_str(), localApPassword.c_str());
    if (serviceApActive) {
        Serial.printf("Upper tank local AP started: SSID=%s IP=%s\n",
                      localApName.c_str(), WiFi.softAPIP().toString().c_str());
    } else {
        WiFi.enableAP(false);
        Serial.println("Upper tank service AP could not start; will retry.");
    }
}

void initializeEthernet(void*) {
    // The reset and driver startup may wait for hardware. Keep them off the control loop.
    delay(500);
    for (unsigned attempt = 1; attempt <= 3; ++attempt) {
        Serial.printf("W5500 initialization attempt %u/3.\n", attempt);
        pinMode(HwcPins::Esp32_2::W5500_RST, OUTPUT);
        digitalWrite(HwcPins::Esp32_2::W5500_RST, LOW);
        delay(50);
        digitalWrite(HwcPins::Esp32_2::W5500_RST, HIGH);
        delay(200);

        // Firmware controls reset above. INT is unwired; core 3.3.7 polls RX.
        if (ETH.begin(ETH_PHY_W5500, 1, HwcPins::Esp32_2::W5500_CS, -1, -1,
                      SPI3_HOST, HwcPins::Esp32_2::W5500_SCK, HwcPins::Esp32_2::W5500_MISO,
                      HwcPins::Esp32_2::W5500_MOSI, 10) && ETH.netif() != nullptr) {
            ethernetStartup.store(EthernetStartup::Ready);
            Serial.println("W5500 driver initialized.");
            vTaskDelete(nullptr);
            return;
        }

        Serial.println("W5500 driver initialization failed.");
        if (attempt < 3) {
            ETH.end();
            if (ETH.netif() != nullptr) break;
            delay(2500);
        }
    }
    ethernetStartup.store(EthernetStartup::Failed);
    vTaskDelete(nullptr);
}

void startEthernet() {
    if (xTaskCreatePinnedToCore(initializeEthernet, "hwc-eth-init", 5120, nullptr, 1, nullptr, 0) != pdPASS) {
        ethernetStartup.store(EthernetStartup::Failed);
        Serial.println("W5500 initialization task could not start.");
    }
}

void reportEthernetAddress() {
    if (ethernetStartup.load() != EthernetStartup::Ready) return;
    const IPAddress ip = ETH.linkUp() && ETH.hasIP() ? ETH.localIP() : IPAddress();
    if (ip == reportedEthernetIp) return;
    reportedEthernetIp = ip;
    if (ip != IPAddress()) {
        Serial.printf("Ethernet management: http://%s/\n", ip.toString().c_str());
    }
}

void printReading(const char* name, const HWC::LevelSensorReading& r) {
    Serial.printf("%s: ADC=%s, mV=%u, status=%u", name,
                  r.adcReadOk ? "OK" : "ERROR", r.millivolts,
                  static_cast<unsigned>(r.result.status));
    if (r.result.status == HWC::SENSOR_MATH_OK) {
        Serial.printf(", current=%.3f mA, raw=%.1f cm, calibrated=%.1f cm",
                      r.result.currentMilliAmps, r.result.rawLevelCm,
                      r.result.calibratedLevelCm);
    } else if (r.adcReadOk) {
        Serial.printf(", current=%.3f mA", r.result.currentMilliAmps);
    }
    Serial.println();
}
}

void setup() {
    Serial.begin(115200);
    // Active-HIGH pump permit: LOW is the de-energized safety state.
    digitalWrite(HwcPins::Esp32_2::UPPER_TANK_PUMP_SAFETY_RELAY, LOW);
    pinMode(HwcPins::Esp32_2::UPPER_TANK_PUMP_SAFETY_RELAY, OUTPUT);
    digitalWrite(HwcPins::Esp32_2::UPPER_TANK_PUMP_SAFETY_RELAY, LOW);
    upperPumpSafetyRelayReady = upperPumpSafetyRelayOutput.begin(
        HwcPins::Esp32_2::UPPER_TANK_PUMP_SAFETY_RELAY, HwcPins::Esp32_2::PUMP_RELAY_ACTIVE_HIGH,
        HwcPins::Esp32_2::PUMP_SAFETY_OUTPUTS_ENABLED, writePumpSafetyRelayLevel);
    delay(250);
    Serial.println();
    Serial.println("HOME WATER CONTROLLER - ESP32 #2 / REV104 CORE 3 BENCH");
    Serial.println("SENSOR ACQUISITION STAGE - UPPER PUMP SAFETY RELAY ENABLED");
    Serial.printf("W5500 pin plan: SCK=%u MISO=%u MOSI=%u CS=%u RST=%u\n",
                  HwcPins::Esp32_2::W5500_SCK, HwcPins::Esp32_2::W5500_MISO,
                  HwcPins::Esp32_2::W5500_MOSI, HwcPins::Esp32_2::W5500_CS,
                  HwcPins::Esp32_2::W5500_RST);
    Serial.printf("ADC pin plan: upper tank=%u; DS18B20 data GPIO=%u (informational measurement)\n",
                  HwcPins::Esp32_2::UPPER_TANK_LEVEL_ADC,
                  HwcPins::Esp32_2::WATER_TEMPERATURE_DATA);

    analogReadResolution(12);
    if (!configStore.begin()) {
        upperTankConfig = HWC::defaultSensorConfig();
        upperSensorConfigState = HWC::SENSOR_CONFIG_NVS_UNAVAILABLE;
        Serial.println("ERROR: NVS calibration namespace unavailable; provisional defaults are diagnostic only and upper-pump safety is blocked.");
    } else if (!configStore.hasRecord("upper")) {
        upperTankConfig = HWC::defaultSensorConfig();
        upperSensorConfigState = HWC::SENSOR_CONFIG_MISSING_PROVISIONAL;
        Serial.println("NVS sensor config 'upper': missing; provisional defaults are diagnostic only; upper-pump safety is blocked");
    } else if (configStore.load("upper", upperTankConfig)) {
        upperSensorConfigState = HWC::SENSOR_CONFIG_VALID_STORED;
        Serial.println("NVS calibration 'upper': stored record loaded and checksum valid");
    } else {
        upperTankConfig = HWC::defaultSensorConfig();
        upperSensorConfigState = HWC::SENSOR_CONFIG_INVALID_PRESENT;
        Serial.println("ERROR: NVS calibration 'upper' exists but is invalid; defaults are diagnostic only and upper-pump safety is blocked.");
    }

    static HWC::LevelSensorReader upperReader(HwcPins::Esp32_2::UPPER_TANK_LEVEL_ADC, HWC::defaultSensorConfig());
    upperReader.setConfig(upperTankConfig);
    upperReader.begin();
    upperTankSensor = &upperReader;
    Serial.println("NOTE: upper tank geometry and sensor installation calibration are not yet configured.");

    // Independent local upper-tank pump protection; its physical safety relay is driven on GPIO25.
    // Limits must be configured and stored locally on ESP32 #2; they are not copied
    // from ESP32 #1 over unauthenticated UDP. Missing/corrupt storage fails closed.
    upperProtectionStorageReady = upperLimitsStore.begin();
    if (upperProtectionStorageReady && upperLimitsStore.load(upperTankLimits)) {
        upperProtectionConfigValid = HWC::isUpperTankLimitsValid(upperTankLimits);
        Serial.println(upperProtectionConfigValid
            ? "Upper tank limits loaded locally; startup validation still required."
            : "Upper tank limits invalid; pump permit stays blocked.");
    } else {
        upperProtectionConfigValid = false;
        Serial.println("Upper tank limits are UNCONFIGURED locally; pump permit stays blocked.");
    }
    if (!upperProtectionStorageReady) Serial.println("Upper tank limits NVS unavailable; pump permit stays blocked.");

    // Persist the critical-low latch separately from the adjustable limits. A missing
    // state record is not treated as proof that no previous critical event occurred.
    const bool upperSafetyStoreOpened = upperSafetyStore.begin();
    if (upperSafetyStoreOpened) {
        upperSafetyRecordPresent = upperSafetyStore.hasRecord();
        if (upperSafetyRecordPresent && upperSafetyStore.load(upperSafetyState)) {
            upperSafetyStorageReady = upperSafetyState.stateValid;
            upperPumpProtection = HWC::UpperTankProtection(upperSafetyState.criticalLatch, upperSafetyState.stateValid);
            Serial.println("Upper pump safety latch loaded from local NVS.");
        } else if (upperSafetyRecordPresent) {
            upperSafetyStorageReady = false;
            Serial.println("ERROR: upper pump safety latch record is corrupt; emergency and pump permit remain blocked.");
        } else {
            upperSafetyStorageReady = false;
            Serial.println("Upper pump safety latch record missing; it can be initialized only after a valid reading reaches recovery level.");
        }
    } else {
        upperSafetyStorageReady = false;
        Serial.println("ERROR: upper pump safety-state NVS unavailable; pump permit remains blocked.");
    }
    discoverWaterTemperatureSensor(millis());

    if (Network.begin()) {
        startEthernet();
        startLocalWeb();
        if (localWebReady) loadLocalWifi();
    } else {
        Serial.println("Network initialization failed; local sensing and protection remain active.");
    }
}

void loop() {
    if (localWebReady) localWebServer.handleClient();
    const uint32_t now = millis();
    maintainLocalWifi(now);
    maintainServiceAp(now);
    reportEthernetAddress();
    maintainWaterTemperature(now);
    if (static_cast<uint32_t>(now - lastReportMs) >= 2000U) {
        lastReportMs = now;
        const HWC::LevelSensorReading upper = upperTankSensor->read();
        lastUpperReading = upper;
        haveUpperReading = true;
        printReading("UPPER_TANK", upper);
        const bool upperSensorValid = HWC::sensorConfigUsableForLevelMeasurement(upperSensorConfigState) &&
            upper.result.status == HWC::SENSOR_MATH_OK;
        const float upperLevel = upperSensorValid ? upper.result.calibratedLevelCm : 0.0f;
        const bool latchBeforeUpdate = upperPumpProtection.criticalLevelLatched();
        upperPumpProtectionState = upperPumpProtection.update(
            upperTankLimits, upperLevel, upperSensorValid,
            upperProtectionStorageReady && upperProtectionConfigValid && upperSafetyStorageReady,
            upperEmergencyEnableRequested, upperEmergencyConfirmationRequested,
            upperEmergencyDisableRequested);

        // Safe first-install migration: do not create a fresh "clear" safety record
        // until a valid sensor has reached the configured recovery level. Corrupt
        // existing records are never silently replaced.
        if (upperSafetyStore.begin() && !upperSafetyRecordPresent &&
            upperProtectionConfigValid && upperSensorValid &&
            upperLevel >= upperTankLimits.pumpRecoveryCm &&
            !upperPumpProtection.criticalLevelLatched()) {
            if (upperSafetyStore.save(false, true)) {
                upperSafetyRecordPresent = true;
                upperSafetyState = {false, true};
                upperSafetyStorageReady = true;
                Serial.println("Upper pump safety state initialized after valid recovery-level reading.");
            }
        } else if (upperSafetyStore.begin() && upperSafetyRecordPresent && upperSafetyStorageReady &&
                   latchBeforeUpdate != upperPumpProtection.criticalLevelLatched()) {
            if (!upperSafetyStore.save(upperPumpProtection.criticalLevelLatched(), true)) {
                upperSafetyStorageReady = false;
                upperPumpProtectionState.pumpPermit = upperPumpProtectionState.emergencyModeActive;
                upperPumpProtectionState.systemFault = true;
                Serial.println("ERROR: failed to persist upper critical latch; pump permit disabled.");
            } else {
                upperSafetyState.criticalLatch = upperPumpProtection.criticalLevelLatched();
                upperSafetyState.stateValid = true;
            }
        }
        upperEmergencyEnableRequested = false;
        upperEmergencyConfirmationRequested = false;
        upperEmergencyDisableRequested = false;
        Serial.printf("UPPER_PUMP_PROTECTION: permit=%s, config=%s, sensor=%s, criticalLatch=%s, startupCheck=%s (relay output enabled, HIGH permits motor)\n",
                      upperPumpProtectionState.pumpPermit ? "YES" : "NO",
                      upperProtectionConfigValid ? "VALID" : "INVALID",
                      upperPumpProtectionState.sensorWarning ? "INVALID" : "OK",
                      upperPumpProtectionState.criticalLevelLatched ? "SET" : "clear",
                      upperPumpProtectionState.startupValidationRequired ? "REQUIRED" : "passed");

    }
    syncUpperPumpSafetyRelay(millis());
    delay(10);
}
