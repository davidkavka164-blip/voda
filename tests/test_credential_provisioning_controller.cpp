#include "CredentialProvisioningController.h"
#include <assert.h>
#include <map>
#include <string>
#include <stdio.h>
#include <string.h>

class FakeBackend : public HWC::CredentialBackend {
public:
    std::map<std::string, std::string> data;
    bool failGet;
    bool failErase;
    std::string failEraseKey;
    bool failPut;
    FakeBackend() : failGet(false), failErase(false), failPut(false) {}
    HWC::CredentialReadResult get(const char* key, char* out, size_t cap) {
        if (failGet) { if (cap) out[0] = '\0'; return HWC::CREDENTIAL_READ_ERROR; }
        std::map<std::string,std::string>::const_iterator it = data.find(key);
        if (it == data.end()) { if (cap) out[0] = '\0'; return HWC::CREDENTIAL_READ_MISSING; }
        if (it->second.size() >= cap) { if (cap) out[0] = '\0'; return HWC::CREDENTIAL_READ_ERROR; }
        memcpy(out, it->second.c_str(), it->second.size() + 1U);
        return HWC::CREDENTIAL_READ_FOUND;
    }
    bool put(const char* key, const char* value) {
        if (failPut) return false;
        data[key] = value;
        return true;
    }
    bool erase(const char* key) {
        if (failErase || (!failEraseKey.empty() && failEraseKey == key)) return false;
        data.erase(key);
        return true;
    }
};

static HWC::Credentials makeCredentials(const char* suffix) {
    HWC::Credentials c;
    HWC::CredentialStore::clear(c);
    strcpy(c.apName, "HWC-UNIT-1234");
    strcpy(c.apPassword, suffix);
    strcpy(c.adminPassword, "Admin-secret-2026");
    return c;
}

static void authorize(HWC::LocalServiceAuthorization& gate, uint32_t start) {
    gate.samplePhysicalInput(start, true);
    gate.samplePhysicalInput(static_cast<uint32_t>(start + HWC::LocalServiceAuthorization::HOLD_MS), true);
}

static bool isWiped(const HWC::Credentials& c) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&c);
    for (size_t i = 0; i < sizeof(c); ++i) if (p[i] != 0U) return false;
    return true;
}

