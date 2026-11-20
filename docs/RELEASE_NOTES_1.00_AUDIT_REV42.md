# Release notes – REV42

- Added explicit service recovery for missing/corrupt electrical sensor configuration on ESP32 #1 (well/lower) and ESP32 #2 (upper).
- Recovery requires same-origin authenticated POST, physical-reference confirmation, valid live sensor signal, scale, and independently measured reference level.
- Offset is calculated from the current raw reading and reference height.
- NVS recovery refuses to overwrite a valid record; unavailable NVS remains a hard failure.
- Added native tests for missing/corrupt/valid/NVS-unavailable recovery cases and static integration checks.
- ESP32 #2 dashboard no longer presents provisional default-converted height as trusted.
- No change to volume calibration or pump/transfer protection thresholds.
