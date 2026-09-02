// This module is included by main.cpp inside its private namespace.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String shellyCompatId() {
  return deviceIdentity.hostname;
}

String shellyMac() {
  return deviceIdentity.mac;
}

String shellyDeviceInfo() {
  const bool authEnabled = !config.storageCompatibilityMode;
  return "{\"id\":\"" + shellyCompatId() + "\",\"mac\":\"" + shellyMac() +
         "\",\"serial\":\"" + String(deviceIdentity.serial) +
         "\",\"model\":\"" + DeviceIdentity::kShellyApiModel +
         "\",\"gen\":2,\"fw_id\":\"irtracker/" +
         String(kFirmwareVersion) + "\",\"ver\":\"" + kFirmwareVersion +
         "\",\"app\":\"IRTracker\",\"profile\":\"triphase\",\"auth_en\":" +
         String(authEnabled ? "true" : "false") + ",\"auth_domain\":" +
         (authEnabled ? "\"" + shellyCompatId() + "\"" : String("null")) +
         ",\"discoverable\":true}";
}

double shellyAverageVoltage() {
  double total = 0.0;
  uint8_t count = 0;
  for (double value : meter.phaseVoltageV) {
    if (!std::isfinite(value)) continue;
    total += value;
    ++count;
  }
  return count ? total / count : NAN;
}

double shellyTotalCurrent() {
  double total = 0.0;
  bool available = false;
  for (double value : meter.phaseCurrentA) {
    if (!std::isfinite(value)) continue;
    total += value;
    available = true;
  }
  return available ? total : NAN;
}

String shellyGen1Emeter() {
  const bool fresh = valueFresh(meter.powerUpdatedMs);
  return "{\"power\":" + numberOrNull(meter.powerW, 2) +
         ",\"reactive\":null,\"voltage\":" +
         numberOrNull(shellyAverageVoltage(), 2) + ",\"is_valid\":" +
         String(fresh ? "true" : "false") + ",\"total\":" +
         (std::isfinite(meter.importKwh) ? String(meter.importKwh * 1000.0, 3)
                                         : "null") +
         ",\"total_returned\":" +
         (std::isfinite(meter.exportKwh) ? String(meter.exportKwh * 1000.0, 3)
                                         : "null") +
         ",\"irtracker_age_s\":" + ageOrNull(meter.powerUpdatedMs) + "}";
}

String shellyGen1Status() {
  return "{\"wifi_sta\":{\"connected\":" +
         String(networkConnected() ? "true" : "false") +
         ",\"ssid\":\"" + jsonEscape(WiFi.SSID()) + "\",\"ip\":\"" +
         primaryNetworkIp() + "\",\"rssi\":" +
         String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) +
         "},\"has_update\":false,\"uptime\":" + String(millis() / 1000) +
         ",\"total_power\":" + numberOrNull(meter.powerW, 2) +
         ",\"emeters\":[" + shellyGen1Emeter() + "]}";
}

String shellyEmStatus() {
  const bool fresh = valueFresh(meter.powerUpdatedMs);
  String json = "{\"id\":0";
  const char names[] = {'a', 'b', 'c'};
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const double apparent =
        std::isfinite(meter.phaseVoltageV[phase]) &&
                std::isfinite(meter.phaseCurrentA[phase])
            ? meter.phaseVoltageV[phase] * meter.phaseCurrentA[phase]
            : NAN;
    const double pf = std::isfinite(apparent) && apparent > 0.001 &&
                              std::isfinite(meter.phasePowerW[phase])
                          ? meter.phasePowerW[phase] / apparent
                          : NAN;
    json += ",\"";
    json += names[phase];
    json += "_current\":" + numberOrNull(meter.phaseCurrentA[phase], 3) + ",\"";
    json += names[phase];
    json += "_voltage\":" + numberOrNull(meter.phaseVoltageV[phase], 2) + ",\"";
    json += names[phase];
    json += "_act_power\":" + numberOrNull(meter.phasePowerW[phase], 2) + ",\"";
    json += names[phase];
    json += "_aprt_power\":" + numberOrNull(apparent, 2) + ",\"";
    json += names[phase];
    json += "_pf\":" + numberOrNull(pf, 3) + ",\"";
    json += names[phase];
    json += "_freq\":null,\"";
    json += names[phase];
    json += "_errors\":[],\"";
    json += names[phase];
    json += "_flags\":[]";
  }
  double totalApparent = 0.0;
  bool apparentAvailable = false;
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (!std::isfinite(meter.phaseVoltageV[phase]) ||
        !std::isfinite(meter.phaseCurrentA[phase]))
      continue;
    totalApparent += meter.phaseVoltageV[phase] * meter.phaseCurrentA[phase];
    apparentAvailable = true;
  }
  json += ",\"n_current\":null,\"total_current\":" +
          numberOrNull(shellyTotalCurrent(), 3) +
          ",\"total_act_power\":" + numberOrNull(meter.powerW, 2) +
          ",\"total_aprt_power\":" +
          numberOrNull(apparentAvailable ? totalApparent : NAN, 2) +
          ",\"user_calibrated_phase\":[],"
          "\"irtracker_age_s\":" + ageOrNull(meter.powerUpdatedMs) +
          ",\"errors\":" + String(fresh ? "[]" : "[\"meter_stale\"]") + "}";
  return json;
}

