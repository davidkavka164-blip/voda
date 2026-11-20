#include "NvsCredentialBackend.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

bool Preferences::present = false;
size_t Preferences::length = 0;
unsigned char Preferences::storage[4096] = {0};
char Preferences::lastNamespace[32] = {0};

int main() {
    Preferences::clearForTest();
    HWC::NvsCredentialBackend backend("hwcCred");
    char value[65] = {0};
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_ERROR);
    assert(backend.begin());
    assert(strcmp(Preferences::lastNamespace, "hwcCred") == 0);
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_MISSING);
    assert(backend.erase("missing"));
    assert(backend.put("ap", "AP-password-123"));
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_FOUND);
    assert(strcmp(value, "AP-password-123") == 0);

    // A stored byte sequence must be a single, NUL-terminated C string.
    Preferences::storage[0] = 'A';
    Preferences::storage[1] = '\0';
    Preferences::storage[2] = 'B';
    Preferences::storage[3] = '\0';
    Preferences::length = 4;
    Preferences::present = true;
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_ERROR);

    // Missing final NUL must never be exposed as a valid credential.
    Preferences::storage[0] = 'A';
    Preferences::storage[1] = 'B';
    Preferences::length = 2;
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_ERROR);

    // A payload larger than the caller's output buffer is rejected.
    memset(Preferences::storage, 'X', 70);
    Preferences::length = 70;
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_ERROR);

    // A one-byte empty C string is syntactically valid at adapter level;
    // CredentialStore policy rejects it as an invalid AP/admin credential.
    Preferences::storage[0] = '\0';
    Preferences::length = 1;
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_FOUND);
    assert(value[0] == '\0');

    assert(!backend.put("tooLong", "123456789012345678901234567890123456789012345678901234567890123456"));
    assert(!backend.put("null", 0));
    assert(backend.erase("ap"));
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_MISSING);
    backend.end();
    assert(!backend.isOpen());
    assert(backend.get("ap", value, sizeof(value)) == HWC::CREDENTIAL_READ_ERROR);

    HWC::NvsCredentialBackend invalidNamespace("");
    assert(!invalidNamespace.begin());
    puts("PASS: Preferences NVS credential adapter tests (malformed payloads, bounds, lifecycle)");
}
