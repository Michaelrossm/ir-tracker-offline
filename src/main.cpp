#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WebServer.h>
#if IR_TRACKER_ENABLE_DEVELOPER_IO
#include <WebSocketsServer.h>
#endif
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <esp32-hal-cpu.h>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <sys/time.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "app/storage/HistoryStore.h"
#include "app/storage/DebugStorage.h"
#include "app/hardware/HardwareProfile.h"
#include "app/network/EthernetManager.h"
#include "app/meter/MeterData.h"
#include "app/meter/D0Parser.h"
#include "app/meter/SmlParser.h"
#include "app/core/EventLog.h"
#include "app/core/DeviceIdentity.h"
#include "app/update/FirmwareSigningPublicKey.h"
#if IR_TRACKER_ENABLE_GITHUB_UPDATE
#include "app/update/GithubRootCertificates.h"
#endif
#include <WebAssets.h>

#if IR_TRACKER_ENABLE_MDNS
#include <ESPmDNS.h>
#endif

#define IR_TRACKER_AMALGAMATED_BUILD 1

// Keep protocol implementations in dedicated source files, but compile them
// into the main translation unit so the size optimizer can devirtualize the
// parser interface on the small ESP32-C3 OTA partition.
#include "app/meter/D0Parser.cpp"
#include "app/meter/SmlParser.cpp"

