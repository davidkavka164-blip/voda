#ifndef HWC_WATER_MATH_H
#define HWC_WATER_MATH_H

#include <stddef.h>
#include <stdint.h>

namespace HWC {

static const float LOWER_TANK_LITERS_PER_CM = 61.6f;
static const float LOWER_TANK_MAX_LEVEL_CM = 121.0f;
// Informational tank-use reference only; never a pump/transfer control threshold.
static const float LOWER_TANK_MIN_WORKING_LEVEL_CM = 11.0f;
static const size_t MAX_WELL_CALIBRATION_POINTS = 32;
static const size_t INFLOW_HISTORY_DAYS = 30;

struct CalibrationPoint {
    float levelCm;
    float volumeLiters;
};

enum EstimateStatus {
    ESTIMATE_OK_EXACT,
    ESTIMATE_OK_INTERPOLATED,
    ESTIMATE_INVALID_LEVEL,
    ESTIMATE_INVALID_CURVE,
    ESTIMATE_OUT_OF_RANGE
};

struct InflowDay {
    uint32_t dateKey; // YYYYMMDD; 0 means unused
    float liters;
};

struct InflowUpdate {
    bool baselineEstablished;
    bool positiveChange;
    float positiveLiters;
    bool storedInDatedHistory;
    bool storedAsUndated;
};

struct DailyInflowSnapshot {
    uint8_t baselineValid;
    uint8_t previousDateValid;
    float previousVolumeLiters;
    InflowDay days[INFLOW_HISTORY_DAYS];
    float undatedLiters;
};

bool lowerTankVolumeLiters(float levelCm, float &litersOut);
bool lowerTankTargetLevelCm(float currentLevelCm, float requestedLiters, float &targetLevelCmOut);
EstimateStatus estimateWellVolumeLiters(float levelCm, const CalibrationPoint *points, size_t count, float &litersOut);
bool validateCalibrationCurve(const CalibrationPoint *points, size_t count);

class DailyInflowTracker {
public:
    DailyInflowTracker();
    void reset();
    InflowUpdate observe(float volumeLiters, bool volumeValid, uint32_t dateKey, bool dateValid);
    float litersForDate(uint32_t dateKey) const;
    float undatedLiters() const;
    const InflowDay *history() const;
    size_t historySize() const;
    void exportSnapshot(DailyInflowSnapshot& out) const;
    bool restoreSnapshot(const DailyInflowSnapshot& snapshot);

private:
    bool baselineValid_;
    bool previousDateValid_;
    float previousVolumeLiters_;
    InflowDay days_[INFLOW_HISTORY_DAYS];
    float undatedLiters_;
    void addToDate(uint32_t dateKey, float liters);
    void pruneHistory(uint32_t dateKey);
};

} // namespace HWC

#endif
