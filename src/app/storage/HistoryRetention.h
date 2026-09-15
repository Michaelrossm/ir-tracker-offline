#pragma once

#include <stdint.h>

namespace HistoryRetention {
// A leap year must also retain every quarter-hour value.
constexpr uint32_t kYearSeconds = 366UL * 86400UL;
constexpr uint32_t kMinuteSeconds = 24UL * 3600UL;
constexpr uint32_t kFiveMinuteSeconds = 2UL * 86400UL;
constexpr uint32_t kQuarterSeconds = 460UL * 86400UL;
constexpr uint32_t kHalfSeconds = kQuarterSeconds + 365UL * 86400UL;
constexpr uint32_t kHourSeconds = kHalfSeconds + 365UL * 86400UL;
}
