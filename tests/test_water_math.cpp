#include "../firmware/common/WaterMath.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static bool near(float a, float b, float epsilon = 0.02f) { return fabsf(a - b) <= epsilon; }

int main() {
    using namespace HWC;
    float liters = 0.0f;
    assert(lowerTankVolumeLiters(11.0f, liters) && near(liters, 677.6f));
    assert(lowerTankVolumeLiters(121.0f, liters) && near(liters, 7453.6f));
    assert(!lowerTankVolumeLiters(121.1f, liters));

    float target = 0.0f;
    assert(lowerTankTargetLevelCm(72.0f, 200.0f, target) && near(target, 75.24675f));
    assert(!lowerTankTargetLevelCm(120.0f, 200.0f, target));
    assert(!lowerTankTargetLevelCm(72.0f, 0.0f, target));

    const CalibrationPoint curve[] = {{100.0f, 1000.0f}, {200.0f, 2200.0f}, {300.0f, 3500.0f}};
    assert(estimateWellVolumeLiters(200.0f, curve, 3, liters) == ESTIMATE_OK_EXACT && near(liters, 2200.0f));
    assert(estimateWellVolumeLiters(150.0f, curve, 3, liters) == ESTIMATE_OK_INTERPOLATED && near(liters, 1600.0f));
    assert(estimateWellVolumeLiters(99.0f, curve, 3, liters) == ESTIMATE_OUT_OF_RANGE);
    const CalibrationPoint badCurve[] = {{100.0f, 1000.0f}, {200.0f, 900.0f}};
    assert(estimateWellVolumeLiters(150.0f, badCurve, 2, liters) == ESTIMATE_INVALID_CURVE);

    DailyInflowTracker inflow;
    InflowUpdate u = inflow.observe(100.0f, true, 20261028, true);
    assert(u.baselineEstablished);
    inflow.observe(135.0f, true, 20261028, true);
    inflow.observe(155.0f, true, 20261028, true);
    inflow.observe(120.0f, true, 20261028, true); // decrease is ignored, baseline moves down
    inflow.observe(135.0f, true, 20261028, true);
    assert(near(inflow.litersForDate(20261028), 70.0f));

    DailyInflowTracker rolling;
    rolling.observe(100.0f, true, 20260901, true);
    rolling.observe(110.0f, true, 20260901, true);
    assert(near(rolling.litersForDate(20260901), 10.0f));
    rolling.observe(110.0f, true, 20261001, true); // prune dates older than the rolling 30-day window
    assert(near(rolling.litersForDate(20260901), 0.0f));
    rolling.observe(115.0f, true, 20261340, true); // malformed date changes to undated baseline
    rolling.observe(120.0f, true, 20261340, true);
    assert(near(rolling.undatedLiters(), 5.0f));

    DailyInflowTracker timeInvalid;
    timeInvalid.observe(500.0f, true, 0, false);
    timeInvalid.observe(530.0f, true, 0, false);
    assert(near(timeInvalid.undatedLiters(), 30.0f));
    timeInvalid.observe(540.0f, true, 20261028, true); // date-validity transition resets baseline; no false date attribution
    assert(near(timeInvalid.litersForDate(20261028), 0.0f));

    DailyInflowSnapshot snapshot = {};
    inflow.exportSnapshot(snapshot);
    DailyInflowTracker restored;
    assert(restored.restoreSnapshot(snapshot));
    assert(near(restored.litersForDate(20261028), 70.0f));
    DailyInflowSnapshot invalidSnapshot = snapshot;
    invalidSnapshot.undatedLiters = -1.0f;
    assert(!restored.restoreSnapshot(invalidSnapshot));
    invalidSnapshot = snapshot;
    invalidSnapshot.baselineValid = 2;
    assert(!restored.restoreSnapshot(invalidSnapshot));
    invalidSnapshot = snapshot;
    invalidSnapshot.days[0].dateKey = 20261340;
    assert(!restored.restoreSnapshot(invalidSnapshot));

    puts("PASS: WaterMath native tests");
    return 0;
}
