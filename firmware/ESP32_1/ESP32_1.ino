/*
 * HOME WATER CONTROLLER — ESP32 #1 (main unit)
 * Independent main unit: well/lower-tank sensing, local transfer control, Wi-Fi web configuration and W5500 DHCP networking.
 * Manual and automatic valve paths are relay-driven. Pump safety relays are enabled with the proven standalone-well configuration.
 */
#include "src/HardwarePins.h"
#include "src/WaterLevelSketch.h"
#include "src/HttpOriginPolicy.h"
#include "src/SampleFreshness.h"
#include "src/SensorMath.h"
#include "src/SensorConfig.h"
#include "src/NvsSensorConfigStore.h"
#include "src/LevelSensorReader.h"
#include "src/WaterMath.h"
#include "src/TransferLogic.h"
#include "src/ManualValveControl.h"
#include "src/EmergencyValvePumpGuard.h"
#include "src/ValveRelayOutput.h"
#include "src/PumpSafetyRelayOutput.h"
#include "src/TransferHistory.h"
#include "src/NvsTransferHistoryStore.h"
#include "src/TransferRestartGuard.h"
#include "src/NvsTransferJournalStore.h"
#include "src/WellPumpProtection.h"
#include "src/WellProtectionConfig.h"
#include "src/NvsWellProtectionStore.h"
#include "src/LowerTankPumpProtection.h"
#include "src/NvsLowerTankPumpProtectionStore.h"
#include "src/NvsLowerTankInflowWatchdogStore.h"
#include "src/WellGeometry.h"
#include "src/NvsWellGeometryStore.h"
#include "src/NvsWellCalibrationStore.h"
#include "src/NvsDailyInflowStore.h"
#include "src/InflowPersistencePolicy.h"
#include <ETH.h>
#include <atomic>
#include <Network.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>
#include "src/NvsCredentialTextStorage.h"
#include "src/CredentialStore.h"
#include "src/NvsCredentialBackend.h"
#include "src/WifiStaCredentialStore.h"
#include "src/CredentialLegacyMigrator.h"
#include "src/CredentialRandom.h"
#include "src/WebAuthRateLimiter.h"
#include <cmath>
#include <stdlib.h>
#include <string.h>
#include <Esp.h>
#include <esp_system.h>
#include <time.h>

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
HWC::NvsWellGeometryStore geometryStore;
HWC::NvsWellCalibrationStore wellCalibrationStore;
HWC::NvsDailyInflowStore inflowStore;
HWC::DailyInflowTracker inflowTracker;
HWC::NvsWellProtectionStore protectionStore;
HWC::NvsLowerTankPumpProtectionStore lowerPumpProtectionStore;
HWC::LowerTankPumpConfig lowerPumpConfig = HWC::emptyLowerTankPumpConfig();
HWC::LowerTankPumpProtection* lowerPumpProtection = nullptr;
HWC::LowerTankPumpState lowerPumpState = {false, false, true, true, true, false};
bool lowerPumpConfigConfigured = false;
bool lowerPumpStorageReady = false;
bool lowerPumpPersistedCriticalLatch = false;
bool lowerEmergencyEnableRequested = false;
bool lowerEmergencyConfirmationRequested = false;
bool lowerEmergencyDisableRequested = false;
HWC::NvsLowerTankInflowWatchdogStore inflowWatchdogStore;
HWC::LowerTankInflowWatchdogSettings inflowWatchdogSettings = HWC::defaultLowerTankInflowWatchdogSettings();
bool inflowWatchdogSettingsLoaded = false;
bool inflowWatchdogSettingsStorageFault = false;
// CRC-only UDP status is telemetry, not an authenticated control channel.
// Keep normal/service upper-source valve control disabled until the authenticated
// HELLO/CHALLENGE/FINISH/READY + PeerCommandSession path is integrated and tested.
HWC::WellProtectionConfig wellProtectionConfig = HWC::emptyWellProtectionConfig();
HWC::WellPumpProtection* wellPumpProtection = nullptr;
HWC::PumpProtectionOutputs pumpProtectionState = {false, false, false, true, true};
HWC::WellGeometryConfig wellGeometry;
bool wellGeometryLoaded = false; // True only when dimensions were loaded from NVS; RAM defaults remain usable estimates.
HWC::WellCalibration wellCalibration;
HWC::SensorConfig wellConfig;
HWC::SensorConfig lowerTankConfig;
HWC::SensorConfigLoadState wellSensorConfigState = HWC::SENSOR_CONFIG_NVS_UNAVAILABLE;
HWC::SensorConfigLoadState lowerSensorConfigState = HWC::SENSOR_CONFIG_NVS_UNAVAILABLE;
HWC::LevelSensorReader* wellSensor = nullptr;
HWC::LevelSensorReader* lowerTankSensor = nullptr;
HWC::LevelSensorReading lastWellReading = {};
HWC::LevelSensorReading lastLowerTankReading = {};
HWC::WellVolumeResult lastWellVolume = {HWC::WELL_VOLUME_INVALID_LEVEL, 0.0f, false};
bool haveWellReading = false;
bool haveLowerTankReading = false;
bool lastLowerTankVolumeValid = false;
float lastLowerTankVolumeLiters = 0.0f;
uint32_t lastReportMs = 0;
uint32_t lastWellReadingMs = 0;
uint32_t lastLowerTankReadingMs = 0;
WebServer webServer(80);
HWC::WebAuthRateLimiter webAuthRateLimiter;
HWC::TransferLogic transferLogic;
HWC::ManualValveControl manualValveControl; // RAM-only; reboot always returns to inactive/CLOSE_ALL.
HWC::ValveRelayOutput manualValveRelayOutput;
bool manualValveRelayOutputReady = false;
HWC::PumpSafetyRelayOutput wellPumpSafetyRelayOutput;
HWC::PumpSafetyRelayOutput lowerPumpSafetyRelayOutput;
bool wellPumpSafetyRelayReady = false;
bool lowerPumpSafetyRelayReady = false;
HWC::TransferHistory transferHistory;
HWC::NvsTransferHistoryStore transferHistoryStore;
HWC::NvsTransferJournalStore transferJournalStore;
HWC::TransferRestartGuard transferRestartGuard;
bool transferJournalStorageReady = false;
bool transferHistoryStorageReady = false;
bool transferHistoryStorageFault = false;
bool transferHistoryLoadChecked = false;
bool transferHistoryDirty = false;
bool transferHistorySaveAttempted = false;
bool transferHistoryBeginAttempted = false;
uint32_t lastTransferHistoryAttemptId = 0;
uint32_t lastTransferHistorySaveAttemptMs = 0;
uint32_t lastTransferHistoryBeginAttemptMs = 0;
HWC::TransferRequest pendingTransferRequest = {};
bool transferStartRequested = false;
bool transferStopRequested = false;
uint32_t transferRequestCounter = 1;
HWC::TransferOutputs transferOutputs = {HWC::TRANSFER_IDLE, HWC::VALVES_CLOSE_ALL, HWC::TRANSFER_REASON_NONE, 0, 0.0f, 0.0f};
bool webStarted = false;
bool apMode = false;
bool apCredentialStorageReady = false;
bool initialConfigApPasswordGenerated = false;
String apName;
String apPassword;
String adminPassword;
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
HWC::InflowPersistencePolicy inflowPersistence = HWC::defaultInflowPersistencePolicy();
bool inflowStorageWritable = false;
bool timeSyncConfigured = false;
bool protectionConfigLoaded = false;
bool wellProtectionConfigConfigured = false;
bool protectionStorageWritable = false;
bool persistedCriticalLatch = false;
bool emergencyEnableRequested = false;
bool emergencyConfirmationRequested = false;
bool emergencyDisableRequested = false;
bool otaAuthorized = false;
bool otaStarted = false;
bool otaSucceeded = false;
String otaError;


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

uint32_t currentLocalDateKey(bool& valid) {
    valid = false;
    const time_t now = time(nullptr);
    if (now < 1577836800) return 0; // 2020-01-01; reject unset/invalid system time
    struct tm local = {};
    if (localtime_r(&now, &local) == nullptr || local.tm_year < 120) return 0;
    valid = true;
    return static_cast<uint32_t>((local.tm_year + 1900) * 10000 + (local.tm_mon + 1) * 100 + local.tm_mday);
}

void configureTimeSyncIfNeeded() {
    if (timeSyncConfigured) return;
    const bool wifiReady = WiFi.STA.connected() && WiFi.STA.hasIP();
    const bool ethernetReady = ethernetStartup.load() == EthernetStartup::Ready && ETH.linkUp() && ETH.hasIP();
    if (!wifiReady && !ethernetReady) return;
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    configTime(0, 0, "pool.ntp.org");
    timeSyncConfigured = true;
    Serial.println("Time sync requested; daily history will remain undated until system time is valid.");
}

void observeWellInflow(const HWC::LevelSensorReading& reading, uint32_t nowMs) {
    bool dateValid = false;
    const uint32_t dateKey = currentLocalDateKey(dateValid);
    bool volumeValid = false;
    float volumeLiters = 0.0f;
    if (reading.result.status == HWC::SENSOR_MATH_OK) {
        const HWC::WellVolumeResult volume = HWC::estimateWellVolume(
            wellGeometry, reading.result.calibratedLevelCm, wellCalibration);
        if (volume.status == HWC::WELL_VOLUME_GEOMETRY_ESTIMATE ||
            volume.status == HWC::WELL_VOLUME_CALIBRATED_EXACT ||
            volume.status == HWC::WELL_VOLUME_CALIBRATED_INTERPOLATED) {
            volumeValid = true;
            volumeLiters = volume.liters;
        }
    }
    const HWC::InflowUpdate update = inflowTracker.observe(volumeLiters, volumeValid, dateKey, dateValid);
    if (update.positiveChange) {
        Serial.printf("INFLOW: +%.1f L (%s date)\n", update.positiveLiters,
                      update.storedInDatedHistory ? "dated" : "undated");
    }
    // Only positive inflow or a valid-date rollover dirties the persistent history.
    // This avoids writing NVS every minute when the water level is unchanged.
    HWC::observeInflowPersistence(inflowPersistence, update.positiveChange, dateKey, dateValid);
    // Persist changed snapshots at most once per five minutes to reduce flash wear.
    // On reboot the measurement baseline is deliberately reset to avoid attributing
    // an unobserved interval to inflow.
    if (HWC::inflowPersistenceDue(inflowPersistence, inflowStorageWritable, nowMs)) {
        HWC::DailyInflowSnapshot snapshot = {};
        inflowTracker.exportSnapshot(snapshot);
        if (inflowStore.save(snapshot)) {
            HWC::markInflowPersisted(inflowPersistence, nowMs);
        } else {
            inflowStorageWritable = false;
            Serial.println("ERROR: daily inflow history save failed; automatic overwrite disabled.");
        }
    }
}

HWC::SensorConfigLoadState loadConfig(const char* key, HWC::SensorConfig& config) {
    if (!configStore.hasRecord(key)) {
        config = HWC::defaultSensorConfig();
        Serial.printf("NVS sensor config '%s': missing; provisional defaults are diagnostic only; safety use is blocked\n", key);
        return HWC::SENSOR_CONFIG_MISSING_PROVISIONAL;
    }
    if (configStore.load(key, config)) {
        Serial.printf("NVS calibration '%s': stored record loaded and checksum valid\n", key);
        return HWC::SENSOR_CONFIG_VALID_STORED;
    }
    config = HWC::defaultSensorConfig();
    Serial.printf("ERROR: NVS calibration '%s' exists but is invalid; defaults are diagnostic only and safety use is blocked\n", key);
    return HWC::SENSOR_CONFIG_INVALID_PRESENT;
}
}


String htmlEscape(const String& value) {
    String out = value;
    out.replace("&", "&amp;"); out.replace("<", "&lt;");
    out.replace(">", "&gt;"); out.replace("\"", "&quot;");
    out.replace("'", "&#39;");
    return out;
}

bool parseFloatStrict(const String& text, float& value) {
    const char* begin = text.c_str();
    char* end = nullptr;
    value = strtof(begin, &end);
    if (end == begin || !std::isfinite(value)) return false;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    return *end == '\0';
}

