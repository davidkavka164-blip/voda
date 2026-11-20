#include "PeerHandshakeRetryPolicy.h"
#include <assert.h>
#include <stdint.h>

using namespace HWC;

int main() {
    PeerHandshakeRetryPolicy policy;
    assert(!policy.start(0U, 0U, 3U));
    assert(!policy.start(0U, 10U, 0U));
    assert(policy.start(100U, 50U, 3U));
    assert(policy.active());
    assert(policy.attempts() == 1U);
    assert(!policy.due(149U));
    assert(policy.due(150U));
    assert(policy.markSent(150U));
    assert(policy.attempts() == 2U);
    assert(!policy.due(199U));
    assert(policy.due(200U));
    assert(policy.markSent(200U));
    assert(policy.attempts() == 3U);
    assert(!policy.due(250U));
    assert(!policy.exhausted(249U));
    assert(policy.exhausted(250U));
    assert(!policy.markSent(250U));
    policy.cancel();
    assert(!policy.active());
    assert(policy.attempts() == 0U);

    // Unsigned subtraction keeps the schedule correct across millis() wrap.
    assert(policy.start(0xFFFFFFF0UL, 32U, 2U));
    assert(!policy.due(0x0000000FUL));
    assert(policy.due(0x00000010UL));
    assert(policy.markSent(0x00000010UL));
    assert(policy.exhausted(0x00000030UL));
    return 0;
}
