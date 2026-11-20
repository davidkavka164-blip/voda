#include "WifiStaCredentialStore.h"
#include <assert.h>
#include <map>
#include <string>
#include <stdio.h>

class FakeBackend : public HWC::CredentialBackend {
public:
    std::map<std::string, std::string> values;
    std::string failKey;
    std::string silentKey;
    bool failWrites;
    FakeBackend() : failWrites(false) {}
    HWC::CredentialReadResult get(const char* key, char* out, size_t cap) {
        std::map<std::string, std::string>::const_iterator it = values.find(key);
        if (it == values.end()) return HWC::CREDENTIAL_READ_MISSING;
        if (it->second.size() + 1U > cap) return HWC::CREDENTIAL_READ_ERROR;
        memcpy(out, it->second.c_str(), it->second.size() + 1U);
        return HWC::CREDENTIAL_READ_FOUND;
    }
    bool put(const char* key, const char* value) {
        if (failWrites && failKey == key) return false;
        if (silentKey == key) return true; // backend claims success but keeps old bytes
        values[key] = value;
        return true;
    }
    bool erase(const char* key) { values.erase(key); return true; }
};

static HWC::WifiStaCredentials make(const char* ssid, const char* password) {
    HWC::WifiStaCredentials c = {};
    strncpy(c.ssid, ssid, sizeof(c.ssid) - 1U);
    strncpy(c.password, password, sizeof(c.password) - 1U);
    return c;
}

int main() {
    FakeBackend backend;
    HWC::WifiStaCredentialStore store(backend);
    HWC::WifiStaCredentials loaded = {};
    assert(store.load(loaded) == HWC::WIFI_STA_CREDENTIAL_EMPTY);

    HWC::WifiStaCredentials oldValue = make("HomeOld", "old-password");
    HWC::WifiStaCredentials newValue = make("HomeNew", "new-password");
    assert(store.save(oldValue));
    assert(backend.values["sta_active"] == "0");

    // Interrupted update in inactive slot must leave the committed slot intact.
    backend.failWrites = true;
    backend.failKey = "s1_pass";
    assert(!store.save(newValue));
    backend.failWrites = false;
    assert(store.load(loaded) == HWC::WIFI_STA_CREDENTIAL_VALID);
    assert(strcmp(loaded.ssid, "HomeOld") == 0);
    assert(strcmp(loaded.password, "old-password") == 0);

    // Retry rewrites the incomplete inactive slot and commits it atomically.
    assert(store.save(newValue));
    assert(backend.values["sta_active"] == "1");
    assert(store.load(loaded) == HWC::WIFI_STA_CREDENTIAL_VALID);
    assert(strcmp(loaded.ssid, "HomeNew") == 0);
    assert(strcmp(loaded.password, "new-password") == 0);

    // A false-success backend is caught by exact read-back.
    HWC::WifiStaCredentials third = make("HomeThird", "third-password");
    backend.silentKey = "s0_ssid";
    assert(!store.save(third));
    backend.silentKey.clear();
    assert(store.load(loaded) == HWC::WIFI_STA_CREDENTIAL_VALID);
    assert(strcmp(loaded.ssid, "HomeNew") == 0);

    // Legacy migration writes and verifies a complete slot before selecting it.
    FakeBackend legacyBackend;
    legacyBackend.values["ssid"] = "LegacyHome";
    legacyBackend.values["password"] = "legacy-pass";
    HWC::WifiStaCredentialStore legacyStore(legacyBackend);
    assert(legacyStore.load(loaded) == HWC::WIFI_STA_CREDENTIAL_VALID);
    assert(legacyBackend.values["sta_active"] == "0");
    assert(legacyBackend.values["ssid"] == "LegacyHome");
    assert(legacyBackend.values["password"] == "legacy-pass");
    assert(strcmp(loaded.ssid, "LegacyHome") == 0);

    // Interrupted migration can be retried from the untouched legacy pair.
    FakeBackend interruptedMigration;
    interruptedMigration.values["ssid"] = "RetryHome";
    interruptedMigration.values["password"] = "retry-pass";
    interruptedMigration.failWrites = true;
    interruptedMigration.failKey = "s0_pass";
    HWC::WifiStaCredentialStore retryStore(interruptedMigration);
    assert(retryStore.load(loaded) == HWC::WIFI_STA_CREDENTIAL_IO_ERROR);
    interruptedMigration.failWrites = false;
    assert(retryStore.load(loaded) == HWC::WIFI_STA_CREDENTIAL_VALID);
    assert(interruptedMigration.values["sta_active"] == "0");
    assert(strcmp(loaded.ssid, "RetryHome") == 0);

    // A partial legacy pair is not combined with defaults or silently accepted.
    FakeBackend partialLegacy;
    partialLegacy.values["ssid"] = "OnlySSID";
    HWC::WifiStaCredentialStore partialStore(partialLegacy);
    assert(partialStore.load(loaded) == HWC::WIFI_STA_CREDENTIAL_CORRUPT);
    assert(partialLegacy.values.find("sta_active") == partialLegacy.values.end());

    // Open Wi-Fi networks with an empty password remain supported.
    FakeBackend openBackend;
    HWC::WifiStaCredentialStore openStore(openBackend);
    HWC::WifiStaCredentials openNetwork = make("OpenNetwork", "");
    assert(openStore.save(openNetwork));
    assert(openStore.load(loaded) == HWC::WIFI_STA_CREDENTIAL_VALID);
    assert(strcmp(loaded.password, "") == 0);

    // A corrupt selector is fail-closed; never fall back to legacy values.
    FakeBackend corruptBackend;
    corruptBackend.values["sta_active"] = "x";
    corruptBackend.values["ssid"] = "LegacyHome";
    corruptBackend.values["password"] = "legacy-pass";
    HWC::WifiStaCredentialStore corruptStore(corruptBackend);
    assert(corruptStore.load(loaded) == HWC::WIFI_STA_CREDENTIAL_CORRUPT);

    HWC::WifiStaCredentialStore::clear(loaded);
    HWC::WifiStaCredentialStore::clear(oldValue);
    HWC::WifiStaCredentialStore::clear(newValue);
    HWC::WifiStaCredentialStore::clear(third);
    HWC::WifiStaCredentialStore::clear(openNetwork);
    puts("PASS: transactional Wi-Fi STA credential storage and legacy migration");
    return 0;
}
