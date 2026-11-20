# HOME WATER CONTROLLER – audit draft REV38

**Date:** 2026-11-13  
**Base:** REV37

## Changes

- Added four-state electrical sensor calibration trust model: missing/provisional, valid stored, invalid present, and NVS unavailable.
- Both ESP32 startup paths now distinguish a genuinely missing key from an invalid existing record and from an unavailable Preferences namespace.
- Normal well, lower-tank and upper-tank pump protection now require a trusted calibration state in addition to valid sensor math.
- Automatic transfer source/destination readiness now requires trusted well/lower calibration; upper source readiness is protected by the ESP32 #2 peer validity flag.
- ESP32 #2 no longer advertises upper level as valid to ESP32 #1 when upper calibration is invalid or NVS is unavailable.
- Existing explicit emergency pump overrides remain intact and still bypass software sensor/calibration/NVS faults only after physical confirmation.
- Service valve mode requires trusted sensor calibration; emergency manual valve mode retains its explicit operator override semantics.
- Dashboards distinguish stored calibration, provisional defaults, corrupt record and unavailable NVS.
- Added native policy assertions and a source-level integration check.

## Verification

- Native tests: 65/65 PASS.
- Static/integration checks: PASS, including new sensor calibration trust integration check and common-module synchronization.
- Python tests: 17/17 PASS.
- Python compileall: PASS.
- Shell syntax checks: PASS.
- No Arduino-ESP32 target build or physical NVS/power-cut/relay/peer test performed.

## Release status

REV38 is an audit draft. It closes the REV37 source-level startup-state propagation finding, subject to target compilation and hardware verification. A deliberate repair workflow for an already-corrupt calibration record remains a service/open item; normal web save continues to refuse silent overwrite. Do not treat it as a field-validated production release.