bool parseUnsignedStrict(const String& text, uint32_t& value) {
    const char* begin = text.c_str();
    if (*begin == '-' || *begin == '+') return false;
    char* end = nullptr;
    const unsigned long parsed = strtoul(begin, &end, 10);
    if (end == begin || parsed > 0xFFFFFFFFUL) return false;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    if (*end != '\0') return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

String pageStart(const char* title) {
    String h = "<!doctype html><html lang='cs'><head><meta charset='utf-8'>";
    h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    h += "<title>Home Water Controller</title><style>body{font:16px Arial;max-width:900px;margin:20px auto;padding:0 14px;background:#f4f6f8;color:#17202a}section{background:white;padding:16px;margin:14px 0;border-radius:10px}input,textarea{box-sizing:border-box;width:100%;padding:9px;margin:5px 0 12px}button{padding:10px 16px}small{color:#555}.warn{color:#9b1c1c}</style>";
    h += HWC::WATER_LEVEL_SKETCH_CSS;
    h += "<style>.level-vessel.well{height:156px;width:76px}"
         ".status-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,180px),1fr));gap:10px}"
         ".status-card,.route{border:1px solid #d8e1eb;border-radius:8px;padding:12px}.status-card p{margin:7px 0 0}"
         ".relay-state{display:inline-block;border-radius:6px;padding:3px 9px;margin-right:5px;color:white}"
         ".relay-on{background:#176241}.relay-off{background:#a12525}.relay-fault{background:#725019}"
         ".status-card details{margin-top:10px}.status-card table{width:100%;margin-top:7px}"
         ".route{margin:10px 0;padding:0}.route summary{padding:10px 12px;background:#eaf2fa;border-radius:8px}"
         ".route-body{padding:4px 12px 12px}.route-body label{display:block;margin-top:8px}"
         "summary{cursor:pointer;font-weight:bold}.good{color:#176241}.running{color:#12578c}"
         ".stop{background:#a12525;color:white;border:0;border-radius:6px;font-weight:bold}</style>";
    h += "</head><body><h1>Home Water Controller</h1><h2>";
    h += title; h += "</h2>"; return h;
}

bool isSameOriginPost(WebServer& server) {
    // HTTP Basic Auth is browser-cached. Require explicit same-origin headers
    // on every state-changing POST, independent of the network transport.
    if (server.method() != HTTP_POST || !server.hasHeader("Origin") || !server.hasHeader("Host"))
        return false;
    return HWC::isSameHttpOriginAuthority(server.header("Origin").c_str(),
                                          server.header("Host").c_str());
}

bool requireWebAuth() {
    if (webServer.method() == HTTP_POST && !isSameOriginPost(webServer)) {
        webServer.send(403, "text/plain; charset=utf-8", "Cross-origin or unverifiable request rejected");
        return false;
    }
    if (adminPassword.length() < 12) {
        webServer.send(503, "text/plain; charset=utf-8", "Web administrator credentials unavailable");
        return false;
    }
    const IPAddress remoteIp = webServer.client().remoteIP();
    const uint32_t clientKey = (static_cast<uint32_t>(remoteIp[0]) << 24) |
                               (static_cast<uint32_t>(remoteIp[1]) << 16) |
                               (static_cast<uint32_t>(remoteIp[2]) << 8) |
                               static_cast<uint32_t>(remoteIp[3]);
    const uint32_t now = millis();
    if (webAuthRateLimiter.isBlocked(clientKey, now)) {
        webServer.send(429, "text/plain; charset=utf-8", "Too many failed login attempts; wait 60 seconds.");
        return false;
    }
    if (webServer.authenticate("admin", adminPassword.c_str())) {
        webAuthRateLimiter.recordSuccess(clientKey);
        return true;
    }
    webAuthRateLimiter.recordFailure(clientKey, now);
    webServer.requestAuthentication(BASIC_AUTH, "Home Water Controller", "Přihlášení správce");
    return false;
}

void sendResult(const String& message, bool ok);

const char* transferStateText(HWC::TransferState state) {
    switch (state) {
        case HWC::TRANSFER_IDLE: return "Připraveno";
        case HWC::TRANSFER_PRECHECK: return "Kontrola před transferem";
        case HWC::TRANSFER_RUNNING: return "TRANSFER PROBÍHÁ";
        case HWC::TRANSFER_COMPLETED: return "Transfer dokončen";
        case HWC::TRANSFER_STOPPED: return "Transfer zastaven";
        case HWC::TRANSFER_ABORTED: return "Transfer přerušen chybou";
        default: return "Neznámý stav";
    }
}
const char* transferReasonText(HWC::TransferEndReason reason) {
    switch (reason) {
        case HWC::TRANSFER_REASON_NONE: return "bez výsledku";
        case HWC::TRANSFER_REASON_TARGET_REACHED: return "cílová hladina dosažena";
        case HWC::TRANSFER_REASON_USER_STOP: return "zastaveno uživatelem";
        case HWC::TRANSFER_REASON_INVALID_REQUEST: return "neplatný požadavek";
        case HWC::TRANSFER_REASON_DUPLICATE_REQUEST: return "duplicitní ID požadavku";
        case HWC::TRANSFER_REASON_SENSOR_INVALID: return "neplatné nebo fyzicky nemožné měření";
        case HWC::TRANSFER_REASON_SOURCE_MINIMUM: return "zdroj dosáhl minimální hladiny";
        case HWC::TRANSFER_REASON_CRITICAL_PROTECTION: return "kritická ochrana aktivní";
        case HWC::TRANSFER_REASON_LOWER_CAPACITY: return "dolní nádrž nemá kapacitu";
        case HWC::TRANSFER_REASON_COMMUNICATION: return "historický důvod: komunikace mezi jednotkami";
        case HWC::TRANSFER_REASON_SYSTEM_NOT_READY: return "systém nebo konfigurace není připravena";
        case HWC::TRANSFER_REASON_TIMEOUT: return "vypršel celkový časový limit transferu";
        case HWC::TRANSFER_REASON_NO_INFLOW: return "nebyl zjištěn přítok nebo se přítok přerušil";
        default: return "neznámá příčina";
    }
}

void restoreTransferRequestReplayGuard(const HWC::TransferHistorySnapshot& snapshot) {
    uint32_t ids[HWC::TRANSFER_HISTORY_CAPACITY] = {};
    const size_t first = (static_cast<size_t>(snapshot.nextIndex) + HWC::TRANSFER_HISTORY_CAPACITY - snapshot.count) % HWC::TRANSFER_HISTORY_CAPACITY;
    for (size_t i = 0; i < snapshot.count; ++i) {
        ids[i] = snapshot.entries[(first + i) % HWC::TRANSFER_HISTORY_CAPACITY].requestId;
    }
    if (!transferLogic.restoreRecentRequestIds(ids, snapshot.count)) {
        Serial.println("ERROR: transfer request replay guard could not be restored.");
    } else {
        Serial.printf("Transfer request replay guard restored: %u recent IDs.\n", static_cast<unsigned>(snapshot.count));
    }
}

void writeValveRelayLevel(uint8_t pin, bool high) {
    digitalWrite(pin, high ? HIGH : LOW);
}

void writePumpSafetyRelayLevel(uint8_t pin, bool high) {
    digitalWrite(pin, high ? HIGH : LOW);
}

void syncPumpSafetyRelayOutputs(uint32_t nowMs) {
    const bool wellFresh = HWC::sampleIsFresh(haveWellReading, nowMs, lastWellReadingMs, 5000U);
    const bool lowerFresh = HWC::sampleIsFresh(haveLowerTankReading, nowMs, lastLowerTankReadingMs, 5000U);
    const bool wellPermit = pumpProtectionState.emergencyModeActive ||
        (wellFresh && pumpProtectionState.pumpPermit);
    const bool lowerPermit = lowerPumpState.emergencyModeActive ||
        (lowerFresh && lowerPumpState.pumpPermit);
    if (!wellPumpSafetyRelayReady || !wellPumpSafetyRelayOutput.applyPermit(wellPermit)) {
        if (wellPumpSafetyRelayReady) wellPumpSafetyRelayOutput.forceOff();
    }
    if (!lowerPumpSafetyRelayReady || !lowerPumpSafetyRelayOutput.applyPermit(lowerPermit)) {
        if (lowerPumpSafetyRelayReady) lowerPumpSafetyRelayOutput.forceOff();
    }
}

HWC::ManualValveCommand transferValveCommandForRelay() {
    // Explicit mapping keeps transfer logic independent from the manual-control enum.
    switch (transferOutputs.valveCommand) {
        case HWC::VALVE_OPEN_WELL_TO_LOWER:
            return HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER;
        case HWC::VALVE_OPEN_UPPER_TO_LOWER:
            return HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER;
        case HWC::VALVES_CLOSE_ALL:
        default:
            return HWC::MANUAL_VALVES_CLOSE_ALL;
    }
}

void syncManualValveRelayOutput() {
    if (!manualValveRelayOutputReady) {
        if (manualValveControl.active()) manualValveControl.stop();
        if (transferLogic.state() == HWC::TRANSFER_PRECHECK ||
            transferLogic.state() == HWC::TRANSFER_RUNNING) {
            transferOutputs = transferLogic.forceAbort(HWC::TRANSFER_REASON_SYSTEM_NOT_READY);
            Serial.println("ERROR: valve relay driver unavailable; transfer aborted before/while opening.");
        }
        return;
    }

    const HWC::ManualValveCommand desired = manualValveControl.active()
        ? manualValveControl.command() : transferValveCommandForRelay();
    if (!manualValveRelayOutput.apply(desired)) {
        manualValveRelayOutput.forceClose();
        if (manualValveControl.active()) manualValveControl.stop();
        if (transferLogic.state() == HWC::TRANSFER_PRECHECK ||
            transferLogic.state() == HWC::TRANSFER_RUNNING) {
            transferOutputs = transferLogic.forceAbort(HWC::TRANSFER_REASON_SYSTEM_NOT_READY);
        }
        Serial.println("ERROR: valve relay output rejected command; transfer/manual command aborted and both relays commanded OFF.");
    }
}

bool manualValveSensorFaultPresent(uint32_t nowMs) {
    const bool wellFresh = HWC::sampleIsFresh(haveWellReading, nowMs, lastWellReadingMs, 5000U) &&
        lastWellReading.result.status == HWC::SENSOR_MATH_OK &&
        HWC::sensorConfigUsableForLevelMeasurement(wellSensorConfigState) &&
        std::isfinite(lastWellReading.result.calibratedLevelCm) &&
        lastWellReading.result.calibratedLevelCm >= 0.0f &&
        lastWellReading.result.calibratedLevelCm <= wellGeometry.usableDepthCm;
    const bool lowerFresh = HWC::sampleIsFresh(haveLowerTankReading, nowMs, lastLowerTankReadingMs, 5000U) &&
        lastLowerTankReading.result.status == HWC::SENSOR_MATH_OK &&
        HWC::sensorConfigUsableForLevelMeasurement(lowerSensorConfigState);
    // Upper-tank protection is local to ESP32 #2; no peer link is part of this unit's fault decision.
    return !wellFresh || !lowerFresh;
}

bool manualValveSafetyPermitted(HWC::ManualValveCommand command, uint32_t nowMs) {
    if (!manualValveRelayOutputReady) return false;
    if (command == HWC::MANUAL_VALVES_CLOSE_ALL) return true;

    // Emergency mode is an intentional operator override for valve-process gates:
    // it does not require lower-tank data, upper peer communication, upper hysteresis,
    // or ordinary transfer thresholds. The well-pump dry-run permit is the exception:
    // if the well sensor is usable, its existing pump permit must still allow the route.
    // If that sensor itself is unavailable, the operator's physical inspection is the
    // emergency fallback. ESP32 #2 continues its own local upper-pump protection
    // independently of this ESP32 #1 valve command.
    if (manualValveControl.operatingMode() == HWC::MANUAL_VALVE_OPERATING_EMERGENCY) {
        if (command == HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER) {
            const bool wellSensorUsable =
                HWC::sampleIsFresh(haveWellReading, nowMs, lastWellReadingMs, 5000U) &&
                lastWellReading.result.status == HWC::SENSOR_MATH_OK &&
                HWC::sensorConfigUsableForLevelMeasurement(wellSensorConfigState) &&
                std::isfinite(lastWellReading.result.calibratedLevelCm) &&
                lastWellReading.result.calibratedLevelCm >= 0.0f &&
                lastWellReading.result.calibratedLevelCm <= wellGeometry.usableDepthCm;
            return HWC::emergencyWellValvePumpGuardAllows(
                wellSensorUsable, pumpProtectionState.pumpPermit);
        }
        return command == HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER;
    }

    const bool lowerFresh = HWC::sampleIsFresh(haveLowerTankReading, nowMs, lastLowerTankReadingMs, 5000U) &&
        lastLowerTankReading.result.status == HWC::SENSOR_MATH_OK &&
        HWC::sensorConfigUsableForLevelMeasurement(lowerSensorConfigState) &&
        std::isfinite(lastLowerTankReading.result.calibratedLevelCm) &&
        lastLowerTankReading.result.calibratedLevelCm >= 0.0f &&
        lastLowerTankReading.result.calibratedLevelCm < HWC::LOWER_TANK_MAX_LEVEL_CM;
    if (!lowerFresh) return false; // Unknown destination capacity is never overridden by manual mode.

    if (command == HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER) {
        if (!wellProtectionConfigConfigured || !protectionStorageWritable ||
            pumpProtectionState.criticalLevelLatched || pumpProtectionState.systemFault) return false;
        const bool wellFresh = HWC::sampleIsFresh(haveWellReading, nowMs, lastWellReadingMs, 5000U) &&
            lastWellReading.result.status == HWC::SENSOR_MATH_OK &&
                HWC::sensorConfigUsableForLevelMeasurement(wellSensorConfigState);
        // Service mode requires a valid well sensor. Emergency mode has already
        // returned through the explicit override branch above.
        if (!wellFresh) return false;
        if (wellFresh && lastWellReading.result.calibratedLevelCm <= wellProtectionConfig.transferMinimumLevelCm)
            return false;
        return true;
    }

    if (command == HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER) {
        // ESP32 #2 independently protects the upper pump. ESP32 #1 only checks
        // its own receiving tank and valve relay before opening the route.
        return lowerFresh;
    }
    return command == HWC::MANUAL_VALVES_CLOSE_ALL;
}

const char* manualValveCommandText(HWC::ManualValveCommand command) {
    switch (command) {
        case HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER: return "povel relé: studna → dolní nádrž";
        case HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER: return "povel relé: horní → dolní nádrž";
        default: return "povel relé: CLOSE_ALL";
    }
}

void handleManualValveMode() {
    if (!requireWebAuth()) return;
    const String mode = webServer.arg("mode");
    if (mode == "disable") {
        manualValveControl.deactivate();
        sendResult("Ruční ovládání ventilů ukončeno; obě relé dostanou povel CLOSE_ALL.", true);
        return;
    }
    HWC::ManualValveOperatingMode requestedMode = HWC::MANUAL_VALVE_OPERATING_NONE;
    if (mode == "service") requestedMode = HWC::MANUAL_VALVE_OPERATING_SERVICE;
    else if (mode == "emergency") requestedMode = HWC::MANUAL_VALVE_OPERATING_EMERGENCY;
    else { sendResult("Neznámý režim ručního ovládání ventilů.", false); return; }
    if (!manualValveRelayOutputReady) {
        sendResult("Ruční ovládání je blokováno: reléový výstup není připraven.", false);
        return;
    }
    const bool physical = webServer.hasArg("physical") && webServer.arg("physical") == "yes";
    const uint32_t nowMs = millis();
    const bool busy = transferLogic.state() == HWC::TRANSFER_PRECHECK ||
        transferLogic.state() == HWC::TRANSFER_RUNNING;
    if (!busy && transferLogic.state() != HWC::TRANSFER_IDLE && transferRestartGuard.canStart())
        transferLogic.acknowledgeResult();
    const bool recoveryReady = transferJournalStorageReady && transferRestartGuard.canStart();
    const HWC::ManualValveResult result = manualValveControl.activate(
        requestedMode, physical, manualValveSensorFaultPresent(nowMs), busy, recoveryReady);
    if (result != HWC::MANUAL_VALVE_OK) {
        const char* message = result == HWC::MANUAL_VALVE_CONFIRMATION_REQUIRED
            ? "Nouzový režim vyžaduje výslovné potvrzení fyzické kontroly hladin."
            : result == HWC::MANUAL_VALVE_SENSOR_FAULT_REQUIRED
                ? "Nouzový režim vyžaduje výslovné potvrzení obsluhy."
                : result == HWC::MANUAL_VALVE_SENSOR_FAULT_BLOCKS_SERVICE
                    ? "Servisní režim vyžaduje funkční a aktuální relevantní čidla. Při poruše použijte nouzový režim s fyzickou kontrolou hladin."
                    : result == HWC::MANUAL_VALVE_TRANSFER_BUSY
                        ? "Nejprve musí být transfer zcela ukončen a jeho výsledek potvrzen."
                        : "Ruční režim blokuje nevyřešený stav žurnálu transferu.";
        sendResult(message, false);
        return;
    }
    sendResult(requestedMode == HWC::MANUAL_VALVE_OPERATING_SERVICE
        ? "Servisní režim ventilů aktivní. Všechna běžná bezpečnostní omezení zůstávají zachována; obě relé jsou vypnutá až do samostatného povelu."
        : "Nouzové ruční ovládání ventilů aktivní po potvrzení fyzické kontroly hladin. Zachovány jsou všechny dostupné ochrany; obě relé jsou vypnutá až do samostatného povelu.", true);
}

void handleManualValveCommand() {
    if (!requireWebAuth()) return;
    const String requested = webServer.arg("command");
    HWC::ManualValveCommand command = HWC::MANUAL_VALVES_CLOSE_ALL;
    if (requested == "well") command = HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER;
    else if (requested == "upper") command = HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER;
    else if (requested != "close") { sendResult("Neznámý ventilový povel.", false); return; }
    const bool safetyPermitted = command == HWC::MANUAL_VALVES_CLOSE_ALL ||
        manualValveSafetyPermitted(command, millis());
    const HWC::ManualValveResult result = manualValveControl.command(command, safetyPermitted);
    if (result != HWC::MANUAL_VALVE_OK) {
        const char* message = result == HWC::MANUAL_VALVE_NOT_ACTIVE
            ? "Nejdříve aktivujte nouzový ruční režim."
            : result == HWC::MANUAL_VALVE_CLOSE_BEFORE_SWITCH
                ? "Před změnou směru nejprve použijte STOP / CLOSE_ALL."
                : "Povel je blokován bezpečnostní podmínkou nebo neplatným měřením.";
        sendResult(message, false);
        return;
    }
    sendResult(String("Povel předán relé: ") + manualValveCommandText(command) + ". Zobrazen je stav povelu, nikoli ověřená fyzická poloha ventilu.", true);
}

void updateManualValveSafety(uint32_t nowMs) {
    if (!manualValveControl.active()) return;
    const HWC::ManualValveCommand command = manualValveControl.command();
    if (command != HWC::MANUAL_VALVES_CLOSE_ALL && !manualValveSafetyPermitted(command, nowMs)) {
        manualValveControl.stop();
        Serial.println("MANUAL VALVE: safety condition lost; logical command changed to CLOSE_ALL.");
    }
}

void handleTransferStart() {
    if (!requireWebAuth()) return;
    if (manualValveControl.active()) {
        sendResult("Automatický transfer je blokován, dokud je aktivní nouzový ruční režim ventilů.", false); return;
    }
    if (!manualValveRelayOutputReady) {
        sendResult("Transfer nelze spustit: reléový výstup ventilů není inicializován.", false); return;
    }
    if (transferLogic.state() == HWC::TRANSFER_PRECHECK || transferLogic.state() == HWC::TRANSFER_RUNNING) {
        sendResult("Transfer již probíhá. Nejprve jej zastavte.", false); return;
    }
    if (transferLogic.state() != HWC::TRANSFER_IDLE) transferLogic.acknowledgeResult();
    const String sourceText = webServer.arg("source");
    const HWC::TransferSource source = sourceText == "well" ? HWC::TRANSFER_SOURCE_WELL :
        (sourceText == "upper" ? HWC::TRANSFER_SOURCE_UPPER_TANK : HWC::TRANSFER_SOURCE_NONE);
    float liters = 0.0f, timeoutMinutes = 0.0f;
    if (source == HWC::TRANSFER_SOURCE_WELL && !wellProtectionConfigConfigured) {
        sendResult("Transfer ze studny je blokován: nejsou platně nastaveny bezpečnostní meze ochrany čerpadla.", false); return;
    }
    if (source == HWC::TRANSFER_SOURCE_NONE || !parseFloatStrict(webServer.arg("liters"), liters) ||
        !parseFloatStrict(webServer.arg("timeout"), timeoutMinutes) || liters <= 0.0f || liters > 7000.0f ||
        timeoutMinutes < 1.0f || timeoutMinutes > 120.0f) {
        sendResult("Zadejte zdroj, objem 0–7000 l a časový limit 1–120 minut.", false); return;
    }
    pendingTransferRequest.requestId = (static_cast<uint32_t>(esp_random()) ^ (++transferRequestCounter));
    if (pendingTransferRequest.requestId == 0) pendingTransferRequest.requestId = ++transferRequestCounter;
    pendingTransferRequest.source = source;
    pendingTransferRequest.requestedLiters = liters;
    pendingTransferRequest.timeoutMs = static_cast<uint32_t>(timeoutMinutes * 60000.0f);
    if (!transferJournalStorageReady || !transferHistoryStorageReady || transferHistoryStorageFault ||
        !transferRestartGuard.canStart() ||
        !transferRestartGuard.prepare(pendingTransferRequest.requestId, source, liters)) {
        sendResult("Transfer nelze spustit: žurnál obnovy nebo historie NVS není připravena, případně čeká na potvrzení předchozího stavu.", false); return;
    }
    transferStartRequested = true;
    transferStopRequested = false;
    sendResult("Požadavek na automatický transfer byl přijat. Po bezpečnostní kontrole bude ovládán příslušný ventil; čerpadlo se nespouští. Výsledek se objeví na hlavní stránce.", true);
}

void handleTransferStop() {
    if (!requireWebAuth()) return;
    transferStopRequested = true;
    transferStartRequested = false;
    sendResult("Požadavek na zastavení transferu byl přijat. Ventil bude uzavřen; čerpadlo se neovládá.", true);
}

void handleInflowWatchdogConfig() {
    if (!requireWebAuth()) return;
    if (transferLogic.state() == HWC::TRANSFER_PRECHECK || transferLogic.state() == HWC::TRANSFER_RUNNING) {
        sendResult("Nastavení ochrany přítoku nelze měnit během aktivního transferu. Zkuste to po jeho ukončení.", false);
        return;
    }
    uint32_t startupSeconds = 0U, noProgressSeconds = 0U;
    if (!parseUnsignedStrict(webServer.arg("startup_seconds"), startupSeconds) ||
        !parseUnsignedStrict(webServer.arg("no_progress_seconds"), noProgressSeconds)) {
        sendResult("Zadejte oba časové limity jako celá čísla sekund.", false); return;
    }
    HWC::LowerTankInflowWatchdogSettings candidate = {startupSeconds, noProgressSeconds};
    if (!HWC::isLowerTankInflowWatchdogSettingsValid(candidate)) {
        sendResult("Oba limity musí být v rozsahu 1 až 3600 sekund.", false); return;
    }
    if (!inflowWatchdogStore.begin() || !inflowWatchdogStore.save(candidate)) {
        inflowWatchdogSettingsStorageFault = true;
        inflowWatchdogSettingsLoaded = false;
        sendResult("Uložení nebo kontrolní načtení nastavení ochrany přítoku z NVS selhalo. Horní transfer zůstává blokovaný.", false);
        return;
    }
    inflowWatchdogSettings = candidate;
    inflowWatchdogSettingsLoaded = true;
    inflowWatchdogSettingsStorageFault = false;
    sendResult("Časové limity ochrany přítoku byly uloženy a zpětně ověřeny. Použijí se při následujícím transferu.", true);
}

bool transferHistoryContainsRequest(uint32_t requestId) {
    const HWC::TransferHistorySnapshot& snapshot = transferHistory.snapshot();
    for (size_t i = 0; i < snapshot.count; ++i) {
        if (snapshot.entries[i].requestId == requestId) return true;
    }
    return false;
}

void handleTransferRecoveryAck() {
    if (!requireWebAuth()) return;
    const HWC::TransferRestartGuardState state = transferRestartGuard.state();
    if (state != HWC::TRANSFER_RESTART_INTERRUPTED && state != HWC::TRANSFER_RESTART_RESULT_PENDING) {
        sendResult("Není zde čekající přerušený transfer k potvrzení.", false); return;
    }
    if (!transferJournalStorageReady || transferHistoryStorageFault) {
        sendResult("Obnovu nelze potvrdit: úložiště transferů není spolehlivě dostupné.", false); return;
    }
    if (!transferHistoryStorageReady) {
        sendResult("Obnovu nelze potvrdit: NVS historie není dostupná.", false); return;
    }
    const HWC::TransferJournalRecord& journal = transferRestartGuard.record();
    if (!transferHistoryContainsRequest(journal.requestId)) {
        HWC::TransferHistoryEntry entry = {};
        entry.requestId = journal.requestId;
        entry.source = journal.source;
        entry.terminalState = state == HWC::TRANSFER_RESTART_RESULT_PENDING
            ? journal.terminalState : static_cast<uint8_t>(HWC::TRANSFER_ABORTED);
        entry.endReason = state == HWC::TRANSFER_RESTART_RESULT_PENDING
            ? journal.endReason : static_cast<uint8_t>(HWC::TRANSFER_REASON_SYSTEM_NOT_READY);
        entry.requestedLiters = journal.requestedLiters;
        entry.estimatedTransferredLiters = state == HWC::TRANSFER_RESTART_RESULT_PENDING
            ? journal.estimatedTransferredLiters : 0.0f;
        entry.targetLowerLevelCm = journal.targetLowerLevelCm;
        entry.finishedAtUptimeMs = millis();
        if (!transferHistory.append(entry)) {
            sendResult("Obnovu nelze potvrdit: záznam přerušeného transferu se nepodařilo zařadit do historie.", false); return;
        }
    }
    if (!transferHistoryStore.save(transferHistory.snapshot())) {
        transferHistoryDirty = true;
        sendResult("Historii se nepodařilo uložit. Transfer zůstává zablokovaný; zkuste potvrzení později.", false); return;
    }
    transferHistoryDirty = false;
    if (!transferRestartGuard.acknowledgeRecovery()) {
        sendResult("Historie byla uložena, ale žurnál obnovy se nepodařilo vymazat. Nový transfer zůstává zablokovaný.", false); return;
    }
    sendResult("Přerušený stav byl zaznamenán do historie a potvrzen. Nový transfer lze znovu připravit.", true);
}

void maintainTransferHistoryStorage(uint32_t nowMs) {
    if (transferHistoryStorageFault) return;

    if (!transferHistoryStorageReady &&
        (!transferHistoryBeginAttempted || static_cast<uint32_t>(nowMs - lastTransferHistoryBeginAttemptMs) >= 30000U)) {
        transferHistoryBeginAttempted = true;
        lastTransferHistoryBeginAttemptMs = nowMs;
        transferHistoryStorageReady = transferHistoryStore.begin();
        if (transferHistoryStorageReady && !transferHistoryLoadChecked) {
            transferHistoryLoadChecked = true;
            HWC::TransferHistorySnapshot saved = {};
            if (transferHistoryStore.hasRecord()) {
                HWC::TransferHistorySnapshot pending = transferHistory.snapshot();
                HWC::TransferHistorySnapshot merged = {};
                if (transferHistoryStore.load(saved) &&
                    HWC::mergeTransferHistorySnapshots(saved, pending, merged) &&
                    transferHistory.restore(merged)) {
                    restoreTransferRequestReplayGuard(merged);
                    if (pending.count > 0) transferHistoryDirty = true;
                    Serial.printf("Transfer history recovered from NVS: %u entries; RAM entries merged.\n", static_cast<unsigned>(transferHistory.snapshot().count));
                } else {
                    transferHistoryStorageFault = true;
                    transferHistoryStorageReady = false;
                    transferHistoryStore.end();
                    Serial.println("ERROR: transfer history NVS record is corrupt; automatic overwrite disabled.");
                    return;
                }
            } else {
                Serial.println("Transfer history NVS became available; no prior record exists.");
            }
        }
        if (!transferHistoryStorageReady) {
            Serial.println("WARNING: transfer history NVS unavailable; will retry in 30 seconds.");
        }
    }

    if (transferHistoryDirty && transferHistoryStorageReady &&
        (!transferHistorySaveAttempted || static_cast<uint32_t>(nowMs - lastTransferHistorySaveAttemptMs) >= 5000U)) {
        transferHistorySaveAttempted = true;
        lastTransferHistorySaveAttemptMs = nowMs;
        if (transferHistoryStore.save(transferHistory.snapshot())) {
            transferHistoryDirty = false;
            if (transferRestartGuard.state() == HWC::TRANSFER_RESTART_RESULT_PENDING &&
                !transferRestartGuard.acknowledgeRecovery()) {
                Serial.println("ERROR: transfer result is saved, but restart journal could not be cleared; new transfers remain blocked.");
            }
            Serial.println("TRANSFER HISTORY: pending snapshot persisted to NVS.");
        } else {
            Serial.println("ERROR: transfer history save failed; pending snapshot retained in RAM and will be retried.");
        }
    }
}

void updateTransferControl(uint32_t nowMs) {
    if (manualValveControl.active()) {
        transferStartRequested = false;
        transferStopRequested = false;
        return;
    }
    HWC::TransferInputs in = {};
    in.nowMs = nowMs;
    in.startRequested = transferStartRequested;
    in.stopRequested = transferStopRequested;
    in.request = pendingTransferRequest;
    // Freshness is tracked independently: an unrelated failed source sensor must not
    // invalidate a healthy lower-tank destination or the other transfer source.
    const bool lowerFresh = HWC::sampleIsFresh(
        haveLowerTankReading, nowMs, lastLowerTankReadingMs, 5000U);
    const bool wellFresh = HWC::sampleIsFresh(
        haveWellReading, nowMs, lastWellReadingMs, 5000U);
    in.lowerLevelValid = lowerFresh && lastLowerTankReading.result.status == HWC::SENSOR_MATH_OK &&
        HWC::sensorConfigUsableForLevelMeasurement(lowerSensorConfigState);
    in.lowerLevelCm = in.lowerLevelValid ? lastLowerTankReading.result.calibratedLevelCm : 0.0f;
    const bool upperSource = pendingTransferRequest.source == HWC::TRANSFER_SOURCE_UPPER_TANK;
    in.sourceLevelValid = !upperSource && wellFresh &&
        lastWellReading.result.status == HWC::SENSOR_MATH_OK &&
        HWC::sensorConfigUsableForLevelMeasurement(wellSensorConfigState);
    in.sourceLevelCm = in.sourceLevelValid ? lastWellReading.result.calibratedLevelCm : 0.0f;
    in.sourceMinimumTransferLevelCm = upperSource ? NAN :
        (wellProtectionConfigConfigured ? wellProtectionConfig.transferMinimumLevelCm : NAN);
    in.sourceMaximumLevelCm = upperSource ? NAN : wellGeometry.usableDepthCm;
    in.criticalProtectionActive = !upperSource &&
        (!protectionConfigLoaded || pumpProtectionState.criticalLevelLatched || pumpProtectionState.systemFault);
    // No peer link: ESP32 #2 independently enforces the upper-pump minimum-level protection.
    in.inflowWatchdogConfigValid = inflowWatchdogSettingsLoaded && !inflowWatchdogSettingsStorageFault;
    in.inflowStartupGraceMs = inflowWatchdogSettings.startupGraceSeconds * 1000UL;
    in.inflowNoProgressTimeoutMs = inflowWatchdogSettings.noProgressTimeoutSeconds * 1000UL;
    in.inflowMinimumProgressLiters = 12.32f; // Provisional deadband: 0.2 cm in the 6.16 m2 lower tank.
    in.systemReady = manualValveRelayOutputReady && in.lowerLevelValid &&
        (upperSource ? in.inflowWatchdogConfigValid
                     : (in.sourceLevelValid && wellProtectionConfigConfigured &&
                        protectionStorageWritable && !pumpProtectionState.systemFault));
    const HWC::TransferState previousState = transferLogic.state();
    transferOutputs = transferLogic.update(in);
    if (transferOutputs.state == HWC::TRANSFER_RUNNING && previousState != HWC::TRANSFER_RUNNING &&
        transferRestartGuard.state() == HWC::TRANSFER_RESTART_PREPARED) {
        if (!transferRestartGuard.markActive(in.lowerLevelCm, transferOutputs.targetLowerLevelCm)) {
            transferOutputs = transferLogic.forceAbort(HWC::TRANSFER_REASON_SYSTEM_NOT_READY);
            Serial.println("ERROR: transfer journal could not persist ACTIVE state; transfer aborted and valve command forced closed.");
        }
    }
    if ((transferOutputs.state == HWC::TRANSFER_COMPLETED ||
         transferOutputs.state == HWC::TRANSFER_STOPPED ||
         transferOutputs.state == HWC::TRANSFER_ABORTED) &&
        transferOutputs.requestId != 0 && transferOutputs.requestId != lastTransferHistoryAttemptId) {
        if (transferRestartGuard.state() == HWC::TRANSFER_RESTART_PREPARED ||
            transferRestartGuard.state() == HWC::TRANSFER_RESTART_ACTIVE) {
            if (!transferRestartGuard.markResult(transferOutputs.state, transferOutputs.endReason,
                                                 transferOutputs.estimatedTransferredLiters)) {
                Serial.println("ERROR: transfer result could not be journaled; restart guard remains fail-closed.");
            }
        }
        HWC::TransferHistoryEntry entry = {};
        entry.requestId = transferOutputs.requestId;
        entry.source = static_cast<uint8_t>(pendingTransferRequest.source);
        entry.terminalState = static_cast<uint8_t>(transferOutputs.state);
        entry.endReason = static_cast<uint8_t>(transferOutputs.endReason);
        entry.requestedLiters = pendingTransferRequest.requestedLiters;
        entry.estimatedTransferredLiters = transferOutputs.estimatedTransferredLiters;
        entry.targetLowerLevelCm = transferOutputs.targetLowerLevelCm;
        entry.finishedAtUptimeMs = nowMs;
        if (transferHistory.append(entry)) {
            lastTransferHistoryAttemptId = transferOutputs.requestId;
            transferHistoryDirty = true;
            Serial.printf("TRANSFER HISTORY: request %lu queued for persistent save. Volume is estimated from lower-tank level change.\n",
                          static_cast<unsigned long>(entry.requestId));
        } else {
            // Do not retry append on every loop; invalid data must not create a busy loop.
            lastTransferHistoryAttemptId = transferOutputs.requestId;
            Serial.printf("ERROR: transfer history rejected request %lu; record remains unavailable.\n",
                          static_cast<unsigned long>(entry.requestId));
        }
    }
    maintainTransferHistoryStorage(nowMs);
    transferStartRequested = false;
    transferStopRequested = false;
}

void appendTransferAction(String& h, const char* source, const char* title, const char* blocker) {
    h += "<details class='route'><summary>";
    h += title;
    h += blocker ? " · Blokováno" : " · Nastavit";
    h += "</summary><div class='route-body'>";
    if (blocker) {
        h += "<p class='warn'>";
        h += blocker;
        h += "</p>";
    } else {
        h += "<form method='post' action='/transfer-start'><input type='hidden' name='source' value='";
        h += source;
        h += "'><label>Přírůstek v dolní nádrži (l)<input name='liters' type='number' min='1' max='7000' step='1' value='500' required></label>"
             "<label>Časový limit (min)<input name='timeout' type='number' min='1' max='120' step='1' value='10' required></label>"
             "<button type='submit'>Potvrdit spuštění</button></form>";
    }
    h += "</div></details>";
}

void appendRelayCard(String& h, const char* title, bool ready, bool energized,
                     const char* onText, const char* offText, bool emergency = false) {
    h += "<div class='status-card'><strong>";
    h += title;
    h += "</strong><p>";
    if (!ready) {
        h += "<b class='relay-state relay-fault'>CHYBA</b> Výstup nepřipraven";
    } else {
        h += energized ? "<b class='relay-state relay-on'>ON</b>" :
                         "<b class='relay-state relay-off'>OFF</b>";
        h += energized ? onText : offText;
    }
    if (emergency) h += "<br><span class='warn'>Nouzový režim aktivní</span>";
    h += "</p></div>";
}

void handleRoot() {
    if (!requireWebAuth()) return;
    String h = pageStart("ESP32 #1 – studna");
    const uint32_t levelNow = millis();
    const bool wellLevelValid = HWC::sampleIsFresh(haveWellReading, levelNow, lastWellReadingMs, 5000U) &&
        lastWellReading.result.status == HWC::SENSOR_MATH_OK &&
        HWC::sensorConfigUsableForLevelMeasurement(wellSensorConfigState);
    const bool lowerLevelValid = HWC::sampleIsFresh(haveLowerTankReading, levelNow, lastLowerTankReadingMs, 5000U) &&
        lastLowerTankReading.result.status == HWC::SENSOR_MATH_OK &&
        HWC::sensorConfigUsableForLevelMeasurement(lowerSensorConfigState);
    h += "<section><h3>Aktuální měření</h3><div class='level-sketches'>";
    HWC::appendWaterLevelSketch(h, "Studna", true, wellLevelValid,
                                lastWellReading.result.calibratedLevelCm, wellGeometry.usableDepthCm);
    HWC::appendWaterLevelSketch(h, "Dolní nádrž", false, lowerLevelValid,
                                lastLowerTankReading.result.calibratedLevelCm, HWC::LOWER_TANK_MAX_LEVEL_CM);
    h += "</div></section><section><h3>Provozní stav</h3><div class='status-grid'>";

    const bool manualActive = manualValveControl.active();
    const bool busy = transferLogic.state() == HWC::TRANSFER_PRECHECK ||
                      transferLogic.state() == HWC::TRANSFER_RUNNING || transferStartRequested;
    const bool watchdogReady = inflowWatchdogSettingsLoaded && !inflowWatchdogSettingsStorageFault;
    const float lowerCm = lastLowerTankReading.result.calibratedLevelCm;
    const float wellCm = lastWellReading.result.calibratedLevelCm;
    const char* commonBlocker = nullptr;
    if (manualActive) commonBlocker = "Je aktivní ruční režim ventilů.";
    else if (busy) commonBlocker = "Transfer už probíhá nebo čeká na kontrolu.";
    else if (!manualValveRelayOutputReady) commonBlocker = "Výstupy ventilů nejsou připravené.";
    else if (!transferJournalStorageReady || !transferHistoryStorageReady ||
             transferHistoryStorageFault || !transferRestartGuard.canStart())
        commonBlocker = "Historie nebo žurnál transferu nejsou připravené.";
    else if (!lowerLevelValid || !std::isfinite(lowerCm) ||
             lowerCm < 0.0f || lowerCm > HWC::LOWER_TANK_MAX_LEVEL_CM)
        commonBlocker = "Chybí platné měření hladiny dolní nádrže.";
    else if (lowerCm >= HWC::LOWER_TANK_MAX_LEVEL_CM)
        commonBlocker = "Dolní nádrž je plná.";

    const char* wellBlocker = commonBlocker;
    if (!wellBlocker) {
        if (!wellLevelValid || !std::isfinite(wellCm) || wellCm < 0.0f || wellCm > wellGeometry.usableDepthCm)
            wellBlocker = "Chybí platné měření hladiny studny.";
        else if (!wellProtectionConfigConfigured || !protectionConfigLoaded || !protectionStorageWritable)
            wellBlocker = "Ochranné meze studny nebo jejich úložiště nejsou připravené.";
        else if (pumpProtectionState.criticalLevelLatched || pumpProtectionState.systemFault)
            wellBlocker = "Ochrana studny blokuje transfer.";
        else if (wellCm <= wellProtectionConfig.transferMinimumLevelCm)
            wellBlocker = "Studna je na transferovém minimu nebo pod ním.";
    }
    const char* upperBlocker = commonBlocker;
    if (!upperBlocker && !watchdogReady) upperBlocker = "Kontrola přítoku do dolní nádrže není nastavena.";

    appendRelayCard(h, "Ventil studny", manualValveRelayOutputReady,
        manualValveRelayOutput.wellRelayEnergized(), "Povel otevřít", "Povel zavřít");
    appendRelayCard(h, "Ventil horní nádrže", manualValveRelayOutputReady,
        manualValveRelayOutput.upperRelayEnergized(), "Povel otevřít", "Povel zavřít");
    appendRelayCard(h, "Čerpadlo studny", wellPumpSafetyRelayReady, wellPumpSafetyRelayOutput.energized(),
        "Napájení povoleno",
        !wellLevelValid ? "Chybí platná hladina." : pumpProtectionState.sensorWarning ?
        "Zkontrolujte čidlo a nastavení ochrany." : pumpProtectionState.criticalLevelLatched ?
        "Čeká na obnovovací hladinu." : "Ochrana odebrala povolení.",
        pumpProtectionState.emergencyModeActive);
    appendRelayCard(h, "Čerpadlo dolní nádrže", lowerPumpSafetyRelayReady, lowerPumpSafetyRelayOutput.energized(),
        "Napájení povoleno",
        !lowerLevelValid ? "Chybí platná hladina." : lowerPumpState.sensorWarning ?
        "Zkontrolujte čidlo a nastavení ochrany." : lowerPumpState.startupValidationRequired ?
        "Čeká na ověření po startu." : lowerPumpState.criticalLevelLatched ?
        "Čeká na obnovovací hladinu." : "Ochrana odebrala povolení.",
        lowerPumpState.emergencyModeActive);

    h += "<div class='status-card'><strong>Přítok do dolní nádrže</strong><p>";
    if (!lowerLevelValid) h += "<b class='warn'>Hladina nedostupná</b>";
    else if (transferOutputs.state == HWC::TRANSFER_RUNNING) {
        h += "Přírůstek od startu: " + String(transferOutputs.estimatedTransferredLiters, 1) + " l (odhad)";
    } else if (!watchdogReady) h += "<b class='warn'>Kontrola horní trasy není nastavena</b>";
    else h += "Automatický transfer neprobíhá";
    h += "</p></div>";

    bool todayValid = false;
    const uint32_t todayKey = currentLocalDateKey(todayValid);
    h += "<div class='status-card'><strong>Přítok do studny</strong><p>";
    if (!todayValid) h += "Čeká na platný čas";
    else h += "Dnes: " + String(inflowTracker.litersForDate(todayKey), 1) + " l (odhad)";
    if (!wellLevelValid) h += "<br><span class='warn'>Aktuální hladina nedostupná</span>";
    h += "</p><details><summary>Historie denního přítoku</summary>";
    h += "<p>Nedatované přírůstky: " + String(inflowTracker.undatedLiters(), 1) + " l</p><table><tr><th align='left'>Datum</th><th align='right'>Přítok</th></tr>";
    const HWC::InflowDay* days = inflowTracker.history();
    for (size_t i = 0; i < inflowTracker.historySize(); ++i) {
        if (days[i].dateKey == 0) continue;
        const uint32_t key = days[i].dateKey;
        char dateText[11];
        snprintf(dateText, sizeof(dateText), "%04lu-%02lu-%02lu",
                 static_cast<unsigned long>(key / 10000U),
                 static_cast<unsigned long>((key / 100U) % 100U),
                 static_cast<unsigned long>(key % 100U));
        h += "<tr><td>"; h += dateText; h += "</td><td align='right'>"; h += String(days[i].liters, 1); h += " l</td></tr>";
    }
    h += "</table><small>Součet zahrnuje jen kladné změny odhadovaného objemu. </small></details></div></div></section>";
    h += "<section><h3>Přečerpávání</h3>";
    if (manualActive)
        h += "<form method='post' action='/manual-valve-command'><button class='stop' name='command' value='close' type='submit'>STOP · zavřít ventily</button></form>";
    else
        h += "<form method='post' action='/transfer-stop'><button class='stop' type='submit'>STOP · zavřít ventily</button></form>";

    if (busy) {
        h += "<p class='running'><b>";
        h += transferStartRequested ? "Čeká na bezpečnostní kontrolu" : transferStateText(transferOutputs.state);
        h += "</b></p>";
    } else if (wellBlocker && upperBlocker) {
        h += "<p class='warn'><b>Spuštění blokováno</b>";
        if (commonBlocker) { h += " · "; h += commonBlocker; }
        h += "</p>";
    }
    appendTransferAction(h, "well", "Studna → dolní nádrž", wellBlocker);
    appendTransferAction(h, "upper", "Horní → dolní nádrž", upperBlocker);
    if (transferOutputs.state == HWC::TRANSFER_RUNNING) {
        h += "<p>Odhad přírůstku: " + String(transferOutputs.estimatedTransferredLiters, 1)
             + " / " + String(pendingTransferRequest.requestedLiters, 0) + " l.</p>";
    }
    if (transferRestartGuard.state() == HWC::TRANSFER_RESTART_INTERRUPTED ||
        transferRestartGuard.state() == HWC::TRANSFER_RESTART_RESULT_PENDING) {
        h += "<p class='warn'><b>POZOR: Předchozí transfer nebyl po restartu bezpečně uzavřen.</b> Automatické pokračování je zakázáno. ID požadavku: ";
        h += String(transferRestartGuard.record().requestId);
        h += ". Po kontrole fyzického stavu nádrží a ventilů potvrďte zaznamenání přerušení.</p><form method='post' action='/transfer-recovery-ack'><button type='submit'>Potvrdit přerušení a odblokovat přípravu</button></form>";
    } else if (transferRestartGuard.state() == HWC::TRANSFER_RESTART_LOCKED) {
        h += "<p class='warn'><b>Transfery jsou zablokované:</b> žurnál obnovy není dostupný nebo je poškozený. Automatický zápis je zakázán.</p>";
    }
    h += "<details><summary>Podrobnosti a poslední výsledek</summary><p>Ventily ukazují povel. U čerpadel je zobrazené bezpečnostní povolení napájení.</p>";
    h += "<p>Stav: <b>"; h += transferStateText(transferOutputs.state); h += "</b>; výsledek: "; h += transferReasonText(transferOutputs.endReason); h += ".</p>";
    h += "<p>Cílová hladina dolní nádrže: "; h += String(transferOutputs.targetLowerLevelCm, 1); h += " cm; odhad změny objemu podle snímače dolní nádrže: "; h += String(transferOutputs.estimatedTransferredLiters, 1); h += " l.</p>";
    h += "<p>Příkaz relé: ";
    if (transferOutputs.valveCommand == HWC::VALVE_OPEN_WELL_TO_LOWER) h += "studna → dolní nádrž";
    else if (transferOutputs.valveCommand == HWC::VALVE_OPEN_UPPER_TO_LOWER) h += "horní → dolní nádrž";
    else h += "obě relé vypnuta – povel CLOSE_ALL";
    h += "</p><p>Stav inicializace reléového výstupu: "; h += manualValveRelayOutputReady ? "READY" : "FAULT / BLOKOVÁNO";
    h += "</p>";
    h += "</details></section>";
    h += "<section><details><summary>Diagnostika hladin a objemu</summary><p><b>Studna:</b> ";
    if (!haveWellReading || lastWellReading.result.status != HWC::SENSOR_MATH_OK) {
        h += "neplatné nebo nedostupné měření";
    } else if (HWC::sensorConfigUsableForLevelMeasurement(wellSensorConfigState)) {
        h += String(lastWellReading.result.calibratedLevelCm, 1) + " cm; " + String(lastWellReading.result.currentMilliAmps, 2) + " mA";
        h += "<br>Objem: ";
        if (lastWellVolume.status == HWC::WELL_VOLUME_GEOMETRY_ESTIMATE) {
            h += "orientační odhad " + String(lastWellVolume.liters, 1) + " l";
            if (!lastWellVolume.absoluteVolumeKnown) h += " (absolutní zásoba neověřena)";
        } else if (lastWellVolume.status == HWC::WELL_VOLUME_CALIBRATED_EXACT || lastWellVolume.status == HWC::WELL_VOLUME_CALIBRATED_INTERPOLATED) {
            h += "naměřená kalibrace " + String(lastWellVolume.liters, 1) + " l";
        } else h += "nedostupný";
    } else {
        h += "<b class='warn'>NEOVĚŘENÝ DIAGNOSTICKÝ PŘEPOČET: ";
        h += String(lastWellReading.result.rawLevelCm, 1) + " cm surově; ";
        h += String(lastWellReading.result.currentMilliAmps, 2) + " mA – nepoužívat jako výšku ani objem</b>";
    }
    h += "<br>Elektrické nastavení snímače: ";
    h += wellSensorConfigState == HWC::SENSOR_CONFIG_VALID_STORED ? "ULOŽENÁ A OVĚŘENÁ" :
         (wellSensorConfigState == HWC::SENSOR_CONFIG_MISSING_PROVISIONAL ? "CHYBÍ OVĚŘENÉ NASTAVENÍ – výška není použitelná pro ochranu" :
          (wellSensorConfigState == HWC::SENSOR_CONFIG_INVALID_PRESENT ? "POŠKOZENÉ ELEKTRICKÉ NASTAVENÍ – výška není použitelná pro ochranu" : "NVS NEDOSTUPNÉ – elektrické nastavení není ověřeno"));
    h += "</p><p><b>Dolní nádrž:</b> ";
    if (!haveLowerTankReading || lastLowerTankReading.result.status != HWC::SENSOR_MATH_OK) {
        h += "neplatné nebo nedostupné měření";
    } else if (HWC::sensorConfigUsableForLevelMeasurement(lowerSensorConfigState)) {
        h += String(lastLowerTankReading.result.calibratedLevelCm, 1) + " cm; " + String(lastLowerTankReading.result.currentMilliAmps, 2) + " mA";
        if (lastLowerTankVolumeValid) h += "<br>Objem podle 61,6 l/cm: " + String(lastLowerTankVolumeLiters, 1) + " l";
        else h += "<br>Objem mimo povolený rozsah 0–121 cm";
        if (lastLowerTankReading.result.calibratedLevelCm < HWC::LOWER_TANK_MIN_WORKING_LEVEL_CM) h += "<br><small>Hladina je pod informativní hranicí 11 cm (orientační hranice běžného vyčerpání).</small>";
        h += "<br><small>Pracovní minimum 11 cm je pouze informativní; neřídí čerpadlo ani transfer.</small>";
    } else {
        h += "<b class='warn'>NEOVĚŘENÝ DIAGNOSTICKÝ PŘEPOČET: ";
        h += String(lastLowerTankReading.result.rawLevelCm, 1) + " cm surově; ";
        h += String(lastLowerTankReading.result.currentMilliAmps, 2) + " mA – nepoužívat jako výšku ani objem</b>";
    }
    h += "<br>Elektrické nastavení snímače: ";
    h += lowerSensorConfigState == HWC::SENSOR_CONFIG_VALID_STORED ? "ULOŽENÁ A OVĚŘENÁ" :
         (lowerSensorConfigState == HWC::SENSOR_CONFIG_MISSING_PROVISIONAL ? "CHYBÍ OVĚŘENÉ NASTAVENÍ – výška není použitelná pro ochranu" :
          (lowerSensorConfigState == HWC::SENSOR_CONFIG_INVALID_PRESENT ? "POŠKOZENÉ ELEKTRICKÉ NASTAVENÍ – výška není použitelná pro ochranu" : "NVS NEDOSTUPNÉ – elektrické nastavení není ověřeno"));
    h += "</p></details></section>";
    h += "<section><details";
    if (manualActive) h += " open";
    h += "><summary>Ruční ovládání ventilů – servis / nouze</summary>";
    h += "<p class='warn'>Ruční i automatický transfer ovládá ventily přes relé GPIO 16/17. Čerpadla běžně spouští a vypíná vlastní tlaková automatika; ESP32 přes samostatná bezpečnostní relé povoluje nebo přerušuje napájení čerpadel (studna GPIO 25, dolní nádrž GPIO 32).</p>";
    h += "<p>Servisní režim slouží k vědomému testu a údržbě při funkčních čidlech. Nouzový režim je určen pouze pro poruchu relevantního čidla. Po výslovném potvrzení fyzické kontroly dovoluje ručně zapnout zvolený ventil a ponechat jej otevřený až do povelu STOP. Nezadává se množství vody a neprobíhá automatické ukončení podle hladiny; kontrolu a včasné vypnutí zajišťuje obsluha. Oba režimy jsou po restartu vypnuté.</p>";
    h += "<p>Stav: <b>";
    if (!manualValveControl.active()) h += "vypnuto";
    else if (manualValveControl.operatingMode() == HWC::MANUAL_VALVE_OPERATING_SERVICE) h += "SERVISNÍ REŽIM AKTIVNÍ";
    else h += "NOUZOVÝ RUČNÍ REŽIM AKTIVNÍ";
    h += "</b><br>";
    h += manualValveCommandText(manualValveControl.command()); h += "</p>";
    h += "<p class='warn'>Ruční příkazy jsou předávány relé GPIO16/17. Zobrazený povel nepotvrzuje fyzickou polohu ventilu; bez koncového spínače není zpětná vazba.</p>";
    if (!manualValveControl.active()) {
        h += "<form method='post' action='/manual-valve-mode'><input type='hidden' name='mode' value='service'><button type='submit'>Aktivovat servisní režim (funkční čidla)</button></form>";
        h += "<form method='post' action='/manual-valve-mode'><input type='hidden' name='mode' value='emergency'><label><input style='width:auto' type='checkbox' name='physical' value='yes' required> Potvrzuji, že jsem si vědom, že nouzový režim obchází běžné kontroly hladin pro ventil, a před otevřením ventilu jsem fyzicky ověřil situaci u nádrží.</label><button type='submit'>Aktivovat nouzové ruční ovládání</button></form>";
    } else {
        h += "<form method='post' action='/manual-valve-command'><input type='hidden' name='command' value='well'><button type='submit'>ZAPNOUT ventil: studna → dolní nádrž</button></form>";
        h += "<form method='post' action='/manual-valve-command'><input type='hidden' name='command' value='upper'><button type='submit'>ZAPNOUT ventil: horní → dolní nádrž</button></form>";
        h += "<form method='post' action='/manual-valve-command'><input type='hidden' name='command' value='close'><button type='submit'>STOP – CLOSE_ALL</button></form>";
        h += "<form method='post' action='/manual-valve-mode'><input type='hidden' name='mode' value='disable'><button type='submit'>Ukončit nouzový ruční režim</button></form>";
    }
    h += "<small>Nouzový režim lze aktivovat jen při poruše relevantního čidla; při plně funkčním systému není dostupný. Nezadává se požadovaný objem ani časový cíl: ZAPNOUT ponechá vybraný ventil otevřený až do STOP. Obsluha musí průběžně fyzicky sledovat hladiny a sama včas zastavit přítok. Nouzový režim ignoruje měření dolní nádrže, běžné transferové prahy a hysterezi. Pokud je čidlo studny platné, otevření studniční trasy stále respektuje existující povolení ochrany studničního čerpadla; při neplatném čidle se tato čidlová ochrana nedá vyhodnotit a odpovědnost přebírá obsluha. Ochrana horního čerpadla běží místně na ESP32 #2 nezávisle na ručním ventilu; při výpadku samotného ESP32 #2 není dostupná. Každé čerpadlo má samostatné bezpečnostní relé a vlastní nouzové povolení při softwarové poruše své ochrany; nouzové otevření ventilu samo o sobě čerpadlo nepovoluje. Nouzové povolení čerpadla vyžaduje samostatné fyzické potvrzení hladiny v části ochrany příslušného čerpadla. Servisní režim zachovává běžné ventilové ochrany. Reléový výstup musí být inicializovaný; aktivní může být vždy jen jeden směr. Před změnou směru použijte STOP. Po skončení nouzového zásahu ověřte stav čidel před návratem k automatickému provozu.</small></details></section>";
    h += "<section><details><summary>Ochrana čerpadla studny a nouzové povolení</summary>";
    if (wellProtectionConfigConfigured) {
        h += "<p>Transferové minimum: "; h += String(wellProtectionConfig.transferMinimumLevelCm, 1); h += " cm; kritické vypnutí: ";
        h += String(wellProtectionConfig.criticalTripLevelCm, 1); h += " cm; obnova: "; h += String(wellProtectionConfig.recoveryLevelCm, 1); h += " cm.</p>";
    } else h += "<p class='warn'>Bezpečnostní meze nejsou nastaveny nebo neodpovídají využitelné hloubce. Běžný automatický provoz je blokován; po fyzické kontrole lze použít nouzové povolení.</p>";
    h += "<p>Logický stav: ";
    h += pumpProtectionState.pumpPermit ? "PODMÍNĚNĚ POVOLENO" : "BLOKOVÁNO";
    h += "; kritická blokace: "; h += pumpProtectionState.criticalLevelLatched ? "AKTIVNÍ" : "neaktivní";
    h += "; čidlo/konfigurace: "; h += pumpProtectionState.sensorWarning ? "VAROVÁNÍ" : "platné";
    h += "; nouzový režim: "; h += pumpProtectionState.emergencyModeActive ? "AKTIVNÍ" : "vypnutý";
    h += "</p><p>Povel bezpečnostnímu relé: ";
    h += !wellPumpSafetyRelayReady ? "CHYBA / NEPŘIPRAVENO" : (wellPumpSafetyRelayOutput.energized() ? "POVOLIT MOTOR" : "ODPOJIT MOTOR");
    h += "</p><p class='warn'>Nouzový režim po fyzickém ověření hladiny obchází softwarovou ochranu hladiny, její kritickou blokaci i chybu konfigurace/NVS. Obsluha přebírá průběžnou kontrolu hladiny. Neobchází jistič, tepelnou/elektrickou ochranu motoru ani fyzickou připravenost bezpečnostního relé. Po restartu se vypne.</p>";
    if (!pumpProtectionState.emergencyModeActive) {
        h += "<form method='post' action='/pump-emergency'><input type='hidden' name='mode' value='enable'><label><input style='width:auto' type='checkbox' name='physical' value='yes' required> Potvrzuji, že jsem fyzicky zkontroloval hladinu studny a je bezpečná pro provoz čerpadla. Přebírám průběžnou kontrolu hladiny.</label><button type='submit'>NOUZOVĚ POVOLIT STUDNIČNÍ ČERPADLO</button></form>";
    }
    if (pumpProtectionState.emergencyModeActive) h += "<form method='post' action='/pump-emergency'><input type='hidden' name='mode' value='disable'><button type='submit'>Ukončit nouzový režim</button></form>";
    h += "</details></section>";
    h += "<section><details><summary>Ochrana čerpadla dolní nádrže a nouzové povolení</summary>";
    if (lowerPumpConfigConfigured) {
        h += "<p>Vypnutí při hladině: "; h += String(lowerPumpConfig.minimumLevelCm, 1);
        h += " cm; hystereze: "; h += String(lowerPumpConfig.hysteresisCm, 1);
        h += " cm; obnovení při: "; h += String(lowerPumpConfig.minimumLevelCm + lowerPumpConfig.hysteresisCm, 1); h += " cm.</p>";
    } else {
        h += "<p class='warn'>Mez ochrany není nastavena nebo není ověřena. Běžný automatický provoz je blokován; po fyzické kontrole lze použít nouzové povolení.</p>";
    }
    h += "<p>Logický stav: ";
    h += lowerPumpState.pumpPermit ? "PODMÍNĚNĚ POVOLENO" : "BLOKOVÁNO";
    h += "; kritická blokace: "; h += lowerPumpState.criticalLevelLatched ? "AKTIVNÍ" : "neaktivní";
    h += "; čidlo: "; h += lowerPumpState.sensorWarning ? "VAROVÁNÍ" : "platné";
    h += "; startovní ověření: "; h += lowerPumpState.startupValidationRequired ? "ČEKÁ" : "splněno";
    h += "; nouzový režim: "; h += lowerPumpState.emergencyModeActive ? "AKTIVNÍ" : "vypnutý"; h += "</p>";
    h += "<p>Povel bezpečnostnímu relé: ";
    h += !lowerPumpSafetyRelayReady ? "CHYBA / NEPŘIPRAVENO" : (lowerPumpSafetyRelayOutput.energized() ? "POVOLIT MOTOR" : "ODPOJIT MOTOR"); h += "</p>";
    if (lowerPumpState.emergencyModeActive) {
        h += "<form method='post' action='/lower-pump-emergency'><input type='hidden' name='mode' value='disable'><button type='submit'>Ukončit nouzový režim dolního čerpadla</button></form>";
    } else {
        h += "<form method='post' action='/lower-pump-emergency'><input type='hidden' name='mode' value='enable'><label><input style='width:auto' type='checkbox' name='physical' value='yes' required> Potvrzuji, že jsem fyzicky zkontroloval hladinu dolní nádrže a je bezpečná pro provoz čerpadla. Přebírám průběžnou kontrolu hladiny.</label><button type='submit'>NOUZOVĚ POVOLIT ČERPADLO DOLNÍ NÁDRŽE</button></form>";
    }
    h += "<p>Výstup bezpečnostního relé: ";
    h += !lowerPumpSafetyRelayReady ? "CHYBA / NEPŘIPRAVENO" : (lowerPumpSafetyRelayOutput.energized() ? "POVEL POVOLIT MOTOR" : "POVEL ODPOJIT MOTOR"); h += "</p>";
    h += "<p class='warn'>Nouzový režim po fyzickém ověření hladiny obchází softwarovou ochranu hladiny, kritickou blokaci i chybu konfigurace/NVS. Obsluha přebírá průběžnou kontrolu hladiny. Neobchází jistič, tepelnou/elektrickou ochranu motoru ani fyzickou připravenost bezpečnostního relé. Po restartu se vypne.</p>";
    h += "</details></section>";
    h += "<section><h3>Poslední uložené transfery</h3>";
    if (transferHistory.snapshot().count == 0) h += "<p>Zatím bez záznamu.</p>";
    else {
        const HWC::TransferHistorySnapshot& history = transferHistory.snapshot();
        for (uint8_t n = 0; n < history.count && n < 5; ++n) {
            const uint8_t index = history.count < HWC::TRANSFER_HISTORY_CAPACITY
                ? static_cast<uint8_t>(history.count - 1U - n)
                : static_cast<uint8_t>((history.nextIndex + HWC::TRANSFER_HISTORY_CAPACITY - 1U - n) % HWC::TRANSFER_HISTORY_CAPACITY);
            const HWC::TransferHistoryEntry& e = history.entries[index];
            h += "<p>#" + String(e.requestId) + " – ";
            h += e.source == HWC::TRANSFER_SOURCE_WELL ? "studna → dolní" : "horní → dolní";
            h += ", požadavek " + String(e.requestedLiters, 0) + " l, odhad " + String(e.estimatedTransferredLiters, 1) + " l, ";
            h += transferStateText(static_cast<HWC::TransferState>(e.terminalState));
            h += " (";
            h += transferReasonText(static_cast<HWC::TransferEndReason>(e.endReason));
            h += ")</p>";
        }
    }
    h += "<small>Historie ukládá výsledky řízení; objem je odhad ze změny hladiny dolní nádrže, nikoli měření průtokoměrem.</small></section>";
    h += "<section><h3>Horní nádrž / ESP32 #2</h3>";
    h += "<p>ESP32 #2 samostatně hlídá horní čerpadlo proti chodu nasucho a měří teplotu vody. ESP32 #1 řídí ventily a sleduje přítok do dolní nádrže.</p></section>";
    h += "<h2>Konfigurace a servis</h2>";
    h += "<section><h3>Meze ochrany studničního čerpadla</h3>";
    h += "<form method='post' action='/protection-config'><h4>Hladinové meze (cm)</h4><label>Transferové minimum (neodpojuje samo o sobě čerpadlo)</label><input name='transferMin' type='number' min='0' max='2000' step='0.1' required value='";
    if (protectionConfigLoaded) h += String(wellProtectionConfig.transferMinimumLevelCm, 1);
    h += "'><label>Kritická hladina pro ochranu čerpadla</label><input name='criticalTrip' type='number' min='0' max='2000' step='0.1' required value='";
    if (protectionConfigLoaded) h += String(wellProtectionConfig.criticalTripLevelCm, 1);
    h += "'><label>Obnovovací hladina</label><input name='recovery' type='number' min='0' max='2000' step='0.1' required value='";
    if (protectionConfigLoaded) h += String(wellProtectionConfig.recoveryLevelCm, 1);
    h += "'><button type='submit'>Uložit bezpečnostní meze</button></form>";
    h += "</section><section><h3>Meze ochrany čerpadla dolní nádrže</h3>";
    h += "<form method='post' action='/lower-pump-protection-config'><h4>Bezpečnostní meze (cm)</h4>";
    h += "<label>Minimální hladina – vypnutí čerpadla</label><input name='minimum' type='number' min='0' max='121' step='0.1' required value='";
    if (lowerPumpConfigConfigured) h += String(lowerPumpConfig.minimumLevelCm, 1);
    h += "'><label>Hystereze – zvýšení hladiny pro opětovné povolení</label><input name='hysteresis' type='number' min='0.1' max='121' step='0.1' required value='";
    if (lowerPumpConfigConfigured) h += String(lowerPumpConfig.hysteresisCm, 1);
    h += "'><button type='submit'>Uložit ochranu čerpadla dolní nádrže</button></form>";
    h += "<p><small>Pracovní minimum nádrže 11 cm je pouze informativní údaj o běžně využitelné hloubce. Nemá žádný vliv na povolení ani blokování čerpadla. Ochranu řídí výhradně nastavená minimální hladina a hystereze.</small></p>";
    h += "</section>";
    h += "<section><h3>Ochrana horního → dolního transferu: kontrola přítoku</h3>";
    h += "<p>Počáteční čas čekání na první přírůstek a maximální doba bez dalšího přírůstku. Nejde o celkový časový limit transferu.</p>";
    h += "<p>Stav nastavení: <b>"; h += inflowWatchdogSettingsLoaded ? "uloženo a ověřeno v NVS" : (inflowWatchdogSettingsStorageFault ? "CHYBA úložiště – transfer blokován" : "neuloženo – použijte počáteční hodnoty a uložte") ; h += "</b></p>";
    h += "<form method='post' action='/inflow-watchdog-config'><label>Čas na zahájení přítoku (sekundy, 1–3600)</label><input name='startup_seconds' type='number' min='1' max='3600' step='1' required value='" + String(inflowWatchdogSettings.startupGraceSeconds) + "'><label>Maximální doba bez dalšího přítoku (sekundy, 1–3600)</label><input name='no_progress_seconds' type='number' min='1' max='3600' step='1' required value='" + String(inflowWatchdogSettings.noProgressTimeoutSeconds) + "'><button type='submit'>Uložit časové limity</button></form>";
    h += "<small>Počáteční hodnoty 30 s / 30 s jsou pouze orientační, nikoli ověřené provozní hodnoty. Změna se projeví od dalšího transferu. Minimální objemový přírůstek a celkový timeout transferu jsou samostatné parametry.</small></section>";
    h += "<section><h3>Rozměry studny</h3><form method='post' action='/geometry'>";
    h += "<label>Vnitřní průměr (cm)</label><input name='diameter' type='number' min='20' max='500' step='0.1' required value='" + String(wellGeometry.internalDiameterCm,1) + "'>";
    h += "<label>Využitelná hloubka (cm)</label><input name='depth' type='number' min='20' max='2000' step='0.1' required value='" + String(wellGeometry.usableDepthCm,1) + "'>";
    h += "<button type='submit'>Uložit rozměry</button></form><small>Změna rozměrů nemění naměřenou objemovou kalibraci.</small></section>";
    h += "<section><h3>Uložené rozměry a kalibrace</h3>";
    h += "<p>Průměr studny: "; h += String(wellGeometry.internalDiameterCm,1);
    h += " cm; využitelná hloubka: "; h += String(wellGeometry.usableDepthCm,1); h += " cm</p><p>Kalibračních bodů: ";
    h += String(wellCalibration.pointCount); h += "</p>";
    h += "</section>";
    h += "<section><h3>Objemová kalibrace studny</h3><p>Zadejte alespoň dva body, jeden na řádek ve formátu <code>hladina_cm,objem_l</code>, například <code>100,2500</code>. Používejte desetinnou tečku; hladina i objem musí vzestupně růst.</p>";
    h += "<form method='post' action='/calibration'><label>Kalibrační body (max. 32)</label><textarea name='points' rows='8' required placeholder='100,2500&#10;200,5100'></textarea><label><input style='width:auto' type='checkbox' name='absolute' value='yes'> Potvrzuji, že zadané objemy jsou skutečně změřené absolutní objemy v litrech.</label><button type='submit'>Sloučit a uložit kalibraci</button></form>";
    h += "<small>Body mimo nově zadaný interval zůstanou zachovány. Pokud je stávající kalibrace poškozená nebo si body odporují, zápis bude odmítnut.</small></section>";
    h += "<section><h3>Elektrická kalibrace snímačů</h3><p>Mění pouze přepočet hladiny snímače, nikoli objemovou křivku studny. Rozsah 4–20 mA a nominální shunt 150 Ω zůstávají samostatně nastavené.</p>";
    if (wellSensorConfigState == HWC::SENSOR_CONFIG_VALID_STORED) {
        h += "<form method='post' action='/sensor-calibration'><input type='hidden' name='sensor' value='well'><h4>Snímač studny</h4><label>Měřítko (scale, 0,5–1,5)</label><input name='scale' type='number' min='0.5' max='1.5' step='0.001' required value='" + String(wellConfig.calibrationScale, 3) + "'><label>Posun hladiny (cm, −2000 až 2000)</label><input name='offset' type='number' min='-2000' max='2000' step='0.1' required value='" + String(wellConfig.calibrationOffsetCm, 1) + "'><button type='submit'>Uložit kalibraci studny</button></form>";
    } else {
        h += "<h4>Servisní obnova snímače studny</h4><p class='warn'>Použijte pouze po nezávislém fyzickém změření skutečné hladiny. Zadejte ověřené měřítko; posun se vypočítá z aktuálního surového měření a referenční hladiny.</p><form method='post' action='/sensor-recovery'><input type='hidden' name='sensor' value='well'><label>Ověřené měřítko (0,5–1,5)</label><input name='scale' type='number' min='0.5' max='1.5' step='0.001' required><label>Skutečná hladina změřená nezávislou metodou (cm)</label><input name='reference' type='number' min='0' max='2000' step='0.1' required><label><input style='width:auto' type='checkbox' name='physical' value='yes' required> Potvrzuji fyzické změření hladiny nezávislou referencí a správné přiřazení snímače.</label><button type='submit'>Obnovit elektrické nastavení studny</button></form>";
    }
    if (lowerSensorConfigState == HWC::SENSOR_CONFIG_VALID_STORED) {
        h += "<form method='post' action='/sensor-calibration'><input type='hidden' name='sensor' value='lower'><h4>Snímač dolní nádrže</h4><label>Měřítko (scale, 0,5–1,5)</label><input name='scale' type='number' min='0.5' max='1.5' step='0.001' required value='" + String(lowerTankConfig.calibrationScale, 3) + "'><label>Posun hladiny (cm, −2000 až 2000)</label><input name='offset' type='number' min='-2000' max='2000' step='0.1' required value='" + String(lowerTankConfig.calibrationOffsetCm, 1) + "'><button type='submit'>Uložit kalibraci dolní nádrže</button></form>";
    } else {
        h += "<h4>Servisní obnova snímače dolní nádrže</h4><p class='warn'>Použijte pouze po nezávislém fyzickém změření skutečné hladiny. Zadejte ověřené měřítko; posun se vypočítá z aktuálního surového měření a referenční hladiny.</p><form method='post' action='/sensor-recovery'><input type='hidden' name='sensor' value='lower'><label>Ověřené měřítko (0,5–1,5)</label><input name='scale' type='number' min='0.5' max='1.5' step='0.001' required><label>Skutečná hladina změřená nezávislou metodou (cm)</label><input name='reference' type='number' min='0' max='2000' step='0.1' required><label><input style='width:auto' type='checkbox' name='physical' value='yes' required> Potvrzuji fyzické změření hladiny nezávislou referencí a správné přiřazení snímače.</label><button type='submit'>Obnovit elektrické nastavení dolní nádrže</button></form>";
    }
    h += "</section>";
    h += "<section><h3>Síť a verze firmwaru</h3><p>Ethernet W5500: ";
    const EthernetStartup startup = ethernetStartup.load();
    if (startup == EthernetStartup::Ready && ETH.linkUp() && ETH.hasIP()) {
        h += "DHCP adresa přidělena</p><p>IP adresa: <b>" + ETH.localIP().toString() + "</b></p>";
    } else {
        if (startup == EthernetStartup::Pending) h += "Probíhá inicializace W5500";
        else if (startup == EthernetStartup::Failed) h += "Inicializace W5500 selhala";
        else h += ETH.linkUp() ? "Čeká se na DHCP" : "Fyzický link je odpojen";
        h += "</p>";
    }
    h += "<p>Ovládací web je dostupný přes Ethernet i Wi-Fi. REV109 Core 3 – stolní zkouška.</p>";
    h += apMode ? "<p>Servisní AP je zapnutý.</p>" : "<p>Servisní AP je vypnutý.</p>";
    if (apMode) {
        h += "<p>Adresa AP: " + WiFi.softAPIP().toString() + "<br>AP: <b>" + htmlEscape(apName) + "</b></p>";
    }
    h += "</section>";
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
    h += "<section><h3>Systém – aktualizace firmwaru</h3><p class='warn'>Před aktualizací ověřte, že čerpadla právě nečerpají a ventily jsou zavřené. Při restartu ESP32 se bezpečnostní relé krátce odpojí a čerpadlo se zastaví.</p><p>Nahrajte soubor firmwaru <code>.bin</code> určený pro ESP32 #1. Během aktualizace nevypínejte napájení. Nastavení a kalibrace v NVS zůstávají odděleně.</p><form method='post' action='/update' enctype='multipart/form-data'><input type='file' name='firmware' accept='.bin,application/octet-stream' required><button type='submit'>Aktualizovat ESP32 #1</button></form><small>ESP32 #2 má vlastní lokální aktualizační stránku. Pro každou jednotku použijte pouze její odpovídající firmware a funkční OTA partition.</small></section>";
    h += "</body></html>";
    webServer.send(200, "text/html; charset=utf-8", h);
}

void sendResult(const String& message, bool ok) {
    String h = pageStart(ok ? "Hotovo" : "Operace odmítnuta");
    h += "<section><p>" + htmlEscape(message) + "</p><p><a href='/'>Zpět na hlavní stránku</a></p></section></body></html>";
    webServer.send(ok ? 200 : 400, "text/html; charset=utf-8", h);
}

void handleGeometry() {
    if (!requireWebAuth()) return;
    if (!webServer.hasArg("diameter") || !webServer.hasArg("depth")) { sendResult("Chybí rozměr.", false); return; }
    float diameter = 0.0f;
    float depth = 0.0f;
    if (!parseFloatStrict(webServer.arg("diameter"), diameter) ||
        !parseFloatStrict(webServer.arg("depth"), depth)) {
        sendResult("Rozměry musí být platná čísla bez dalších znaků.", false);
        return;
    }
    HWC::WellGeometryConfig candidate = HWC::defaultWellGeometryConfig();
    candidate.internalDiameterCm = diameter; candidate.usableDepthCm = depth;
    HWC::updateWellGeometryChecksum(candidate);
    if (!HWC::isWellGeometryConfigValid(candidate)) { sendResult("Neplatné rozměry. Průměr 20–500 cm, hloubka 20–2000 cm.", false); return; }
    if (!geometryStore.begin() || !geometryStore.save(candidate)) { sendResult("Uložení rozměrů do NVS selhalo.", false); return; }
    HWC::WellGeometryConfig verify = {};
    if (!geometryStore.load(verify) || verify.checksum != candidate.checksum) { sendResult("Kontrolní načtení uložených rozměrů selhalo.", false); return; }
    wellGeometry = verify;
    wellGeometryLoaded = true;
    if (protectionConfigLoaded && protectionStorageWritable) {
        wellProtectionConfigConfigured = HWC::isWellProtectionConfigValid(wellProtectionConfig, wellGeometry.usableDepthCm) &&
            HWC::wellProtectionConfigChecksumMatches(wellProtectionConfig);
    }
    sendResult("Rozměry byly uloženy a znovu ověřeny. Naměřená kalibrace zůstala beze změny.", true);
}

void handleCalibration() {
    if (!requireWebAuth()) return;
    if (!webServer.hasArg("points")) { sendResult("Chybí kalibrační body.", false); return; }
    String input = webServer.arg("points");
    if (input.length() > 1800) { sendResult("Vstup je příliš dlouhý.", false); return; }
    HWC::CalibrationPoint points[HWC::MAX_WELL_CALIBRATION_POINTS];
    size_t count = 0;
    char buffer[1801]; input.toCharArray(buffer, sizeof(buffer));
    char* savePtr = nullptr;
    for (char* line = strtok_r(buffer, "\r\n", &savePtr); line; line = strtok_r(nullptr, "\r\n", &savePtr)) {
        while (*line == ' ' || *line == '\t') ++line;
        if (*line == '\0') continue;
        if (count >= HWC::MAX_WELL_CALIBRATION_POINTS) { sendResult("Překročen maximální počet 32 bodů.", false); return; }
        char* comma = strchr(line, ',');
        if (!comma) { sendResult("Chybný řádek. Použijte hladina_cm,objem_l.", false); return; }
        *comma = '\0'; char* endA = nullptr; char* endB = nullptr;
        const float level = strtof(line, &endA); const float liters = strtof(comma + 1, &endB);
        const bool levelParsed = endA && endA != line;
        const bool litersParsed = endB && endB != comma + 1;
        while (endA && (*endA == ' ' || *endA == '\t')) ++endA;
        while (endB && (*endB == ' ' || *endB == '\t')) ++endB;
        if (!levelParsed || !litersParsed || !endA || *endA || !endB || *endB || !std::isfinite(level) || !std::isfinite(liters)) { sendResult("Nečíselná nebo neplatná hodnota v kalibračním bodě.", false); return; }
        points[count++] = {level, liters};
    }
    if (count < 2) { sendResult("Je nutné zadat alespoň dva body.", false); return; }
    HWC::WellCalibration saved = HWC::emptyWellCalibration();
    if (!wellCalibrationStore.begin()) { sendResult("Úložiště kalibrace není dostupné.", false); return; }
    const bool absoluteConfirmed = webServer.hasArg("absolute") && webServer.arg("absolute") == "yes";
    const HWC::WellCalibrationSaveStatus status = wellCalibrationStore.mergeAndSave(wellGeometry, points, count, absoluteConfirmed, saved);
    if (status != HWC::WELL_CAL_SAVE_OK) {
        String msg = "Kalibrace nebyla uložena. Stav úložiště: "; msg += String(static_cast<unsigned>(status));
        msg += ". Zkontrolujte rostoucí hladiny/objemy, hloubku a potvrzení absolutních objemů.";
        sendResult(msg, false); return;
    }
    wellCalibration = saved;
    sendResult("Kalibrace byla sloučena, uložena a ověřena opětovným načtením. Počet bodů: " + String(saved.pointCount) + ".", true);
}

void handleSensorCalibration() {
    if (!requireWebAuth()) return;
    if (!webServer.hasArg("sensor") || !webServer.hasArg("scale") || !webServer.hasArg("offset")) {
        sendResult("Chybí typ snímače nebo kalibrační hodnota.", false);
        return;
    }
    const String sensor = webServer.arg("sensor");
    const bool isWell = sensor == "well";
    if (!isWell && !(sensor == "lower")) { sendResult("Neznámý snímač.", false); return; }
    const HWC::SensorConfigLoadState currentState = isWell ? wellSensorConfigState : lowerSensorConfigState;
    if (currentState != HWC::SENSOR_CONFIG_VALID_STORED) {
        sendResult("Snímač nemá ověřené elektrické nastavení. Použijte samostatný postup servisní obnovy s fyzickou referencí.", false);
        return;
    }
    float scale = 0.0f;
    float offset = 0.0f;
    if (!parseFloatStrict(webServer.arg("scale"), scale) ||
        !parseFloatStrict(webServer.arg("offset"), offset)) {
        sendResult("Měřítko a posun musí být platná čísla.", false);
        return;
    }
    HWC::SensorConfig candidate = isWell ? wellConfig : lowerTankConfig;
    candidate.calibrationScale = scale;
    candidate.calibrationOffsetCm = offset;
    HWC::updateSensorConfigChecksum(candidate);
    if (!HWC::isSensorConfigValid(candidate) || !HWC::sensorConfigChecksumMatches(candidate)) {
        sendResult("Kalibrace je mimo povolený rozsah.", false);
        return;
    }
    if (!configStore.begin()) { sendResult("Úložiště kalibrace snímačů není dostupné.", false); return; }
    const char* key = isWell ? "well" : "lower";
    if (configStore.hasRecord(key)) {
        HWC::SensorConfig existing = HWC::defaultSensorConfig();
        if (!configStore.load(key, existing)) {
            sendResult("Uložená kalibrace tohoto snímače je poškozená; automatické přepsání je zakázáno.", false);
            return;
        }
    }
    if (!configStore.save(key, candidate)) { sendResult("Zápis kalibrace do NVS selhal.", false); return; }
    HWC::SensorConfig verify = HWC::defaultSensorConfig();
    if (!configStore.load(key, verify) || !HWC::sensorConfigChecksumMatches(verify)) {
        sendResult("Kontrolní načtení kalibrace selhalo.", false);
        return;
    }
    if (isWell) {
        wellConfig = verify;
        wellSensorConfigState = HWC::SENSOR_CONFIG_VALID_STORED;
        if (wellSensor != nullptr) wellSensor->setConfig(verify);
    } else {
        lowerTankConfig = verify;
        lowerSensorConfigState = HWC::SENSOR_CONFIG_VALID_STORED;
        if (lowerTankSensor != nullptr) lowerTankSensor->setConfig(verify);
    }
    sendResult(String("Elektrická kalibrace snímače ") + (isWell ? "studny" : "dolní nádrže") + " byla uložena a ověřena.", true);
}

void handleSensorRecovery() {
    if (!requireWebAuth()) return;
    if (!webServer.hasArg("sensor") || !webServer.hasArg("scale") ||
        !webServer.hasArg("reference") || !webServer.hasArg("physical") ||
        webServer.arg("physical") != "yes") {
        sendResult("Obnova vyžaduje snímač, měřítko, skutečnou referenční hladinu a výslovné potvrzení fyzické kontroly.", false);
        return;
    }
    const String sensor = webServer.arg("sensor");
    const bool isWell = sensor == "well";
    if (!isWell && sensor != "lower") { sendResult("Neznámý snímač.", false); return; }
    HWC::SensorConfigLoadState& state = isWell ? wellSensorConfigState : lowerSensorConfigState;
    if (state == HWC::SENSOR_CONFIG_VALID_STORED) {
        sendResult("Záznam je již platný. Servisní obnova nesmí přepsat platné nastavení; použijte běžnou úpravu kalibrace.", false);
        return;
    }
    if (state == HWC::SENSOR_CONFIG_NVS_UNAVAILABLE || !configStore.begin()) {
        sendResult("NVS není dostupné. Servisní obnova nemůže pokračovat.", false);
        return;
    }
    float scale = 0.0f, reference = 0.0f;
    if (!parseFloatStrict(webServer.arg("scale"), scale) ||
        !parseFloatStrict(webServer.arg("reference"), reference)) {
        sendResult("Měřítko a referenční hladina musí být platná čísla.", false); return;
    }
    HWC::LevelSensorReader* reader = isWell ? wellSensor : lowerTankSensor;
    if (reader == nullptr) { sendResult("Snímač není inicializován.", false); return; }
    const HWC::LevelSensorReading live = reader->read();
    if (!live.adcReadOk || live.result.status != HWC::SENSOR_MATH_OK) {
        sendResult("Aktuální elektrický signál není platný. Obnovu nelze provést.", false); return;
    }
    HWC::SensorConfig candidate = HWC::defaultSensorConfig();
    if (!HWC::buildSensorConfigRecoveryCandidate(scale, live.result.rawLevelCm, reference, candidate)) {
        sendResult("Zadané hodnoty vedou k neplatnému elektrickému nastavení. Zkontrolujte měřítko a referenční hladinu.", false); return;
    }
    const char* key = isWell ? "well" : "lower";
    if (!configStore.recover(key, candidate)) {
        sendResult("Obnova nebyla uložena. Platný záznam nebyl přepsán; při chybě NVS zůstává ochrana blokována.", false); return;
    }
    HWC::SensorConfig verify = {};
    if (!configStore.load(key, verify) || !HWC::sensorConfigChecksumMatches(verify)) {
        sendResult("Kontrolní načtení po obnově selhalo. Ochrana zůstává blokována.", false); return;
    }
    if (isWell) {
        wellConfig = verify;
        if (wellSensor) wellSensor->setConfig(verify);
        // Never let the cached provisional/default-converted sample become trusted
        // merely because the NVS record was repaired. Wait for a new sample produced
        // with the verified configuration before pump or transfer logic can use it.
        haveWellReading = false;
        lastWellVolume = {HWC::WELL_VOLUME_INVALID_LEVEL, 0.0f, false};
    } else {
        lowerTankConfig = verify;
        if (lowerTankSensor) lowerTankSensor->setConfig(verify);
        // Same cache invalidation for the lower-tank pump and transfer destination.
        haveLowerTankReading = false;
        lastLowerTankVolumeValid = false;
    }
    state = HWC::SENSOR_CONFIG_VALID_STORED;
    Serial.printf("SENSOR_RECOVERY %s: reference=%.1f cm raw=%.1f cm scale=%.3f offset=%.1f cm; NVS read-back PASS\n",
                  isWell ? "well" : "lower", reference, live.result.rawLevelCm,
                  verify.calibrationScale, verify.calibrationOffsetCm);
    sendResult(String("Elektrické nastavení snímače ") + (isWell ? "studny" : "dolní nádrže") +
               " bylo obnoveno, zapsáno a ověřeno kontrolním načtením. Zkontrolujte nyní zobrazenou výšku proti fyzické referenci.", true);
}

const char* otaOperationBlockReason() {
    if (pumpProtectionState.emergencyModeActive || lowerPumpState.emergencyModeActive) {
        return "Aktualizace ESP32 #1 je blokována během nouzového režimu některého místního čerpadla.";
    }
    if (transferOutputs.state == HWC::TRANSFER_PRECHECK ||
        transferOutputs.state == HWC::TRANSFER_RUNNING || manualValveControl.active()) {
        return "Aktualizace ESP32 #1 je blokována během transferu nebo aktivního ručního režimu ventilů.";
    }
    return nullptr;
}

void handleUpdateUpload() {
    HTTPUpload& upload = webServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        otaAuthorized = requireWebAuth();
        otaStarted = false;
        otaSucceeded = false;
        otaError = "";
        if (!otaAuthorized) return;
        String filename = upload.filename;
        filename.toLowerCase();
        if (!filename.endsWith(".bin")) {
            otaError = "Odmítnuto: vyberte firmware soubor s příponou .bin určený pro ESP32 #1.";
            Serial.println("OTA #1 rejected: filename is not .bin");
            return;
        }
        const char* blockReason = otaOperationBlockReason();
        if (blockReason != nullptr) {
            otaError = blockReason;
            Serial.printf("OTA #1 rejected: %s\n", blockReason);
            return;
        }
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            otaError = "Nelze zahájit OTA zápis. Ověřte OTA partition a volnou velikost firmwaru.";
            Update.printError(Serial);
            return;
        }
        otaStarted = true;
        Serial.printf("OTA #1 started: %s\n", upload.filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (!otaAuthorized || !otaStarted) return;
        const char* blockReason = otaOperationBlockReason();
        if (blockReason != nullptr) {
            otaError = blockReason;
            Update.abort();
            otaStarted = false;
            Serial.printf("OTA #1 aborted: %s\n", blockReason);
            return;
        }
        const size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
            otaError = "Zápis firmwaru byl neúplný.";
            Update.printError(Serial);
            Update.abort();
            otaStarted = false;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!otaAuthorized || !otaStarted) return;
        const char* blockReason = otaOperationBlockReason();
        if (blockReason != nullptr) {
            otaError = blockReason;
            Update.abort();
            otaStarted = false;
            Serial.printf("OTA #1 aborted before finalization: %s\n", blockReason);
            return;
        }
        if (Update.end(true)) {
            otaSucceeded = true;
            Serial.printf("OTA #1 finished: %u bytes\n", static_cast<unsigned>(upload.totalSize));
        } else {
            otaError = "Firmware nebyl dokončen nebo ověřen.";
            Update.printError(Serial);
            Update.abort();
        }
        otaStarted = false;
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (otaStarted) Update.abort();
        otaStarted = false;
        otaSucceeded = false;
        otaError = "Nahrávání firmwaru bylo přerušeno.";
    }
}

