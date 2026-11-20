#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
ESP1=firmware/ESP32_1/ESP32_1.ino
ESP2=firmware/ESP32_2/ESP32_2.ino

for SKETCH in "$ESP1" "$ESP2"; do
  grep -q '#include "src/CredentialRandom.h"' "$SKETCH"
  grep -q 'bool enableCredentialRandomSource()' "$SKETCH"
  grep -q 'WiFi.persistent(false);' "$SKETCH"
  grep -q 'if (!WiFi.mode(WIFI_STA))' "$SKETCH"
  grep -q 'HWC::generateHexCredential(.*credentialRandomWordSource, 4U)' "$SKETCH"
  awk '/bool enableCredentialRandomSource\(\)/,/^}/ { print }' "$SKETCH" | grep -q 'WiFi.persistent(false);'
  awk '/bool enableCredentialRandomSource\(\)/,/^}/ { print }' "$SKETCH" | grep -q 'if (!WiFi.mode(WIFI_STA))'
done

# Every new secret must be generated only after the RF entropy-source gate.
awk '/bool prepareConfigApCredentials\(\)/,/^}/ { print }' "$ESP1" | grep -q 'enableCredentialRandomSource()'
awk '/bool prepareLegacyAdminCredential\(bool& generatedNew\)/,/^}/ { print }' "$ESP1" | grep -q 'enableCredentialRandomSource()'
awk '/void startLocalWeb\(\)/,/^}/ { print }' "$ESP2" | grep -q 'enableCredentialRandomSource()'
awk '/void startLocalWeb\(\)/,/^}/ { print }' "$ESP2" | grep -q 'WiFi.persistent(false);'

# The RF gate must appear before the first generation operation in each path.
awk '/bool prepareConfigApCredentials\(\)/,/^}/ { if (!gate && /enableCredentialRandomSource\(\)/) gate=NR; if (!gen && /generateHexCredential/) gen=NR } END { exit !(gate && gen && gate < gen) }' "$ESP1"
awk '/bool prepareLegacyAdminCredential\(bool& generatedNew\)/,/^}/ { if (!gate && /enableCredentialRandomSource\(\)/) gate=NR; if (!gen && /generateHexCredential/) gen=NR } END { exit !(gate && gen && gate < gen) }' "$ESP1"
awk '/void startLocalWeb\(\)/,/^}/ { if (!gate && /enableCredentialRandomSource\(\)/) gate=NR; if (!gen && /generateHexCredential/) gen=NR } END { exit !(gate && gen && gate < gen) }' "$ESP2"

# Four 32-bit words must be used for the AP and admin credential defaults.
[ "$(grep -c 'credentialRandomWordSource, 4U' "$ESP1")" -ge 2 ]
[ "$(grep -c 'credentialRandomWordSource, 4U' "$ESP2")" -ge 2 ]

# Old low-entropy templates must not reappear in either credential path.
if grep -Eq 'H%08X|HWC-%08X|UP-%08X%04X' "$ESP1" "$ESP2"; then
  echo 'FAIL: legacy low-entropy credential template remains' >&2
  exit 1
fi

printf 'PASS: new credential generation is RF-gated and uses four 32-bit random words in both sketches\n'
