#include "NvsWellGeometryStore.h"
#include <Preferences.h>
#include <assert.h>
#include <stdio.h>
bool Preferences::present = false;
bool Preferences::ignoreNextWrite = false;
size_t Preferences::length = 0;
unsigned char Preferences::storage[4096] = {0};
int main() {
    using namespace HWC;
    Preferences::clearForTest();
    NvsWellGeometryStore store; assert(store.begin());
    WellGeometryConfig config = defaultWellGeometryConfig();
    config.internalDiameterCm = 92.0f; updateWellGeometryChecksum(config);
    assert(store.save(config));
    WellGeometryConfig loaded = {}; assert(store.load(loaded));
    assert(loaded.internalDiameterCm == 92.0f);
    // A reported full write that leaves the old valid record must fail verification.
    WellGeometryConfig ignoredWrite = config;
    ignoredWrite.internalDiameterCm = 96.0f; updateWellGeometryChecksum(ignoredWrite);
    Preferences::ignoreNextWrite = true;
    assert(!store.save(ignoredWrite));
    WellGeometryConfig afterIgnoredWrite = {};
    assert(store.load(afterIgnoredWrite));
    assert(afterIgnoredWrite.internalDiameterCm == 92.0f);
    Preferences::storage[0] ^= 0xFFU;
    WellGeometryConfig replacement = defaultWellGeometryConfig();
    replacement.internalDiameterCm = 100.0f; updateWellGeometryChecksum(replacement);
    assert(!store.save(replacement)); assert(!store.load(loaded));
    puts("PASS: NvsWellGeometryStore read-back and corrupt-record overwrite protection");
}