void handleUpdateComplete() {
    if (!requireWebAuth()) return;
    if (otaSucceeded) {
        sendResult("Aktualizace ESP32 #1 byla dokončena. Zařízení se restartuje. "
                   "Za několik sekund klikněte na Zpět na hlavní stránku.", true);
        delay(1000);
        ESP.restart();
        return;
    }
    if (otaError.length() == 0) otaError = "Aktualizace se nezdařila. Firmware nebyl aktivován.";
    sendResult(otaError, false);
}

void handleProtectionConfig() {
    if (!requireWebAuth()) return;
    if (!webServer.hasArg("transferMin") || !webServer.hasArg("criticalTrip") || !webServer.hasArg("recovery")) {
        sendResult("Chybí jedna nebo více hladinových mezí.", false);
        return;
    }
    float transferMin = 0.0f, criticalTrip = 0.0f, recovery = 0.0f;
    if (!parseFloatStrict(webServer.arg("transferMin"), transferMin) ||
        !parseFloatStrict(webServer.arg("criticalTrip"), criticalTrip) ||
        !parseFloatStrict(webServer.arg("recovery"), recovery)) {
        sendResult("Hladinové meze musí být platná čísla.", false);
        return;
    }
    HWC::WellProtectionConfig candidate = wellProtectionConfig;
    candidate.transferMinimumLevelCm = transferMin;
    candidate.criticalTripLevelCm = criticalTrip;
    candidate.recoveryLevelCm = recovery;
    HWC::updateWellProtectionConfigChecksum(candidate);
    if (!HWC::isWellProtectionConfigValid(candidate, wellGeometry.usableDepthCm) ||
        !HWC::wellProtectionConfigChecksumMatches(candidate)) {
        sendResult("Neplatné meze: všechny musí být v rozsahu využitelné hloubky a obnovovací hladina musí být vyšší než kritická.", false);
        return;
    }
    if (!protectionStore.begin()) { sendResult("Úložiště bezpečnostních mezí není dostupné.", false); return; }
    if (protectionStore.hasRecord()) {
        HWC::WellProtectionConfig existing = HWC::emptyWellProtectionConfig();
        bool oldLatch = false, oldSafetyValid = false;
        if (!protectionStore.load(existing, oldLatch, oldSafetyValid)) {
            sendResult("Uložená bezpečnostní konfigurace je poškozená; automatické přepsání je zakázáno.", false);
            return;
        }
    }
    const bool latch = wellPumpProtection != nullptr && wellPumpProtection->criticalLevelLatched();
    if (!protectionStore.save(candidate, latch, true)) {
        sendResult("Zápis nebo kontrolní načtení bezpečnostních mezí selhalo.", false);
        return;
    }
    wellProtectionConfig = candidate;
    protectionConfigLoaded = true;
    wellProtectionConfigConfigured = true;
    protectionStorageWritable = true;
    persistedCriticalLatch = latch;
    sendResult("Hladinové meze byly uloženy a ověřeny. Kritická ochrana a transferové minimum zůstávají oddělené.", true);
}

