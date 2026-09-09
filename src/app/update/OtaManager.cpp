// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

bool otaRequestAuthorized() {
  return requireAdmin();
}

struct AssetRawUploadState {
  bool authorized = false;
  bool ok = false;
  bool shaStarted = false;
  size_t written = 0;
  String expectedSha256;
  String error;
  mbedtls_sha256_context sha;
} assetRawUpload;

void resetAssetRawUpload() {
  if (assetRawUpload.shaStarted)
    mbedtls_sha256_free(&assetRawUpload.sha);
  assetRawUpload = AssetRawUploadState{};
  mbedtls_sha256_init(&assetRawUpload.sha);
}

bool beginAssetRawUpload(const String &expectedSha256) {
  assetRawUpload.expectedSha256 = expectedSha256;
  assetRawUpload.expectedSha256.toLowerCase();
  if (assetRawUpload.expectedSha256.length() != 64 ||
      !debugStorage.fixedLayoutValid()) {
    assetRawUpload.error = !debugStorage.fixedLayoutValid()
                                ? debugStorage.fixedLayoutError()
                                : "asset_sha256_missing";
    return false;
  }
  requestCpuBoost("asset_update");
  assetRawUpload.ok = debugStorage.beginRawAssetUpdate() &&
                      mbedtls_sha256_starts_ret(&assetRawUpload.sha, 0) == 0;
  assetRawUpload.shaStarted = assetRawUpload.ok;
  if (!assetRawUpload.ok) assetRawUpload.error = "asset_erase_failed";
  return assetRawUpload.ok;
}

bool writeAssetRawUpload(const uint8_t *data, size_t length) {
  if (!assetRawUpload.ok || assetRawUpload.written > 0x10000U ||
      length > 0x10000U - assetRawUpload.written ||
      mbedtls_sha256_update_ret(&assetRawUpload.sha, data, length) != 0 ||
      !debugStorage.writeRawAsset(assetRawUpload.written, data, length)) {
    assetRawUpload.ok = false;
    assetRawUpload.error = "asset_write_failed";
    return false;
  }
  assetRawUpload.written += length;
  return true;
}

bool finishAssetRawUpload() {
  uint8_t digest[32] = {};
  const bool complete = assetRawUpload.written == 0x10000U;
  const bool digestOk =
      complete && mbedtls_sha256_finish_ret(&assetRawUpload.sha, digest) == 0 &&
      constantTimeEqual(hexBytes(digest, sizeof(digest)),
                        assetRawUpload.expectedSha256);
  memset(digest, 0, sizeof(digest));
  assetRawUpload.ok = digestOk;
  if (!complete)
    assetRawUpload.error = "asset_size_mismatch";
  else if (!digestOk)
    assetRawUpload.error = "asset_sha256_mismatch";
  else if (!debugStorage.finishRawAssetUpdate(kFirmwareVersion)) {
    assetRawUpload.ok = false;
    assetRawUpload.error = debugStorage.assetManifestError();
  }
  if (assetRawUpload.ok) assetBackupServed = false;
  return assetRawUpload.ok;
}

String assetPartitionLayoutJson() {
  String json;
  json.reserve(280);
  json = "{\"valid\":" +
         String(debugStorage.fixedLayoutValid() ? "true" : "false") +
         ",\"error\":\"" + jsonEscape(debugStorage.fixedLayoutError()) +
         "\",\"label\":\"" +
         jsonEscape(debugStorage.observedTargetLabel()) +
         "\",\"offset\":" +
         String(debugStorage.observedTargetAddress()) +
         ",\"size\":" + String(debugStorage.observedTargetSize()) +
         ",\"required_offset\":2818048,\"required_size\":65536,"
         "\"history_offset\":2883584,\"history_size\":1310720,"
         "\"ota_0_offset\":65536,\"ota_1_offset\":1441792}";
  return json;
}

void handleAssetPartitionBackup() {
  if (!requireAdmin()) return;
  if (!debugStorage.fixedLayoutValid()) {
    server.send(409, "application/json", assetPartitionLayoutJson());
    return;
  }
  requestCpuBoost("asset_backup");
  server.sendHeader("Content-Disposition",
                    "attachment; filename=ir-tracker-debugfs-backup.bin");
  server.setContentLength(0x10000U);
  server.send(200, "application/octet-stream", "");
  uint8_t buffer[512];
  for (size_t offset = 0; offset < 0x10000U; offset += sizeof(buffer)) {
    if (!debugStorage.readRaw(offset, buffer, sizeof(buffer)) ||
        server.client().write(buffer, sizeof(buffer)) != sizeof(buffer))
      return;
    delay(0);
  }
  assetBackupServed = true;
}

void handleAssetPartitionUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    resetAssetRawUpload();
    assetRawUpload.authorized = otaRequestAuthorized();
    assetRawUpload.ok = assetRawUpload.authorized &&
                        server.header("X-Asset-Confirm") ==
                            "BACKUP-VERIFIED-0x2B0000-0x10000" &&
                        debugStorage.fixedLayoutValid();
    if (!assetRawUpload.ok) {
      assetRawUpload.error = !assetRawUpload.authorized
                                 ? "unauthorized"
                                 : !debugStorage.fixedLayoutValid()
                                       ? debugStorage.fixedLayoutError()
                                       : "backup_or_sha_confirmation_missing";
      return;
    }
    assetRawUpload.ok = beginAssetRawUpload(server.header("X-Asset-SHA256"));
  } else if (upload.status == UPLOAD_FILE_WRITE && assetRawUpload.ok) {
    writeAssetRawUpload(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END && assetRawUpload.ok) {
    finishAssetRawUpload();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    assetRawUpload.ok = false;
    assetRawUpload.error = "asset_upload_aborted";
  }
}

void handleAssetPartitionUploadFinished() {
  if (!assetRawUpload.authorized) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  if (!assetRawUpload.ok) {
    if (assetRawUpload.written)
      debugStorage.finishRawAssetUpdate(kFirmwareVersion);
    server.send(400, "application/json",
                "{\"error\":\"" + jsonEscape(assetRawUpload.error) +
                    "\",\"layout\":" + assetPartitionLayoutJson() + "}");
    return;
  }
  eventLog.add("INFO", "ASSET_UPDATE",
               "Webasset-Image ueber WLAN geprueft und installiert");
  server.send(200, "application/json",
              "{\"ok\":true,\"bytes\":65536,\"sha256\":\"" +
              assetRawUpload.expectedSha256 + "\"}");
}

void resetSignedOta();
bool consumeSignedOta(const uint8_t *data, size_t length);
bool verifySignedOta(const char *expectedSha256);
bool commitVerifiedOta();

constexpr size_t kCombinedPlanMaximumBytes = 1536;
// IRUP200 is one signed stream: header, signature, manifest, app and 64 KiB
// assets.  The inactive app slot is committed only after both payload hashes
// have been checked successfully.
struct CombinedBundleUploadState {
  bool authorized = false;
  bool ok = false;
  bool manifestVerified = false;
  uint8_t header[16] = {};
  size_t headerRead = 0;
  uint8_t signature[80] = {};
  size_t signatureRead = 0;
  uint8_t manifest[kCombinedPlanMaximumBytes] = {};
  size_t manifestRead = 0;
  uint32_t manifestSize = 0;
  uint16_t signatureSize = 0;
  size_t firmwareWritten = 0;
  size_t assetsWritten = 0;
  bool firmwareShaStarted = false;
  mbedtls_sha256_context firmwareSha;
  String error;
} combinedBundleUpload;

void resetCombinedBundleUpload() {
  if (combinedBundleUpload.firmwareShaStarted)
    mbedtls_sha256_free(&combinedBundleUpload.firmwareSha);
  combinedBundleUpload = CombinedBundleUploadState{};
  mbedtls_sha256_init(&combinedBundleUpload.firmwareSha);
}

bool validUpdateSha256(const char *value) {
  if (!value || strlen(value) != 64) return false;
  for (uint8_t index = 0; index < 64; ++index)
    if (!isxdigit(static_cast<unsigned char>(value[index]))) return false;
  return true;
}

bool verifySignedBundleManifest(const uint8_t *signature, size_t signatureSize,
                                const uint8_t *manifest, size_t manifestSize,
                                String &error) {
  uint8_t digest[32] = {};
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  const bool hashOk = mbedtls_sha256_starts_ret(&sha, 0) == 0 &&
      mbedtls_sha256_update_ret(&sha, manifest, manifestSize) == 0 &&
      mbedtls_sha256_finish_ret(&sha, digest) == 0;
  mbedtls_sha256_free(&sha);
  mbedtls_pk_context key;
  mbedtls_pk_init(&key);
  const int parsed = mbedtls_pk_parse_public_key(
      &key, reinterpret_cast<const unsigned char *>(kFirmwareSigningPublicKey),
      strlen(kFirmwareSigningPublicKey) + 1);
  const int verified = hashOk && parsed == 0
      ? mbedtls_pk_verify(&key, MBEDTLS_MD_SHA256, digest, sizeof(digest), signature, signatureSize)
      : -1;
  mbedtls_pk_free(&key);
  memset(digest, 0, sizeof(digest));
  if (verified != 0) { error = "update_bundle_signature_invalid"; return false; }
  StaticJsonDocument<768> document;
  if (deserializeJson(document, manifest, manifestSize)) { error = "update_bundle_json_invalid"; return false; }
  const char *version = document["version"] | "";
  const char *firmwareSha = document["firmware"]["sha256"] | "";
  const char *assetsSha = document["assets"]["sha256"] | "";
  const uint32_t firmwareSize = document["firmware"]["size"] | 0U;
  const uint32_t assetsSize = document["assets"]["size"] | 0U;
  if ((document["schema"] | 0) != 2 || !version[0] ||
      strlen(version) >= sizeof(combinedUpdate.version) ||
      !validUpdateSha256(firmwareSha) || !validUpdateSha256(assetsSha) ||
      firmwareSize < 1024 || firmwareSize > 0x150000U || assetsSize != 0x10000U) {
    error = "update_bundle_content_invalid"; return false;
  }
  combinedUpdate = CombinedUpdatePlan{};
  strlcpy(combinedUpdate.version, version, sizeof(combinedUpdate.version));
  strlcpy(combinedUpdate.firmwareSha256, firmwareSha, sizeof(combinedUpdate.firmwareSha256));
  strlcpy(combinedUpdate.assetsSha256, assetsSha, sizeof(combinedUpdate.assetsSha256));
  combinedUpdate.firmwareSize = firmwareSize;
  combinedUpdate.assetsSize = assetsSize;
  combinedUpdate.valid = true;
  return true;
}

