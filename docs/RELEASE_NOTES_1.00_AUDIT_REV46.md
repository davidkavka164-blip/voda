# HOME WATER CONTROLLER – audit revision REV46

Date: 2026-11-13

## Test package permissions

- Corrected Unix executable permissions for the shell test scripts (`tests/*.sh`).
- The top-level `tests/run_native_tests.sh` invokes integration checks directly; the previous REV45 ZIP did not preserve executable bits, causing `Permission denied` after the native tests had completed.
- No firmware source, runtime behavior, network topology, safety logic, or hardware configuration was changed in this revision.

## REV45 audit rerun

- 67 native test programs passed.
- All integration shell checks passed after restoring executable permissions in the working tree.
- 17 Python unit tests passed.
- Python `compileall` passed.
- Shell syntax checks passed.
- ZIP integrity check passed for the source archive.

## Remaining verification limits

- Arduino-ESP32 target compilation has not been performed.
- Physical ESP32 testing has not been performed.
- UDP broadcast forwarding between ESP32 #2 W5500 Ethernet and ESP32 #1 Wi-Fi STA remains dependent on the actual router/AP configuration and must be tested on site.
