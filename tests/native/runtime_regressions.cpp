// Execute the real parser and Modbus implementation with a simulated TCP peer.
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <vector>
#include "../../src/app/meter/SmlParser.cpp"
#include "../../src/app/storage/HistoryStore.cpp"
#include "../../src/app/core/EventLog.cpp"

struct IPAddress {
  String toString() const { return "192.168.1.2"; }
};
struct Connection {
  std::vector<uint8_t> incoming;
  std::vector<std::vector<uint8_t>> outgoing;
  size_t position = 0;
  bool open = true;
};
struct WiFiClient {
  std::shared_ptr<Connection> channel;
  explicit operator bool() const { return channel && channel->open; }
  bool connected() const { return bool(*this); }
  void stop() { if (channel) channel->open = false; }
  IPAddress remoteIP() const { return {}; }
  int available() const {
    return connected() ? int(channel->incoming.size() - channel->position) : 0;
  }
  int read(uint8_t *data, size_t count) {
    count = std::min<size_t>(count, available());
    std::copy_n(channel->incoming.data() + channel->position, count, data);
    channel->position += count;
    return int(count);
  }
  size_t write(const uint8_t *data, size_t count) {
    channel->outgoing.emplace_back(data, data + count);
    return count;
  }
};
struct WiFiServer {
  explicit WiFiServer(uint16_t) {}
  void begin() {}
  void end() {}
  void setNoDelay(bool) {}
  WiFiClient available() { return {}; }
};
struct { bool modbusTcp = true; } config;
struct { void add(const char *, const char *, const char *) {} } eventLog;
MeterData meter;
bool networkConnected() { return true; }
bool valueFresh(uint32_t timestamp) { return timestamp != 0; }
bool isPrivateLocalAddress(IPAddress) { return true; }
#define IR_TRACKER_AMALGAMATED_BUILD 1
#include "../../src/app/network/ModbusMeterServer.cpp"

constexpr size_t MQTT_MAX_HEADER_SIZE = 5;
constexpr uint32_t kReadingStaleMs = 15000;
String deviceId = "test-meter";
unsigned meterServices = 0;
void serviceMeterInput() { ++meterServices; }
bool wifiConnected() { return true; }
const char *primaryTransportName() { return "wifi"; }
String primaryNetworkIp() { return "192.168.1.2"; }
String statusJson() { return String(5000, 's'); }
String neutralMeterJson() { return String(600, 'm'); }
struct {
  int RSSI() { return -50; }
  String SSID() { return "test"; }
} WiFi;
WiFiClient mqttNetwork;
struct {
  std::vector<std::pair<String, String>> messages;
  size_t expected = 0;
  bool failHeader = false;
  bool failWrite = false;
  size_t getBufferSize() { return 1024; }
  bool publish(const char *topic, const char *value, bool retained) {
    assert(retained);
    if (!mqttNetwork.connected()) return false;
    assert(strlen(topic) + strlen(value) + 7 <= getBufferSize());
    messages.emplace_back(topic, value);
    return true;
  }
  bool beginPublish(const char *topic, size_t length, bool retained) {
    assert(retained);
    if (failHeader) return false;
    expected = length;
    messages.emplace_back(topic, "");
    return true;
  }
  size_t write(const uint8_t *data, size_t length) {
    if (failWrite) return 0;
    messages.back().second.append(reinterpret_cast<const char *>(data), length);
    return length;
  }
  int endPublish() { assert(messages.back().second.size() == expected); return 1; }
} mqtt;
#include "runtime_selected.inc"

void testMqttAndJson() {
  String raw;
  for (int c = 0; c < 32; ++c) raw += char(c);
  const String escaped = jsonEscape(raw);
  assert(escaped.size() == 32 * 6);
  assert(escaped.substr(0, 6) == "\\u0000");
  assert(escaped.substr(31 * 6) == "\\u001f");
  assert(jsonEscape("a\"b\\c") == "a\\\"b\\\\c");
  mqttNetwork.channel = std::make_shared<Connection>();
  meter.powerW = 123;
  publishMqttValues();
  assert(mqtt.messages[0].first == "irtracker/test-meter/state");
  assert(mqtt.messages[0].second == statusJson());
  assert(mqtt.messages[1].first == "irtracker/test-meter/meter");
  assert(mqtt.messages[1].second == neutralMeterJson());
  assert(meterServices > 10);
  mqtt.failWrite = true;
  publishMqttValues();
  assert(!mqttNetwork.connected());
  mqttNetwork.channel = std::make_shared<Connection>();
  mqtt.failWrite = false;
  mqtt.failHeader = true;
  publishMqttValues();
  assert(!mqttNetwork.connected());
}

