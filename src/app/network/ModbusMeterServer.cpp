// DE: Kleiner herstellerneutraler Modbus-TCP-Messwertserver.
// EN: Small vendor-neutral Modbus TCP meter server.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

constexpr uint16_t kModbusMeterPort = 502;
constexpr uint16_t kModbusRegisterCount = 28;
constexpr uint32_t kModbusClientTimeoutMs = 1000;

WiFiServer modbusMeterServer(kModbusMeterPort);
WiFiClient modbusMeterClient;
bool modbusMeterServerRunning = false;
uint8_t modbusRequest[260] = {};
size_t modbusRequestLength = 0;
uint32_t modbusClientLastDataMs = 0;
uint32_t modbusConnectionCount = 0;
uint32_t modbusValidRequestCount = 0;
uint32_t modbusInvalidRequestCount = 0;
IPAddress modbusLastClient;
bool modbusLastClientKnown = false;

void setModbusU32(uint16_t *registers, const uint16_t address,
                  const uint32_t value) {
  registers[address] = static_cast<uint16_t>(value >> 16U);
  registers[address + 1U] = static_cast<uint16_t>(value & 0xffffU);
}

uint32_t scaledModbusSigned(const double value, const double scale) {
  if (!std::isfinite(value)) return 0x80000000U;
  const double scaled = value * scale;
  if (scaled > 2147483647.0) return 0x7fffffffU;
  if (scaled < -2147483648.0) return 0x80000001U;
  return static_cast<uint32_t>(static_cast<int32_t>(std::lround(scaled)));
}

uint32_t scaledModbusUnsigned(const double value, const double scale) {
  if (!std::isfinite(value) || value < 0.0) return 0xffffffffU;
  const double scaled = value * scale;
  if (scaled >= 4294967295.0) return 0xfffffffeU;
  return static_cast<uint32_t>(std::llround(scaled));
}

void buildModbusMeterRegisters(uint16_t *registers) {
  std::fill(registers, registers + kModbusRegisterCount, 0U);
  registers[0] = 1;  // irtracker.meter.v1
  uint16_t available = valueFresh(meter.powerUpdatedMs) ? 0x8000U : 0U;
  if (std::isfinite(meter.powerW)) available |= 1U << 0U;
  if (std::isfinite(meter.importKwh)) available |= 1U << 1U;
  if (std::isfinite(meter.exportKwh)) available |= 1U << 2U;
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (std::isfinite(meter.phasePowerW[phase])) available |= 1U << (3U + phase);
    if (std::isfinite(meter.phaseVoltageV[phase])) available |= 1U << (6U + phase);
    if (std::isfinite(meter.phaseCurrentA[phase])) available |= 1U << (9U + phase);
  }
  registers[1] = available;
  setModbusU32(registers, 2, scaledModbusSigned(meter.powerW, 100.0));
  setModbusU32(registers, 4, scaledModbusUnsigned(meter.importKwh, 1000.0));
  setModbusU32(registers, 6, scaledModbusUnsigned(meter.exportKwh, 1000.0));
  const time_t now = time(nullptr);
  setModbusU32(registers, 8, now >= 1700000000 ? static_cast<uint32_t>(now)
                                                : 0xffffffffU);
  for (uint8_t phase = 0; phase < 3; ++phase) {
    setModbusU32(registers, 10U + phase * 2U,
                 scaledModbusSigned(meter.phasePowerW[phase], 100.0));
    setModbusU32(registers, 16U + phase * 2U,
                 scaledModbusUnsigned(meter.phaseVoltageV[phase], 100.0));
    setModbusU32(registers, 22U + phase * 2U,
                 scaledModbusUnsigned(meter.phaseCurrentA[phase], 1000.0));
  }
}

void sendModbusException(const uint8_t function, const uint8_t exception) {
  ++modbusInvalidRequestCount;
  uint8_t response[9] = {modbusRequest[0], modbusRequest[1], 0, 0, 0, 3,
                         modbusRequest[6], static_cast<uint8_t>(function | 0x80U),
                         exception};
  modbusMeterClient.write(response, sizeof(response));
}

