#include "PeerHandshakeTimeout.h"
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>

int main() {
    HWC::PeerHandshakeTimeout timeout;
    assert(!timeout.active());
    assert(!timeout.start(10U, 0U));
    assert(!timeout.active());
    assert(!timeout.start(10U, static_cast<uint32_t>(INT32_MAX) + 1U));

    assert(timeout.start(100U, 5000U));
    assert(timeout.active());
    assert(!timeout.expired(5099U));
    assert(timeout.expired(5100U));
    assert(!timeout.consume(5100U));
    assert(!timeout.active());

    // millis() wrap-around must not extend or shorten the timeout.
    assert(timeout.start(0xFFFFFF00UL, 512U));
    assert(!timeout.expired(0x000000FFUL));
    assert(timeout.expired(0x00000100UL));
    assert(!timeout.consume(0x00000100UL));

    assert(timeout.start(200U, 1000U));
    assert(timeout.consume(1199U));
    assert(!timeout.active());
    assert(!timeout.consume(1200U));

    assert(timeout.start(300U, 100U));
    timeout.cancel();
    assert(!timeout.active());
    assert(!timeout.consume(350U));

    puts("PASS: PeerHandshakeTimeout wrap-safe deadline and one-shot consume tests");
    return 0;
}
