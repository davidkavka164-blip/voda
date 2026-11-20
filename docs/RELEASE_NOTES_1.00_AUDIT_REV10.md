# HOME WATER CONTROLLER DEV KIT 1.00 — AUDIT DRAFT REV10

**Status:** audit working copy; not approved for deployment.

## Changes from REV9

- Added `docs/OTA_INTEGRATION_AUDIT_REV9.md`, based on static inspection of the actual ESP32 #1 and ESP32 #2 OTA upload handlers and the MASTER 1.3 requirements.
- No firmware source, GPIO mapping, NVS layout, partition table, eFuse setting, relay behavior, transfer behavior, or hardware output was changed.
- README revision and audit index updated.

## Key findings

- OTA images are not explicitly bound to ESP32 role #1/#2 by a signed manifest.
- HTTP Basic Auth is used over plain HTTP; it does not protect credentials or image transport against LAN interception.
- ESP32 #2 blocks OTA when `pumpPermit` is true, which usually represents a healthy normal-protection state rather than actual pump operation. This policy needs redesign before deployment.
- ESP32 #1 has no transfer-state interlock in its OTA handler. This is currently simulation-only, but is a hard gate before physical valve activation.
- The two OTA paths are independent and do not coordinate versions, peer compatibility, restart verification, or rollback.
- NVS preservation and boot rollback remain unverified on the target board/core/partition table.

## Verification

- Standard native C++ tests: **59/59 PASS**.
- ASan/UBSan native C++ tests: **59/59 PASS**.
- Shell integration/static checks: **all PASS**.
- Python provisioning client tests: **17/17 PASS**.
- Python `compileall`: **PASS**.
- Arduino-ESP32 target build, OTA on physical devices, NVS retention, boot rollback, W5500 and hardware tests remain unverified.

**Do not deploy this audit draft to live pump/valve control.**
