#include "PeerSequenceAllocator.h"

namespace HWC {

PeerSequenceAllocator::PeerSequenceAllocator()
    : localBootId_(0U), nextSequence_(1U), initialized_(false) {}

bool PeerSequenceAllocator::beginBoot(uint32_t localBootId) {
    if (localBootId == 0U) return false;
    if (!initialized_ || localBootId_ != localBootId) {
        localBootId_ = localBootId;
        nextSequence_ = 1U;
        initialized_ = true;
    }
    return true;
}

bool PeerSequenceAllocator::advancePastAuthenticatedFloor(uint32_t reservedThrough) {
    if (!initialized_ || reservedThrough > MAX_SEQUENCE) return false;
    const uint32_t candidate = reservedThrough + 1U;
    if (candidate > MAX_SEQUENCE) {
        nextSequence_ = MAX_SEQUENCE + 1U;
        return false;
    }
    if (nextSequence_ <= reservedThrough) nextSequence_ = candidate;
    return true;
}

bool PeerSequenceAllocator::allocate(uint32_t& sequence) {
    sequence = 0U;
    if (!initialized_ || nextSequence_ == 0U || nextSequence_ > MAX_SEQUENCE)
        return false;
    sequence = nextSequence_;
    ++nextSequence_;
    return true;
}

uint32_t PeerSequenceAllocator::nextSequence() const { return nextSequence_; }
uint32_t PeerSequenceAllocator::localBootId() const { return localBootId_; }
bool PeerSequenceAllocator::exhausted() const { return !initialized_ || nextSequence_ > MAX_SEQUENCE; }

} // namespace HWC
