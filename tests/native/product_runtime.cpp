#include <cassert>
#include <type_traits>
#include "../../src/app/meter/MeterData.h"

class ProductString : public std::string {
 public:
  using std::string::string;
  ProductString(const std::string &s) : std::string(s) {}
  template<class T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
  ProductString(T value) : std::string(std::to_string(value)) {}
};
#define String ProductString
uint32_t nowMs = 0;
uint32_t testMillis() { return nowMs; }
#define millis testMillis
#define RTC_DATA_ATTR
#define IR_TRACKER_AMALGAMATED_BUILD 1
#define IR_TRACKER_ENABLE_FACTORY_TEST 1
#define IR_TRACKER_ENABLE_DEVELOPER_IO 1
enum esp_reset_reason_t { ESP_RST_UNKNOWN, ESP_RST_POWERON, ESP_RST_SW,
  ESP_RST_PANIC, ESP_RST_INT_WDT, ESP_RST_TASK_WDT, ESP_RST_WDT, ESP_RST_BROWNOUT };
constexpr uint32_t SERIAL_8N1 = 1, SERIAL_7E1 = 2, kReadingStaleMs = 15000,
                   kMeterRecoveryMs = 45000;
struct { void add(const char *, const char *, const String &) {} } eventLog;
String jsonEscape(const String &s) { return s; }
struct { MeterProtocol meterProtocol = MeterProtocol::Auto; uint32_t baud = 9600;
         int rxPin = 3, txPin = 6; bool bridgeEnabled = false; } config;
MeterData meter;
struct { bool active = false; } gpioScan, irPulse, apatorUnlock;
struct { bool running = false; } factoryTest;
struct { bool active = false, acknowledgementSent = false;
         uint32_t startedMs = 0, lastAttemptMs = 0; } activeD0;
struct {
  unsigned changes = 0;
  uint32_t baud = 0, mode = 0;
  void end() {}
  void begin(uint32_t b, uint32_t m, int, int) { ++changes; baud = b; mode = m; }
  void write(const uint8_t *, size_t) {}
  void flush() {}
} meterSerial;
unsigned saved = 0, restored = 0, resets = 0;
uint32_t lastMeterRecoveryMs = 0, meterReinitializations = 0;
void resetMeterParsers() { ++resets; }
void restoreConfiguredMeterSerial() { ++restored; resetMeterParsers(); }
void requestCpuBoost(const char *) {}
void saveConfig() { ++saved; }
#include "../../src/app/core/ProductSafety.cpp"
#include "../../src/app/meter/MeterAutoCommissioning.cpp"
unsigned routeSetups = 0;
void setupProductExperienceRoutes() { ++routeSetups; }
#include "../../src/app/core/ProductRuntime.cpp"
bool shouldFeedD0ParserFor(MeterProtocol, MeterProtocol, bool) { return false; }
bool shouldFeedSmlParserFor(MeterProtocol, MeterProtocol, bool) { return false; }
bool autoProtocolFresh(MeterProtocol) { return true; } // Previous candidate's soft lock.
#include "product_uart.inc"

bool nvsOpen = true, nvsWrite = true, nvsRead = true, nvsRemove = true;
unsigned nvsCloses = 0;
struct Preferences {
  uint32_t value = 0;
  bool begin(const char *name, bool readOnly) {
    assert(std::string(name) == "ir-fct-probe" && !readOnly); return nvsOpen;
  }
  size_t putUInt(const char *key, uint32_t v) {
    assert(std::string(key) == "token"); value = v; return nvsWrite ? 4 : 0;
  }
  uint32_t getUInt(const char *, uint32_t fallback) { return nvsRead ? value : fallback; }
  bool remove(const char *key) { assert(std::string(key) == "token"); return nvsRemove; }
  void end() { ++nvsCloses; }
};
uint32_t esp_random() { return 1234; }
#include "../../src/app/diagnostics/FactoryNvsProbe.cpp"