void handleLowerPumpProtectionConfig() {
    if (!requireWebAuth()) return;
    float minimum = 0.0f, hysteresis = 0.0f;
    if (!webServer.hasArg("minimum") || !webServer.hasArg("hysteresis") ||
        !parseFloatStrict(webServer.arg("minimum"), minimum) ||
        !parseFloatStrict(webServer.arg("hysteresis"), hysteresis)) {
        sendResult("Minimální hladina a hystereze musí být platná čísla.", false); return;
    }
    HWC::LowerTankPumpConfig candidate = HWC::emptyLowerTankPumpConfig();
    candidate.minimumLevelCm = minimum;
    candidate.hysteresisCm = hysteresis;
    HWC::updateLowerTankPumpConfigChecksum(candidate);
    if (!HWC::isLowerTankPumpConfigValid(candidate) || !HWC::lowerTankPumpConfigChecksumMatches(candidate)) {
        sendResult("Neplatné nastavení. Minimum musí být nejméně 0 cm, hystereze větší než 0 cm a jejich součet nesmí překročit 121 cm.", false); return;
    }
    if (!lowerPumpStorageReady || !lowerPumpProtectionStore.begin()) {
        sendResult("Úložiště ochrany čerpadla dolní nádrže není dostupné.", false); return;
    }
    if (lowerPumpProtectionStore.hasRecord()) {
        HWC::LowerTankPumpConfig existing = {};
        bool oldLatch = false, oldValid = false;
        if (!lowerPumpProtectionStore.load(existing, oldLatch, oldValid)) {
            sendResult("Uložená konfigurace ochrany je poškozená; automatické přepsání je zakázáno.", false); return;
        }
    }
    const bool latch = lowerPumpProtection != nullptr && lowerPumpProtection->criticalLevelLatched();
    if (!lowerPumpProtectionStore.save(candidate, latch, true)) {
        sendResult("Zápis nebo kontrolní načtení ochrany čerpadla dolní nádrže selhalo.", false); return;
    }
    lowerPumpConfig = candidate;
    lowerPumpConfigConfigured = true;
    lowerPumpPersistedCriticalLatch = latch;
    sendResult("Mez a hystereze ochrany čerpadla dolní nádrže byly uloženy a ověřeny.", true);
}

