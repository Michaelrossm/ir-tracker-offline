// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String bytesToHex(const std::vector<uint8_t> &data) {
  static const char digits[] = "0123456789abcdef";
  String result;
  result.reserve(data.size() * 2);
  for (uint8_t value : data) {
    result += digits[value >> 4];
    result += digits[value & 0x0f];
  }
  return result;
}

String obisJson() {
  if (meter.detectedProtocol == MeterProtocol::Iec62056) {
    String json = "{\"protocol\":\"iec62056-21\",\"values\":[";
    bool first = true;
    auto add = [&](const char *obis, double value, uint32_t updatedMs) {
      if (!std::isfinite(value)) return;
      if (!first) json += ",";
      first = false;
      json += "{\"obis\":\"" + String(obis) + "\",\"value\":" +
              numberOrNull(value, 6) + ",\"age_s\":" +
              ageOrNull(updatedMs) + "}";
    };
    add("1.8.0", meter.importKwh, meter.importUpdatedMs);
    add("2.8.0", meter.exportKwh, meter.exportUpdatedMs);
    add("16.7.0", meter.powerW, meter.powerUpdatedMs);
    const char *powerCodes[3] = {"36.7.0", "56.7.0", "76.7.0"};
    const char *voltageCodes[3] = {"32.7.0", "52.7.0", "72.7.0"};
    const char *currentCodes[3] = {"31.7.0", "51.7.0", "71.7.0"};
    for (uint8_t phase = 0; phase < 3; ++phase) {
      add(powerCodes[phase], meter.phasePowerW[phase],
          meter.phasePowerUpdatedMs[phase]);
      add(voltageCodes[phase], meter.phaseVoltageV[phase],
          meter.phaseVoltageUpdatedMs[phase]);
      add(currentCodes[phase], meter.phaseCurrentA[phase],
          meter.phaseCurrentUpdatedMs[phase]);
    }
    return json + "]}";
  }
  String json = "{\"values\":[";
  bool first = true;
  for (size_t i = 2; i + 7 < lastTelegram.size(); ++i) {
    if (lastTelegram[i - 2] != 0x77 || lastTelegram[i - 1] != 0x07) continue;
    const uint8_t *code = lastTelegram.data() + i;
    double value = NAN;
    if (!SmlParser::extractObis(lastTelegram, code, value)) continue;
    if (!first) json += ",";
    first = false;
    char name[28];
    snprintf(name, sizeof(name), "%u-%u:%u.%u.%u*%u",
             code[0], code[1], code[2], code[3], code[4], code[5]);
    json += "{\"obis\":\"" + String(name) + "\",\"value\":" + numberOrNull(value, 6) + "}";
    i += 5;
  }
  json += "]}";
  return json;
}

