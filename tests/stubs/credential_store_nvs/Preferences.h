#ifndef HWC_CREDENTIAL_STORE_NVS_PREFERENCES_STUB_H
#define HWC_CREDENTIAL_STORE_NVS_PREFERENCES_STUB_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

class Preferences {
public:
    Preferences() : opened_(false), namespace_() {}
    bool begin(const char* nameSpace, bool) {
        if (nameSpace == 0 || nameSpace[0] == '\0') return false;
        namespace_ = nameSpace;
        opened_ = true;
        return true;
    }
    void end() { opened_ = false; namespace_.clear(); }
    bool isKey(const char* key) const {
        return opened_ && key != 0 && values_.find(compoundKey(key)) != values_.end();
    }
    size_t getBytesLength(const char* key) const {
        if (!opened_ || key == 0) return 0U;
        const std::map<std::string, std::vector<uint8_t> >::const_iterator it = values_.find(compoundKey(key));
        return it == values_.end() ? 0U : it->second.size();
    }
    size_t getBytes(const char* key, void* output, size_t length) const {
        if (!opened_ || key == 0 || output == 0) return 0U;
        const std::map<std::string, std::vector<uint8_t> >::const_iterator it = values_.find(compoundKey(key));
        if (it == values_.end() || it->second.size() != length) return 0U;
        if (length != 0U) memcpy(output, &it->second[0], length);
        return length;
    }
    size_t putBytes(const char* key, const void* data, size_t length) {
        if (!opened_ || key == 0 || (data == 0 && length != 0U)) return 0U;
        std::vector<uint8_t> value(length);
        if (length != 0U) memcpy(&value[0], data, length);
        values_[compoundKey(key)] = value;
        return length;
    }
    bool remove(const char* key) {
        if (!opened_ || key == 0) return false;
        values_.erase(compoundKey(key));
        return true;
    }
    static void resetForTest() { values_.clear(); }
    static bool eraseForTest(const char* nameSpace, const char* key) {
        return values_.erase(std::string(nameSpace) + "/" + key) != 0U;
    }
    static bool existsForTest(const char* nameSpace, const char* key) {
        return values_.find(std::string(nameSpace) + "/" + key) != values_.end();
    }
private:
    bool opened_;
    std::string namespace_;
    std::string compoundKey(const char* key) const { return namespace_ + "/" + key; }
    static std::map<std::string, std::vector<uint8_t> > values_;
};

#endif
