#define main writer_regressions_main
#include "history_compact_writer.cpp"
#undef main

int main() {
  restore({});
  HistoryStore legacy;
  assert(legacy.begin());
  const uint32_t first = (epoch / 19200 + 1) * 19200;
  for (unsigned i = 0; i < 2880; ++i)
    assert(legacy.importRecord(Tier::Minute,
        {first + i * 60, 12.3f, -1.2f, 40.1f, 100.125f + i, 5.125f}));
  assert(legacy.migrateCompact());
  for (unsigned i = 0; i < 288; ++i)
    assert(legacy.importRecord(Tier::FiveMinute,
        {first + i * 300, 12.3f, -1.2f, 40.1f, 104.125f + i * 5, 5.125f}));
  const auto initial = snapshot();
  fsMutationCount = 0;
  legacy.update(first + 48 * 3600, 10, 3000, 5.125f);
  const int operations = fsMutationCount;
  assert(LittleFS.files["/minute.bin"]->bytes.size() == 24 * HistoryBlockCodec::kBlockBytes);
  const auto expected = records(legacy, Tier::Minute);
  assert(expected.size() == 1536 && expected.front().timestamp == first + 1344 * 60);
  for (int cut = 0; cut < operations; ++cut) {
    for (size_t prefix : {size_t(0), size_t(17), SIZE_MAX}) {
      restore(initial);
      HistoryStore attempt;
      assert(attempt.begin());
      fsMutationCount = 0; fsMutationCut = cut; fsMutationPrefix = prefix;
      try { attempt.update(first + 48 * 3600, 10, 3000, 5.125f); } catch (const TestPowerCut &) {}
      fsMutationCut = -1;
      HistoryStore reboot;
      assert(reboot.begin());
      reboot.update(first + 48 * 3600, 10, 3000, 5.125f);
      check(records(reboot, Tier::Minute), expected);
      assert(LittleFS.files["/minute.bin"]->bytes.size() == 24 * HistoryBlockCodec::kBlockBytes);
      assert(records(reboot, Tier::FiveMinute).size() == 288);
    }
  }
  // Even a CRC-valid but wrong target must not authorize deletion of minutes.
  restore(initial);
  auto &five = LittleFS.files["/five.bin"]->bytes;
  Record wrong{first, 15, -1.2f, 40.1f, 104.125f, 5.125f};
  assert(HistoryBlockCodec::put(five.data(), HistoryBlockCodec::kBlockBytes, wrong));
  assert(HistoryBlockCodec::seal(five.data(), HistoryBlockCodec::kBlockBytes));
  const auto minutes = LittleFS.files["/minute.bin"]->bytes;
  HistoryStore blocked;
  assert(blocked.begin());
  blocked.update(first + 48 * 3600, 10, 3000, 5.125f);
  assert(LittleFS.files["/minute.bin"]->bytes == minutes);
  puts("PASS: verified minute space reclamation, bit-exact retained records, interrupted copy/restart, wrong target rejected");
}
