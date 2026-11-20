#ifndef HWC_TEST_HISTORY_EXACT_READBACK_PREFERENCES_H
#define HWC_TEST_HISTORY_EXACT_READBACK_PREFERENCES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
class Preferences {
public:
    Preferences() : opened_(false) {}
    bool begin(const char* nameSpace, bool) {
        opened_ = nameSpace != nullptr;
        return opened_;
    }
    void end() { opened_ = false; }
    bool isKey(const char*) const { return opened_ && present; }
    size_t getBytesLength(const char*) const { return isKey("") ? length : 0U; }
    size_t getBytes(const char*, void* out, size_t len) const {
        if (!isKey("") || len != length) return 0U;
        memcpy(out, storage, len);
        return len;
    }
    size_t putBytes(const char*, const void* data, size_t len) {
        if (!opened_ || len > sizeof(storage)) return 0U;
        if (ignoreNextWrite) {
            ignoreNextWrite = false;
            return len; // Simulate a backend that reports success but retains old bytes.
        }
        memcpy(storage, data, len);
        length = len;
        present = true;
        return len;
    }
    static void clearForTest() {
        present = false;
        length = 0U;
        ignoreNextWrite = false;
        memset(storage, 0, sizeof(storage));
    }
    static bool present;
    static bool ignoreNextWrite;
    static size_t length;
    static unsigned char storage[4096];
    static char lastNamespace[32];
private:
    bool opened_;
};
#endif
