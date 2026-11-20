#include "NvsPeerSharedKeyStore.h"
#include <string.h>

namespace HWC {

NvsPeerSharedKeyStore::NvsPeerSharedKeyStore(const char* nameSpace)
    : nameSpace_(nameSpace), opened_(false) {}

bool NvsPeerSharedKeyStore::validKey(const uint8_t* key) {
    if (key == 0) return false;
    uint8_t aggregate = 0U;
    for (size_t i = 0; i < PEER_STATUS_HMAC_KEY_SIZE; ++i) aggregate |= key[i];
    return aggregate != 0U;
}

void NvsPeerSharedKeyStore::clear(uint8_t* key) {
    if (key == 0) return;
    volatile uint8_t* p = key;
    for (size_t i = 0; i < PEER_STATUS_HMAC_KEY_SIZE; ++i) p[i] = 0U;
}

bool NvsPeerSharedKeyStore::begin() {
    if (opened_) return true;
    opened_ = nameSpace_ != 0 && nameSpace_[0] != '\0' &&
              preferences_.begin(nameSpace_, false);
    return opened_;
}

void NvsPeerSharedKeyStore::end() {
    if (opened_) preferences_.end();
    opened_ = false;
}

bool NvsPeerSharedKeyStore::load(uint8_t key[PEER_STATUS_HMAC_KEY_SIZE]) {
    if (key == 0) return false;
    clear(key);
    if (!opened_ || !preferences_.isKey("shared") ||
        preferences_.getBytesLength("shared") != PEER_STATUS_HMAC_KEY_SIZE ||
        preferences_.getBytes("shared", key, PEER_STATUS_HMAC_KEY_SIZE) !=
            PEER_STATUS_HMAC_KEY_SIZE || !validKey(key)) {
        clear(key);
        return false;
    }
    return true;
}

bool NvsPeerSharedKeyStore::save(const uint8_t key[PEER_STATUS_HMAC_KEY_SIZE]) {
    if (!opened_ || !validKey(key)) return false;
    if (preferences_.putBytes("shared", key, PEER_STATUS_HMAC_KEY_SIZE) !=
        PEER_STATUS_HMAC_KEY_SIZE) return false;

    uint8_t verify[PEER_STATUS_HMAC_KEY_SIZE] = {0U};
    const bool valid = load(verify) && memcmp(verify, key, PEER_STATUS_HMAC_KEY_SIZE) == 0;
    clear(verify);
    if (valid) return true;
    (void)preferences_.remove("shared");
    return false;
}

bool NvsPeerSharedKeyStore::erase() {
    if (!opened_) return false;
    if (!preferences_.isKey("shared")) return true;
    return preferences_.remove("shared");
}

} // namespace HWC