int main() {
    // Provisioning cannot be opened by a web/session caller without a physical authorization.
    {
        FakeBackend b; HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        assert(!controller.openSession(0U));
        HWC::Credentials input = makeCredentials("AP-secret-1234");
        assert(controller.provision(1U, input) == HWC::CREDENTIAL_PROVISION_SESSION_CLOSED);
        assert(isWiped(input));
    }

    // First-time provisioning consumes exactly one physical authorization and one attempt.
    {
        FakeBackend b; HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        authorize(gate, 100U);
        assert(controller.openSession(3100U));
        HWC::Credentials input = makeCredentials("AP-secret-1234");
        assert(controller.provision(3101U, input) == HWC::CREDENTIAL_PROVISIONED);
        assert(isWiped(input));
        HWC::Credentials loaded; HWC::CredentialStore::clear(loaded);
        assert(store.load(loaded) == HWC::CREDENTIAL_VALID);
        assert(strcmp(loaded.apPassword, "AP-secret-1234") == 0);
        HWC::CredentialStore::clear(loaded);
        HWC::Credentials second = makeCredentials("AP-secret-5678");
        assert(controller.provision(3102U, second) == HWC::CREDENTIAL_PROVISION_SESSION_CLOSED);
        assert(isWiped(second));
        assert(!controller.openSession(3103U)); // still held; release is required
    }

    // A physical recovery can remove orphaned first-write staging data after a power cut.
    {
        FakeBackend b;
        b.data["c0_name"] = "HWC-ORPHAN";
        b.data["c0_ap"] = "orphaned-ap-pass";
        HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        HWC::Credentials state; HWC::CredentialStore::clear(state);
        assert(store.load(state) == HWC::CREDENTIAL_CORRUPT);
        HWC::CredentialStore::clear(state);
        authorize(gate, 10000U);
        assert(controller.openSession(13000U));
        HWC::Credentials input = makeCredentials("AP-recovered-123");
        assert(controller.provision(13001U, input) == HWC::CREDENTIAL_PROVISIONED);
        assert(isWiped(input));
        assert(store.load(state) == HWC::CREDENTIAL_VALID);
        HWC::CredentialStore::clear(state);
    }

    // Recovery is fail-closed if erase/read verification cannot complete.
    {
        FakeBackend b;
        b.data["c0_name"] = "HWC-ORPHAN";
        b.failErase = true;
        HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        authorize(gate, 20000U);
        assert(controller.openSession(23000U));
        HWC::Credentials input = makeCredentials("AP-recovered-456");
        assert(controller.provision(23001U, input) == HWC::CREDENTIAL_PROVISION_RECOVERY_FAILED);
        assert(isWiped(input));
        HWC::Credentials state; HWC::CredentialStore::clear(state);
        assert(store.load(state) == HWC::CREDENTIAL_CORRUPT);
        HWC::CredentialStore::clear(state);
    }


    // A malformed credential consumes the authorized attempt and is securely wiped.
    {
        FakeBackend b; HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        authorize(gate, 35000U);
        assert(controller.openSession(38000U));
        HWC::Credentials input = makeCredentials("short");
        assert(controller.provision(38001U, input) == HWC::CREDENTIAL_PROVISION_INVALID_INPUT);
        assert(isWiped(input));
        assert(b.data.empty());
        assert(!controller.isSessionOpen(38002U));
    }

    // If power/failure interrupts recovery after selector removal, storage remains
    // CORRUPT; a fresh physical authorization can finish the cleanup safely.
    {
        FakeBackend b;
        b.data["cred_active"] = "0";
        b.data["c0_name"] = "HWC-PARTIAL";
        b.data["c0_ap"] = "partial-ap-pass";
        b.failEraseKey = "c0_name";
        HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        authorize(gate, 36000U);
        assert(controller.openSession(39000U));
        HWC::Credentials first = makeCredentials("AP-after-cut-123");
        assert(controller.provision(39001U, first) == HWC::CREDENTIAL_PROVISION_RECOVERY_FAILED);
        assert(isWiped(first));
        HWC::Credentials state; HWC::CredentialStore::clear(state);
        assert(store.load(state) == HWC::CREDENTIAL_CORRUPT); // selector is gone; remnants never become valid
        HWC::CredentialStore::clear(state);
        b.failEraseKey.clear();
        gate.samplePhysicalInput(40000U, false);
        authorize(gate, 41000U);
        assert(controller.openSession(44000U));
        HWC::Credentials retry = makeCredentials("AP-recovered-789");
        assert(controller.provision(44001U, retry) == HWC::CREDENTIAL_PROVISIONED);
        assert(isWiped(retry));
        assert(store.load(state) == HWC::CREDENTIAL_VALID);
        HWC::CredentialStore::clear(state);
    }

    // NVS I/O errors are not treated as CORRUPT and are never auto-erased.
    {
        FakeBackend b;
        b.failGet = true;
        HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        authorize(gate, 30000U);
        assert(controller.openSession(33000U));
        HWC::Credentials input = makeCredentials("AP-no-io-12345");
        assert(controller.provision(33001U, input) == HWC::CREDENTIAL_PROVISION_STORAGE_IO_ERROR);
        assert(isWiped(input));
        assert(b.data.empty());
    }

    // A valid store is rotated in the inactive slot; the submitted RAM buffer is wiped.
    {
        FakeBackend b; HWC::CredentialStore store(b);
        HWC::Credentials initial = makeCredentials("AP-old-pass-123");
        HWC::LocalServiceAuthorization initialGate; HWC::CredentialProvisioningController initialController(store, initialGate);
        authorize(initialGate, 39000U);
        assert(initialController.openSession(42000U));
        assert(initialController.provision(42001U, initial) == HWC::CREDENTIAL_PROVISIONED);
        assert(isWiped(initial));
        HWC::LocalServiceAuthorization gate; HWC::CredentialProvisioningController controller(store, gate);
        authorize(gate, 40000U);
        assert(controller.openSession(43000U));
        HWC::Credentials next = makeCredentials("AP-new-pass-456");
        assert(controller.provision(43001U, next) == HWC::CREDENTIAL_PROVISIONED);
        assert(isWiped(next));
        HWC::Credentials loaded; HWC::CredentialStore::clear(loaded);
        assert(store.load(loaded) == HWC::CREDENTIAL_VALID);
        assert(strcmp(loaded.apPassword, "AP-new-pass-456") == 0);
        HWC::CredentialStore::clear(loaded);
        HWC::CredentialStore::clear(initial);
    }

    // Opening a software session near the end of the physical authorization
    // window must not grant a fresh additional 60 seconds.
    {
        FakeBackend b; HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        gate.samplePhysicalInput(100U, true);
        gate.samplePhysicalInput(3100U, true); // authorization window ends at 63100
        assert(controller.openSession(63099U));
        HWC::Credentials input = makeCredentials("AP-too-late-123");
        assert(controller.provision(63100U, input) == HWC::CREDENTIAL_PROVISION_SESSION_EXPIRED);
        assert(isWiped(input));
        assert(b.data.empty());
    }

    // The authorization session expires at the exact 60-second boundary, wrap-safe.
    {
        FakeBackend b; HWC::CredentialStore store(b); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        const uint32_t holdStart = 0xFFFFFF00U;
        authorize(gate, holdStart);
        const uint32_t opened = static_cast<uint32_t>(holdStart + HWC::LocalServiceAuthorization::HOLD_MS);
        assert(controller.openSession(opened));
        const uint32_t expired = static_cast<uint32_t>(opened + HWC::CredentialProvisioningController::SESSION_WINDOW_MS);
        HWC::Credentials input = makeCredentials("AP-expired-pass-1");
        assert(controller.provision(expired, input) == HWC::CREDENTIAL_PROVISION_SESSION_EXPIRED);
        assert(isWiped(input));
        assert(b.data.empty());
    }

    puts("PASS: physical credential provisioning and recovery workflow");
    return 0;
}
