#ifndef HWC_PEER_SEQUENCE_ALLOCATOR_H
#define HWC_PEER_SEQUENCE_ALLOCATOR_H

#include <stdint.h>

namespace HWC {

// Volatile sender-side sequence allocator. A new local boot ID starts a new
// sender epoch; reconnects during the same boot preserve the next sequence.
// The receiver's authenticated persistent floor must be applied before sending.
class PeerSequenceAllocator {
public:
    static const uint32_t MAX_SEQUENCE = 0x7FFFFFFFUL;

    PeerSequenceAllocator();
    bool beginBoot(uint32_t localBootId);
    bool advancePastAuthenticatedFloor(uint32_t reservedThrough);
    bool allocate(uint32_t& sequence);
    uint32_t nextSequence() const;
    uint32_t localBootId() const;
    bool exhausted() const;

private:
    uint32_t localBootId_;
    uint32_t nextSequence_;
    bool initialized_;
};

} // namespace HWC
#endif