bool beginCombinedBundlePayload(String &error) {
  if (!debugStorage.fixedLayoutValid()) { error = debugStorage.fixedLayoutError(); return false; }
  if (!Update.begin(combinedUpdate.firmwareSize, U_FLASH)) { error = "update_partition_unavailable"; return false; }
  if (mbedtls_sha256_starts_ret(&combinedBundleUpload.firmwareSha, 0) != 0) {
    Update.abort(); error = "sha256_initialization_failed"; return false;
  }
  combinedBundleUpload.firmwareShaStarted = true;
  return true;
}

bool finishCombinedBundleFirmware(String &error) {
  uint8_t digest[32] = {};
  const bool valid = mbedtls_sha256_finish_ret(&combinedBundleUpload.firmwareSha, digest) == 0 &&
      constantTimeEqual(hexBytes(digest, sizeof(digest)), combinedUpdate.firmwareSha256);
  memset(digest, 0, sizeof(digest));
  if (!valid) { error = "firmware_sha256_mismatch"; return false; }
  combinedUpdate.firmwareStaged = true;
  resetAssetRawUpload();
  if (!beginAssetRawUpload(combinedUpdate.assetsSha256)) { error = assetRawUpload.error; return false; }
  return true;
}

bool consumeCombinedBundle(const uint8_t *data, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    if (combinedBundleUpload.headerRead < sizeof(combinedBundleUpload.header)) {
      const size_t count = std::min(length - offset, sizeof(combinedBundleUpload.header) - combinedBundleUpload.headerRead);
      memcpy(combinedBundleUpload.header + combinedBundleUpload.headerRead, data + offset, count);
      combinedBundleUpload.headerRead += count; offset += count;
      if (combinedBundleUpload.headerRead == sizeof(combinedBundleUpload.header)) {
        static const uint8_t magic[8] = {'I','R','U','P','2','0','0',0};
        const uint8_t *h = combinedBundleUpload.header;
        combinedBundleUpload.manifestSize = static_cast<uint32_t>(h[8]) | (static_cast<uint32_t>(h[9]) << 8) | (static_cast<uint32_t>(h[10]) << 16) | (static_cast<uint32_t>(h[11]) << 24);
        combinedBundleUpload.signatureSize = static_cast<uint16_t>(h[12]) | (static_cast<uint16_t>(h[13]) << 8);
        if (memcmp(h, magic, sizeof(magic)) || h[14] || h[15] || !combinedBundleUpload.manifestSize ||
            combinedBundleUpload.manifestSize > kCombinedPlanMaximumBytes || combinedBundleUpload.signatureSize < 64 ||
            combinedBundleUpload.signatureSize > sizeof(combinedBundleUpload.signature)) { combinedBundleUpload.error = "update_bundle_header_invalid"; return false; }
      }
      continue;
    }
    if (combinedBundleUpload.signatureRead < combinedBundleUpload.signatureSize) {
      const size_t count = std::min(length - offset, static_cast<size_t>(combinedBundleUpload.signatureSize) - combinedBundleUpload.signatureRead);
      memcpy(combinedBundleUpload.signature + combinedBundleUpload.signatureRead, data + offset, count);
      combinedBundleUpload.signatureRead += count; offset += count; continue;
    }
    if (combinedBundleUpload.manifestRead < combinedBundleUpload.manifestSize) {
      const size_t count = std::min(length - offset, static_cast<size_t>(combinedBundleUpload.manifestSize) - combinedBundleUpload.manifestRead);
      memcpy(combinedBundleUpload.manifest + combinedBundleUpload.manifestRead, data + offset, count);
      combinedBundleUpload.manifestRead += count; offset += count;
      if (combinedBundleUpload.manifestRead == combinedBundleUpload.manifestSize) {
        if (!verifySignedBundleManifest(combinedBundleUpload.signature, combinedBundleUpload.signatureSize,
                                        combinedBundleUpload.manifest, combinedBundleUpload.manifestSize,
                                        combinedBundleUpload.error) || !beginCombinedBundlePayload(combinedBundleUpload.error)) return false;
        combinedBundleUpload.manifestVerified = true;
      }
      continue;
    }
    if (combinedBundleUpload.firmwareWritten < combinedUpdate.firmwareSize) {
      const size_t count = std::min(length - offset, static_cast<size_t>(combinedUpdate.firmwareSize) - combinedBundleUpload.firmwareWritten);
      if (!combinedBundleUpload.firmwareWritten && data[offset] != 0xE9) { combinedBundleUpload.error = "not_an_esp32_application"; return false; }
      if (mbedtls_sha256_update_ret(&combinedBundleUpload.firmwareSha, data + offset, count) != 0 ||
          Update.write(const_cast<uint8_t *>(data + offset), count) != count) { combinedBundleUpload.error = "firmware_write_failed"; return false; }
      combinedBundleUpload.firmwareWritten += count; offset += count;
      if (combinedBundleUpload.firmwareWritten == combinedUpdate.firmwareSize && !finishCombinedBundleFirmware(combinedBundleUpload.error)) return false;
      continue;
    }
    if (combinedBundleUpload.assetsWritten < combinedUpdate.assetsSize) {
      const size_t count = std::min(length - offset, static_cast<size_t>(combinedUpdate.assetsSize) - combinedBundleUpload.assetsWritten);
      if (!writeAssetRawUpload(data + offset, count)) { combinedBundleUpload.error = assetRawUpload.error; return false; }
      combinedBundleUpload.assetsWritten += count; offset += count; continue;
    }
    combinedBundleUpload.error = "update_bundle_trailing_data"; return false;
  }
  return true;
}

void handleCombinedBundleUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    resetSignedOta(); resetAssetRawUpload(); resetCombinedBundleUpload(); combinedUpdate = CombinedUpdatePlan{};
    combinedBundleUpload.authorized = otaRequestAuthorized();
    combinedBundleUpload.ok = combinedBundleUpload.authorized && upload.filename.endsWith(".irup") && assetBackupServed;
    if (!combinedBundleUpload.ok) combinedBundleUpload.error = !combinedBundleUpload.authorized ? "unauthorized" : !assetBackupServed ? "asset_backup_required" : "signed_irup_bundle_required";
  } else if (upload.status == UPLOAD_FILE_WRITE && combinedBundleUpload.ok) {
    if (!consumeCombinedBundle(upload.buf, upload.currentSize)) { combinedBundleUpload.ok = false; Update.abort(); }
  } else if (upload.status == UPLOAD_FILE_END && combinedBundleUpload.ok) {
    combinedBundleUpload.ok = combinedBundleUpload.manifestVerified && combinedUpdate.firmwareStaged &&
        combinedBundleUpload.assetsWritten == 0x10000U && finishAssetRawUpload();
    if (combinedBundleUpload.ok) combinedUpdate.assetsStaged = true; else Update.abort();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    combinedBundleUpload.ok = false; combinedBundleUpload.error = "update_bundle_upload_aborted"; Update.abort();
  }
}

void handleCombinedBundleFinished() {
  if (!combinedBundleUpload.authorized || !combinedBundleUpload.ok || !combinedUpdate.assetsStaged || !commitVerifiedOta()) {
    const String error = combinedBundleUpload.error.length() ? combinedBundleUpload.error : otaUploadError.length() ? otaUploadError : "update_bundle_invalid";
    server.send(400, "application/json", "{\"error\":\"" + jsonEscape(error) + "\"}"); return;
  }
  eventLog.add("WARN", "COMBINED_UPDATE", "Signiertes Gesamtupdate installiert: " + String(combinedUpdate.version));
  server.send(200, "application/json", "{\"ok\":true,\"restart\":true}");
  delay(500); ESP.restart();
}

void resetSignedOta() {
  if (signedOta.updateStarted) Update.abort();
  mbedtls_sha256_free(&signedOta.sha);
  signedOta = SignedOtaState{};
  mbedtls_sha256_init(&signedOta.sha);
}

