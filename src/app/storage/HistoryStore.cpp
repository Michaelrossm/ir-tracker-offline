#include "HistoryStore.h"

#include <LittleFS.h>

#include <algorithm>
#include <cmath>

bool HistoryStore::begin() {
  mounted_ = LittleFS.begin(true, "/history", 10, "history");
  if (!mounted_) return false;
  for (auto &tier : tiers_) {
    if (!loadHeader(tier)) {
      mounted_ = false;
      return false;
    }
  }
  return true;
}

bool HistoryStore::loadHeader(TierState &tier) {
  File file = LittleFS.open(tier.path, "r");
  if (file && file.size() >= 2 * sizeof(Header)) {
    Header copies[2];
    const bool read =
        file.read(reinterpret_cast<uint8_t *>(&copies[0]), sizeof(Header)) ==
            sizeof(Header) &&
        file.read(reinterpret_cast<uint8_t *>(&copies[1]), sizeof(Header)) ==
            sizeof(Header);
    if (read) {
      const bool firstValid = validHeader(copies[0], tier.capacity);
      const bool secondValid = validHeader(copies[1], tier.capacity);
      if (firstValid || secondValid) {
        tier.header =
            !firstValid ? copies[1]
                        : (!secondValid ||
                                   static_cast<int32_t>(copies[0].generation -
                                                        copies[1].generation) >= 0
                               ? copies[0]
                               : copies[1]);
        const size_t requiredSize =
            2 * sizeof(Header) +
            static_cast<size_t>(tier.header.count < tier.capacity
                                    ? tier.header.count
                                    : tier.capacity) *
                sizeof(Record);
        if (file.size() < requiredSize) {
          file.close();
          if (!LittleFS.remove(tier.path)) return false;
          return loadHeader(tier);
        }
        tier.lastWrittenBucket = 0;
        if (tier.header.count) {
          const uint32_t lastSlot =
              (tier.header.writeIndex + tier.capacity - 1) % tier.capacity;
          Record lastRecord;
          if (file.seek(2 * sizeof(Header) +
                            static_cast<size_t>(lastSlot) * sizeof(Record),
                        SeekSet) &&
              file.read(reinterpret_cast<uint8_t *>(&lastRecord),
                        sizeof(lastRecord)) == sizeof(lastRecord)) {
            if (validRecord(lastRecord))
              tier.lastWrittenBucket = lastRecord.timestamp;
          }
        }
        file.close();
        return true;
      }
    }
  }
  if (file) file.close();
  tier.header = {kMagic, tier.capacity, 0, 0, 0, 0};
  tier.lastWrittenBucket = 0;
  tier.header.checksum = headerChecksum(tier.header);
  file = LittleFS.open(tier.path, "w");
  if (!file) return false;
  bool ok =
      file.write(reinterpret_cast<const uint8_t *>(&tier.header),
                 sizeof(Header)) == sizeof(Header);
  ok &= file.write(reinterpret_cast<const uint8_t *>(&tier.header),
                   sizeof(Header)) == sizeof(Header);
  file.close();
  return ok;
}

uint32_t HistoryStore::headerChecksum(const Header &header) const {
  return header.magic ^ header.capacity ^ header.writeIndex ^ header.count ^
         header.generation ^ 0xA57C31E9;
}

bool HistoryStore::validHeader(const Header &header, uint32_t capacity) const {
  return header.magic == kMagic && header.capacity == capacity &&
         header.writeIndex < capacity && header.count <= capacity &&
         header.checksum == headerChecksum(header);
}

bool HistoryStore::validRecord(const Record &record) const {
  const auto validPower = [](float value) {
    return std::isfinite(value) &&
           std::abs(value) <= kMaximumPlausiblePowerW;
  };
  const auto validEnergy = [](float value) {
    return std::isnan(value) ||
           (std::isfinite(value) && value >= 0.0f &&
            value <= kMaximumPlausibleEnergyKwh);
  };
  return record.timestamp >= 1700000000UL &&
         validPower(record.averageW) && validPower(record.minimumW) &&
         validPower(record.maximumW) &&
         record.minimumW <= record.maximumW &&
         record.averageW >= record.minimumW &&
         record.averageW <= record.maximumW &&
         validEnergy(record.importKwh) && validEnergy(record.exportKwh);
}

bool HistoryStore::writeRecord(TierState &tier, const Record &record) {
  if (!validRecord(record)) return false;
  File file = LittleFS.open(tier.path, "r+");
  if (!file) return false;
  const size_t offset =
      2 * sizeof(Header) +
      static_cast<size_t>(tier.header.writeIndex) * sizeof(Record);
  if (!file.seek(offset, SeekSet) ||
      file.write(reinterpret_cast<const uint8_t *>(&record), sizeof(Record)) !=
          sizeof(Record)) {
    file.close();
    return false;
  }
  tier.header.writeIndex = (tier.header.writeIndex + 1) % tier.capacity;
  tier.header.count = std::min(tier.header.count + 1, tier.capacity);
  ++tier.header.generation;
  tier.header.checksum = headerChecksum(tier.header);
  const size_t headerOffset =
      (tier.header.generation & 1U) ? sizeof(Header) : 0;
  if (!file.seek(headerOffset, SeekSet) ||
      file.write(reinterpret_cast<const uint8_t *>(&tier.header),
                 sizeof(Header)) != sizeof(Header)) {
    file.close();
    return false;
  }
  file.close();
  tier.lastWrittenBucket = record.timestamp;
  return true;
}