void handleLowerPumpEmergency() {
    if (!requireWebAuth()) return;
    const String mode = webServer.arg("mode");
    if (mode == "disable") {
        lowerEmergencyDisableRequested = true;
        lowerEmergencyEnableRequested = false;
        lowerEmergencyConfirmationRequested = false;
        sendResult("Požadavek na ukončení nouzového režimu dolního čerpadla byl přijat.", true);
        return;
    }
    if (mode != "enable" || !webServer.hasArg("physical") || webServer.arg("physical") != "yes") {
        sendResult("Nouzové povolení vyžaduje výslovné potvrzení fyzické kontroly hladiny.", false);
        return;
    }
    lowerEmergencyEnableRequested = true;
    lowerEmergencyConfirmationRequested = true;
    lowerEmergencyDisableRequested = false;
    sendResult("Požadavek na nouzové povolení dolního čerpadla byl přijat. Aktivace proběhne v nejbližším cyklu ochrany; ověřte stav AKTIVNÍ a povel relé v přehledu. Po fyzické kontrole hladiny ji obsluha musí průběžně sledovat.", true);
}

void updateLowerTankPumpProtection(const HWC::LevelSensorReading& reading) {
    if (lowerPumpProtection == nullptr) return;
    const bool sensorValid = HWC::sensorConfigUsableForLevelMeasurement(lowerSensorConfigState) &&
        reading.result.status == HWC::SENSOR_MATH_OK &&
        std::isfinite(reading.result.calibratedLevelCm) && reading.result.calibratedLevelCm >= 0.0f &&
        reading.result.calibratedLevelCm <= HWC::LOWER_PUMP_TANK_MAX_LEVEL_CM;
    lowerPumpState = lowerPumpProtection->update(lowerPumpConfig,
        sensorValid ? reading.result.calibratedLevelCm : 0.0f,
        sensorValid, lowerPumpStorageReady && lowerPumpConfigConfigured,
        lowerEmergencyEnableRequested, lowerEmergencyConfirmationRequested,
        lowerEmergencyDisableRequested);
    lowerEmergencyEnableRequested = false;
    lowerEmergencyConfirmationRequested = false;
    lowerEmergencyDisableRequested = false;
    if (lowerPumpConfigConfigured && lowerPumpStorageReady &&
        lowerPumpState.criticalLevelLatched != lowerPumpPersistedCriticalLatch) {
        if (lowerPumpProtectionStore.saveCriticalLatch(lowerPumpState.criticalLevelLatched)) {
            lowerPumpPersistedCriticalLatch = lowerPumpState.criticalLevelLatched;
        } else {
            lowerPumpStorageReady = false;
            lowerPumpState = lowerPumpProtection->update(lowerPumpConfig,
                sensorValid ? reading.result.calibratedLevelCm : 0.0f,
                sensorValid, false);
            Serial.println("ERROR: lower tank pump safety latch persistence failed; logical pump permit forced OFF.");
        }
    }
}

