#ifndef HWC_CREDENTIAL_LEGACY_MIGRATOR_H
#define HWC_CREDENTIAL_LEGACY_MIGRATOR_H

#include "CredentialStore.h"

namespace HWC {

enum CredentialLegacyMigrationResult {
    CREDENTIAL_MIGRATION_DONE,
    CREDENTIAL_MIGRATION_ALREADY_PRESENT,
    CREDENTIAL_MIGRATION_SOURCE_INVALID,
    CREDENTIAL_MIGRATION_TARGET_CORRUPT,
    CREDENTIAL_MIGRATION_STORAGE_ERROR,
    CREDENTIAL_MIGRATION_COMMIT_FAILED
};

// One-time, boot-local migration from the old split NVS namespaces into the
// atomic two-slot CredentialStore. It never overwrites a non-empty target and
// never falls back to legacy values after a target corruption/error.
class CredentialLegacyMigrator {
public:
    static CredentialLegacyMigrationResult loadOrMigrate(
        CredentialStore& target, const Credentials& legacy, Credentials& active) {
        CredentialStore::clear(active);
        Credentials current;
        CredentialStore::clear(current);
        const CredentialStoreStatus before = target.load(current);
        if (before == CREDENTIAL_VALID) {
            active = current;
            CredentialStore::clear(current);
            return CREDENTIAL_MIGRATION_ALREADY_PRESENT;
        }
        CredentialStore::clear(current);
        if (before == CREDENTIAL_IO_ERROR) return CREDENTIAL_MIGRATION_STORAGE_ERROR;
        if (before == CREDENTIAL_CORRUPT) return CREDENTIAL_MIGRATION_TARGET_CORRUPT;
        if (!CredentialStore::validate(legacy)) return CREDENTIAL_MIGRATION_SOURCE_INVALID;

        // saveInternal is private and this class is its only additional friend.
        // The operation is permitted only when target.load() proved EMPTY.
        target.saveInternal(legacy);

        CredentialStore::clear(current);
        const CredentialStoreStatus after = target.load(current);
        if (after == CREDENTIAL_IO_ERROR) {
            CredentialStore::clear(current);
            return CREDENTIAL_MIGRATION_STORAGE_ERROR;
        }
        if (after == CREDENTIAL_VALID && CredentialStore::same(current, legacy)) {
            active = current;
            CredentialStore::clear(current);
            return CREDENTIAL_MIGRATION_DONE;
        }
        CredentialStore::clear(current);
        if (after == CREDENTIAL_CORRUPT) return CREDENTIAL_MIGRATION_TARGET_CORRUPT;
        return CREDENTIAL_MIGRATION_COMMIT_FAILED;
    }
};

} // namespace HWC
#endif
