# REV7 source audit findings and correction

Date: 2026-11-07

## Finding F-REV7-01 — peer replay store mirror mismatch

The first full mirror comparison found exactly one source mismatch between
`firmware/common/` and each sketch source tree:

- `NvsPeerReplayStore.cpp`

The common copy included an initialization marker (`init`, value `0xA7`) for
the persistent anti-replay journal. The ESP32 #1 and ESP32 #2 copies did not.
The marker makes loss of both initialized journal slots distinguishable from
first-time empty storage and therefore prevents an unsafe silent reset of the
replay floor. The host test compiled the common copy, so the prior passing test
result did not establish mirror consistency.

### Correction

Both sketch copies were replaced byte-for-byte with the common implementation.
A new `tests/check_common_module_sync.sh` checks every file in
`firmware/common/` against both sketch source trees. Both native test runners
now execute this check.

### Verification

- Standard native suite after correction: **59/59 test programs passed**; all integration checks passed, including the new complete source-mirror check.
- ASan/UBSan suite after correction: **59/59 test programs passed**; all integration checks passed, including the new complete source-mirror check.
- Python provisioning-client suite: **17/17 passed**.
- `python3 -m compileall -q tools tests/python`: passed.
- Arduino-ESP32 target build: not available in this environment.
- Real ESP32 Preferences/NVS and power-cut behavior: not verified.
- Peer command path remains unintegrated/default-deny; this correction does not
  enable network control or physical outputs.

## Scope note

This was a source-mirror consistency defect, not evidence that the peer command
path is active. `NvsPeerReplayStore` is not currently instantiated by either
`.ino` sketch. Its persistent anti-replay integration remains a future gate
before any network command path is enabled.