bool beginSignedOtaImage() {
  static const uint8_t magic[8] = {'I', 'R', 'F', 'W', '1', '0', '0', 0};
  if (memcmp(signedOta.header, magic, sizeof(magic)) != 0) {
    otaUploadError = "invalid_package_magic";
    return false;
  }
  signedOta.firmwareSize =
      static_cast<uint32_t>(signedOta.header[8]) |
      (static_cast<uint32_t>(signedOta.header[9]) << 8) |
      (static_cast<uint32_t>(signedOta.header[10]) << 16) |
      (static_cast<uint32_t>(signedOta.header[11]) << 24);
  signedOta.signatureSize =
      static_cast<uint16_t>(signedOta.header[12]) |
      (static_cast<uint16_t>(signedOta.header[13]) << 8);
  if (signedOta.firmwareSize < 1024 ||
      signedOta.firmwareSize > ESP.getFreeSketchSpace() ||
      signedOta.signatureSize < 64 ||
      signedOta.signatureSize > sizeof(signedOta.signature)) {
    otaUploadError = "invalid_package_sizes";
    return false;
  }
  if (!Update.begin(signedOta.firmwareSize, U_FLASH)) {
    otaUploadError = "update_partition_unavailable";
    return false;
  }
  signedOta.updateStarted = true;
  if (mbedtls_sha256_starts_ret(&signedOta.sha, 0) != 0) {
    otaUploadError = "sha256_initialization_failed";
    return false;
  }
  return true;
}

bool consumeSignedOta(const uint8_t *data, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    if (signedOta.headerRead < sizeof(signedOta.header)) {
      const size_t count =
          std::min(length - offset,
                   sizeof(signedOta.header) - signedOta.headerRead);
      memcpy(signedOta.header + signedOta.headerRead, data + offset, count);
      signedOta.headerRead += count;
      offset += count;
      if (signedOta.headerRead == sizeof(signedOta.header) &&
          !beginSignedOtaImage())
        return false;
      continue;
    }
    if (signedOta.signatureRead < signedOta.signatureSize) {
      const size_t count =
          std::min(length - offset,
                   static_cast<size_t>(signedOta.signatureSize) -
                       signedOta.signatureRead);
      memcpy(signedOta.signature + signedOta.signatureRead, data + offset,
             count);
      signedOta.signatureRead += count;
      offset += count;
      continue;
    }
    const size_t remaining =
        signedOta.firmwareSize - signedOta.firmwareWritten;
    if (!remaining) {
      otaUploadError = "package_has_trailing_data";
      return false;
    }
    const size_t count = std::min(length - offset, remaining);
    if (!signedOta.firstFirmwareByteChecked) {
      signedOta.firstFirmwareByteChecked = true;
      if (data[offset] != 0xE9) {
        otaUploadError = "not_an_esp32_application";
        return false;
      }
    }
    if (mbedtls_sha256_update_ret(&signedOta.sha, data + offset, count) != 0 ||
        Update.write(const_cast<uint8_t *>(data + offset), count) != count) {
      otaUploadError = "firmware_write_failed";
      return false;
    }
    signedOta.firmwareWritten += count;
    offset += count;
  }
  return true;
}

bool verifySignedOta(const char *expectedSha256 = nullptr) {
  if (!signedOta.updateStarted ||
      signedOta.firmwareWritten != signedOta.firmwareSize ||
      signedOta.signatureRead != signedOta.signatureSize) {
    otaUploadError = "incomplete_signed_package";
    return false;
  }
  uint8_t digest[32];
  if (mbedtls_sha256_finish_ret(&signedOta.sha, digest) != 0) {
    otaUploadError = "sha256_finalization_failed";
    return false;
  }
  mbedtls_pk_context publicKey;
  mbedtls_pk_init(&publicKey);
  const int parseResult = mbedtls_pk_parse_public_key(
      &publicKey,
      reinterpret_cast<const unsigned char *>(kFirmwareSigningPublicKey),
      strlen(kFirmwareSigningPublicKey) + 1);
  const int verifyResult =
      parseResult == 0
          ? mbedtls_pk_verify(&publicKey, MBEDTLS_MD_SHA256, digest,
                              sizeof(digest), signedOta.signature,
                              signedOta.signatureSize)
          : parseResult;
  mbedtls_pk_free(&publicKey);
  if (verifyResult != 0) {
    memset(digest, 0, sizeof(digest));
    otaUploadError = "firmware_signature_invalid";
    return false;
  }
  if (expectedSha256 &&
      !constantTimeEqual(hexBytes(digest, sizeof(digest)), expectedSha256)) {
    memset(digest, 0, sizeof(digest));
    otaUploadError = "firmware_sha256_mismatch";
    return false;
  }
  memset(digest, 0, sizeof(digest));
  return true;
}

