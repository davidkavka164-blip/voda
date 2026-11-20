#ifndef HWC_CREDENTIAL_TEXT_PREFERENCES_STUB_H
#define HWC_CREDENTIAL_TEXT_PREFERENCES_STUB_H
#include <stddef.h>
#include <map>
#include <string>

class String {
public:
    String() : value_() {}
    String(const char* value) : value_(value ? value : "") {}
    explicit String(const std::string& value) : value_(value) {}
    size_t length() const { return value_.size(); }
    const char* c_str() const { return value_.c_str(); }
private:
    std::string value_;
};

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
    String getString(const char* key, const char* defaultValue = "") const {
        if (!opened_ || key == 0) return String(defaultValue);
        const std::map<std::string, std::string>::const_iterator it = values_.find(compoundKey(key));
        return it == values_.end() ? String(defaultValue) : String(it->second);
    }
    size_t putString(const char* key, const char* value) {
        if (!opened_ || key == 0 || value == 0) return 0U;
        values_[compoundKey(key)] = value;
        return std::string(value).size() + 1U;
    }
    static void clearAllForTest() { values_.clear(); }
private:
    bool opened_;
    std::string namespace_;
    std::string compoundKey(const char* key) const { return namespace_ + "/" + key; }
    static std::map<std::string, std::string> values_;
};

#endif
