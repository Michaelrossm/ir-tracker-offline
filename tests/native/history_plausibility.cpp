#include <new>
#include <cstdlib>
static int allocationBudget = -1;
static unsigned migrationAllocations = 0;
void *operator new[](size_t bytes, const std::nothrow_t &) noexcept {
  ++migrationAllocations;
  if (allocationBudget == 0) return nullptr;
  if (allocationBudget > 0) --allocationBudget;
  try { return ::operator new[](bytes); }
  catch (const std::bad_alloc &) { return nullptr; }
}
#define main writer_regressions_main
#include "history_compact_writer.cpp"
#undef main

int main(int argc, char **) {
  if (argc > 1) {
    // Optional local replay: tier id/count followed by the six Record fields.
    // No device data is committed to the repository or changed on hardware.
    restore({});
    HistoryStore store;
    assert(store.begin());
    const char *paths[] = {"/minute.bin", "/quarter.bin", "/hour.bin", "/day.bin"};
    const uint32_t steps[] = {60, 900, 3600, 86400};
    unsigned id, count;
    uint32_t latest = 0;
    while (scanf("%u %u", &id, &count) == 2) {
      assert(id < 4);
      for (unsigned i = 0; i < count; ++i) {
        Record r{};
        unsigned timestamp;
        assert(scanf("%u %f %f %f %f %f", &timestamp, &r.averageW, &r.minimumW,
                     &r.maximumW, &r.importKwh, &r.exportKwh) == 6);
        r.timestamp = timestamp;
        latest = std::max(latest, r.timestamp);
        assert(store.importRecord(static_cast<Tier>(id),
            {epoch + i * steps[id], 0, 0, 0, 0, 0}));
        memcpy(LittleFS.files[paths[id]]->bytes.data() + 48 + i * sizeof(Record), &r, sizeof(r));
      }
    }
    HistoryStore replay;
    assert(replay.begin());
    const bool converted = replay.migrateCompact();
    if (converted) replay.update(latest + 60, 10, 20000, 1000);
    for (unsigned id = 0; id < 4; ++id)
      printf("Tier %u: count=%zu compact=%d\n", id, replay.count(static_cast<Tier>(id)),
             HistoryBlockCodec::load32(LittleFS.files[paths[id]]->bytes.data()) == HistoryBlockCodec::kMagic);
    printf("Replay complete: %d\n", converted);
    return converted ? 0 : 2;
  }
  Snapshot resolvable;
  for (unsigned mode = 0; mode < 7; ++mode) {
    restore({});
    HistoryStore original;
    assert(original.begin());
    Record input[4] = {
      {epoch, 10, 0, 20, 100, 1},
      {epoch + 900, 11, 0, 20, 101, 1},
      {epoch + 1800, 12, 0, 20, 102, 1},
      {epoch + 2700, 13, 0, 20, 103, 1}};
    for (const auto &r : input) assert(original.importRecord(Tier::QuarterHour, r));
    input[2].timestamp = input[1].timestamp;
    if (mode == 0) input[2] = input[1]; // Exact duplicate, including energy bits.
    if (mode == 1) input[2].importKwh = 500; // Only first candidate fits neighbours.
    if (mode == 2) input[1].importKwh = 50; // Only second candidate fits neighbours.
    if (mode == 3) { input[2].averageW = 5000; input[2].maximumW = 6000; }
    if (mode == 4) input[3].importKwh = 50; // Counter reset: do not choose.
    if (mode == 5) for (auto &r : input) r.importKwh = r.exportKwh = NAN;
    if (mode == 6) { input[2].averageW = 559036928.0f; input[2].maximumW = 352193150976.0f; }
    auto &bytes = LittleFS.files["/quarter.bin"]->bytes;
    memcpy(bytes.data() + 48, input, sizeof(input));
    const auto source = bytes;
    if (mode == 6) resolvable = snapshot();
    HistoryStore history;
    assert(history.begin());
    const bool converted = history.migrateCompact();
    assert(converted == (mode < 3 || mode == 6));
    if (!converted) {
      assert(LittleFS.files["/quarter.bin"]->bytes == source);
      continue;
    }
    const auto values = records(history, Tier::QuarterHour);
    assert(values.size() == 3);
    assert(values[1].timestamp == input[1].timestamp);
    const Record &selected = input[mode == 2 ? 2 : 1];
    assert(values[1].averageW == selected.averageW);
    assert(!memcmp(&values[1].importKwh, &selected.importKwh, 4));
    assert(values.back().timestamp == epoch + 2700); // Missing interval stays missing.
  }
  // Invalid-only archives become an empty compact ring; invalid runs longer
  // than the look-ahead and corruption at either end never invent observations.
  for (bool allInvalid : {false, true}) {
    restore({});
    HistoryStore original;
    assert(original.begin());
    for (unsigned i = 0; i < 70; ++i)
      assert(original.importRecord(Tier::QuarterHour,
          {epoch + i * 900, 10, 0, 20, float(100 + i), 1}));
    auto &bytes = LittleFS.files["/quarter.bin"]->bytes;
    for (unsigned i = 0; i < 70; ++i) {
      if (!allInvalid && (i == 1 || i == 68)) continue;
      Record damaged;
      memcpy(&damaged, bytes.data() + 48 + i * sizeof(Record), sizeof(damaged));
      damaged.averageW = INFINITY;
      memcpy(bytes.data() + 48 + i * sizeof(Record), &damaged, sizeof(damaged));
    }
    HistoryStore history;
    assert(history.begin() && history.migrateCompact());
    const auto values = records(history, Tier::QuarterHour);
    assert(values.size() == (allInvalid ? 0 : 2));
    if (!allInvalid) {
      assert(values.front().timestamp == epoch + 900);
      assert(values.back().timestamp == epoch + 68 * 900);
      assert(values.front().importKwh == 101 && values.back().importKwh == 168);
    }
  }
  restore(resolvable);
  HistoryStore successful;
  assert(successful.begin());
  fsMutationCount = 0;
  migrationAllocations = 0;
  assert(successful.migrateCompact());
  const int operations = fsMutationCount;
  const unsigned allocations = migrationAllocations;
  for (unsigned fail = 0; fail < allocations; ++fail) {
    restore(resolvable);
    HistoryStore attempt;
    assert(attempt.begin());
    allocationBudget = static_cast<int>(fail);
    attempt.migrateCompact();
    allocationBudget = -1;
    HistoryStore restarted;
    assert(restarted.begin() && restarted.migrateCompact());
    const auto values = records(restarted, Tier::QuarterHour);
    assert(values.size() == 3 && values[1].importKwh == 101);
    assert(values.front().timestamp == epoch && values.back().timestamp == epoch + 2700);
  }
  for (int cut = 0; cut < operations; ++cut) {
    for (size_t prefix : {size_t(0), size_t(17), SIZE_MAX}) {
      restore(resolvable);
      HistoryStore attempt;
      assert(attempt.begin());
      fsMutationCount = 0; fsMutationCut = cut; fsMutationPrefix = prefix;
      try { attempt.migrateCompact(); } catch (const TestPowerCut &) {}
      fsMutationCut = -1;
      HistoryStore restarted;
      assert(restarted.begin() && restarted.migrateCompact());
      const auto values = records(restarted, Tier::QuarterHour);
      assert(values.size() == 3 && values[1].importKwh == 101);
      assert(values[0].timestamp == epoch && values[2].timestamp == epoch + 2700);
    }
  }
  puts("PASS: duplicate selection, ambiguous spikes/resets/unknown counters preserved, unchanged timestamps and interrupted migration");
}