bool commitVerifiedOta() {
  if (!history.flushPending(HistoryStore::Tier::Minute)) {
    otaUploadError = "history_flush_before_update_failed";
    eventLog.add("ERROR", "OTA_HISTORY_FLUSH",
                 "Update abgebrochen: Minutenpuffer nicht speicherbar");
    return false;
  }
  eventLog.add("INFO", "OTA_HISTORY_FLUSH",
               "Offenen Minutenblock vor Update gespeichert");
  if (!Update.end(true)) {
    otaUploadError = "firmware_image_validation_failed";
    return false;
  }
  return true;
}

bool finishSignedOta() {
  return verifySignedOta() && commitVerifiedOta();
}

#if IR_TRACKER_ENABLE_GITHUB_UPDATE
uint64_t firmwareVersionNumber(const String &value) {
  String normalized = value;
  if (normalized.startsWith("v")) normalized.remove(0, 1);
  unsigned int major = 0, minor = 0, patch = 0, beta = 255;
  if (sscanf(normalized.c_str(), "%u.%u.%u-beta.%u", &major, &minor,
             &patch, &beta) < 3) {
    beta = 255;
    if (sscanf(normalized.c_str(), "%u.%u.%u", &major, &minor, &patch) != 3)
      return 0;
  }
  if (major > 65535 || minor > 65535 || patch > 65535 || beta > 255)
    return 0;
  return (static_cast<uint64_t>(major) << 40) |
         (static_cast<uint64_t>(minor) << 24) |
         (static_cast<uint64_t>(patch) << 8) | beta;
}

String githubUpdateJson() {
  String json = "{\"current_version\":\"" + String(kFirmwareVersion) +
                "\",\"automatic_checks\":" +
                String(config.githubUpdateCheck ? "true" : "false") +
                ",\"automatic_install\":" +
                String(config.githubAutoInstall ? "true" : "false") +
                ",\"checked\":" + String(githubUpdate.checked ? "true" : "false") +
                ",\"checking\":" + String(githubUpdate.checking ? "true" : "false") +
                ",\"installing\":" + String(githubUpdate.installing ? "true" : "false") +
                ",\"available\":" + String(githubUpdate.available ? "true" : "false") +
                ",\"latest_version\":\"" + jsonEscape(githubUpdate.version) +
                "\",\"asset_name\":\"" + jsonEscape(githubUpdate.assetName) +
                "\",\"asset_size\":" + String(githubUpdate.assetSize) +
                ",\"complete_package\":" +
                String(githubUpdate.planUrl.length() && githubUpdate.webAssetUrl.length() ? "true" : "false") +
                ",\"last_success\":" + String(static_cast<uint32_t>(githubUpdate.lastSuccess)) +
                ",\"error\":\"" + jsonEscape(githubUpdate.error) + "\"}";
  return json;
}

