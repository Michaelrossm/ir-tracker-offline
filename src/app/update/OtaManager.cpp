// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

bool otaRequestAuthorized() {
  return requireAdmin();
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

bool finishSignedOta() {
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
  memset(digest, 0, sizeof(digest));
  if (verifyResult != 0) {
    otaUploadError = "firmware_signature_invalid";
    return false;
  }
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
  filter[0]["assets"][0]["name"] = true;
  filter[0]["assets"][0]["browser_download_url"] = true;
  filter[0]["assets"][0]["size"] = true;
  DynamicJsonDocument releases(12288);
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
    for (JsonObject asset : release["assets"].as<JsonArray>()) {
      const String name = asset["name"] | "";
      const String url = asset["browser_download_url"] | "";
      const size_t size = asset["size"] | 0;
      if (!name.startsWith("ir-tracker-custom-") || !name.endsWith(".irfw") ||
          !url.startsWith(kGithubAssetPrefix) || size < 1024 ||
          size > kGithubMaximumPackageBytes)
        continue;
      best = candidate;
      githubUpdate.version = tag.startsWith("v") ? tag.substring(1) : tag;
      githubUpdate.assetName = name;
      githubUpdate.assetUrl = url;
      githubUpdate.assetSize = size;
      githubUpdate.available = true;
      break;
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
      githubUpdate.assetSize < 1024 ||
      githubUpdate.assetSize > kGithubMaximumPackageBytes) {
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
      if (count <= 0 || !consumeSignedOta(buffer, count)) {
        ok = false;
        break;
      }
      received += count;
      lastProgress = millis();
    } else if (!http.connected() || millis() - lastProgress > 12000) {
      otaUploadError = "update_download_incomplete";
      ok = false;
      break;
    } else {
      delay(2);
    }
  }
  if (ok && received == githubUpdate.assetSize) ok = finishSignedOta();
  if (!ok) Update.abort();
  http.end();
  memset(buffer, 0, sizeof(buffer));
  githubUpdate.installing = false;
  if (!ok) {
    githubUpdate.error = otaUploadError.length() ? otaUploadError : "update_failed";
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
