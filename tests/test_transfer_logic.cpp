#include "../firmware/common/TransferLogic.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

using namespace HWC;

static TransferInputs baseInputs() {
    TransferInputs in = {};
    in.nowMs = 1000;
    in.lowerLevelValid = true;
    in.lowerLevelCm = 72.0f;
    in.sourceLevelValid = true;
    in.sourceLevelCm = 500.0f;
    in.sourceMinimumTransferLevelCm = 50.0f;
    in.sourceMaximumLevelCm = 800.0f;
    in.criticalProtectionActive = false;
    in.systemReady = true;
    in.inflowWatchdogConfigValid = true;
    in.inflowStartupGraceMs = 10000U;
    in.inflowNoProgressTimeoutMs = 5000U;
    in.inflowMinimumProgressLiters = 12.32f;
    return in;
}

static TransferRequest request(TransferSource source = TRANSFER_SOURCE_WELL) {
    TransferRequest r;
    r.requestId = 42;
    r.source = source;
    r.requestedLiters = 200.0f;
    r.timeoutMs = 60000;
    return r;
}

int main() {
    TransferLogic logic;
    TransferInputs in = baseInputs();
    in.startRequested = true;
    in.request = request();
    TransferOutputs out = logic.update(in);
    assert(out.state == TRANSFER_PRECHECK);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    in.startRequested = false;
    in.nowMs += 10;
    out = logic.update(in);
    assert(out.state == TRANSFER_RUNNING);
    assert(out.valveCommand == VALVE_OPEN_WELL_TO_LOWER);
    assert(fabsf(out.targetLowerLevelCm - 75.24675f) < 0.02f);

    in.lowerLevelCm = 75.25f;
    in.nowMs += 1000;
    out = logic.update(in);
    assert(out.state == TRANSFER_COMPLETED);
    assert(out.endReason == TRANSFER_REASON_TARGET_REACHED);
    assert(out.valveCommand == VALVES_CLOSE_ALL);
    assert(out.estimatedTransferredLiters > 199.0f);
    logic.acknowledgeResult();
    assert(logic.state() == TRANSFER_IDLE);
    in.startRequested = true;
    in.request = request(); // Same requestId must not start the same transfer twice.
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_DUPLICATE_REQUEST);
    assert(out.valveCommand == VALVES_CLOSE_ALL);
    logic.acknowledgeResult();

    // Upper source is independent of peer telemetry and remote upper-level limits.
    // ESP32 #2 owns the upper-pump minimum-level protection locally.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request(TRANSFER_SOURCE_UPPER_TANK);
    in.sourceLevelValid = false;
    in.sourceLevelCm = NAN;
    in.sourceMinimumTransferLevelCm = NAN;
    in.sourceMaximumLevelCm = NAN;
    out = logic.update(in);
    assert(out.state == TRANSFER_PRECHECK);
    assert(out.valveCommand == VALVES_CLOSE_ALL);
    in.startRequested = false;
    in.nowMs += 1;
    out = logic.update(in);
    assert(out.state == TRANSFER_RUNNING);
    assert(out.valveCommand == VALVE_OPEN_UPPER_TO_LOWER);
    // Missing peer telemetry and remote thresholds do not affect the active transfer.
    in.sourceLevelValid = false;
    in.sourceLevelCm = NAN;
    in.nowMs += 1000U;
    in.lowerLevelCm += 0.3f;
    out = logic.update(in);
    assert(out.state == TRANSFER_RUNNING);
    assert(out.valveCommand == VALVE_OPEN_UPPER_TO_LOWER);

    // Low source level aborts; it does not command a pump relay, only closes transfer valves.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    out = logic.update(in);
    assert(out.state == TRANSFER_PRECHECK);
    in.startRequested = false;
    in.sourceLevelCm = 40.0f;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.valveCommand == VALVES_CLOSE_ALL);
    assert(out.endReason == TRANSFER_REASON_SOURCE_MINIMUM);

    // Explicit STOP during a running transfer closes all transfer valves.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    logic.update(in);
    in.startRequested = false;
    logic.update(in);
    in.stopRequested = true;
    out = logic.update(in);
    assert(out.state == TRANSFER_STOPPED);
    assert(out.valveCommand == VALVES_CLOSE_ALL);
    assert(out.endReason == TRANSFER_REASON_USER_STOP);

    // Critical protection during a transfer closes the route immediately.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    logic.update(in);
    in.startRequested = false;
    logic.update(in);
    in.criticalProtectionActive = true;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.valveCommand == VALVES_CLOSE_ALL);
    assert(out.endReason == TRANSFER_REASON_CRITICAL_PROTECTION);

    // An out-of-physical-range lower-tank reading must not be mistaken for reaching the target.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    logic.update(in);
    in.startRequested = false;
    logic.update(in);
    in.lowerLevelCm = 122.0f;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_SENSOR_INVALID);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    // Timeout is a backup guard and uses wrap-safe unsigned millisecond arithmetic.
    logic = TransferLogic();
    in = baseInputs();
    in.nowMs = 0xFFFFFFF0u;
    in.startRequested = true;
    in.request = request();
    in.request.timeoutMs = 100;
    logic.update(in);
    in.startRequested = false;
    logic.update(in);
    in.nowMs = 0x00000060u;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_TIMEOUT);
    assert(out.valveCommand == VALVES_CLOSE_ALL);


    // A source reading above its configured physical depth is invalid, even if finite.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    in.sourceLevelCm = 801.0f;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_SENSOR_INVALID);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    // The source maximum must itself be configured and greater than the transfer minimum.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    in.sourceMaximumLevelCm = in.sourceMinimumTransferLevelCm;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_SYSTEM_NOT_READY);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    // An over-range source reading during transfer aborts and closes the route.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    logic.update(in);
    in.startRequested = false;
    logic.update(in);
    in.sourceLevelCm = 801.0f;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_SENSOR_INVALID);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    // After reboot, restore the newest IDs from persistent history.
    // The same request must remain rejected even though RAM was reconstructed.
    logic = TransferLogic();
    const uint32_t restoredIds[] = {101U, 102U, 103U, 104U, 105U, 106U, 107U, 108U, 109U, 110U, 111U, 112U};
    assert(logic.restoreRecentRequestIds(restoredIds, 12));
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    in.request.requestId = 112U;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_DUPLICATE_REQUEST);
    assert(out.valveCommand == VALVES_CLOSE_ALL);
    logic.acknowledgeResult();

    // Only the newest fixed-size window is retained when a longer history is supplied.
    logic = TransferLogic();
    const uint32_t longHistoryIds[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U, 13U, 14U};
    assert(logic.restoreRecentRequestIds(longHistoryIds, 14));
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    in.request.requestId = 2U; // Outside the retained 12-ID window.
    out = logic.update(in);
    assert(out.state == TRANSFER_PRECHECK);
    logic = TransferLogic();
    assert(!logic.restoreRecentRequestIds(0, 1));

    // Late NVS recovery must merge persisted IDs without forgetting requests
    // already observed in RAM while storage was unavailable.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    in.request.requestId = 2000U;
    out = logic.update(in);
    assert(out.state == TRANSFER_PRECHECK);
    assert(logic.restoreRecentRequestIds(restoredIds, 12));
    in.startRequested = false;
    in.stopRequested = true;
    out = logic.update(in);
    assert(out.state == TRANSFER_STOPPED);
    logic.acknowledgeResult();
    in.stopRequested = false;
    in.startRequested = true;
    in.request.requestId = 2000U;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_DUPLICATE_REQUEST);

    // An external persistence failure can force an immediate fail-safe abort.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request();
    logic.update(in);
    in.startRequested = false;
    logic.update(in);
    out = logic.forceAbort(TRANSFER_REASON_SYSTEM_NOT_READY);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_SYSTEM_NOT_READY);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    // Upper transfer aborts if no measurable receiving-tank increase appears within startup grace.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request(TRANSFER_SOURCE_UPPER_TANK);
    out = logic.update(in);
    assert(out.state == TRANSFER_PRECHECK);
    in.startRequested = false;
    in.nowMs += 1;
    out = logic.update(in);
    assert(out.state == TRANSFER_RUNNING);
    in.nowMs += 10000U;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_NO_INFLOW);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    // A measurable increase confirms inflow; a later interruption trips the second timer.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request(TRANSFER_SOURCE_UPPER_TANK);
    out = logic.update(in);
    in.startRequested = false;
    in.nowMs += 1;
    out = logic.update(in);
    assert(out.state == TRANSFER_RUNNING);
    in.lowerLevelCm += 0.3f; // 18.48 L, above the provisional 12.32 L deadband.
    in.nowMs += 1000U;
    out = logic.update(in);
    assert(out.state == TRANSFER_RUNNING);
    in.nowMs += 5000U;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_NO_INFLOW);
    assert(out.valveCommand == VALVES_CLOSE_ALL);

    // Upper transfer cannot start when its watchdog settings have not been saved and verified.
    logic = TransferLogic();
    in = baseInputs();
    in.startRequested = true;
    in.request = request(TRANSFER_SOURCE_UPPER_TANK);
    in.inflowWatchdogConfigValid = false;
    out = logic.update(in);
    assert(out.state == TRANSFER_ABORTED);
    assert(out.endReason == TRANSFER_REASON_SYSTEM_NOT_READY);


    puts("PASS: TransferLogic native tests");
    return 0;


}
