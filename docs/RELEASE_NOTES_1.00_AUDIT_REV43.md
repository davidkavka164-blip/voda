# Release notes – REV43

- Audited the protection re-arm sequence after service recovery of missing/corrupt electrical sensor configuration.
- Fixed a stale-sample trust window: ESP32 #1 now invalidates the cached well/lower readings (and volume display validity) after verified recovery; ESP32 #2 invalidates the cached upper reading.
- The sensor configuration is marked trusted only after NVS read-back; no pre-recovery cached sample can then be consumed by transfer or pump logic as a fresh calibrated measurement.
- Existing startup/recovery hysteresis remains: each pump protection must receive a fresh valid reading at/above its configured recovery level before normal permit can return.
- Added native and static integration tests for all three pump protection paths.
- No changes to emergency mode semantics, protection thresholds, well volume calibration, or transfer policy.
