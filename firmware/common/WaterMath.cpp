#include "WaterMath.h"
#include <math.h>

namespace HWC {

static bool finiteFloat(float value) {
    return isfinite(value) != 0;
}

static bool dateKeyToOrdinal(uint32_t dateKey, int64_t& ordinalOut) {
    const int year = static_cast<int>(dateKey / 10000U);
    const unsigned month = static_cast<unsigned>((dateKey / 100U) % 100U);
    const unsigned day = static_cast<unsigned>(dateKey % 100U);
    if (year < 1970 || year > 9999 || month < 1U || month > 12U || day < 1U) return false;
    static const unsigned monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned maxDay = monthDays[month - 1U];
    const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (month == 2U && leap) maxDay = 29U;
    if (day > maxDay) return false;

    int y = year;
    y -= month <= 2U;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned adjustedMonth = month > 2U ? month - 3U : month + 9U;
    const unsigned doy = (153U * adjustedMonth + 2U) / 5U + day - 1U;
    const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    ordinalOut = static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe);
    return true;
}

bool lowerTankVolumeLiters(float levelCm, float &litersOut) {
    if (!finiteFloat(levelCm) || levelCm < 0.0f || levelCm > LOWER_TANK_MAX_LEVEL_CM) return false;
    litersOut = levelCm * LOWER_TANK_LITERS_PER_CM;
    return true;
}

bool lowerTankTargetLevelCm(float currentLevelCm, float requestedLiters, float &targetLevelCmOut) {
    if (!finiteFloat(currentLevelCm) || !finiteFloat(requestedLiters)) return false;
    if (currentLevelCm < 0.0f || currentLevelCm > LOWER_TANK_MAX_LEVEL_CM || requestedLiters <= 0.0f) return false;
    targetLevelCmOut = currentLevelCm + requestedLiters / LOWER_TANK_LITERS_PER_CM;
    return targetLevelCmOut <= LOWER_TANK_MAX_LEVEL_CM;
}

bool validateCalibrationCurve(const CalibrationPoint *points, size_t count) {
    if (points == 0 || count < 2 || count > MAX_WELL_CALIBRATION_POINTS) return false;
    for (size_t i = 0; i < count; ++i) {
        if (!finiteFloat(points[i].levelCm) || !finiteFloat(points[i].volumeLiters)) return false;
        if (points[i].levelCm < 0.0f || points[i].volumeLiters < 0.0f) return false;
        if (i > 0) {
            if (points[i].levelCm <= points[i - 1].levelCm) return false;
            if (points[i].volumeLiters <= points[i - 1].volumeLiters) return false;
        }
    }
    return true;
}

EstimateStatus estimateWellVolumeLiters(float levelCm, const CalibrationPoint *points, size_t count, float &litersOut) {
    if (!finiteFloat(levelCm) || levelCm < 0.0f) return ESTIMATE_INVALID_LEVEL;
    if (!validateCalibrationCurve(points, count)) return ESTIMATE_INVALID_CURVE;
    if (levelCm < points[0].levelCm || levelCm > points[count - 1].levelCm) return ESTIMATE_OUT_OF_RANGE;

    for (size_t i = 0; i < count; ++i) {
        if (fabsf(levelCm - points[i].levelCm) < 0.0001f) {
            litersOut = points[i].volumeLiters;
            return ESTIMATE_OK_EXACT;
        }
        if (i + 1 < count && levelCm > points[i].levelCm && levelCm < points[i + 1].levelCm) {
            const float span = points[i + 1].levelCm - points[i].levelCm;
            const float fraction = (levelCm - points[i].levelCm) / span;
            litersOut = points[i].volumeLiters + fraction * (points[i + 1].volumeLiters - points[i].volumeLiters);
            return ESTIMATE_OK_INTERPOLATED;
        }
    }
    return ESTIMATE_OUT_OF_RANGE;
}

DailyInflowTracker::DailyInflowTracker() { reset(); }

void DailyInflowTracker::reset() {
    baselineValid_ = false;
    previousDateValid_ = false;
    previousVolumeLiters_ = 0.0f;
    undatedLiters_ = 0.0f;
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        days_[i].dateKey = 0;
        days_[i].liters = 0.0f;
    }
}

void DailyInflowTracker::pruneHistory(uint32_t dateKey) {
    int64_t currentOrdinal = 0;
    if (!dateKeyToOrdinal(dateKey, currentOrdinal)) return;
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        if (days_[i].dateKey == 0) continue;
        int64_t storedOrdinal = 0;
        if (!dateKeyToOrdinal(days_[i].dateKey, storedOrdinal) ||
            storedOrdinal < currentOrdinal - static_cast<int64_t>(INFLOW_HISTORY_DAYS - 1U)) {
            days_[i].dateKey = 0;
            days_[i].liters = 0.0f;
        }
    }
}

