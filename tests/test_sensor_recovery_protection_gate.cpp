#include "SensorConfig.h"
#include "WellPumpProtection.h"
#include "LowerTankPumpProtection.h"
#include "UpperTankProtection.h"
#include <assert.h>
#include <stdio.h>
using namespace HWC;

static UpperTankLimits upperConfig() {
    UpperTankLimits c = emptyUpperTankLimits();
    c.usableDepthCm = 120.0f;
    c.pumpCriticalTripCm = 18.0f;
    c.pumpRecoveryCm = 28.0f;
    c.householdReserveCm = 35.0f;
    c.transferValveOpenCm = 60.0f;
    updateUpperTankLimitsChecksum(c);
    return c;
}

int main() {
    const SensorConfigLoadState blockedStates[] = {
        SENSOR_CONFIG_MISSING_PROVISIONAL,
        SENSOR_CONFIG_INVALID_PRESENT,
        SENSOR_CONFIG_NVS_UNAVAILABLE
    };

    for (unsigned i = 0; i < sizeof(blockedStates) / sizeof(blockedStates[0]); ++i) {
        assert(!sensorConfigUsableForLevelMeasurement(blockedStates[i]));

        WellPumpProtection well;
        PumpProtectionInputs wi = {};
        wi.sensorValid = sensorConfigUsableForLevelMeasurement(blockedStates[i]);
        wi.waterLevelCm = 100.0f; wi.criticalTripLevelCm = 20.0f;
        wi.recoveryLevelCm = 40.0f; wi.maximumLevelCm = 800.0f;
        wi.configurationValid = true;
        assert(!well.update(wi).pumpPermit);

        LowerTankPumpConfig lc = emptyLowerTankPumpConfig();
        lc.minimumLevelCm = 15.0f; lc.hysteresisCm = 5.0f;
        updateLowerTankPumpConfigChecksum(lc);
        LowerTankPumpProtection lower;
        assert(!lower.update(lc, 100.0f,
            sensorConfigUsableForLevelMeasurement(blockedStates[i]), true).pumpPermit);

        UpperTankProtection upper;
        assert(!upper.update(upperConfig(), 100.0f,
            sensorConfigUsableForLevelMeasurement(blockedStates[i]), true).pumpPermit);
    }

    // A successful service recovery validates the stored electrical conversion,
    // but startup protection still requires a fresh reading at/above recovery level.
    assert(sensorConfigUsableForLevelMeasurement(SENSOR_CONFIG_VALID_STORED));

    WellPumpProtection well;
    PumpProtectionInputs wi = {};
    wi.sensorValid = true; wi.criticalTripLevelCm = 20.0f;
    wi.recoveryLevelCm = 40.0f; wi.maximumLevelCm = 800.0f;
    wi.configurationValid = true; wi.waterLevelCm = 39.9f;
    assert(!well.update(wi).pumpPermit);
    wi.waterLevelCm = 40.0f;
    assert(well.update(wi).pumpPermit);

    LowerTankPumpConfig lc = emptyLowerTankPumpConfig();
    lc.minimumLevelCm = 15.0f; lc.hysteresisCm = 5.0f;
    updateLowerTankPumpConfigChecksum(lc);
    LowerTankPumpProtection lower;
    assert(!lower.update(lc, 19.9f, true, true).pumpPermit);
    assert(lower.update(lc, 20.0f, true, true).pumpPermit);

    UpperTankProtection upper;
    assert(!upper.update(upperConfig(), 27.9f, true, true).pumpPermit);
    assert(upper.update(upperConfig(), 28.0f, true, true).pumpPermit);

    puts("PASS: recovered sensor config does not trust cached readings; all three pump protections wait for recovery-level validation");
    return 0;
}