void handlePumpEmergency() {
    if (!requireWebAuth()) return;
    const String mode = webServer.arg("mode");
    if (mode == "disable") {
        emergencyDisableRequested = true;
        emergencyEnableRequested = false;
        emergencyConfirmationRequested = false;
        sendResult("Požadavek na ukončení nouzového režimu byl přijat.", true);
        return;
    }
    if (!(mode == "enable")) { sendResult("Neznámá nouzová akce.", false); return; }
    if (!webServer.hasArg("physical") || !(webServer.arg("physical") == "yes")) {
        sendResult("Nouzový režim vyžaduje výslovné potvrzení fyzické kontroly hladiny.", false);
        return;
    }
    emergencyEnableRequested = true;
    emergencyConfirmationRequested = true;
    emergencyDisableRequested = false;
    sendResult("Požadavek na nouzové povolení studničního čerpadla byl přijat. Aktivace proběhne v nejbližším cyklu ochrany; ověřte stav AKTIVNÍ a povel relé v přehledu. Po fyzické kontrole hladiny ji obsluha musí průběžně sledovat.", true);
}

void updateWellPumpProtection(const HWC::LevelSensorReading& reading) {
    if (wellPumpProtection == nullptr) return;
    HWC::PumpProtectionInputs inputs = {};
    inputs.sensorValid = HWC::sensorConfigUsableForLevelMeasurement(wellSensorConfigState) &&
        reading.result.status == HWC::SENSOR_MATH_OK &&
        std::isfinite(reading.result.calibratedLevelCm) && reading.result.calibratedLevelCm >= 0.0f &&
        reading.result.calibratedLevelCm <= wellGeometry.usableDepthCm;
    inputs.waterLevelCm = inputs.sensorValid ? reading.result.calibratedLevelCm : 0.0f;
    inputs.criticalTripLevelCm = wellProtectionConfig.criticalTripLevelCm;
    inputs.recoveryLevelCm = wellProtectionConfig.recoveryLevelCm;
    inputs.maximumLevelCm = wellGeometry.usableDepthCm;
    inputs.configurationValid = wellProtectionConfigConfigured &&
        HWC::isWellProtectionConfigValid(wellProtectionConfig, wellGeometry.usableDepthCm) &&
        HWC::wellProtectionConfigChecksumMatches(wellProtectionConfig);
    inputs.otherCriticalFault = !protectionStorageWritable;
    inputs.enableEmergencyCommand = emergencyEnableRequested;
    inputs.emergencyConfirmation = emergencyConfirmationRequested;
    inputs.disableEmergencyCommand = emergencyDisableRequested;
    pumpProtectionState = wellPumpProtection->update(inputs);
    emergencyEnableRequested = false;
    emergencyConfirmationRequested = false;
    emergencyDisableRequested = false;

    if (inputs.configurationValid && protectionStorageWritable &&
        pumpProtectionState.criticalLevelLatched != persistedCriticalLatch) {
        if (protectionStore.saveCriticalLatch(pumpProtectionState.criticalLevelLatched)) {
            persistedCriticalLatch = pumpProtectionState.criticalLevelLatched;
            Serial.printf("Pump critical latch persisted: %s\n", persistedCriticalLatch ? "LATCHED" : "CLEARED");
        } else {
            protectionStorageWritable = false;
            Serial.println("ERROR: could not persist pump critical latch; logical pump permit forced OFF.");
            inputs.otherCriticalFault = true;
            pumpProtectionState = wellPumpProtection->update(inputs);
        }
    }
}