void resetCommissioning() {
  beginProductSafety(ESP_RST_SW);
  meterCommissioning = {};
  meter = {};
  config.meterProtocol = MeterProtocol::Auto;
  gpioScan.active = irPulse.active = apatorUnlock.active = activeD0.active = false;
  factoryTest.running = config.bridgeEnabled = false;
  saved = restored = 0;
  nowMs = 18000;
}
int main() {
  for (auto reason : {ESP_RST_PANIC, ESP_RST_INT_WDT, ESP_RST_TASK_WDT,
                     ESP_RST_WDT, ESP_RST_BROWNOUT}) {
    beginProductRuntimeEarly(ESP_RST_POWERON);
    for (unsigned i = 1; i <= 3; ++i) {
      beginProductRuntimeEarly(reason);
      assert(productSafety.earlyCrashCount == i);
      assert(productRuntimeAllowsHistoryMigration() == (i < 3));
      assert(productRuntimeAllowsAutomaticUpdate() == (i < 3));
    }
    nowMs = 59999; manageProductSafety(); assert(!productSafety.healthyMarked);
    nowMs = 60000; manageProductSafety(); assert(productSafety.healthyMarked);
    assert(productSafeRecoveryActive()); // Healthy recovery does not unlock this boot.
    beginProductRuntimeEarly(ESP_RST_SW);
    assert(productRuntimeAllowsAutomaticUpdate() && productRuntimeAllowsHistoryMigration());
  }
  irEarlyCrashCount = 255; irPreviousBootHealthy = 0;
  beginProductSafety(ESP_RST_WDT); assert(productSafety.earlyCrashCount == 255);
  resetCommissioning();
  nowMs = 17000; manageMeterCommissioning(); assert(!meterCommissioning.started);
  nowMs = 18000; meter.lastTelegramMs = nowMs; manageMeterCommissioning();
  assert(!meterCommissioning.started);
  meter.lastTelegramMs = 0; manageMeterCommissioning();
  assert(meterCommissioningOwnsSerial() && meterSerial.mode == SERIAL_8N1);
  assert(shouldFeedSmlParser() && !shouldFeedD0Parser());
  const auto changes = meterSerial.changes;
  nowMs = 50000; beginActiveD0Attempt(); updateMeterRecovery();
  assert(meterSerial.changes == changes && !activeD0.active && restored == 0);
  meter.telegrams = 1; meter.lastTelegramMs = nowMs; meter.lastCrcValid = true;
  meter.detectedProtocol = MeterProtocol::Sml;
  nowMs = 19000; manageMeterCommissioning(); assert(saved == 0);
  meter.telegrams = 2; meter.lastTelegramMs = nowMs; manageMeterCommissioning();
  assert(saved == 1 && config.meterProtocol == MeterProtocol::Sml && restored == 1);
  manageMeterCommissioning(); assert(saved == 1);

  resetCommissioning(); startMeterCommissioning();
  nowMs += 4800; manageMeterCommissioning();
  assert(meterSerial.mode == SERIAL_7E1 && shouldFeedD0Parser() && !shouldFeedSmlParser());
  meter.telegrams = 2; meter.lastTelegramMs = nowMs; meter.lastCrcValid = true;
  meter.detectedProtocol = MeterProtocol::Iec62056;
  manageMeterCommissioning();
  assert(saved == 1 && config.meterProtocol == MeterProtocol::Iec62056 && config.rxPin == 3);

  resetCommissioning(); startMeterCommissioning();
  meter.telegrams = 1; meter.lastTelegramMs = nowMs; meter.lastCrcValid = true;
  meter.detectedProtocol = MeterProtocol::Sml;
  nowMs += 4800; manageMeterCommissioning(); // One SML frame must not count for D0.
  meter.telegrams = 2; meter.lastTelegramMs = nowMs; meter.detectedProtocol = MeterProtocol::Iec62056;
  manageMeterCommissioning(); assert(saved == 0);
  meter.telegrams = 3; meter.lastCrcValid = false;
  manageMeterCommissioning(); assert(saved == 0);
  meter.lastCrcValid = true; manageMeterCommissioning(); assert(saved == 1);

  resetCommissioning(); startMeterCommissioning();
  for (unsigned i = 0; i < 5; ++i) { nowMs += 4800; manageMeterCommissioning(); }
  assert(saved == 0 && restored == 1 && meterCommissioning.suggestGpioScan);
  for (unsigned owner = 0; owner < 7; ++owner) {
    resetCommissioning();
    bool *blocked[] = {&gpioScan.active, &irPulse.active, &apatorUnlock.active,
                      &activeD0.active, &factoryTest.running, &config.bridgeEnabled,
                      &productSafety.safeRecovery};
    *blocked[owner] = true; startMeterCommissioning(); assert(!meterCommissioning.active);
  }
  assert(runFactoryNvsProbe() && factoryNvsProbePassed());
  for (bool *failure : {&nvsOpen, &nvsWrite, &nvsRead, &nvsRemove}) {
    *failure = false;
    assert(!runFactoryNvsProbe() && !factoryNvsProbePassed());
    *failure = true;
  }
  assert(nvsCloses == 4);
  puts("PASS: production safety policy, UART ownership, two-frame SML/D0 commissioning, isolated NVS probe");
  return 0;
}