namespace {

constexpr char kFirmwareVersion[] = "1.3.5-beta.2";
constexpr char kGithubReleasesApi[] =
    "https://api.github.com/repos/Michaelrossm/ir-tracker-offline/releases?per_page=5";
constexpr char kGithubAssetPrefix[] =
    "https://github.com/Michaelrossm/ir-tracker-offline/releases/download/";
constexpr char kFirmwareAuthor[] = "Michael Roßmann";
constexpr char kFirmwareLicense[] = "PolyForm Noncommercial 1.0.0";
constexpr uint8_t kWifiSlots = 3;
constexpr uint32_t kWifiPerNetworkMs = 6000;
constexpr uint32_t kWifiRetryMs = 30000;
constexpr uint32_t kMqttPublishMs = 5000;
constexpr uint32_t kReadingStaleMs = 15000;
constexpr uint32_t kHeapWarningBytes = 36000;
constexpr uint32_t kLoginWindowMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kLoginMaxLockMs = 60UL * 60UL * 1000UL;
constexpr uint32_t kBrowserSessionSeconds = 60UL * 24UL * 60UL * 60UL;
constexpr uint32_t kCpuBoostHoldMs = 2UL * 60UL * 1000UL;
constexpr uint32_t kCpuNormalHoldMs = 60UL * 1000UL;
constexpr uint32_t kEcoCpuMhz = 80;
constexpr uint32_t kPerformanceCpuMhz = 160;
constexpr uint32_t kWifiPowerStableMs = 3UL * 60UL * 1000UL;
constexpr uint32_t kWifiPowerEvaluateMs = 60UL * 1000UL;
constexpr uint32_t kGithubInitialCheckMs = 5UL * 60UL * 1000UL;
constexpr uint32_t kGithubCheckIntervalMs = 24UL * 60UL * 60UL * 1000UL;
constexpr size_t kGithubMaximumPackageBytes = 4UL * 1024UL * 1024UL;
constexpr uint8_t kDefaultRxPin = 3;
constexpr int8_t kDefaultTxPin = 6;
constexpr uint32_t kDefaultBaud = 9600;

WebServer server(80);
#if IR_TRACKER_ENABLE_DEVELOPER_IO
WebSocketsServer snifferSocket(81);
WebSocketsServer bridgeSocket(82);
#endif
DNSServer dns;
Preferences prefs;
HardwareSerial meterSerial(1);
WiFiClient mqttNetwork;
PubSubClient mqtt(mqttNetwork);
HistoryStore history;
DebugStorage debugStorage;
EventLog eventLog;
EthernetManager ethernet;
D0Parser d0Parser;
SmlParser smlParser;
uint32_t meterReinitializations = 0;
uint32_t lastMeterRecoveryMs = 0;

struct Config {
  String ssid[kWifiSlots];
  String password[kWifiSlots];
  String hostname = "ir-tracker";
  uint8_t rxPin = kDefaultRxPin;
  int8_t txPin = kDefaultTxPin;
  int8_t ledPin = 5;
  bool ledInverted = true;
  uint32_t baud = kDefaultBaud;
  MeterProtocol meterProtocol = MeterProtocol::Auto;
  String mqttHost;
  uint16_t mqttPort = 1883;
  String mqttUser;
  String mqttPassword;
  bool homeAssistantDiscovery = true;
  uint8_t apiAccess = 0;  // DE: 0=lokal offen, 1=Admin, 2=aus | EN: 0=local public, 1=admin, 2=off
  bool storageCompatibilityMode = false;
  bool modbusTcp = false;
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  bool snifferEnabled = false;
  bool bridgeEnabled = false;
#endif
  String meterPin;
  bool autoPin = false;
  bool pinInverted = false;
  uint16_t pinPulseMs = 300;
  uint16_t pinDigitGapMs = 3000;
  String adminPassword;
  String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
  uint16_t setupApMinutes = 15;
  bool persistEventLog = false; // DEFAULT: keep EventLog in RAM only (do not persist to flash)
  bool ecoMode = true;
  bool ecoLedOff = true;
  bool adaptiveWifiPower = true;
  bool wifiPowerSave = false;
  bool githubUpdateCheck = true;
  bool githubAutoInstall = false;
} config;

MeterData meter;
std::vector<uint8_t> lastTelegram;
bool accessPointMode = false;
uint32_t accessPointStartedMs = 0;
bool accessPointAllowed = true;
#if IR_TRACKER_ENABLE_MDNS
bool mdnsRunning = false;
String mdnsAdvertisedIp;
String mdnsAdvertisedTransport;
#endif
uint32_t lastWifiAttemptMs = 0;
uint32_t lastMqttAttemptMs = 0;
uint32_t mqttRetryMs = 10000;
uint32_t lastMqttPublishMs = 0;
String deviceId;
DeviceIdentity deviceIdentity;
uint8_t wifiCandidate = 0;
uint8_t wifiTried = 0;
uint32_t wifiCandidateStartedMs = 0;
bool ntpConfigured = false;
bool otaUploadAuthorized = false;
bool otaUploadOk = false;
String otaUploadError;
bool autoPinAttempted = false;
uint32_t lastHistorySampleMs = 0;
uint32_t lastLiveSampleMs = 0;
String csrfToken;
const char *browserSessionState = "not_checked";
String bootResetReason;
bool heapWarningActive = false;
uint32_t cpuBoostUntilMs = 0;
uint32_t cpuNormalUntilMs = 0;
uint32_t cpuFrequencySwitches = 0;
uint32_t cpuFrequencyErrors = 0;
bool cpuEcoRuntimeFault = false;
char cpuBoostReason[32] = "startup";

enum class WifiTxProfile : uint8_t { Full, Medium, Reduced };
WifiTxProfile wifiTxProfile = WifiTxProfile::Full;
uint32_t wifiConnectedSinceMs = 0;
uint32_t lastWifiPowerEvaluateMs = 0;
uint32_t wifiTxPowerChanges = 0;
uint32_t wifiTxPowerErrors = 0;
uint32_t wifiModeErrors = 0;
bool wifiTxPowerRuntimeFault = false;
bool wifiMinModemSleepActive = false;

#include "app/network/NetworkStatus.cpp"

struct GithubUpdateState {
  bool checking = false;
  bool installing = false;
  bool available = false;
  bool checked = false;
  String version;
  String assetName;
  String assetUrl;
  String error;
  size_t assetSize = 0;
  uint32_t lastAttemptMs = 0;
  time_t lastSuccess = 0;
} githubUpdate;

// DE: Die gefuehrte GPIO-Suche veraendert nur die laufende UART-Konfiguration.
// Sie speichert nichts und stellt die normale Konfiguration nach Erfolg, Fehler
// oder Abbruch wieder her. | EN: The guided GPIO scan only changes the running
// UART configuration. It stores nothing and always restores normal operation.
struct GpioScanState {
  bool active = false;
  bool complete = false;
  bool found = false;
  uint8_t pins[11] = {};
  uint32_t bauds[10] = {};
  uint8_t pinCount = 0;
  uint8_t baudCount = 0;
  uint8_t pinIndex = 0;
  uint8_t baudIndex = 0;
  int8_t currentPin = -1;
  uint32_t currentBaud = 0;
  int8_t foundPin = -1;
  uint32_t foundBaud = 0;
  uint16_t tested = 0;
  uint16_t total = 0;
  uint32_t candidateStartedMs = 0;
  uint32_t baselineTelegrams = 0;
  String error;
} gpioScan;

struct ActiveD0State {
  bool active = false;
  bool acknowledgementSent = false;
  uint32_t startedMs = 0;
  uint32_t lastAttemptMs = 0;
} activeD0;

#if IR_TRACKER_ENABLE_FACTORY_TEST
struct FactoryTestState {
  bool running = false;
  bool finished = false;
  bool loopbackPassed = false;
  bool ledConfirmed = false;
  bool poeConfirmed = false;
  uint8_t matched = 0;
  uint32_t startedMs = 0;
} factoryTest;

constexpr uint8_t kFactoryLoopbackPattern[] = {
    'I', 'R', 'F', 'C', 'T', '-', '1', 0x55, 0x2a};
constexpr uint32_t kFactoryLoopbackTimeoutMs = 5000;
#endif

constexpr uint32_t kGpioScanWindowMs = 2200;
constexpr uint32_t kActiveD0InitialDelayMs = 45UL * 1000UL;
constexpr uint32_t kActiveD0RetryMs = 30UL * 1000UL;
constexpr uint32_t kActiveD0TimeoutMs = 6000;
constexpr uint32_t kMeterRecoveryMs = 45UL * 1000UL;

#include "app/core/EcoManager.cpp"

struct LoginGuard {
  IPAddress ip;
  uint8_t failures = 0;
  uint8_t lockLevel = 0;
  uint32_t firstFailureMs = 0;
  uint32_t lockUntilMs = 0;
  uint32_t lastSeenMs = 0;
};
constexpr size_t kLoginGuardSlots = 8;
LoginGuard loginGuards[kLoginGuardSlots];

struct SignedOtaState {
  uint8_t header[16] = {};
  size_t headerRead = 0;
  uint32_t firmwareSize = 0;
  uint16_t signatureSize = 0;
  uint8_t signature[80] = {};
  size_t signatureRead = 0;
  size_t firmwareWritten = 0;
  bool updateStarted = false;
  bool firstFirmwareByteChecked = false;
  mbedtls_sha256_context sha;
} signedOta;

struct LiveSample {
  uint32_t timestamp = 0;
  float powerW = NAN;
  float importKwh = NAN;
  float exportKwh = NAN;
};
constexpr size_t kLiveSamples = 840;  // DE: 70 Minuten bei 5 s | EN: 70 minutes at 5 s
LiveSample liveSamples[kLiveSamples];
size_t liveWriteIndex = 0;
size_t liveCount = 0;

struct IrPulseJob {
  bool active = false;
  bool outputActive = false;
  bool inverted = false;
  int8_t pin = -1;
  uint8_t digits[4] = {};
  uint8_t digitIndex = 0;
  uint8_t pulsesRemaining = 0;
  uint16_t pulseMs = 300;
  uint16_t pulseGapMs = 300;
  uint16_t digitGapMs = 3000;
  uint32_t nextChangeMs = 0;
} irPulse;

struct ApatorUnlockJob {
  bool active = false;
  uint8_t phase = 0;
  uint32_t nextMs = 0;
  uint32_t verifyUntilMs = 0;
} apatorUnlock;

#include "app/core/SecurityManager.cpp"

#include "app/core/CoreHelpers.cpp"

#include "app/web/IntegrationApi.cpp"

#include "app/core/ConfigManager.cpp"

#include "app/web/WebUi.cpp"

bool modbusMeterRunning();
uint32_t modbusMeterConnections();
uint32_t modbusMeterValidRequests();
uint32_t modbusMeterInvalidRequests();
String modbusMeterLastClient();
void manageModbusMeterServer();

#include "app/meter/MeterManager.cpp"

#include "app/diagnostics/FactoryTest.cpp"

#include "app/diagnostics/GpioScanner.cpp"

#include "app/web/StatusApi.cpp"

#include "app/web/TelemetryApi.cpp"

#include "app/web/EcoTrackerEmulation.cpp"

#include "app/web/ShellyEmulation.cpp"

#include "app/diagnostics/DiagnosticsApi.cpp"

#include "app/web/DashboardHistory.cpp"

#include "app/web/SettingsApi.cpp"

#include "app/update/OtaManager.cpp"

#include "app/web/MaintenanceWeb.cpp"

#include "app/meter/IrControl.cpp"

#include "app/network/NetworkManager.cpp"

#include "app/network/ModbusMeterServer.cpp"

#include "app/network/MqttManager.cpp"

#include "app/web/WebApi.cpp"

void updateLed() {
  static uint32_t lastToggle = 0;
  static bool state = false;
  if (gpioScan.active) return;
#if IR_TRACKER_ENABLE_FACTORY_TEST
  // Keep the LED steadily lit until the operator has completed the visual FCT.
  if ((factoryTest.running || factoryTest.finished) &&
      !factoryTest.ledConfirmed)
    return;
#endif
  if (config.ledPin < 0) return;
  if (ecoLedSuppressed()) {
    if (state) {
      state = false;
      digitalWrite(config.ledPin, config.ledInverted);
    }
    return;
  }
  const uint32_t intervalMs = trackerFaultActive() ? 150U : 1000U;
  if (millis() - lastToggle < intervalMs) return;
  lastToggle = millis();
  state = !state;
  digitalWrite(config.ledPin, state ^ config.ledInverted);
}

void monitorHeap() {
  static uint32_t lastCheckMs = 0;
  if (millis() - lastCheckMs < 30000) return;
  lastCheckMs = millis();
  const uint32_t freeHeap = ESP.getFreeHeap();
  const bool low = freeHeap < kHeapWarningBytes;
  if (low && !heapWarningActive) {
    eventLog.add("WARN", "HEAP_LOW",
                 "Freier RAM unter Sicherheitsgrenze: " +
                     String(freeHeap) + " Bytes");
  } else if (!low && heapWarningActive) {
    eventLog.add("INFO", "HEAP_RECOVERED",
                 "Freier RAM wieder im sicheren Bereich");
  }
  heapWarningActive = low;
}

}  // DE: Namensraum | EN: namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  createCsrfToken();
  bootResetReason = resetReasonText(esp_reset_reason());
  const esp_err_t watchdogInit = esp_task_wdt_init(15, true);
  if (watchdogInit == ESP_OK || watchdogInit == ESP_ERR_INVALID_STATE)
    esp_task_wdt_add(nullptr);
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_mark_app_valid_cancel_rollback();
  deviceIdentity.begin();
  deviceId = deviceIdentity.mqttId;
  loadConfig();
  // Probe Ethernet before UART/LED GPIOs are configured. If no W5500 answers
  // VERSIONR, the SPI bus is released and all GPIOs remain available to the
  // existing Wi-Fi-only hardware configuration.
  if (ethernet.begin(config.hostname.c_str())) {
    Serial.println("Universal network: W5500 driver started");
  } else {
    Serial.printf("Universal network: Wi-Fi fallback (%s)\n",
                  ethernet.lastError().c_str());
  }
  normalizeHardwarePins();
  const bool debugStorageReady = debugStorage.begin(kFirmwareVersion);
  if (!debugStorageReady) {
    Serial.printf("Debug storage disabled (%s): using embedded web assets\n",
                  debugStorage.lastError());
  }
  const bool historyReady = history.begin();
  eventLog.begin(config.persistEventLog);
  if (!debugStorageReady) {
    eventLog.add("WARN", "DEBUG_STORAGE_UNAVAILABLE",
                 "Optionale Debug-Partition nicht verfuegbar: " +
                     String(debugStorage.lastError()));
  } else if (debugStorage.usingLegacyLabel()) {
    eventLog.add("INFO", "DEBUG_STORAGE_LEGACY",
                 "Legacy-Partitionslabel coredump wird kompatibel verwendet");
  }
  eventLog.add(historyReady ? "INFO" : "ERROR", "BOOT",
               "Firmware " + String(kFirmwareVersion) +
                   (historyReady ? " gestartet" : " ohne Historie gestartet") +
                   ", Ursache: " + bootResetReason);
  meterSerial.setRxBufferSize(2048);
  restoreConfiguredMeterSerial();
  if (config.ledPin >= 0) {
    pinMode(config.ledPin, OUTPUT);
    digitalWrite(config.ledPin, config.ledInverted);
  }
  // DE: ESP-IDFs dauerhaften WLAN-Namensraum nicht nutzen; er gehört Solakon. | EN: Do not use ESP-IDF's persistent Wi-Fi namespace; it belongs to Solakon.
  WiFi.persistent(false);
  WiFi.setHostname(config.hostname.c_str());
  // Start Eco control before networking so connection attempts can request a
  // deterministic temporary boost instead of being down-clocked mid-DHCP.
  startCpuPowerMode();
  startAccessPoint();
  wifiTried = 0;
  beginNextKnownWifi();
  mqtt.setServer(config.mqttHost.c_str(), config.mqttPort);
  // The retained state contains all values plus per-value age information.
  // Keep enough packet space so MQTT never silently drops the complete JSON.
  mqtt.setBufferSize(4096);
  mqtt.setSocketTimeout(1);
  setupRoutes();
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  setupWebSockets();
#endif
  Serial.printf("Offline firmware %s, partition=%s, RX=GPIO%u @ %lu baud\n",
                kFirmwareVersion, running ? running->label : "?", config.rxPin, config.baud);
  Serial.printf("Open http://%s/\n", primaryNetworkIp().c_str());
}

