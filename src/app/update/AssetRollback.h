#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace AssetRollback {
constexpr uint32_t kAppLimit = 0x13F000;
constexpr uint32_t kJournalSize = 0x1000;
constexpr uint32_t kBackupOffset = 0x140000;
constexpr uint32_t kAssetSize = 0x10000;
constexpr uint8_t kAssets = 2;
enum class State : uint32_t { Ready = 1, Verified = 2, Complete = 3, Restored = 4 };
enum class Result : uint8_t { Clean, Restored, NewReady, Reboot, Error };

// Physical partition validation is owned by the existing ESP storage layer.
// This boundary also allows interrupted writes to be tested with the real engine.
struct Io {
  virtual bool read(uint8_t slot, uint32_t offset, void *data, size_t size) = 0;
  virtual bool write(uint8_t slot, uint32_t offset, const void *data, size_t size) = 0;
  virtual bool erase(uint8_t slot, uint32_t offset, size_t size) = 0;
  virtual bool hash(uint8_t slot, uint32_t offset, size_t size, uint8_t out[32]) = 0;
  virtual bool digest(const void *data, size_t size, uint8_t out[32]) = 0;
  virtual bool appIdentity(uint8_t slot, uint8_t out[32]) = 0;
  virtual bool boot(uint8_t slot) = 0;
  virtual void service() = 0;
};

struct Record {
  uint32_t magic;
  uint32_t schema;
  uint32_t sequence;
  State state;
  uint32_t source;
  uint32_t target;
  uint32_t appSize;
  uint8_t oldApp[32];
  uint8_t newApp[32];
  uint8_t oldAssets[32];
  uint8_t newAssets[32];
  uint8_t checksum[32];
};
static_assert(sizeof(Record) == 188, "journal layout must stay stable");
constexpr uint32_t kMagic = 0x31524241; // ABR1
constexpr uint32_t kStride = 256;
constexpr uint32_t kCommit = 0xC047AB1E;

inline bool terminal(State state) {
  return state == State::Complete || state == State::Restored;
}

class Engine {
 public:
  explicit Engine(Io &io) : io_(io) {}

  // Torn entries are skipped; a previously committed entry remains authoritative.
  // If there is no committed entry, assets could not legally have been erased.
  bool latest(uint8_t slot, Record &latest, bool &found) {
    found = false;
    for (uint32_t offset = 0; offset < kJournalSize; offset += kStride) {
      Record value;
      if (!io_.read(slot, kAppLimit + offset, &value, sizeof(value))) return false;
      uint32_t commit;
      if (!io_.read(slot, kAppLimit + offset + kStride - sizeof(commit), &commit, sizeof(commit))) return false;
      if (commit != kCommit) continue;
      uint8_t digest[32];
      if (value.magic != kMagic || value.schema != 1 || value.target != slot ||
          value.source > 1 || value.source == value.target ||
          value.appSize < 1024 || value.appSize > kAppLimit ||
          value.sequence != offset / kStride + 1 ||
          static_cast<uint32_t>(value.state) < 1 ||
          static_cast<uint32_t>(value.state) > 4) return false;
      if (!io_.digest(&value, offsetof(Record, checksum), digest)) return false;
      if (memcmp(digest, value.checksum, sizeof(digest))) return false;
      if (found && (memcmp(value.oldApp, latest.oldApp, 128) ||
                    value.source != latest.source || value.appSize != latest.appSize))
        return false;
      latest = value;
      found = true;
    }
    return true;
  }

  bool idle() {
    for (uint8_t slot = 0; slot < 2; ++slot) {
      Record record; bool found;
      if (!latest(slot, record, found) || (found && !terminal(record.state))) return false;
    }
    return true;
  }

  // A format-changing migration may run only after both app slots contain
  // this same verified reader/writer and the asset transaction is complete.
  // App identities exclude the journal/backup tail of the OTA partitions.
  bool sameVerifiedAppPair(uint8_t running) {
    uint8_t first[32], second[32];
    return running < 2 && idle() && io_.appIdentity(0, first) &&
           io_.appIdentity(1, second) && !memcmp(first, second, sizeof(first));
  }

  bool prepare(uint8_t source, uint8_t target, uint32_t appSize,
               const uint8_t newApp[32], const uint8_t newAssets[32]) {
    if (source > 1 || target > 1 || source == target ||
        appSize < 1024 || appSize > kAppLimit || !idle()) return false;
    Record record = {};
    record.magic = kMagic; record.schema = 1;
    record.source = source; record.target = target; record.appSize = appSize;
    record.state = State::Ready;
    memcpy(record.newApp, newApp, 32);
    memcpy(record.newAssets, newAssets, 32);
    if (!io_.appIdentity(source, record.oldApp) ||
        !io_.hash(kAssets, 0, kAssetSize, record.oldAssets)) return false;
    // Erase journal before replacing a completed transaction's backup.
    if (!io_.erase(target, kAppLimit, kJournalSize) ||
        !io_.erase(target, kBackupOffset, kAssetSize) ||
        !copy(kAssets, 0, target, kBackupOffset) ||
        !matches(target, kBackupOffset, kAssetSize, record.oldAssets)) return false;
    return append(record);
  }

