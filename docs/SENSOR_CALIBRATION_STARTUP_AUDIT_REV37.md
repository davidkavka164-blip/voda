> **Status update: The firmware-side trust propagation finding was implemented in REV38. This document remains the historical audit record for REV37; see `SENSOR_CALIBRATION_TRUST_IMPLEMENTATION_REV38.md` for the implementation.**

# Audit REV37 – startup handling of sensor calibration records

**Baseline:** audit draft REV36  
**Scope:** NVS sensor calibration load at startup on ESP32 #1 and ESP32 #2; effect on level validity, pump protection and transfer decisions.

## Finding

`NvsSensorConfigStore::load()` correctly rejects a record with the wrong length, invalid fields or a checksum mismatch. `save()` also refuses to overwrite an existing record that fails validation and verifies a successful write by reading it back.

The startup callers do not preserve the distinction between these cases:

1. no calibration key exists (first installation),
2. a calibration key exists but is corrupt or incompatible,
3. the Preferences/NVS namespace cannot be opened.

ESP32 #1's `loadConfig()` and ESP32 #2's `setup()` assign `defaultSensorConfig()` in RAM for all three cases. The default is a valid provisional electrical conversion (150 Ω, 4–20 mA, 1000 cm, scale 1.0, offset 0). `LevelSensorReader` then reports a normal `SENSOR_MATH_OK` reading when the measured current is in range.

The pump-protection update paths and transfer readiness checks consume the resulting sensor-math validity. They do not separately know whether the calibration record was absent, validated from NVS, corrupt, or unavailable. Consequently, a previously customized scale/offset can be silently replaced at runtime by provisional defaults after a corrupt/unreadable record, while the reading can still appear valid to software protections and transfer gates.

## Risk assessment

This is not a CRC-validation defect in the store. It is a startup-state propagation gap. A plausible but incorrectly converted level can be more dangerous than an explicitly invalid sensor reading because it may satisfy normal software range checks.

The risk is most significant where a custom scale or offset had been used. The factory defaults remain useful for first-install commissioning and diagnostics, so treating every missing key as equivalent to corruption would unnecessarily change first-install behavior.

## Recommended correction for a subsequent firmware revision

Keep the provisional default conversion available for display/diagnostics, but track a separate per-sensor calibration state:

- `MISSING_PROVISIONAL`: no key exists and the NVS namespace was opened successfully; defaults may be used for commissioning, with an explicit provisional indication.
- `VALID_STORED`: the stored record passed length, field and checksum validation.
- `INVALID_PRESENT`: a key exists but failed validation; do not silently substitute defaults for safety decisions.
- `NVS_UNAVAILABLE`: the namespace could not be opened; stored-vs-missing state is unknown.

For `INVALID_PRESENT` and `NVS_UNAVAILABLE`, normal automatic pump permits and any transfer decision that depends on that sensor should fail closed. A confirmed emergency pump mode must continue to follow the already-established user rule: it may bypass software sensor/configuration/NVS faults after explicit physical level confirmation, but it must not bypass independent electrical/motor protection. Do not change the emergency-mode contract.

For `MISSING_PROVISIONAL`, retain the agreed first-install behavior unless commissioning policy is explicitly changed. The web/serial status should clearly distinguish provisional defaults from a stored calibrated record. The implementation must not rewrite defaults to NVS automatically.

Apply the same policy consistently to well, lower-tank and upper-tank sensors, and review all consumers: pump protection, transfer start/continuation, peer status validity, and displayed sensor state. Do not mark the raw ADC conversion itself invalid solely to hide this state; keep measurement diagnostics separate from permission validity.

## Why no firmware change is included in REV37

The correction affects three independent sensor consumers and several transfer/peer gates. A partial patch could leave a path that still trusts a provisional/corrupt calibration. This revision records the complete finding and test requirements rather than introducing an unverified partial safety change. The behavior must be corrected with dedicated integration tests in a later revision.

## Required tests for the implementation revision

For each of the three sensors:

1. No NVS key + NVS available: provisional defaults are visible and clearly labelled; verify the intended commissioning policy.
2. Valid stored record: exact stored scale/offset is used and status is `VALID_STORED`.
3. Present record with wrong length, bad checksum, unsupported version or out-of-range value: status is `INVALID_PRESENT`; normal automatic pump permit and dependent transfer permission remain OFF.
4. NVS namespace open failure: status is `NVS_UNAVAILABLE`; normal automatic permits remain OFF.
5. Explicit confirmed emergency pump mode still bypasses these software faults as specified, and returns to normal protection after disable/reboot.
6. Corrupt record is not overwritten by defaults; an explicit validated calibration save/service process is required.
7. Peer status from ESP32 #2 does not advertise an upper level as usable when upper calibration is invalid/unavailable.
8. Sensor display/diagnostics distinguish electrical measurement validity from calibration trust and pump/transfer permission.

## Verification performed on REV36 source

- Confirmed the store rejects invalid/corrupt records and refuses to overwrite them.
- Confirmed both ESP32 startup paths fall back to `defaultSensorConfig()` when load fails.
- Confirmed pump and transfer consumers use sensor-math validity without a separate persisted-calibration trust flag.
- `tests/run_native_tests.sh`: 65/65 native test programs PASS, including all integration checks.
- Python unittest discovery: 17/17 PASS.
- `python3 -m compileall -q tools tests/python`: PASS.
- `bash -n tests/*.sh`: PASS.

These tests verify the existing behavior; they do not demonstrate that the identified startup-state gap is safe. Arduino target compilation and physical ESP32/NVS tests remain outstanding.

## Status

**Finding confirmed; firmware correction remains open.** REV37 is a documentation/audit revision only. Do not treat it as closing this issue or as a production-ready release.
