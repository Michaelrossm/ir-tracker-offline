#include "DebugStorage.h"

#include <ArduinoJson.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>

#include <algorithm>
#include <cstring>

namespace {
constexpr char kMountPoint[] = "/debug";
constexpr char kPreferredDirectory[] = "/debug";
constexpr char kLegacyDirectory[] = "/coredump";
constexpr char kAssetManifestPath[] = "/assets/manifest.json";
constexpr char kMaintenanceAssetPath[] = "/assets/maintenance.js.gz";
constexpr uint32_t kAssetAddress = 0x2B0000U;
constexpr size_t kAssetSize = 0x10000U;
constexpr uint16_t kRawAssetSchema = 1;
constexpr uint16_t kRawAssetHeaderSize = 1024;
constexpr char kRawAssetMagic[8] = {'I', 'R', 'A', 'S', 'S', 'E', 'T', '1'};
constexpr const char *kKnownAssetPaths[] = {
    "/assets/common.css.gz",      "/assets/common.js.gz",
    "/assets/i18n.js.gz",        "/assets/dashboard.js.gz",
    "/assets/history.js.gz",     "/assets/maintenance.js.gz",
    "/assets/diagnostics.js.gz", "/assets/setup.html.gz",
    "/assets/setup.js.gz"};
constexpr uint8_t kKnownAssetCount =
    sizeof(kKnownAssetPaths) / sizeof(kKnownAssetPaths[0]);
constexpr uint16_t kAllKnownAssetsMask =
    static_cast<uint16_t>((1U << kKnownAssetCount) - 1U);

struct __attribute__((packed)) RawAssetEntry {
  char name[32];
  uint32_t offset;
  uint32_t size;
  uint8_t sha256[32];
};

struct __attribute__((packed)) RawAssetHeader {
  char magic[8];
  uint16_t schema;
  uint16_t headerSize;
  uint32_t imageSize;
  char version[32];
  uint16_t fileCount;
  uint16_t reserved;
  RawAssetEntry files[9];
};

static_assert(sizeof(RawAssetHeader) <= kRawAssetHeaderSize,
              "raw asset header must fit reserved header area");

const esp_partition_t *findPartition(const char *label) {
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                  ESP_PARTITION_SUBTYPE_DATA_SPIFFS, label);
}

