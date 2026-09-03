#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

// DE: Kleine, zentrale Kompatibilitaetsschicht fuer die optionale 64-kB-
// Debug-Partition. Das Partitionslabel und die Dateipfade sind absichtlich
// voneinander getrennt.
// EN: Small central compatibility layer for the optional 64-kB debug
// partition. The partition label and file paths are deliberately independent.
class DebugStorage {
 public:
  enum class State : uint8_t { NotStarted, Ready, Missing, MountFailed };

  static constexpr const char *selectCompatibleLabel(bool hasDebugfs,
                                                       bool hasCoredump) {
    return hasDebugfs ? "debugfs" : (hasCoredump ? "coredump" : nullptr);
  }

  bool begin(const char *firmwareVersion = nullptr);
  bool existsAsset(const char *relativePath);
  File openAsset(const char *relativePath, const char *mode = "r");
  File openVerifiedAsset(const char *relativePath);

  bool ready() const { return state_ == State::Ready; }
  bool assetManifestReady() const { return assetManifestReady_; }
  const char *assetManifestError() const { return assetManifestError_; }
  bool usingLegacyLabel() const { return legacyLabel_; }
  State state() const { return state_; }
  const char *partitionLabel() const { return partitionLabel_; }
  const char *lastError() const { return lastError_; }

 private:
  bool safeRelativePath(const char *relativePath) const;
  String preferredPath(const char *relativePath) const;
  String legacyPath(const char *relativePath) const;
  bool loadAssetManifest(const char *firmwareVersion);
  int8_t knownAssetIndex(const char *relativePath) const;
  bool verifyAsset(const char *relativePath, size_t expectedSize,
                   const char *expectedSha256);

  fs::LittleFSFS filesystem_;
  State state_ = State::NotStarted;
  const char *partitionLabel_ = nullptr;
  const char *lastError_ = "not_started";
  bool legacyLabel_ = false;
  bool assetManifestReady_ = false;
  const char *assetManifestError_ = "not_checked";
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
