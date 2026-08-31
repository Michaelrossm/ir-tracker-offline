// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String mqttBaseTopic() {
  return "irtracker/" + deviceId;
}

void publishDiscoverySensor(const char *key, const char *name, const char *unit,
                            const char *deviceClass, const char *stateClass) {
  String payload = "{\"name\":\"" + String(name) + "\",\"unique_id\":\"" + deviceId + "_" + key +
                   "\",\"state_topic\":\"" + mqttBaseTopic() + "/state\",\"value_template\":\"{{ value_json." +
                   key + " }}\",\"availability_topic\":\"" + mqttBaseTopic() +
                   "/availability\",\"device\":{\"identifiers\":[\"" + deviceId +
                   "\"],\"name\":\"IR-Tracker Offline\",\"manufacturer\":\"Michael Roßmann / Community Firmware\","
                   "\"model\":\"PowerTracker IR\",\"sw_version\":\"" + kFirmwareVersion + "\"}";
  if (strlen(unit)) payload += ",\"unit_of_measurement\":\"" + String(unit) + "\"";
  if (strlen(deviceClass)) payload += ",\"device_class\":\"" + String(deviceClass) + "\"";
  if (strlen(stateClass)) payload += ",\"state_class\":\"" + String(stateClass) + "\"";
  payload += "}";
  mqtt.publish(("homeassistant/sensor/" + deviceId + "/" + key + "/config").c_str(), payload.c_str(), true);
}

void publishHomeAssistantDiscovery() {
  if (!config.homeAssistantDiscovery) return;
  publishDiscoverySensor("power_w", "Aktuelle Leistung", "W", "power", "measurement");
  publishDiscoverySensor("import_kwh", "Netzbezug", "kWh", "energy", "total_increasing");
  publishDiscoverySensor("export_kwh", "Einspeisung", "kWh", "energy", "total_increasing");
  publishDiscoverySensor("wifi_rssi", "WLAN Signal", "dBm", "signal_strength", "measurement");
}

void publishHomieMetadata() {
  const String root = "homie/" + deviceId;
  mqtt.publish((root + "/$homie").c_str(), "4.0.0", true);
  mqtt.publish((root + "/$name").c_str(), "IR-Tracker Offline", true);
  mqtt.publish((root + "/$state").c_str(), "init", true);
  mqtt.publish((root + "/$nodes").c_str(), "meter,network", true);
  mqtt.publish((root + "/meter/$name").c_str(), "Stromzaehler", true);
  mqtt.publish((root + "/meter/$properties").c_str(), "power,import,export,fresh,telegrams,crc-errors", true);
  struct Property { const char *id; const char *name; const char *type; const char *unit; };
  const Property properties[] = {
    {"power", "Aktuelle Leistung", "float", "W"},
    {"import", "Netzbezug", "float", "kWh"},
    {"export", "Einspeisung", "float", "kWh"},
    {"fresh", "Daten aktuell", "boolean", ""},
    {"telegrams", "Telegramme", "integer", ""},
    {"crc-errors", "CRC Fehler", "integer", ""}
  };
  for (const auto &property : properties) {
    const String base = root + "/meter/" + property.id;
    mqtt.publish((base + "/$name").c_str(), property.name, true);
    mqtt.publish((base + "/$datatype").c_str(), property.type, true);
    mqtt.publish((base + "/$settable").c_str(), "false", true);
    mqtt.publish((base + "/$retained").c_str(), "true", true);
    if (strlen(property.unit)) mqtt.publish((base + "/$unit").c_str(), property.unit, true);
  }
  mqtt.publish((root + "/network/$name").c_str(), "Netzwerk", true);
  mqtt.publish((root + "/network/$properties").c_str(),
               "transport,rssi,ssid,ip", true);
  mqtt.publish((root + "/network/transport/$name").c_str(),
               "Aktiver Netzwerkweg", true);
  mqtt.publish((root + "/network/transport/$datatype").c_str(), "enum", true);
  mqtt.publish((root + "/network/transport/$format").c_str(),
               "ethernet,wifi,setup_ap,offline", true);
  mqtt.publish((root + "/network/rssi/$name").c_str(), "WLAN Signal", true);
  mqtt.publish((root + "/network/rssi/$datatype").c_str(), "integer", true);
  mqtt.publish((root + "/network/rssi/$unit").c_str(), "dBm", true);
  mqtt.publish((root + "/network/ssid/$datatype").c_str(), "string", true);
  mqtt.publish((root + "/network/ip/$datatype").c_str(), "string", true);
  mqtt.publish((root + "/$state").c_str(), "ready", true);
}

