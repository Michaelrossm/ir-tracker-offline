#include "../../src/app/storage/HistoryStore.h"
#include "../../src/app/storage/HistoryBlockCodec.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <random>

using namespace HistoryBlockCodec;
using Record = HistoryStore::Record;

int main() {
  static_assert(kBlockBytes == 1056, "Capacity model depends on this size");
  static_assert(sizeof(Record) == 24, "External record format must not change");
  std::array<uint8_t, kBlockBytes> block;
  constexpr uint32_t start = 1700000040;
  std::mt19937 rng(91283);
  for (const uint32_t step : {60U, 300U, 900U, 1800U, 3600U, 86400U}) {
    for (unsigned run = 0; run < 100; ++run) {
      assert(begin(block.data(), block.size(), start, step));
      std::array<Record, kSlots> input = {};
      for (size_t slot = 0; slot < kSlots; ++slot) {
        if (slot % 7 == 0) continue;
        float power = float(int32_t(rng() % 20000001) - 10000000) / 100.0f;
        input[slot] = {start + uint32_t(slot) * step, power,
                      std::max(-100000.0f, power - 2.13f),
                      std::min(100000.0f, power + 3.12f),
                      float(rng() % 1000000000), NAN};
        assert(put(block.data(), block.size(), input[slot]));
      }
      assert(seal(block.data(), block.size()));
      assert(valid(block.data(), block.size()));
      for (size_t slot = 0; slot < kSlots; ++slot) {
        Record out = {};
        auto result = getValidated(block.data(), slot, out);
        if (slot % 7 == 0) { assert(result == ReadResult::Missing); continue; }
        assert(result == ReadResult::Value);
        assert(out.timestamp == input[slot].timestamp);
        // binary32 representation at 100 kW adds at most one float ULP.
        assert(std::fabs(out.averageW - input[slot].averageW) <= 0.054f);
        assert(std::fabs(out.minimumW - input[slot].minimumW) <= 0.054f);
        assert(std::fabs(out.maximumW - input[slot].maximumW) <= 0.054f);
        assert(memcmp(&out.importKwh, &input[slot].importKwh, 4) == 0);
        assert(memcmp(&out.exportKwh, &input[slot].exportKwh, 4) == 0);
      }
    }
  }
  assert(begin(block.data(), block.size(), start, 900));
  Record sample = {start, 0, -100000, 100000, 0, NAN};
  assert(put(block.data(), block.size(), sample));
  assert(seal(block.data(), block.size()));
  const auto good = block;
  for (size_t i = 0; i < block.size(); ++i) {
    block = good; block[i] ^= 1;
    assert(!valid(block.data(), block.size()));
    assert(!valid(good.data(), i)); // Every truncation is rejected.
  }
  for (size_t cut = 0; cut < block.size(); ++cut) {
    block.fill(0xff);
    memcpy(block.data(), good.data(), cut);
    assert(!valid(block.data(), block.size()));
  }
  block = good;
  for (float value : {NAN, INFINITY, -INFINITY, 100001.0f, -100001.0f}) {
    auto bad = sample; bad.averageW = value;
    assert(!put(block.data(), block.size(), bad));
    assert(block == good);
  }
  auto bad = sample; bad.timestamp++;
  assert(!put(block.data(), block.size(), bad)); // No time rounding.
  bad = sample; bad.importKwh = -1;
  assert(!put(block.data(), block.size(), bad));
  bad = sample; bad.minimumW = 1;
  assert(!put(block.data(), block.size(), bad));
  assert(block == good);
  Record out = {};
  assert(getValidated(block.data(), 0, out) == ReadResult::Value);
  assert(out.averageW == 0 && out.minimumW == -100000 && out.maximumW == 100000);
  assert(getValidated(block.data(), 1, out) == ReadResult::Missing);
  assert(!begin(block.data(), block.size(), UINT32_MAX - 1, 60));
  assert(!begin(block.data(), block.size(), start, 0));
  assert(!begin(block.data(), block.size(), start, 7));
  puts("PASS: history codec, 38400 slots, bitwise energy preservation, corrupt/truncated/torn blocks");
}