std::shared_ptr<Connection> peer() {
  auto connection = std::make_shared<Connection>();
  modbusMeterClient.channel = connection;
  modbusRequestLength = 0;
  modbusClientLastDataMs = millis();
  return connection;
}
const std::vector<uint8_t> request = {0, 1, 0, 0, 0, 6, 1, 3, 0, 0, 0, 2};

void testModbus() {
  for (size_t split = 0; split <= request.size(); ++split) {
    auto c = peer();
    c->incoming.assign(request.begin(), request.begin() + split);
    manageModbusMeterServer();
    c->incoming.insert(c->incoming.end(), request.begin() + split, request.end());
    manageModbusMeterServer();
    assert(c->outgoing.size() == 1);
    assert(c->outgoing[0].size() == 13 && c->outgoing[0][7] == 3);
  }
  auto c = peer();
  for (int i = 0; i < 3; ++i)
    c->incoming.insert(c->incoming.end(), request.begin(), request.end());
  for (int i = 0; i < 3; ++i) {
    manageModbusMeterServer();
    assert(c->outgoing.size() == size_t(i + 1));
    assert(c->position == request.size() * size_t(i + 1));
  }
  for (unsigned length : {0U, 1U, 255U, 65535U}) {
    c = peer();
    c->incoming = {0, 0, 0, 0, uint8_t(length >> 8), uint8_t(length)};
    manageModbusMeterServer();
    assert(!c->open && c->outgoing.empty());
  }
  c = peer();
  c->incoming = request;
  c->incoming[7] = 6; // Writes remain unsupported.
  manageModbusMeterServer();
  assert(c->outgoing[0][7] == 0x86 && c->outgoing[0][8] == 1);
  assert(scaledModbusUnsigned(NAN, 1) == 0xffffffffU);
  assert(scaledModbusUnsigned(4294967294.8, 1) == 0xfffffffeU);
  assert(scaledModbusSigned(-2147483647.8, 1) == 0x80000001U);
  assert(scaledModbusSigned(NAN, 1) == 0x80000000U);
}

std::vector<uint8_t> telegramPayload() {
  std::vector<uint8_t> data(800, 0x11);
  for (const auto &spec : kSpecs) {
    data.insert(data.end(), {0x77, 0x07});
    data.insert(data.end(), spec.code, spec.code + 6);
    data.insert(data.end(), {1, 0x62, 0x1b, 0x52, 0xff,
                            0x55, 0, 0, 0x12, 0x34, 1});
  }
  return data;
}

