// Included by HistoryStore.cpp: the existing store remains the only writer.
#include "HistoryRetention.h"

bool HistoryStore::promoteRetained(Tier sourceId, Tier targetId,
                                  uint32_t cutoff) {
  TierState &target = state(targetId);
  const uint32_t end = cutoff - cutoff % target.seconds;
  if (!end || target.lastWrittenBucket >= end) return true;
  const uint32_t since = target.lastWrittenBucket
                             ? target.lastWrittenBucket + target.seconds : 0;
  if (since >= end) return true;
  Record combined{};
  double sum = 0;
  uint32_t samples = 0;
  bool ok = true;
  const auto flush = [&]() {
    if (!samples) return true;
    combined.averageW = static_cast<float>(sum / samples);
    if (!writeRecord(target, combined)) return false;
    samples = 0;
    sum = 0;
    return true;
  };
  const bool read = forEach(sourceId, since, end - 1, [&](const Record &record) {
    const uint32_t bucket = record.timestamp - record.timestamp % target.seconds;
    if (samples && bucket != combined.timestamp && !flush()) {
      ok = false;
      return false;
    }
    if (!samples) {
      combined = record;
      combined.timestamp = bucket;
    } else {
      combined.minimumW = std::min(combined.minimumW, record.minimumW);
      combined.maximumW = std::max(combined.maximumW, record.maximumW);
      // Cumulative counters are copied, never rounded or averaged.
      combined.importKwh = record.importKwh;
      combined.exportKwh = record.exportKwh;
    }
    sum += record.averageW;
    ++samples;
    return true;
  });
  return read && ok && flush();
}

void HistoryStore::update(uint32_t epoch, double powerW, double importKwh,
                          double exportKwh) {
  if (!mounted_ || readOnly() || epoch < 1700000000UL) return;
  if (state(Tier::QuarterHour).header.magic != HistoryBlockCodec::kMagic ||
      state(Tier::HalfHour).header.magic != HistoryBlockCodec::kMagic ||
      state(Tier::FiveMinute).header.magic != HistoryBlockCodec::kMagic) {
    // Preserve the previous two-year hourly coverage while the source tier
    // still needs legacy handling. A converted independent tier is not a
    // reason to shorten retention of the remaining old observations.
    state(Tier::Hour).capacity = std::max(state(Tier::Hour).capacity, uint32_t(17520));
    state(Tier::Minute).capacity = std::max(state(Tier::Minute).capacity, uint32_t(2880));
    for (Tier tier : {Tier::Minute, Tier::QuarterHour, Tier::Hour, Tier::Day})
      updateTier(state(tier), epoch, powerW, importKwh, exportKwh);
    return;
  }
  if (state(Tier::Hour).header.magic != HistoryBlockCodec::kMagic)
    updateTier(state(Tier::Hour), epoch, powerW, importKwh, exportKwh);
  // These independent tiers must keep receiving samples even if an older
  // promotion cannot be committed. The quarter ring must not overwrite its
  // unpromoted source, but a fault there must not stop live/minute recording.
  // Commit the second day's five-minute values before the minute ring advances.
  // Legacy minute rings with clock conflicts remain untouched and keep 48 hours.
  if (state(Tier::Minute).header.magic != HistoryBlockCodec::kMagic ||
      (promoteRetained(Tier::Minute, Tier::FiveMinute,
                       epoch - HistoryRetention::kMinuteSeconds) &&
       pruneCompactMinute(epoch - HistoryRetention::kMinuteSeconds)))
    updateTier(state(Tier::Minute), epoch, powerW, importKwh, exportKwh);
  updateTier(state(Tier::Day), epoch, powerW, importKwh, exportKwh);
  TierState &quarter = state(Tier::QuarterHour);
  const uint32_t bucket = epoch - epoch % quarter.seconds;
  if (quarter.aggregate.bucket != bucket) {
    if (!pruneCompactHour(epoch - HistoryRetention::kHourSeconds)) return;
    // Preserve the older half-hour before appending a half-hour, and preserve
    // the older quarter-hour before its ring can overwrite it. A failed write
    // prevents upstream advancement; retry uses the committed target cursor.
    if (!promoteRetained(Tier::HalfHour, Tier::Hour,
                         epoch - HistoryRetention::kHalfSeconds) ||
        !promoteRetained(Tier::QuarterHour, Tier::HalfHour,
                         epoch - HistoryRetention::kQuarterSeconds)) return;
  }
  updateTier(quarter, epoch, powerW, importKwh, exportKwh);
}

bool HistoryStore::forEachRetained(uint32_t since, uint32_t until,
                                  const RetainedCallback &callback) {
  if (since > until) return true;
  const Tier order[] = {Tier::Day, Tier::Hour, Tier::HalfHour,
                        Tier::QuarterHour, Tier::FiveMinute, Tier::Minute};
  uint32_t first[6];
  for (size_t i = 0; i < 6; ++i) {
    first[i] = UINT32_MAX;
    if (!forEach(order[i], 0, UINT32_MAX, [&](const Record &record) {
          first[i] = record.timestamp;
          return false;
        })) return false;
  }
  uint32_t lower = since;
  bool stopped = false;
  for (size_t i = 0; i < 6 && lower <= until && !stopped; ++i) {
    uint32_t finer = UINT32_MAX;
    for (size_t j = i + 1; j < 6; ++j) finer = std::min(finer, first[j]);
    if (finer <= lower) continue;
    const uint32_t upper = finer == UINT32_MAX ? until : std::min(until, finer - 1);
    if (first[i] != UINT32_MAX &&
        !forEach(order[i], lower, upper, [&](const Record &record) {
          if (!callback(record, state(order[i]).seconds)) {
            stopped = true;
            return false;
          }
          return true;
        })) return false;
    // Never fill holes inside a finer tier with overlapping coarse values.
    // Coarse legacy data is retained only before finer coverage begins.
    if (finer == UINT32_MAX || upper == UINT32_MAX) break;
    lower = std::max(lower, finer);
  }
  return true;
}
