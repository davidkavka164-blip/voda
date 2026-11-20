# HOME WATER CONTROLLER – audit draft REV37

**Date:** 2026-11-13  
**Base:** REV36

## Scope

Documentation/audit-only revision covering startup handling of sensor calibration records.

## Finding

The sensor calibration store rejects corrupt records, but both ESP32 startup paths fall back to valid provisional defaults when a record is missing, invalid, or NVS cannot be opened. Pump-protection and transfer consumers currently use sensor-math validity without a separate calibration-trust state. A corrupt customized calibration can therefore be replaced in RAM by defaults while the resulting level may still be accepted as valid.

## Decision

No firmware behavior was changed in REV37. A partial correction would be unsafe because all three sensor consumers and their dependent pump, transfer and peer-status paths must be updated consistently. The proposed state model and required tests are documented in `SENSOR_CALIBRATION_STARTUP_AUDIT_REV37.md`.

## Verification

- Native tests: 65/65 PASS.
- Python tests: 17/17 PASS.
- Python compileall: PASS.
- Shell syntax checks: PASS.
- No Arduino-ESP32 target build or physical NVS/power-cut test was performed.

## Release status

REV37 is an audit draft, not a production release. The calibration startup-state finding remains open. Do not deploy based on this documentation-only revision.
