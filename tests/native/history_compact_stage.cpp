#include <cassert>
#include <cstdio>
#include "../../src/app/storage/HistoryStore.cpp"

using Record = HistoryStore::Record;
using namespace HistoryBlockCodec;
constexpr auto tier = HistoryStore::Tier::QuarterHour;
constexpr uint32_t epoch = 1700000100;

void populate(HistoryStore &history, unsigned count = 130) {
  LittleFS.files.clear();
  LittleFS.capacityBytes = 1310720;
  failedWriteAfter = -1;
  failedWritePrefix = 0;
  corruptWriteAfter = -1;
  assert(history.begin());
  for (unsigned i = 0; i < count; ++i)
    assert(history.importRecord(tier,
        {epoch + i * 900, 10.14f, -10.25f, 100.19f, 123456.75f, NAN}));
}

int main() {
  HistoryStore history;
  populate(history);
  const auto old = LittleFS.files["/quarter.bin"]->bytes;
  assert(history.stageCompactCopy(tier));
  const auto prepared = LittleFS.files[HistoryStore::compactStagePath(tier)]->bytes;
  assert(prepared.size() == 3 * kBlockBytes);
  assert(old == LittleFS.files["/quarter.bin"]->bytes);
  assert(!history.readOnly());
  assert(!history.stageCompactCopy(tier)); // Never overwrite even a valid stage.
  assert(prepared == LittleFS.files[HistoryStore::compactStagePath(tier)]->bytes);
  assert(history.resumeCompactCopy(tier));
  assert(prepared == LittleFS.files[HistoryStore::compactStagePath(tier)]->bytes);
  HistoryStore reboot;
  assert(reboot.begin() && !reboot.readOnly() && reboot.count(tier) == 130);
  for (size_t offset = 0; offset < prepared.size(); offset += kBlockBytes)
    assert(valid(prepared.data() + offset, kBlockBytes));

  // Failure before or within any of the three destination writes: after a
  // simulated restart the untouched IRH1 source remains the only active file.
  for (int write = 0; write < 3; ++write) {
    for (size_t prefix : {size_t(0), size_t(1), size_t(12), size_t(31),
                          kBlockBytes / 2, kBlockBytes - 1}) {
      HistoryStore attempt;
      populate(attempt);
      const auto before = LittleFS.files["/quarter.bin"]->bytes;
      failedWriteAfter = write;
      failedWritePrefix = prefix;
      assert(!attempt.stageCompactCopy(tier));
      failedWriteAfter = -1;
      failedWritePrefix = 0;
      assert(before == LittleFS.files["/quarter.bin"]->bytes);
      HistoryStore restarted;
      assert(restarted.begin() && restarted.count(tier) == 130);
      assert(!restarted.readOnly());
      assert(!restarted.stageCompactCopy(tier));
      assert(restarted.resumeCompactCopy(tier));
      assert(before == LittleFS.files["/quarter.bin"]->bytes);
      assert(restarted.resumeCompactCopy(tier));
    }
  }
  for (int write = 0; write < 3; ++write) {
    populate(history);
    const auto before = LittleFS.files["/quarter.bin"]->bytes;
    corruptWriteAfter = write;
    assert(!history.stageCompactCopy(tier)); // Successful write return, bad readback.
    corruptWriteAfter = -1;
    assert(before == LittleFS.files["/quarter.bin"]->bytes);
  }
  populate(history);
  LittleFS.capacityBytes = LittleFS.usedBytes() + 8192 + 3 * kBlockBytes - 1;
  assert(!history.stageCompactCopy(tier));
  assert(!LittleFS.exists(HistoryStore::compactStagePath(tier)));
  LittleFS.capacityBytes = 1310720;

  populate(history);
  assert(history.stageCompactCopy(tier));
  assert(history.importRecord(tier, {epoch + 130 * 900, 15.67f, 1, 20, 123457, 1}));
  const auto advanced = LittleFS.files["/quarter.bin"]->bytes;
  assert(history.resumeCompactCopy(tier));
  assert(advanced == LittleFS.files["/quarter.bin"]->bytes);
  const auto goodStage = LittleFS.files[HistoryStore::compactStagePath(tier)]->bytes;
  // Damaged source must never cause the surviving staging snapshot to be deleted.
  LittleFS.files["/quarter.bin"]->bytes.resize(0);
  assert(!history.resumeCompactCopy(tier));
  assert(goodStage == LittleFS.files[HistoryStore::compactStagePath(tier)]->bytes);
  assert(LittleFS.files["/quarter.bin"]->bytes.empty());

  populate(history, 1);
  assert(history.importRecord(tier, {epoch + 901, 10, 10, 10, 10, 0}));
  assert(!history.stageCompactCopy(tier)); // No rounding a timestamp onto a grid.
  assert(!LittleFS.exists(HistoryStore::compactStagePath(tier)));
  populate(history, 1);
  assert(history.importRecord(tier, {epoch, 10, 10, 10, 10, 0}));
  assert(!history.stageCompactCopy(tier)); // Do not silently lose duplicates.
  assert(!LittleFS.exists(HistoryStore::compactStagePath(tier)));

  // A full wrapped source is read and verified in logical, not physical order.
  LittleFS.files.clear();
  HistoryStore wrapped;
  assert(wrapped.begin());
  for (unsigned i = 0; i < 3000; ++i)
    assert(wrapped.importRecord(HistoryStore::Tier::Minute,
        {epoch + i * 60, 1.23f, -4.56f, 7.89f, float(i), NAN}));
  const auto ring = LittleFS.files["/minute.bin"]->bytes;
  assert(wrapped.stageCompactCopy(HistoryStore::Tier::Minute));
  assert(ring == LittleFS.files["/minute.bin"]->bytes);
  const auto &stage = LittleFS.files[HistoryStore::compactStagePath(HistoryStore::Tier::Minute)]->bytes;
  assert(stage.size() == 45 * kBlockBytes);
  Record first;
  assert(getValidated(stage.data(), 0, first) == ReadResult::Value);
  assert(first.timestamp == epoch + 120 * 60);
  puts("PASS: compact staging, full readback comparison, unchanged sources, wrapped ring, insufficient space, interrupted destination writes");
}
