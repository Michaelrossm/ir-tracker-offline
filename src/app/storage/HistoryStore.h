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

  enum class Tier : uint8_t { Minute, QuarterHour, Hour, Day, HalfHour, FiveMinute };
  using RecordCallback = std::function<bool(const Record &)>;
  using RetainedCallback = std::function<bool(const Record &, uint32_t)>;
  bool forEachRetained(uint32_t since, uint32_t until, const RetainedCallback &callback);
  bool compactActive() const;
  bool migrateCompact();

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
  // Transitional IRH2 reader: never apply the IRH1 writer to compact data.
  bool readOnly() const;
  // Prepare and verify a separate snapshot. Never activates it or removes IRH1.
  // Existing stage files (including interrupted attempts) are never overwritten.
  bool stageCompactCopy(Tier tier);
  // Resume preparation after reboot. Does not activate or delete the source.
  bool resumeCompactCopy(Tier tier);
  static const char *compactStagePath(Tier tier);
  void setServiceHook(void (*hook)()) { serviceHook_ = hook; }

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
    bool orderKnown;
    bool ordered;
  };

  static constexpr uint32_t kMagic = 0x49524831;  // IRH1
  // DE: Großzügige Grenzen für größere Anlagen; defekte Gleitkommawerte dürfen
  // nie Diagramme/Statistiken erreichen. | EN: Generous limits for larger
  // installations; corrupt floating-point payloads must never reach charts or statistics.
  static constexpr float kMaximumPlausiblePowerW = 100000.0f;
  static constexpr float kMaximumPlausibleEnergyKwh = 1000000000.0f;
  TierState tiers_[6] = {
      {"/minute.bin", 60, 2880, {}, {}, 0, false, false},
      {"/quarter.bin", 900, 17280, {}, {}, 0, false, false},
      {"/hour.bin", 3600, 17520, {}, {}, 0, false, false},
      {"/day.bin", 86400, 7300, {}, {}, 0, false, false},
      {"/half.bin", 1800, 17568, {}, {}, 0, false, false},
      {"/five.bin", 300, 288, {}, {}, 0, false, false}};
  bool mounted_ = false;
  void (*serviceHook_)() = nullptr;

  TierState &state(Tier tier) { return tiers_[static_cast<uint8_t>(tier)]; }
  const TierState &state(Tier tier) const {
    return tiers_[static_cast<uint8_t>(tier)];
  }
  bool loadHeader(TierState &tier);
  bool promoteRetained(Tier source, Tier target, uint32_t cutoff);
  bool recoverCompactWrite();
  bool recoverCompactShrink();
  bool pruneCompactHour(uint32_t cutoff);
  bool pruneCompactMinute(uint32_t cutoff);
  bool writeCompact(TierState &tier, const Record &record);
  bool commitCompactBlock(TierState &tier, uint32_t block, const uint8_t *data);
  bool verifyCompactCopy(Tier tier);
  bool forEachMigrationRecord(Tier tier, const RecordCallback &callback);
  bool forEachCompact(TierState &tier, uint32_t since, uint32_t until,
                      const RecordCallback &callback);
  bool validHeader(const Header &header, uint32_t capacity) const;
  bool validRecord(const Record &record) const;
  uint32_t headerChecksum(const Header &header) const;
  bool writeRecord(TierState &tier, const Record &record);
  void updateTier(TierState &tier, uint32_t epoch, double powerW,
                  double importKwh, double exportKwh);
};
