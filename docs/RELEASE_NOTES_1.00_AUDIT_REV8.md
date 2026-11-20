# HOME WATER CONTROLLER DEV KIT 1.00 — AUDIT DRAFT REV8

**Status:** audit working copy; not approved for deployment.

## Changes from REV7

- Corrected the `NvsPeerReplayStore.cpp` mirror mismatch: the common implementation,
  ESP32 #1 sketch copy and ESP32 #2 sketch copy are now byte-identical.
- Added `tests/check_common_module_sync.sh`, which checks every file in
  `firmware/common/` against both sketch source trees.
- Added the full mirror check to the standard and ASan/UBSan test runners.
- Recorded the finding and correction in `REV7_SOURCE_AUDIT_FINDINGS.md`.

## Verification status

- REV7 before correction: 59/59 native C++ test programs passed.
- REV7 before correction: 59/59 ASan/UBSan test programs passed.
- REV7 before correction: Python provisioning client 17/17 passed.
- Post-correction standard native suite: **59/59 PASS**; all integration checks PASS.
- Post-correction ASan/UBSan suite: **59/59 PASS**; all integration checks PASS.
- Python provisioning-client suite: **17/17 PASS**.
- Python `compileall`: PASS.
- Full mirror check: every file in `firmware/common/` is byte-identical in both sketch source trees.
- Arduino-ESP32 target build, real ESP32 NVS, physical power-cut tests and
  hardware tests remain unverified.

**Do not deploy this audit draft to live pump/valve control.**
