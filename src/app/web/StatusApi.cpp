// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String statusJson() {
  const bool fresh = valueFresh(meter.powerUpdatedMs);
  const bool browserSessionValid = validBrowserSession();
  String json = "{";
  json += "\"firmware\":\"offline-" + String(kFirmwareVersion) + "\",";
  json += "\"hardware_profile\":\"universal\",";
  json += "\"w5500_gpio_reserved\":" +
          String(HardwareProfile::kLanPrepared ? "true" : "false") + ",";
  json += "\"installer_wifi_ota\":true,";
  json += "\"installer_gpio_tx_scan\":true,";
  json += "\"author\":\"" + String(kFirmwareAuthor) + "\",";
  json += "\"license\":\"" + String(kFirmwareLicense) + "\",";
  json += "\"transport_security\":\"http_local_trusted_network_only\",";
  json += "\"browser_session_valid\":" +
          String(browserSessionValid ? "true" : "false") + ",";
  json += "\"browser_session_state\":\"" +
          String(browserSessionState) + "\",";
  json += "\"mode\":\"" + String(primaryTransportName()) + "\",";
  json += "\"hostname\":\"" + jsonEscape(config.hostname) + "\",";
  json += "\"ip\":\"" + primaryNetworkIp() + "\",";
  json += "\"ethernet_initialized\":" +
          String(ethernet.initialized() ? "true" : "false") + ",";
  json += "\"ethernet_detected\":" +
          String(ethernet.hardwareDetected() ? "true" : "false") + ",";
  json += "\"ethernet_link\":" +
          String(ethernet.linkUp() ? "true" : "false") + ",";
  json += "\"ethernet_connected\":" +
          String(ethernet.connected() ? "true" : "false") + ",";
  json += "\"ethernet_ip\":\"" + ethernet.localIP().toString() + "\",";
  json += "\"ethernet_error\":\"" + jsonEscape(ethernet.lastError()) + "\",";
  json += "\"ethernet_controller\":\"w5500_spi\",";
  json += "\"poe_power_detectable\":false,";
  json += "\"wifi_connected\":" +
          String(wifiConnected() ? "true" : "false") + ",";
  json += "\"wifi_ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"wifi_rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
  json += "\"wifi_ssid\":\"" + jsonEscape(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "") + "\",";
  json += "\"setup_ap_active\":" +
          String(accessPointMode ? "true" : "false") + ",";
  json += "\"mdns_running\":";
#if IR_TRACKER_ENABLE_MDNS
  json += String(mdnsRunning ? "true" : "false");
#else
  json += "false";
#endif
  json += ",\"mdns_name\":\"";
#if IR_TRACKER_ENABLE_MDNS
  json += jsonEscape(config.hostname) + ".local";
