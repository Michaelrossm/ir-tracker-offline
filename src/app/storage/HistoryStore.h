#pragma once

#include <Arduino.h>
#include <FS.h>

#include <functional>

class HistoryStore {
 public:
  struct __attribute__((packed)) Record {
    uint32_t timestamp;
    float averageW;
    float minimumW;
    float maximumW;
    float importKwh;
    float exportKwh;
  };

  enum class Tier : uint8_t { Minute, QuarterHour, Hour, Day };
  using RecordCallback = std::function<bool(const Record &)>;

  bool begin();
  void update(uint32_t epoch, double powerW, double importKwh, double exportKwh);
  bool forEach(Tier tier, uint32_t since, uint32_t until,
               const RecordCallback &callback);
  bool clear(Tier tier);
  bool flushPending(Tier tier);
  bool importRecord(Tier tier, const Record &record);
  size_t count(Tier tier) const;
  size_t usedBytes() const;
  size_t totalBytes() const;
  bool ready() const { return mounted_; }

 private:
  struct __attribute__((packed)) Header {
    uint32_t magic;
    uint32_t capacity;
    uint32_t writeIndex;
    uint32_t count;
    uint32_t generation;
    uint32_t checksum;
  };

  struct Aggregate {
    uint32_t bucket = 0;
    double sum = 0;
    float minimum = NAN;
    float maximum = NAN;
    float importKwh = NAN;
    float exportKwh = NAN;
    uint32_t samples = 0;
  };

  struct TierState {
    const char *path;
    uint32_t seconds;
    uint32_t capacity;
    Header header;
    Aggregate aggregate;
    uint32_t lastWrittenBucket;
  };

  static constexpr uint32_t kMagic = 0x49524831;  // IRH1
  // DE: Großzügige Grenzen für größere Anlagen; defekte Gleitkommawerte dürfen
  // nie Diagramme/Statistiken erreichen. | EN: Generous limits for larger
  // installations; corrupt floating-point payloads must never reach charts or statistics.
  static constexpr float kMaximumPlausiblePowerW = 100000.0f;
  static constexpr float kMaximumPlausibleEnergyKwh = 1000000000.0f;
  TierState tiers_[4] = {
      {"/minute.bin", 60, 2880, {}, {}, 0},
      {"/quarter.bin", 900, 17280, {}, {}, 0},
      {"/hour.bin", 3600, 17520, {}, {}, 0},
      {"/day.bin", 86400, 7300, {}, {}, 0}};
  bool mounted_ = false;

  TierState &state(Tier tier) { return tiers_[static_cast<uint8_t>(tier)]; }
  const TierState &state(Tier tier) const {
    return tiers_[static_cast<uint8_t>(tier)];
  }
  bool loadHeader(TierState &tier);
  bool validHeader(const Header &header, uint32_t capacity) const;
  bool validRecord(const Record &record) const;
  uint32_t headerChecksum(const Header &header) const;
  bool writeRecord(TierState &tier, const Record &record);
  void updateTier(TierState &tier, uint32_t epoch, double powerW,
                  double importKwh, double exportKwh);
};