bool checkGithubFirmwareUpdate() {
  if (githubUpdate.checking || githubUpdate.installing) return false;
  githubUpdate.checking = true;
  githubUpdate.error = "";
  githubUpdate.lastAttemptMs = millis();
  githubUpdate.available = false;
  githubUpdate.version = "";
  githubUpdate.assetName = "";
  githubUpdate.assetUrl = "";
  githubUpdate.assetSize = 0;
  githubUpdate.planUrl = "";
  githubUpdate.webAssetUrl = "";
  githubUpdate.planSize = 0;
  githubUpdate.webAssetSize = 0;
  if (!networkConnected()) {
    githubUpdate.error = "network_not_connected";
    githubUpdate.checking = false;
    return false;
  }
  if (time(nullptr) < 1700000000) {
    githubUpdate.error = "system_time_not_synchronized";
    githubUpdate.checking = false;
    return false;
  }
  requestCpuBoost("github_update_check");
  WiFiClientSecure client;
  client.setCACert(kGithubRootCertificates);
  HTTPClient http;
  http.setConnectTimeout(7000);
  http.setTimeout(9000);
  if (!http.begin(client, kGithubReleasesApi)) {
    githubUpdate.error = "github_connection_initialization_failed";
    githubUpdate.checking = false;
    return false;
  }
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("X-GitHub-Api-Version", "2022-11-28");
  http.addHeader("User-Agent", "IR-Tracker-Offline/" + String(kFirmwareVersion));
  const int response = http.GET();
  if (response != HTTP_CODE_OK) {
    githubUpdate.error = "github_http_" + String(response);
    http.end();
    githubUpdate.checking = false;
    return false;
  }
  StaticJsonDocument<512> filter;
  filter[0]["draft"] = true;
  filter[0]["tag_name"] = true;
  filter[0]["assets"] = true;
  DynamicJsonDocument releases(16384);
  const DeserializationError parseError = deserializeJson(
      releases, http.getStream(), DeserializationOption::Filter(filter));
  if (parseError) {
    githubUpdate.error = "github_json_invalid";
    http.end();
    githubUpdate.checking = false;
    return false;
  }
  const uint64_t current = firmwareVersionNumber(kFirmwareVersion);
  uint64_t best = current;
  for (JsonObject release : releases.as<JsonArray>()) {
    if (release["draft"] | true) continue;
    const String tag = release["tag_name"] | "";
    const uint64_t candidate = firmwareVersionNumber(tag);
    if (!candidate || candidate <= best) continue;
    const String version = tag.startsWith("v") ? tag.substring(1) : tag;
    String bundleName, bundleUrl, firmwareUrl;
    size_t bundleSize = 0, firmwareSize = 0;
    for (JsonObject asset : release["assets"].as<JsonArray>()) {
      const String name = asset["name"] | "";
      const String url = asset["browser_download_url"] | "";
      const size_t size = asset["size"] | 0;
      if (!url.startsWith(kGithubAssetPrefix)) continue;
      if (name == "ir-tracker-update-" + version + ".irup" &&
          size >= 0x11000U && size <= kGithubMaximumPackageBytes + 0x11000U) {
        bundleName = name;
        bundleUrl = url;
        bundleSize = size;
      } else if (name == "ir-tracker-custom-" + version + ".irfw" &&
                 size >= 1024 && size <= kGithubMaximumPackageBytes) {
        firmwareUrl = url;
        firmwareSize = size;
      }
    }
    // IRFW stays published for older/manual installs, but automatic updates
    // deliberately accept only the one-file, asset-bound IRUP200 package.
    if (bundleUrl.length() && firmwareUrl.length()) {
      best = candidate;
      githubUpdate.version = version;
      githubUpdate.assetName = bundleName;
      githubUpdate.assetUrl = bundleUrl;
      githubUpdate.assetSize = bundleSize;
      githubUpdate.planUrl = firmwareUrl;
      githubUpdate.planSize = firmwareSize;
      githubUpdate.webAssetUrl = "bundled";
      githubUpdate.webAssetSize = 0x10000U;
      githubUpdate.available = true;
    }
  }
  http.end();
  githubUpdate.checked = true;
  githubUpdate.lastSuccess = time(nullptr);
  githubUpdate.checking = false;
  eventLog.add("INFO", "GITHUB_UPDATE_CHECK",
               githubUpdate.available
                   ? "Signiertes Firmwareupdate " + githubUpdate.version + " gefunden"
                   : "Keine neuere signierte Firmware verfuegbar");
  return true;
}

bool installGithubFirmwareUpdate() {
  if (!githubUpdate.available || githubUpdate.installing ||
      !githubUpdate.assetUrl.startsWith(kGithubAssetPrefix) ||
      githubUpdate.assetSize < 0x11000U ||
      githubUpdate.assetSize > kGithubMaximumPackageBytes + 0x11000U) {
    githubUpdate.error = "no_valid_update_selected";
    return false;
  }
  githubUpdate.installing = true;
  githubUpdate.error = "";
  requestCpuBoost("github_update_install");
  WiFiClientSecure client;
  client.setCACert(kGithubRootCertificates);
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, githubUpdate.assetUrl)) {
    githubUpdate.error = "update_connection_initialization_failed";
    githubUpdate.installing = false;
    return false;
  }
  http.addHeader("Accept", "application/octet-stream");
  http.addHeader("User-Agent", "IR-Tracker-Offline/" + String(kFirmwareVersion));
  const int response = http.GET();
  if (response != HTTP_CODE_OK) {
    githubUpdate.error = "update_http_" + String(response);
    http.end();
    githubUpdate.installing = false;
    return false;
  }
  const int declaredLength = http.getSize();
  if (declaredLength > 0 &&
      static_cast<size_t>(declaredLength) != githubUpdate.assetSize) {
    githubUpdate.error = "update_size_mismatch";
    http.end();
    githubUpdate.installing = false;
    return false;
  }
  resetSignedOta();
  resetAssetRawUpload();
  resetCombinedBundleUpload();
  combinedUpdate = CombinedUpdatePlan{};
  otaUploadError = "";
  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[1024];
  size_t received = 0;
  uint32_t lastProgress = millis();
  bool ok = true;
  while (received < githubUpdate.assetSize) {
    esp_task_wdt_reset();
    const int available = stream->available();
    if (available > 0) {
      const size_t wanted = std::min<size_t>(
          sizeof(buffer), std::min<size_t>(available,
                                           githubUpdate.assetSize - received));
      const int count = stream->readBytes(buffer, wanted);
      if (count <= 0 || !consumeCombinedBundle(buffer, count)) {
        ok = false;
        break;
      }
      received += count;
      lastProgress = millis();
    } else if (!http.connected() || millis() - lastProgress > 12000) {
      combinedBundleUpload.error = "update_bundle_download_incomplete";
      ok = false;
      break;
    } else {
      delay(2);
    }
  }
  if (ok && received == githubUpdate.assetSize)
    ok = combinedBundleUpload.manifestVerified && combinedUpdate.firmwareStaged &&
         combinedBundleUpload.assetsWritten == 0x10000U && finishAssetRawUpload();
  http.end();
  memset(buffer, 0, sizeof(buffer));
  if (ok) combinedUpdate.assetsStaged = true;
  if (ok) ok = commitVerifiedOta();
  if (!ok) Update.abort();
  githubUpdate.installing = false;
  if (!ok) {
    githubUpdate.error = combinedBundleUpload.error.length() ? combinedBundleUpload.error : otaUploadError.length() ? otaUploadError : "update_failed";
    eventLog.add("ERROR", "GITHUB_UPDATE_FAILED", githubUpdate.error);
    return false;
  }
  eventLog.add("WARN", "GITHUB_UPDATE_INSTALLED",
               "Signiertes GitHub-Update " + githubUpdate.version + " installiert");
  return true;
}

