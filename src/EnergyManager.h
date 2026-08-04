#pragma once

#include <Arduino.h>
#include <WiFiClient.h>

#include <functional>

class EnergyManager {
 public:
  enum class Driver : uint8_t { Disabled, ModbusTcp, Mqtt, Http };

  struct Config {
    Driver driver = Driver::Disabled;
    bool enabled = false;
    bool dryRun = true;
    bool inverted = false;
    String host;
    uint16_t port = 502;
    uint8_t unitId = 1;
    uint16_t powerRegister = 0;
    uint8_t registerWidth = 1;
    bool wordSwap = false;
    float registerScale = 1.0f;
    String mqttTopic;
    String mqttPayload = "{power}";
    String httpPath = "/api/power";
    String httpMethod = "POST";
    String httpPayload =
        "{\"setpoint_w\":{power},\"grid_w\":{grid}}";
    String httpBearerToken;
    int16_t targetGridW = 0;
    uint16_t deadbandW = 30;
    uint16_t maxChargeW = 800;
    uint16_t maxDischargeW = 800;
    uint16_t rampWPerSecond = 200;
    uint16_t intervalMs = 2000;
    uint16_t staleMs = 10000;
  };

  struct Status {
    int32_t requestedW = 0;
    int32_t sentW = 0;
    uint32_t lastAttemptMs = 0;
    uint32_t lastSuccessMs = 0;
    uint32_t failures = 0;
    bool failsafe = true;
    String message = "deaktiviert";
  };

  using MqttPublish =
      std::function<bool(const String &topic, const String &payload)>;

  void configure(const Config &config) {
    config_ = config;
    faultLatched_ = false;
  }
  void update(double gridW, bool meterFresh, const MqttPublish &mqttPublish);
  void stop(const MqttPublish &mqttPublish);
  const Config &config() const { return config_; }
  const Status &status() const { return status_; }

 private:
  Config config_;
  Status status_;
  uint16_t transactionId_ = 1;
  bool faultLatched_ = false;

  int32_t calculateTarget(double gridW) const;
  int32_t applyRamp(int32_t target) const;
  bool sendCommand(int32_t powerW, double gridW,
                   const MqttPublish &mqttPublish);
  bool sendModbus(int32_t powerW);
  bool sendMqtt(int32_t powerW, const MqttPublish &mqttPublish);
  bool sendHttp(int32_t powerW, double gridW);
  String renderPayload(const String &pattern, int32_t powerW,
                       double gridW) const;
  bool readExact(WiFiClient &client, uint8_t *buffer, size_t length,
                 uint32_t timeoutMs);
};
