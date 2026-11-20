# HOME WATER CONTROLLER – REV40

## Safety clarification

- A missing persisted electrical sensor configuration is no longer treated as usable for pump/transfer safety decisions.
- Provisional default parameters are diagnostic only. A sensor's measured height becomes safety-usable only after its electrical conversion configuration is saved and successfully read back/validated.
- The rule applies to the well and lower-tank sensors on ESP32 #1 and the upper-tank sensor on ESP32 #2.
- Corrupt or unavailable NVS states remain blocked. Emergency pump modes retain their separately approved explicit-confirmation behavior.
- `WellCalibration` remains a separate height-to-litres curve and is not an input to pump or transfer protection.

## Verification

Host-side tests and source integration checks are required. Arduino target compilation and physical ESP32/sensor/pump tests are not performed by this revision.