void HistoryStore::updateTier(TierState &tier, uint32_t epoch, double powerW,
                              double importKwh, double exportKwh) {
  const uint32_t bucket = epoch - epoch % tier.seconds;
  Aggregate &aggregate = tier.aggregate;
  if (!aggregate.bucket && tier.lastWrittenBucket == bucket) return;
  if (aggregate.bucket && bucket != aggregate.bucket && aggregate.samples) {
    Record record = {
        aggregate.bucket,
        static_cast<float>(aggregate.sum / aggregate.samples),
        aggregate.minimum,
        aggregate.maximum,
        static_cast<float>(aggregate.importKwh),
        static_cast<float>(aggregate.exportKwh)};
    writeRecord(tier, record);
    aggregate = {};
  }
  if (!aggregate.bucket) aggregate.bucket = bucket;
  if (std::isfinite(powerW)) {
    aggregate.sum += powerW;
    aggregate.minimum =
        std::isfinite(aggregate.minimum)
            ? std::min(aggregate.minimum, static_cast<float>(powerW))
            : static_cast<float>(powerW);
    aggregate.maximum =
        std::isfinite(aggregate.maximum)
            ? std::max(aggregate.maximum, static_cast<float>(powerW))
            : static_cast<float>(powerW);
    ++aggregate.samples;
  }
  if (std::isfinite(importKwh)) aggregate.importKwh = importKwh;
  if (std::isfinite(exportKwh)) aggregate.exportKwh = exportKwh;
}

void HistoryStore::update(uint32_t epoch, double powerW, double importKwh,
                          double exportKwh) {
  if (!mounted_ || epoch < 1700000000UL) return;
  for (auto &tier : tiers_)
    updateTier(tier, epoch, powerW, importKwh, exportKwh);
}

bool HistoryStore::forEach(Tier tierId, uint32_t since, uint32_t until,
                           const RecordCallback &callback) {
  if (!mounted_) return false;
  TierState &tier = state(tierId);
  File file = LittleFS.open(tier.path, "r");
  if (!file) return false;
  const uint32_t first =
      tier.header.count < tier.capacity ? 0 : tier.header.writeIndex;
  bool callbackStopped = false;
  const auto readRange = [&](uint32_t startSlot, uint32_t recordCount) {
    if (!recordCount) return true;
    const size_t offset =
        2 * sizeof(Header) +
        static_cast<size_t>(startSlot) * sizeof(Record);
    if (!file.seek(offset, SeekSet)) return false;
    for (uint32_t i = 0; i < recordCount; ++i) {
      Record record;
      if (file.read(reinterpret_cast<uint8_t *>(&record), sizeof(record)) !=
          sizeof(record))
        return false;
      if (!validRecord(record)) {
        delay(0);
        continue;
      }
      if (record.timestamp >= since && record.timestamp <= until &&
          !callback(record)) {
        callbackStopped = true;
        return true;
      }
      delay(0);
    }
    return true;
  };

  // A ring is physically stored in at most two contiguous regions. Preserve
  // chronological callback order while reducing count seeks to at most two.
  const uint32_t firstRange =
      std::min(tier.header.count, tier.capacity - first);
  bool ok = readRange(first, firstRange);
  if (ok && !callbackStopped && firstRange < tier.header.count)
    ok = readRange(0, tier.header.count - firstRange);
  file.close();
  return ok;
}

bool HistoryStore::clear(Tier tierId) {
  if (!mounted_) return false;
  TierState &tier = state(tierId);
  if (LittleFS.exists(tier.path) && !LittleFS.remove(tier.path)) return false;
  tier.aggregate = {};
  return loadHeader(tier);
}

bool HistoryStore::flushPending(Tier tierId) {
  if (!mounted_) return false;
  TierState &tier = state(tierId);
  Aggregate &aggregate = tier.aggregate;
  if (!aggregate.bucket || !aggregate.samples) return true;
  const Record record = {
      aggregate.bucket,
      static_cast<float>(aggregate.sum / aggregate.samples),
      aggregate.minimum,
      aggregate.maximum,
      static_cast<float>(aggregate.importKwh),
      static_cast<float>(aggregate.exportKwh)};
  if (!writeRecord(tier, record)) return false;
  aggregate = {};
  return true;
}

bool HistoryStore::importRecord(Tier tierId, const Record &record) {
  if (!mounted_ || !validRecord(record)) {
    return false;
  }
  return writeRecord(state(tierId), record);
}

size_t HistoryStore::count(Tier tier) const { return state(tier).header.count; }

size_t HistoryStore::usedBytes() const {
  return mounted_ ? LittleFS.usedBytes() : 0;
}

size_t HistoryStore::totalBytes() const {
  return mounted_ ? LittleFS.totalBytes() : 0;
}