void loadWifi() {
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

void handleWifi() {
    if (!requireWebAuth()) return;
    String ssid = webServer.arg("ssid");
    const String password = webServer.arg("password");
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 64 ||
        (password.length() != 0 && password.length() < 8)) {
        webServer.send(400, "text/plain; charset=utf-8", "Neplatný název nebo heslo Wi-Fi.");
        return;
    }

    HWC::NvsCredentialBackend backend("hwc_wifi");
    if (!backend.begin()) {
        webServer.send(503, "text/plain; charset=utf-8", "Úložiště Wi-Fi není dostupné.");
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
        webServer.send(500, "text/plain; charset=utf-8", "Uložení Wi-Fi údajů nebylo potvrzené.");
        return;
    }
    savedWifiSsid = ssid;
    savedWifiPassword = password;
    wifiReconnectRequested = true;
    sendResult(wifiStaDisabled
        ? "Wi-Fi údaje byly uloženy. Domácí Wi-Fi je vypnutá. Zapneš ji na přehledu."
        : "Wi-Fi údaje byly uloženy. Jednotka zkusí nové připojení. Přes kabel můžeš výsledek zkontrolovat na přehledu.", true);
}

void handleWifiMode() {
    if (!requireWebAuth()) return;
    const String mode = webServer.arg("mode");
    if (mode != "on" && mode != "off") {
        webServer.send(400, "text/plain; charset=utf-8", "Neplatný režim domácí Wi-Fi.");
        return;
    }

    const bool disable = mode == "off";
    Preferences settings;
    if (!settings.begin("hwc_wifi", false)) {
        webServer.send(503, "text/plain; charset=utf-8", "Úložiště Wi-Fi není dostupné.");
        return;
    }
    settings.putBool("sta_off", disable);
    const bool saved = settings.isKey("sta_off") && settings.getBool("sta_off", !disable) == disable;
    settings.end();
    if (!saved) {
        webServer.send(500, "text/plain; charset=utf-8", "Uložení režimu Wi-Fi nebylo potvrzené.");
        return;
    }
    wifiStaDisabled = disable;
    wifiAttemptPending = false;
    wifiReconnectRequested = true;
    sendResult(disable
        ? "Domácí Wi-Fi se vypne. Údaje sítě zůstávají uložené."
        : "Domácí Wi-Fi je povolená. Pokud jsou uložené údaje, zařízení se zkusí připojit.", true);
}

bool prepareConfigApCredentials() {
    const uint64_t mac = ESP.getEfuseMac();
    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned>((mac >> 16) & 0xFFFF));
    apName = String("HomeWater-") + suffix;
    apCredentialStorageReady = false;

    Preferences apPreferences;
    if (!apPreferences.begin("hwc_ap", false)) {
        apPassword = "";
        Serial.println("ERROR: configuration AP disabled; credential NVS is unavailable.");
        return false;
    }

    const bool existed = apPreferences.isKey("password");
    initialConfigApPasswordGenerated = !existed;
    char generated[33] = {0};
    char verified[64] = {0};
    if (!existed) {
        if (!enableCredentialRandomSource() ||
            !HWC::generateHexCredential(generated, sizeof(generated), credentialRandomWordSource, 4U)) {
            apPreferences.end();
            apPassword = "";
            return false;
        }
    }
    HWC::NvsCredentialTextStorage storage(apPreferences);
    const HWC::CredentialStartupResult result = HWC::ensureCredentialText(
        storage, "password", generated, sizeof(generated), 8U, 63U,
        verified, sizeof(verified));
    storage.close();
    apPreferences.end();

    if (result == HWC::CREDENTIAL_STARTUP_READY &&
        HWC::validCredentialText(apName.c_str(), apName.length() + 1U, 8U, 32U)) {
        apPassword = verified;
        apCredentialStorageReady = true;
    } else {
        apPassword = "";
        Serial.printf("ERROR: configuration AP disabled; credential storage state=%u.\n",
                      static_cast<unsigned>(result));
    }
    HWC::clearCredentialText(generated, sizeof(generated));
    HWC::clearCredentialText(verified, sizeof(verified));
    return apCredentialStorageReady;
}

void maintainWifi(uint32_t now) {
    if (!webStarted) return;
    if (wifiStaDisabled) {
        wifiAttemptPending = false;
        if ((WiFi.getMode() & WIFI_MODE_STA) &&
            (wifiReconnectRequested || now - lastWifiAttemptMs >= WIFI_RETRY_INTERVAL_MS)) {
            lastWifiAttemptMs = now;
            if (!WiFi.enableSTA(false)) Serial.println("ESP32 #1 Wi-Fi STA could not be disabled; will retry.");
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
            Serial.println("ESP32 #1 Wi-Fi STA initialization failed; will retry.");
        }
        return;
    }

    if (!wifiAttemptPending || now - lastWifiAttemptMs < 250U) return;
    if (!WiFi.STA.started() || WiFi.STA.connected()) {
        if (now - lastWifiAttemptMs >= 2000U) {
            wifiAttemptPending = false;
            wifiReconnectRequested = true;
            Serial.println("ESP32 #1 Wi-Fi STA not ready for new credentials; will retry.");
        }
        return;
    }

    wifiAttemptPending = false;
    lastWifiAttemptMs = now;
    if (!WiFi.STA.connect(savedWifiSsid.c_str(), savedWifiPassword.c_str())) {
        Serial.println("ESP32 #1 Wi-Fi connection could not start; will retry.");
    }
}

void maintainServiceAp(uint32_t now) {
    if (!webStarted) return;
    const bool wifiReady = WiFi.STA.connected() && WiFi.STA.hasIP();
    const bool ethernetReady = ethernetStartup.load() == EthernetStartup::Ready && ETH.linkUp() && ETH.hasIP();
    if (wifiReady || ethernetReady) {
        lastManagementLinkMs = now;
        if (apMode && WiFi.enableAP(false)) {
            apMode = false;
            Serial.println("ESP32 #1 service AP stopped: management link restored.");
        }
        return;
    }

    if (apMode || now - lastManagementLinkMs < MANAGEMENT_LINK_GRACE_MS ||
        now - lastServiceApAttemptMs < 5000U) return;
    lastServiceApAttemptMs = now;
    if (!apCredentialStorageReady || apPassword.length() < 8 || apPassword.length() > 63 ||
        adminPassword.length() < 12 || adminPassword.length() > 64) {
        Serial.println("ERROR: configuration AP start blocked; credentials are not verified in NVS.");
        return;
    }

    apMode = WiFi.softAP(apName.c_str(), apPassword.c_str());
    if (apMode) {
        Serial.printf("ESP32 #1 local AP started: SSID=%s IP=%s\n",
                      apName.c_str(), WiFi.softAPIP().toString().c_str());
    } else {
        WiFi.enableAP(false);
        Serial.println("ESP32 #1 service AP could not start; will retry.");
    }
}

