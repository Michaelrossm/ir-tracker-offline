#pragma once

#include <Arduino.h>

#include <functional>

class EventLog {
 public:
  struct __attribute__((packed)) Record {
    uint32_t timestamp;
    uint32_t uptimeSeconds;
    char level[8];
    char code[24];
    char message[88];
  };

  using Callback = std::function<bool(const Record &)>;

  bool begin(bool persistent);
  bool setPersistence(bool persistent);
  bool add(const char *level, const char *code, const String &message);
  bool forEach(const Callback &callback);
  bool clear();
  size_t count() const { return count_; }
  bool persistent() const { return persistent_; }

 private:
  struct __attribute__((packed)) Header {
    uint32_t magic;
    uint32_t writeIndex;
    uint32_t count;
    uint32_t generation;
    uint32_t checksum;
  };

  static constexpr uint32_t kMagic = 0x49524531;  // IRE1
  static constexpr uint32_t kCapacity = 256;
  Header fileHeader_ = {};
  Record records_[kCapacity] = {};
  uint32_t writeIndex_ = 0;
  uint32_t count_ = 0;
  bool ready_ = false;
  bool persistent_ = false;

  uint32_t checksum(const Header &header) const;
  bool valid(const Header &header) const;
  bool validRecord(const Record &record) const;
  void appendRam(const Record &record);
  bool initializeFile();
  bool loadPersistent();
  bool writePersistent(const Record &record);
};