String shellyEmDataStatus() {
  const String imported = std::isfinite(meter.importKwh)
                              ? String(meter.importKwh * 1000.0, 3)
                              : "null";
  const String exported = std::isfinite(meter.exportKwh)
                              ? String(meter.exportKwh * 1000.0, 3)
                              : "null";
  return "{\"id\":0,\"a_total_act_energy\":null,"
         "\"a_total_act_ret_energy\":null,\"b_total_act_energy\":null,"
         "\"b_total_act_ret_energy\":null,\"c_total_act_energy\":null,"
         "\"c_total_act_ret_energy\":null,\"total_act\":" + imported +
         ",\"total_act_ret\":" + exported + "}";
}

String shellyRpcStatus() {
  return "{\"sys\":{\"uptime\":" + String(millis() / 1000) +
         "},\"wifi\":{\"sta_ip\":\"" + WiFi.localIP().toString() +
         "\",\"rssi\":" + String(wifiConnected() ? WiFi.RSSI() : 0) +
         "},\"eth\":{\"ip\":" +
         (ethernet.connected() ? "\"" + ethernet.localIP().toString() + "\""
                               : String("null")) +
         "},\"em:0\":" + shellyEmStatus() +
         ",\"emdata:0\":" + shellyEmDataStatus() + "}";
}

String shellyMethodList() {
  return "{\"methods\":[\"Shelly.GetDeviceInfo\",\"Shelly.GetStatus\","
         "\"Shelly.ListMethods\",\"EM.GetStatus\",\"EMData.GetStatus\"]}";
}

String shellyRpcResult(const String &method) {
  if (method == "Shelly.GetDeviceInfo") return shellyDeviceInfo();
  if (method == "Shelly.GetStatus") return shellyRpcStatus();
  if (method == "Shelly.ListMethods") return shellyMethodList();
  if (method == "EM.GetStatus") return shellyEmStatus();
  if (method == "EMData.GetStatus") return shellyEmDataStatus();
  return "";
}

void handleShellyRpc() {
  if (!requireStorageCompatibilityAccess()) return;
  if (server.arg("plain").length() > 1024) {
    sendIntegrationJson(
        "{\"error\":{\"code\":-32600,\"message\":\"Request too large\"}}",
        413);
    return;
  }
  StaticJsonDocument<512> request;
  const DeserializationError error = deserializeJson(request, server.arg("plain"));
  if (error || !request["method"].is<const char *>()) {
    sendIntegrationJson(
        "{\"error\":{\"code\":-32700,\"message\":\"Invalid JSON-RPC request\"}}",
        400);
    return;
  }
  const String result = shellyRpcResult(request["method"].as<String>());
  String id;
  if (request["id"].isNull())
    id = "null";
  else
    serializeJson(request["id"], id);
  String response = "{\"id\":" + id + ",\"src\":\"" + shellyCompatId() + "\",";
  if (result.length()) {
    sendIntegrationJson(response + "\"result\":" + result + "}");
  } else {
    sendIntegrationJson(
        response + "\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}",
        404);
  }
}
