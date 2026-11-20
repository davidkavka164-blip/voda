# Release notes — REV59

- Added `PEER_KEY_USB_PROVISIONING_AUDIT_REV59.md`.
- Audited the existing HMAC codec, plain-NVS key store, and current Serial usage in both ESP32 sketches.
- Selected a minimal local USB–UART provisioning approach for the next implementation step; no extra GPIO or network provisioning.
- No firmware, wiring, pin, NVS partition, eFuse, or production control-gate changes.
- Upper transfer authentication gate remains disabled.
- Re-ran REV58 validation: 72 native tests, 17 Python tests, and 70 ASan/UBSan native tests passed; no Arduino target build or hardware test was performed.
