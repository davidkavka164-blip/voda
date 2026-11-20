#include "NvsSensorConfigStore.h"
#include <Preferences.h>
#include <assert.h>
#include <stdio.h>

bool Preferences::present = false;
size_t Preferences::length = 0;
unsigned char Preferences::storage[4096] = {0};
char Preferences::lastNamespace[32] = {0};

int main() {
    using namespace HWC;
    Preferences::clearForTest();
    NvsSensorConfigStore store;
    assert(store.begin());
    assert(!store.hasRecord("well"));
    SensorConfig config = defaultSensorConfig();
    assert(store.save("well", config));
    assert(store.hasRecord("well"));

    SensorConfig loaded = defaultSensorConfig();
    assert(store.load("well", loaded));
    assert(sensorConfigChecksumMatches(loaded));
    loaded.calibrationScale = 1.1f;
    loaded.calibrationOffsetCm = -2.5f;
    updateSensorConfigChecksum(loaded);
    assert(store.save("well", loaded));
    SensorConfig verify = defaultSensorConfig();
    assert(store.load("well", verify));
    assert(verify.magic == loaded.magic && verify.version == loaded.version);
    assert(verify.shuntOhms == loaded.shuntOhms);
    assert(verify.currentMinMilliAmps == loaded.currentMinMilliAmps);
    assert(verify.currentMaxMilliAmps == loaded.currentMaxMilliAmps);
    assert(verify.currentToleranceMilliAmps == loaded.currentToleranceMilliAmps);
    assert(verify.sensorRangeCm == loaded.sensorRangeCm);
    assert(verify.calibrationScale == 1.1f && verify.calibrationOffsetCm == -2.5f);
    assert(verify.checksum == loaded.checksum);

    Preferences::storage[0] ^= 0xFFU;
    assert(store.hasRecord("well"));
    assert(!store.load("well", verify));
    assert(!store.save("well", defaultSensorConfig())); // normal save must not overwrite corrupt calibration
    SensorConfig recovered = {};
    assert(buildSensorConfigRecoveryCandidate(1.05f, 250.0f, 260.0f, recovered));
    assert(store.recover("well", recovered)); // explicit recovery may replace corrupt record
    assert(store.load("well", verify));
    assert(verify.calibrationScale == 1.05f);
    assert(verify.calibrationOffsetCm == -2.5f);
    assert(sensorConfigChecksumMatches(verify));
    assert(!store.recover("well", defaultSensorConfig())); // valid record cannot be overwritten by recovery

    assert(!buildSensorConfigRecoveryCandidate(0.1f, 250.0f, 260.0f, recovered));
    assert(!buildSensorConfigRecoveryCandidate(1.0f, 250.0f, 2601.0f, recovered));
    store.end();
    assert(!store.recover("well", defaultSensorConfig())); // unavailable NVS cannot recover

    Preferences::clearForTest();
    NvsSensorConfigStore missingStore;
    assert(missingStore.begin());
    assert(!missingStore.hasRecord("lower"));
    assert(buildSensorConfigRecoveryCandidate(1.0f, 100.0f, 105.0f, recovered));
    assert(missingStore.recover("lower", recovered)); // missing record can be initialized explicitly
    assert(missingStore.load("lower", verify));
    puts("PASS: NvsSensorConfigStore native stub tests including gated recovery");
    return 0;
}
