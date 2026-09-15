// Included by HistoryStore.cpp; shares the existing filesystem and Record API.
#ifdef ARDUINO
#include <unistd.h>
#endif
namespace {
constexpr const char *kHistoryPending = "/history-write.pending";
constexpr const char *kHistoryTemporary = "/history-write.tmp";
constexpr size_t kHistoryJournalBytes = 16 + HistoryBlockCodec::kBlockBytes;
constexpr const char *kHistoryShrink = "/history-shrink.pending";
constexpr const char *kHistoryShrinkTemp = "/history-shrink.tmp";
uint32_t shrinkChecksum(const uint32_t *state) {
  uint32_t crc = UINT32_MAX;
  const auto *bytes = reinterpret_cast<const uint8_t *>(state);
  for (size_t i = 0; i < 20; ++i) {
    crc ^= bytes[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
  }
  return ~crc;
}
bool saveShrinkState(uint32_t *state) {
  state[5] = shrinkChecksum(state);
  File file = LittleFS.open(kHistoryShrinkTemp, "w");
  if (!file || file.write(reinterpret_cast<uint8_t *>(state), 24) != 24) return false;
  file.close();
  file = LittleFS.open(kHistoryShrinkTemp, "r");
  uint32_t checked[6];
  if (!file || file.size() != 24 || file.read(reinterpret_cast<uint8_t *>(checked), 24) != 24 ||
      memcmp(state, checked, 24)) return false;
  file.close();
  return LittleFS.rename(kHistoryShrinkTemp, kHistoryShrink);
}
uint32_t historyJournalCrc(const uint8_t *data) {
  uint32_t crc = UINT32_MAX;
  for (size_t i = 0; i < kHistoryJournalBytes; ++i) {
    crc ^= i >= 12 && i < 16 ? 0 : data[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
  }
  return ~crc;
}

}
bool HistoryStore::recoverCompactShrink() {
  using namespace HistoryBlockCodec;
  if (!LittleFS.exists(kHistoryShrink)) return true;
  uint32_t state[6];
  File journal = LittleFS.open(kHistoryShrink, "r");
  if (!journal || journal.size() != sizeof(state) ||
      journal.read(reinterpret_cast<uint8_t *>(state), sizeof(state)) != sizeof(state)) return false;
  journal.close();
  // Only the two explicitly supported files; never accept a stored path.
  const bool minute = state[0] == 0x32534a48;
  TierState &tier = tiers_[minute ? 0 : 2];
  const uint32_t drop = state[1], total = state[2], keep = total - drop;
  if ((!minute && state[0] != 0x31534a48) || state[5] != shrinkChecksum(state) ||
      !drop || drop >= total || total > 1024 || state[3] > keep ||
      state[4] < 1700000000UL) return false;
  while (state[3] < keep) {
    if (!recoverCompactWrite()) return false;
    File file = LittleFS.open(tier.path, "r");
    uint8_t block[kBlockBytes];
    if (!file || file.size() != size_t(total) * kBlockBytes ||
        !file.seek(size_t(drop + state[3]) * kBlockBytes, SeekSet) ||
        file.read(block, sizeof(block)) != sizeof(block) || !valid(block, sizeof(block)) ||
        load32(block + 8) != tier.seconds) return false;
    file.close();
    if (!commitCompactBlock(tier, state[3], block)) return false;
    // Destination is strictly before its source. If power fails before this
    // cursor commit, replaying the same source is safe and deterministic.
    ++state[3];
    if (!saveShrinkState(state)) return false;
    if (serviceHook_) serviceHook_();
  }
  const size_t bytes = size_t(keep) * kBlockBytes;
#ifdef ARDUINO
  if (::truncate(minute ? "/history/minute.bin" : "/history/hour.bin", bytes) != 0) return false;
#else
  if (!LittleFS.truncate(tier.path, bytes)) return false;
#endif
  File verify = LittleFS.open(tier.path, "r");
  if (!verify || verify.size() != bytes) return false;
  verify.close();
  return LittleFS.remove(kHistoryShrink);
}

bool HistoryStore::pruneCompactMinute(uint32_t cutoff) {
  using namespace HistoryBlockCodec;
  TierState &minute = state(Tier::Minute);
  if (minute.header.magic != kMagic && minute.header.magic != HistoryBlockCodec::kMagic) return false;
  if (minute.header.magic != HistoryBlockCodec::kMagic || minute.header.writeIndex ||
      minute.header.generation <= 24) return true;
  if (LittleFS.exists(kHistoryShrink)) return false; // Replayed before loading on reboot.
  File file = LittleFS.open(minute.path, "r");
  if (!file || file.size() % kBlockBytes) return false;
  const uint32_t blocks = file.size() / kBlockBytes;
  uint32_t drop = 0, first = 0, end = 0;
  uint8_t buffer[kBlockBytes];
  while (blocks - drop > 24) {
    if (file.read(buffer, sizeof(buffer)) != sizeof(buffer) || !valid(buffer, sizeof(buffer))) return false;
    const uint32_t start = load32(buffer + 4);
    if (uint64_t(start) + kSlots * 60 > cutoff) break;
    if (!drop) first = start;
    end = start + kSlots * 60;
    ++drop;
  }
  file.close();
  if (!drop) return true;
  // Recompute complete five-minute buckets from the still-intact source and
  // compare with the committed target, including bit-exact cumulative energy.
  Record expected{};
  double sum = 0;
  unsigned samples = 0;
  const auto verify = [&]() {
    if (!samples) return true;
    expected.averageW = static_cast<float>(sum / samples);
    bool found = false;
    const bool read = forEach(Tier::FiveMinute, expected.timestamp, expected.timestamp,
        [&](const Record &got) {
          found = got.averageW == powerValue(powerBits(expected.averageW)) &&
                  got.minimumW == expected.minimumW && got.maximumW == expected.maximumW &&
                  !memcmp(&got.importKwh, &expected.importKwh, sizeof(float)) &&
                  !memcmp(&got.exportKwh, &expected.exportKwh, sizeof(float));
          return true;
        });
    return read && found;
  };
  bool ok = true;
  if (!forEach(Tier::Minute, first - first % 300, ((end - 1) / 300 + 1) * 300 - 1,
      [&](const Record &record) {
        const uint32_t bucket = record.timestamp - record.timestamp % 300;
        if (samples && expected.timestamp != bucket) {
          if (!verify()) { ok = false; return false; }
          samples = 0; sum = 0;
        }
        if (!samples) { expected = record; expected.timestamp = bucket; }
        else {
          expected.minimumW = std::min(expected.minimumW, record.minimumW);
          expected.maximumW = std::max(expected.maximumW, record.maximumW);
          expected.importKwh = record.importKwh;
          expected.exportKwh = record.exportKwh;
        }
        sum += record.averageW; ++samples;
        return true;
      }) || !ok || !verify()) return false;
  uint32_t journal[6] = {0x32534a48, drop, blocks, 0, cutoff, 0};
  return saveShrinkState(journal) && recoverCompactShrink() && loadHeader(minute);
}

bool HistoryStore::pruneCompactHour(uint32_t cutoff) {
  using namespace HistoryBlockCodec;
  TierState &hour = tiers_[2];
  if (hour.header.magic != HistoryBlockCodec::kMagic || hour.header.writeIndex ||
      hour.header.generation <= 139) return true;
  if (LittleFS.exists(kHistoryShrink)) {
    return recoverCompactShrink() && loadHeader(hour);
  }
  File file = LittleFS.open(hour.path, "r");
  if (!file || file.size() % kBlockBytes) return false;
  const uint32_t blocks = file.size() / kBlockBytes;
  uint32_t drop = 0;
  uint8_t block[kBlockBytes];
  while (drop + 1 < blocks && blocks - drop > 139) {
    if (file.read(block, sizeof(block)) != sizeof(block) || !valid(block, sizeof(block))) return false;
    const uint32_t start = load32(block + 4);
    if (uint64_t(start) + kSlots * 3600 > cutoff) break;
    const uint32_t firstDay = start - start % 86400;
    uint8_t required = 0, covered = 0;
    for (size_t i = 0; i < kSlots; ++i) {
      Record record;
      const auto result = getValidated(block, i, record);
      if (result == ReadResult::Invalid) return false;
      if (result == ReadResult::Value)
        required |= uint8_t(1U << ((record.timestamp - firstDay) / 86400));
    }
    if (!forEach(Tier::Day, firstDay, firstDay + 4 * 86400 - 1, [&](const Record &day) {
          covered |= uint8_t(1U << ((day.timestamp - firstDay) / 86400));
          return true;
        })) return false;
    if ((required & covered) != required) break;
    ++drop;
  }
  file.close();
  if (!drop) return true;
  uint32_t state[6] = {0x31534a48, drop, blocks, 0, cutoff, 0};
  return saveShrinkState(state) && recoverCompactShrink() && loadHeader(hour);
}

bool HistoryStore::recoverCompactWrite() {
  using namespace HistoryBlockCodec;
  if (!LittleFS.exists(kHistoryPending)) return true;
  uint8_t journal[kHistoryJournalBytes];
  File pending = LittleFS.open(kHistoryPending, "r");
  if (!pending || pending.size() != sizeof(journal) ||
      pending.read(journal, sizeof(journal)) != sizeof(journal)) return false;
  pending.close();
  const uint32_t id = load32(journal + 4), block = load32(journal + 8);
  if (load32(journal) != 0x31574a48 || id >= 6 ||
      load32(journal + 12) != historyJournalCrc(journal) ||
      !valid(journal + 16, kBlockBytes) ||
      load32(journal + 24) != tiers_[id].seconds || block >= 1024) return false;
  File target = LittleFS.open(tiers_[id].path, "r+");
  if (!target || size_t(block) * kBlockBytes > target.size() ||
      target.size() > 1024 * kBlockBytes) return false;
  // Pending is durable before the active block is touched. A short/torn write
  // leaves it available for exactly the same replay after the next boot.
  if (!target.seek(size_t(block) * kBlockBytes, SeekSet) ||
      target.write(journal + 16, kBlockBytes) != kBlockBytes) return false;
  target.close();
  target = LittleFS.open(tiers_[id].path, "r");
  if (!target || !target.seek(size_t(block) * kBlockBytes, SeekSet)) return false;
  uint8_t check[64];
  for (size_t i = 0; i < kBlockBytes; i += sizeof(check)) {
    const size_t n = std::min(sizeof(check), kBlockBytes - i);
    if (target.read(check, n) != n || memcmp(check, journal + 16 + i, n))
      return false;
    if (serviceHook_) serviceHook_();
  }
  target.close();
  return LittleFS.remove(kHistoryPending);
}

bool HistoryStore::commitCompactBlock(TierState &tier, uint32_t block,
                                       const uint8_t *data) {
  using namespace HistoryBlockCodec;
  // Do not replace another uncompleted transaction. The caller retries after
  // reboot/replay, so an acknowledged record is never silently skipped.
  if (LittleFS.exists(kHistoryPending)) return false;
  uint8_t journal[kHistoryJournalBytes];
  store32(journal, 0x31574a48);
  store32(journal + 4, uint32_t(&tier - tiers_));
  store32(journal + 8, block);
  store32(journal + 12, 0);
  memcpy(journal + 16, data, kBlockBytes);
  store32(journal + 12, historyJournalCrc(journal));
  File file = LittleFS.open(kHistoryTemporary, "w");
  if (!file || file.write(journal, sizeof(journal)) != sizeof(journal)) return false;
  file.close();
  file = LittleFS.open(kHistoryTemporary, "r");
  if (!file || file.size() != sizeof(journal)) return false;
  uint8_t check[64];
  for (size_t i = 0; i < sizeof(journal); i += sizeof(check)) {
    const size_t n = std::min(sizeof(check), sizeof(journal) - i);
    if (file.read(check, n) != n || memcmp(check, journal + i, n)) return false;
  }
  file.close();
  // LittleFS rename commits atomically; only the committed filename is replayed.
  return LittleFS.rename(kHistoryTemporary, kHistoryPending) && recoverCompactWrite();
}

bool HistoryStore::writeCompact(TierState &tier, const Record &record) {
  using namespace HistoryBlockCodec;
  const auto samples = [](const uint8_t *data) {
    uint32_t n = 0;
    for (size_t i = 0; i < kSlots; ++i)
      if (data[kHeaderBytes + i * kValueBytes + 7] & 0x80) ++n;
    return n;
  };
  if (!validRecord(record) || record.timestamp < tier.lastWrittenBucket) return false;
  uint8_t block[kBlockBytes];
  File file = LittleFS.open(tier.path, "r");
  if (!file || !file.size() || file.size() % kBlockBytes) return false;
  const uint32_t blocks = file.size() / kBlockBytes;
  uint32_t index = (tier.header.writeIndex + blocks - 1) % blocks;
  if (!file.seek(size_t(index) * kBlockBytes, SeekSet) ||
      file.read(block, sizeof(block)) != sizeof(block) || !valid(block, sizeof(block)))
    return false;
  file.close();
  uint32_t replaced = samples(block);
  const uint32_t oldStart = load32(block + 4);
  const bool newBlock = !tier.header.count ||
      uint64_t(record.timestamp) >= uint64_t(oldStart) + kSlots * tier.seconds;
  if (newBlock) {
    const uint32_t limit = (tier.capacity + kSlots - 1) / kSlots + 1;
    index = !tier.header.count ? index : (blocks < limit ? blocks : tier.header.writeIndex);
    replaced = 0;
    if (tier.header.count && index < blocks) {
      File old = LittleFS.open(tier.path, "r");
      if (!old || !old.seek(size_t(index) * kBlockBytes, SeekSet) ||
          old.read(block, sizeof(block)) != sizeof(block) || !valid(block, sizeof(block)))
        return false;
      replaced = samples(block);
    }
    uint32_t start = record.timestamp - record.timestamp % (kSlots * tier.seconds);
    if (!validTime(start, tier.seconds)) start = record.timestamp;
    // Legacy staging used the first actual timestamp as its block origin.
    // Keep that last block's full time span disjoint from the new block.
    if (tier.header.count && start < oldStart + kSlots * tier.seconds)
      start = oldStart + kSlots * tier.seconds;
    if (!HistoryBlockCodec::begin(block, sizeof(block), start, tier.seconds)) return false;
  }
  if (!put(block, sizeof(block), record) || !seal(block, sizeof(block)) ||
      !commitCompactBlock(tier, index, block)) return false;
  if (newBlock && tier.header.count && index < blocks)
    tier.header.writeIndex = (index + 1) % blocks;
  tier.header.count = tier.header.count - replaced + samples(block);
  tier.header.generation = std::max(blocks, index + 1);
  tier.lastWrittenBucket = record.timestamp;
  tier.orderKnown = tier.ordered = true;
  return true;
}

bool HistoryStore::compactActive() const {
  if (!mounted_) return false;
  for (const auto &tier : tiers_)
    if (tier.header.magic != HistoryBlockCodec::kMagic) return false;
  return true;
}

bool HistoryStore::migrateCompact() {
  using namespace HistoryBlockCodec;
  if (!mounted_ || !recoverCompactWrite()) return false;
  // Convert smaller files first; reclaim verified savings before the largest
  // quarter-hour snapshot is created. No source is removed before verification.
  const uint8_t order[] = {0, 3, 2, 1, 4, 5};
  for (uint8_t id : order) {
    TierState &tier = tiers_[id];
    if (tier.header.magic == HistoryBlockCodec::kMagic) continue;
    const Tier tierId = static_cast<Tier>(id);
    if (!flushPending(tierId)) return false;
    // Read every original before selecting any duplicates. Ambiguous tiers
    // remain unchanged while other independent tiers can still be converted.
    uint32_t seen = 0, previous = 0, first = 0;
    bool representable = true;
    if (!forEachMigrationRecord(tierId, [&](const Record &record) {
          if (!seen) first = record.timestamp;
          if ((seen && record.timestamp <= previous) ||
              (record.timestamp - first) % tier.seconds)
            representable = false;
          previous = record.timestamp;
          ++seen;
          return true;
        })) representable = false;
    if (!representable) continue;
    const char *stage = compactStagePath(tierId);
    if (seen) {
      if (!resumeCompactCopy(tierId)) return false;
    } else {
      std::unique_ptr<uint8_t[]> block(new (std::nothrow) uint8_t[kBlockBytes]);
      if (!block || !HistoryBlockCodec::begin(block.get(), kBlockBytes, 1700000040UL, tier.seconds) ||
          !seal(block.get(), kBlockBytes)) return false;
      File file = LittleFS.open(stage, "w");
      if (!file || file.write(block.get(), kBlockBytes) != kBlockBytes) return false;
      file.close();
      block.reset();
      TierState checked = tier;
      checked.path = stage;
      if (!forEachCompact(checked, 0, UINT32_MAX, [](const Record &) { return false; }))
        return false;
    }
    // The replacement was fully compared against the original, including the
    // energy bits and rounded power. Atomic rename leaves either complete file.
    if (!LittleFS.rename(stage, tier.path) || !loadHeader(tier)) return false;
  }
  // loadHeader applies the new capacity only to successfully converted files.
  // Preserved legacy rings must retain their original physical geometry.
  return compactActive();
}
