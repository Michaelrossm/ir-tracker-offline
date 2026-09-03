#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_partition.h>

// DE: Kleine, zentrale Kompatibilitaetsschicht fuer die optionale 64-kB-
// Debug-Partition. Das Partitionslabel und die Dateipfade sind absichtlich
// voneinander getrennt.
// EN: Small central compatibility layer for the optional 64-kB debug
// partition. The partition label and file paths are deliberately independent.
class DebugStorage {
 public:
  enum class State : uint8_t { NotStarted, Ready, Missing, MountFailed };
  enum class AssetValidation : uint8_t {
    Valid,
    ManifestInvalid,
    VersionMismatch,
    FileMissing,
    SizeMismatch,
    Sha256Mismatch
  };

  static constexpr const char *selectCompatibleLabel(bool hasDebugfs,
                                                       bool hasCoredump) {
    return hasDebugfs ? "debugfs" : (hasCoredump ? "coredump" : nullptr);
  }

  bool begin(const char *firmwareVersion = nullptr);
  bool existsAsset(const char *relativePath);
  File openAsset(const char *relativePath, const char *mode = "r");
  File openVerifiedAsset(const char *relativePath);
  void noteAssetServed(const char *relativePath, bool fromPartition);
  bool readRaw(size_t offset, uint8_t *buffer, size_t length) const;
  bool beginRawAssetUpdate();
  bool writeRawAsset(size_t offset, const uint8_t *data, size_t length);
  bool finishRawAssetUpdate(const char *firmwareVersion);
  bool rawVerifiedAsset(const char *relativePath, uint32_t &offset,
                        size_t &size) const;

  bool ready() const { return state_ == State::Ready; }
  bool assetManifestReady() const { return assetManifestReady_; }
  const char *assetManifestError() const { return assetManifestError_; }
  const char *assetVersion() const { return assetVersion_; }
  const char *maintenanceAssetSource() const {
    return maintenanceServedFromPartition_ ? "partition"
                                           : "embedded_fallback";
  }
  bool fixedLayoutValid() const { return fixedLayoutValid_; }
  const char *fixedLayoutError() const { return fixedLayoutError_; }
  const char *observedTargetLabel() const {
    return observedTarget_ ? observedTarget_->label : "";
  }
  uint32_t observedTargetAddress() const {
    return observedTarget_ ? observedTarget_->address : 0U;
  }
  uint32_t observedTargetSize() const {
    return observedTarget_ ? observedTarget_->size : 0U;
  }
  bool usingLegacyLabel() const { return legacyLabel_; }
  State state() const { return state_; }
  const char *partitionLabel() const { return partitionLabel_; }
  const char *lastError() const { return lastError_; }

 private:
  bool safeRelativePath(const char *relativePath) const;
  String preferredPath(const char *relativePath) const;
  String legacyPath(const char *relativePath) const;
  bool loadAssetManifest(const char *firmwareVersion);
  bool loadRawAssetManifest(const char *firmwareVersion);
  AssetValidation verifyRawAsset(uint32_t offset, size_t expectedSize,
                                 const uint8_t expectedSha256[32]);
  void inspectFixedLayout();
  int8_t knownAssetIndex(const char *relativePath) const;
  AssetValidation verifyAsset(const char *relativePath, size_t expectedSize,
                              const char *expectedSha256);
  static const char *validationError(AssetValidation result);

  fs::LittleFSFS filesystem_;
  State state_ = State::NotStarted;
  const char *partitionLabel_ = nullptr;
  const char *lastError_ = "not_started";
  bool legacyLabel_ = false;
  bool assetManifestReady_ = false;
  const char *assetManifestError_ = "not_checked";
  char assetVersion_[32] = {};
  bool maintenanceServedFromPartition_ = false;
  bool filesystemMounted_ = false;
  bool rawContainerReady_ = false;
  bool fixedLayoutValid_ = false;
  const char *fixedLayoutError_ = "not_checked";
  const esp_partition_t *observedTarget_ = nullptr;
  const esp_partition_t *assetPartition_ = nullptr;
  uint32_t rawAssetOffsets_[9] = {};
  uint32_t rawAssetSizes_[9] = {};
  uint16_t verifiedAssetMask_ = 0;
};

// DE: Kompilierbare Vertragspruefung: neues Label zuerst, altes als Rueckfall,
// Fehlen sicher abbilden. EN: Compile-time contract tests: prefer the new
// label, retain the legacy fallback, and represent absence safely.
static_assert(DebugStorage::selectCompatibleLabel(true, true)[0] == 'd',
              "debugfs must be preferred");
static_assert(DebugStorage::selectCompatibleLabel(false, true)[0] == 'c',
              "legacy coredump must remain supported");
static_assert(DebugStorage::selectCompatibleLabel(false, false) == nullptr,
              "missing debug storage must be safe");
