# HOME WATER CONTROLLER – audit draft REV36

## Changes

- ESP32 #2 OTA upload now rechecks the upper-pump emergency override immediately before `Update.end(true)` selects the uploaded image.
- If the override is active at finalization, the in-progress OTA write is aborted and the device does not mark the image as successfully uploaded.
- Extended `tests/check_ota_safety_interlock.sh` to verify the finalization-time guard.

## Reason

REV35 already checked the emergency state at OTA start and between upload chunks on ESP32 #2. The finalization branch did not repeat that check. REV36 closes this narrow timing gap and aligns the upper unit with the more complete ESP32 #1 upload guard.

## Verification status

- Native suite: to be confirmed for this package.
- Python suite: to be confirmed for this package.
- Shell syntax and ZIP integrity: to be confirmed for this package.
- Arduino-ESP32 target compilation and physical OTA/browser tests remain outstanding.
