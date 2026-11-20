#ifndef HWC_CREDENTIAL_STORE_H
#define HWC_CREDENTIAL_STORE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace HWC {

enum CredentialStoreStatus { CREDENTIAL_VALID, CREDENTIAL_EMPTY, CREDENTIAL_CORRUPT, CREDENTIAL_IO_ERROR };

struct Credentials {
    char apName[33];
    char apPassword[65];
    char adminPassword[65];
};

enum CredentialReadResult { CREDENTIAL_READ_FOUND, CREDENTIAL_READ_MISSING, CREDENTIAL_READ_ERROR };

class CredentialBackend {
public:
    virtual ~CredentialBackend() {}
    virtual CredentialReadResult get(const char* key, char* out, size_t capacity) = 0;
    virtual bool put(const char* key, const char* value) = 0;
    // Erasing an already missing key must also return true (idempotent).
    virtual bool erase(const char* key) = 0;
};

class CredentialProvisioningController;
class CredentialLegacyMigrator;

class CredentialStore {
public:
    explicit CredentialStore(CredentialBackend& backend) : backend_(backend) {}

    CredentialStoreStatus load(Credentials& out) {
        clear(out);
        char active[8] = {0};
        const CredentialReadResult activeResult = backend_.get("cred_active", active, sizeof(active));
        if (activeResult == CREDENTIAL_READ_ERROR) return CREDENTIAL_IO_ERROR;
        if (activeResult == CREDENTIAL_READ_MISSING) {
            bool any = false;
            const char* keys[6] = {"c0_name", "c0_ap", "c0_admin", "c1_name", "c1_ap", "c1_admin"};
            for (unsigned i = 0; i < 6; ++i) {
                char value[65] = {0};
                const CredentialReadResult r = backend_.get(keys[i], value, sizeof(value));
                secureClear(value, sizeof(value));
                if (r == CREDENTIAL_READ_ERROR) return CREDENTIAL_IO_ERROR;
                if (r == CREDENTIAL_READ_FOUND) any = true;
            }
            return any ? CREDENTIAL_CORRUPT : CREDENTIAL_EMPTY;
        }
        if (strcmp(active, "0") != 0 && strcmp(active, "1") != 0) return CREDENTIAL_CORRUPT;
        const unsigned slot = static_cast<unsigned>(active[0] - '0');
        Credentials candidate;
        clear(candidate);
        const CredentialStoreStatus slotStatus = readSlotStatus(slot, candidate);
        if (slotStatus != CREDENTIAL_VALID) {
            clear(candidate);
            return slotStatus;
        }
        out = candidate;
        clear(candidate);
        return CREDENTIAL_VALID;
    }

    static void clear(Credentials& value) { secureClear(&value, sizeof(value)); }

    static bool validate(const Credentials& c) {
        return bounded(c.apName, sizeof(c.apName), 8U, 32U) &&
               bounded(c.apPassword, sizeof(c.apPassword), 8U, 63U) &&
               bounded(c.adminPassword, sizeof(c.adminPassword), 12U, 64U);
    }

private:
    friend class CredentialProvisioningController;
    friend class CredentialLegacyMigrator;

    bool saveInternal(const Credentials& value) {
        if (!validate(value)) return false;
        char active[8] = {0};
        const CredentialReadResult activeResult = backend_.get("cred_active", active, sizeof(active));
        if (activeResult == CREDENTIAL_READ_ERROR) return false;

        unsigned target = 0;
        if (activeResult == CREDENTIAL_READ_FOUND) {
            if (strcmp(active, "0") == 0) target = 1;
            else if (strcmp(active, "1") == 0) target = 0;
            else return false;
        } else {
            Credentials current;
            clear(current);
            const CredentialStoreStatus currentStatus = load(current);
            clear(current);
            if (currentStatus != CREDENTIAL_EMPTY) return false;
        }

        const char* n = key(target, 'n');
        const char* a = key(target, 'a');
        const char* d = key(target, 'd');
        if (!backend_.put(n, value.apName) || !matches(n, value.apName) ||
            !backend_.put(a, value.apPassword) || !matches(a, value.apPassword) ||
            !backend_.put(d, value.adminPassword) || !matches(d, value.adminPassword)) {
            invalidate(target);
            return false;
        }
        Credentials staged;
        clear(staged);
        const bool stagedOk = readSlot(target, staged) && same(staged, value);
        clear(staged);
        if (!stagedOk) {
            invalidate(target);
            return false;
        }

        const char* marker = target == 0 ? "0" : "1";
        // A failed return code may follow a completed flash write. The exact
        // selector read-back determines whether the commit reached storage.
        backend_.put("cred_active", marker);
        char verify[8] = {0};
        const bool selectorCommitted = backend_.get("cred_active", verify, sizeof(verify)) == CREDENTIAL_READ_FOUND &&
                                       strcmp(verify, marker) == 0;
        secureClear(verify, sizeof(verify));
        if (!selectorCommitted) return false;
        Credentials committed;
        clear(committed);
        const bool committedOk = readSlot(target, committed) && same(committed, value);
        clear(committed);
        return committedOk;
    }

