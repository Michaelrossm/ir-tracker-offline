#include <cassert>
#include <cstdio>
#include <array>
#include "../../src/app/storage/HistoryStore.cpp"

using namespace HistoryBlockCodec;
using Record = HistoryStore::Record;
constexpr auto quarter = HistoryStore::Tier::QuarterHour;
constexpr uint32_t first = 1700000100;

void prepare(unsigned blocks = 3) {
  LittleFS.files.clear();
  HistoryStore legacy;
  assert(legacy.begin());
  auto &bytes = LittleFS.files["/quarter.bin"]->bytes;
  bytes.clear();
  std::array<uint8_t, kBlockBytes> buffer;
  for (unsigned i = 0; i < blocks; ++i) {
    const uint32_t start = first + i * kSlots * 900;
    assert(begin(buffer.data(), buffer.size(), start, 900));
    for (unsigned slot : {0U, 2U, 31U, 63U}) {
      Record value{start + slot * 900, 12.34f, -1.23f, 90.12f, 15678.25f, NAN};
      assert(put(buffer.data(), buffer.size(), value));
    }
    assert(seal(buffer.data(), buffer.size()));
    bytes.insert(bytes.end(), buffer.begin(), buffer.end());
  }
}

int main() {
  prepare();
  HistoryStore reader;
  assert(reader.begin() && reader.ready() && !reader.readOnly());
  assert(reader.count(quarter) == 12);
  const auto original = LittleFS.files["/quarter.bin"]->bytes;
  for (const auto range : {std::pair<uint32_t, uint32_t>{0, UINT32_MAX},
       {first + 900, first + 3 * 900}, {first - 900, first - 1},
       {first + 3 * kSlots * 900, UINT32_MAX}, {first + 1, first},
       {first + 63 * 900, first + 65 * 900}}) {
    unsigned count = 0, expected = 0;
    for (unsigned b = 0; b < 3; ++b)
      for (unsigned s : {0U, 2U, 31U, 63U}) {
        const uint32_t ts = first + (b * kSlots + s) * 900;
        if (ts >= range.first && ts <= range.second) ++expected;
      }
    assert(reader.forEach(quarter, range.first, range.second, [&](const Record &r) {
      assert(r.timestamp >= range.first && r.timestamp <= range.second);
      assert(r.averageW == 12.3f && r.minimumW == -1.2f && r.maximumW == 90.1f);
      assert(r.importKwh == 15678.25f && std::isnan(r.exportKwh));
      ++count;
      return true;
    }));
    assert(count == expected);
  }
  unsigned stopped = 0;
  assert(reader.forEach(quarter, 0, UINT32_MAX, [&](const Record &) {
    ++stopped; return false;
  }));
  assert(stopped == 1);
  // Reads alone must not mutate the archive. Mutable compact writes and their
  // interruption recovery are exercised by history_compact_writer.cpp.
  assert(reader.flushPending(quarter));
  assert(LittleFS.files["/quarter.bin"]->bytes == original);

  // A truncated or corrupt archive must not become an empty/new history.
  for (size_t length = 0; length < kBlockBytes; ++length) {
    prepare(1);
    auto &bytes = LittleFS.files["/quarter.bin"]->bytes;
    bytes.resize(length);
    const auto damaged = bytes;
    HistoryStore invalid;
    assert(!invalid.begin());
    assert(bytes == damaged);
  }
  for (const unsigned mutation : {0U, 1U, 2U, 3U}) {
    prepare();
    auto &bytes = LittleFS.files["/quarter.bin"]->bytes;
    auto *second = bytes.data() + kBlockBytes;
    if (mutation == 0) second[kHeaderBytes] ^= 1;
    if (mutation == 1) { store32(second + 4, first); assert(seal(second, kBlockBytes)); }
    if (mutation == 2) { store32(second + 8, 1800); assert(seal(second, kBlockBytes)); }
    if (mutation == 3) {
      second[kHeaderBytes + kValueBytes] = 1; // Invalid absent sample, valid CRC.
      assert(seal(second, kBlockBytes));
    }
    const auto damaged = bytes;
    HistoryStore invalid;
    assert(!invalid.begin());
    assert(bytes == damaged);
  }
  LittleFS.files.clear();
  HistoryStore legacy;
  assert(legacy.begin() && !legacy.readOnly());
  assert(legacy.importRecord(quarter, {first, 12.34f, 0, 15, 5, 0}));
  assert(legacy.count(quarter) == 1);
  puts("PASS: real HistoryStore dual reader, ranges, gaps, corruption, truncation, read-only access, IRH1 regression");
}
