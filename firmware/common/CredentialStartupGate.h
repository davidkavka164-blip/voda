#ifndef HWC_CREDENTIAL_STARTUP_GATE_H
#define HWC_CREDENTIAL_STARTUP_GATE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace HWC {

enum CredentialTextReadStatus {
    CREDENTIAL_TEXT_FOUND,
    CREDENTIAL_TEXT_MISSING,
    CREDENTIAL_TEXT_READ_ERROR
};

class CredentialTextStorage {
public:
    virtual ~CredentialTextStorage() {}
    virtual CredentialTextReadStatus read(const char* key, char* output, size_t capacity) = 0;
    virtual bool write(const char* key, const char* value) = 0;
};

enum CredentialStartupResult {
    CREDENTIAL_STARTUP_READY,
    CREDENTIAL_STARTUP_INVALID,
    CREDENTIAL_STARTUP_STORAGE_ERROR,
    CREDENTIAL_STARTUP_WRITE_FAILED,
    CREDENTIAL_STARTUP_VERIFY_FAILED
};

inline void clearCredentialText(char* value, size_t capacity) {
    if (value == 0) return;
    volatile char* p = value;
    while (capacity--) *p++ = '\0';
}

inline size_t credentialTextLength(const char* value, size_t capacity) {
    if (value == 0) return capacity;
    size_t length = 0U;
    while (length < capacity && value[length] != '\0') ++length;
    return length;
}

inline bool validCredentialText(const char* value, size_t capacity,
                                size_t minimumLength, size_t maximumLength) {
    if (value == 0 || capacity == 0U || minimumLength == 0U || maximumLength < minimumLength) return false;
    const size_t length = credentialTextLength(value, capacity);
    if (length < minimumLength || length > maximumLength || length >= capacity) return false;
    for (size_t i = 0U; i < length; ++i) {
        const uint8_t ch = static_cast<uint8_t>(value[i]);
        if (ch < 0x20U || ch > 0x7EU) return false;
    }
    return true;
}

// Existing values are never silently overwritten. A missing value may be
// created from a generated default, but only after an exact read-back check.
// A write return code is not authoritative if the subsequent read-back matches.
inline CredentialStartupResult ensureCredentialText(
    CredentialTextStorage& storage, const char* key,
    const char* generatedDefault, size_t generatedDefaultCapacity,
    size_t minimumLength, size_t maximumLength, char* output, size_t outputCapacity) {
    if (output == 0 || outputCapacity == 0U) return CREDENTIAL_STARTUP_INVALID;
    clearCredentialText(output, outputCapacity);
    if (key == 0 || key[0] == '\0' || minimumLength == 0U ||
        maximumLength < minimumLength || maximumLength > 64U || outputCapacity <= maximumLength) {
        return CREDENTIAL_STARTUP_INVALID;
    }

    char observed[65] = {0};
    CredentialTextReadStatus readStatus = storage.read(key, observed, sizeof(observed));
    if (readStatus != CREDENTIAL_TEXT_FOUND && readStatus != CREDENTIAL_TEXT_MISSING) {
        clearCredentialText(observed, sizeof(observed));
        return CREDENTIAL_STARTUP_STORAGE_ERROR;
    }
    if (readStatus == CREDENTIAL_TEXT_FOUND) {
        if (!validCredentialText(observed, sizeof(observed), minimumLength, maximumLength) ||
            credentialTextLength(observed, sizeof(observed)) >= outputCapacity) {
            clearCredentialText(observed, sizeof(observed));
            return CREDENTIAL_STARTUP_INVALID;
        }
        memcpy(output, observed, credentialTextLength(observed, sizeof(observed)) + 1U);
        clearCredentialText(observed, sizeof(observed));
        return CREDENTIAL_STARTUP_READY;
    }

    if (!validCredentialText(generatedDefault, generatedDefaultCapacity,
                             minimumLength, maximumLength)) {
        clearCredentialText(observed, sizeof(observed));
        return CREDENTIAL_STARTUP_INVALID;
    }

    const bool writeReportedSuccess = storage.write(key, generatedDefault);
    readStatus = storage.read(key, observed, sizeof(observed));
    if (readStatus != CREDENTIAL_TEXT_FOUND && readStatus != CREDENTIAL_TEXT_MISSING) {
        clearCredentialText(observed, sizeof(observed));
        return CREDENTIAL_STARTUP_STORAGE_ERROR;
    }
    if (readStatus != CREDENTIAL_TEXT_FOUND) {
        clearCredentialText(observed, sizeof(observed));
        return writeReportedSuccess ? CREDENTIAL_STARTUP_VERIFY_FAILED : CREDENTIAL_STARTUP_WRITE_FAILED;
    }

    const bool valid = validCredentialText(observed, sizeof(observed), minimumLength, maximumLength);
    const bool exact = valid && strcmp(observed, generatedDefault) == 0;
    if (exact && credentialTextLength(observed, sizeof(observed)) < outputCapacity) {
        memcpy(output, observed, credentialTextLength(observed, sizeof(observed)) + 1U);
        clearCredentialText(observed, sizeof(observed));
        return CREDENTIAL_STARTUP_READY;
    }

    clearCredentialText(observed, sizeof(observed));
    return writeReportedSuccess ? CREDENTIAL_STARTUP_VERIFY_FAILED : CREDENTIAL_STARTUP_WRITE_FAILED;
}

} // namespace HWC
#endif
