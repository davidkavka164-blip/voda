#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
ESP1=firmware/ESP32_1/ESP32_1.ino
ESP2=firmware/ESP32_2/ESP32_2.ino

grep -q '#include "src/NvsCredentialTextStorage.h"' "$ESP1"
grep -q '#include "src/CredentialLegacyMigrator.h"' "$ESP1"
grep -q 'CredentialLegacyMigrator::loadOrMigrate' "$ESP1"
grep -q '#include "src/NvsCredentialTextStorage.h"' "$ESP2"
grep -q '#include "src/CredentialLegacyMigrator.h"' "$ESP2"
grep -q 'CredentialLegacyMigrator::loadOrMigrate' "$ESP2"
[ "$(grep -c 'ensureCredentialText(' "$ESP1")" -ge 2 ]
[ "$(grep -c 'ensureCredentialText(' "$ESP2")" -ge 3 ]

# ESP32 #1 must block AP start until AP and administrator credentials are valid.
awk '/void maintainServiceAp\(uint32_t now\)/,/^}/ { print }' "$ESP1" | grep -q 'if (!apCredentialStorageReady'
awk '/void maintainServiceAp\(uint32_t now\)/,/^}/ { print }' "$ESP1" | grep -q 'adminPassword.length() < 12'
awk '/void maintainServiceAp\(uint32_t now\)/,/^}/ { print }' "$ESP1" | grep -q 'WiFi.softAP'

# ESP32 #2 must not call softAP before all three credentials pass the startup gate.
awk '/void startLocalWeb\(\)/,/^}/ { print }' "$ESP2" | grep -q 'if (!credentialsReady)'
awk '/void maintainServiceAp\(uint32_t now\)/,/^}/ { print }' "$ESP2" | grep -Fq 'if (!localWebReady) return;'
awk '/void maintainServiceAp\(uint32_t now\)/,/^}/ { print }' "$ESP2" | grep -Fq 'WiFi.softAP'
awk '/void startLocalWeb\(\)/,/^}/ { print }' "$ESP2" | grep -Fq 'localWebReady = true;'

# The main dashboard no longer exposes the AP password in HTML.
if grep -q 'htmlEscape(apPassword)' "$ESP1"; then
  echo 'FAIL: ESP32 #1 dashboard still exposes the AP password' >&2
  exit 1
fi
if grep -q 'Heslo AP:' "$ESP1"; then
  echo 'FAIL: ESP32 #1 dashboard still contains an AP password label' >&2
  exit 1
fi

# Credential displays are limited to first creation, not routine boot diagnostics.
grep -q 'Initial configuration AP password (one-time display)' "$ESP1"
grep -q 'Initial WEB ADMIN password (one-time display)' "$ESP1"
grep -q 'Initial upper-tank AP password (one-time display)' "$ESP2"
grep -q 'Initial upper-tank web admin password (one-time display)' "$ESP2"
if grep -q 'Configuration AP password: %s' "$ESP1" || grep -q 'password=%s IP=' "$ESP2"; then
  echo 'FAIL: recurring credential log remains' >&2
  exit 1
fi

# Unified two-slot CredentialStore is authoritative after migration. A corrupt
# or unavailable target may not fall back to legacy namespaces.
grep -q 'initialStatus != HWC::CREDENTIAL_EMPTY' "$ESP1"
grep -q 'legacy fallback is blocked' "$ESP1"
grep -q 'initialStatus == HWC::CREDENTIAL_EMPTY' "$ESP2"
grep -q 'legacy fallback blocked' "$ESP2"

printf 'PASS: both sketch AP startup paths use unified credentials, fail-closed migration and dashboard AP password removal\n'
