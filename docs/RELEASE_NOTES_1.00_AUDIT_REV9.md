# HOME WATER CONTROLLER DEV KIT 1.00 — AUDIT DRAFT REV9

**Status:** audit working copy; not approved for deployment.

## Changes from REV8

- Added `docs/PEER_COMMUNICATION_AUDIT_REV8.md` with a source-level audit of the ESP32 #1/#2 UDP status path, security module call-sites, transfer-simulation inputs, and remaining hardware/network gates.
- No firmware source, GPIO mapping, relay behavior, transfer behavior, or hardware output was changed.
- Corrected the README revision header to identify this package as REV9 and linked the new audit report.

## Key findings

- Current peer status is UDP broadcast with CRC32, not cryptographic authentication.
- Handshake/session/replay modules are not integrated into either `.ino`; no production peer-secret backend is wired into the sketches.
- Upper-tank data is currently used only by dashboard/simulation; no physical valve or pump output is enabled.
- Stale/lost peer data is detected and causes the upper-source transfer simulation to stop with a communication fault.
- Upper volume and temperature are explicitly marked invalid/not yet integrated.
- The role of the reported `pumpProtectionReady` flag in any future upper-tank transfer precheck needs an explicit design decision; it is currently diagnostic only.

## Verification

The unchanged REV8 code was re-run before producing this documentation-only revision:

- Standard native C++ test programs: **59/59 PASS**.
- ASan/UBSan native test programs: **59/59 PASS**.
- Integration/static checks, including full common-module mirror check: **PASS**.
- Python provisioning client: **17/17 PASS**.
- Python `compileall`: **PASS**.
- Arduino-ESP32 target build, real NVS, physical network and hardware tests remain unverified.

**Do not deploy this audit draft to live pump/valve control.**
