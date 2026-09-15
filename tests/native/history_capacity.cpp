#include <cassert>
#include <cstdio>
#include <vector>
#include "../../src/app/storage/HistoryStore.cpp"
using Tier = HistoryStore::Tier;
using Record = HistoryStore::Record;
constexpr uint32_t start = (1700000100UL / 86400 + 1) * 86400;
std::vector<Record> rows(HistoryStore &h, Tier tier) {
  std::vector<Record> result;
  assert(h.forEach(tier, 0, UINT32_MAX, [&](const Record &r) { result.push_back(r); return true; }));
  return result;
}
int main() {
  LittleFS.files.clear();
  LittleFS.enforceCapacity = true;
  HistoryStore full;
  assert(full.begin());
  const Tier tiers[] = {Tier::Minute, Tier::QuarterHour, Tier::Hour, Tier::Day};
  const unsigned counts[] = {2880, 17280, 17520, 7300};
  const unsigned steps[] = {60, 900, 3600, 86400};
  std::vector<Record> before[4];
  for (unsigned tier = 0; tier < 4; ++tier) {
    for (unsigned i = 0; i < counts[tier]; ++i)
      assert(full.importRecord(tiers[tier],
          {start + i * steps[tier], 12.34f, -1.29f, 40.19f, float(i) + .125f, 5.125f}));
    before[tier] = rows(full, tiers[tier]);
  }
  const auto beforeBytes = LittleFS.usedBytes();
  const bool migrated = full.migrateCompact();
  for (unsigned tier = 0; tier < 4; ++tier) {
    const auto actual = rows(full, tiers[tier]);
    assert(actual.size() == before[tier].size());
    for (size_t i = 0; i < actual.size(); ++i) {
      assert(actual[i].timestamp == before[tier][i].timestamp);
      assert(!memcmp(&actual[i].importKwh, &before[tier][i].importKwh, 4));
      assert(!memcmp(&actual[i].exportKwh, &before[tier][i].exportKwh, 4));
    }
  }
  if (!migrated) {
    printf("Full legacy migration rejected safely: before=%zu used=%zu total=%zu\n",
           beforeBytes, LittleFS.usedBytes(), LittleFS.totalBytes());
  }
  assert(migrated);
  assert(full.usedBytes() < full.totalBytes());
  // Every minute/quarter remains independently writable after migration.
  assert(full.importRecord(Tier::Minute, {start + counts[0] * 60, 0, 0, 0, 0, 0}));
  // Independent fully grown/wrapped 15-minute ring; inspect through production
  // reader after reboot, not private in-memory state.
  LittleFS.files.clear();
  HistoryStore growing;
  assert(growing.begin() && growing.migrateCompact());
  constexpr unsigned samples = 370 * 96;
  for (unsigned i = 0; i < samples; ++i)
    assert(growing.importRecord(Tier::QuarterHour,
        {start + i * 900, 12.34f, -1.29f, 40.19f, float(i) + .125f, 5.125f}));
  HistoryStore restarted;
  assert(restarted.begin());
  const auto actual = rows(restarted, Tier::QuarterHour);
  assert(actual.size() >= 366 * 96);
  assert(actual.back().timestamp == start + (samples - 1) * 900);
  assert(actual.front().timestamp <= actual.back().timestamp - (366 * 96 - 1) * 900);
  for (size_t i = 1; i < actual.size(); ++i)
    assert(actual[i].timestamp == actual[i - 1].timestamp + 900);
  printf("PASS: full legacy migration preserves %zu bytes of source records; wrapped quarter ring retains %zu samples\n",
         size_t(2880 + 17280 + 17520 + 7300) * sizeof(Record), actual.size());
}
