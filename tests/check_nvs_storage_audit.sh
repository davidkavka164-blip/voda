#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
ESP1="$ROOT/firmware/ESP32_1/ESP32_1.ino"
ESP2="$ROOT/firmware/ESP32_2/ESP32_2.ino"

# Electrical calibration is never promoted from diagnostic defaults to trusted data.
grep -Fq 'SENSOR_CONFIG_MISSING_PROVISIONAL' "$ESP1"
grep -Fq 'SENSOR_CONFIG_INVALID_PRESENT' "$ESP1"
grep -Fq 'SENSOR_CONFIG_NVS_UNAVAILABLE' "$ESP1"
grep -Fq 'SENSOR_CONFIG_MISSING_PROVISIONAL' "$ESP2"
grep -Fq 'SENSOR_CONFIG_INVALID_PRESENT' "$ESP2"

# Watchdog configuration faults and unverified defaults must block upper-to-lower transfer.
grep -Fq 'inflowWatchdogSettingsStorageFault = true;' "$ESP1"
grep -Fq 'inflowWatchdogSettingsLoaded = false;' "$ESP1"
grep -Fq 'in.inflowWatchdogConfigValid = inflowWatchdogSettingsLoaded && !inflowWatchdogSettingsStorageFault;' "$ESP1"

# Pump safety stores must remain independent and fail closed on unavailable/corrupt state.
grep -Fq 'wellProtectionConfigConfigured = false;' "$ESP1"
grep -Fq 'lowerPumpStorageReady = false;' "$ESP1"
grep -Fq 'upperProtectionConfigValid = false;' "$ESP2"
grep -Fq 'upperSafetyStorageReady = false;' "$ESP2"

# Restart journal failure must block new transfer requests.
grep -Fq 'transferRestartGuard.canStart()' "$ESP1"
grep -Fq 'all new transfer requests are blocked' "$ESP1"

# Upper volume calibration remains informational and fail-closed on corrupt storage.
grep -Fq 'upperVolumeCalibrationCorrupt = !upperVolumeCalibrationValid;' "$ESP2"
grep -Fq 'upperVolumeCalibrationCorrupt' "$ESP2"

# REV78 transactional upper limits store must retain all four journal components.
LIMITS="$ROOT/firmware/common/NvsUpperTankLimitsStore.cpp"
for key in '"slot0"' '"slot1"' '"head"' '"txn"'; do grep -Fq "$key" "$LIMITS"; done

# REV84: home Wi-Fi STA credentials use a two-slot commit marker and exact read-back.
grep -Fq 'WifiStaCredentialStore store(backend)' "$ESP1"
grep -Fq '"sta_active"' "$ROOT/firmware/common/WifiStaCredentialStore.h"
grep -Fq 'backend_.get("sta_active", verify' "$ROOT/firmware/common/WifiStaCredentialStore.h"

# REV81: informational history/statistics stores verify exact persisted content.
grep -Fq 'return load(verify)&&sameSnapshot(verify,s);' "$ROOT/firmware/common/NvsTransferHistoryStore.cpp"
grep -Fq 'return load(verify) && sameSnapshot(verify, snapshot);' "$ROOT/firmware/common/NvsDailyInflowStore.cpp"
grep -Fq 'assert(!store.save(second));' "$ROOT/tests/test_nvs_transfer_history_store.cpp"
# REV82: well geometry and volume calibration compare persisted semantic fields.
grep -Fq 'sameCalibration(verify, calibration)' "$ROOT/firmware/common/NvsWellCalibrationStore.cpp"
grep -Fq 'sameCalibration(verified, candidate)' "$ROOT/firmware/common/NvsWellCalibrationStore.cpp"
grep -Fq 'verify.internalDiameterCm == config.internalDiameterCm' "$ROOT/firmware/common/NvsWellGeometryStore.cpp"
grep -Fq 'Preferences::ignoreNextWrite = true;' "$ROOT/tests/test_nvs_well_calibration_store.cpp"
grep -Fq 'Preferences::ignoreNextWrite = true;' "$ROOT/tests/test_nvs_well_geometry_store.cpp"

echo 'PASS: NVS storage safety gates and recovery boundaries'