void processModbusRequest() {
  const uint8_t function = modbusRequest[7];
  if (modbusRequest[2] || modbusRequest[3]) {
    sendModbusException(function, 0x01);
    return;
  }
  if (function != 0x03 && function != 0x04) {
    sendModbusException(function, 0x01);
    return;
  }
  if (modbusRequestLength != 12U) {
    sendModbusException(function, 0x03);
    return;
  }
  const uint16_t address =
      static_cast<uint16_t>(modbusRequest[8] << 8U) | modbusRequest[9];
  const uint16_t count =
      static_cast<uint16_t>(modbusRequest[10] << 8U) | modbusRequest[11];
  if (!count || count > 32U || address >= kModbusRegisterCount ||
      static_cast<uint32_t>(address) + count > kModbusRegisterCount) {
    sendModbusException(function, 0x02);
    return;
  }
  ++modbusValidRequestCount;
  uint16_t registers[kModbusRegisterCount];
  buildModbusMeterRegisters(registers);
  uint8_t response[9U + 64U] = {};
  response[0] = modbusRequest[0];
  response[1] = modbusRequest[1];
  response[4] = static_cast<uint8_t>((3U + count * 2U) >> 8U);
  response[5] = static_cast<uint8_t>((3U + count * 2U) & 0xffU);
  response[6] = modbusRequest[6];
  response[7] = function;
  response[8] = static_cast<uint8_t>(count * 2U);
  for (uint16_t index = 0; index < count; ++index) {
    response[9U + index * 2U] =
        static_cast<uint8_t>(registers[address + index] >> 8U);
    response[10U + index * 2U] =
        static_cast<uint8_t>(registers[address + index] & 0xffU);
  }
  modbusMeterClient.write(response, 9U + count * 2U);
}

void stopModbusMeterServer() {
  if (modbusMeterClient) modbusMeterClient.stop();
  modbusRequestLength = 0;
  if (modbusMeterServerRunning) modbusMeterServer.end();
  modbusMeterServerRunning = false;
}

bool modbusMeterRunning() { return modbusMeterServerRunning; }
uint32_t modbusMeterConnections() { return modbusConnectionCount; }
uint32_t modbusMeterValidRequests() { return modbusValidRequestCount; }
uint32_t modbusMeterInvalidRequests() { return modbusInvalidRequestCount; }
String modbusMeterLastClient() {
  return modbusLastClientKnown ? modbusLastClient.toString() : String();
}

void manageModbusMeterServer() {
  if (!config.modbusTcp || !networkConnected()) {
    stopModbusMeterServer();
    return;
  }
  if (!modbusMeterServerRunning) {
    modbusMeterServer.begin();
    modbusMeterServer.setNoDelay(true);
    modbusMeterServerRunning = true;
    eventLog.add("INFO", "MODBUS_STARTED", "Modbus TCP read-only auf Port 502");
  }
  if (!modbusMeterClient || !modbusMeterClient.connected()) {
    if (modbusMeterClient) modbusMeterClient.stop();
    modbusMeterClient = modbusMeterServer.available();
    modbusRequestLength = 0;
    if (!modbusMeterClient) return;
    ++modbusConnectionCount;
    modbusLastClient = modbusMeterClient.remoteIP();
    modbusLastClientKnown = true;
    if (!isPrivateLocalAddress(modbusMeterClient.remoteIP())) {
      modbusMeterClient.stop();
      return;
    }
    modbusClientLastDataMs = millis();
  }
  while (modbusMeterClient.available() &&
         modbusRequestLength < sizeof(modbusRequest)) {
    modbusRequest[modbusRequestLength++] =
        static_cast<uint8_t>(modbusMeterClient.read());
    modbusClientLastDataMs = millis();
  }
  if (modbusRequestLength >= 6U) {
    const size_t expected =
        6U + (static_cast<size_t>(modbusRequest[4]) << 8U) + modbusRequest[5];
    if (expected < 8U || expected > sizeof(modbusRequest)) {
      ++modbusInvalidRequestCount;
      modbusMeterClient.stop();
      modbusRequestLength = 0;
      return;
    }
    if (modbusRequestLength >= expected) {
      modbusRequestLength = expected;
      processModbusRequest();
      modbusRequestLength = 0;
    }
  }
  if (modbusMeterClient &&
      millis() - modbusClientLastDataMs > kModbusClientTimeoutMs) {
    if (modbusRequestLength) ++modbusInvalidRequestCount;
    modbusMeterClient.stop();
    modbusRequestLength = 0;
  }
}
