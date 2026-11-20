#include "PeerSecretVault.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

class FakeBackend : public HWC::PeerSecretBackend {
public:
    FakeBackend() : protectedStorage(false), present(false), failWrite(false),
        failRead(false), corruptOnWrite(false), storedLocal(0), storedPeer(0) {
        memset(stored, 0, sizeof(stored));
    }
    bool confidentialStorageAvailable() const { return protectedStorage; }
    bool read(uint8_t local, uint8_t peer, uint8_t out[HWC::PEER_SHARED_SECRET_SIZE]) {
        if (failRead || !present || local != storedLocal || peer != storedPeer) return false;
        memcpy(out, stored, sizeof(stored)); return true;
    }
    bool write(uint8_t local, uint8_t peer, const uint8_t in[HWC::PEER_SHARED_SECRET_SIZE]) {
        storedLocal = local; storedPeer = peer;
        if (failWrite) {
            memcpy(stored, in, 8U); // simulate a backend that fails after a partial commit
            present = true;
            return false;
        }
        memcpy(stored, in, sizeof(stored));
        if (corruptOnWrite) stored[7] ^= 0x80U;
        present = true; return true;
    }
    bool erase(uint8_t local, uint8_t peer) {
        if (!present || local != storedLocal || peer != storedPeer) return false;
        memset(stored, 0, sizeof(stored)); present = false; return true;
    }
    bool protectedStorage; bool present; bool failWrite; bool failRead;
    bool corruptOnWrite; uint8_t storedLocal, storedPeer;
    uint8_t stored[HWC::PEER_SHARED_SECRET_SIZE];
};

int main() {
    FakeBackend backend;
    HWC::PeerSecretVault vault(backend);
    uint8_t key[HWC::PEER_SHARED_SECRET_SIZE];
    uint8_t out[HWC::PEER_SHARED_SECRET_SIZE];
    for (size_t i = 0; i < sizeof(key); ++i) key[i] = static_cast<uint8_t>(i + 1U);
    memset(out, 0xA5, sizeof(out));
    assert(!vault.provision(1, 2, key, true)); // plain NVS is not accepted
    assert(!vault.load(1, 2, out));
    for (size_t i = 0; i < sizeof(out); ++i) assert(out[i] == 0U);
    backend.protectedStorage = true;
    assert(!vault.provision(1, 2, key, false));
    assert(!vault.provision(1, 1, key, true));
    backend.failWrite = true;
    assert(!vault.provision(1, 2, key, true));
    assert(!backend.present); // partial write failure is invalidated
    backend.failWrite = false;
    backend.corruptOnWrite = true;
    assert(!vault.provision(1, 2, key, true)); // write reported success, read-back differs
    assert(!backend.present); // failed replacement is invalidated
    backend.corruptOnWrite = false;
    backend.failRead = true;
    assert(!vault.provision(1, 2, key, true)); // cannot verify stored value
    assert(!backend.present); // unverified value is not left for next boot
    backend.failRead = false;
    assert(vault.provision(1, 2, key, true));
    assert(vault.load(1, 2, out));
    assert(memcmp(key, out, sizeof(key)) == 0);
    memset(out, 0xA5, sizeof(out));
    assert(!vault.load(2, 1, out)); // pair order is bound
    for (size_t i = 0; i < sizeof(out); ++i) assert(out[i] == 0U);
    assert(!vault.erase(1, 2, false));
    assert(vault.erase(1, 2, true));
    assert(!vault.load(1, 2, out));
    HWC::PeerSecretVault::clear(key, sizeof(key));
    for (size_t i = 0; i < sizeof(key); ++i) assert(key[i] == 0U);
    puts("PASS: PeerSecretVault fail-closed storage policy tests");
    return 0;
}
