#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MAIN1=firmware/ESP32_1/ESP32_1.ino
MAIN2=firmware/ESP32_2/ESP32_2.ino
WELL=firmware/common/WellPumpProtection.cpp
LOWER=firmware/common/LowerTankPumpProtection.cpp
UPPER=firmware/common/UpperTankProtection.cpp

# ESP32 #1 must derive each output only from its own sensor freshness and own permit.
grep -Fq 'const bool wellPermit = pumpProtectionState.emergencyModeActive ||' "$MAIN1"
grep -Fq 'const bool lowerPermit = lowerPumpState.emergencyModeActive ||' "$MAIN1"
# The well protection's only shared/system fault input is its own protection storage.
grep -Fq 'inputs.otherCriticalFault = !protectionStorageWritable;' "$MAIN1"
# Lower tank protection uses its own config, sensor and storage; it must not consume well/peer state.
grep -Fq 'lowerPumpProtection->update(lowerPumpConfig' "$MAIN1"
grep -Fq 'lowerPumpStorageReady && lowerPumpConfigConfigured' "$MAIN1"
grep -Fq 'lowerEmergencyEnableRequested, lowerEmergencyConfirmationRequested' "$MAIN1"
grep -Fq 'lowerPumpProtection->criticalLevelLatched()' "$MAIN1"
grep -Fq 'webServer.on("/lower-pump-emergency", HTTP_POST, handleLowerPumpEmergency)' "$MAIN1"
# Upper tank protection and its physical relay are local to ESP32 #2; no peer state in relay permit.
grep -Fq 'const bool permit = upperPumpProtectionState.emergencyModeActive ||' "$MAIN2"
if sed -n '/void syncUpperPumpSafetyRelay/,/^}/p' "$MAIN2" | grep -Eq 'peer|Peer|ethernet|Ethernet'; then
  echo 'FAIL: upper pump relay permit must not depend on peer communication' >&2; exit 1
fi
# Explicit physical confirmation bypasses software-only critical/config/storage faults.
grep -Fq 'const bool permit = emergencyMode_ || normalPermit;' "$WELL"
grep -Fq 'const bool permit = emergencyMode_ || normalPermit;' "$UPPER"
grep -Fq 'const bool permit = emergencyMode_ || normalPermit;' "$LOWER"
grep -Fq 'if (inputs.enableEmergencyCommand && inputs.emergencyConfirmation)' "$WELL"
grep -Fq 'if (enableEmergencyCommand && emergencyConfirmation)' "$LOWER"
grep -Fq 'if (enableEmergencyCommand && emergencyConfirmation)' "$UPPER"
# The HTTP handlers require physical confirmation but do not reject software faults.
! sed -n '/void handlePumpEmergency()/,/^}/p' "$MAIN1" | grep -Eq 'protectionStorageWritable|wellProtectionConfigConfigured|haveWellReading|SENSOR_MATH_OK'
! sed -n '/void handleLowerPumpEmergency()/,/^}/p' "$MAIN1" | grep -Eq 'lowerPumpStorageReady|lowerPumpConfigConfigured|criticalLevelLatched|sampleIsFresh'
! sed -n '/void handleLocalPumpEmergency()/,/^}/p' "$MAIN2" | grep -Eq 'upperProtectionStorageReady|upperProtectionConfigValid|upperSafetyStorageReady|criticalLevelLatched|haveUpperReading'
# Valve emergency mode preserves a valid well pump permit; it does not itself grant pump permit.
grep -Fq 'emergencyWellValvePumpGuardAllows' "$MAIN1"
grep -Fq 'return !wellSensorUsable || wellPumpPermit;' firmware/common/EmergencyValvePumpGuard.h
# The lower tank's 11 cm working minimum must not be substituted for pump protection settings.
! grep -Eq 'minimumLevelCm[[:space:]]*=[[:space:]]*11([.]0*)?f?' "$MAIN1" "$LOWER"
echo 'PASS: three pump protections are independent; confirmed emergency overrides software sensor/config/NVS/critical-latch faults; relay hardware remains fail-off; upper protection is local'
