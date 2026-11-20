#include "CredentialLegacyMigrator.h"
#include <assert.h>
#include <map>
#include <string>
#include <string.h>
#include <stdio.h>

class FakeBackend : public HWC::CredentialBackend {
public:
    bool failGet;
    bool failPut;
    std::map<std::string, std::string> data;
    FakeBackend() : failGet(false), failPut(false) {}
    HWC::CredentialReadResult get(const char* key, char* out, size_t cap) {
        if (failGet) return HWC::CREDENTIAL_READ_ERROR;
        std::map<std::string, std::string>::const_iterator it = data.find(key);
        if (it == data.end()) return HWC::CREDENTIAL_READ_MISSING;
        if (it->second.size() >= cap) return HWC::CREDENTIAL_READ_ERROR;
        memcpy(out, it->second.c_str(), it->second.size() + 1U);
        return HWC::CREDENTIAL_READ_FOUND;
    }
    bool put(const char* key, const char* value) {
        if (failPut) return false;
        data[key] = value;
        return true;
    }
    bool erase(const char* key) { data.erase(key); return true; }
};

static HWC::Credentials legacyCredentials() {
    HWC::Credentials c;
    HWC::CredentialStore::clear(c);
    strcpy(c.apName, "HomeWater-1234");
    strcpy(c.apPassword, "H12345678");
    strcpy(c.adminPassword, "HWC-123456789ABC");
    return c;
}

int main() {
    FakeBackend b;
    HWC::CredentialStore store(b);
    HWC::Credentials legacy = legacyCredentials();
    HWC::Credentials active;
    HWC::CredentialStore::clear(active);

    assert(store.load(active) == HWC::CREDENTIAL_EMPTY);
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(store, legacy, active) == HWC::CREDENTIAL_MIGRATION_DONE);
    assert(HWC::CredentialStore::validate(active));
    assert(strcmp(active.apName, legacy.apName) == 0);
    assert(strcmp(active.apPassword, legacy.apPassword) == 0);

    // A later boot trusts the committed unified store, not changed legacy data.
    HWC::Credentials stale = legacyCredentials();
    strcpy(stale.apPassword, "STALE-12345678");
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(store, stale, active) == HWC::CREDENTIAL_MIGRATION_ALREADY_PRESENT);
    assert(strcmp(active.apPassword, "H12345678") == 0);

    // A corrupt target is not repaired or overwritten by valid legacy values.
    b.data.erase("c0_admin");
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(store, legacy, active) == HWC::CREDENTIAL_MIGRATION_TARGET_CORRUPT);
    assert(store.load(active) == HWC::CREDENTIAL_CORRUPT);

    // Storage errors and invalid legacy source fail closed.
    FakeBackend ioBackend;
    HWC::CredentialStore ioStore(ioBackend);
    ioBackend.failGet = true;
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(ioStore, legacy, active) == HWC::CREDENTIAL_MIGRATION_STORAGE_ERROR);
    ioBackend.failGet = false;
    HWC::Credentials invalid = legacyCredentials();
    invalid.apPassword[0] = '\0';
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(ioStore, invalid, active) == HWC::CREDENTIAL_MIGRATION_SOURCE_INVALID);
    assert(ioStore.load(active) == HWC::CREDENTIAL_EMPTY);

    // Failed first commit leaves the target empty or visibly invalid, never VALID.
    FakeBackend failBackend;
    HWC::CredentialStore failStore(failBackend);
    failBackend.failPut = true;
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(failStore, legacy, active) == HWC::CREDENTIAL_MIGRATION_COMMIT_FAILED);
    assert(failStore.load(active) == HWC::CREDENTIAL_EMPTY);
    puts("PASS: legacy credential migration is one-time, atomic-target-only and fail-closed");
}
