#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "NvsCredentialTextStorage.h"

std::map<std::string, std::string> Preferences::values_;

int main() {
    Preferences::clearAllForTest();
    Preferences preferences;
    assert(preferences.begin("hwc_ap", false));
    HWC::NvsCredentialTextStorage storage(preferences);
    char output[65] = {0};

    assert(HWC::ensureCredentialText(storage, "password", "H12345678", sizeof("H12345678"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_READY);
    assert(strcmp(output, "H12345678") == 0);
    memset(output, 0, sizeof(output));
    assert(HWC::ensureCredentialText(storage, "password", "H87654321", sizeof("H87654321"), 8U, 63U,
                                     output, sizeof(output)) == HWC::CREDENTIAL_STARTUP_READY);
    assert(strcmp(output, "H12345678") == 0); // Existing values are not overwritten.

    char admin[65] = {0};
    assert(HWC::ensureCredentialText(storage, "admin", "Admin-Pass-123", sizeof("Admin-Pass-123"), 12U, 64U,
                                     admin, sizeof(admin)) == HWC::CREDENTIAL_STARTUP_READY);
    assert(strcmp(admin, "Admin-Pass-123") == 0);

    storage.close();
    preferences.end();
    assert(storage.read("password", output, sizeof(output)) == HWC::CREDENTIAL_TEXT_READ_ERROR);
    puts("PASS: Preferences credential text adapter integration");
    return 0;
}