    CredentialBackend& backend_;

    static void secureClear(void* data, size_t length) {
        volatile uint8_t* p = static_cast<volatile uint8_t*>(data);
        while (length--) *p++ = 0U;
    }

    static const char* key(unsigned slot, char field) {
        static const char* keys[2][3] = {{"c0_name", "c0_ap", "c0_admin"}, {"c1_name", "c1_ap", "c1_admin"}};
        const unsigned index = field == 'n' ? 0U : (field == 'a' ? 1U : 2U);
        return keys[slot & 1U][index];
    }

    static bool bounded(const char* s, size_t cap, size_t min, size_t max) {
        if (s == 0 || cap == 0U || min == 0U || max < min) return false;
        size_t n = 0U;
        while (n < cap && s[n] != '\0') ++n;
        if (n < min || n > max || n >= cap) return false;
        for (size_t i = 0U; i < n; ++i) {
            const uint8_t ch = static_cast<uint8_t>(s[i]);
            if (ch < 0x20U || ch > 0x7EU) return false;
        }
        return true;
    }

    bool invalidate(unsigned slot) {
        const bool a = backend_.erase(key(slot, 'n'));
        const bool b = backend_.erase(key(slot, 'a'));
        const bool c = backend_.erase(key(slot, 'd'));
        return a && b && c;
    }

    bool matches(const char* k, const char* expected) {
        char actual[65] = {0};
        const bool ok = backend_.get(k, actual, sizeof(actual)) == CREDENTIAL_READ_FOUND && strcmp(actual, expected) == 0;
        secureClear(actual, sizeof(actual));
        return ok;
    }

    CredentialStoreStatus readSlotStatus(unsigned slot, Credentials& out) {
        const CredentialReadResult n = backend_.get(key(slot, 'n'), out.apName, sizeof(out.apName));
        const CredentialReadResult a = backend_.get(key(slot, 'a'), out.apPassword, sizeof(out.apPassword));
        const CredentialReadResult d = backend_.get(key(slot, 'd'), out.adminPassword, sizeof(out.adminPassword));
        if (n == CREDENTIAL_READ_ERROR || a == CREDENTIAL_READ_ERROR || d == CREDENTIAL_READ_ERROR) return CREDENTIAL_IO_ERROR;
        if (n != CREDENTIAL_READ_FOUND || a != CREDENTIAL_READ_FOUND || d != CREDENTIAL_READ_FOUND || !validate(out)) return CREDENTIAL_CORRUPT;
        return CREDENTIAL_VALID;
    }

    bool readSlot(unsigned slot, Credentials& out) { return readSlotStatus(slot, out) == CREDENTIAL_VALID; }

    static bool same(const Credentials& a, const Credentials& b) {
        return strcmp(a.apName, b.apName) == 0 && strcmp(a.apPassword, b.apPassword) == 0 &&
               strcmp(a.adminPassword, b.adminPassword) == 0;
    }

    // Only the physically authorized provisioning controller may call this.
    // Selector is removed and verified first, so a power cut during recovery
    // can only leave a fail-closed CORRUPT state, never a selected partial slot.
    bool eraseAllForPhysicalRecovery() {
        if (!backend_.erase("cred_active")) return false;
        char selector[8] = {0};
        const CredentialReadResult selectorResult = backend_.get("cred_active", selector, sizeof(selector));
        secureClear(selector, sizeof(selector));
        if (selectorResult != CREDENTIAL_READ_MISSING) return false;

        const char* keys[6] = {"c0_name", "c0_ap", "c0_admin", "c1_name", "c1_ap", "c1_admin"};
        for (unsigned i = 0; i < 6; ++i) {
            if (!backend_.erase(keys[i])) return false;
            char value[65] = {0};
            const CredentialReadResult r = backend_.get(keys[i], value, sizeof(value));
            secureClear(value, sizeof(value));
            if (r != CREDENTIAL_READ_MISSING) return false;
        }
        Credentials check;
        clear(check);
        const CredentialStoreStatus status = load(check);
        clear(check);
        return status == CREDENTIAL_EMPTY;
    }
};

} // namespace HWC
#endif
