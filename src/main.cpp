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
#include <esp_flash.h>
#include <esp_image_format.h>
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
#include "app/network/ServicedNetworkClient.h"
#include "app/meter/MeterData.h"
#include "app/meter/D0Parser.h"
#include "app/meter/SmlParser.h"
#include "app/core/EventLog.h"
#include "app/core/DeviceIdentity.h"
#include "app/update/FirmwareSigningPublicKey.h"
#include "app/update/AssetRollback.h"
#if IR_TRACKER_ENABLE_GITHUB_UPDATE
#include "app/update/GithubRootCertificates.h"
#endif
#include <WebAssets.h>

#if IR_TRACKER_ENABLE_MDNS
#include <errno.h>
#include <fcntl.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <strings.h>
#endif

#define IR_TRACKER_AMALGAMATED_BUILD 1

// Keep protocol implementations in dedicated source files, but compile them
// into the main translation unit so the size optimizer can devirtualize the
// parser interface on the small ESP32-C3 OTA partition.
#include "app/meter/D0Parser.cpp"
#include "app/meter/SmlParser.cpp"

namespace {

constexpr char kFirmwareVersion[] = "2.1.1";
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
void serviceMeterInput();
void serviceOtaMeasurementTick();
ServicedNetworkClient<WiFiClient, serviceMeterInput> mqttNetwork;
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
  bool wifiScheduleOff = true;
  uint16_t wifiScheduleStartMinutes = 0;
  uint16_t wifiScheduleEndMinutes = 5 * 60;
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
bool mdnsAdvertisedModbus = false;
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
String updateCommitError;
// True only while a manual/GitHub combined IRUP transfer is in progress.
// Measurement + history stay active; nonessential interfaces pause.
bool otaMeasurementMode = false;
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
bool wifiScheduledOff = false;

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

struct CombinedUpdatePlan {
  bool firmwareStaged = false;
  bool assetsStaged = false;
  char version[32] = {};
  char firmwareSha256[65] = {};
  char assetsSha256[65] = {};
  uint32_t firmwareSize = 0;
  uint32_t assetsSize = 0;
} combinedUpdate;


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

struct LiveSample {
  // Only slots below liveCount are read; each is fully written before use.
  // Zero initialization keeps the unused ring in BSS, not in the BIN image.
  uint32_t timestamp;
  float powerW;
  float importKwh;
  float exportKwh;
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

#include "app/update/AssetRollbackEsp.cpp"

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
#include "app/core/ProductSafety.cpp"
#include "app/meter/MeterAutoCommissioning.cpp"

#include "app/diagnostics/FactoryNvsProbe.cpp"
#include "app/diagnostics/FactoryTest.cpp"

#include "app/diagnostics/GpioScanner.cpp"

uint32_t largestFreeHeapBlockBytes();
uint32_t loopStackHighWaterMarkBytes();

#include "app/diagnostics/DiagnosticsApi.cpp"
#include "app/diagnostics/ProductExperience.cpp"
#include "app/core/ProductRuntime.cpp"

#include "app/web/StatusApi.cpp"

#include "app/web/TelemetryApi.cpp"

#include "app/web/EcoTrackerEmulation.cpp"

#include "app/web/ShellyEmulation.cpp"

#include "app/web/DashboardHistory.cpp"

#include "app/web/SettingsApi.cpp"

#include "app/update/OtaManager.cpp"

#include "app/web/MaintenanceWeb.cpp"

#include "app/meter/IrControl.cpp"

#if IR_TRACKER_ENABLE_MDNS
#include "app/network/MinimalMdns.cpp"
#endif

#include "app/network/NetworkManager.cpp"

#include "app/network/ModbusMeterServer.cpp"

#include "app/network/MqttManager.cpp"

#include "app/web/WebApi.cpp"

#include "app/core/RuntimeHealth.cpp"

// Called both from loop() and from the synchronous HTTP/flash update callback.
// Never calls the web server itself: handleClient() is not re-entrant.
void serviceOtaMeasurementTick() {
  // History storage can call its meter service hook during flash I/O; prevent
  // recursive history updates if that hook changes in the future.
  static bool inside = false;
  if (inside) { serviceMeterInput(); return; }
  inside = true;
  esp_task_wdt_reset();
  serviceMeterInput();
  updateIrPulseJob();
  updateApatorUnlock();
  updateActiveD0();
  updateMeterRecovery();
  serviceMeterInput();
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
  inside = false;
}

}  // DE: Namensraum | EN: namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  createCsrfToken();
  bootResetReason = resetReasonText(esp_reset_reason());
  beginProductRuntimeEarly(esp_reset_reason());
  const esp_err_t watchdogInit = esp_task_wdt_init(15, true);
  if (watchdogInit == ESP_OK || watchdogInit == ESP_ERR_INVALID_STATE)
    esp_task_wdt_add(nullptr);
  const esp_partition_t *running = esp_ota_get_running_partition();
  const bool assetRecoveryReady = recoverAssetTransaction();
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
  const bool debugStorageReady = assetRecoveryReady && debugStorage.begin(kFirmwareVersion);
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
  assetRollbackIo.uartReady = true;
  history.setServiceHook([]() {
    esp_task_wdt_reset();
    serviceMeterInput();
  });
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
  // Larger telemetry is streamed by publishMqttValues; discovery and CONNECT
  // fit this buffer. Keep PubSubClient's original buffer if allocation fails.
  mqtt.setBufferSize(1024);
  mqtt.setSocketTimeout(1);
  setupRoutes();
  finishProductRuntimeSetup();
  if (assetRecoveryReady && debugStorageReady) {
    // Confirm the pair only after local initialization, never on MQTT/Internet availability.
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK &&
        (strcmp(assetRollbackStatus, "new_ready") != 0 ||
         assetRollback.confirm(assetRollbackIo.running()))) {
      assetRollbackStatus = "ready";
    } else {
      assetRollbackStatus = "confirmation_failed";
      assetRollbackBlocked = true;
    }
  }