uint32_t largestFreeHeapBlockBytes() {
  return static_cast<uint32_t>(
      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

uint32_t loopStackHighWaterMarkBytes() {
  // ESP-IDF's FreeRTOS port reports the high-water mark in bytes.
  return static_cast<uint32_t>(uxTaskGetStackHighWaterMark(nullptr));
}

String memoryJson() {
  String json = "{";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"minimum_free_heap\":" + String(ESP.getMinFreeHeap()) + ",";
  json += "\"largest_free_heap_block\":" +
          String(largestFreeHeapBlockBytes()) + ",";
  json += "\"stack_high_water_mark_bytes\":" +
          String(loopStackHighWaterMarkBytes()) + ",";
  json += "\"heap_size\":" + String(ESP.getHeapSize()) + ",";
  json += "\"flash_size\":" + String(ESP.getFlashChipSize()) + ",";
  json += "\"firmware_size\":" + String(ESP.getSketchSize()) + ",";
  json += "\"free_firmware_space\":" + String(ESP.getFreeSketchSpace()) + ",";
  json += "\"history_used_bytes\":" + String(history.usedBytes()) + ",";
  json += "\"history_total_bytes\":" + String(history.totalBytes());
  json += ",\"eco_mode_enabled\":" +
          String(config.ecoMode ? "true" : "false");
  json += ",\"eco_runtime_fault\":" +
          String(cpuEcoRuntimeFault ? "true" : "false");
  json += ",\"cpu_frequency_mhz\":" + String(getCpuFrequencyMhz());
  json += ",\"cpu_boost_active\":" +
          String(cpuBoostActive() ? "true" : "false");
  json += ",\"cpu_boost_remaining_s\":" +
          String(cpuBoostRemainingSeconds());
  json += ",\"cpu_boost_reason\":\"" + jsonEscape(cpuBoostReason) + "\"";
  json += ",\"cpu_frequency_switches\":" + String(cpuFrequencySwitches);
  json += ",\"cpu_frequency_errors\":" + String(cpuFrequencyErrors);
  json += ",\"heap_warning\":" +
          String(heapWarningActive ? "true" : "false");
  json += ",\"restart_reason\":\"" + bootResetReason + "\"";
  json += "}";
  return json;
}

String metricsText() {
  String text = "# HELP irtracker_power_w Current grid power in watts\n"
                "# TYPE irtracker_power_w gauge\n";
  if (std::isfinite(meter.powerW)) text += "irtracker_power_w " + String(meter.powerW, 3) + "\n";
  text += "# HELP irtracker_import_kwh Imported grid energy\n# TYPE irtracker_import_kwh counter\n";
  if (std::isfinite(meter.importKwh)) text += "irtracker_import_kwh " + String(meter.importKwh, 6) + "\n";
  text += "# HELP irtracker_export_kwh Exported grid energy\n# TYPE irtracker_export_kwh counter\n";
  if (std::isfinite(meter.exportKwh)) text += "irtracker_export_kwh " + String(meter.exportKwh, 6) + "\n";
  if (meter.lastTelegramMs)
    text += "irtracker_meter_age_seconds " + ageOrNull(meter.lastTelegramMs) + "\n";
  if (meter.powerUpdatedMs)
    text += "irtracker_power_age_seconds " + ageOrNull(meter.powerUpdatedMs) + "\n";
  if (meter.importUpdatedMs)
    text += "irtracker_import_age_seconds " + ageOrNull(meter.importUpdatedMs) + "\n";
  if (meter.exportUpdatedMs)
    text += "irtracker_export_age_seconds " + ageOrNull(meter.exportUpdatedMs) + "\n";
  text += "irtracker_telegrams_total " + String(meter.telegrams) + "\n";
  text += "irtracker_parse_errors_total " + String(meter.parseErrors) + "\n";
  text += "irtracker_crc_errors_total " + String(meter.crcErrors) + "\n";
  text += "irtracker_sml_crc_errors_total " +
          String(meter.smlCrcErrors) + "\n";
  text += "irtracker_d0_bcc_errors_total " + String(d0BccErrors()) + "\n";
  text += "irtracker_wifi_rssi_dbm " + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + "\n";
  text += "irtracker_network_connected " +
          String(networkConnected() ? 1 : 0) + "\n";
  text += "irtracker_ethernet_link " +
          String(ethernet.linkUp() ? 1 : 0) + "\n";
  text += "irtracker_ethernet_connected " +
          String(ethernet.connected() ? 1 : 0) + "\n";
  text += "irtracker_mqtt_connected " + String(mqtt.connected() ? 1 : 0) + "\n";
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const String label = "{phase=\"L" + String(phase + 1) + "\"} ";
    if (std::isfinite(meter.phasePowerW[phase]))
      text += "irtracker_phase_power_w" + label +
              String(meter.phasePowerW[phase], 3) + "\n";
    if (std::isfinite(meter.phaseVoltageV[phase]))
      text += "irtracker_phase_voltage_v" + label +
              String(meter.phaseVoltageV[phase], 3) + "\n";
    if (std::isfinite(meter.phaseCurrentA[phase]))
      text += "irtracker_phase_current_a" + label +
              String(meter.phaseCurrentA[phase], 4) + "\n";
    if (meter.phasePowerUpdatedMs[phase])
      text += "irtracker_phase_power_age_seconds" + label +
              ageOrNull(meter.phasePowerUpdatedMs[phase]) + "\n";
    if (meter.phaseVoltageUpdatedMs[phase])
      text += "irtracker_phase_voltage_age_seconds" + label +
              ageOrNull(meter.phaseVoltageUpdatedMs[phase]) + "\n";
    if (meter.phaseCurrentUpdatedMs[phase])
      text += "irtracker_phase_current_age_seconds" + label +
              ageOrNull(meter.phaseCurrentUpdatedMs[phase]) + "\n";
  }
  return text;
}

String influxLineProtocol() {
  String line = "irtracker,device=" + deviceId + ",hostname=" + config.hostname;
  bool hasField = false;
  String fields;
  auto addFloat = [&](const char *name, double value) {
    if (!std::isfinite(value)) return;
    if (hasField) fields += ",";
    fields += String(name) + "=" + String(value, 6);
    hasField = true;
  };
  addFloat("power_w", meter.powerW);
  addFloat("import_kwh", meter.importKwh);
  addFloat("export_kwh", meter.exportKwh);
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const String prefix = "l" + String(phase + 1) + "_";
    addFloat((prefix + "power_w").c_str(), meter.phasePowerW[phase]);
    addFloat((prefix + "voltage_v").c_str(), meter.phaseVoltageV[phase]);
    addFloat((prefix + "current_a").c_str(), meter.phaseCurrentA[phase]);
  }
  if (hasField) fields += ",";
  fields += "wifi_rssi=" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + "i";
  fields += ",telegrams=" + String(meter.telegrams) + "i";
  fields += ",parse_errors=" + String(meter.parseErrors) + "i";
  fields += ",crc_errors=" + String(meter.crcErrors) + "i";
  fields += ",sml_crc_errors=" + String(meter.smlCrcErrors) + "i";
  fields += ",d0_bcc_errors=" + String(d0BccErrors()) + "i";
  if (meter.lastTelegramMs)
    fields += ",meter_age_s=" + ageOrNull(meter.lastTelegramMs) + "i";
  if (meter.powerUpdatedMs)
    fields += ",power_age_s=" + ageOrNull(meter.powerUpdatedMs) + "i";
  if (meter.importUpdatedMs)
    fields += ",import_age_s=" + ageOrNull(meter.importUpdatedMs) + "i";
  if (meter.exportUpdatedMs)
    fields += ",export_age_s=" + ageOrNull(meter.exportUpdatedMs) + "i";
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const String prefix = ",l" + String(phase + 1) + "_";
    if (meter.phasePowerUpdatedMs[phase])
      fields += prefix + "power_age_s=" +
                ageOrNull(meter.phasePowerUpdatedMs[phase]) + "i";
    if (meter.phaseVoltageUpdatedMs[phase])
      fields += prefix + "voltage_age_s=" +
                ageOrNull(meter.phaseVoltageUpdatedMs[phase]) + "i";
    if (meter.phaseCurrentUpdatedMs[phase])
      fields += prefix + "current_age_s=" +
                ageOrNull(meter.phaseCurrentUpdatedMs[phase]) + "i";
  }
  fields += ",meter_fresh=" + String(meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs ? "true" : "false");
  return line + " " + fields + "\n";
}

