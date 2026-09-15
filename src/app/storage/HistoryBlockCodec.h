#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// HistoryStore can read this format but does not create or migrate it yet.
// Age-tier routing and a transactional writer are required before activation.
// The public HistoryStore::Record remains the decoded representation.
namespace HistoryBlockCodec {
constexpr size_t kSlots = 64;
constexpr size_t kHeaderBytes = 32;
constexpr size_t kValueBytes = 16;
constexpr size_t kBlockBytes = kHeaderBytes + kSlots * kValueBytes;
constexpr uint32_t kMagic = 0x32485249;  // IRH2, little endian.
enum class ReadResult : uint8_t { Invalid, Missing, Value };

inline uint32_t load32(const uint8_t *p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 |
         uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
inline void store32(uint8_t *p, uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) p[i] = uint8_t(value >> (8 * i));
}
inline bool supportedStep(uint32_t step) {
  return step == 60 || step == 300 || step == 900 || step == 1800 ||
         step == 3600 || step == 86400;
}
inline bool validTime(uint32_t start, uint32_t step) {
  return start >= 1700000000UL && supportedStep(step) &&
         uint64_t(start) + (kSlots - 1) * uint64_t(step) <= UINT32_MAX;
}
inline uint32_t checksum(const uint8_t *block) {
  uint32_t crc = UINT32_MAX;
  for (size_t i = 0; i < kBlockBytes; ++i) {
    // Checksum field is treated as zero, protecting all other header bytes.
    crc ^= i >= 12 && i < 16 ? 0 : block[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
  }
  return ~crc;
}
inline bool begin(uint8_t *block, size_t size, uint32_t start, uint32_t step) {
  if (!block || size != kBlockBytes || !validTime(start, step)) return false;
  memset(block, 0, size);
  store32(block, kMagic);
  store32(block + 4, start);
  store32(block + 8, step);
  return true;
}
inline bool validPower(float value) {
  return std::isfinite(value) && std::fabs(value) <= 100000.0f;
}
inline bool validEnergy(float value) {
  return std::isnan(value) ||
         (std::isfinite(value) && value >= 0 && value <= 1000000000.0f);
}
inline uint32_t powerBits(float value) {
  return uint32_t(int32_t(std::round(double(value) * 10.0))) & 0x1fffffU;
}
inline float powerValue(uint32_t bits) {
  const int32_t value = (bits & 0x100000U)
      ? int32_t(bits) - 0x200000 : int32_t(bits);
  return float(double(value) / 10.0);
}

template <typename Record>
bool put(uint8_t *block, size_t size, const Record &record) {
  static_assert(sizeof(record.importKwh) == 4 && sizeof(record.exportKwh) == 4,
                "Energy counters must retain their original binary32 bits");
  if (!block || size != kBlockBytes || load32(block) != kMagic) return false;
  const uint32_t start = load32(block + 4), step = load32(block + 8);
  if (!validTime(start, step) || record.timestamp < start) return false;
  const uint32_t delta = record.timestamp - start;
  if (delta % step || delta / step >= kSlots ||
      !validPower(record.averageW) || !validPower(record.minimumW) ||
      !validPower(record.maximumW) || record.minimumW > record.averageW ||
      record.averageW > record.maximumW || !validEnergy(record.importKwh) ||
      !validEnergy(record.exportKwh)) return false;
  uint8_t *out = block + kHeaderBytes + delta / step * kValueBytes;
  // Bit 63 distinguishes a real zero-valued sample from an absent sample.
  const uint64_t powers = uint64_t(powerBits(record.averageW)) |
      uint64_t(powerBits(record.minimumW)) << 21 |
      uint64_t(powerBits(record.maximumW)) << 42 | (uint64_t(1) << 63);
  for (unsigned i = 0; i < 8; ++i) out[i] = uint8_t(powers >> (8 * i));
  uint32_t bits;
  memcpy(&bits, &record.importKwh, 4); store32(out + 8, bits);
  memcpy(&bits, &record.exportKwh, 4); store32(out + 12, bits);
  return true;
}
inline bool seal(uint8_t *block, size_t size) {
  if (!block || size != kBlockBytes || load32(block) != kMagic ||
      !validTime(load32(block + 4), load32(block + 8))) return false;
  store32(block + 12, checksum(block));
  return true;
}
inline bool valid(const uint8_t *block, size_t size) {
  if (!block || size != kBlockBytes || load32(block) != kMagic ||
      !validTime(load32(block + 4), load32(block + 8))) return false;
  for (size_t i = 16; i < kHeaderBytes; ++i)
    if (block[i]) return false;
  return load32(block + 12) == checksum(block);
}

// Validate a complete block once before iterating it. This avoids a CRC pass
// for every sample. Never call this helper on an unvalidated disk buffer.
template <typename Record>
ReadResult getValidated(const uint8_t *block, size_t slot, Record &record) {
  if (!block || slot >= kSlots) return ReadResult::Invalid;
  const uint8_t *in = block + kHeaderBytes + slot * kValueBytes;
  uint64_t powers = 0;
  for (unsigned i = 0; i < 8; ++i) powers |= uint64_t(in[i]) << (8 * i);
  if (!(powers >> 63)) {
    for (size_t i = 0; i < kValueBytes; ++i)
      if (in[i]) return ReadResult::Invalid;
    return ReadResult::Missing;
  }
  Record decoded = {};
  decoded.timestamp = load32(block + 4) + uint32_t(slot) * load32(block + 8);
  decoded.averageW = powerValue(uint32_t(powers) & 0x1fffffU);
  decoded.minimumW = powerValue(uint32_t(powers >> 21) & 0x1fffffU);
  decoded.maximumW = powerValue(uint32_t(powers >> 42) & 0x1fffffU);
  uint32_t bits = load32(in + 8); memcpy(&decoded.importKwh, &bits, 4);
  bits = load32(in + 12); memcpy(&decoded.exportKwh, &bits, 4);
  if (!validPower(decoded.averageW) || !validPower(decoded.minimumW) ||
      !validPower(decoded.maximumW) || decoded.minimumW > decoded.averageW ||
      decoded.averageW > decoded.maximumW || !validEnergy(decoded.importKwh) ||
      !validEnergy(decoded.exportKwh)) return ReadResult::Invalid;
  record = decoded;
  return ReadResult::Value;
}
}  // namespace HistoryBlockCodec
