#include "NvsSensorConfigStore.h"
#include <Preferences.h>
#include <assert.h>
#include <stdio.h>

bool Preferences::present = false;
size_t Preferences::length = 0U;
uint8_t Preferences::storage[256] = {0};
Preferences::WriteFault Preferences::writeFault = Preferences::WRITE_OK;

int main() {
    using namespace HWC;
    Preferences::clearForTest();

    SensorConfig candidate = {};
    assert(buildSensorConfigRecoveryCandidate(1.05f, 250.0f, 260.0f, candidate));

    // A simulated interruption before the replacement write leaves the old
    // corrupt record in place. A fresh store instance must still reject it.
    {
        NvsSensorConfigStore store;
        assert(store.begin());
        SensorConfig original = defaultSensorConfig();
        assert(store.save("well", original));
        Preferences::corruptForTest();
        Preferences::writeFault = Preferences::WRITE_FAIL_BEFORE_CHANGE;
        assert(!store.recover("well", candidate));
        store.end();
    }
    {
        NvsSensorConfigStore afterRestart;
        SensorConfig loaded = {};
        assert(afterRestart.begin());
        assert(!afterRestart.load("well", loaded));
        afterRestart.end();
    }

    // A torn/short blob is rejected by both the write result and the next boot's
    // exact-size + checksum validation. It never becomes a trusted config.
    Preferences::writeFault = Preferences::WRITE_OK;
    {
        NvsSensorConfigStore store;
        assert(store.begin());
        assert(store.recover("well", defaultSensorConfig()));
        Preferences::corruptForTest();
        Preferences::writeFault = Preferences::WRITE_PARTIAL_BLOB;
        assert(!store.recover("well", candidate));
        store.end();
    }
    {
        NvsSensorConfigStore afterRestart;
        SensorConfig loaded = {};
        assert(afterRestart.begin());
        assert(!afterRestart.load("well", loaded));
        afterRestart.end();
    }

    // If the complete new blob reached NVS but the API reports a failure before
    // the caller can read it back, the running handler remains blocked. After a
    // reboot, only a complete valid record can be loaded and trusted.
    Preferences::writeFault = Preferences::WRITE_OK;
    {
        NvsSensorConfigStore store;
        assert(store.begin());
        assert(store.recover("well", defaultSensorConfig()));
        Preferences::corruptForTest();
        Preferences::writeFault = Preferences::WRITE_COMPLETE_BUT_REPORT_FAILURE;
        assert(!store.recover("well", candidate));
        store.end();
    }
    {
        NvsSensorConfigStore afterRestart;
        SensorConfig loaded = {};
        assert(afterRestart.begin());
        assert(afterRestart.load("well", loaded));
        assert(sensorConfigChecksumMatches(loaded));
        assert(loaded.calibrationScale == candidate.calibrationScale);
        assert(loaded.calibrationOffsetCm == candidate.calibrationOffsetCm);
        afterRestart.end();
    }

    puts("PASS: sensor recovery power-cut/failure-state tests");
    return 0;
}
