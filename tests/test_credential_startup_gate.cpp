#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "CredentialStartupGate.h"

class FakeCredentialTextStorage : public HWC::CredentialTextStorage {
public:
    FakeCredentialTextStorage()
        : present(false), readError(false), writeError(false), commitThenReportFailure(false),
          corruptAfterWrite(false), writes(0U), value() {}

    HWC::CredentialTextReadStatus read(const char*, char* output, size_t capacity) {
        if (output == 0 || capacity == 0U || readError) return HWC::CREDENTIAL_TEXT_READ_ERROR;
        output[0] = '\0';
        if (!present) return HWC::CREDENTIAL_TEXT_MISSING;
        if (value.size() >= capacity) return HWC::CREDENTIAL_TEXT_READ_ERROR;
        memcpy(output, value.c_str(), value.size() + 1U);
        return HWC::CREDENTIAL_TEXT_FOUND;
    }

    bool write(const char*, const char* text) {
        ++writes;
        if (writeError && !commitThenReportFailure) return false;
        value = text ? text : "";
        if (corruptAfterWrite && !value.empty()) value[value.size() - 1U] = 'X';
        present = true;
        return !writeError;
    }

    bool present;
    bool readError;
    bool writeError;
    bool commitThenReportFailure;
    bool corruptAfterWrite;
    unsigned writes;
    std::string value;
};

int main() {
    char output[65];
    memset(output, 0xA5, sizeof(output));

    // Existing valid value is loaded exactly and is never rewritten.
    FakeCredentialTextStorage existing;
    existing.present = true;
    existing.value = "Existing-Admin-Password";
    assert(HWC::ensureCredentialText(existing, "admin", "Generated-Admin-12", sizeof("Generated-Admin-12"), 12U, 64U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_READY);
    assert(strcmp(output, "Existing-Admin-Password") == 0);
    assert(existing.writes == 0U);

    // First boot: missing value is generated, written, and verified by read-back.
    FakeCredentialTextStorage firstBoot;
    assert(HWC::ensureCredentialText(firstBoot, "ap", "H01234567", sizeof("H01234567"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_READY);
    assert(firstBoot.present && firstBoot.writes == 1U);
    assert(strcmp(output, "H01234567") == 0);

    // A backend may commit and still report failure; exact read-back is authoritative.
    FakeCredentialTextStorage commitThenError;
    commitThenError.writeError = true;
    commitThenError.commitThenReportFailure = true;
    assert(HWC::ensureCredentialText(commitThenError, "ap", "H76543210", sizeof("H76543210"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_READY);
    assert(strcmp(output, "H76543210") == 0);

    // If the write did not persist, startup must fail closed.
    FakeCredentialTextStorage noWrite;
    noWrite.writeError = true;
    assert(HWC::ensureCredentialText(noWrite, "ap", "H76543210", sizeof("H76543210"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_WRITE_FAILED);
    assert(output[0] == '\0');

    // A successful API return with altered read-back is not accepted.
    FakeCredentialTextStorage corrupted;
    corrupted.corruptAfterWrite = true;
    assert(HWC::ensureCredentialText(corrupted, "ap", "H76543210", sizeof("H76543210"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_VERIFY_FAILED);
    assert(output[0] == '\0');

    // Existing malformed credentials are never silently replaced.
    FakeCredentialTextStorage malformed;
    malformed.present = true;
    malformed.value = "short";
    assert(HWC::ensureCredentialText(malformed, "ap", "H76543210", sizeof("H76543210"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_INVALID);
    assert(malformed.writes == 0U && output[0] == '\0');

    // NVS read errors block startup, even when a generated fallback exists.
    FakeCredentialTextStorage unavailable;
    unavailable.readError = true;
    assert(HWC::ensureCredentialText(unavailable, "ap", "H76543210", sizeof("H76543210"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_STORAGE_ERROR);
    assert(unavailable.writes == 0U && output[0] == '\0');

    // AP passphrase boundary: 8 and 63 accepted, 7 and 64 rejected.
    assert(HWC::validCredentialText("12345678", 9U, 8U, 63U));
    char maxApPassword[64];
    memset(maxApPassword, 'P', 63U); maxApPassword[63] = '\0';
    assert(HWC::validCredentialText(maxApPassword, sizeof(maxApPassword), 8U, 63U));
    char tooLongApPassword[65];
    memset(tooLongApPassword, 'P', 64U); tooLongApPassword[64] = '\0';
    assert(!HWC::validCredentialText(tooLongApPassword, sizeof(tooLongApPassword), 8U, 63U));
    assert(!HWC::validCredentialText("1234567", 8U, 8U, 63U));

    // Administrator password boundary: 12 through 64.
    char maxAdminPassword[65];
    memset(maxAdminPassword, 'A', 64U); maxAdminPassword[64] = '\0';
    assert(HWC::validCredentialText(maxAdminPassword, sizeof(maxAdminPassword), 12U, 64U));
    assert(!HWC::validCredentialText("short-admin", 12U, 12U, 64U));

    // Reject non-printable and non-ASCII bytes.
    const char controlText[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', '\n', '\0'};
    const char nonAsciiText[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', static_cast<char>(0x80), '\0'};
    assert(!HWC::validCredentialText(controlText, sizeof(controlText), 8U, 32U));
    assert(!HWC::validCredentialText(nonAsciiText, sizeof(nonAsciiText), 8U, 32U));

    // The helper's bounded temporary storage deliberately caps supported values at 64.
    FakeCredentialTextStorage oversizedPolicy;
    assert(HWC::ensureCredentialText(oversizedPolicy, "ap", "H76543210", sizeof("H76543210"),
                                     8U, 65U, output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_INVALID);

    // Output buffer must have room for max length plus NUL.
    char tooSmall[63];
    assert(HWC::ensureCredentialText(firstBoot, "ap", "H01234567", sizeof("H01234567"), 8U, 63U,
                                     tooSmall, sizeof(tooSmall)) == HWC::CREDENTIAL_STARTUP_INVALID);

    puts("PASS: credential startup gate read/write/read-back and fail-closed boundaries");
    return 0;
}
