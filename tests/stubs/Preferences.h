#ifndef HWC_TEST_PREFERENCES_H
#define HWC_TEST_PREFERENCES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
class Preferences {
public:
    Preferences() : opened_(false) {}
    bool begin(const char* nameSpace, bool) {
        opened_ = true;
        if (nameSpace == nullptr) return false;
        strncpy(lastNamespace, nameSpace, sizeof(lastNamespace) - 1);
        lastNamespace[sizeof(lastNamespace) - 1] = '\0';
        return true;
    }
    void end() { opened_ = false; }
    bool isKey(const char*) const { return opened_ && present; }
    size_t getBytesLength(const char*) const { return (opened_ && present) ? length : 0; }
    size_t getBytes(const char*, void* out, size_t len) const {
        if (!opened_ || !present || len != length) return 0;
        memcpy(out, storage, len); return len;
    }
    bool remove(const char*) { if (!opened_) return false; present = false; length = 0; memset(storage, 0, sizeof(storage)); return true; }
    size_t putBytes(const char*, const void* data, size_t len) {
        if (!opened_ || len > sizeof(storage)) return 0;
        memcpy(storage, data, len); length = len; present = true; return len;
    }
    static void clearForTest() { present = false; length = 0; memset(storage, 0, sizeof(storage)); }
    static bool present;
    static size_t length;
    static unsigned char storage[4096];
    static char lastNamespace[32];
private:
    bool opened_;
};
#endif
