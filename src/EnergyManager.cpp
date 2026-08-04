#include "EnergyManager.h"

#include <algorithm>
#include <cmath>

int32_t EnergyManager::calculateTarget(double gridW) const {
  if (!std::isfinite(gridW)) return 0;
  double requested = gridW - config_.targetGridW;
  if (std::abs(requested) <= config_.deadbandW) return 0;
  if (config_.inverted) requested = -requested;
  return constrain(static_cast<int32_t>(lround(requested)),
                   -static_cast<int32_t>(config_.maxChargeW),
                   static_cast<int32_t>(config_.maxDischargeW));
}

int32_t EnergyManager::applyRamp(int32_t target) const {
  if (!status_.lastAttemptMs || !config_.rampWPerSecond) return target;
  const uint32_t elapsed = millis() - status_.lastAttemptMs;
  const int32_t step =
      std::max<int32_t>(1, config_.rampWPerSecond * elapsed / 1000);
  return constrain(target, status_.sentW - step, status_.sentW + step);
}

void EnergyManager::update(double gridW, bool meterFresh,
                           const MqttPublish &mqttPublish) {
  if (!config_.enabled || config_.driver == Driver::Disabled) {
    status_.failsafe = true;
    status_.message = "deaktiviert";
    return;
  }
  if (millis() - status_.lastAttemptMs < config_.intervalMs) return;
  const bool stale = !meterFresh;
  status_.failsafe = stale || faultLatched_;
  status_.requestedW =
      (stale || faultLatched_) ? 0 : calculateTarget(gridW);
  const int32_t command =
      (stale || faultLatched_) ? 0 : applyRamp(status_.requestedW);
  status_.lastAttemptMs = millis();
  if (config_.dryRun) {
    status_.sentW = command;
    status_.lastSuccessMs = millis();
    status_.message =
        (stale || faultLatched_) ? "Trockenlauf: Sicherheitsstopp"
                                 : "Trockenlauf: keine Ausgabe";
    return;
  }
  if (sendCommand(command, gridW, mqttPublish)) {
    status_.sentW = command;
    status_.lastSuccessMs = millis();
    status_.message =
        (stale || faultLatched_) ? "Sicherheitsstopp gesendet"
                                 : "Sollwert gesendet";
  } else {
    ++status_.failures;
    faultLatched_ = true;
    status_.failsafe = true;
    status_.message = "Kommunikationsfehler: Null-Sollwert wird erzwungen";
    sendCommand(0, NAN, mqttPublish);
  }
}

void EnergyManager::stop(const MqttPublish &mqttPublish) {
  if (config_.enabled && !config_.dryRun &&
      config_.driver != Driver::Disabled) {
    sendCommand(0, NAN, mqttPublish);
  }
  status_.requestedW = 0;
  status_.sentW = 0;
  status_.failsafe = true;
  status_.message = "manuell gestoppt";
}

bool EnergyManager::sendCommand(int32_t powerW, double gridW,
                                const MqttPublish &mqttPublish) {
  switch (config_.driver) {
    case Driver::ModbusTcp:
      return sendModbus(powerW);
    case Driver::Mqtt:
      return sendMqtt(powerW, mqttPublish);
    case Driver::Http:
      return sendHttp(powerW, gridW);
    default:
      return false;
  }
}

bool EnergyManager::readExact(WiFiClient &client, uint8_t *buffer,
                              size_t length, uint32_t timeoutMs) {
  const uint32_t started = millis();
  size_t received = 0;
  while (received < length && millis() - started < timeoutMs) {
    while (client.available() && received < length)
      buffer[received++] = client.read();
    delay(1);
  }
  return received == length;
}

bool EnergyManager::sendModbus(int32_t powerW) {
  if (!config_.host.length() || !std::isfinite(config_.registerScale) ||
      config_.registerScale == 0) {
    return false;
  }
  WiFiClient client;
  client.setTimeout(400);
  if (!client.connect(config_.host.c_str(), config_.port, 400)) return false;
  const int32_t raw = lround(powerW / config_.registerScale);
  uint8_t pdu[13] = {};
  const uint16_t transaction = transactionId_++;
  pdu[0] = transaction >> 8;
  pdu[1] = transaction & 0xff;
  pdu[5] = config_.registerWidth == 2 ? 11 : 6;
  pdu[6] = config_.unitId;
  pdu[7] = config_.registerWidth == 2 ? 0x10 : 0x06;
  pdu[8] = config_.powerRegister >> 8;
  pdu[9] = config_.powerRegister & 0xff;
  size_t requestLength = 12;
  if (config_.registerWidth == 2) {
    pdu[10] = 0;
    pdu[11] = 2;
    pdu[12] = 4;
    uint8_t request[17];
    memcpy(request, pdu, 13);
    const uint16_t high = static_cast<uint32_t>(raw) >> 16;
    const uint16_t low = raw & 0xffff;
    const uint16_t first = config_.wordSwap ? low : high;
    const uint16_t second = config_.wordSwap ? high : low;
    request[13] = first >> 8;
    request[14] = first & 0xff;
    request[15] = second >> 8;
    request[16] = second & 0xff;
    if (client.write(request, sizeof(request)) != sizeof(request)) return false;
  } else {
    const int16_t value = constrain(raw, -32768, 32767);
    pdu[10] = static_cast<uint16_t>(value) >> 8;
    pdu[11] = static_cast<uint16_t>(value) & 0xff;
    if (client.write(pdu, requestLength) != requestLength) return false;
  }
  uint8_t response[12];
  if (!readExact(client, response, sizeof(response), 400)) return false;
  return response[0] == (transaction >> 8) &&
         response[1] == (transaction & 0xff) && response[2] == 0 &&
         response[3] == 0 && response[6] == config_.unitId &&
         !(response[7] & 0x80);
}

bool EnergyManager::sendMqtt(int32_t powerW,
                             const MqttPublish &mqttPublish) {
  return config_.mqttTopic.length() &&
         mqttPublish(config_.mqttTopic,
                     renderPayload(config_.mqttPayload, powerW, NAN));
}

bool EnergyManager::sendHttp(int32_t powerW, double gridW) {
  if (!config_.host.length() || !config_.httpPath.startsWith("/")) return false;
  WiFiClient client;
  client.setTimeout(400);
  if (!client.connect(config_.host.c_str(), config_.port, 400)) return false;
  const String payload =
      renderPayload(config_.httpPayload, powerW, gridW);
  const String method = config_.httpMethod == "PUT" ? "PUT" : "POST";
  String request = method + " " + config_.httpPath +
                   " HTTP/1.1\r\nHost: " + config_.host +
                   "\r\nContent-Type: application/json\r\n";
  if (config_.httpBearerToken.length())
    request += "Authorization: Bearer " + config_.httpBearerToken + "\r\n";
  request += "Connection: close\r\nContent-Length: " +
             String(payload.length()) + "\r\n\r\n" + payload;
  client.print(request);
  String statusLine = client.readStringUntil('\n');
  return statusLine.indexOf(" 200 ") > 0 || statusLine.indexOf(" 202 ") > 0 ||
         statusLine.indexOf(" 204 ") > 0;
}

String EnergyManager::renderPayload(const String &pattern, int32_t powerW,
                                    double gridW) const {
  String payload = pattern;
  payload.replace("{power}", String(powerW));
  payload.replace("{grid}",
                  std::isfinite(gridW) ? String(gridW, 2) : "null");
  return payload;
}
