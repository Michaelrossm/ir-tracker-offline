#pragma once
#include "FS.h"
struct TestLittleFS {
  std::map<std::string, std::shared_ptr<TestFileData>> files;
  bool mountFails = false;
  unsigned formatAttempts = 0;
  size_t capacityBytes = 1310720;
  bool enforceCapacity = false;
  bool begin(bool format, const char *, int, const char *) {
    if (format) ++formatAttempts;
    return !mountFails || format;
  }
  File open(const char *path, const char *mode) {
    if (*mode == 'w') {
      const bool cut = fsMutationStart();
      if (!cut || fsMutationPrefix) files[path] = std::make_shared<TestFileData>();
      if (cut) throw TestPowerCut{};
    }
    const auto found = files.find(path);
    return {found == files.end() ? nullptr : found->second};
  }
  bool exists(const char *path) const { return files.count(path) != 0; }
  bool truncate(const char *path, size_t length) {
    File file = open(path, "r+");
    return file && file.truncate(length);
  }
  bool rename(const char *from, const char *to) {
    const auto source = files.find(from);
    if (source == files.end()) return false;
    const bool cut = fsMutationStart();
    if (cut && !fsMutationPrefix) throw TestPowerCut{};
    const auto data = source->second;
    files[to] = data;
    files.erase(from);
    if (cut) throw TestPowerCut{};
    return true;
  }
  bool remove(const char *path) {
    const bool cut = fsMutationStart();
    if (cut && !fsMutationPrefix) throw TestPowerCut{};
    const bool result = files.erase(path) != 0;
    if (cut) throw TestPowerCut{};
    return result;
  }
  size_t usedBytes() const {
    size_t total = 0;
    for (const auto &file : files) total += file.second->bytes.size();
    return total;
  }
  size_t totalBytes() const { return capacityBytes; }
};
inline TestLittleFS LittleFS;
inline size_t testAvailableBytes() {
  if (!LittleFS.enforceCapacity) return SIZE_MAX / 2;
  const size_t used = LittleFS.usedBytes();
  return used < LittleFS.capacityBytes ? LittleFS.capacityBytes - used : 0;
}
inline bool testCapacityHookInstalled = (fsAvailableBytes = testAvailableBytes, true);
