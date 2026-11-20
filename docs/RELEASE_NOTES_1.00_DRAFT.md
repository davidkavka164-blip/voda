# HOME WATER CONTROLLER DEV KIT 1.00 — DRAFT / NOT FOR DEPLOYMENT

This draft records the NVS protection-store work based on DEV KIT 0.99. It is not a release approval.

## Changes in this working copy

- Replaced single-key `state` rewrites in `NvsWellProtectionStore` and `NvsUpperTankProtectionStore` with two independently keyed slots (`slot0`, `slot1`).
- Added per-slot generation, format metadata, payload length and CRC.
- Added verified writes to the inactive slot while retaining the other slot.
- Added legacy `state` migration after validation and successful slot read-back; legacy key is retained.
- Added fail-closed handling for malformed slots, malformed legacy records and ambiguous equal generations.
- Preserved the well store's `safetyStateValid=false` state through migration and latch updates.
- Added multi-key Preferences host simulation and protection-store synchronization integration check.

## Host verification

- 58/58 native test programs PASS.
- 58/58 ASan/UBSan test programs PASS.
- 17/17 Python tests PASS.
- Python compileall PASS.
- Protection-store source/header mirrors byte-identical across `common`, ESP32 #1 and ESP32 #2.

## Not verified

- Arduino-ESP32 1.0.6 target build.
- Real ESP32 Preferences/NVS behavior and flash power-cut recovery.
- Physical pumps, valves, sensors or other hardware.
- NVS/Flash encryption behavior.
- Recovery procedure for a detected corrupt slot. Current strict policy intentionally blocks loading if either present slot is corrupt, even if the other slot is valid.

**Do not deploy this draft to live hardware.** DEV KIT 0.99 remains the last verified release baseline pending target and hardware validation.

## Follow-up review correction (2026-11-06)

- Well-slot CRC now covers semantic fields only; compiler padding bytes are excluded.
- Well payload read-back comparison is field-by-field, not raw-structure `memcmp`.
- Added interruption tests for replacement of an already-existing older slot in both protection stores.
- Re-ran native, ASan/UBSan and Python test suites successfully.
- Target compilation and physical NVS/power-cut validation remain outstanding; this is still not approved for deployment.


## Follow-up review — 2026-11-07

- Both protection stores now require two valid slot generations to be exactly consecutive; generation zero/wrap is rejected. Non-adjacent pairs fail closed.
- Upper-tank slot CRC and read-back comparison now operate on semantic fields, excluding compiler padding from the persistence contract.
- Added a conservative service recovery procedure to `NVS_PROTECTION_POWER_CUT_AUDIT.md`. No automatic corrupt-slot repair is implemented.
- This remains an audit draft, not approved for deployment. Target build and physical ESP32 NVS/power-cut/hardware validation are still outstanding.

## Follow-up review — 2026-11-07: broader NVS audit

- Geometry and well-volume calibration save APIs now refuse to overwrite a present corrupt record.
- ESP32 #1 keeps the editable 80 cm / 800 cm RAM geometry defaults available for basic volume estimates and protection-threshold range checks when stored geometry is missing/corrupt. The estimate remains explicitly approximate; measured calibration is independent and can be added gradually. Well-source transfers still require valid, configured pump-protection thresholds, but do not wait for geometry persistence or a completed volume calibration.
- Transfer restart journal now fails closed if either slot is corrupt; it no longer falls back to an older `CLEAR` record that could hide a newer interrupted transfer.
- Added native coverage for corrupt geometry-record overwrite protection and updated transfer-journal corruption expectations.
- Verification: 59/59 native, 59/59 ASan/UBSan, 17/17 Python tests, compileall PASS; audited module mirrors synchronized.
- Sensor-config startup still uses provisional RAM defaults when a record is missing or invalid; this is explicitly left for a separate review.
- No target build or physical NVS/power-cut/hardware validation was performed. This remains a draft and must not be deployed to live pump control.

## Neměnný princip minimální geometrie studny – 7. 11. 2026
- Základní geometrie studny je dolní mez objemu, nikoli pouze alternativní odhad. Kalibrační křivka smí přidávat objem z dutin/rozšíření, nikdy nesmí objem snížit pod geometrické minimum.
- V kalibrovaném rozsahu se vrací max(geometrický objem, interpolovaný kalibrovaný objem). Mimo rozsah se k základní geometrii přenáší pouze kladný přírůstek nad geometrickou hodnotu na nejbližší hranici.
- Chybějící kalibrace neblokuje základní provoz. Chybějící/poškozená uložená geometrie použije výchozí 80 cm / 800 cm v RAM jako minimální geometrii a zůstává diagnosticky označena jako neuložená/odhad.
- Transfer ze studny se stále řídí samostatnými bezpečnostními podmínkami ochrany čerpadla a validity měření; nevyžaduje dokončenou objemovou kalibraci ani persistenci geometrie.
