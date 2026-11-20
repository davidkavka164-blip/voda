#ifndef HWC_TRANSFER_JOURNAL_TEST_PREFERENCES_H
#define HWC_TRANSFER_JOURNAL_TEST_PREFERENCES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
class Preferences {
public:
    Preferences() : opened_(false) {}
    bool begin(const char* ns, bool) { if (!ns) return false; opened_ = true; strncpy(lastNamespace, ns, sizeof(lastNamespace)-1); lastNamespace[sizeof(lastNamespace)-1] = '\0'; return true; }
    void end() { opened_ = false; }
    bool isKey(const char* key) const { int i = index(key); return opened_ && i >= 0 && present[i]; }
    size_t getBytesLength(const char* key) const { int i = index(key); return isKey(key) ? lengths[i] : 0U; }
    size_t getBytes(const char* key, void* out, size_t len) const { int i=index(key); if (!isKey(key) || lengths[i] != len) return 0U; memcpy(out, data[i], len); return len; }
    size_t putBytes(const char* key, const void* in, size_t len) { int i=index(key); if (!opened_ || i<0 || failWrite || len>sizeof(data[i])) return 0U; memcpy(data[i],in,len); lengths[i]=len; present[i]=true; return len; }
    static void clearForTest() { memset(data,0,sizeof(data)); memset(lengths,0,sizeof(lengths)); memset(present,0,sizeof(present)); failWrite=false; }
    static unsigned char data[2][256]; static size_t lengths[2]; static bool present[2]; static bool failWrite; static char lastNamespace[32];
private:
    bool opened_;
    static int index(const char* key) { if (!key) return -1; if (!strcmp(key,"slot0")) return 0; if (!strcmp(key,"slot1")) return 1; return -1; }
};
#endif
