#ifndef HWC_CREDENTIAL_PROVISIONING_CONTROLLER_H
#define HWC_CREDENTIAL_PROVISIONING_CONTROLLER_H

#include <stdint.h>
#include "CredentialStore.h"
#include "LocalServiceAuthorization.h"

namespace HWC {

enum CredentialProvisioningResult {
    CREDENTIAL_PROVISIONED,
    CREDENTIAL_PROVISION_SESSION_CLOSED,
    CREDENTIAL_PROVISION_SESSION_EXPIRED,
    CREDENTIAL_PROVISION_INVALID_INPUT,
    CREDENTIAL_PROVISION_STORAGE_IO_ERROR,
    CREDENTIAL_PROVISION_RECOVERY_FAILED,
    CREDENTIAL_PROVISION_SAVE_FAILED
};

// Transport-independent, single-attempt service workflow. A board adapter is
// responsible for sampling the real physical input. Network/HTTP/UDP handlers
// must never call samplePhysicalInput() or manufacture authorization state.
// The controller consumes the one-shot physical authorization itself.
class CredentialProvisioningController {
public:
    static const uint32_t SESSION_WINDOW_MS = LocalServiceAuthorization::WINDOW_MS;

    CredentialProvisioningController(CredentialStore& store, LocalServiceAuthorization& authorization)
        : store_(store), authorization_(authorization), active_(false), authorizationStartedAt_(0U) {}

    bool openSession(uint32_t nowMs) {
        closeSession();
        uint32_t authorizationStartedAt = 0U;
        if (!authorization_.consume(nowMs, &authorizationStartedAt)) return false;
        active_ = true;
        authorizationStartedAt_ = authorizationStartedAt;
        return true;
    }

    bool isSessionOpen(uint32_t nowMs) {
        if (!active_) return false;
        if (elapsed(nowMs, authorizationStartedAt_) >= SESSION_WINDOW_MS) {
            closeSession();
            return false;
        }
        return true;
    }

    void closeSession() {
        active_ = false;
        authorizationStartedAt_ = 0U;
    }

    // The caller must pass a mutable temporary, not a persistent application
    // object. This function wipes that input on every return path and never
    // returns or logs the stored values.
    CredentialProvisioningResult provision(uint32_t nowMs, Credentials& submitted) {
        if (!active_) {
            CredentialStore::clear(submitted);
            return CREDENTIAL_PROVISION_SESSION_CLOSED;
        }
        if (elapsed(nowMs, authorizationStartedAt_) >= SESSION_WINDOW_MS) {
            closeSession();
            CredentialStore::clear(submitted);
            return CREDENTIAL_PROVISION_SESSION_EXPIRED;
        }
        // Every accepted provisioning attempt closes the session on every
        // outcome, so the same physical authorization can never be reused.
        if (!CredentialStore::validate(submitted)) {
            CredentialStore::clear(submitted);
            closeSession();
            return CREDENTIAL_PROVISION_INVALID_INPUT;
        }

        Credentials previous;
        CredentialStore::clear(previous);
        const CredentialStoreStatus before = store_.load(previous);
        CredentialStore::clear(previous);
        if (before == CREDENTIAL_IO_ERROR) {
            CredentialStore::clear(submitted);
            closeSession();
            return CREDENTIAL_PROVISION_STORAGE_IO_ERROR;
        }
        if (before == CREDENTIAL_CORRUPT && !store_.eraseAllForPhysicalRecovery()) {
            Credentials state;
            CredentialStore::clear(state);
            const CredentialStoreStatus afterRecovery = store_.load(state);
            CredentialStore::clear(state);
            CredentialStore::clear(submitted);
            closeSession();
            return afterRecovery == CREDENTIAL_IO_ERROR ? CREDENTIAL_PROVISION_STORAGE_IO_ERROR : CREDENTIAL_PROVISION_RECOVERY_FAILED;
        }

        const bool saveReportedSuccess = store_.saveInternal(submitted);
        Credentials verified;
        CredentialStore::clear(verified);
        const CredentialStoreStatus after = store_.load(verified);
        const bool exact = after == CREDENTIAL_VALID && CredentialStore::same(verified, submitted);
        CredentialStore::clear(verified);
        CredentialStore::clear(submitted);
        closeSession();

        if (exact) return CREDENTIAL_PROVISIONED;
        if (after == CREDENTIAL_IO_ERROR) return CREDENTIAL_PROVISION_STORAGE_IO_ERROR;
        (void)saveReportedSuccess; // read-back state is authoritative, not the return code alone
        return CREDENTIAL_PROVISION_SAVE_FAILED;
    }

private:
    CredentialStore& store_;
    LocalServiceAuthorization& authorization_;
    bool active_;
    uint32_t authorizationStartedAt_;

    static uint32_t elapsed(uint32_t now, uint32_t then) {
        return static_cast<uint32_t>(now - then);
    }
};

} // namespace HWC
#endif
