#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
FILES="WebAuthRateLimiter.h CredentialRandom.h CredentialStore.h CredentialLegacyMigrator.h NvsCredentialBackend.h LocalServiceAuthorization.h CredentialProvisioningController.h CredentialProvisioningSerialProtocol.h CredentialProvisioningBoardAdapter.h CredentialStartupGate.h NvsCredentialTextStorage.h"
for FILE in $FILES; do
  cmp "firmware/common/$FILE" "firmware/ESP32_1/src/$FILE"
  cmp "firmware/common/$FILE" "firmware/ESP32_2/src/$FILE"
done
printf 'PASS: credential security modules are byte-identical in common and both ESP32 sketches\n'