void publishMqttValues() {
  // Build all recurring topics and values in reusable fixed buffers. The
  // discovery and Homie metadata paths run only after a reconnect and remain
  // unchanged; this removes heap churn from the five-second publish hotpath.
  char base[48];
  char homie[48];
  char root[64];
  char topic[96];
  char value[40];
  snprintf(base, sizeof(base), "irtracker/%s", deviceId.c_str());
  snprintf(homie, sizeof(homie), "homie/%s", deviceId.c_str());
  const auto publish = [&](const char *prefix, const char *suffix,
                           const char *payload) {
    const int length =
        snprintf(topic, sizeof(topic), "%s%s", prefix, suffix);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(topic))
      return false;
    return mqtt.publish(topic, payload, true);
  };
  const bool fresh = meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  const String status = statusJson();
  publish(base, "/state", status.c_str());
  if (std::isfinite(meter.powerW)) {
    snprintf(value, sizeof(value), "%.3f", meter.powerW);
    publish(base, "/power_w", value);
    publish(homie, "/meter/power", value);
  }
  if (std::isfinite(meter.importKwh)) {
    snprintf(value, sizeof(value), "%.6f", meter.importKwh);
    publish(base, "/import_kwh", value);
    publish(homie, "/meter/import", value);
  }
  if (std::isfinite(meter.exportKwh)) {
    snprintf(value, sizeof(value), "%.6f", meter.exportKwh);
    publish(base, "/export_kwh", value);
    publish(homie, "/meter/export", value);
  }
  for (uint8_t phase = 0; phase < 3; ++phase) {
    snprintf(root, sizeof(root), "%s/phase_l%u", base, phase + 1);
    if (std::isfinite(meter.phasePowerW[phase])) {
      snprintf(value, sizeof(value), "%.3f", meter.phasePowerW[phase]);
      publish(root, "/power_w", value);
    }
    if (std::isfinite(meter.phaseVoltageV[phase])) {
      snprintf(value, sizeof(value), "%.3f", meter.phaseVoltageV[phase]);
      publish(root, "/voltage_v", value);
    }
    if (std::isfinite(meter.phaseCurrentA[phase])) {
      snprintf(value, sizeof(value), "%.4f", meter.phaseCurrentA[phase]);
      publish(root, "/current_a", value);
    }
  }
  publish(homie, "/meter/fresh", fresh ? "true" : "false");
  snprintf(value, sizeof(value), "%lu",
           static_cast<unsigned long>(meter.telegrams));
  publish(homie, "/meter/telegrams", value);
  snprintf(value, sizeof(value), "%lu",
           static_cast<unsigned long>(meter.crcErrors));
  publish(homie, "/meter/crc-errors", value);
  publish(homie, "/network/transport", primaryTransportName());
  snprintf(value, sizeof(value), "%d",
           wifiConnected() ? WiFi.RSSI() : 0);
  publish(homie, "/network/rssi", value);
  const String ssid = wifiConnected() ? WiFi.SSID() : String();
  publish(homie, "/network/ssid", ssid.c_str());
  const String ip = primaryNetworkIp();
  publish(homie, "/network/ip", ip.c_str());
}

void manageMqtt() {
  if (!networkConnected() || !config.mqttHost.length()) {
    if (mqtt.connected()) mqtt.disconnect();
    mqttNetwork.stop();
    mqttRetryMs = 10000;
    return;
  }
  if (!mqtt.connected()) {
    if (millis() - lastMqttAttemptMs < mqttRetryMs) return;
    lastMqttAttemptMs = millis();
    mqttNetwork.stop();
    // DE: PubSubClient kann bei fehlendem Broker Sekunden blockieren; ein kurzer
    // TCP-Vortest schützt IR-Erfassung und Web-Latenz. | EN: PubSubClient may
    // block for seconds when the broker is missing; a short TCP preflight
    // protects IR capture and web latency.
    if (!mqttNetwork.connect(config.mqttHost.c_str(), config.mqttPort, 250)) {
      mqttRetryMs = std::min<uint32_t>(mqttRetryMs * 2, 60000);
      return;
    }
    const String availability = mqttBaseTopic() + "/availability";
    bool connected;
    if (config.mqttUser.length()) {
      connected = mqtt.connect(deviceId.c_str(), config.mqttUser.c_str(), config.mqttPassword.c_str(),
                               availability.c_str(), 0, true, "offline");
    } else {
      connected = mqtt.connect(deviceId.c_str(), availability.c_str(), 0, true, "offline");
    }
    if (!connected) {
      mqttNetwork.stop();
      mqttRetryMs = std::min<uint32_t>(mqttRetryMs * 2, 60000);
      return;
    }
    mqttRetryMs = 10000;
    mqtt.publish(availability.c_str(), "online", true);
    publishHomeAssistantDiscovery();
    publishHomieMetadata();
  }
  mqtt.loop();
  if (millis() - lastMqttPublishMs < kMqttPublishMs) return;
  lastMqttPublishMs = millis();
  publishMqttValues();
}
