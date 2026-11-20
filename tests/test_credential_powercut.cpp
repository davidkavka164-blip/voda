#include "CredentialProvisioningController.h"
#include <assert.h>
#include <map>
#include <string>
#include <stdio.h>
#include <string.h>

class PersistentBackend : public HWC::CredentialBackend {
public:
    std::map<std::string, std::string> data;
    unsigned mutationCount;
    unsigned cutAt;
    bool powered;
    bool cutAfterWrite;
    PersistentBackend() : mutationCount(0), cutAt(0), powered(true), cutAfterWrite(true) {}
    HWC::CredentialReadResult get(const char* key, char* out, size_t cap) {
        if (!powered || !key || !out || cap == 0) return HWC::CREDENTIAL_READ_ERROR;
        std::map<std::string,std::string>::const_iterator it = data.find(key);
        if (it == data.end()) return HWC::CREDENTIAL_READ_MISSING;
        if (it->second.size() + 1U > cap) return HWC::CREDENTIAL_READ_ERROR;
        memcpy(out, it->second.c_str(), it->second.size() + 1U);
        return HWC::CREDENTIAL_READ_FOUND;
    }
    bool put(const char* key, const char* value) {
        if (!powered) return false;
        ++mutationCount;
        const bool cut = cutAt != 0 && mutationCount == cutAt;
        if (cut && !cutAfterWrite) { powered = false; return false; }
        data[key] = value;
        if (cut) powered = false;
        return powered;
    }
    bool erase(const char* key) {
        if (!powered) return false;
        ++mutationCount;
        const bool cut = cutAt != 0 && mutationCount == cutAt;
        if (cut && !cutAfterWrite) { powered = false; return false; }
        data.erase(key);
        if (cut) powered = false;
        return powered;
    }
};

static HWC::Credentials credentials(const char* ap, const char* admin) {
    HWC::Credentials c; HWC::CredentialStore::clear(c);
    strcpy(c.apName, "HWC-POWER-CUT"); strcpy(c.apPassword, ap); strcpy(c.adminPassword, admin);
    return c;
}
static bool provision(PersistentBackend& backend, HWC::Credentials& submitted, uint32_t now) {
    HWC::CredentialStore store(backend); HWC::LocalServiceAuthorization gate;
    HWC::CredentialProvisioningController controller(store, gate);
    gate.samplePhysicalInput(now, true);
    gate.samplePhysicalInput(now + HWC::LocalServiceAuthorization::HOLD_MS, true);
    if (!controller.openSession(now + HWC::LocalServiceAuthorization::HOLD_MS)) return false;
    return controller.provision(now + HWC::LocalServiceAuthorization::HOLD_MS + 1U, submitted) == HWC::CREDENTIAL_PROVISIONED;
}
static bool equals(const HWC::Credentials& a, const HWC::Credentials& b) {
    return strcmp(a.apName,b.apName)==0 && strcmp(a.apPassword,b.apPassword)==0 && strcmp(a.adminPassword,b.adminPassword)==0;
}

int main() {
    const HWC::Credentials oldValue = credentials("Old-ap-pass-123", "Old-admin-pass-123");
    const HWC::Credentials newValue = credentials("New-ap-pass-123", "New-admin-pass-123");
    unsigned testedCuts = 0;
    for (unsigned cut = 1; cut <= 4; ++cut) {
        for (unsigned after = 0; after <= 1; ++after) {
            PersistentBackend seed;
            HWC::CredentialStore seedStore(seed);
            HWC::Credentials first = oldValue;
            assert(provision(seed, first, 100U));
            PersistentBackend trial = seed;
            trial.mutationCount = 0; trial.cutAt = cut; trial.cutAfterWrite = after != 0;
            HWC::Credentials replacement = newValue;
            (void)provision(trial, replacement, 10000U);
            // Reboot: power returns, while the persisted key/value image remains.
            trial.powered = true; trial.cutAt = 0; trial.mutationCount = 0;
            HWC::CredentialStore rebooted(trial); HWC::Credentials observed;
            const HWC::CredentialStoreStatus status = rebooted.load(observed);
            assert(status == HWC::CREDENTIAL_VALID);
            assert(equals(observed, oldValue) || equals(observed, newValue));
            HWC::CredentialStore::clear(observed);
            HWC::CredentialStore::clear(replacement);
            ++testedCuts;
        }
    }
    puts("PASS: credential update power-cut model (8 interruption points; old-or-new credential invariant)");
    return testedCuts == 8 ? 0 : 1;
}