#if IR_TRACKER_ENABLE_DEVELOPER_IO
  setupWebSockets();
#endif
  // A rollback must never boot a firmware unable to read the new history.
  // First install the bridge into one slot; a second identical IRUP puts the
  // same verified reader/writer into the other slot before conversion starts.
  if (productRuntimeAllowsHistoryMigration() &&
      history.ready() && !history.compactActive() && !assetRollbackBlocked &&
      assetRecoveryReady && debugStorageReady &&
      assetRollback.sameVerifiedAppPair(assetRollbackIo.running()))
    history.migrateCompact();
  Serial.printf("Offline firmware %s, partition=%s, RX=GPIO%u @ %lu baud\n",
                kFirmwareVersion, running ? running->label : "?", config.rxPin, config.baud);
  Serial.printf("Open http://%s/\n", primaryNetworkIp().c_str());
}

void loop() {
  esp_task_wdt_reset();
  serviceMeterInput();
  ethernet.loop();
  if (!otaMeasurementMode) {
    manageWifi();
    manageModbusMeterServer();
    manageAdaptiveWifiPower();
    manageMqtt();
    serviceMeterInput();
    manageGithubFirmwareUpdate();
  }
  if (accessPointMode) dns.processNextRequest();
  server.handleClient();

  // The same meter/IR/history code runs during the upload callback itself;
  // do not leave a gap while WebServer::handleClient() blocks the loop.
  serviceOtaMeasurementTick();
  if (otaMeasurementMode) {
    // Eco mode must not down-clock the CPU during long updates.
    manageCpuPowerMode();
    delay(1);
    return;
  }

#if IR_TRACKER_ENABLE_DEVELOPER_IO
  if (config.snifferEnabled) snifferSocket.loop();
  if (config.bridgeEnabled) bridgeSocket.loop();
#endif
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
  updateLed();
  monitorHeap();
  manageCpuPowerMode();
  delay(2);
}
