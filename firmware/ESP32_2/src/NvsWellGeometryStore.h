#ifndef HWC_NVS_WELL_GEOMETRY_STORE_H
#define HWC_NVS_WELL_GEOMETRY_STORE_H

#include <Arduino.h>
#include <Preferences.h>
#include "WellGeometry.h"

namespace HWC {
class NvsWellGeometryStore {
public:
    NvsWellGeometryStore();
    bool begin();
    void end();
    bool load(WellGeometryConfig& out);
    bool save(WellGeometryConfig config);
private:
    Preferences preferences_;
    bool opened_;
};
}
#endif
