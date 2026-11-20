#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
INO="$ROOT/firmware/ESP32_1/ESP32_1.ino"
grep -Fq 'bool wellGeometryLoaded = false;' "$INO"
! grep -Fq 'wellProtectionConfigConfigured = wellGeometryLoaded &&' "$INO"
! grep -Fq 'if (!wellGeometryLoaded) {' "$INO"
grep -Fq 'if (source == HWC::TRANSFER_SOURCE_WELL && !wellProtectionConfigConfigured)' "$INO"
grep -Fq 'wellGeometryLoaded = true;' "$INO"
grep -Fq 'using default dimensions for clearly labelled volume estimates and protection-level range checks' "$INO"
echo 'PASS: RAM default geometry supports basic estimates and protection range checks; well transfers still require valid pump protection thresholds'