void initializeEthernet(void*) {
    // The reset and driver startup may wait for hardware. Keep them off the control loop.
    delay(500);
    for (unsigned attempt = 1; attempt <= 3; ++attempt) {
        Serial.printf("W5500 initialization attempt %u/3.\n", attempt);
        pinMode(HwcPins::Esp32_1::W5500_RST, OUTPUT);
        digitalWrite(HwcPins::Esp32_1::W5500_RST, LOW);
        delay(50);
        digitalWrite(HwcPins::Esp32_1::W5500_RST, HIGH);
        delay(200);

        // Firmware controls reset above. INT is unwired; core 3.3.7 polls RX.
        if (ETH.begin(ETH_PHY_W5500, 1, HwcPins::Esp32_1::W5500_CS, -1, -1,
                      SPI3_HOST, HwcPins::Esp32_1::W5500_SCK, HwcPins::Esp32_1::W5500_MISO,
                      HwcPins::Esp32_1::W5500_MOSI, 10) && ETH.netif() != nullptr) {
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

bool prepareLegacyAdminCredential(bool& generatedNew) {
    generatedNew = false;
    Preferences authPreferences;
    if (!authPreferences.begin("hwc_auth", false)) return false;
    const bool adminExisted = authPreferences.isKey("admin");
    char generated[33] = {0};
    char verified[65] = {0};
    if (!adminExisted) {
        if (!enableCredentialRandomSource() ||
            !HWC::generateHexCredential(generated, sizeof(generated), credentialRandomWordSource, 4U)) {
            authPreferences.end();
            adminPassword = "";
            return false;
        }
    }
    HWC::NvsCredentialTextStorage storage(authPreferences);
    const HWC::CredentialStartupResult result = HWC::ensureCredentialText(
        storage, "admin", generated, sizeof(generated), 12U, 64U,
        verified, sizeof(verified));
    storage.close();
    authPreferences.end();
    const bool ready = result == HWC::CREDENTIAL_STARTUP_READY;
    if (ready) {
        adminPassword = verified;
        generatedNew = !adminExisted;
    } else {
        adminPassword = "";
        Serial.printf("ERROR: legacy web administrator credential unavailable; state=%u.\n",
                      static_cast<unsigned>(result));
    }
    HWC::clearCredentialText(generated, sizeof(generated));
    HWC::clearCredentialText(verified, sizeof(verified));
    return ready;
}

bool copyStringToCredentialField(const String& source, char* target, size_t capacity) {
    if (target == nullptr || capacity == 0U || source.length() >= capacity) return false;
    memset(target, 0, capacity);
    if (source.length() != 0U) memcpy(target, source.c_str(), source.length());
    target[source.length()] = '\0';
    return true;
}

bool loadUnifiedCredentials() {
    apCredentialStorageReady = false;
    initialConfigApPasswordGenerated = false;
    HWC::NvsCredentialBackend initialBackend("hwcCred");
    if (!initialBackend.begin()) {
        apName = ""; apPassword = ""; adminPassword = "";
        Serial.println("ERROR: unified credential NVS unavailable; configuration AP disabled.");
        return false;
    }
    HWC::CredentialStore initialStore(initialBackend);
    HWC::Credentials active;
    HWC::CredentialStore::clear(active);
    const HWC::CredentialStoreStatus initialStatus = initialStore.load(active);
    initialBackend.end();

    if (initialStatus == HWC::CREDENTIAL_VALID) {
        apName = active.apName;
        apPassword = active.apPassword;
        adminPassword = active.adminPassword;
        apCredentialStorageReady = true;
        HWC::CredentialStore::clear(active);
        return true;
    }
    HWC::CredentialStore::clear(active);
    if (initialStatus != HWC::CREDENTIAL_EMPTY) {
        apName = ""; apPassword = ""; adminPassword = "";
        Serial.printf("ERROR: unified credential store is not usable (state=%u); legacy fallback is blocked.\n",
                      static_cast<unsigned>(initialStatus));
        return false;
    }

    // First boot after upgrade: read and verify the old split namespaces once.
    bool adminGenerated = false;
    if (!prepareConfigApCredentials() || !prepareLegacyAdminCredential(adminGenerated)) {
        apName = ""; apPassword = ""; adminPassword = "";
        apCredentialStorageReady = false;
        return false;
    }

    HWC::Credentials legacy;
    HWC::CredentialStore::clear(legacy);
    const bool legacyCopied = copyStringToCredentialField(apName, legacy.apName, sizeof(legacy.apName)) &&
                              copyStringToCredentialField(apPassword, legacy.apPassword, sizeof(legacy.apPassword)) &&
                              copyStringToCredentialField(adminPassword, legacy.adminPassword, sizeof(legacy.adminPassword)) &&
                              HWC::CredentialStore::validate(legacy);
    if (!legacyCopied) {
        HWC::CredentialStore::clear(legacy);
        apName = ""; apPassword = ""; adminPassword = "";
        return false;
    }

    HWC::NvsCredentialBackend migrationBackend("hwcCred");
    if (!migrationBackend.begin()) {
        HWC::CredentialStore::clear(legacy);
        apName = ""; apPassword = ""; adminPassword = "";
        Serial.println("ERROR: unified credential NVS could not be reopened; AP disabled.");
        return false;
    }
    HWC::CredentialStore migrationStore(migrationBackend);
    HWC::CredentialStore::clear(active);
    const HWC::CredentialLegacyMigrationResult migration =
        HWC::CredentialLegacyMigrator::loadOrMigrate(migrationStore, legacy, active);
    migrationBackend.end();
    HWC::CredentialStore::clear(legacy);

    const bool ready = (migration == HWC::CREDENTIAL_MIGRATION_DONE ||
                        migration == HWC::CREDENTIAL_MIGRATION_ALREADY_PRESENT) &&
                       HWC::CredentialStore::validate(active);
    if (ready) {
        apName = active.apName;
        apPassword = active.apPassword;
        adminPassword = active.adminPassword;
        apCredentialStorageReady = true;
        if (migration == HWC::CREDENTIAL_MIGRATION_DONE) {
            if (initialConfigApPasswordGenerated)
                Serial.printf("Initial configuration AP password (one-time display): %s\n", apPassword.c_str());
            if (adminGenerated)
                Serial.printf("Initial WEB ADMIN password (one-time display): %s\n", adminPassword.c_str());
        }
    } else {
        apName = ""; apPassword = ""; adminPassword = "";
        apCredentialStorageReady = false;
        Serial.printf("ERROR: legacy credential migration failed (state=%u); AP remains disabled.\n",
                      static_cast<unsigned>(migration));
    }
    HWC::CredentialStore::clear(active);
    return ready;
}

void startWebAndNetwork() {
    if (!Network.begin()) {
        Serial.println("Network initialization failed; local sensing and protection remain active.");
        return;
    }
    startEthernet();
    if (!loadUnifiedCredentials()) {
        Serial.println("Web/AP credential startup gate: CLOSED.");
        return;
    }
    loadWifi();
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);

    static const char* requestHeaders[] = {"Origin", "Host"};
    webServer.collectHeaders(requestHeaders, 2);
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/status", HTTP_GET, handleRoot);
    webServer.on("/geometry", HTTP_POST, handleGeometry);
    webServer.on("/calibration", HTTP_POST, handleCalibration);
    webServer.on("/sensor-calibration", HTTP_POST, handleSensorCalibration);
    webServer.on("/sensor-recovery", HTTP_POST, handleSensorRecovery);
    webServer.on("/protection-config", HTTP_POST, handleProtectionConfig);
    webServer.on("/lower-pump-protection-config", HTTP_POST, handleLowerPumpProtectionConfig);
    webServer.on("/lower-pump-emergency", HTTP_POST, handleLowerPumpEmergency);
    webServer.on("/transfer-start", HTTP_POST, handleTransferStart);
    webServer.on("/transfer-stop", HTTP_POST, handleTransferStop);
    webServer.on("/inflow-watchdog-config", HTTP_POST, handleInflowWatchdogConfig);
    webServer.on("/transfer-recovery-ack", HTTP_POST, handleTransferRecoveryAck);
    webServer.on("/pump-emergency", HTTP_POST, handlePumpEmergency);
    webServer.on("/manual-valve-mode", HTTP_POST, handleManualValveMode);
    webServer.on("/manual-valve-command", HTTP_POST, handleManualValveCommand);
    webServer.on("/wifi", HTTP_POST, handleWifi);
    webServer.on("/wifi-mode", HTTP_POST, handleWifiMode);
    webServer.on("/update", HTTP_POST, handleUpdateComplete, handleUpdateUpload);
    webServer.onNotFound([](){ if (!requireWebAuth()) return; webServer.send(404, "text/plain; charset=utf-8", "Not found"); });
    webServer.begin();
    lastManagementLinkMs = millis();
    lastServiceApAttemptMs = lastManagementLinkMs;
    webStarted = true;
}

void setup() {
    Serial.begin(115200);
    // Configure the known working pump-relay outputs LOW before startup delay or network/NVS work.
    // The separate 5V valve-relay modules retain their own hardware commissioning checks.
    digitalWrite(HwcPins::Esp32_1::WELL_PUMP_SAFETY_RELAY, LOW);
    digitalWrite(HwcPins::Esp32_1::LOWER_TANK_PUMP_SAFETY_RELAY, LOW);
    pinMode(HwcPins::Esp32_1::WELL_PUMP_SAFETY_RELAY, OUTPUT);
    pinMode(HwcPins::Esp32_1::LOWER_TANK_PUMP_SAFETY_RELAY, OUTPUT);
    digitalWrite(HwcPins::Esp32_1::WELL_PUMP_SAFETY_RELAY, LOW);
    digitalWrite(HwcPins::Esp32_1::LOWER_TANK_PUMP_SAFETY_RELAY, LOW);
    wellPumpSafetyRelayReady = wellPumpSafetyRelayOutput.begin(
        HwcPins::Esp32_1::WELL_PUMP_SAFETY_RELAY, HwcPins::Esp32_1::PUMP_RELAY_ACTIVE_HIGH,
        HwcPins::Esp32_1::PUMP_SAFETY_OUTPUTS_ENABLED, writePumpSafetyRelayLevel);
    lowerPumpSafetyRelayReady = lowerPumpSafetyRelayOutput.begin(
        HwcPins::Esp32_1::LOWER_TANK_PUMP_SAFETY_RELAY, HwcPins::Esp32_1::PUMP_RELAY_ACTIVE_HIGH,
        HwcPins::Esp32_1::PUMP_SAFETY_OUTPUTS_ENABLED, writePumpSafetyRelayLevel);
    digitalWrite(HwcPins::Esp32_1::VALVE_1_RELAY, LOW);
    digitalWrite(HwcPins::Esp32_1::VALVE_2_RELAY, LOW);
    pinMode(HwcPins::Esp32_1::VALVE_1_RELAY, OUTPUT);
    pinMode(HwcPins::Esp32_1::VALVE_2_RELAY, OUTPUT);
    digitalWrite(HwcPins::Esp32_1::VALVE_1_RELAY, LOW);
    digitalWrite(HwcPins::Esp32_1::VALVE_2_RELAY, LOW);
    manualValveRelayOutputReady = manualValveRelayOutput.begin(
        HwcPins::Esp32_1::VALVE_1_RELAY,
        HwcPins::Esp32_1::VALVE_2_RELAY,
        HwcPins::Esp32_1::VALVE_RELAY_ACTIVE_HIGH,
        writeValveRelayLevel);
    delay(250);
    Serial.println();
    Serial.println("HOME WATER CONTROLLER - ESP32 #1 / REV109 CORE 3 BENCH");
    Serial.println("DEVELOPMENT BUILD - VALVE RELAYS AND PUMP SAFETY OUTPUTS ENABLED");
    // Relay pins were initialized OFF before the startup delay above.
    Serial.printf("Valve relay driver: %s; well GPIO=%u, upper GPIO=%u, active-high; relay board supply 5V\n",
                  manualValveRelayOutputReady ? "READY" : "FAULT",
                  HwcPins::Esp32_1::VALVE_1_RELAY, HwcPins::Esp32_1::VALVE_2_RELAY);
    Serial.printf("W5500 pin plan: SCK=%u MISO=%u MOSI=%u CS=%u RST=%u\n",
                  HwcPins::Esp32_1::W5500_SCK, HwcPins::Esp32_1::W5500_MISO,
                  HwcPins::Esp32_1::W5500_MOSI, HwcPins::Esp32_1::W5500_CS,
                  HwcPins::Esp32_1::W5500_RST);
    Serial.printf("ADC pin plan: well=%u, lower tank=%u\n",
                  HwcPins::Esp32_1::WELL_LEVEL_ADC,
                  HwcPins::Esp32_1::LOWER_TANK_LEVEL_ADC);

    analogReadResolution(12);
    if (!configStore.begin()) {
        Serial.println("ERROR: NVS Preferences could not be opened; provisional defaults are diagnostic only and sensor-dependent safety is blocked.");
        wellConfig = HWC::defaultSensorConfig();
        lowerTankConfig = HWC::defaultSensorConfig();
        wellSensorConfigState = HWC::SENSOR_CONFIG_NVS_UNAVAILABLE;
        lowerSensorConfigState = HWC::SENSOR_CONFIG_NVS_UNAVAILABLE;
    } else {
        wellSensorConfigState = loadConfig("well", wellConfig);
        lowerSensorConfigState = loadConfig("lower", lowerTankConfig);
    }

    static HWC::LevelSensorReader wellReader(HwcPins::Esp32_1::WELL_LEVEL_ADC, HWC::defaultSensorConfig());
    static HWC::LevelSensorReader lowerReader(HwcPins::Esp32_1::LOWER_TANK_LEVEL_ADC, HWC::defaultSensorConfig());
    wellReader.setConfig(wellConfig);
    lowerReader.setConfig(lowerTankConfig);
    wellReader.begin();
    lowerReader.begin();
    wellSensor = &wellReader;
    lowerTankSensor = &lowerReader;
    if (geometryStore.begin() && geometryStore.load(wellGeometry)) {
        wellGeometryLoaded = true;
        Serial.printf("Well geometry: loaded and checksum valid (internal diameter %.1f cm, usable depth %.1f cm)\n",
                      wellGeometry.internalDiameterCm, wellGeometry.usableDepthCm);
    } else {
        wellGeometry = HWC::defaultWellGeometryConfig();
        wellGeometryLoaded = false;
        Serial.printf("Well geometry: using editable defaults in RAM only (diameter %.1f cm, usable depth %.1f cm); not saved\n",
                      wellGeometry.internalDiameterCm, wellGeometry.usableDepthCm);
    }
    if (wellCalibrationStore.begin() && wellCalibrationStore.load(wellCalibration)) {
        Serial.printf("Well volume calibration: loaded %u measured points; absolute anchor %s\n",
                      static_cast<unsigned>(wellCalibration.pointCount),
                      wellCalibration.absoluteAnchorKnown ? "KNOWN" : "NOT CONFIRMED");
    } else {
        wellCalibration = HWC::emptyWellCalibration();
        Serial.println("Well volume calibration: no valid measured curve stored; using geometry estimate only.");
    }
    Serial.println("Upper-tank limits are configured only on ESP32 #2; ESP32 #1 does not mirror or synchronize them.");
    bool savedCriticalLatch = false;
    bool savedSafetyStateValid = false;
    if (protectionStore.begin()) {
        protectionStorageWritable = true;
        if (protectionStore.hasRecord()) {
            if (protectionStore.load(wellProtectionConfig, savedCriticalLatch, savedSafetyStateValid)) {
                protectionConfigLoaded = true;
                persistedCriticalLatch = savedCriticalLatch;
                wellProtectionConfigConfigured =
                    HWC::isWellProtectionConfigValid(wellProtectionConfig, wellGeometry.usableDepthCm) &&
                    HWC::wellProtectionConfigChecksumMatches(wellProtectionConfig);
                if (!wellGeometryLoaded) Serial.println("WARNING: well geometry is missing/corrupt; using default dimensions for clearly labelled volume estimates and protection-level range checks.");
                if (!wellProtectionConfigConfigured) Serial.println("WARNING: stored well protection thresholds do not match current well depth; pump permit remains blocked.");
            } else {
                protectionStorageWritable = false;
                wellProtectionConfig = HWC::emptyWellProtectionConfig();
                Serial.println("ERROR: stored well protection record is corrupt; automatic overwrite is disabled.");
            }
        } else {
            wellProtectionConfig = HWC::emptyWellProtectionConfig();
            Serial.println("Well pump protection thresholds are not configured; logical pump permit remains blocked.");
        }
    } else {
        protectionStorageWritable = false;
        Serial.println("ERROR: well protection NVS unavailable; logical pump permit remains blocked.");
    }
    wellPumpProtection = new HWC::WellPumpProtection(savedCriticalLatch, savedSafetyStateValid);

    bool lowerSavedLatch = false, lowerSavedStateValid = false;
    if (inflowWatchdogStore.begin()) {
        if (inflowWatchdogStore.hasRecord()) {
            if (inflowWatchdogStore.load(inflowWatchdogSettings)) {
                inflowWatchdogSettingsLoaded = true;
                Serial.printf("Lower-tank inflow watchdog settings loaded: startup=%lu s, no-progress=%lu s\n", static_cast<unsigned long>(inflowWatchdogSettings.startupGraceSeconds), static_cast<unsigned long>(inflowWatchdogSettings.noProgressTimeoutSeconds));
            } else {
                inflowWatchdogSettingsStorageFault = true;
                inflowWatchdogSettingsLoaded = false;
                Serial.println("ERROR: lower-tank inflow watchdog settings are corrupt; upper-to-lower automatic transfer must remain blocked.");
            }
        } else {
            inflowWatchdogSettings = HWC::defaultLowerTankInflowWatchdogSettings();
            inflowWatchdogSettingsLoaded = false;
            Serial.println("Lower-tank inflow watchdog settings not saved yet; initial values are 30 s / 30 s and are not verified for operation.");
        }
    } else {
        inflowWatchdogSettingsStorageFault = true;
        Serial.println("ERROR: lower-tank inflow watchdog NVS unavailable; upper-to-lower automatic transfer must remain blocked.");
    }

    if (lowerPumpProtectionStore.begin()) {
        lowerPumpStorageReady = true;
        if (lowerPumpProtectionStore.hasRecord()) {
            if (lowerPumpProtectionStore.load(lowerPumpConfig, lowerSavedLatch, lowerSavedStateValid)) {
                lowerPumpConfigConfigured = HWC::isLowerTankPumpConfigValid(lowerPumpConfig) &&
                    HWC::lowerTankPumpConfigChecksumMatches(lowerPumpConfig);
                lowerPumpPersistedCriticalLatch = lowerSavedLatch;
                if (!lowerPumpConfigConfigured) {
                    lowerPumpStorageReady = false;
                    Serial.println("WARNING: lower tank pump protection config invalid; logical pump permit remains blocked.");
                }
            } else {
                lowerPumpStorageReady = false;
                Serial.println("ERROR: lower tank pump protection record corrupt; automatic overwrite disabled.");
            }
        } else {
            Serial.println("Lower tank pump protection thresholds not configured; logical pump permit remains blocked.");
        }
    } else {
        lowerPumpStorageReady = false;
        Serial.println("ERROR: lower tank pump protection NVS unavailable; logical pump permit remains blocked.");
    }
    lowerPumpProtection = new HWC::LowerTankPumpProtection(lowerSavedLatch, lowerSavedStateValid);

    if (inflowStore.begin()) {
        inflowStorageWritable = true;
        if (inflowStore.hasRecord()) {
            HWC::DailyInflowSnapshot snapshot = {};
            if (inflowStore.load(snapshot)) {
                // Do not attribute the unobserved interval across a reboot to daily inflow.
                snapshot.baselineValid = false;
                snapshot.previousDateValid = false;
                snapshot.previousVolumeLiters = 0.0f;
                if (inflowTracker.restoreSnapshot(snapshot)) {
                    Serial.println("Daily inflow history: loaded 30-day record; measurement baseline reset after reboot.");
                } else {
                    inflowStorageWritable = false;
                    Serial.println("ERROR: daily inflow history invalid; preserving existing record and refusing writes.");
                }
            } else {
                inflowStorageWritable = false;
                Serial.println("ERROR: daily inflow history checksum/format invalid; preserving existing record and refusing writes.");
            }
        } else {
            Serial.println("Daily inflow history: no record yet; a new record will be created.");
        }
    } else {
        inflowStorageWritable = false;
        Serial.println("ERROR: daily inflow NVS unavailable; history will be RAM-only.");
    }
    inflowPersistence = HWC::defaultInflowPersistencePolicy();
    transferHistoryStorageReady = transferHistoryStore.begin();
    if (transferHistoryStorageReady) {
        transferHistoryLoadChecked = true;
        HWC::TransferHistorySnapshot savedTransferHistory = {};
        if (transferHistoryStore.hasRecord()) {
            if (transferHistoryStore.load(savedTransferHistory) && transferHistory.restore(savedTransferHistory)) {
                restoreTransferRequestReplayGuard(savedTransferHistory);
                Serial.printf("Transfer history loaded: %u entries (control records; volume is level-derived estimate).\n", static_cast<unsigned>(savedTransferHistory.count));
            }
            else {
                transferHistoryStorageFault = true;
                transferHistoryStorageReady = false;
                transferHistoryStore.end();
                Serial.println("ERROR: transfer history record is corrupt; automatic overwrite disabled.");
            }
        } else {
            Serial.println("Transfer history: no record yet; first result will create it.");
        }
    } else {
        Serial.println("WARNING: transfer history NVS unavailable; will retry in 30 seconds.");
    }
    transferJournalStorageReady = transferJournalStore.begin();
    if (transferJournalStorageReady) {
        if (!transferRestartGuard.restore(transferJournalStore)) {
            Serial.println("ERROR: transfer restart journal is corrupt/unavailable; all new transfer requests are blocked.");
        } else if (transferRestartGuard.state() == HWC::TRANSFER_RESTART_INTERRUPTED ||
                   transferRestartGuard.state() == HWC::TRANSFER_RESTART_RESULT_PENDING) {
            Serial.printf("WARNING: transfer %lu was not cleanly acknowledged before reboot; all new transfers remain blocked until operator review.\n",
                          static_cast<unsigned long>(transferRestartGuard.record().requestId));
        } else {
            Serial.println("Transfer restart journal restored; no interrupted transfer is pending.");
        }
    } else {
        Serial.println("ERROR: transfer restart journal NVS unavailable; all new transfer requests are blocked.");
    }
    Serial.println("NOTE: geometric well volume is an APPROXIMATE estimate until measured volume calibration is entered.");
    Serial.println("NOTE: changing geometry does not overwrite measured calibration points.");
    Serial.println("NOTE: sensor calibration defaults are provisional until installed sensors are calibrated.");
    startWebAndNetwork();
}

void loop() {
    if (webStarted) webServer.handleClient();
    const uint32_t now = millis();
    maintainWifi(now);
    maintainServiceAp(now);
    reportEthernetAddress();
    configureTimeSyncIfNeeded();
    // The two controllers are independent; peer telemetry is not polled.
    if (static_cast<uint32_t>(now - lastReportMs) >= 2000U) {
        lastReportMs = now;
        const HWC::LevelSensorReading well = wellSensor->read();
        lastWellReading = well;
        haveWellReading = true;
        lastWellReadingMs = now;
        updateWellPumpProtection(well);
        printReading("WELL", well);
        observeWellInflow(well, now);
        if (well.result.status == HWC::SENSOR_MATH_OK) {
            const HWC::WellVolumeResult volume = HWC::estimateWellVolume(
                wellGeometry, well.result.calibratedLevelCm, wellCalibration);
            lastWellVolume = volume;
            if (volume.status == HWC::WELL_VOLUME_GEOMETRY_ESTIMATE) {
                Serial.printf("WELL_VOLUME: APPROXIMATE geometry estimate %.1f L; absolute anchor %s\n",
                              volume.liters, volume.absoluteVolumeKnown ? "known" : "not confirmed");
            } else if (volume.status == HWC::WELL_VOLUME_CALIBRATED_EXACT ||
                       volume.status == HWC::WELL_VOLUME_CALIBRATED_INTERPOLATED) {
                Serial.printf("WELL_VOLUME: measured calibration %.1f L (%s)\n", volume.liters,
                              volume.status == HWC::WELL_VOLUME_CALIBRATED_EXACT ? "exact point" : "interpolated");
            } else {
                Serial.printf("WELL_VOLUME: unavailable, status=%u\n", static_cast<unsigned>(volume.status));
            }
        } else {
            lastWellVolume = {HWC::WELL_VOLUME_INVALID_LEVEL, 0.0f, false};
        }
        const HWC::LevelSensorReading lower = lowerTankSensor->read();
        lastLowerTankReading = lower;
        haveLowerTankReading = true;
        lastLowerTankReadingMs = now;
        lastLowerTankVolumeValid = lower.result.status == HWC::SENSOR_MATH_OK &&
            HWC::lowerTankVolumeLiters(lower.result.calibratedLevelCm, lastLowerTankVolumeLiters);
        updateLowerTankPumpProtection(lower);
        printReading("LOWER_TANK", lower);
    }
    updateTransferControl(millis());
    updateManualValveSafety(millis());
    syncManualValveRelayOutput();
    syncPumpSafetyRelayOutputs(millis());
    delay(10);
}
