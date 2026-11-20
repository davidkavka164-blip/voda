#include "PeerSequenceAllocator.h"
#include <assert.h>
#include <stdio.h>
using namespace HWC;

int main() {
    PeerSequenceAllocator allocator;
    uint32_t sequence = 99U;
    assert(!allocator.allocate(sequence) && sequence == 0U);
    assert(!allocator.beginBoot(0U));
    assert(allocator.beginBoot(0x12345678UL));
    assert(allocator.allocate(sequence) && sequence == 1U);
    assert(allocator.advancePastAuthenticatedFloor(64U));
    assert(allocator.nextSequence() == 65U);
    assert(allocator.allocate(sequence) && sequence == 65U);
    // A stale/lower floor must never move the sender backwards.
    assert(allocator.advancePastAuthenticatedFloor(20U));
    assert(allocator.nextSequence() == 66U);
    // Reconnect in the same boot preserves the sequence.
    assert(allocator.beginBoot(0x12345678UL));
    assert(allocator.nextSequence() == 66U);
    // A genuinely new local boot starts a new sender epoch.
    assert(allocator.beginBoot(0x12345679UL));
    assert(allocator.nextSequence() == 1U);
    assert(!allocator.advancePastAuthenticatedFloor(PeerSequenceAllocator::MAX_SEQUENCE));
    assert(allocator.allocate(sequence) == false && sequence == 0U);
    assert(allocator.exhausted());
    assert(!allocator.advancePastAuthenticatedFloor(PeerSequenceAllocator::MAX_SEQUENCE + 1U));
    puts("PASS: PeerSequenceAllocator");
    return 0;
}
