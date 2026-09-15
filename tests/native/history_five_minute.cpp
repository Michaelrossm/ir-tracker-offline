#define main existing_retention_test_main
#include "history_retention.cpp"
#undef main

int main() {
  LittleFS.files.clear();
  HistoryStore history;
  assert(history.begin() && history.migrateCompact());
  const uint32_t first = (start / 300 + 1) * 300;
  for (unsigned i = 0; i < 10; ++i)
    assert(history.importRecord(Tier::Minute,
        {first + i * 60, float(i * 10), float(i), float(100 + i),
         1000.125f + i, 7.125f + i}));
  history.update(first + HistoryRetention::kMinuteSeconds + 299, 10, 2000, 20);
  assert(read(history, Tier::FiveMinute).empty());
  std::map<std::string, std::vector<uint8_t>> beforePromotion;
  for (const auto &entry : LittleFS.files) beforePromotion[entry.first] = entry.second->bytes;
  fsMutationCount = 0;
  history.update(first + HistoryRetention::kMinuteSeconds + 300, 10, 2000, 20);
  const int operations = fsMutationCount;
  auto values = read(history, Tier::FiveMinute);
  assert(values.size() == 1 && values[0].timestamp == first);
  assert(values[0].averageW == 20 && values[0].minimumW == 0 && values[0].maximumW == 104);
  assert(values[0].importKwh == 1004.125f && values[0].exportKwh == 11.125f);
  HistoryStore reboot;
  assert(reboot.begin());
  reboot.update(first + HistoryRetention::kMinuteSeconds + 600, 10, 2000, 20);
  values = read(reboot, Tier::FiveMinute);
  assert(values.size() == 2 && values[1].averageW == 70);
  assert(values[1].importKwh == 1009.125f && values[1].exportKwh == 16.125f);
  reboot.update(first + HistoryRetention::kMinuteSeconds + 600, 10, 2000, 20);
  assert(read(reboot, Tier::FiveMinute).size() == 2);
  for (int cut = 0; cut < operations; ++cut) {
    for (size_t prefix : {size_t(0), size_t(17), SIZE_MAX}) {
      LittleFS.files.clear();
      for (const auto &entry : beforePromotion) {
        auto file = std::make_shared<TestFileData>();
        file->bytes = entry.second;
        LittleFS.files[entry.first] = file;
      }
      HistoryStore attempt;
      assert(attempt.begin());
      fsMutationCount = 0; fsMutationCut = cut; fsMutationPrefix = prefix;
      try { attempt.update(first + HistoryRetention::kMinuteSeconds + 300, 10, 2000, 20); }
      catch (const TestPowerCut &) {}
      fsMutationCut = -1;
      HistoryStore recovered;
      assert(recovered.begin());
      recovered.update(first + HistoryRetention::kMinuteSeconds + 600, 10, 2000, 20);
      const auto rows = read(recovered, Tier::FiveMinute);
      assert(rows.size() == 2);
      assert(rows[0].averageW == 20 && rows[1].averageW == 70);
      assert(rows[0].importKwh == 1004.125f && rows[1].importKwh == 1009.125f);
      assert(read(recovered, Tier::Minute).size() >= 10);
    }
  }

  // Fill every new-format ring, including wrap and its extra working block.
  LittleFS.files.clear();
  LittleFS.enforceCapacity = true;
  HistoryStore full;
  assert(full.begin() && full.migrateCompact());
  const Tier tiers[] = {Tier::Minute, Tier::FiveMinute, Tier::QuarterHour,
                        Tier::HalfHour, Tier::Hour, Tier::Day};
  const unsigned counts[] = {1440, 288, 460 * 96, 365 * 48, 365 * 24, 3650};
  const unsigned steps[] = {60, 300, 900, 1800, 3600, 86400};
  for (unsigned t = 0; t < 6; ++t) {
    for (unsigned i = 0; i < counts[t] + 128; ++i)
      assert(full.importRecord(tiers[t],
          {start + i * steps[t], 12.3f, -1.2f, 40.1f, float(i) + .125f, 5.125f}));
    const auto records = read(full, tiers[t]);
    assert(records.size() >= counts[t]);
    assert(records.back().timestamp == start + (counts[t] + 127) * steps[t]);
  }
  assert(full.totalBytes() - full.usedBytes() >= 48 * 1024);
  HistoryStore again;
  assert(again.begin() && again.compactActive());
  for (unsigned t = 0; t < 6; ++t) assert(read(again, tiers[t]).size() >= counts[t]);
  const size_t filledBytes = full.usedBytes();
  for (unsigned t = 0; t < 6; ++t)
    for (unsigned i = counts[t] + 128; i < counts[t] + 256; ++i)
      assert(again.importRecord(tiers[t],
          {start + i * steps[t], 12.3f, -1.2f, 40.1f, float(i) + .125f, 5.125f}));
  assert(again.usedBytes() == filledBytes); // A reboot must not enlarge any ring.
  printf("PASS: 1->5 minute boundary, counters, restart, full six-tier rings; bytes=%zu reserve=%zu\n",
         full.usedBytes(), full.totalBytes() - full.usedBytes());
}