void DailyInflowTracker::addToDate(uint32_t dateKey, float liters) {
    if (dateKey == 0 || liters <= 0.0f) return;
    pruneHistory(dateKey);
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        if (days_[i].dateKey == dateKey) {
            days_[i].liters += liters;
            return;
        }
    }
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        if (days_[i].dateKey == 0) {
            days_[i].dateKey = dateKey;
            days_[i].liters = liters;
            return;
        }
    }
    size_t oldest = 0;
    for (size_t i = 1; i < INFLOW_HISTORY_DAYS; ++i) {
        if (days_[i].dateKey < days_[oldest].dateKey) oldest = i;
    }
    days_[oldest].dateKey = dateKey;
    days_[oldest].liters = liters;
}

InflowUpdate DailyInflowTracker::observe(float volumeLiters, bool volumeValid, uint32_t dateKey, bool dateValid) {
    InflowUpdate result;
    result.baselineEstablished = false;
    result.positiveChange = false;
    result.positiveLiters = 0.0f;
    result.storedInDatedHistory = false;
    result.storedAsUndated = false;

    if (!volumeValid || !finiteFloat(volumeLiters) || volumeLiters < 0.0f) {
        baselineValid_ = false;
        previousDateValid_ = dateValid && dateKey != 0;
        return result;
    }

    int64_t dateOrdinal = 0;
    const bool validDate = dateValid && dateKeyToOrdinal(dateKey, dateOrdinal);
    if (validDate) pruneHistory(dateKey);
    if (!baselineValid_ || validDate != previousDateValid_) {
        previousVolumeLiters_ = volumeLiters;
        baselineValid_ = true;
        previousDateValid_ = validDate;
        result.baselineEstablished = true;
        return result;
    }

    if (volumeLiters > previousVolumeLiters_) {
        const float increase = volumeLiters - previousVolumeLiters_;
        result.positiveChange = true;
        result.positiveLiters = increase;
        if (validDate) {
            addToDate(dateKey, increase);
            result.storedInDatedHistory = true;
        } else {
            undatedLiters_ += increase;
            result.storedAsUndated = true;
        }
    }

    // Always move the baseline, including after a decrease. A decrease never subtracts from the inflow total.
    previousVolumeLiters_ = volumeLiters;
    previousDateValid_ = validDate;
    return result;
}

float DailyInflowTracker::litersForDate(uint32_t dateKey) const {
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        if (days_[i].dateKey == dateKey) return days_[i].liters;
    }
    return 0.0f;
}

float DailyInflowTracker::undatedLiters() const { return undatedLiters_; }
const InflowDay *DailyInflowTracker::history() const { return days_; }
size_t DailyInflowTracker::historySize() const { return INFLOW_HISTORY_DAYS; }

void DailyInflowTracker::exportSnapshot(DailyInflowSnapshot& out) const {
    out = DailyInflowSnapshot();
    out.baselineValid = baselineValid_;
    out.previousDateValid = previousDateValid_;
    out.previousVolumeLiters = previousVolumeLiters_;
    out.undatedLiters = undatedLiters_;
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) out.days[i] = days_[i];
}

bool DailyInflowTracker::restoreSnapshot(const DailyInflowSnapshot& snapshot) {
    if (snapshot.baselineValid > 1U || snapshot.previousDateValid > 1U ||
        !finiteFloat(snapshot.previousVolumeLiters) || snapshot.previousVolumeLiters < 0.0f ||
        !finiteFloat(snapshot.undatedLiters) || snapshot.undatedLiters < 0.0f) return false;
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        if (!finiteFloat(snapshot.days[i].liters) || snapshot.days[i].liters < 0.0f) return false;
        if (snapshot.days[i].dateKey == 0 && snapshot.days[i].liters != 0.0f) return false;
        if (snapshot.days[i].dateKey != 0) {
            int64_t dateOrdinal = 0;
            if (!dateKeyToOrdinal(snapshot.days[i].dateKey, dateOrdinal)) return false;
            for (size_t j = i + 1; j < INFLOW_HISTORY_DAYS; ++j) {
                if (snapshot.days[i].dateKey == snapshot.days[j].dateKey) return false;
            }
        }
    }
    baselineValid_ = snapshot.baselineValid != 0;
    previousDateValid_ = snapshot.previousDateValid != 0;
    previousVolumeLiters_ = snapshot.previousVolumeLiters;
    undatedLiters_ = snapshot.undatedLiters;
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) days_[i] = snapshot.days[i];
    return true;
}

} // namespace HWC
