#define main writer_regressions_main
#include "history_compact_writer.cpp"
#undef main

int main() {
  for (bool duplicate : {false, true}) {
    restore({});
    HistoryStore original;
    assert(original.begin());
    const unsigned count = duplicate ? 4 : 40;
    for (unsigned i = 0; i < count; ++i)
      assert(original.importRecord(Tier::QuarterHour,
          {epoch + 1800 + i * 900, float(10 + i), 0, 80, float(100 + i), float(i)}));
    assert(original.importRecord(Tier::Day, {epoch, 12.34f, 0, 20, 100, 0}));
    // Model an old imported/clock-corrected record. Header counts and physical
    // ring geometry remain intact; the second observation must not be lost.
    auto &bytes = LittleFS.files["/quarter.bin"]->bytes;
    const uint32_t timestamp = duplicate ? epoch + 2700 : epoch + 900;
    memcpy(bytes.data() + 48 + (duplicate ? 2 : 39) * sizeof(Record), &timestamp, sizeof(timestamp));
    const auto source = bytes;
    const auto initial = snapshot();
    HistoryStore history;
    assert(history.begin());
    fsMutationCount = 0;
    assert(!history.migrateCompact());
    const int operations = fsMutationCount;
    assert(LittleFS.files["/quarter.bin"]->bytes == source);
    assert(HistoryBlockCodec::load32(LittleFS.files["/day.bin"]->bytes.data()) ==
           HistoryBlockCodec::kMagic);
    assert(records(history, Tier::QuarterHour).size() == count);
    assert(history.importRecord(Tier::QuarterHour, {epoch + 40000, 15, 0, 20, 144, 44}));
    HistoryStore restart;
    assert(restart.begin() && restart.count(Tier::QuarterHour) == count + 1);
    for (int cut = 0; cut < operations; ++cut) {
      for (size_t prefix : {size_t(0), size_t(17), SIZE_MAX}) {
        restore(initial);
        HistoryStore attempt;
        assert(attempt.begin());
        fsMutationCount = 0; fsMutationCut = cut; fsMutationPrefix = prefix;
        try { attempt.migrateCompact(); } catch (const TestPowerCut &) {}
        fsMutationCut = -1;
        HistoryStore reboot;
        assert(reboot.begin());
        assert(!reboot.migrateCompact());
        assert(LittleFS.files["/quarter.bin"]->bytes == source);
        assert(records(reboot, Tier::QuarterHour).size() == count);
        assert(HistoryBlockCodec::load32(LittleFS.files["/day.bin"]->bytes.data()) ==
               HistoryBlockCodec::kMagic);
      }
    }
  }
  restore({});
  HistoryStore mixed;
  assert(mixed.begin());
  for (unsigned i = 0; i < 3; ++i)
    assert(mixed.importRecord(Tier::Minute, {epoch + i * 60, float(10 + i), 0, 20, 100, 0}));
  auto &minute = LittleFS.files["/minute.bin"]->bytes;
  memcpy(minute.data() + 48 + sizeof(Record), &epoch, sizeof(epoch));
  const uint32_t quarter = (epoch / 1800 + 1) * 1800;
  assert(mixed.importRecord(Tier::QuarterHour, {quarter, 10, 0, 20, 100, 0}));
  assert(mixed.importRecord(Tier::QuarterHour, {quarter + 900, 30, 20, 40, 101, 1}));
  HistoryStore reopened;
  assert(reopened.begin() && !reopened.migrateCompact());
  reopened.update(quarter + 1800 + HistoryRetention::kQuarterSeconds, 10, 105, 1);
  const auto halves = records(reopened, Tier::HalfHour);
  assert(halves.size() == 1 && halves[0].averageW == 20 && halves[0].importKwh == 101);
  puts("PASS: conflicting originals byte-identical, independent conversion, mixed-format retention and interruption recovery");
}
