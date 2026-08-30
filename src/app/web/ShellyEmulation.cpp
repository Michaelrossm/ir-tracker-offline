// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String shellyGen1Status() {
  const bool fresh = valueFresh(meter.powerUpdatedMs);
  String json = "{\"wifi_sta\":{\"connected\":" +
                String(WiFi.status() == WL_CONNECTED ? "true" : "false") +
                ",\"ssid\":\"" + jsonEscape(WiFi.SSID()) + "\",\"rssi\":" +
                String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) +
                "},\"has_update\":false,\"uptime\":" + String(millis() / 1000) +
                ",\"emeters\":[{\"power\":" + numberOrNull(meter.powerW, 2) +
                ",\"total\":" +
                (std::isfinite(meter.importKwh)
                     ? String(meter.importKwh * 1000.0, 3)
                     : "null") +
                ",\"total_returned\":" +
                (std::isfinite(meter.exportKwh)
                     ? String(meter.exportKwh * 1000.0, 3)
                     : "null") +
                ",\"is_valid\":" + String(fresh ? "true" : "false") +
                ",\"irtracker_age_s\":" + ageOrNull(meter.powerUpdatedMs) +
                "}]}";
  return json;
}

String shellyEmStatus() {
  const bool fresh = valueFresh(meter.powerUpdatedMs);
  String json = "{\"id\":0,\"total_act_power\":" +
                numberOrNull(meter.powerW, 2) +
                ",\"total_current\":null,\"total_aprt_power\":null,"
                "\"total_act_energy\":" +
                (std::isfinite(meter.importKwh)
                     ? String(meter.importKwh * 1000.0, 3)
                     : "null") +
                ",\"total_act_ret_energy\":" +
                (std::isfinite(meter.exportKwh)
                     ? String(meter.exportKwh * 1000.0, 3)
                     : "null");
  const char names[] = {'a', 'b', 'c'};
  for (uint8_t phase = 0; phase < 3; ++phase) {
    json += ",\"";
    json += names[phase];
    json += "_act_power\":" + numberOrNull(meter.phasePowerW[phase], 2) +
            ",\"";
    json += names[phase];
    json += "_voltage\":" + numberOrNull(meter.phaseVoltageV[phase], 2) +
            ",\"";
    json += "_current\":" + numberOrNull(meter.phaseCurrentA[phase], 3);
  }
  json += ",\"irtracker_age_s\":" + ageOrNull(meter.powerUpdatedMs) +
          ",\"errors\":" + String(fresh ? "[]" : "[\"meter_stale\"]") +
          "}";
  return json;
}
