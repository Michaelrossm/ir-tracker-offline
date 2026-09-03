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
constexpr const char *kKnownAssetPaths[] = {
    "/assets/common.css.gz",      "/assets/common.js.gz",
    "/assets/i18n.js.gz",        "/assets/dashboard.js.gz",
    "/assets/history.js.gz",     "/assets/maintenance.js.gz",
    "/assets/diagnostics.js.gz", "/assets/setup.html.gz",
    "/assets/setup.js.gz"};

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

bool DebugStorage::begin(const char *firmwareVersion) {
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
    assetManifestError_ = "storage_unavailable";
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
    assetManifestError_ = "storage_unavailable";
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
       index < sizeof(kKnownAssetPaths) / sizeof(kKnownAssetPaths[0]); ++index)
    if (strcmp(relativePath, kKnownAssetPaths[index]) == 0) return index;
  return -1;
}

bool DebugStorage::verifyAsset(const char *relativePath, size_t expectedSize,
                               const char *expectedSha256) {
  if (!expectedSha256 || strlen(expectedSha256) != 64) return false;
  File file = openAsset(relativePath, "r");
  if (!file || static_cast<size_t>(file.size()) != expectedSize) {
    if (file) file.close();
    return false;
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
  return ok && difference == 0;
}

bool DebugStorage::loadAssetManifest(const char *firmwareVersion) {
  assetManifestReady_ = false;
  verifiedAssetMask_ = 0;
  if (!firmwareVersion || !*firmwareVersion) {
    assetManifestError_ = "firmware_version_missing";
    return false;
  }
  File file = openAsset(kAssetManifestPath, "r");
  if (!file) {
    assetManifestError_ = "manifest_missing";
    return false;
  }
  if (file.size() <= 0 || file.size() > 4096) {
    file.close();
    assetManifestError_ = "manifest_size_invalid";
    return false;
  }
  DynamicJsonDocument manifest(4096);
  const DeserializationError error = deserializeJson(manifest, file);
  file.close();
  if (error) {
    assetManifestError_ = "manifest_json_invalid";
    return false;
  }
  if ((manifest["schema"] | 0) != 1) {
    assetManifestError_ = "manifest_schema_incompatible";
    return false;
  }
  const char *assetsVersion = manifest["assets_version"] | "";
  if (strcmp(assetsVersion, firmwareVersion) != 0) {
    assetManifestError_ = "manifest_version_incompatible";
    return false;
  }
  JsonObject files = manifest["files"].as<JsonObject>();
  if (files.isNull() || files.size() == 0 || files.size() > 9) {
    assetManifestError_ = "manifest_files_invalid";
    return false;
  }
  for (JsonPair entry : files) {
    String path = "/assets/";
    path += entry.key().c_str();
    const int8_t index = knownAssetIndex(path.c_str());
    const size_t expectedSize = entry.value()["size"] | 0U;
    const char *expectedSha256 = entry.value()["sha256"] | "";
    if (index < 0 || !expectedSize ||
        !verifyAsset(path.c_str(), expectedSize, expectedSha256)) {
      assetManifestError_ = "manifest_asset_invalid";
      verifiedAssetMask_ = 0;
      return false;
    }
    verifiedAssetMask_ |= static_cast<uint16_t>(1U << index);
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
  return openAsset(relativePath, "r");
}
