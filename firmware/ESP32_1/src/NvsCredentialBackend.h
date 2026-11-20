#ifndef HWC_NVS_CREDENTIAL_BACKEND_H
#define HWC_NVS_CREDENTIAL_BACKEND_H

#include "CredentialStore.h"
#include <Preferences.h>
#include <string.h>

namespace HWC {

// Arduino-ESP32 Preferences adapter. Credentials are stored as NUL-terminated
// byte strings in a dedicated namespace. This is NOT encryption; production
// confidentiality still depends on platform flash/NVS protection.
class NvsCredentialBackend : public CredentialBackend {
public:
    explicit NvsCredentialBackend(const char* nameSpace = "hwcCred")
        : opened_(false), nameSpace_(nameSpace) {}

    bool begin() {
        if (opened_) return true;
        if (nameSpace_ == 0 || nameSpace_[0] == '\0') return false;
        opened_ = preferences_.begin(nameSpace_, false);
        return opened_;
    }

    void end() {
        if (opened_) preferences_.end();
        opened_ = false;
    }

    bool isOpen() const { return opened_; }

    CredentialReadResult get(const char* key, char* out, size_t capacity) {
        if (!opened_ || key == 0 || out == 0 || capacity == 0) return CREDENTIAL_READ_ERROR;
        out[0] = '\0';
        if (!preferences_.isKey(key)) return CREDENTIAL_READ_MISSING;
        const size_t length = preferences_.getBytesLength(key);
        if (length == 0 || length > capacity) return CREDENTIAL_READ_ERROR;
        const size_t read = preferences_.getBytes(key, out, length);
        if (read != length || out[length - 1] != '\0') {
            out[0] = '\0';
            return CREDENTIAL_READ_ERROR;
        }
        // Reject embedded NULs and malformed/non-string payloads.
        if (strlen(out) + 1U != length) {
            out[0] = '\0';
            return CREDENTIAL_READ_ERROR;
        }
        return CREDENTIAL_READ_FOUND;
    }

    bool put(const char* key, const char* value) {
        if (!opened_ || key == 0 || value == 0) return false;
        const size_t length = strlen(value) + 1U;
        if (length > 65U) return false;
        return preferences_.putBytes(key, value, length) == length;
    }

    bool erase(const char* key) {
        if (!opened_ || key == 0) return false;
        if (!preferences_.isKey(key)) return true;
        return preferences_.remove(key);
    }

private:
    Preferences preferences_;
    bool opened_;
    const char* nameSpace_;
};

} // namespace HWC
#endif
