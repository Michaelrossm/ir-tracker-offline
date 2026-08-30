// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void setupRoutes() {
  const char *securityHeaders[] = {"Origin", "Referer", "Authorization",
                                   "X-CSRF-Token", "Cookie"};
  server.collectHeaders(securityHeaders, 5);
  server.on("/assets/common.css", HTTP_GET, [] {
    if (tryServeDebugAsset("/assets/common.css", "text/css; charset=utf-8",
                           kCommonCssGzip, kCommonCssGzipSize))
      return;
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("text/css; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kCommonCssGzip), kCommonCssGzipSize);
  });
  server.on("/assets/common.js", HTTP_GET, [] {
    if (tryServeDebugAsset("/assets/common.js",
                           "application/javascript; charset=utf-8",
                           kCommonJsGzip, kCommonJsGzipSize))
      return;
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kCommonJsGzip), kCommonJsGzipSize);
  });
  server.on("/assets/i18n.js", HTTP_GET, [] {
    if (tryServeDebugAsset("/assets/i18n.js",
                           "application/javascript; charset=utf-8",
                           kI18nJsGzip, kI18nJsGzipSize))
      return;
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kI18nJsGzip), kI18nJsGzipSize);
  });
  server.on("/assets/dashboard.js", HTTP_GET, [] {
    if (tryServeDebugAsset("/assets/dashboard.js",
                           "application/javascript; charset=utf-8",
                           kDashboardJsGzip, kDashboardJsGzipSize))
      return;
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kDashboardJsGzip),
                  kDashboardJsGzipSize);
  });
  server.on("/assets/history.js", HTTP_GET, [] {
    if (tryServeDebugAsset("/assets/history.js",
                           "application/javascript; charset=utf-8",
                           kHistoryJsGzip, kHistoryJsGzipSize))
      return;
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kHistoryJsGzip), kHistoryJsGzipSize);
  });
  server.on("/", HTTP_GET, handleRoot);
  server.on("/history", HTTP_GET, handleHistoryPage);
  server.on("/interfaces", HTTP_GET, handleInterfacesPage);
  server.on("/maintenance", HTTP_GET, handleMaintenancePage);
  server.on("/maintenance/diagnostics", HTTP_GET, handleDiagnostics);
#if IR_TRACKER_ENABLE_FACTORY_TEST
  server.on("/maintenance/factory-test", HTTP_GET, handleFactoryTestPage);
  server.on("/api/v1/factory-test", HTTP_GET, [] {
    if (requireAdmin())
      server.send(200, "application/json", factoryTestJson());
  });
  server.on("/api/v1/factory-test/start", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (!startFactoryTest()) {
      server.send(409, "application/json", factoryTestJson());
      return;
    }
    server.send(202, "application/json", factoryTestJson());
  });
  server.on("/api/v1/factory-test/led-confirm", HTTP_POST, [] {
    if (!requireAdmin()) return;
    factoryTest.ledConfirmed = true;
    if (config.ledPin >= 0)
      digitalWrite(config.ledPin, config.ledInverted);
    server.send(200, "application/json", factoryTestJson());
  });
  server.on("/api/v1/factory-test/poe-confirm", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (!ethernet.connected()) {
      server.send(409, "application/json", factoryTestJson());
      return;
    }
    factoryTest.poeConfirmed = true;
    server.send(200, "application/json", factoryTestJson());
  });
