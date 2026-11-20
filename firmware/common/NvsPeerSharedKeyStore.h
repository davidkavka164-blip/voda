#ifndef HWC_NVS_PEER_SHARED_KEY_STORE_H
#define HWC_NVS_PEER_SHARED_KEY_STORE_H

#include "PeerStatusHmacCodec.h"
#include <Preferences.h>

namespace HWC {

// Minimal plain-NVS storage for the one shared peer HMAC key.
// This is NOT encrypted at rest; physical flash extraction remains possible.
class NvsPeerSharedKeyStore {
public:
    explicit NvsPeerSharedKeyStore(const char* nameSpace = "hwc_peer");
    bool begin();
    void end();
    bool load(uint8_t key[PEER_STATUS_HMAC_KEY_SIZE]);
    bool save(const uint8_t key[PEER_STATUS_HMAC_KEY_SIZE]);
    bool erase();
private:
    Preferences preferences_;
    const char* nameSpace_;
    bool opened_;
    static bool validKey(const uint8_t* key);
    static void clear(uint8_t* key);
};

} // namespace HWC
#endif
