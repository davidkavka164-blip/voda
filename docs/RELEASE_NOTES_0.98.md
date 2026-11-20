# HOME WATER CONTROLLER — DEV KIT 0.98

## Credential NVS power-cut model

- Added `tests/test_credential_powercut.cpp`.
- The test seeds a valid old credential set, simulates interruption at each of the four NVS mutation points of a credential update (three inactive-slot fields and active-slot selector), and covers interruption immediately before or after persistence.
- After simulated reboot, the store must load exactly the previous or the new complete credential set. A partial credential set must never be accepted as valid.
- This is a deterministic host-side persistence model. It is **not** a physical ESP32 power-cut test and does not prove flash/NVS behavior under real brownout or power loss.
- Firmware source and hardware pin assignments are unchanged.

## Verification

- Native C++ suite: 58/58 test programs PASS.
- ASan/UBSan suite: 58/58 PASS.
- Python provisioning client tests: 17/17 PASS; `py_compile` PASS.
- Arduino-ESP32 1.0.6 target build, real NVS, physical power-cut, provisioning GPIO/UART, and hardware remain unverified.