void testSml() {
  auto data = telegramPayload();
  MeterData oldValues, newValues;
  assert(parseLegacy(data, oldValues));
  assert(parseOnePass(data, newValues));
  assert(sameMeterData(oldValues, newValues));
  assert(std::fabs(newValues.powerW - 466.0) < 1e-8);
  for (int scaler = -9; scaler <= 9; ++scaler) {
    std::vector<uint8_t> entry = {1, 0, 16, 7, 0, 255, 1, 0x62, 0x1b,
                                0x52, uint8_t(scaler), 0x55, 0, 0, 0x12, 0x34, 1};
    double value;
    assert(decodeLegacyWindow(entry, 0, value));
    const double expected = 4660.0 * std::pow(10.0, scaler);
    assert(std::fabs(value - expected) <= std::fabs(expected) * 1e-12);
  }
  uint32_t random = 1;
  for (unsigned trial = 0; trial < 500; ++trial) {
    std::vector<uint8_t> noise(1500);
    for (auto &byte : noise) {
      random = random * 1664525U + 1013904223U;
      byte = uint8_t(random >> 24);
    }
    if (trial % 2) noise.insert(noise.end(), data.begin() + 800, data.end());
    MeterData a, b;
    assert(parseOnePass(noise, a) == parseLegacy(noise, b));
    assert(sameMeterData(a, b));
  }
  assert(!readSmlNumber({0xda, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0).valid);
  assert(!readSmlNumber({0x5a, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0).valid);
  SmlParser parser;
  MeterParseResult result;
  std::vector<uint8_t> oversize(kSmlStart, kSmlStart + sizeof(kSmlStart));
  oversize.resize(2049, 0x11);
  assert(parser.feed(oversize.data(), oversize.size(), result) ==
         MeterParseStatus::InvalidFrame);
  std::vector<uint8_t> frame(kSmlStart, kSmlStart + sizeof(kSmlStart));
  frame.insert(frame.end(), data.begin(), data.end());
  frame.insert(frame.end(), kSmlEnd, kSmlEnd + sizeof(kSmlEnd));
  frame.push_back(0);
  uint16_t crc = smlCrc16(frame.data(), frame.size());
  frame.push_back(uint8_t(crc)); frame.push_back(uint8_t(crc >> 8));
  for (unsigned i = 0; i < 600; ++i)
    assert(parser.feed(frame.data(), frame.size(), result) == MeterParseStatus::Valid);
  assert(parser.diagnostics().onePassActive);
  assert(parser.diagnostics().sentinelComparisons == 1);
  assert(!parser.diagnostics().legacyFallbackLatched);
  frame.back() ^= 1;
  assert(parser.feed(frame.data(), frame.size(), result) == MeterParseStatus::IntegrityError);
}

void testHistoryFailuresAndWrap() {
  LittleFS.files.clear();
  HistoryStore history;
  assert(history.begin());
  constexpr auto tier = HistoryStore::Tier::Minute;
  const uint32_t start = 1700000040;
  history.update(start, 100, 10, 0);
  failedWriteAfter = 1; // Record succeeds, header fails.
  history.update(start + 60, 200, 11, 0);
  assert(history.count(tier) == 0);
  failedWriteAfter = -1;
  history.update(start + 60, 200, 11, 0);
  assert(history.count(tier) == 1);
  assert(history.flushPending(tier));
  unsigned returned = 0;
  history.forEach(tier, 0, UINT32_MAX, [&](const HistoryStore::Record &record) {
    assert(record.averageW == (returned == 0 ? 100 : 200));
    ++returned;
    return true;
  });
  assert(returned == 2);
  assert(history.clear(tier));
  for (uint32_t index = 0; index < 3000; ++index) {
    const HistoryStore::Record record = {start + index * 60, 100, 100, 100, 10, 0};
    assert(history.importRecord(tier, record));
  }
  for (const auto range : std::vector<std::pair<uint32_t, uint32_t>>{
           {0, UINT32_MAX}, {start + 60 * 130, start + 60 * 150},
           {start + 60 * 2800, start + 60 * 2950},
           {start - 120, start - 60}, {start + 60 * 3001, UINT32_MAX}}) {
    returned = 0;
    uint32_t previous = 0;
    assert(history.forEach(tier, range.first, range.second,
        [&](const HistoryStore::Record &record) {
          assert(record.timestamp > previous);
          assert(record.timestamp >= range.first && record.timestamp <= range.second);
          previous = record.timestamp;
          ++returned;
          return true;
        }));
    unsigned expected = 0;
    for (uint32_t index = 120; index < 3000; ++index)
      if (start + index * 60 >= range.first && start + index * 60 <= range.second)
        ++expected;
    assert(returned == expected);
  }
}

#include "../../src/app/network/ServicedNetworkClient.h"
namespace networkServiceTest {
unsigned calls = 0;
void service() { ++calls; }
struct Client {
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int read(uint8_t *, size_t) { return 0; }
};
void run() {
  ServicedNetworkClient<Client, service> client;
  static_assert(sizeof(client) == sizeof(Client), "adapter adds no state");
  for (int i = 0; i < 1000; ++i) assert(client.available() == 0);
  assert(calls == 1000);
  assert(client.read() == -1);
  uint8_t byte;
  assert(client.read(&byte, 1) == 0);
  assert(calls == 1002);
}
}

void testHistoryPreservation() {
  LittleFS.files.clear();
  LittleFS.mountFails = true;
  LittleFS.formatAttempts = 0;
  testPartitionBytes[123] = 0;
  HistoryStore damagedFs;
  assert(!damagedFs.begin());
  assert(LittleFS.formatAttempts == 0);
  testPartitionBytes[123] = 0xff;
  testPartitionReadError = true;
  HistoryStore unreadableFs;
  assert(!unreadableFs.begin());
  assert(LittleFS.formatAttempts == 0);
  testPartitionReadError = false;
  testPartitionMissing = true;
  HistoryStore absentFs;
  assert(!absentFs.begin());
  assert(LittleFS.formatAttempts == 0);
  testPartitionMissing = false;
  HistoryStore blankFs;
  assert(blankFs.begin());
  assert(LittleFS.formatAttempts == 1);
  LittleFS.mountFails = false;

  const auto file = LittleFS.files.at("/minute.bin");
  for (size_t size : {size_t(0), size_t(12), size_t(48)}) {
    file->bytes.assign(size, 0x12);
    const auto original = file->bytes;
    HistoryStore damaged;
    assert(!damaged.begin());
    assert(file->bytes == original);
  }
  LittleFS.files.clear();
  HistoryStore history;
  assert(history.begin());
  constexpr auto tier = HistoryStore::Tier::Minute;
  constexpr uint32_t start = 1700000040;
  for (uint32_t delta : {0U, 60U, 120U, 600U})
    assert(history.importRecord(tier, {start + delta, 100, 100, 100, 10, 0}));
  assert(!history.importRecord(tier, {start + 60, 100, 100, 100, 10, 0}));
  // Simulate a legacy file with out-of-order records, without changing headers.
  auto &legacy = LittleFS.files.at("/minute.bin")->bytes;
  std::swap_ranges(legacy.begin() + 48 + 24, legacy.begin() + 48 + 48,
                   legacy.begin() + 48 + 72);
  for (int boot = 0; boot < 2; ++boot) {
    HistoryStore loaded;
    assert(loaded.begin());
    for (int query = 0; query < 2; ++query) {
      unsigned found = 0;
      assert(loaded.forEach(tier, start + 60, start + 120,
          [&](const HistoryStore::Record &) { ++found; return true; }));
      assert(found == 2);
    }
  }
  assert(history.clear(tier));
  history.update(start + 600, 100, 10, 0);
  history.update(start, 999, 10, 0); // Clock reversal must not contaminate the bucket.
  assert(history.flushPending(tier));
  assert(history.forEach(tier, 0, UINT32_MAX, [&](const HistoryStore::Record &r) {
    assert(r.timestamp == start + 600 && r.averageW == 100); return true;
  }));
  LittleFS.files.at("/minute.bin")->bytes.resize(48); // Valid header, truncated records.
  const auto truncated = LittleFS.files.at("/minute.bin")->bytes;
  HistoryStore shortened;
  assert(!shortened.begin());
  assert(LittleFS.files.at("/minute.bin")->bytes == truncated);
}

void testEventLogWriteFailure() {
  LittleFS.files.clear();
  EventLog log;
  assert(log.begin(true));
  failedWriteAfter = 1;
  assert(!log.add("INFO", "FAILED", "header write fails"));
  failedWriteAfter = -1;
  assert(log.add("INFO", "SAVED", "retry uses the uncommitted slot"));
  EventLog restored;
  assert(restored.begin(true));
  assert(restored.count() == 1);
  restored.forEach([](const EventLog::Record &record) {
    assert(strcmp(record.code, "SAVED") == 0);
    return true;
  });
}

int main() {
  testModbus();
  testSml();
  testMqttAndJson();
  testHistoryFailuresAndWrap();
  testHistoryPreservation();
  networkServiceTest::run();
  testEventLogWriteFailure();
  puts("PASS: Modbus fragmentation/coalescing/invalid frames/sentinels; SML equivalence/qualification/CRC/oversize");
}
