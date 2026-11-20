# HOME WATER CONTROLLER DEV KIT 1.00 — AUDIT DRAFT REV11

**Status:** audit working copy; not approved for deployment.

## Changes from REV10

- Added `docs/TRANSFER_UPPER_SOURCE_GATE_AUDIT_REV10.md` documenting the mismatch between the upper-tank valve hysteresis indicator and the transfer precheck.
- Added the unresolved semantic decision for `pumpProtectionReady`; it must not be conflated with valve/transfer permission until the actual hydraulic role is confirmed.
- Retained the OTA audit from REV10.
- No firmware source, GPIO mapping, NVS layout, partition table, eFuse setting, relay behavior, transfer logic, or hardware output was changed.

## Verification

The unchanged REV9 firmware/tests were rerun while preparing REV10: standard native C++ **59/59 PASS**, ASan/UBSan **59/59 PASS**, all shell integration/static checks **PASS**, Python **17/17 PASS**, Python `compileall` **PASS**. REV10/REV11 add documentation only; no source or tests were changed.

Arduino-ESP32 target build, physical OTA, real NVS, W5500, sensors, relays, valves and pump tests remain unverified.

**Do not deploy this audit draft to live pump/valve control.**
