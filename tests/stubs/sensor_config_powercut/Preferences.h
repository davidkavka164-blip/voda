#ifndef HWC_SENSOR_CONFIG_POWERCUT_PREFERENCES_STUB_H
#define HWC_SENSOR_CONFIG_POWERCUT_PREFERENCES_STUB_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

class Preferences {
public:
    enum WriteFault {
        WRITE_OK = 0,
        WRITE_FAIL_BEFORE_CHANGE = 1,
        WRITE_PARTIAL_BLOB = 2,
        WRITE_COMPLETE_BUT_REPORT_FAILURE = 3
    };

    Preferences() : opened_(false) {}
    bool begin(const char* nameSpace, bool) {
        opened_ = nameSpace != nullptr && nameSpace[0] != '\0';
        return opened_;
    }
    void end() { opened_ = false; }
    bool isKey(const char*) const { return opened_ && present; }
    size_t getBytesLength(const char*) const { return (opened_ && present) ? length : 0U; }
    size_t getBytes(const char*, void* out, size_t len) const {
        if (!opened_ || !present || out == nullptr || len != length) return 0U;
        memcpy(out, storage, len);
        return len;
    }
    size_t putBytes(const char*, const void* data, size_t len) {
        if (!opened_ || data == nullptr || len == 0U || len > sizeof(storage)) return 0U;
        if (writeFault == WRITE_FAIL_BEFORE_CHANGE) return 0U;
        if (writeFault == WRITE_PARTIAL_BLOB) {
            const size_t partial = len / 2U;
            memcpy(storage, data, partial);
            length = partial;
            present = true;
            return partial;
        }
        memcpy(storage, data, len);
        length = len;
        present = true;
        if (writeFault == WRITE_COMPLETE_BUT_REPORT_FAILURE) return 0U;
        return len;
    }
    static void clearForTest() {
        present = false;
        length = 0U;
        memset(storage, 0, sizeof(storage));
        writeFault = WRITE_OK;
    }
    static void corruptForTest() {
        if (present && length > 0U) storage[0] ^= 0x5AU;
    }

    static bool present;
    static size_t length;
    static uint8_t storage[256];
    static WriteFault writeFault;
private:
    bool opened_;
};

#endif
