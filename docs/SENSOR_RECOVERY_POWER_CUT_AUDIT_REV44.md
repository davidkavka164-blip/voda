# Sensor recovery power-cut audit – REV44

Date: 2026-11-13

## Scope

Review of the three electrical sensor configuration recovery paths (well and lower tank on ESP32 #1; upper tank on ESP32 #2) for power interruption or NVS write failure during replacement of a missing/corrupt `SensorConfig` record.

## Findings

- `NvsSensorConfigStore::recover()` refuses to overwrite a record that validates successfully.
- It writes the replacement as one NVS blob using `Preferences::putBytes()`.
- `Preferences::putBytes()` calls `nvs_set_blob()` followed by `nvs_commit()` and returns zero on either error; a non-zero result is required by the recovery store.
- The store then reads the entire record back and validates its exact size, fields, and checksum before reporting success.
- Both firmware recovery handlers leave the sensor state untrusted on a failed write/read-back. They only set `SENSOR_CONFIG_VALID_STORED` after successful read-back. ESP32 #1 and #2 also invalidate the cached pre-recovery reading before switching the trust state.
- At boot, missing, malformed, checksum-invalid, or unavailable NVS states are not converted into `SENSOR_CONFIG_VALID_STORED`; ordinary sensor-dependent protection remains blocked.

Espressif's ESP-IDF v4.4 NVS documentation states that NVS is designed to recover from power interruption and that the new key-value pair being written at the instant of power loss may be lost. For this recovery use case, the previous record is already known to be missing or invalid. Therefore a power cut during the replacement can leave either the old invalid/missing state or a complete new record; firmware trusts only a complete record that passes its validation on boot. If NVS itself cannot initialize after unstable power, the firmware remains in `NVS_UNAVAILABLE` and blocks sensor-dependent normal operation.

## REV44 test additions

Added a dedicated Preferences stub with deterministic fault modes and a new native test covering:

1. write fails before changing storage; after simulated restart the corrupt record is still rejected;
2. short/partial blob write; after simulated restart the record fails exact-size/checksum validation;
3. complete blob reaches storage but the write API reports failure; the active handler reports failure and remains blocked, while a later boot may load the complete record only if it passes the full validation.

The third case models the important distinction between a failed API result and uncertain persistence. It does not claim to emulate flash physics or the ESP-IDF NVS implementation.

## Result and limitations

No production firmware logic change was necessary for this audit. Existing fail-closed ordering is correct. The new native test validates the store's response to modeled failure outcomes; it is not a physical power-cut test and does not replace target testing with the exact Arduino-ESP32/ESP-IDF version installed by the user.

Still required before deployment:

- identify the exact Arduino-ESP32 core version used for compilation;
- compile both ESP32 sketches against that core;
- perform a controlled bench test by interrupting power during a service recovery, then verify after reboot that no invalid record enables ordinary pump protection;
- repeat for all three sensor keys and verify all pump relays remain de-energized until valid fresh level readings satisfy the configured recovery thresholds.
