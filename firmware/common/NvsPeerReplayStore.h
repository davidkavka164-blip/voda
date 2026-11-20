#ifndef HWC_NVS_PEER_REPLAY_STORE_H
#define HWC_NVS_PEER_REPLAY_STORE_H

#include <Preferences.h>
#include "PersistentPeerReplayGuard.h"

namespace HWC {

// Two-slot journal over ESP32 NVS. Each slot is one NVS blob. The old slot is
// never overwritten while writing the new checkpoint. This reduces the risk of
// losing the last valid checkpoint, but its power-loss behavior must still be
// validated against the exact ESP-IDF/Arduino-ESP32 version and target flash.
class NvsPeerReplayStore : public IPeerReplayStore {
public:
    explicit NvsPeerReplayStore(const char* nameSpace = "hwc_peer_rp");
    bool begin();
    void end();
    bool isOpen() const;

    PeerReplayStoreLoadResult load(PeerReplayCheckpoint& checkpoint);
    bool saveAtomically(const PeerReplayCheckpoint& checkpoint);

private:
    Preferences preferences_;
    const char* nameSpace_;
    bool opened_;

    PeerReplayStoreLoadResult readSlot(uint8_t slot, PeerReplayCheckpoint& checkpoint);
    static bool newerGeneration(uint32_t a, uint32_t b);
};

} // namespace HWC
#endif
