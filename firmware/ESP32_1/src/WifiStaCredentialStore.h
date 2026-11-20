#ifndef HWC_WIFI_STA_CREDENTIAL_STORE_H
#define HWC_WIFI_STA_CREDENTIAL_STORE_H

#include "CredentialStore.h"
#include <string.h>

namespace HWC {

enum WifiStaCredentialStatus {
    WIFI_STA_CREDENTIAL_VALID,
    WIFI_STA_CREDENTIAL_EMPTY,
    WIFI_STA_CREDENTIAL_CORRUPT,
    WIFI_STA_CREDENTIAL_IO_ERROR
};

struct WifiStaCredentials {
    char ssid[33];
    char password[65];
};

// Transactional two-slot storage for the home Wi-Fi STA pair. The legacy
// `ssid`/`password` keys are retained after migration for rollback/audit.
class WifiStaCredentialStore {
public:
    explicit WifiStaCredentialStore(CredentialBackend& backend) : backend_(backend) {}

    static void clear(WifiStaCredentials& value) {
        volatile char* p = reinterpret_cast<volatile char*>(&value);
        for (size_t i = 0; i < sizeof(value); ++i) p[i] = '\0';
    }

    static bool valid(const WifiStaCredentials& value) {
        return bounded(value.ssid, sizeof(value.ssid), 1U, 32U) &&
               bounded(value.password, sizeof(value.password), 0U, 64U);
    }

    WifiStaCredentialStatus load(WifiStaCredentials& out) {
        clear(out);
        char marker[8] = {0};
        const CredentialReadResult markerResult = backend_.get("sta_active", marker, sizeof(marker));
        if (markerResult == CREDENTIAL_READ_ERROR) return WIFI_STA_CREDENTIAL_IO_ERROR;
        if (markerResult == CREDENTIAL_READ_FOUND) {
            if (strcmp(marker, "0") != 0 && strcmp(marker, "1") != 0) return WIFI_STA_CREDENTIAL_CORRUPT;
            WifiStaCredentials candidate;
            clear(candidate);
            const bool ok = readSlot(static_cast<unsigned>(marker[0] - '0'), candidate);
            if (ok) copy(out, candidate);
            clear(candidate);
            return ok ? WIFI_STA_CREDENTIAL_VALID : WIFI_STA_CREDENTIAL_CORRUPT;
        }

        // Without a selector no slot is committed. If legacy data still
        // exists, it remains authoritative and can safely repair an interrupted
        // first migration by rewriting slot 0 before committing the selector.
        bool slotData = false;
        const char* slotKeys[4] = {"s0_ssid", "s0_pass", "s1_ssid", "s1_pass"};
        for (unsigned i = 0; i < 4; ++i) {
            char scratch[65] = {0};
            const CredentialReadResult r = backend_.get(slotKeys[i], scratch, sizeof(scratch));
            volatile char* wipe = scratch;
            for (size_t j = 0; j < sizeof(scratch); ++j) wipe[j] = '\0';
            if (r == CREDENTIAL_READ_ERROR) return WIFI_STA_CREDENTIAL_IO_ERROR;
            if (r == CREDENTIAL_READ_FOUND) slotData = true;
        }
        WifiStaCredentials legacy;
        clear(legacy);
        const CredentialReadResult ssidResult = backend_.get("ssid", legacy.ssid, sizeof(legacy.ssid));
        const CredentialReadResult passResult = backend_.get("password", legacy.password, sizeof(legacy.password));
        if (ssidResult == CREDENTIAL_READ_ERROR || passResult == CREDENTIAL_READ_ERROR) {
            clear(legacy); return WIFI_STA_CREDENTIAL_IO_ERROR;
        }
        if (ssidResult == CREDENTIAL_READ_MISSING && passResult == CREDENTIAL_READ_MISSING) {
            clear(legacy);
            return slotData ? WIFI_STA_CREDENTIAL_CORRUPT : WIFI_STA_CREDENTIAL_EMPTY;
        }
        if (ssidResult != CREDENTIAL_READ_FOUND || passResult != CREDENTIAL_READ_FOUND || !valid(legacy)) {
            clear(legacy); return WIFI_STA_CREDENTIAL_CORRUPT;
        }
        const bool migrated = save(legacy);
        if (migrated) copy(out, legacy);
        clear(legacy);
        return migrated ? WIFI_STA_CREDENTIAL_VALID : WIFI_STA_CREDENTIAL_IO_ERROR;
    }