#endif
  json += "\",";
  json += "\"wifi_sta_only\":" +
          String(WiFi.getMode() == WIFI_STA ? "true" : "false") + ",";
  json += "\"wifi_min_modem_sleep\":" +
          String(wifiMinModemSleepActive ? "true" : "false") + ",";
  json += "\"adaptive_wifi_power\":" +
          String(config.adaptiveWifiPower ? "true" : "false") + ",";
  json += "\"wifi_tx_profile\":\"" + String(wifiTxProfileName()) + "\",";
  json += "\"wifi_tx_power_dbm\":" + String(wifiTxPowerDbm(), 1) + ",";
  json += "\"wifi_tx_power_changes\":" + String(wifiTxPowerChanges) + ",";
  json += "\"wifi_tx_power_errors\":" + String(wifiTxPowerErrors) + ",";
  json += "\"wifi_mode_errors\":" + String(wifiModeErrors) + ",";
  json += "\"wifi_tx_runtime_fault\":" +
          String(wifiTxPowerRuntimeFault ? "true" : "false") + ",";
  json += "\"mqtt_connected\":" + String(mqtt.connected() ? "true" : "false") + ",";
  json += "\"meter_fresh\":" + String(fresh ? "true" : "false") + ",";
  json += "\"meter_protocol\":\"" +
          String(meterProtocolName(meter.detectedProtocol)) + "\",";
  json += "\"configured_meter_protocol\":\"" +
          String(meterProtocolName(config.meterProtocol)) + "\",";
  json += "\"telegram_age_s\":" + ageOrNull(meter.lastTelegramMs) + ",";
  json += "\"power_w\":" + numberOrNull(meter.powerW) + ",";
  json += "\"power_age_s\":" + ageOrNull(meter.powerUpdatedMs) + ",";
  json += "\"import_kwh\":" + numberOrNull(meter.importKwh) + ",";
  json += "\"import_age_s\":" + ageOrNull(meter.importUpdatedMs) + ",";
  json += "\"export_kwh\":" + numberOrNull(meter.exportKwh) + ",";
  json += "\"export_age_s\":" + ageOrNull(meter.exportUpdatedMs) + ",";
  json += "\"phases\":[";
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (phase) json += ",";
    json += "{\"phase\":\"L" + String(phase + 1) +
            "\",\"power_w\":" + numberOrNull(meter.phasePowerW[phase]) +
            ",\"power_age_s\":" + ageOrNull(meter.phasePowerUpdatedMs[phase]) +
            ",\"voltage_v\":" + numberOrNull(meter.phaseVoltageV[phase]) +
            ",\"voltage_age_s\":" + ageOrNull(meter.phaseVoltageUpdatedMs[phase]) +
            ",\"current_a\":" + numberOrNull(meter.phaseCurrentA[phase]) +
            ",\"current_age_s\":" + ageOrNull(meter.phaseCurrentUpdatedMs[phase]) +
            "}";
  }
  json += "],";
  json += "\"telegrams\":" + String(meter.telegrams) + ",";
  json += "\"received_bytes\":" + String(meter.bytes) + ",";
  json += "\"parse_errors\":" + String(meter.parseErrors) + ",";
  json += "\"crc_errors\":" + String(meter.crcErrors) + ",";
  json += "\"meter_reinitializations\":" +
          String(meterReinitializations) + ",";
  json += "\"last_crc_valid\":" + String(meter.lastCrcValid ? "true" : "false") + ",";
  json += "\"last_integrity_present\":" +
          String(meter.lastIntegrityPresent ? "true" : "false") + ",";
  json += "\"rx_gpio\":" + String(config.rxPin) + ",";
  json += "\"tx_gpio\":" + String(config.txPin) + ",";
  json += "\"ir_transmitting\":" + String(irPulse.active ? "true" : "false") + ",";
  json += "\"apator_unlock_active\":" +
          String(apatorUnlock.active ? "true" : "false") + ",";
  json += "\"apator_unlock_phase\":" + String(apatorUnlock.phase) + ",";
  json += "\"history_ready\":" + String(history.ready() ? "true" : "false") + ",";
  json += "\"live_history_minutes\":70,";
  json += "\"time_valid\":" + String(time(nullptr) >= 1700000000 ? "true" : "false") + ",";
  json += "\"event_count\":" + String(eventLog.count()) + ",";
  json += "\"event_log_persistent\":" +
          String(eventLog.persistent() ? "true" : "false") + ",";
  json += "\"eco_mode_enabled\":" +
          String(config.ecoMode ? "true" : "false") + ",";
  json += "\"eco_led_idle_off\":" +
          String(config.ecoLedOff ? "true" : "false") + ",";
  json += "\"led_fault_active\":" +
          String(trackerFaultActive() ? "true" : "false") + ",";
  json += "\"led_suppressed\":" +
          String(ecoLedSuppressed() ? "true" : "false") + ",";
  json += "\"eco_runtime_fault\":" +
          String(cpuEcoRuntimeFault ? "true" : "false") + ",";
  json += "\"cpu_frequency_mhz\":" + String(getCpuFrequencyMhz()) + ",";
  json += "\"cpu_boost_active\":" +
          String(cpuBoostActive() ? "true" : "false") + ",";
  json += "\"cpu_boost_remaining_s\":" +
          String(cpuBoostRemainingSeconds()) + ",";
  json += "\"cpu_boost_reason\":\"" + jsonEscape(cpuBoostReason) + "\",";
  json += "\"cpu_frequency_switches\":" + String(cpuFrequencySwitches) + ",";
  json += "\"cpu_frequency_errors\":" + String(cpuFrequencyErrors) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"heap_warning\":" +
          String(heapWarningActive ? "true" : "false") + ",";
  json += "\"restart_reason\":\"" + bootResetReason + "\",";
  json += "\"led_gpio\":" + String(config.ledPin) + ",";
  json += "\"baud\":" + String(meterBaud()) + ",";
  json += "\"configured_baud\":" + String(config.baud) + ",";
  json += "\"github_update_check\":" +
          String(config.githubUpdateCheck ? "true" : "false") + ",";
  json += "\"github_auto_install\":" +
          String(config.githubAutoInstall ? "true" : "false") + ",";
  json += "\"github_update_available\":" +
          String(githubUpdate.available ? "true" : "false") + ",";
  json += "\"github_update_version\":\"" +
          jsonEscape(githubUpdate.version) + "\",";
  json += "\"uptime_s\":" + String(millis() / 1000);
  json += "}";
  return json;
}
