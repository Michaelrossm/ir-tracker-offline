#include "DebugStorage.h"

#include <esp_partition.h>

#include <algorithm>
#include <cstring>

namespace {
constexpr char kMountPoint[] = "/debug";
constexpr char kPreferredDirectory[] = "/debug";
constexpr char kLegacyDirectory[] = "/coredump";

const esp_partition_t *findPartition(const char *label) {
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                  ESP_PARTITION_SUBTYPE_DATA_SPIFFS, label);
}

bool partitionIsBlank(const esp_partition_t *partition) {
  if (!partition) return false;
  uint8_t buffer[256];
  for (size_t offset = 0; offset < partition->size; offset += sizeof(buffer)) {
    const size_t length = std::min<size_t>(
        sizeof(buffer), static_cast<size_t>(partition->size) - offset);
    if (esp_partition_read(partition, offset, buffer, length) != ESP_OK)
      return false;
    for (size_t index = 0; index < length; ++index)
      if (buffer[index] != 0xff) return false;
  }
  return true;
}
}  // namespace

bool DebugStorage::begin() {
  if (ready()) return true;

  const esp_partition_t *debugPartition = findPartition("debugfs");
  const esp_partition_t *legacyPartition = findPartition("coredump");
  const bool hasDebugfs = debugPartition != nullptr;
  const bool hasCoredump = legacyPartition != nullptr;
  partitionLabel_ = selectCompatibleLabel(hasDebugfs, hasCoredump);
  legacyLabel_ = partitionLabel_ && strcmp(partitionLabel_, "coredump") == 0;

  if (!partitionLabel_) {
    state_ = State::Missing;
    lastError_ = "compatible_partition_missing";
    Serial.println(
        "Debug storage unavailable: neither debugfs nor legacy coredump "
        "partition exists");
    return false;
  }

  const esp_partition_t *partition =
      hasDebugfs ? debugPartition : legacyPartition;
  bool mounted = filesystem_.begin(false, kMountPoint, 5, partitionLabel_);
  // DE: Nur vollstaendig leerer Erstflash-Speicher darf initialisiert werden;
  // nichtleere Bestandsdaten bleiben unangetastet. EN: Only completely erased
  // first-flash storage may be initialized; non-empty existing data stays intact.
  const bool blank = !mounted && partitionIsBlank(partition);
  if (!mounted && blank)
    mounted = filesystem_.begin(true, kMountPoint, 5, partitionLabel_);
  if (!mounted) {
    state_ = State::MountFailed;
    lastError_ = blank ? "blank_partition_initialization_failed"
                       : "existing_partition_not_littlefs_preserved";
    Serial.printf("Debug storage mount failed without erasing data: label=%s, "
                  "reason=%s\n",
                  partitionLabel_, lastError_);
    return false;
  }

  state_ = State::Ready;
  lastError_ = "";
  Serial.printf("Debug storage ready: label=%s%s, mount=%s\n",
                partitionLabel_, legacyLabel_ ? " (legacy fallback)" : "",
                kMountPoint);
  return true;
}

bool DebugStorage::safeRelativePath(const char *relativePath) const {
  return relativePath && relativePath[0] == '/' &&
         strstr(relativePath, "..") == nullptr;
}

String DebugStorage::preferredPath(const char *relativePath) const {
  return String(kPreferredDirectory) + relativePath;
}

String DebugStorage::legacyPath(const char *relativePath) const {
  return String(kLegacyDirectory) + relativePath;
}

bool DebugStorage::existsAsset(const char *relativePath) {
  if (!ready() || !safeRelativePath(relativePath)) return false;
  return filesystem_.exists(preferredPath(relativePath)) ||
         filesystem_.exists(legacyPath(relativePath)) ||
         filesystem_.exists(relativePath);
}

File DebugStorage::openAsset(const char *relativePath, const char *mode) {
  if (!ready() || !safeRelativePath(relativePath)) return File();
  const String preferred = preferredPath(relativePath);
  if (filesystem_.exists(preferred)) return filesystem_.open(preferred, mode);
  const String legacy = legacyPath(relativePath);
  if (filesystem_.exists(legacy)) return filesystem_.open(legacy, mode);
  return filesystem_.open(relativePath, mode);
}
