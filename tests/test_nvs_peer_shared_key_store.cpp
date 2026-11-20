#include "NvsPeerSharedKeyStore.h"
#include <assert.h>
#include <string.h>

bool Preferences::present = false;
size_t Preferences::length = 0;
unsigned char Preferences::storage[4096] = {0};
char Preferences::lastNamespace[32] = {0};

using namespace HWC;

int main() {
    Preferences::clearForTest();
    NvsPeerSharedKeyStore store;
    assert(store.begin());
    assert(strcmp(Preferences::lastNamespace, "hwc_peer") == 0);

    uint8_t loaded[PEER_STATUS_HMAC_KEY_SIZE];
    memset(loaded, 0xA5, sizeof(loaded));
    assert(!store.load(loaded));
    for (size_t i = 0; i < sizeof(loaded); ++i) assert(loaded[i] == 0U);

    uint8_t key[PEER_STATUS_HMAC_KEY_SIZE];
    for (size_t i = 0; i < sizeof(key); ++i) key[i] = static_cast<uint8_t>(0x31U + i);
    assert(store.save(key));
    assert(store.load(loaded));
    assert(memcmp(loaded, key, sizeof(key)) == 0);

    Preferences::length = PEER_STATUS_HMAC_KEY_SIZE - 1U;
    assert(!store.load(loaded));
    for (size_t i = 0; i < sizeof(loaded); ++i) assert(loaded[i] == 0U);

    Preferences::length = PEER_STATUS_HMAC_KEY_SIZE;
    assert(store.load(loaded));
    uint8_t zero[PEER_STATUS_HMAC_KEY_SIZE] = {0U};
    assert(!store.save(zero));
    assert(store.erase());
    assert(!store.load(loaded));
    store.end();
    assert(!store.load(loaded));
    return 0;
}
