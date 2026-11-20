#include "PeerSecretVault.h"

namespace HWC {

PeerSecretVault::PeerSecretVault(PeerSecretBackend& backend) : backend_(backend) {}

bool PeerSecretVault::validPair(uint8_t localId, uint8_t peerId) {
    return localId != 0U && peerId != 0U && localId != peerId;
}

bool PeerSecretVault::validSecret(const uint8_t* secret) {
    if (secret == 0) return false;
    uint8_t aggregate = 0U;
    for (size_t i = 0; i < PEER_SHARED_SECRET_SIZE; ++i) aggregate |= secret[i];
    return aggregate != 0U;
}

void PeerSecretVault::clear(uint8_t* data, size_t length) {
    if (data == 0) return;
    volatile uint8_t* p = data;
    while (length-- != 0U) *p++ = 0U;
}

bool PeerSecretVault::load(uint8_t localId, uint8_t peerId,
                           uint8_t secret[PEER_SHARED_SECRET_SIZE]) {
    if (secret == 0) return false;
    clear(secret, PEER_SHARED_SECRET_SIZE);
    if (!validPair(localId, peerId) || !backend_.confidentialStorageAvailable()) return false;
    if (!backend_.read(localId, peerId, secret) || !validSecret(secret)) {
        clear(secret, PEER_SHARED_SECRET_SIZE);
        return false;
    }
    return true;
}

bool PeerSecretVault::provision(uint8_t localId, uint8_t peerId,
                                const uint8_t secret[PEER_SHARED_SECRET_SIZE],
                                bool physicalProvisioningConfirmed) {
    if (!validPair(localId, peerId) || !validSecret(secret) ||
        !physicalProvisioningConfirmed || !backend_.confidentialStorageAvailable()) return false;

    if (!backend_.write(localId, peerId, secret)) {
        // A backend may fail after partially changing persistent state. Do not
        // assume a false return means that no bytes were committed.
        (void)backend_.erase(localId, peerId);
        return false;
    }

    // A successful backend write is not proof that the exact secret can be
    // read back. Verify it without exposing the value and clear the temporary
    // buffer on every path. The command path remains fail-closed on failure.
    uint8_t verify[PEER_SHARED_SECRET_SIZE] = {0U};
    const bool readOk = backend_.read(localId, peerId, verify);
    uint8_t difference = 0U;
    if (readOk) {
        for (size_t i = 0; i < PEER_SHARED_SECRET_SIZE; ++i) {
            difference |= static_cast<uint8_t>(verify[i] ^ secret[i]);
        }
    }
    clear(verify, sizeof(verify));
    if (readOk && difference == 0U) return true;

    // A failed replacement must not leave an unverified value available to
    // a later boot. The backend erase contract requires durable invalidation.
    (void)backend_.erase(localId, peerId);
    return false;
}

bool PeerSecretVault::erase(uint8_t localId, uint8_t peerId,
                            bool physicalProvisioningConfirmed) {
    if (!validPair(localId, peerId) || !physicalProvisioningConfirmed ||
        !backend_.confidentialStorageAvailable()) return false;
    return backend_.erase(localId, peerId);
}

} // namespace HWC
