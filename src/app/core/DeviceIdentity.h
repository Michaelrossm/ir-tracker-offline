#pragma once

#include <Arduino.h>
#include <esp_mac.h>

// One neutral identity for every integration. Compatibility describes the
// protocol only; the tracker never impersonates a third-party product.
struct DeviceIdentity {
  static constexpr const char *kModel = "IRTRACKER-C3";
  static constexpr const char *kShellyApiModel = "IRTRACKER-C3-3EM";

  char suffix[7] = {};
  char serial[11] = {};
  char hostname[24] = {};
  char instance[24] = {};
  char mqttId[24] = {};
  char mac[13] = {};

  void begin() {
    uint8_t address[6] = {};
    if (esp_read_mac(address, ESP_MAC_WIFI_STA) != ESP_OK) {
      const uint64_t chip = ESP.getEfuseMac();
      for (uint8_t i = 0; i < 6; ++i)
        address[5 - i] = static_cast<uint8_t>(chip >> (i * 8));
    }
    snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X", address[0],
             address[1], address[2], address[3], address[4], address[5]);
    snprintf(suffix, sizeof(suffix), "%02X%02X%02X", address[3], address[4],
             address[5]);
    snprintf(serial, sizeof(serial), "IRT-%s", suffix);
    snprintf(hostname, sizeof(hostname), "irtracker-%s", suffix);
    snprintf(instance, sizeof(instance), "IRTracker-%s", suffix);
    snprintf(mqttId, sizeof(mqttId), "irtracker_%s", suffix);
  }
};
