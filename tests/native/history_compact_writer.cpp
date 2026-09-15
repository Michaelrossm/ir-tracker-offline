#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "../../src/app/storage/HistoryStore.cpp"

using Record = HistoryStore::Record;
using Tier = HistoryStore::Tier;
using Snapshot = std::map<std::string, std::vector<uint8_t>>;
constexpr uint32_t epoch = 1700000100;

Snapshot snapshot() {
  Snapshot result;
  for (const auto &file : LittleFS.files) result[file.first] = file.second->bytes;
  return result;
}
void restore(const Snapshot &saved) {
  LittleFS.files.clear();
  for (const auto &file : saved) {
    auto data = std::make_shared<TestFileData>();
    data->bytes = file.second;
    LittleFS.files[file.first] = data;
  }
  failedWriteAfter = corruptWriteAfter = fsMutationCut = -1;
  failedWritePrefix = 0;
  fsMutationCount = 0;
  LittleFS.capacityBytes = 1310720;
}
std::vector<Record> records(HistoryStore &history, Tier tier) {
  std::vector<Record> result;
  assert(history.forEach(tier, 0, UINT32_MAX, [&](const Record &r) {
    result.push_back(r); return true;
  }));
  return result;
}
void check(const std::vector<Record> &actual, const std::vector<Record> &before) {
  assert(actual.size() == before.size());
  for (size_t i = 0; i < before.size(); ++i) {
    assert(actual[i].timestamp == before[i].timestamp);
    assert(std::fabs(actual[i].averageW - before[i].averageW) <= 0.051f);
    assert(std::fabs(actual[i].minimumW - before[i].minimumW) <= 0.051f);
    assert(std::fabs(actual[i].maximumW - before[i].maximumW) <= 0.051f);
    assert(!memcmp(&actual[i].importKwh, &before[i].importKwh, 4));
    assert(!memcmp(&actual[i].exportKwh, &before[i].exportKwh, 4));
  }
}
int main() {
  restore({});
  HistoryStore legacy;
  assert(legacy.begin());
  for (unsigned i = 0; i < 130; ++i) {
    if (i == 62 || i == 63 || i == 64) continue; // gap across block boundary
    assert(legacy.importRecord(Tier::QuarterHour,
        {epoch + i * 900, 12.34f, -0.15f, 90.19f, 123456.75f + i, NAN}));
  }
  const auto before = records(legacy, Tier::QuarterHour);
  const Snapshot original = snapshot();
  fsMutationCount = 0;
  assert(legacy.migrateCompact());
  const int migrationOperations = fsMutationCount;
  assert(legacy.compactActive() && !legacy.readOnly());
  check(records(legacy, Tier::QuarterHour), before);
  const Snapshot converted = snapshot();

  unsigned cases = 0;
  for (int cut = 0; cut < migrationOperations; ++cut) {
    for (size_t prefix : {size_t(0), size_t(1), size_t(31), size_t(511), SIZE_MAX}) {
      restore(original);
      HistoryStore attempt;
      assert(attempt.begin());
      fsMutationCount = 0; fsMutationCut = cut; fsMutationPrefix = prefix;
      try { attempt.migrateCompact(); } catch (const TestPowerCut &) {}
      fsMutationCut = -1;
      HistoryStore restarted;
      assert(restarted.begin());
      assert(restarted.migrateCompact());
      assert(restarted.compactActive());
      check(records(restarted, Tier::QuarterHour), before);
      ++cases;
    }
  }
  restore(converted);
  HistoryStore writer;
  assert(writer.begin() && writer.compactActive());
  const Record next{epoch + 130 * 900, 45.67f, 0, 99.99f, 123700.125f, 5.125f};
  fsMutationCount = 0;
  assert(writer.importRecord(Tier::QuarterHour, next));
  const int writeOperations = fsMutationCount;
  auto appended = before; appended.push_back(next);
  check(records(writer, Tier::QuarterHour), appended);
  for (int cut = 0; cut < writeOperations; ++cut) {
    for (size_t prefix : {size_t(0), size_t(1), size_t(31), size_t(511), SIZE_MAX}) {
      restore(converted);
      HistoryStore attempt;
      assert(attempt.begin());
      fsMutationCount = 0; fsMutationCut = cut; fsMutationPrefix = prefix;
      try { attempt.importRecord(Tier::QuarterHour, next); } catch (const TestPowerCut &) {}
      fsMutationCut = -1;
      HistoryStore restarted;
      assert(restarted.begin());
      auto actual = records(restarted, Tier::QuarterHour);
      // The last unacknowledged record can be absent or committed; all prior
      // records must survive, and a replay must never duplicate a sample.
      assert(actual.size() == before.size() || actual.size() == appended.size());
      check(actual, actual.size() == before.size() ? before : appended);
      HistoryStore again;
      assert(again.begin());
      check(records(again, Tier::QuarterHour), actual);
      ++cases;
    }
  }
  // Exercise the actual age cascade, not just the format converter.
  restore({});
  HistoryStore retained;
  assert(retained.begin() && retained.migrateCompact());
  const uint32_t now = 1800000000UL;
  const uint32_t oldQuarter = (now - HistoryRetention::kQuarterSeconds - 3600) / 1800 * 1800;
  const uint32_t oldHalf = (now - HistoryRetention::kHalfSeconds - 7200) / 3600 * 3600;
  assert(retained.importRecord(Tier::QuarterHour, {oldQuarter, 10, 5, 15, 100, 2}));
  assert(retained.importRecord(Tier::QuarterHour, {oldQuarter + 900, 30, 20, 40, 101, 3}));
  assert(retained.importRecord(Tier::HalfHour, {oldHalf, 40, 20, 50, 80, 1}));
  assert(retained.importRecord(Tier::HalfHour, {oldHalf + 1800, 60, 30, 80, 81, 2}));
  retained.update(now, 5, 102, 3);
  auto halves = records(retained, Tier::HalfHour);
  auto hours = records(retained, Tier::Hour);
  assert(halves.back().timestamp == oldQuarter && halves.back().averageW == 20);
  assert(halves.back().minimumW == 5 && halves.back().maximumW == 40);
  assert(halves.back().importKwh == 101 && halves.back().exportKwh == 3);
  assert(hours.back().timestamp == oldHalf && hours.back().averageW == 50);
  assert(hours.back().minimumW == 20 && hours.back().maximumW == 80);
  assert(hours.back().importKwh == 81 && hours.back().exportKwh == 2);
  retained.update(now + 1, 5, 102, 3);
  assert(records(retained, Tier::HalfHour).size() == halves.size());
  assert(records(retained, Tier::Hour).size() == hours.size());
  printf("PASS: %u migration/write interruption cases, age cascade, rounded powers and bit-exact energies\n", cases);
}
