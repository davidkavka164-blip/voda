# Sensor recovery and protection re-arm audit – REV43

## Scope

This audit checks what happens immediately after a missing/corrupt electrical sensor configuration is restored through the authenticated service-recovery endpoint, especially whether a cached reading converted with provisional/default parameters can be treated as trusted before a new acquisition.

## Finding

REV42 correctly writes and reads back the recovered `SensorConfig`, and all three pump protection state machines require a valid reading at or above their configured recovery threshold before clearing the startup-validation gate. However, the recovery handlers changed the sensor configuration trust state to `SENSOR_CONFIG_VALID_STORED` while the most recently cached reading could still have been acquired using provisional/default conversion parameters.

On ESP32 #1, transfer control runs on every loop, not only when a new sensor sample is acquired. Therefore the cached reading could have been considered valid for up to the normal sample interval after the trust-state transition. The pump protection state machines remained fail-closed until their next update, but transfer logic could inspect that stale cached sample.

## REV43 correction

After successful NVS write and read-back verification, each recovery handler now:

- applies the verified configuration to the relevant `LevelSensorReader`;
- invalidates the cached reading for that sensor before marking the configuration trusted;
- invalidates the corresponding cached volume display where applicable;
- leaves the pump protection state machine to re-evaluate only after the next scheduled acquisition using the verified configuration.

Specific caches:

- ESP32 #1 well: `haveWellReading = false`, well volume result reset to invalid;
- ESP32 #1 lower tank: `haveLowerTankReading = false`, lower volume-valid flag reset;
- ESP32 #2 upper tank: `haveUpperReading = false`, so the local pump relay remains freshness-gated and no old peer reading is advertised as fresh.

The normal pump protections still require a fresh, valid reading at or above their configured recovery level before clearing startup validation. A reading below the recovery level remains blocked. Critical low latches, configuration/NVS faults, relay readiness, and emergency-mode rules are unchanged.

## Test coverage

- Added native test `test_sensor_recovery_protection_gate.cpp` covering missing, corrupt, and unavailable electrical configuration states across all three pump protection state machines.
- The test also verifies that after transition to a stored/verified configuration, each protection remains blocked below its recovery threshold and only permits after a valid reading reaches the threshold.
- Added static integration test `check_sensor_recovery_fresh_sample_gate.sh` to verify all three recovery handlers invalidate cached samples before marking the configuration trusted.

## Limitations

This is a host/native and source-integration audit. It does not replace Arduino target compilation, bench testing with the actual 4–20 mA sensors, relay observation, or physical verification of the service reference procedure.
