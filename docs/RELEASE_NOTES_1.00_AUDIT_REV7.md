# HOME WATER CONTROLLER DEV KIT 1.00 — AUDIT DRAFT REV7

**Status:** consolidated updated working package; not approved for deployment.

## Included project decisions and changes

- Preserves the NVS protection-store work and broader NVS audit from the preceding 1.00 audit draft.
- Preserves the well geometry rule adopted on 2026-11-07:
  - The basic well geometry is a hard minimum volume model.
  - Default geometry is 80 cm internal diameter and 800 cm usable depth unless the user changes it.
  - Calibration may add positive volume attributable to cavities/widenings, but must never reduce the estimate below the geometric minimum.
  - Outside the measured calibration range, only a positive cavity offset at the nearest measured boundary is carried over the geometric estimate.
  - Incomplete volume calibration does not block basic operation or well-source transfer.
  - Well-source transfer still requires valid, separately configured pump-protection thresholds and valid/fresh measurements.
  - Missing/corrupt persisted geometry falls back to the editable 80 cm / 800 cm RAM baseline and remains diagnostically identifiable as a RAM default.
- The minimum-geometry rule is a project invariant and must not be changed in later revisions without the user's explicit request.

## Verification record

Passing host tests do not establish Arduino target compatibility or physical safety.

## Not verified

- Build with the target Arduino-ESP32 1.0.6 toolchain.
- Real ESP32 Preferences/NVS behavior and physical power-cut recovery.
- Physical sensors, pumps, valves, W5500, Wi-Fi, OTA or end-to-end transfer operation.
- Any physical output activation; outputs and peer command path remain disabled.

**Do not deploy this audit draft to live pump/valve control.** DEV KIT 0.99 remains the last deployment baseline previously identified as verified; this draft requires target and hardware validation.
