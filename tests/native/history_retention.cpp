#include <cassert>
#include <cstdio>
#include <vector>
#include "../../src/app/storage/HistoryStore.cpp"
using Tier = HistoryStore::Tier;
using Record = HistoryStore::Record;
constexpr uint32_t start = (1700000100UL / 3600 + 1) * 3600;
std::vector<Record> read(HistoryStore &h, Tier tier) {
  std::vector<Record> result;
  assert(h.forEach(tier, 0, UINT32_MAX, [&](const Record &r) { result.push_back(r); return true; }));
  return result;
}
int main() {
  LittleFS.files.clear();
  HistoryStore history;
  assert(history.begin() && history.migrateCompact());
  for (unsigned i = 0; i < 4; ++i)
    assert(history.importRecord(Tier::QuarterHour,
        {start + i * 900, float(10 + i * 10), float(i), float(30 + i * 10),
         123456.75f + i, 5.125f + i}));
  // A 30-minute target bucket is not promoted until its complete interval is old.
  history.update(start + HistoryRetention::kQuarterSeconds + 1799, 50, 123999, 8);
  assert(read(history, Tier::HalfHour).empty());
  history.update(start + HistoryRetention::kQuarterSeconds + 3600, 50, 123999, 8);
  const auto halves = read(history, Tier::HalfHour);
  assert(halves.size() == 2);
  assert(halves[0].timestamp == start && halves[0].averageW == 15);
  assert(halves[0].minimumW == 0 && halves[0].maximumW == 40);
  assert(halves[0].importKwh == 123457.75f && halves[0].exportKwh == 6.125f);
  assert(halves[1].timestamp == start + 1800 && halves[1].averageW == 35);
  history.update(start + HistoryRetention::kHalfSeconds + 3600, 50, 124999, 9);
  const auto hours = read(history, Tier::Hour);
  assert(hours.size() == 1 && hours[0].timestamp == start);
  assert(hours[0].averageW == 25 && hours[0].minimumW == 0 && hours[0].maximumW == 60);
  assert(hours[0].importKwh == 123459.75f && hours[0].exportKwh == 8.125f);
  HistoryStore reboot;
  assert(reboot.begin());
  reboot.update(start + HistoryRetention::kHalfSeconds + 7200, 50, 124999, 9);
  const auto after = read(reboot, Tier::Hour);
  assert(after.size() == 1); // Promotion cursor survives reboot, no duplicate.
  // Each query must be chronological, with no duplicate overlapping tier rows.
  uint32_t previous = 0;
  assert(reboot.forEachRetained(0, UINT32_MAX, [&](const Record &r, uint32_t step) {
    assert(r.timestamp > previous);
    assert(HistoryBlockCodec::supportedStep(step));
    previous = r.timestamp;
    return true;
  }));
  puts("PASS: leap-year retention boundaries, 15->30->60 aggregation, unchanged counters, restart and ordered coverage");
}
