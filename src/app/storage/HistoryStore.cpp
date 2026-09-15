#include "HistoryStore.h"
#include "PartitionSafety.h"
#include "HistoryBlockCodec.h"
#include "HistoryRetention.h"

#include <LittleFS.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <new>

bool HistoryStore::begin() {
  mounted_ = LittleFS.begin(false, "/history", 10, "history");
  if (!mounted_) {
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "history");
    if (partitionIsBlank(partition))
      mounted_ = LittleFS.begin(true, "/history", 10, "history");
  }
  if (!mounted_) return false;
  if (!recoverCompactWrite() || !recoverCompactShrink()) { mounted_ = false; return false; }
  for (auto &tier : tiers_) {
    if (!loadHeader(tier)) {
      mounted_ = false;
      return false;
    }
  }
  return true;
}

bool HistoryStore::loadHeader(TierState &tier) {
  tier.orderKnown = false;
  tier.ordered = false;
  const bool existed = LittleFS.exists(tier.path);
  File file = LittleFS.open(tier.path, "r");
  if (file && file.size() >= 4) {
    uint8_t magic[4];
    if (file.read(magic, sizeof(magic)) != sizeof(magic) ||
        !file.seek(0, SeekSet)) return false;
    if (HistoryBlockCodec::load32(magic) == HistoryBlockCodec::kMagic) {
      file.close();
      tier.header = {HistoryBlockCodec::kMagic, tier.capacity, 0, 0, 0, 0};
      if (tier.seconds == 60) tier.capacity = 1440;
      if (tier.seconds == 900) tier.capacity = HistoryRetention::kQuarterSeconds / 900;
      if (tier.seconds == 1800) tier.capacity = 17520;
      if (tier.seconds == 86400) tier.capacity = 3650;
      uint32_t count = 0, last = 0;
      if (!forEachCompact(tier, 0, UINT32_MAX, [&](const Record &record) {
            ++count;
            last = record.timestamp;
            return true;
          })) return false;
      tier.header.count = count;
      if (tier.seconds == 3600) tier.capacity = 8760;
      // Keep oversized legacy conversions, but do not grow a normal ring on
      // every reboot merely because its working block holds extra samples.
      const uint32_t blocks = (tier.capacity + HistoryBlockCodec::kSlots - 1) /
                                  HistoryBlockCodec::kSlots + 1;
      if (tier.header.generation > blocks) tier.capacity = std::max(tier.capacity, count);
      tier.lastWrittenBucket = last;
      tier.orderKnown = tier.ordered = true;
      return true;
    }
  }
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
          return false;
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
  // Preserve damaged files, including zero-byte files and failed reads.
  if (existed) return false;
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
  tier.orderKnown = ok;
  tier.ordered = ok;
  return ok;
}

uint32_t HistoryStore::headerChecksum(const Header &header) const {
  return header.magic ^ header.capacity ^ header.writeIndex ^ header.count ^
         header.generation ^ 0xA57C31E9;
}

bool HistoryStore::validHeader(const Header &header, uint32_t capacity) const {
  return header.magic == kMagic && header.capacity == capacity &&
         header.writeIndex < capacity && header.count <= capacity &&
         (header.count == capacity || header.writeIndex == header.count) &&
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
  if (tier.header.magic == HistoryBlockCodec::kMagic) return writeCompact(tier, record);
  File file = LittleFS.open(tier.path, "r+");
  if (!file) return false;
  const size_t offset =
      2 * sizeof(Header) +
      static_cast<size_t>(tier.header.writeIndex) * sizeof(Record);
  if (!file.seek(offset, SeekSet) ||
      file.write(reinterpret_cast<const uint8_t *>(&record), sizeof(Record)) !=
          sizeof(Record)) {
    tier.orderKnown = false;
    file.close();
    return false;
  }
  Header next = tier.header;
  next.writeIndex = (next.writeIndex + 1) % tier.capacity;
  next.count = std::min(next.count + 1, tier.capacity);
  ++next.generation;
  next.checksum = headerChecksum(next);
  const size_t headerOffset =
      (next.generation & 1U) ? sizeof(Header) : 0;
  if (!file.seek(headerOffset, SeekSet) ||
      file.write(reinterpret_cast<const uint8_t *>(&next),
                 sizeof(Header)) != sizeof(Header)) {
    tier.orderKnown = false;
    file.close();
    return false;
  }
  file.close();
  tier.header = next;
  if (tier.lastWrittenBucket && record.timestamp < tier.lastWrittenBucket) {
    tier.orderKnown = true;
    tier.ordered = false;
  }
  tier.lastWrittenBucket = record.timestamp;
  return true;
}

