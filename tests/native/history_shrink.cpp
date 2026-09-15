#define main writer_regressions_main
#include "history_compact_writer.cpp"
#undef main

int main() {
  restore({});
  HistoryStore original;
  assert(original.begin());
  const uint32_t first = 1700006400UL;
  for (unsigned i = 0; i < 140; ++i) {
    const uint32_t timestamp = first + i * 64 * 3600;
    assert(original.importRecord(Tier::Hour, {timestamp, 12.34f, 0, 20, float(i), 0}));
  }
  assert(original.importRecord(Tier::Day, {first - first % 86400, 12, 0, 20, 0, 0}));
  assert(original.migrateCompact());
  const auto initial = snapshot();
  const auto before = records(original, Tier::Hour);
  const uint32_t now = first + 64 * 3600 + HistoryRetention::kHourSeconds;
  fsMutationCount = 0;
  original.update(now, 12, 200, 0);
  const int operations = fsMutationCount;
  assert(LittleFS.files["/hour.bin"]->bytes.size() == 139 * HistoryBlockCodec::kBlockBytes);
  auto expected = before;
  expected.erase(expected.begin());
  check(records(original, Tier::Hour), expected);
  for (int cut = 0; cut < operations; ++cut) {
    for (size_t prefix : {size_t(0), size_t(11), SIZE_MAX}) {
      restore(initial);
      HistoryStore interrupted;
      assert(interrupted.begin());
      fsMutationCount = 0; fsMutationCut = cut; fsMutationPrefix = prefix;
      try { interrupted.update(now, 12, 200, 0); } catch (const TestPowerCut &) {}
      fsMutationCut = -1;
      HistoryStore reboot;
      assert(reboot.begin());
      reboot.update(now, 12, 200, 0);
      check(records(reboot, Tier::Hour), expected);
      assert(LittleFS.files["/hour.bin"]->bytes.size() == 139 * HistoryBlockCodec::kBlockBytes);
    }
  }
  printf("PASS: %d physical shrink interruption cases; retained timestamps and counters unchanged\n", operations * 3);
}
