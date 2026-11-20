# HOME WATER CONTROLLER – audit revision REV44

Date: 2026-11-13

## Sensor recovery power-interruption audit

- Reviewed NVS replacement semantics and the three service-recovery handlers.
- Confirmed the runtime trust state is promoted only after successful write and read-back validation.
- Confirmed cached readings are invalidated before the trusted state transition.
- Confirmed startup classifies missing, invalid, or unavailable sensor configuration as non-trusted; sensor-dependent ordinary protections remain blocked.
- Added deterministic native fault-injection coverage for write failure before storage change, partial blob write, and complete write with reported API failure.
- Added `docs/SENSOR_RECOVERY_POWER_CUT_AUDIT_REV44.md`.

No production firmware code was changed in REV44 because the existing NVS write/read-back and fail-closed state ordering passed this audit.

## Validation limits

Native fault-injection tests model storage outcomes and are not physical flash power-cut tests. Arduino target compilation and ESP32 bench/power-interruption testing remain outstanding.