    bool save(const WifiStaCredentials& value) {
        if (!valid(value)) return false;
        char marker[8] = {0};
        const CredentialReadResult markerResult = backend_.get("sta_active", marker, sizeof(marker));
        if (markerResult == CREDENTIAL_READ_ERROR) return false;
        unsigned target = 0U;
        if (markerResult == CREDENTIAL_READ_FOUND) {
            if (strcmp(marker, "0") == 0) target = 1U;
            else if (strcmp(marker, "1") == 0) target = 0U;
            else return false;
        } else {
            // With no selector there is no committed slot. Slot 0 is therefore
            // safe to rewrite even if a previous first-save was interrupted.
            target = 0U;
        }

        const char* ssidKey = target == 0U ? "s0_ssid" : "s1_ssid";
        const char* passKey = target == 0U ? "s0_pass" : "s1_pass";
        backend_.put(ssidKey, value.ssid);
        if (!matches(ssidKey, value.ssid)) return false;
        backend_.put(passKey, value.password);
        if (!matches(passKey, value.password)) return false;
        WifiStaCredentials staged;
        clear(staged);
        const bool stagedOk = readSlot(target, staged) && same(staged, value);
        clear(staged);
        if (!stagedOk) return false;

        const char* nextMarker = target == 0U ? "0" : "1";
        backend_.put("sta_active", nextMarker);
        char verify[8] = {0};
        const bool committed = backend_.get("sta_active", verify, sizeof(verify)) == CREDENTIAL_READ_FOUND &&
                               strcmp(verify, nextMarker) == 0;
        if (!committed) return false;
        WifiStaCredentials finalCheck;
        clear(finalCheck);
        const bool finalOk = readSlot(target, finalCheck) && same(finalCheck, value);
        clear(finalCheck);
        if (finalOk) return true;

        // If post-commit verification becomes uncertain, restore the previous
        // selector (or remove the first selector) so the new candidate is not
        // left selected after a failed save result.
        if (markerResult == CREDENTIAL_READ_FOUND) {
            backend_.put("sta_active", marker);
            char restored[8] = {0};
            const bool rollbackOk = backend_.get("sta_active", restored, sizeof(restored)) == CREDENTIAL_READ_FOUND &&
                                    strcmp(restored, marker) == 0;
            (void)rollbackOk;
            return false;
        }
        backend_.erase("sta_active");
        char absent[8] = {0};
        const bool rollbackOk = backend_.get("sta_active", absent, sizeof(absent)) == CREDENTIAL_READ_MISSING;
        (void)rollbackOk;
        return false;
    }

private:
    CredentialBackend& backend_;

    static bool bounded(const char* value, size_t capacity, size_t minimum, size_t maximum) {
        if (!value || capacity == 0U) return false;
        size_t n = 0U;
        while (n < capacity && value[n] != '\0') ++n;
        return n >= minimum && n <= maximum && n < capacity;
    }
    static void copy(WifiStaCredentials& dst, const WifiStaCredentials& src) {
        memcpy(dst.ssid, src.ssid, sizeof(dst.ssid));
        memcpy(dst.password, src.password, sizeof(dst.password));
    }
    static bool same(const WifiStaCredentials& a, const WifiStaCredentials& b) {
        return strcmp(a.ssid, b.ssid) == 0 && strcmp(a.password, b.password) == 0;
    }
    bool matches(const char* key, const char* expected) {
        char actual[65] = {0};
        const bool ok = backend_.get(key, actual, sizeof(actual)) == CREDENTIAL_READ_FOUND &&
                        strcmp(actual, expected) == 0;
        volatile char* p = actual;
        for (size_t i = 0; i < sizeof(actual); ++i) p[i] = '\0';
        return ok;
    }
    bool readSlot(unsigned slot, WifiStaCredentials& out) {
        const char* ssidKey = slot == 0U ? "s0_ssid" : "s1_ssid";
        const char* passKey = slot == 0U ? "s0_pass" : "s1_pass";
        const CredentialReadResult a = backend_.get(ssidKey, out.ssid, sizeof(out.ssid));
        const CredentialReadResult b = backend_.get(passKey, out.password, sizeof(out.password));
        return a == CREDENTIAL_READ_FOUND && b == CREDENTIAL_READ_FOUND && valid(out);
    }
};

} // namespace HWC
#endif
