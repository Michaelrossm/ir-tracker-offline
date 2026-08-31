#include "EventLog.h"

#include <LittleFS.h>

#include <algorithm>
#include <cstring>
#include <ctime>

namespace {
constexpr char kPath[] = "/events.bin";
}

uint32_t EventLog::checksum(const Header &header) const {
  return header.magic ^ header.writeIndex ^ header.count ^ header.generation ^
         0xE71A49C3;
}

bool EventLog::valid(const Header &header) const {
  return header.magic == kMagic && header.writeIndex < kCapacity &&
         header.count <= kCapacity && header.checksum == checksum(header);
}

bool EventLog::validRecord(const Record &record) const {
  return memchr(record.level, '\0', sizeof(record.level)) &&
         memchr(record.code, '\0', sizeof(record.code)) &&
         memchr(record.message, '\0', sizeof(record.message));
}

void EventLog::appendRam(const Record &record) {
  records_[writeIndex_] = record;
  writeIndex_ = (writeIndex_ + 1) % kCapacity;
  count_ = std::min(count_ + 1, kCapacity);
}

bool EventLog::initializeFile() {
  fileHeader_ = {kMagic, 0, 0, 0, 0};
  fileHeader_.checksum = checksum(fileHeader_);
  File file = LittleFS.open(kPath, "w");
  if (!file) return false;
  bool ok = file.write(reinterpret_cast<const uint8_t *>(&fileHeader_),
                       sizeof(Header)) == sizeof(Header);
  ok &= file.write(reinterpret_cast<const uint8_t *>(&fileHeader_),
                   sizeof(Header)) == sizeof(Header);
  file.close();
  return ok;
}

bool EventLog::loadPersistent() {
  File file = LittleFS.open(kPath, "r");
  if (file && file.size() >= 2 * sizeof(Header)) {
    Header copies[2];
    const bool read =
        file.read(reinterpret_cast<uint8_t *>(&copies[0]), sizeof(Header)) ==
            sizeof(Header) &&
        file.read(reinterpret_cast<uint8_t *>(&copies[1]), sizeof(Header)) ==
            sizeof(Header);
    if (read && (valid(copies[0]) || valid(copies[1]))) {
      if (!valid(copies[0]))
        fileHeader_ = copies[1];
      else if (!valid(copies[1]))
        fileHeader_ = copies[0];
      else
        fileHeader_ =
            static_cast<int32_t>(copies[0].generation - copies[1].generation) >=
                    0
                ? copies[0]
                : copies[1];
      const size_t requiredSize =
          2 * sizeof(Header) +
          static_cast<size_t>(fileHeader_.count < kCapacity
                                  ? fileHeader_.count
                                  : kCapacity) *
              sizeof(Record);
      if (file.size() < requiredSize) {
        file.close();
        if (!LittleFS.remove(kPath)) return false;
        return initializeFile();
      }
      const uint32_t first = fileHeader_.count < kCapacity
                                 ? 0
                                 : fileHeader_.writeIndex;
      for (uint32_t i = 0; i < fileHeader_.count; ++i) {
        const uint32_t slot = (first + i) % kCapacity;
        if (!file.seek(2 * sizeof(Header) + slot * sizeof(Record), SeekSet)) {
          file.close();
          return false;
        }
        Record record;
        if (file.read(reinterpret_cast<uint8_t *>(&record), sizeof(record)) !=
            sizeof(record)) {
          file.close();
          return false;
        }
        if (validRecord(record)) appendRam(record);
      }
      file.close();
      return true;
    }
  }
  if (file) file.close();
  return initializeFile();
}

bool EventLog::begin(bool persistent) {
  writeIndex_ = 0;
  count_ = 0;
  persistent_ = persistent;
  if (!persistent_) {
    ready_ = true;
    if (LittleFS.exists(kPath)) LittleFS.remove(kPath);
    return true;
  }
  ready_ = loadPersistent();
  return ready_;
}

bool EventLog::setPersistence(bool persistent) {
  if (!ready_) return false;
  if (persistent == persistent_) return true;
  if (!persistent) {
    persistent_ = false;
    if (LittleFS.exists(kPath)) LittleFS.remove(kPath);
    return true;
  }
  persistent_ = true;
  if (LittleFS.exists(kPath) && !LittleFS.remove(kPath)) {
    persistent_ = false;
    return false;
  }
  if (!initializeFile()) {
    persistent_ = false;
    return false;
  }
  return true;
}

bool EventLog::add(const char *level, const char *code, const String &message) {
  if (!ready_) return false;
  Record record = {};
  record.timestamp = time(nullptr);
  record.uptimeSeconds = millis() / 1000;
  strlcpy(record.level, level, sizeof(record.level));
  strlcpy(record.code, code, sizeof(record.code));
  strlcpy(record.message, message.c_str(), sizeof(record.message));
  appendRam(record);
  return !persistent_ || writePersistent(record);
}

bool EventLog::writePersistent(const Record &record) {
  File file = LittleFS.open(kPath, "r+");
  if (!file) return false;
  const size_t offset =
      2 * sizeof(Header) + fileHeader_.writeIndex * sizeof(Record);
  if (!file.seek(offset, SeekSet) ||
      file.write(reinterpret_cast<const uint8_t *>(&record), sizeof(record)) !=
          sizeof(record)) {
    file.close();
    return false;
  }
  fileHeader_.writeIndex = (fileHeader_.writeIndex + 1) % kCapacity;
  fileHeader_.count = std::min(fileHeader_.count + 1, kCapacity);
  ++fileHeader_.generation;
  fileHeader_.checksum = checksum(fileHeader_);
  const size_t headerOffset =
      (fileHeader_.generation & 1U) ? sizeof(Header) : 0;
  const bool ok =
      file.seek(headerOffset, SeekSet) &&
      file.write(reinterpret_cast<const uint8_t *>(&fileHeader_),
                 sizeof(Header)) == sizeof(Header);
  file.close();
  return ok;
}

bool EventLog::forEach(const Callback &callback) {
  if (!ready_) return false;
  const uint32_t first = count_ < kCapacity ? 0 : writeIndex_;
  for (uint32_t i = 0; i < count_; ++i) {
    const uint32_t slot = (first + i) % kCapacity;
    const Record &record = records_[slot];
    if (!validRecord(record)) {
      delay(0);
      continue;
    }
    if (!callback(record)) break;
    delay(0);
  }
  return true;
}

bool EventLog::clear() {
  writeIndex_ = 0;
  count_ = 0;
  if (LittleFS.exists(kPath) && !LittleFS.remove(kPath)) return false;
  return !persistent_ || initializeFile();
}
