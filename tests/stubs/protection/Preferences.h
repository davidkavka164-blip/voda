#ifndef HWC_PROTECTION_TEST_PREFERENCES_H
#define HWC_PROTECTION_TEST_PREFERENCES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Isolated multi-key NVS model used only by protection-store tests.
class Preferences {
public:
    struct Entry {
        bool used;
        char nameSpace[16];
        char key[16];
        size_t length;
        uint8_t bytes[256];
    };

    Preferences() : opened_(false) { nameSpace_[0] = '\0'; }
    bool begin(const char* nameSpace, bool) {
        if (!nameSpace || !nameSpace[0] || strlen(nameSpace) > 15) return false;
        strncpy(nameSpace_, nameSpace, sizeof(nameSpace_) - 1);
        nameSpace_[sizeof(nameSpace_) - 1] = '\0';
        opened_ = true;
        return true;
    }
    void end() { opened_ = false; }
    bool isKey(const char* key) const { return find(key) != nullptr; }
    size_t getBytesLength(const char* key) const {
        const Entry* entry = find(key);
        return entry ? entry->length : 0;
    }
    size_t getBytes(const char* key, void* out, size_t len) const {
        const Entry* entry = find(key);
        if (!entry || !out || len != entry->length) return 0;
        memcpy(out, entry->bytes, len);
        return len;
    }
    size_t putBytes(const char* key, const void* data, size_t len) {
        if (!opened_ || !key || !data || strlen(key) > 15 || len > sizeof(entries[0].bytes)) return 0;
        Entry* entry = findMutable(key);
        if (!entry) entry = freeEntry();
        if (!entry) return 0;
        entry->used = true;
        strncpy(entry->nameSpace, nameSpace_, sizeof(entry->nameSpace) - 1);
        entry->nameSpace[sizeof(entry->nameSpace) - 1] = '\0';
        strncpy(entry->key, key, sizeof(entry->key) - 1);
        entry->key[sizeof(entry->key) - 1] = '\0';
        ++writeCallCount; size_t written = len;
        if (writeLimit >= 0 && static_cast<size_t>(writeLimit) < written) written = static_cast<size_t>(writeLimit);
        if (failAtWriteCall && writeCallCount == failAtWriteCall && failBytes < written) written = failBytes;
        memcpy(entry->bytes, data, written);
        entry->length = written;
        return written;
    }
    bool remove(const char* key) {
        Entry* entry = findMutable(key);
        if (!entry) return false;
        memset(entry, 0, sizeof(*entry));
        return true;
    }
    static void clearForTest() {
        memset(entries, 0, sizeof(entries));
        writeLimit = -1; failAtWriteCall = failBytes = writeCallCount = 0;
    }
    static void simulatePartialWriteAfter(size_t bytes) { writeLimit = static_cast<int32_t>(bytes); }
    static void simulatePartialWriteOnCall(size_t call, size_t bytes) { writeCallCount = 0; failAtWriteCall = call; failBytes = bytes; }
    static void clearWriteFailure() { writeLimit = -1; failAtWriteCall = failBytes = writeCallCount = 0; }
    static Entry entries[16];
    static int32_t writeLimit;
    static size_t failAtWriteCall, failBytes, writeCallCount;
private:
    const Entry* find(const char* key) const {
        if (!opened_ || !key) return nullptr;
        for (size_t i = 0; i < 16; ++i)
            if (entries[i].used && strcmp(entries[i].nameSpace, nameSpace_) == 0 && strcmp(entries[i].key, key) == 0) return &entries[i];
        return nullptr;
    }
    Entry* findMutable(const char* key) {
        if (!opened_ || !key) return nullptr;
        for (size_t i = 0; i < 16; ++i)
            if (entries[i].used && strcmp(entries[i].nameSpace, nameSpace_) == 0 && strcmp(entries[i].key, key) == 0) return &entries[i];
        return nullptr;
    }
    Entry* freeEntry() {
        for (size_t i = 0; i < 16; ++i) if (!entries[i].used) return &entries[i];
        return nullptr;
    }
    char nameSpace_[16];
    bool opened_;
};
#endif
