#ifndef HWC_UPPER_VOLUME_POWERCUT_PREFERENCES_STUB_H
#define HWC_UPPER_VOLUME_POWERCUT_PREFERENCES_STUB_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Models the ESP-IDF NVS atomic key replacement contract: after interruption,
// either the old complete value or the new complete value is visible, never a
// torn mixture. This is a host model, not a substitute for a hardware power-cut test.
class Preferences {
public:
    enum WriteFault {
        WRITE_OK = 0,
        WRITE_INTERRUPTED_BEFORE_COMMIT = 1,
        WRITE_COMMITTED_BUT_REPORT_FAILURE = 2,
        REMOVE_INTERRUPTED_BEFORE_COMMIT = 3,
        REMOVE_COMMITTED_BUT_REPORT_FAILURE = 4,
        READ_FAILURE = 5
    };

    Preferences() : opened_(false) {}
    bool begin(const char* nameSpace, bool) {
        opened_ = nameSpace != nullptr && nameSpace[0] != '\0';
        return opened_;
    }
    void end() { opened_ = false; }
    bool isKey(const char*) const { return opened_ && present; }
    size_t getBytesLength(const char*) const { return (opened_ && present) ? length : 0U; }
    size_t getBytes(const char*, void* out, size_t maxLen) const {
        if (!opened_ || !present || out == nullptr || maxLen < length || writeFault == READ_FAILURE) return 0U;
        memcpy(out, storage, length);
        return length;
    }
    bool remove(const char*) {
        if (!opened_) return false;
        if (writeFault == REMOVE_INTERRUPTED_BEFORE_COMMIT) return false;
        present = false;
        length = 0U;
        memset(storage, 0, sizeof(storage));
        return writeFault != REMOVE_COMMITTED_BUT_REPORT_FAILURE;
    }
    size_t putBytes(const char*, const void* data, size_t len) {
        if (!opened_ || data == nullptr || len == 0U || len > sizeof(storage)) return 0U;
        if (writeFault == WRITE_INTERRUPTED_BEFORE_COMMIT) return 0U;
        // Commit is atomic in this model: old value remains until this copy.
        memcpy(storage, data, len);
        length = len;
        present = true;
        if (writeFault == WRITE_COMMITTED_BUT_REPORT_FAILURE) return 0U;
        return len;
    }
    static void clearForTest() {
        present = false;
        length = 0U;
        memset(storage, 0, sizeof(storage));
        writeFault = WRITE_OK;
    }
    static bool present;
    static size_t length;
    static uint8_t storage[512];
    static WriteFault writeFault;
private:
    bool opened_;
};
#endif