const esp_partition_t *findPartitionAt(uint32_t address) {
  esp_partition_iterator_t iterator = esp_partition_find(
      ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
  const esp_partition_t *found = nullptr;
  while (iterator) {
    const esp_partition_t *partition = esp_partition_get(iterator);
    if (partition && partition->address == address) {
      found = partition;
      break;
    }
    iterator = esp_partition_next(iterator);
  }
  if (iterator) esp_partition_iterator_release(iterator);
  return found;
}

bool exactPartition(const esp_partition_t *partition, uint32_t address,
                    size_t size) {
  return partition && partition->address == address && partition->size == size;
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

void DebugStorage::inspectFixedLayout() {
  fixedLayoutValid_ = false;
  fixedLayoutError_ = "partition_missing";
  observedTarget_ = findPartitionAt(kAssetAddress);
  if (!observedTarget_) return;
  if (observedTarget_->type != ESP_PARTITION_TYPE_DATA ||
      (strcmp(observedTarget_->label, "debugfs") != 0 &&
       strcmp(observedTarget_->label, "coredump") != 0)) {
    fixedLayoutError_ = "asset_label_invalid";
    return;
  }
  assetPartition_ = observedTarget_;
  if (!exactPartition(assetPartition_, kAssetAddress, kAssetSize)) {
    fixedLayoutError_ = "asset_geometry_mismatch";
    return;
  }
  const esp_partition_t *ota0 = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
  const esp_partition_t *ota1 = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
  const esp_partition_t *history = findPartition("history");
  if (!exactPartition(ota0, 0x10000U, 0x150000U) ||
      !exactPartition(ota1, 0x160000U, 0x150000U) ||
      !exactPartition(history, 0x2C0000U, 0x140000U)) {
    fixedLayoutError_ = "fixed_layout_mismatch";
    return;
  }
  fixedLayoutValid_ = true;
  fixedLayoutError_ = "";
}

bool DebugStorage::begin(const char *firmwareVersion) {
  if (ready()) return true;

  inspectFixedLayout();

  const esp_partition_t *debugPartition =
      observedTarget_ && strcmp(observedTarget_->label, "debugfs") == 0
          ? observedTarget_
          : nullptr;
  const esp_partition_t *legacyPartition =
      observedTarget_ && strcmp(observedTarget_->label, "coredump") == 0
          ? observedTarget_
          : nullptr;
  const bool hasDebugfs = debugPartition != nullptr;
  const bool hasCoredump = legacyPartition != nullptr;
  partitionLabel_ = selectCompatibleLabel(hasDebugfs, hasCoredump);
  legacyLabel_ = partitionLabel_ && strcmp(partitionLabel_, "coredump") == 0;

  if (!partitionLabel_) {
    state_ = State::Missing;
    lastError_ = "compatible_partition_missing";
    assetManifestError_ = "partition_missing";
    Serial.println(
        "Debug storage unavailable: neither debugfs nor legacy coredump "
        "partition exists");
    return false;
  }

  const esp_partition_t *partition =
      hasDebugfs ? debugPartition : legacyPartition;
  bool mounted = false;
  if (partition->subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS)
    mounted = filesystem_.begin(false, kMountPoint, 5, partitionLabel_);
  // DE: Nur vollstaendig leerer Erstflash-Speicher darf initialisiert werden;
  // nichtleere Bestandsdaten bleiben unangetastet. EN: Only completely erased
  // first-flash storage may be initialized; non-empty existing data stays intact.
  const bool blank = !mounted && partitionIsBlank(partition);
  if (!mounted && blank &&
      partition->subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS)
    mounted = filesystem_.begin(true, kMountPoint, 5, partitionLabel_);
  filesystemMounted_ = mounted;
  if (!mounted && loadRawAssetManifest(firmwareVersion)) {
    state_ = State::Ready;
    lastError_ = "";
    Serial.printf("Raw asset storage ready: label=%s%s\n", partitionLabel_,
                  legacyLabel_ ? " (legacy fallback)" : "");
    return true;
  }
  if (!mounted) {
    state_ = State::MountFailed;
    lastError_ = blank ? "blank_partition_initialization_failed"
                       : "existing_partition_not_littlefs_preserved";
    // A legacy coredump subtype cannot be mounted as LittleFS. In that case
    // preserve the more precise raw-container validation error. A true SPIFFS
    // mount failure remains distinguishable as mount_failed.
    if (partition->subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS)
      assetManifestError_ = "mount_failed";
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
  loadAssetManifest(firmwareVersion);
  return true;
}

bool DebugStorage::readRaw(size_t offset, uint8_t *buffer,
                           size_t length) const {
  return fixedLayoutValid_ && assetPartition_ && buffer &&
         offset <= kAssetSize && length <= kAssetSize - offset &&
         esp_partition_read(assetPartition_, offset, buffer, length) == ESP_OK;
}

bool DebugStorage::beginRawAssetUpdate() {
  inspectFixedLayout();
  if (!fixedLayoutValid_ || !assetPartition_) return false;
  if (filesystemMounted_) filesystem_.end();
  filesystemMounted_ = false;
  rawContainerReady_ = false;
  state_ = State::NotStarted;
  assetManifestReady_ = false;
  maintenanceServedFromPartition_ = false;
  verifiedAssetMask_ = 0;
  return esp_partition_erase_range(assetPartition_, 0, kAssetSize) == ESP_OK;
}

bool DebugStorage::writeRawAsset(size_t offset, const uint8_t *data,
                                 size_t length) {
  return fixedLayoutValid_ && assetPartition_ && data &&
         offset <= kAssetSize && length <= kAssetSize - offset &&
         esp_partition_write(assetPartition_, offset, data, length) == ESP_OK;
}

bool DebugStorage::finishRawAssetUpdate(const char *firmwareVersion) {
  state_ = State::NotStarted;
  partitionLabel_ = nullptr;
  legacyLabel_ = false;
  return begin(firmwareVersion) && assetManifestReady_;
}

DebugStorage::AssetValidation DebugStorage::verifyRawAsset(
    uint32_t offset, size_t expectedSize, const uint8_t expectedSha256[32]) {
  if (!assetPartition_ || offset < kRawAssetHeaderSize || !expectedSize ||
      offset > kAssetSize || expectedSize > kAssetSize - offset)
    return AssetValidation::ManifestInvalid;
  mbedtls_sha256_context context;
  mbedtls_sha256_init(&context);
  bool ok = mbedtls_sha256_starts_ret(&context, 0) == 0;
  uint8_t buffer[256];
  size_t position = 0;
  while (ok && position < expectedSize) {
    const size_t count = std::min(sizeof(buffer), expectedSize - position);
    if (esp_partition_read(assetPartition_, offset + position, buffer, count) !=
            ESP_OK ||
        mbedtls_sha256_update_ret(&context, buffer, count) != 0) {
      ok = false;
      break;
    }
    position += count;
  }
  uint8_t digest[32] = {};
  if (ok) ok = mbedtls_sha256_finish_ret(&context, digest) == 0;
  mbedtls_sha256_free(&context);
  uint8_t difference = 0;
  for (uint8_t index = 0; index < sizeof(digest); ++index)
    difference |= digest[index] ^ expectedSha256[index];
  return ok && difference == 0 ? AssetValidation::Valid
                               : AssetValidation::Sha256Mismatch;
}

bool DebugStorage::loadRawAssetManifest(const char *firmwareVersion) {
  rawContainerReady_ = false;
  assetManifestReady_ = false;
  verifiedAssetMask_ = 0;
  memset(rawAssetOffsets_, 0, sizeof(rawAssetOffsets_));
  memset(rawAssetSizes_, 0, sizeof(rawAssetSizes_));
  if (!fixedLayoutValid_ || !assetPartition_ || !firmwareVersion)
    return false;
  RawAssetHeader header = {};
  if (esp_partition_read(assetPartition_, 0, &header, sizeof(header)) != ESP_OK ||
      memcmp(header.magic, kRawAssetMagic, sizeof(kRawAssetMagic)) != 0 ||
      header.schema != kRawAssetSchema ||
      header.headerSize != kRawAssetHeaderSize ||
      header.imageSize != kAssetSize || !header.fileCount ||
      header.fileCount != kKnownAssetCount ||
      !memchr(header.version, '\0', sizeof(header.version))) {
    assetManifestError_ = "manifest_invalid";
    return false;
  }
  strncpy(assetVersion_, header.version, sizeof(assetVersion_) - 1U);
  assetVersion_[sizeof(assetVersion_) - 1U] = '\0';
  if (strcmp(assetVersion_, firmwareVersion) != 0) {
    assetManifestError_ = "version_mismatch";
    return false;
  }
  for (uint8_t fileIndex = 0; fileIndex < header.fileCount; ++fileIndex) {
    const RawAssetEntry &entry = header.files[fileIndex];
    if (!memchr(entry.name, '\0', sizeof(entry.name))) {
      assetManifestError_ = "manifest_invalid";
      return false;
    }
    String path = "/assets/";
    path += entry.name;
    const int8_t knownIndex = knownAssetIndex(path.c_str());
    if (knownIndex < 0 ||
        (verifiedAssetMask_ & static_cast<uint16_t>(1U << knownIndex))) {
      assetManifestError_ = "manifest_invalid";
      return false;
    }
    const AssetValidation validation =
        verifyRawAsset(entry.offset, entry.size, entry.sha256);
    if (validation != AssetValidation::Valid) {
      assetManifestError_ = validationError(validation);
      return false;
    }
    rawAssetOffsets_[knownIndex] = entry.offset;
    rawAssetSizes_[knownIndex] = entry.size;
    verifiedAssetMask_ |= static_cast<uint16_t>(1U << knownIndex);
  }
  if (verifiedAssetMask_ != kAllKnownAssetsMask) {
    assetManifestError_ = "file_missing";
    verifiedAssetMask_ = 0;
    return false;
  }
  rawContainerReady_ = true;
  assetManifestReady_ = true;
  assetManifestError_ = "";
  return true;
}

bool DebugStorage::rawVerifiedAsset(const char *relativePath, uint32_t &offset,
                                    size_t &size) const {
  if (!rawContainerReady_ || !assetManifestReady_) return false;
  const int8_t index = knownAssetIndex(relativePath);
  if (index < 0 || !(verifiedAssetMask_ & static_cast<uint16_t>(1U << index)))
    return false;
  offset = rawAssetOffsets_[index];
  size = rawAssetSizes_[index];
  return size > 0;
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

int8_t DebugStorage::knownAssetIndex(const char *relativePath) const {
  if (!relativePath) return -1;
  for (uint8_t index = 0;
       index < kKnownAssetCount; ++index)
    if (strcmp(relativePath, kKnownAssetPaths[index]) == 0) return index;
  return -1;
}

const char *DebugStorage::validationError(AssetValidation result) {
  switch (result) {
    case AssetValidation::Valid: return "";
    case AssetValidation::ManifestInvalid: return "manifest_invalid";
    case AssetValidation::VersionMismatch: return "version_mismatch";
    case AssetValidation::FileMissing: return "file_missing";
    case AssetValidation::SizeMismatch: return "size_mismatch";
    case AssetValidation::Sha256Mismatch: return "sha256_mismatch";
    default: return "manifest_invalid";
  }
}

DebugStorage::AssetValidation DebugStorage::verifyAsset(
    const char *relativePath, size_t expectedSize,
    const char *expectedSha256) {
  if (!expectedSha256 || strlen(expectedSha256) != 64 || !expectedSize)
    return AssetValidation::ManifestInvalid;
  File file = openAsset(relativePath, "r");
  if (!file) return AssetValidation::FileMissing;
  if (static_cast<size_t>(file.size()) != expectedSize) {
    file.close();
    return AssetValidation::SizeMismatch;
  }
  mbedtls_sha256_context context;
  mbedtls_sha256_init(&context);
  bool ok = mbedtls_sha256_starts_ret(&context, 0) == 0;
  uint8_t buffer[256];
  while (ok && file.available()) {
    const size_t count = file.read(buffer, sizeof(buffer));
    if (!count || mbedtls_sha256_update_ret(&context, buffer, count) != 0)
      ok = false;
  }
  uint8_t digest[32] = {};
  if (ok) ok = mbedtls_sha256_finish_ret(&context, digest) == 0;
  mbedtls_sha256_free(&context);
  file.close();
  static const char hex[] = "0123456789abcdef";
  uint8_t difference = 0;
  for (uint8_t index = 0; ok && index < sizeof(digest); ++index) {
    const char high = hex[digest[index] >> 4U];
    const char low = hex[digest[index] & 0x0fU];
    difference |= static_cast<uint8_t>(high ^ expectedSha256[index * 2U]);
    difference |= static_cast<uint8_t>(low ^ expectedSha256[index * 2U + 1U]);
  }
  return ok && difference == 0 ? AssetValidation::Valid
                               : AssetValidation::Sha256Mismatch;
}

bool DebugStorage::loadAssetManifest(const char *firmwareVersion) {
  assetManifestReady_ = false;
  assetVersion_[0] = '\0';
  maintenanceServedFromPartition_ = false;
  verifiedAssetMask_ = 0;
  if (!firmwareVersion || !*firmwareVersion) {
    assetManifestError_ = "manifest_invalid";
    return false;
  }
  File file = openAsset(kAssetManifestPath, "r");
  if (!file) {
    assetManifestError_ = "manifest_invalid";
    return false;
  }
  if (file.size() <= 0 || file.size() > 4096) {
    file.close();
    assetManifestError_ = "manifest_invalid";
    return false;
  }
  DynamicJsonDocument manifest(4096);
  const DeserializationError error = deserializeJson(manifest, file);
  file.close();
  if (error) {
    assetManifestError_ = "manifest_invalid";
    return false;
  }
  if ((manifest["schema"] | 0) != 1) {
    assetManifestError_ = "manifest_invalid";
    return false;
  }
  const char *assetsVersion = manifest["assets_version"] | "";
  strncpy(assetVersion_, assetsVersion, sizeof(assetVersion_) - 1U);
  assetVersion_[sizeof(assetVersion_) - 1U] = '\0';
  if (strcmp(assetsVersion, firmwareVersion) != 0) {
    assetManifestError_ = "version_mismatch";
    return false;
  }
  JsonObject files = manifest["files"].as<JsonObject>();
  if (files.isNull() || files.size() != kKnownAssetCount) {
    assetManifestError_ = "manifest_invalid";
    return false;
  }
  for (JsonPair entry : files) {
    String path = "/assets/";
    path += entry.key().c_str();
    const int8_t index = knownAssetIndex(path.c_str());
    const size_t expectedSize = entry.value()["size"] | 0U;
    const char *expectedSha256 = entry.value()["sha256"] | "";
    if (index < 0 ||
        (verifiedAssetMask_ & static_cast<uint16_t>(1U << index))) {
      assetManifestError_ = "manifest_invalid";
      verifiedAssetMask_ = 0;
      return false;
    }
    const AssetValidation validation =
        verifyAsset(path.c_str(), expectedSize, expectedSha256);
    if (validation != AssetValidation::Valid) {
      assetManifestError_ = validationError(validation);
      verifiedAssetMask_ = 0;
      return false;
    }
    verifiedAssetMask_ |= static_cast<uint16_t>(1U << index);
  }
  if (verifiedAssetMask_ != kAllKnownAssetsMask) {
    assetManifestError_ = "file_missing";
    verifiedAssetMask_ = 0;
    return false;
  }
  assetManifestReady_ = true;
  assetManifestError_ = "";
  return true;
}

File DebugStorage::openVerifiedAsset(const char *relativePath) {
  if (!assetManifestReady_) return File();
  const int8_t index = knownAssetIndex(relativePath);
  if (index < 0 || !(verifiedAssetMask_ & static_cast<uint16_t>(1U << index)))
    return File();
  File file = openAsset(relativePath, "r");
  if (!file) {
    assetManifestReady_ = false;
    verifiedAssetMask_ = 0;
    assetManifestError_ = "file_missing";
  }
  return file;
}

void DebugStorage::noteAssetServed(const char *relativePath,
                                   bool fromPartition) {
  if (relativePath && strcmp(relativePath, kMaintenanceAssetPath) == 0)
    maintenanceServedFromPartition_ = fromPartition;
}