void HistoryStore::updateTier(TierState &tier, uint32_t epoch, double powerW,
                              double importKwh, double exportKwh) {
  const uint32_t bucket = epoch - epoch % tier.seconds;
  Aggregate &aggregate = tier.aggregate;
  // A backwards clock step must not mix old timestamps into newer buckets.
  if (bucket < tier.lastWrittenBucket ||
      (aggregate.bucket && bucket < aggregate.bucket)) return;
  if (!aggregate.bucket && tier.lastWrittenBucket == bucket) return;
  if (aggregate.bucket && bucket != aggregate.bucket && aggregate.samples) {
    Record record = {
        aggregate.bucket,
        static_cast<float>(aggregate.sum / aggregate.samples),
        aggregate.minimum,
        aggregate.maximum,
        static_cast<float>(aggregate.importKwh),
        static_cast<float>(aggregate.exportKwh)};
    // Keep the pending aggregate and logical cursor when storage fails.
    // A later update retries it instead of silently discarding the samples.
    if (!writeRecord(tier, record)) return;
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

bool HistoryStore::forEach(Tier tierId, uint32_t since, uint32_t until,
                           const RecordCallback &callback) {
  if (!mounted_) return false;
  TierState &tier = state(tierId);
  if (tier.header.magic == HistoryBlockCodec::kMagic)
    return forEachCompact(tier, since, until, callback);
  File file = LittleFS.open(tier.path, "r");
  if (!file) return false;
  if (!tier.header.count || since > until) {
    file.close();
    return true;
  }
  const uint32_t first =
      tier.header.count < tier.capacity ? 0 : tier.header.writeIndex;
  bool callbackStopped = false;
  uint32_t recordsRead = 0;
  const bool ordered = tier.orderKnown && tier.ordered;
  const bool checkOrder = !tier.orderKnown;
  bool observedOrdered = true;
  uint32_t previousTimestamp = 0;

  const auto readLogical = [&](uint32_t logical, Record &record) {
    const uint32_t slot = (first + logical) % tier.capacity;
    const size_t offset =
        2 * sizeof(Header) + static_cast<size_t>(slot) * sizeof(Record);
    return file.seek(offset, SeekSet) &&
           file.read(reinterpret_cast<uint8_t *>(&record), sizeof(record)) ==
               sizeof(record);
  };

  // Only use lower_bound / early exit after chronological order was verified.
  // Old imports may be unordered. The first complete scan checks this once;
  // unordered rings keep a sequential filter without dropping later matches.
  uint32_t startLogical = 0;
  bool binarySearchValid = ordered && since != 0;
  uint32_t low = 0;
  uint32_t high = tier.header.count;
  while (binarySearchValid && low < high) {
    const uint32_t middle = low + (high - low) / 2;
    Record record;
    if (!readLogical(middle, record)) {
      file.close();
      return false;
    }
    ++recordsRead;
    if (!validRecord(record)) {
      binarySearchValid = false;
      break;
    }
    if (record.timestamp < since)
      low = middle + 1;
    else
      high = middle;
  }
  if (binarySearchValid) startLogical = low;

  const auto readRange = [&](uint32_t startSlot, uint32_t recordCount) {
    if (!recordCount) return true;
    const size_t offset =
        2 * sizeof(Header) +
        static_cast<size_t>(startSlot) * sizeof(Record);
    if (!file.seek(offset, SeekSet)) return false;
    constexpr uint32_t kReadBatchRecords = 32;
    Record records[kReadBatchRecords];
    uint32_t remainingRecords = recordCount;
    while (remainingRecords) {
      const uint32_t batchRecords =
          std::min(remainingRecords, kReadBatchRecords);
      const size_t batchBytes = static_cast<size_t>(batchRecords) * sizeof(Record);
      if (file.read(reinterpret_cast<uint8_t *>(records), batchBytes) !=
          batchBytes)
        return false;
      remainingRecords -= batchRecords;
      if (serviceHook_) serviceHook_();
      for (uint32_t i = 0; i < batchRecords; ++i) {
        const Record &record = records[i];
        ++recordsRead;
        if (!validRecord(record)) {
          observedOrdered = false;
          if ((recordsRead & 0x7fU) == 0) delay(0);
          continue;
        }
        if (checkOrder) {
          if (record.timestamp < previousTimestamp) observedOrdered = false;
          previousTimestamp = record.timestamp;
        }
        if (ordered && record.timestamp > until) {
          callbackStopped = true;
          return true;
        }
        if (record.timestamp >= since && record.timestamp <= until &&
            !callback(record)) {
          callbackStopped = true;
          return true;
        }
        if ((recordsRead & 0x7fU) == 0) delay(0);
      }
    }
    return true;
  };

  // Starting at the lower bound, the relevant logical tail is physically
  // stored in at most two contiguous regions. Preserve logical ring order.
  const uint32_t remaining = tier.header.count - startLogical;
  const uint32_t startSlot = (first + startLogical) % tier.capacity;
  const uint32_t firstRange = std::min(remaining, tier.capacity - startSlot);
  bool ok = readRange(startSlot, firstRange);
  if (ok && !callbackStopped && firstRange < remaining)
    ok = readRange(0, remaining - firstRange);
  if (ok && !callbackStopped && checkOrder) {
    tier.orderKnown = true;
    tier.ordered = observedOrdered;
  }
  file.close();
  return ok;
}

bool HistoryStore::clear(Tier tierId) {
  if (!mounted_ || readOnly()) return false;
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
  TierState &tier = state(tierId);
  if (record.timestamp < tier.lastWrittenBucket) return false;
  return writeRecord(tier, record);
}

size_t HistoryStore::count(Tier tier) const { return state(tier).header.count; }

size_t HistoryStore::usedBytes() const {
  return mounted_ ? LittleFS.usedBytes() : 0;
}

size_t HistoryStore::totalBytes() const {
  return mounted_ ? LittleFS.totalBytes() : 0;
}

bool HistoryStore::readOnly() const {
  return false;
}

const char *HistoryStore::compactStagePath(Tier tier) {
  switch (tier) {
    case Tier::Minute: return "/minute.irh2-stage";
    case Tier::QuarterHour: return "/quarter.irh2-stage";
    case Tier::Hour: return "/hour.irh2-stage";
    case Tier::Day: return "/day.irh2-stage";
    case Tier::HalfHour: return "/half.irh2-stage";
    case Tier::FiveMinute: return "/five.irh2-stage";
  }
  return nullptr;
}

bool HistoryStore::forEachMigrationRecord(Tier tierId, const RecordCallback &callback) {
  TierState &tier = state(tierId);
  if (tier.header.magic != kMagic) return false;
  const Header snapshot = tier.header;
  File file = LittleFS.open(tier.path, "r");
  if (!file) return false;
  const uint32_t first = snapshot.count < snapshot.capacity ? 0 : snapshot.writeIndex;
  if (!file.seek(2 * sizeof(Header) + size_t(first) * sizeof(Record), SeekSet)) return false;
  // Small bounded look-ahead, used only during migration. Larger clock jumps
  // remain legacy rather than allocating an archive-sized sorting buffer.
  constexpr size_t lookAhead = 32;
  // Bounded temporary workspace, not multiplied on the loop task's stack.
  std::unique_ptr<Record[]> windowStorage(new (std::nothrow) Record[lookAhead]);
  if (!windowStorage) return false;
  Record *window = windowStorage.get();
  Record previous{};
  size_t buffered = 0;
  uint32_t read = 0;
  bool havePrevious = false;
  while (read < snapshot.count || buffered) {
    while (buffered < lookAhead && read < snapshot.count) {
      if ((read && (first + read) % snapshot.capacity == 0 &&
           !file.seek(2 * sizeof(Header), SeekSet)) ||
          file.read(reinterpret_cast<uint8_t *>(&window[buffered]), sizeof(Record)) != sizeof(Record)) return false;
      ++read;
      // Only proven record corruption is disposable; I/O failures still abort.
      if (validRecord(window[buffered])) ++buffered;
      if ((read & 31U) == 0 && serviceHook_) serviceHook_();
    }
    if (!buffered) break;
    // At most 32 entries, normally already sorted except for the new tail.
    // Iterative insertion avoids recursive library-sort stack frames.
    for (size_t i = 1; i < buffered; ++i) {
      const Record value = window[i];
      size_t j = i;
      while (j && value.timestamp < window[j - 1].timestamp) {
        window[j] = window[j - 1];
        --j;
      }
      window[j] = value;
    }
    if (havePrevious && window[0].timestamp <= previous.timestamp) return false;
    size_t group = 1;
    while (group < buffered && window[group].timestamp == window[0].timestamp) ++group;
    if (group == buffered && read < snapshot.count) return false;
    bool identical = true;
    for (size_t i = 1; i < group; ++i)
      identical &= memcmp(&window[0], &window[i], sizeof(Record)) == 0;
    size_t chosen = 0;
    if (!identical) {
      if (!havePrevious || group == buffered) return false;
      // A conflicting next neighbour is not evidence for choosing this one.
      for (size_t i = group + 1; i < buffered &&
           window[i].timestamp == window[group].timestamp; ++i)
        if (memcmp(&window[group], &window[i], sizeof(Record))) return false;
      bool uniqueNeighbour = true;
      // Rare conflict path only: a later clock correction outside the small
      // look-ahead must not make the apparent right neighbour unreliable.
      if (!forEach(tierId, 0, UINT32_MAX, [&](const Record &record) {
            if (record.timestamp > window[0].timestamp &&
                record.timestamp < window[group].timestamp) uniqueNeighbour = false;
            if (record.timestamp == window[group].timestamp &&
                memcmp(&record, &window[group], sizeof(Record))) uniqueNeighbour = false;
            return true;
          }) || !uniqueNeighbour) return false;
      unsigned candidates = 0;
      for (size_t i = 0; i < group; ++i) {
        bool duplicate = false;
        for (size_t j = 0; j < i; ++j)
          duplicate |= memcmp(&window[i], &window[j], sizeof(Record)) == 0;
        if (duplicate) continue;
        bool evidence = false, contradicted = false, reset = false;
        const auto counter = [&](float before, float value, float after) {
          if (std::isfinite(before) && std::isfinite(value) && std::isfinite(after)) {
            // Counter resets or inconsistent neighbours cannot rank candidates.
            if (before > after) { reset = true; return; }
            evidence = true;
            contradicted |= value < before || value > after;
          }
        };
        counter(previous.importKwh, window[i].importKwh, window[group].importKwh);
        counter(previous.exportKwh, window[i].exportKwh, window[group].exportKwh);
        if (!evidence || reset) return false;
        if (!contradicted) { chosen = i; ++candidates; }
      }
      if (candidates != 1) return false;
    }
    previous = window[chosen];
    havePrevious = true;
    if (!callback(previous)) return false;
    buffered -= group;
    memmove(window, window + group, buffered * sizeof(Record));
    if (serviceHook_) serviceHook_();
  }
  return tier.header.generation == snapshot.generation &&
         tier.header.count == snapshot.count && tier.header.writeIndex == snapshot.writeIndex;
}

bool HistoryStore::stageCompactCopy(Tier tierId) {
  using namespace HistoryBlockCodec;
  const char *target = compactStagePath(tierId);
  if (!target || !mounted_ || readOnly() || LittleFS.exists(target)) return false;
  TierState &tier = state(tierId);
  if (!tier.header.count || tier.aggregate.samples) return false;
  const Header snapshot = tier.header;
  const auto unchanged = [&]() {
    return tier.header.generation == snapshot.generation &&
           tier.header.count == snapshot.count &&
           tier.header.writeIndex == snapshot.writeIndex;
  };
  uint32_t records = 0, blocks = 0, start = 0, previous = 0;
  bool ok = true;
  // Exact space calculation: gaps can require more blocks than count/64.
  if (!forEachMigrationRecord(tierId, [&](const Record &record) {
        if ((records && record.timestamp <= previous) ||
            (blocks && (record.timestamp - start) % tier.seconds)) {
          ok = false; return false;
        }
        if (!blocks || uint64_t(record.timestamp) >=
                           uint64_t(start) + kSlots * tier.seconds) {
          start = record.timestamp;
          if (!validTime(start, tier.seconds)) { ok = false; return false; }
          ++blocks;
        }
        previous = record.timestamp;
        ++records;
        return true;
      }) || !ok || !unchanged() || !records) return false;
  const uint32_t expectedRecords = records;
  const uint64_t bytes = uint64_t(blocks) * kBlockBytes;
  const size_t used = LittleFS.usedBytes(), total = LittleFS.totalBytes();
  // Reserve is deliberately conservative; a successful preflight alone is not
  // proof of LittleFS full-disk behavior. Every write/read is checked below.
  if (total < used || bytes + 8192 > total - used || LittleFS.exists(target)) return false;

  std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[kBlockBytes]);
  if (!buffer) return false;
  File output = LittleFS.open(target, "w");
  if (!output) return false;
  blocks = records = 0;
  const auto writeBlock = [&]() {
    return unchanged() && seal(buffer.get(), kBlockBytes) &&
           output.write(buffer.get(), kBlockBytes) == kBlockBytes;
  };
  ok = true;
  const bool scanned = forEachMigrationRecord(tierId, [&](const Record &record) {
    if (!blocks || uint64_t(record.timestamp) >=
                       uint64_t(start) + kSlots * tier.seconds) {
      if (blocks && !writeBlock()) { ok = false; return false; }
      start = record.timestamp;
      if (!HistoryBlockCodec::begin(buffer.get(), kBlockBytes, start, tier.seconds)) {
        ok = false; return false;
      }
      ++blocks;
    }
    if (!put(buffer.get(), kBlockBytes, record)) { ok = false; return false; }
    ++records;
    return true;
  });
  ok = scanned && ok && records == expectedRecords && unchanged() && writeBlock();
  output.close();
  buffer.reset(); // Writing and readback never keep two block buffers alive.
  if (!ok) return false; // Leave the source and incomplete stage untouched.

  return verifyCompactCopy(tierId);
}

bool HistoryStore::verifyCompactCopy(Tier tierId) {
  using namespace HistoryBlockCodec;
  const char *target = compactStagePath(tierId);
  if (!target || !mounted_ || readOnly()) return false;
  File file = LittleFS.open(target, "r");
  if (!file || !file.size() || file.size() % kBlockBytes) return false;
  const size_t blocks = file.size() / kBlockBytes;
  size_t blockIndex = 0, slot = kSlots;
  std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[kBlockBytes]);
  if (!buffer) return false;
  bool validFile = true;
  const auto nextRecord = [&](Record &record) {
    while (true) {
      if (slot == kSlots) {
        if (blockIndex == blocks) return false;
        if (file.read(buffer.get(), kBlockBytes) != kBlockBytes ||
            !valid(buffer.get(), kBlockBytes) || load32(buffer.get() + 8) != state(tierId).seconds) {
          validFile = false;
          return false;
        }
        ++blockIndex;
        slot = 0;
      }
      const auto result = getValidated(buffer.get(), slot++, record);
      if (result == ReadResult::Invalid) { validFile = false; return false; }
      if (result == ReadResult::Value) return true;
    }
  };
  // Independently read the staged bytes and compare every selected observation;
  // skipped duplicates never change the energy bits of the chosen original.
  const bool compared = forEachMigrationRecord(tierId, [&](const Record &expected) {
    Record got;
    return nextRecord(got) && got.timestamp == expected.timestamp &&
        got.averageW == powerValue(powerBits(expected.averageW)) &&
        got.minimumW == powerValue(powerBits(expected.minimumW)) &&
        got.maximumW == powerValue(powerBits(expected.maximumW)) &&
        !memcmp(&got.importKwh, &expected.importKwh, sizeof(float)) &&
        !memcmp(&got.exportKwh, &expected.exportKwh, sizeof(float));
  });
  Record extra;
  return compared && !nextRecord(extra) && validFile;
}