String csvValues() {
  String csv = "metric,value,unit,age_seconds\n";
  csv += "power_w," + numberOrNull(meter.powerW, 3) + ",W," + ageOrNull(meter.powerUpdatedMs) + "\n";
  csv += "import_kwh," + numberOrNull(meter.importKwh, 6) + ",kWh," + ageOrNull(meter.importUpdatedMs) + "\n";
  csv += "export_kwh," + numberOrNull(meter.exportKwh, 6) + ",kWh," + ageOrNull(meter.exportUpdatedMs) + "\n";
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const String prefix = "l" + String(phase + 1) + "_";
    csv += prefix + "power_w," + numberOrNull(meter.phasePowerW[phase], 3) +
           ",W," + ageOrNull(meter.phasePowerUpdatedMs[phase]) + "\n";
    csv += prefix + "voltage_v," + numberOrNull(meter.phaseVoltageV[phase], 3) +
           ",V," + ageOrNull(meter.phaseVoltageUpdatedMs[phase]) + "\n";
    csv += prefix + "current_a," + numberOrNull(meter.phaseCurrentA[phase], 4) +
           ",A," + ageOrNull(meter.phaseCurrentUpdatedMs[phase]) + "\n";
  }
  csv += "wifi_rssi," + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",dBm,\n";
  csv += "telegrams," + String(meter.telegrams) + ",count,\n";
  csv += "parse_errors," + String(meter.parseErrors) + ",count,\n";
  csv += "crc_errors," + String(meter.crcErrors) + ",count,\n";
  csv += "sml_crc_errors," + String(meter.smlCrcErrors) + ",count,\n";
  csv += "d0_bcc_errors," + String(d0BccErrors()) + ",count,\n";
  return csv;
}