#endif
  server.on("/setup", HTTP_GET, handleSetup);
  server.on("/setup/save", HTTP_POST, handleSetupSave);
  server.on("/auth/logout", HTTP_POST, handleLogout);
  server.on("/diagnostics", HTTP_GET, [] {
    server.sendHeader("Location", "/maintenance/diagnostics", true);
    server.send(301, "text/plain", "");
  });
  server.on("/api/v1/status", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", statusJson());
  });
  server.on("/api/v1/admin-session", HTTP_GET, [] {
    if (!requireAdmin()) return;
    server.send(200, "application/json",
                "{\"csrf_token\":\"" + csrfToken + "\"}");
  });
  server.on("/api/v1/update/status", HTTP_GET, [] {
    if (!requireAdmin()) return;
    server.send(200, "application/json", githubUpdateJson());
  });
  server.on("/api/v1/update/check", HTTP_POST, [] {
    if (!requireAdmin()) return;
    checkGithubFirmwareUpdate();
    server.sendHeader("Location", "/maintenance#firmware-update", true);
    server.send(303, "text/plain", "");
  });
  server.on("/api/v1/update/install", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (!githubUpdate.available && !checkGithubFirmwareUpdate()) {
      server.sendHeader("Location", "/maintenance#firmware-update", true);
      server.send(303, "text/plain", "");
      return;
    }
    if (!installGithubFirmwareUpdate()) {
      server.sendHeader("Location", "/maintenance#firmware-update", true);
      server.send(303, "text/plain", "");
      return;
    }
    server.send(200, "text/html; charset=utf-8",
                page("Update erfolgreich",
                     "<div class='card'><h2>Signiertes GitHub-Update installiert</h2>"
                     "<p>Der Tracker startet jetzt neu.</p></div>"));
    delay(700);
    ESP.restart();
  });
  server.on("/api/v1/gpio-scan", HTTP_GET, [] {
    if (!requireAdmin()) return;
    server.send(200, "application/json", gpioScanJson());
  });
  server.on("/api/v1/gpio-scan/start", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (gpioScan.active) {
      server.send(409, "application/json", gpioScanJson());
      return;
    }
    startGpioScan();
    server.send(202, "application/json", gpioScanJson());
  });
  server.on("/api/v1/gpio-scan/cancel", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (gpioScan.active) finishGpioScan(false, "cancelled");
    server.send(200, "application/json", gpioScanJson());
  });
  server.on("/api/v1/gpio-output-test", HTTP_POST,
            handleGpioOutputTest);
  server.on("/api/v1/gpio-scan-tx", HTTP_POST, handleGpioTxScan);
  server.on("/api/v1/selftest", HTTP_GET, [] {
    if (requireAdmin())
      server.send(200, "application/json", selfTestJson());
  });
  server.on("/api/v1/meter-report", HTTP_GET, [] {
    if (requireAdmin())
      server.send(200, "application/json", meterReportJson());
  });
  // DE: Shelly-kompatible, nur lesende Zählerfassade für Speicher. | EN: Shelly-compatible read-only meter facade for storage systems.
  server.on("/status", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyGen1Status());
  });
  server.on("/emeter/0", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyEmStatus());
  });
  server.on("/rpc/EM.GetStatus", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyEmStatus());
  });
  server.on("/rpc/EMData.GetStatus", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyEmStatus());
  });
  server.on("/rpc/Shelly.GetStatus", HTTP_GET, [] {
    if (!requireApiAccess()) return;
    server.send(200, "application/json",
                "{\"sys\":{\"uptime\":" + String(millis() / 1000) +
                    "},\"wifi\":{\"sta_ip\":\"" +
                    primaryNetworkIp() + "\",\"rssi\":" +
                    String(wifiConnected() ? WiFi.RSSI() : 0) + "},\"em:0\":" +
                    shellyEmStatus() + "}");
  });
  server.on("/api/v1/memory-info", HTTP_GET, [] {
    if (requireAdmin())
      server.send(200, "application/json", memoryJson());
  });
  server.on("/api/v1/obis", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", obisJson());
  });
  server.on("/api/v1/raw", HTTP_GET, [] {
    if (!requireApiAccess()) return;
    server.send(200, "application/json",
                "{\"encoding\":\"hex\",\"length\":" + String(lastTelegram.size()) +
                ",\"data\":\"" + bytesToHex(lastTelegram) + "\"}");
  });
  server.on("/metrics", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "text/plain; version=0.0.4", metricsText());
  });
  server.on("/openmetrics", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/openmetrics-text; version=1.0.0; charset=utf-8", metricsText() + "# EOF\n");
  });
  server.on("/api/v1/influx", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "text/plain; charset=utf-8", influxLineProtocol());
  });
  server.on("/api/v1/values.csv", HTTP_GET, [] {
    if (!requireApiAccess()) return;
    server.sendHeader("Content-Disposition", "inline; filename=irtracker-values.csv");
    server.send(200, "text/csv; charset=utf-8", csvValues());
  });
  server.on("/api/v1/history", HTTP_GET, handleHistoryJson);
  server.on("/api/v1/dashboard-summary", HTTP_GET, handleDashboardSummary);
  server.on("/api/v1/history.csv", HTTP_GET, handleHistoryCsv);
  server.on("/api/v1/backup/settings", HTTP_GET, handleSettingsBackup);
  server.on("/api/v1/backup/settings/restore", HTTP_POST,
            handleSettingsRestore);
  server.on("/api/v1/history/import/start", HTTP_POST,
            handleHistoryImportStart);
  server.on("/api/v1/history/import/batch", HTTP_POST,
            handleHistoryImportBatch);
  server.on("/api/v1/history/clear", HTTP_POST, handleHistoryClearAll);
  server.on("/api/v1/events", HTTP_GET, handleEventsJson);
  server.on("/api/v1/events/clear", HTTP_POST, handleEventsClear);
  server.on("/api/v1/time", HTTP_POST, handleSetTime);
  server.on("/ir/pin", HTTP_POST, handleIrPin);
  server.on("/ir/meter-unlock", HTTP_POST, handleApatorUnlock);
  server.on("/ir/apator", HTTP_POST, handleApatorUnlock);
  server.on("/ir/pin/forget", HTTP_POST, handleForgetPin);
  server.on("/ir/pulse", HTTP_POST, handleIrPulse);
  server.on("/ir/stop", HTTP_POST, handleIrStop);
  server.on("/system/update", HTTP_POST, handleOtaFinished,
            handleOtaUpload);
  server.on("/system/shutdown", HTTP_POST, handleSafeShutdown);
  server.onNotFound([] {
    if (accessPointMode) {
      server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/setup", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "application/json", "{\"error\":\"not_found\"}");
    }
  });
  server.begin();
}

#if IR_TRACKER_ENABLE_DEVELOPER_IO
void setupWebSockets() {
  if (config.snifferEnabled) snifferSocket.begin();
  if (config.bridgeEnabled) {
    bridgeSocket.begin();
    const String password = localAdminPassword();
    bridgeSocket.setAuthorization("admin", password.c_str());
    bridgeSocket.onEvent(
        [](uint8_t, WStype_t type, uint8_t *payload, size_t length) {
          if ((type == WStype_BIN || type == WStype_TEXT) &&
              config.bridgeEnabled && config.txPin >= 0 && length &&
              length <= 512) {
            meterSerial.write(payload, length);
          }
        });
  }
}
#endif
