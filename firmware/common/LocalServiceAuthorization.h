#ifndef HWC_LOCAL_SERVICE_AUTHORIZATION_H
#define HWC_LOCAL_SERVICE_AUTHORIZATION_H

#include <stdint.h>

namespace HWC {

// Portable policy for a physical service input. A board-specific adapter must
// be the ONLY caller of samplePhysicalInput(); never feed this from HTTP,
// UDP, Wi-Fi, a serial command, or a software checkbox.
//
// A continuous hold arms one single-use authorization for a short window.
// The input must be released before another authorization can be armed.
class LocalServiceAuthorization {
public:
    static const uint32_t HOLD_MS = 3000U;
    static const uint32_t WINDOW_MS = 60000U;

    LocalServiceAuthorization()
        : tracking_(false), armed_(false), consumedUntilRelease_(false),
          holdStartedAt_(0), armedAt_(0) {}

    void samplePhysicalInput(uint32_t nowMs, bool active) {
        if (!active) {
            tracking_ = false;
            consumedUntilRelease_ = false;
            return;
        }
        if (consumedUntilRelease_ || armed_) return;
        if (!tracking_) {
            tracking_ = true;
            holdStartedAt_ = nowMs;
            return;
        }
        if (elapsed(nowMs, holdStartedAt_) >= HOLD_MS) {
            armed_ = true;
            armedAt_ = nowMs;
            tracking_ = false;
            consumedUntilRelease_ = true;
        }
    }

    bool consume(uint32_t nowMs) { return consume(nowMs, 0); }

    // Optional output exposes the timestamp at which the physical hold armed
    // the authorization. A downstream service session can therefore share
    // the original deadline instead of silently extending the 60s window.
    bool consume(uint32_t nowMs, uint32_t* authorizationStartedAtMs) {
        if (!armed_) return false;
        if (elapsed(nowMs, armedAt_) >= WINDOW_MS) {
            armed_ = false;
            return false;
        }
        if (authorizationStartedAtMs != 0) *authorizationStartedAtMs = armedAt_;
        armed_ = false;
        return true;
    }

    bool isArmed(uint32_t nowMs) {
        if (armed_ && elapsed(nowMs, armedAt_) >= WINDOW_MS) armed_ = false;
        return armed_;
    }

private:
    bool tracking_;
    bool armed_;
    bool consumedUntilRelease_;
    uint32_t holdStartedAt_;
    uint32_t armedAt_;

    static uint32_t elapsed(uint32_t now, uint32_t then) {
        return static_cast<uint32_t>(now - then); // wrap-safe for bounded intervals
    }
};

} // namespace HWC
#endif