void manageGithubFirmwareUpdate() {
  if (!config.githubUpdateCheck || githubUpdate.checking ||
      githubUpdate.installing || gpioScan.active || irPulse.active ||
      !networkConnected())
    return;
  const uint32_t interval = githubUpdate.checked ? kGithubCheckIntervalMs
                                                 : kGithubInitialCheckMs;
  if (millis() - githubUpdate.lastAttemptMs < interval) return;
  if (checkGithubFirmwareUpdate() && config.githubAutoInstall &&
      githubUpdate.available && installGithubFirmwareUpdate()) {
    delay(500);
    ESP.restart();
  }
}
#else
String githubUpdateJson() {
  return "{\"current_version\":\"" + String(kFirmwareVersion) +
         "\",\"automatic_checks\":false,\"automatic_install\":false,"
         "\"checked\":true,\"checking\":false,\"installing\":false,"
         "\"available\":false,\"latest_version\":\"\",\"asset_name\":\"\","
         "\"asset_size\":0,\"last_success\":0,\"error\":\"factory_build\"}";
}
bool checkGithubFirmwareUpdate() { return false; }
bool installGithubFirmwareUpdate() { return false; }
void manageGithubFirmwareUpdate() {}
#endif

void handleOtaUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    resetSignedOta();
    otaUploadError = "";
    otaUploadAuthorized = otaRequestAuthorized();
    if (otaUploadAuthorized) requestCpuBoost("firmware_update");
    otaUploadOk = otaUploadAuthorized &&
                  upload.filename.endsWith(".irfw");
    if (!otaUploadAuthorized) otaUploadError = "unauthorized";
    if (otaUploadAuthorized && !upload.filename.endsWith(".irfw"))
      otaUploadError = "signed_irfw_package_required";
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (otaUploadOk &&
        !consumeSignedOta(upload.buf, upload.currentSize)) {
      otaUploadOk = false;
      Update.abort();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (otaUploadOk) otaUploadOk = finishSignedOta();
    if (!otaUploadOk) Update.abort();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    otaUploadOk = false;
    otaUploadError = "upload_aborted";
    Update.abort();
  }
}

void handleOtaFinished() {
  if (!otaUploadAuthorized) {
    server.send(
        401, "text/html; charset=utf-8",
        page("Firmwareupdate nicht möglich",
             "<div class='error'><strong>Anmeldung oder Sicherheitsprüfung "
             "abgelaufen.</strong><p>Wartungsseite neu laden, erneut anmelden "
             "und das signierte IRFW-Paket noch einmal auswählen.</p></div>"
             "<p><a href='/maintenance#firmware-update'>Zurück zur Wartung</a></p>"));
    return;
  }
  if (!otaUploadOk) {
    const String technicalCode =
        otaUploadError.length() ? otaUploadError : "invalid_signed_firmware";
    server.send(
        400, "text/html; charset=utf-8",
        page("Firmwareupdate abgelehnt",
             "<div class='error'><strong>Das Firmwarepaket konnte nicht sicher "
             "installiert werden.</strong><p>Nur ein vollständiges, für diesen "
             "Tracker signiertes IRFW-Paket verwenden.</p><details><summary>"
             "Technischer Fehlercode</summary><code>" +
                 htmlEscape(technicalCode) +
                 "</code></details></div><p><a href='/maintenance#firmware-update'>"
                 "Zurück zur Wartung</a></p>"));
    return;
  }
  eventLog.add("WARN", "OTA_UPDATE",
               "Kryptografisch signierte Custom-Firmware installiert");
  server.send(200, "text/html; charset=utf-8",
              page("Update erfolgreich",
                   "<div class='card'><h2>Firmware geprüft und installiert</h2>"
                   "<p>Der Tracker startet jetzt mit dem neuen Custom-Slot.</p></div>"));
  delay(800);
  ESP.restart();
}