  bool verifyAssets(uint8_t target) {
    Record record; bool found;
    if (!latest(target, record, found) || !found || record.state != State::Ready ||
        !matches(kAssets, 0, kAssetSize, record.newAssets)) return false;
    record.state = State::Verified;
    return append(record);
  }

  Result recover(uint8_t running) {
    Record record = {}; bool pending = false;
    for (uint8_t slot = 0; slot < 2; ++slot) {
      Record candidate; bool found;
      if (!latest(slot, candidate, found)) return Result::Error;
      if (found && !terminal(candidate.state)) {
        if (pending) return Result::Error;
        record = candidate; pending = true;
      }
    }
    if (!pending) return Result::Clean;
    if (running == record.target && record.state == State::Verified &&
        matches(running, 0, record.appSize, record.newApp) &&
        matches(kAssets, 0, kAssetSize, record.newAssets)) return Result::NewReady;
    uint8_t oldApp[32];
    if (!io_.appIdentity(record.source, oldApp) || memcmp(oldApp, record.oldApp, 32) ||
        !matches(record.target, kBackupOffset, kAssetSize, record.oldAssets)) return Result::Error;
    if (!matches(kAssets, 0, kAssetSize, record.oldAssets)) {
      if (!io_.erase(kAssets, 0, kAssetSize) ||
          !copy(record.target, kBackupOffset, kAssets, 0) ||
          !matches(kAssets, 0, kAssetSize, record.oldAssets)) return Result::Error;
    }
    if (running != record.source) {
      // Leave the transaction pending until the old app is actually running.
      return io_.boot(record.source) ? Result::Reboot : Result::Error;
    }
    // The old app may still be executing after an attempted boot-target switch.
    // Restore that target too before making the rollback journal terminal.
    if (!io_.boot(record.source)) return Result::Error;
    record.state = State::Restored;
    return append(record) ? Result::Restored : Result::Error;
  }

  bool confirm(uint8_t running) {
    Record record; bool found;
    if (!latest(running, record, found) || !found || record.state != State::Verified ||
        !matches(running, 0, record.appSize, record.newApp) ||
        !matches(kAssets, 0, kAssetSize, record.newAssets)) return false;
    record.state = State::Complete;
    return append(record);
  }

 private:
  Io &io_;
  bool matches(uint8_t slot, uint32_t offset, size_t size, const uint8_t expected[32]) {
    uint8_t actual[32];
    return io_.hash(slot, offset, size, actual) && !memcmp(actual, expected, 32);
  }
  bool copy(uint8_t source, uint32_t sourceOffset, uint8_t target, uint32_t targetOffset) {
    uint8_t buffer[512];
    for (uint32_t offset = 0; offset < kAssetSize; offset += sizeof(buffer)) {
      if (!io_.read(source, sourceOffset + offset, buffer, sizeof(buffer)) ||
          !io_.write(target, targetOffset + offset, buffer, sizeof(buffer))) return false;
      io_.service();
    }
    return true;
  }
  bool append(Record &record) {
    for (uint32_t offset = 0; offset < kJournalSize; offset += kStride) {
      uint8_t bytes[kStride];
      if (!io_.read(record.target, kAppLimit + offset, bytes, sizeof(bytes))) return false;
      record.sequence = offset / kStride + 1;
      if (!io_.digest(&record, offsetof(Record, checksum), record.checksum)) return false;
      uint32_t marker;
      memcpy(&marker, bytes + kStride - sizeof(marker), sizeof(marker));
      if (marker == kCommit) continue;
      // Retrying the identical torn write only clears the same flash bits.
      // This avoids consuming a new slot on every interrupted restore.
      bool compatible = true;
      const auto *desired = reinterpret_cast<const uint8_t *>(&record);
      for (size_t i = 0; i < sizeof(record); ++i)
        if ((bytes[i] & desired[i]) != desired[i]) compatible = false;
      for (size_t i = sizeof(record); i < kStride - sizeof(marker); ++i)
        if (bytes[i] != 0xff) compatible = false;
      if ((marker & kCommit) != kCommit) compatible = false;
      if (!compatible) continue;
      if (!io_.write(record.target, kAppLimit + offset, &record, sizeof(record)) ||
          !io_.read(record.target, kAppLimit + offset, bytes, sizeof(record)) ||
          memcmp(bytes, &record, sizeof(record))) return false;
      if (!io_.write(record.target, kAppLimit + offset + kStride - sizeof(kCommit), &kCommit, sizeof(kCommit)) ||
          !io_.read(record.target, kAppLimit + offset + kStride - sizeof(marker), &marker, sizeof(marker)) ||
          marker != kCommit) return false;
      return true;
    }
    return false;
  }
};
} // namespace AssetRollback
