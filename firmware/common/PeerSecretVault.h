#ifndef HWC_PEER_SECRET_VAULT_H
#define HWC_PEER_SECRET_VAULT_H

#include <stddef.h>
#include <stdint.h>

namespace HWC {

static const size_t PEER_SHARED_SECRET_SIZE = 32U;

// Backend contract: implementations must encrypt/authenticate secret material
// at rest. Plain Preferences/NVS without ESP-IDF NVS encryption MUST return
// false. write() must be durable before returning true; erase() must make the
// record unavailable after returning true. This interface does not pretend
// to provide encryption or transactional guarantees by itself.
class PeerSecretBackend {
public:
    virtual ~PeerSecretBackend() {}
    virtual bool confidentialStorageAvailable() const = 0;
    virtual bool read(uint8_t localId, uint8_t peerId,
                      uint8_t secret[PEER_SHARED_SECRET_SIZE]) = 0;
    virtual bool write(uint8_t localId, uint8_t peerId,
                       const uint8_t secret[PEER_SHARED_SECRET_SIZE]) = 0;
    virtual bool erase(uint8_t localId, uint8_t peerId) = 0;
};

// Fail-closed policy wrapper. It binds a 32-byte provisioned secret to an
// ordered device pair and refuses all access unless backend protection is
// explicitly available. Provisioning/transport of the secret is out of scope.
class PeerSecretVault {
public:
    explicit PeerSecretVault(PeerSecretBackend& backend);
    bool load(uint8_t localId, uint8_t peerId,
              uint8_t secret[PEER_SHARED_SECRET_SIZE]);
    bool provision(uint8_t localId, uint8_t peerId,
                   const uint8_t secret[PEER_SHARED_SECRET_SIZE],
                   bool physicalProvisioningConfirmed);
    bool erase(uint8_t localId, uint8_t peerId,
               bool physicalProvisioningConfirmed);
    static void clear(uint8_t* data, size_t length);
private:
    PeerSecretBackend& backend_;
    static bool validPair(uint8_t localId, uint8_t peerId);
    static bool validSecret(const uint8_t* secret);
};

} // namespace HWC
#endif