bool HistoryStore::resumeCompactCopy(Tier tierId) {
  const char *target = compactStagePath(tierId);
  if (!target || !mounted_ || readOnly()) return false;
  TierState &tier = state(tierId);
  if (tier.aggregate.samples || !tier.header.count || !LittleFS.exists(tier.path))
    return false;
  // Re-read source headers before considering disposal of an interrupted stage.
  // A missing/damaged original is never repaired by silently deleting a copy.
  TierState checked = tier;
  if (!loadHeader(checked) || checked.header.magic != kMagic ||
      checked.header.generation != tier.header.generation ||
      checked.header.count != tier.header.count ||
      checked.header.writeIndex != tier.header.writeIndex) return false;
  const uint32_t generation = tier.header.generation;
  if (!forEachMigrationRecord(tierId, [](const Record &) { return true; }) ||
      generation != tier.header.generation) return false;
  if (LittleFS.exists(target)) {
    if (verifyCompactCopy(tierId)) return true;
    // Only the private staging path is disposable, with its original validated
    // above and still active. No source rename, deletion or activation occurs.
    if (generation != tier.header.generation || !LittleFS.remove(target)) return false;
  }
  return stageCompactCopy(tierId);
}

bool HistoryStore::forEachCompact(TierState &tier, uint32_t since,
                                  uint32_t until, const RecordCallback &callback) {
  using namespace HistoryBlockCodec;
  File file = LittleFS.open(tier.path, "r");
  if (!file || !file.size() || file.size() % kBlockBytes) return false;
  if (since > until) return true;
  const size_t blocks = file.size() / kBlockBytes;
  uint8_t buffer[kBlockBytes];
  const auto readBlock = [&](size_t index) {
    const bool ok = file.seek(index * kBlockBytes, SeekSet) &&
        file.read(buffer, sizeof(buffer)) == sizeof(buffer) &&
        valid(buffer, sizeof(buffer)) && load32(buffer + 8) == tier.seconds;
    if (serviceHook_) serviceHook_();
    delay(0);
    return ok;
  };
  // A full mutable ring has one physical timestamp wrap. Discover it once at
  // boot; logical range queries continue to use binary search thereafter.
  size_t first = tier.header.writeIndex;
  if (!tier.orderKnown) {
    first = 0;
    uint32_t prior = 0, firstStart = 0;
    unsigned wraps = 0;
    for (size_t i = 0; i < blocks; ++i) {
      if (!readBlock(i)) return false;
      const uint32_t start = load32(buffer + 4);
      if (!i) firstStart = start;
      if (i && start <= prior) { first = i; ++wraps; }
      prior = start;
    }
    if (wraps > 1 || (wraps && prior >= firstStart)) return false;
    tier.header.writeIndex = first;
    tier.header.generation = blocks;
  }
  const auto readLogical = [&](size_t index) { return readBlock((first + index) % blocks); };
  // begin() validates the entire immutable archive first. Range queries then
  // locate the first potentially relevant block using verified block headers.
  size_t low = 0, high = blocks;
  if (tier.orderKnown && tier.ordered && since) {
    while (low < high) {
      const size_t middle = low + (high - low) / 2;
      if (!readLogical(middle)) return false;
      const uint32_t end = load32(buffer + 4) + (kSlots - 1) * tier.seconds;
      if (end < since) low = middle + 1;
      else high = middle;
    }
  }
  uint32_t previousEnd = 0;
  for (size_t index = low; index < blocks; ++index) {
    if (!readLogical(index)) return false;
    const uint32_t start = load32(buffer + 4);
    if (previousEnd && start <= previousEnd) return false;
    previousEnd = start + (kSlots - 1) * tier.seconds;
    if (start > until) break;
    for (size_t slot = 0; slot < kSlots; ++slot) {
      Record record;
      const auto result = getValidated(buffer, slot, record);
      if (result == ReadResult::Invalid) return false;
      if (result == ReadResult::Missing) continue;
      if (!validRecord(record)) return false;
      if (record.timestamp >= since && record.timestamp <= until &&
          !callback(record)) return true;
    }
  }
  return true;
}

#include "HistoryCompactWriter.cpp"
#include "HistoryRetention.cpp"
