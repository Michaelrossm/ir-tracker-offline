#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#ifdef _MSC_VER
#define __attribute__(value)
#endif
constexpr int SeekSet = 0;
inline int failedWriteAfter = -1;
inline size_t failedWritePrefix = 0;
inline int corruptWriteAfter = -1;
struct TestPowerCut {};
inline int fsMutationCut = -1;
inline int fsMutationCount = 0;
// SIZE_MAX models a completed operation whose acknowledgement was lost.
inline size_t fsMutationPrefix = SIZE_MAX;
inline bool fsMutationStart() { return fsMutationCount++ == fsMutationCut; }
inline size_t (*fsAvailableBytes)() = nullptr;
struct TestFileData { std::vector<uint8_t> bytes; };
struct File {
  std::shared_ptr<TestFileData> data;
  size_t position = 0;
  explicit operator bool() const { return bool(data); }
  size_t size() const { return data->bytes.size(); }
  bool seek(size_t offset, int) { position = offset; return bool(data); }
  size_t read(uint8_t *buffer, size_t count) {
    if (!data || position > size()) return 0;
    count = std::min(count, size() - position);
    memcpy(buffer, data->bytes.data() + position, count);
    position += count;
    return count;
  }
  size_t write(const uint8_t *buffer, size_t count) {
    const bool cut = fsMutationStart();
    if (cut) count = std::min(count, fsMutationPrefix);
    const bool failing = failedWriteAfter == 0;
    if (failing) count = std::min(count, failedWritePrefix);
    if (failedWriteAfter > 0) --failedWriteAfter;
    if (fsAvailableBytes && position + count > size()) {
      const size_t available = fsAvailableBytes();
      const size_t maximum = size() + available;
      if (position + count > maximum) count = maximum > position ? maximum - position : 0;
    }
    if (position + count > size()) data->bytes.resize(position + count);
    memcpy(data->bytes.data() + position, buffer, count);
    if (corruptWriteAfter == 0 && count) data->bytes[position + count / 2] ^= 1;
    if (corruptWriteAfter > 0) --corruptWriteAfter;
    position += count;
    if (cut) throw TestPowerCut{};
    return count;
  }
  void close() { data.reset(); }
  bool truncate(size_t length) {
    if (!data || length > size()) return false;
    const bool cut = fsMutationStart();
    if (cut && !fsMutationPrefix) throw TestPowerCut{};
    data->bytes.resize(length);
    if (position > length) position = length;
    if (cut) throw TestPowerCut{};
    return true;
  }
};