void loop() {
  esp_task_wdt_reset();
  // Meter input has priority over potentially blocking MQTT/Web/network work.
  serviceMeterInput();
  ethernet.loop();
  manageWifi();
  manageModbusMeterServer();
  manageAdaptiveWifiPower();
  manageMqtt();
  manageGithubFirmwareUpdate();
  if (accessPointMode) dns.processNextRequest();
  server.handleClient();
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  if (config.snifferEnabled) snifferSocket.loop();
  if (config.bridgeEnabled) bridgeSocket.loop();
#endif
  updateIrPulseJob();
  updateApatorUnlock();
  manageAutoPin();
#if IR_TRACKER_ENABLE_FACTORY_TEST
  updateFactoryTest();
  if (factoryTest.running) {
    monitorHeap();
    delay(1);
    return;
  }
#endif
  updateGpioScan();
  updateMeterRecovery();
  updateActiveD0();
  serviceMeterInput();
  updateActiveD0();
  updateGpioScan();
  const bool meterFresh = valueFresh(meter.powerUpdatedMs);
  if (millis() - lastHistorySampleMs >= 1000) {
    lastHistorySampleMs = millis();
    if (meterFresh && !gpioScan.active)
      history.update(time(nullptr), meter.powerW, meter.importKwh,
                     meter.exportKwh);
  }
  if (millis() - lastLiveSampleMs >= 5000 && time(nullptr) >= 1700000000 &&
      meterFresh && !gpioScan.active && std::isfinite(meter.powerW)) {
    lastLiveSampleMs = millis();
    liveSamples[liveWriteIndex].timestamp =
        static_cast<uint32_t>(time(nullptr));
    liveSamples[liveWriteIndex].powerW = static_cast<float>(meter.powerW);
    liveSamples[liveWriteIndex].importKwh =
        static_cast<float>(meter.importKwh);
    liveSamples[liveWriteIndex].exportKwh =
        static_cast<float>(meter.exportKwh);
    liveWriteIndex = (liveWriteIndex + 1) % kLiveSamples;
    liveCount = std::min(liveCount + 1, kLiveSamples);
  }
  updateLed();
  monitorHeap();
  manageCpuPowerMode();
  delay(2);
}
