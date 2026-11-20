#ifndef HWC_NVS_CREDENTIAL_TEXT_STORAGE_H
#define HWC_NVS_CREDENTIAL_TEXT_STORAGE_H

#include <Preferences.h>
#include <string.h>
#include "CredentialStartupGate.h"

namespace HWC {

// Thin adapter for an already-open Preferences namespace. It preserves the
// existing legacy NVS keys and adds exact read-back through CredentialStartupGate.
// This is not encryption; confidentiality depends on platform flash protection.
class NvsCredentialTextStorage : public CredentialTextStorage {
public:
    explicit NvsCredentialTextStorage(Preferences& preferences) : preferences_(preferences), active_(true) {}

    void close() { active_ = false; }

    CredentialTextReadStatus read(const char* key, char* output, size_t capacity) {
        if (!active_ || key == 0 || output == 0 || capacity == 0U) return CREDENTIAL_TEXT_READ_ERROR;
        clearCredentialText(output, capacity);
        if (!preferences_.isKey(key)) return CREDENTIAL_TEXT_MISSING;
        const String value = preferences_.getString(key, "");
        const size_t length = value.length();
        if (length >= capacity) return CREDENTIAL_TEXT_READ_ERROR;
        if (length != 0U) memcpy(output, value.c_str(), length);
        output[length] = '\0';
        return CREDENTIAL_TEXT_FOUND;
    }

    bool write(const char* key, const char* value) {
        if (!active_ || key == 0 || value == 0) return false;
        return preferences_.putString(key, value) > 0U;
    }

private:
    Preferences& preferences_;
    bool active_;
};

} // namespace HWC
#endif
