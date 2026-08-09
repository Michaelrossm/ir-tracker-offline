#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
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

#include "HistoryStore.h"
#include "EnergyManager.h"
#include "EventLog.h"
#include "FirmwareSigningPublicKey.h"
#include "GithubRootCertificates.h"
#include "WebAssets.h"

namespace {

constexpr char kFirmwareVersion[] = "1.0.2-beta.1";
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
constexpr uint32_t kEcoCpuMhz = 80;
constexpr uint32_t kPerformanceCpuMhz = 160;
constexpr uint32_t kWifiPowerStableMs = 3UL * 60UL * 1000UL;
constexpr uint32_t kWifiPowerEvaluateMs = 60UL * 1000UL;
constexpr uint32_t kGithubInitialCheckMs = 5UL * 60UL * 1000UL;
constexpr uint32_t kGithubCheckIntervalMs = 24UL * 60UL * 60UL * 1000UL;
constexpr size_t kGithubMaximumPackageBytes = 4UL * 1024UL * 1024UL;
constexpr size_t kTelegramMax = 2048;
constexpr uint8_t kDefaultRxPin = 3;
constexpr int8_t kDefaultTxPin = 6;
constexpr uint32_t kDefaultBaud = 9600;
constexpr uint8_t kSmlStart[] = {0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01};
constexpr uint8_t kSmlEnd[] = {0x1b, 0x1b, 0x1b, 0x1b, 0x1a};

WebServer server(80);
WebSocketsServer snifferSocket(81);
WebSocketsServer bridgeSocket(82);
DNSServer dns;
Preferences prefs;
HardwareSerial meterSerial(1);
WiFiClient mqttNetwork;
PubSubClient mqtt(mqttNetwork);
HistoryStore history;
EnergyManager energyManager;
EnergyManager::Config energyConfig;
EventLog eventLog;

struct Config {
  String ssid[kWifiSlots];
  String password[kWifiSlots];
  String hostname = "ir-tracker";
  uint8_t rxPin = kDefaultRxPin;
  int8_t txPin = kDefaultTxPin;
  int8_t ledPin = 5;
  bool ledInverted = true;
  uint32_t baud = kDefaultBaud;
  String mqttHost;
  uint16_t mqttPort = 1883;
  String mqttUser;
  String mqttPassword;
  bool homeAssistantDiscovery = true;
  uint8_t apiAccess = 0;  // DE: 0=lokal offen, 1=Admin, 2=aus | EN: 0=local public, 1=admin, 2=off
  bool snifferEnabled = false;
  bool bridgeEnabled = false;
  String meterPin;
  bool autoPin = false;
  bool pinInverted = false;
  uint16_t pinPulseMs = 300;
  uint16_t pinDigitGapMs = 3000;
  String adminPassword;
  String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
  uint16_t setupApMinutes = 15;
  bool persistEventLog = false;
  bool ecoMode = true;
  bool ecoLedOff = true;
  bool adaptiveWifiPower = true;
  bool githubUpdateCheck = true;
  bool githubAutoInstall = false;
} config;

struct MeterValues {
  double powerW = NAN;
  double importKwh = NAN;
  double exportKwh = NAN;
  double phasePowerW[3] = {NAN, NAN, NAN};
  double phaseVoltageV[3] = {NAN, NAN, NAN};
  double phaseCurrentA[3] = {NAN, NAN, NAN};
  uint32_t telegrams = 0;
  uint32_t bytes = 0;
  uint32_t parseErrors = 0;
  uint32_t crcErrors = 0;
  uint32_t lastTelegramMs = 0;
  bool lastCrcValid = false;
} meter;

std::vector<uint8_t> telegram;
std::vector<uint8_t> lastTelegram;
size_t startMatched = 0;
bool capturing = false;
uint8_t smlTrailerRemaining = 0;
bool accessPointMode = false;
uint32_t accessPointStartedMs = 0;
bool accessPointAllowed = true;
bool mdnsRunning = false;
uint32_t lastWifiAttemptMs = 0;
uint32_t lastMqttAttemptMs = 0;
uint32_t mqttRetryMs = 10000;
uint32_t lastMqttPublishMs = 0;
String deviceId;
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
  uint32_t bauds[5] = {};
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

constexpr uint32_t kGpioScanWindowMs = 2200;

const char *wifiTxProfileName() {
  switch (wifiTxProfile) {
    case WifiTxProfile::Medium: return "medium";
    case WifiTxProfile::Reduced: return "reduced";
    default: return "full";
  }
}

wifi_power_t wifiTxProfilePower(WifiTxProfile profile) {
  switch (profile) {
    case WifiTxProfile::Medium: return WIFI_POWER_15dBm;
    case WifiTxProfile::Reduced: return WIFI_POWER_11dBm;
    default: return WIFI_POWER_19_5dBm;
  }
}

float wifiTxPowerDbm() {
  return static_cast<int>(wifiTxProfilePower(wifiTxProfile)) / 4.0f;
}

bool setWifiTxProfile(WifiTxProfile profile) {
  if (wifiTxProfile == profile) return true;
  if (!WiFi.setTxPower(wifiTxProfilePower(profile))) {
    ++wifiTxPowerErrors;
    return false;
  }
  wifiTxProfile = profile;
  ++wifiTxPowerChanges;
  return true;
}

void forceFullWifiPower() {
  if (!setWifiTxProfile(WifiTxProfile::Full))
    wifiTxPowerRuntimeFault = true;
}

void manageAdaptiveWifiPower() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!config.ecoMode || !config.adaptiveWifiPower ||
      wifiTxPowerRuntimeFault) {
    forceFullWifiPower();
    return;
  }
  const uint32_t now = millis();
  if (!wifiConnectedSinceMs || now - wifiConnectedSinceMs < kWifiPowerStableMs ||
      now - lastWifiPowerEvaluateMs < kWifiPowerEvaluateMs)
    return;
  lastWifiPowerEvaluateMs = now;
  const int32_t rssi = WiFi.RSSI();
  WifiTxProfile target = wifiTxProfile;
  switch (wifiTxProfile) {
    case WifiTxProfile::Full:
      if (rssi >= -60) target = WifiTxProfile::Medium;
      break;
    case WifiTxProfile::Medium:
      if (rssi >= -53)
        target = WifiTxProfile::Reduced;
      else if (rssi <= -67)
        target = WifiTxProfile::Full;
      break;
    case WifiTxProfile::Reduced:
      if (rssi <= -67)
        target = WifiTxProfile::Full;
      else if (rssi <= -60)
        target = WifiTxProfile::Medium;
      break;
  }
  if (target != wifiTxProfile && !setWifiTxProfile(target)) {
    wifiTxPowerRuntimeFault = true;
    forceFullWifiPower();
  }
}

bool trackerFaultActive() {
  const bool meterFresh =
      meter.lastTelegramMs &&
      millis() - meter.lastTelegramMs < kReadingStaleMs;
  return !meterFresh || WiFi.status() != WL_CONNECTED || !history.ready() ||
         heapWarningActive || cpuEcoRuntimeFault ||
         (meter.telegrams && !meter.lastCrcValid);
}

bool ecoLedSuppressed() {
  return config.ecoMode && config.ecoLedOff && !trackerFaultActive();
}

bool cpuBoostActive() {
  return config.ecoMode && !cpuEcoRuntimeFault && cpuBoostUntilMs &&
         static_cast<int32_t>(cpuBoostUntilMs - millis()) > 0;
}

uint32_t cpuBoostRemainingSeconds() {
  if (!cpuBoostActive()) return 0;
  return (static_cast<uint32_t>(cpuBoostUntilMs - millis()) + 999U) / 1000U;
}

bool switchCpuFrequency(uint32_t targetMhz) {
  if (getCpuFrequencyMhz() == targetMhz) return true;
  if (!setCpuFrequencyMhz(targetMhz)) {
    ++cpuFrequencyErrors;
    return false;
  }
  ++cpuFrequencySwitches;
  return true;
}

void disableCpuEcoForRuntime(const char *reason) {
  cpuEcoRuntimeFault = true;
  cpuBoostUntilMs = 0;
  strlcpy(cpuBoostReason, reason, sizeof(cpuBoostReason));
  switchCpuFrequency(kPerformanceCpuMhz);
}

void requestCpuBoost(const char *reason) {
  if (!config.ecoMode || cpuEcoRuntimeFault) return;
  cpuBoostUntilMs = millis() + kCpuBoostHoldMs;
  strlcpy(cpuBoostReason, reason, sizeof(cpuBoostReason));
  if (!switchCpuFrequency(kPerformanceCpuMhz))
    disableCpuEcoForRuntime("frequency_error");
}

void startCpuPowerMode() {
  cpuBoostUntilMs = 0;
  if (!config.ecoMode) {
    strlcpy(cpuBoostReason, "eco_disabled", sizeof(cpuBoostReason));
    switchCpuFrequency(kPerformanceCpuMhz);
    return;
  }
  strlcpy(cpuBoostReason, "eco_idle", sizeof(cpuBoostReason));
  if (!switchCpuFrequency(kEcoCpuMhz))
    disableCpuEcoForRuntime("frequency_error");
}

void manageCpuPowerMode() {
  if (!config.ecoMode || cpuEcoRuntimeFault || !cpuBoostUntilMs) return;
  if (static_cast<int32_t>(millis() - cpuBoostUntilMs) < 0) return;
  cpuBoostUntilMs = 0;
  strlcpy(cpuBoostReason, "eco_idle", sizeof(cpuBoostReason));
  if (!switchCpuFrequency(kEcoCpuMhz))
    disableCpuEcoForRuntime("frequency_error");
}

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

String jsonEscape(const String &value) {
  String out;
  out.reserve(value.length() + 8);
  for (char c : value) {
    if (c == '"' || c == '\\') out += '\\';
    if (c == '\n') {
      out += "\\n";
    } else if (c != '\r') {
      out += c;
    }
  }
  return out;
}

String htmlEscape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}

bool safeSingleLine(const String &value, size_t maximumLength) {
  return value.length() <= maximumLength && value.indexOf('\r') < 0 &&
         value.indexOf('\n') < 0 && value.indexOf('\0') < 0;
}

bool validWifiPassword(const String &value) {
  if (!safeSingleLine(value, 64)) return false;
  if (value.length() <= 63) return true;
  // DE: WPA2 erlaubt alternativ zu einer Passphrase exakt 64 Hex-Zeichen.
  // EN: WPA2 permits exactly 64 hexadecimal characters instead of a passphrase.
  for (const char character : value)
    if (!isxdigit(static_cast<unsigned char>(character))) return false;
  return true;
}

bool validHostname(const String &value) {
  if (!value.length() || value.length() > 32 || value[0] == '-' ||
      value[value.length() - 1] == '-')
    return false;
  for (char c : value)
    if (!isalnum(static_cast<unsigned char>(c)) && c != '-') return false;
  return true;
}

String localAdminPassword() {
  if (config.adminPassword.length() >= 4) return config.adminPassword;
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X",
           static_cast<unsigned>(ESP.getEfuseMac() & 0xffff));
  return "IRTracker-" + String(suffix);
}

String hexBytes(const uint8_t *data, size_t length) {
  static const char hex[] = "0123456789abcdef";
  String output;
  output.reserve(length * 2);
  for (size_t i = 0; i < length; ++i) {
    output += hex[data[i] >> 4];
    output += hex[data[i] & 0x0f];
  }
  return output;
}

String sessionSignature(const String &expiry) {
  String key = localAdminPassword() + ":";
  key += String(static_cast<uint32_t>(ESP.getEfuseMac() >> 32), HEX);
  key += String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
  key += ":irtracker-session-v1";
  uint8_t digest[32] = {};
  const mbedtls_md_info_t *info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info ||
      mbedtls_md_hmac(
          info, reinterpret_cast<const unsigned char *>(key.c_str()),
          key.length(),
          reinterpret_cast<const unsigned char *>(expiry.c_str()),
          expiry.length(), digest) != 0) {
    key = "";
    return "";
  }
  key = "";
  const String signature = hexBytes(digest, sizeof(digest));
  memset(digest, 0, sizeof(digest));
  return signature;
}

bool constantTimeEqual(const String &left, const String &right) {
  if (left.length() != right.length()) return false;
  uint8_t difference = 0;
  for (size_t i = 0; i < left.length(); ++i)
    difference |= static_cast<uint8_t>(left[i] ^ right[i]);
  return difference == 0;
}

bool validBrowserSession() {
  const time_t now = time(nullptr);
  if (now < 1700000000) {
    browserSessionState = "time_invalid";
    return false;
  }
  const String cookieHeader = server.header("Cookie");
  const String marker = "ir_session=";
  int start = cookieHeader.indexOf(marker);
  if (start < 0) {
    browserSessionState = "missing";
    return false;
  }
  start += marker.length();
  int end = cookieHeader.indexOf(';', start);
  if (end < 0) end = cookieHeader.length();
  const String token = cookieHeader.substring(start, end);
  const int separator = token.indexOf('.');
  if (separator <= 0 || separator >= static_cast<int>(token.length() - 1)) {
    browserSessionState = "malformed";
    return false;
  }
  const String expiry = token.substring(0, separator);
  if (expiry.length() < 10 || expiry.length() > 11) {
    browserSessionState = "malformed";
    return false;
  }
  for (char c : expiry)
    if (!isDigit(c)) {
      browserSessionState = "malformed";
      return false;
    }
  const uint64_t expiresAt = strtoull(expiry.c_str(), nullptr, 10);
  if (expiresAt <= static_cast<uint64_t>(now) ||
      expiresAt > static_cast<uint64_t>(now) + kBrowserSessionSeconds + 300) {
    browserSessionState = "expired";
    return false;
  }
  const bool valid =
      constantTimeEqual(token.substring(separator + 1),
                        sessionSignature(expiry));
  browserSessionState = valid ? "valid" : "signature_invalid";
  return valid;
}

void issueBrowserSession() {
  const time_t now = time(nullptr);
  if (now < 1700000000) return;
  const String expiry =
      String(static_cast<uint64_t>(now) + kBrowserSessionSeconds);
  const String signature = sessionSignature(expiry);
  if (!signature.length()) return;
  server.sendHeader(
      "Set-Cookie",
      "ir_session=" + expiry + "." + signature +
          "; Max-Age=" + String(kBrowserSessionSeconds) +
          "; Path=/; HttpOnly; SameSite=Strict",
      false);
}

bool timePending(uint32_t deadline) {
  return deadline && static_cast<int32_t>(millis() - deadline) < 0;
}

LoginGuard &loginGuardFor(const IPAddress &ip) {
  LoginGuard *oldest = &loginGuards[0];
  for (auto &guard : loginGuards) {
    if (guard.ip == ip) return guard;
    if (guard.lastSeenMs < oldest->lastSeenMs) oldest = &guard;
  }
  *oldest = LoginGuard{};
  oldest->ip = ip;
  return *oldest;
}

bool validCsrfRequest() {
  if (server.method() != HTTP_POST) return true;
  const String supplied = server.header("X-CSRF-Token").length()
                              ? server.header("X-CSRF-Token")
                              : server.arg("csrf_token");
  if (csrfToken.length() != 64 || supplied != csrfToken) {
    server.send(403, "application/json", "{\"error\":\"csrf_token_invalid\"}");
    return false;
  }
  return true;
}

bool requireAdmin() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("X-Frame-Options", "DENY");
  server.sendHeader("Referrer-Policy", "no-referrer");
  server.sendHeader("Cross-Origin-Resource-Policy", "same-origin");
  server.sendHeader(
      "Content-Security-Policy",
      "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src "
      "'self' 'unsafe-inline'; connect-src 'self' ws:; img-src 'self' data:; "
      "object-src 'none'; base-uri 'none'; form-action 'self'; "
      "frame-ancestors 'none'");
  server.sendHeader("Permissions-Policy",
                    "camera=(), microphone=(), geolocation=()");
  if (validBrowserSession()) return validCsrfRequest();
  const IPAddress remote = server.client().remoteIP();
  LoginGuard &guard = loginGuardFor(remote);
  guard.lastSeenMs = millis();
  if (timePending(guard.lockUntilMs)) {
    server.sendHeader("Retry-After",
                      String((guard.lockUntilMs - millis()) / 1000 + 1));
    server.send(429, "application/json",
                "{\"error\":\"too_many_login_attempts\"}");
    return false;
  }
  if (guard.lockUntilMs) {
    guard.failures = 0;
    guard.lockUntilMs = 0;
  }
  const String password = localAdminPassword();
  if (server.authenticate("admin", password.c_str())) {
    guard.failures = 0;
    guard.firstFailureMs = 0;
    issueBrowserSession();
    return validCsrfRequest();
  }
  // DE: Eine Anfrage ohne Zugangsdaten öffnet nur den Browserdialog und zählt
  // nicht als Fehlversuch. | EN: A credential-free request only opens the
  // browser login dialog and does not count as a failed attempt.
  if (server.header("Authorization").length()) {
    if (!guard.firstFailureMs ||
        millis() - guard.firstFailureMs > kLoginWindowMs) {
      guard.failures = 0;
      guard.firstFailureMs = millis();
    }
    if (++guard.failures >= 5) {
      guard.failures = 0;
      guard.lockLevel = std::min<uint8_t>(guard.lockLevel + 1, 4);
      const uint32_t duration =
          std::min<uint32_t>(5UL * 60UL * 1000UL
                                 << (guard.lockLevel - 1),
                             kLoginMaxLockMs);
      guard.lockUntilMs = millis() + duration;
      server.sendHeader("Retry-After", String(duration / 1000));
      server.send(429, "application/json",
                  "{\"error\":\"too_many_login_attempts\"}");
      return false;
    }
  }
  server.requestAuthentication(BASIC_AUTH, "IR-Tracker Einstellungen",
                               "Anmeldung erforderlich");
  return false;
}

bool requireApiAccess() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  if (config.apiAccess == 0) return true;
  const String password = localAdminPassword();
  if (server.authenticate("admin", password.c_str())) return true;
  if (config.apiAccess == 2) {
    server.send(404, "application/json", "{\"error\":\"api_disabled\"}");
    return false;
  }
  return requireAdmin();
}

String numberOrNull(double value, uint8_t decimals = 3) {
  return std::isfinite(value) ? String(value, static_cast<unsigned int>(decimals)) : "null";
}

String resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "unknown";
  }
}

void createCsrfToken() {
  uint8_t randomBytes[32];
  esp_fill_random(randomBytes, sizeof(randomBytes));
  static const char hex[] = "0123456789abcdef";
  csrfToken = "";
  csrfToken.reserve(64);
  for (uint8_t value : randomBytes) {
    csrfToken += hex[value >> 4];
    csrfToken += hex[value & 0x0f];
  }
  memset(randomBytes, 0, sizeof(randomBytes));
}

void loadConfig() {
  prefs.begin("offline", true);
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    config.ssid[i] = prefs.getString(("ssid" + String(i)).c_str(), "");
    config.password[i] = prefs.getString(("pass" + String(i)).c_str(), "");
  }
  // DE: Migration von Firmware 0.1/0.2. | EN: Migration from firmware 0.1/0.2.
  if (!config.ssid[0].length()) {
    config.ssid[0] = prefs.getString("ssid", "");
    config.password[0] = prefs.getString("password", "");
  }
  config.hostname = prefs.getString("hostname", "ir-tracker");
  config.rxPin = prefs.getUChar("rx_pin", kDefaultRxPin);
  config.txPin = prefs.getChar("tx_pin", kDefaultTxPin);
  config.ledPin = prefs.getChar("led_pin", 5);
  config.ledInverted = prefs.getBool("led_inv", true);
  config.baud = prefs.getULong("baud", kDefaultBaud);
  config.mqttHost = prefs.getString("mqtt_host", "");
  config.mqttPort = prefs.getUShort("mqtt_port", 1883);
  config.mqttUser = prefs.getString("mqtt_user", "");
  config.mqttPassword = prefs.getString("mqtt_pass", "");
  config.homeAssistantDiscovery = prefs.getBool("ha_disc", true);
  config.apiAccess = prefs.getUChar("api_access", 0);
  config.snifferEnabled = prefs.getBool("sniffer", false);
  config.bridgeEnabled = prefs.getBool("bridge", false);
  config.meterPin = prefs.getString("meter_pin", "");
  config.autoPin = prefs.getBool("auto_pin", false);
  config.pinInverted = prefs.getBool("pin_inv", false);
  config.pinPulseMs = prefs.getUShort("pin_pulse", 300);
  config.pinDigitGapMs = prefs.getUShort("pin_gap", 3000);
  config.adminPassword = prefs.getString("admin_pass", "");
  config.timezone = prefs.getString(
      "timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
  config.setupApMinutes = prefs.getUShort("ap_minutes", 15);
  config.persistEventLog = prefs.getBool("event_flash", false);
  config.ecoMode = prefs.getBool("eco_mode", true);
  config.ecoLedOff = prefs.getBool("eco_led_off", true);
  config.adaptiveWifiPower = prefs.getBool("wifi_power_auto", true);
  config.githubUpdateCheck = prefs.getBool("gh_check", true);
  config.githubAutoInstall = prefs.getBool("gh_auto", false);
  energyConfig.driver = static_cast<EnergyManager::Driver>(
      prefs.getUChar("em_drv", 0));
  energyConfig.enabled = prefs.getBool("em_enable", false);
  energyConfig.dryRun = prefs.getBool("em_dry", true);
  energyConfig.inverted = prefs.getBool("em_inv", false);
  energyConfig.host = prefs.getString("em_host", "");
  energyConfig.port = prefs.getUShort("em_port", 502);
  energyConfig.unitId = prefs.getUChar("em_unit", 1);
  energyConfig.powerRegister = prefs.getUShort("em_reg", 0);
  energyConfig.registerWidth = prefs.getUChar("em_width", 1);
  energyConfig.wordSwap = prefs.getBool("em_swap", false);
  energyConfig.registerScale = prefs.getFloat("em_scale", 1.0f);
  energyConfig.mqttTopic = prefs.getString("em_topic", "");
  energyConfig.mqttPayload = prefs.getString("em_mqpay", "{power}");
  energyConfig.httpPath = prefs.getString("em_path", "/api/power");
  energyConfig.httpMethod = prefs.getString("em_method", "POST");
  energyConfig.httpPayload = prefs.getString(
      "em_httppay", "{\"setpoint_w\":{power},\"grid_w\":{grid}}");
  energyConfig.httpBearerToken = prefs.getString("em_token", "");
  energyConfig.targetGridW = prefs.getShort("em_target", 0);
  energyConfig.deadbandW = prefs.getUShort("em_dead", 30);
  energyConfig.maxChargeW = prefs.getUShort("em_charge", 800);
  energyConfig.maxDischargeW = prefs.getUShort("em_discharge", 800);
  energyConfig.rampWPerSecond = prefs.getUShort("em_ramp", 200);
  energyConfig.intervalMs = prefs.getUShort("em_interval", 2000);
  energyConfig.staleMs = prefs.getUShort("em_stale", 10000);
  prefs.end();
  if (config.rxPin > 10) config.rxPin = kDefaultRxPin;
  if (config.txPin > 10) config.txPin = -1;
  if (config.ledPin > 10) config.ledPin = -1;
  if (config.baud < 300 || config.baud > 115200) config.baud = kDefaultBaud;
  if (config.meterPin.length() != 4) {
    config.meterPin = "";
  }
  // DE: Beim LEPUS erfolgt die PIN-Eingabe per Taste/Taschenlampe; automatische
  // IR-Folgen bleiben wegen fehlender sicherer Bestätigung aus. | EN: LEPUS PIN
  // entry uses its button/flashlight; automatic IR sequences stay disabled
  // because the meter does not acknowledge them reliably.
  config.autoPin = false;
  config.apiAccess = constrain(config.apiAccess, 0, 2);
  config.setupApMinutes = constrain(config.setupApMinutes, 5, 60);
  config.pinPulseMs = constrain(config.pinPulseMs, 50, 1000);
  config.pinDigitGapMs = constrain(config.pinDigitGapMs, 1000, 10000);
  if (config.adminPassword.length() && config.adminPassword.length() < 4)
    config.adminPassword = "";
  if (!config.timezone.length() || config.timezone.length() > 80)
    config.timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
  if (static_cast<uint8_t>(energyConfig.driver) >
      static_cast<uint8_t>(EnergyManager::Driver::Http)) {
    energyConfig.driver = EnergyManager::Driver::Disabled;
  }
  energyConfig.registerWidth =
      energyConfig.registerWidth == 2 ? 2 : 1;
  energyConfig.intervalMs = constrain(energyConfig.intervalMs, 1000, 30000);
  energyConfig.staleMs = constrain(energyConfig.staleMs, 3000, 60000);
  energyConfig.deadbandW = constrain(energyConfig.deadbandW, 0, 500);
  // DE: Version 1.x ist nur ein Zähler; niemals Stellbefehle wiederherstellen
  // oder ausführen. | EN: Version 1.x is a read-only meter; never restore or
  // execute actuator writes.
  energyConfig.enabled = false;
  energyConfig.dryRun = true;
  energyConfig.driver = EnergyManager::Driver::Disabled;
  energyManager.configure(energyConfig);
}

void saveConfig() {
  prefs.begin("offline", false);
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    prefs.putString(("ssid" + String(i)).c_str(), config.ssid[i]);
    prefs.putString(("pass" + String(i)).c_str(), config.password[i]);
  }
  prefs.putString("hostname", config.hostname);
  prefs.putUChar("rx_pin", config.rxPin);
  prefs.putChar("tx_pin", config.txPin);
  prefs.putChar("led_pin", config.ledPin);
  prefs.putBool("led_inv", config.ledInverted);
  prefs.putULong("baud", config.baud);
  prefs.putString("mqtt_host", config.mqttHost);
  prefs.putUShort("mqtt_port", config.mqttPort);
  prefs.putString("mqtt_user", config.mqttUser);
  prefs.putString("mqtt_pass", config.mqttPassword);
  prefs.putBool("ha_disc", config.homeAssistantDiscovery);
  prefs.putUChar("api_access", config.apiAccess);
  prefs.putBool("sniffer", config.snifferEnabled);
  prefs.putBool("bridge", config.bridgeEnabled);
  prefs.putString("meter_pin", config.meterPin);
  prefs.putBool("auto_pin", config.autoPin);
  prefs.putBool("pin_inv", config.pinInverted);
  prefs.putUShort("pin_pulse", config.pinPulseMs);
  prefs.putUShort("pin_gap", config.pinDigitGapMs);
  prefs.putString("admin_pass", config.adminPassword);
  prefs.putString("timezone", config.timezone);
  prefs.putUShort("ap_minutes", config.setupApMinutes);
  prefs.putBool("event_flash", config.persistEventLog);
  prefs.putBool("eco_mode", config.ecoMode);
  prefs.putBool("eco_led_off", config.ecoLedOff);
  prefs.putBool("wifi_power_auto", config.adaptiveWifiPower);
  prefs.putBool("gh_check", config.githubUpdateCheck);
  prefs.putBool("gh_auto", config.githubAutoInstall);
  prefs.putUChar("em_drv", static_cast<uint8_t>(energyConfig.driver));
  prefs.putBool("em_enable", energyConfig.enabled);
  prefs.putBool("em_dry", energyConfig.dryRun);
  prefs.putBool("em_inv", energyConfig.inverted);
  prefs.putString("em_host", energyConfig.host);
  prefs.putUShort("em_port", energyConfig.port);
  prefs.putUChar("em_unit", energyConfig.unitId);
  prefs.putUShort("em_reg", energyConfig.powerRegister);
  prefs.putUChar("em_width", energyConfig.registerWidth);
  prefs.putBool("em_swap", energyConfig.wordSwap);
  prefs.putFloat("em_scale", energyConfig.registerScale);
  prefs.putString("em_topic", energyConfig.mqttTopic);
  prefs.putString("em_mqpay", energyConfig.mqttPayload);
  prefs.putString("em_path", energyConfig.httpPath);
  prefs.putString("em_method", energyConfig.httpMethod);
  prefs.putString("em_httppay", energyConfig.httpPayload);
  prefs.putString("em_token", energyConfig.httpBearerToken);
  prefs.putShort("em_target", energyConfig.targetGridW);
  prefs.putUShort("em_dead", energyConfig.deadbandW);
  prefs.putUShort("em_charge", energyConfig.maxChargeW);
  prefs.putUShort("em_discharge", energyConfig.maxDischargeW);
  prefs.putUShort("em_ramp", energyConfig.rampWPerSecond);
  prefs.putUShort("em_interval", energyConfig.intervalMs);
  prefs.putUShort("em_stale", energyConfig.staleMs);
  prefs.end();
}

String nav() {
  return F("<nav><a href='/'>Dashboard</a><a href='/setup'>Einstellungen</a>"
           "<a href='/history'>Historie</a><a href='/interfaces'>Schnittstellen</a>"
           "<a href='/maintenance'>Wartung</a>"
           "<button id='langToggle' class='theme-toggle' "
           "type='button' aria-label='Sprache wechseln'>English</button>"
           "<button id='themeToggle' class='theme-toggle' "
           "type='button' aria-expanded='false' aria-controls='themePanel'>Farben</button></nav>"
           "<aside id='themePanel' class='theme-panel' hidden aria-label='Farbschema anpassen'>"
           "<div class='theme-head'><strong>Farbschema</strong><button id='themeClose' class='secondary' type='button' aria-label='Schließen'>×</button></div>"
           "<p class='muted'>Farben werden sofort und ausschließlich in diesem Browser gespeichert.</p>"
           "<div class='theme-grid'>"
           "<label>Hintergrund<input type='color' data-theme-var='--bg'></label>"
           "<label>Karten<input type='color' data-theme-var='--card'></label>"
           "</div><button id='themeReset' class='secondary' type='button'>Standardfarben wiederherstellen</button>"
           "</aside>");
}

String maintenanceTabs(const bool diagnostics) {
  return String(F("<div class='subnav' aria-label='Wartungsbereiche'>"
                  "<a href='/maintenance'")) +
         (diagnostics ? "" : " class='active'") +
         F(">Backup &amp; System</a><a href='/maintenance/diagnostics'") +
         (diagnostics ? " class='active'" : "") +
         F(">Diagnose &amp; Zähler</a></div>");
}

String page(const String &title, const String &body,
            const String &script = "", const String &assetPath = "") {
  String html;
  html.reserve(body.length() + script.length() + 3600);
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += "<title>" + title + "</title>";
  html += "<script>window.IR_TRACKER_CONFIG={csrfToken:'" + csrfToken +
          "',firmwareVersion:'" + String(kFirmwareVersion) + "'};</script>";
  html += F("<script src='/assets/common.js?v=");
  html += kFirmwareVersion;
  html += F("'></script><link rel='stylesheet' href='/assets/common.css?v=");
  html += kFirmwareVersion;
  html += F("'></head><body><main>");
  html += nav();
  html += "<h1>" + title + "</h1><!--IR_BODY-->" + body;
  if (script.length()) html += "<script>" + script + "</script>";
  html += F("<script src='/assets/i18n.js?v=");
  html += kFirmwareVersion;
  html += F("'></script>");
  if (assetPath.length())
    html += "<script src='" + htmlEscape(assetPath) + "'></script>";
  html += "<footer>Firmware von " + String(kFirmwareAuthor) +
          " · © 2026 Michael Roßmann · " + String(kFirmwareLicense) +
          " · nur nichtkommerzielle Nutzung<br>Unabhängiges Community-Projekt; "
          "nicht mit Solakon verbunden und nicht von Solakon unterstützt.</footer>";
  html += F("</main></body></html>");
  return html;
}

bool sendPageStreamed(const String &title, const String &body,
                      const String &assetPath) {
  // DE: Der gemeinsame Rahmen bleibt klein. Seitentext und komprimiertes
  // JavaScript werden getrennt übertragen. | EN: The common shell remains
  // small. Page markup and compressed JavaScript are transferred separately.
  String shell = page(title, "", "", assetPath);
  const String marker = "<!--IR_BODY-->";
  const int insertion = shell.indexOf(marker);
  if (insertion < 0 || shell.indexOf("</html>") < 0) return false;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent(shell.substring(0, insertion));
  server.sendContent(body);
  shell.remove(0, insertion + marker.length());
  server.sendContent(shell);
  server.sendContent("");
  return true;
}

bool matchAt(const std::vector<uint8_t> &data, size_t pos, const uint8_t *needle, size_t length) {
  return pos + length <= data.size() && memcmp(data.data() + pos, needle, length) == 0;
}

struct SmlNumber {
  bool valid = false;
  double value = 0;
  size_t next = 0;
};

SmlNumber readSmlNumber(const std::vector<uint8_t> &data, size_t pos) {
  if (pos >= data.size()) return {};
  const uint8_t tl = data[pos];
  const uint8_t type = tl & 0x70;
  const uint8_t len = tl & 0x0f;
  if ((type != 0x50 && type != 0x60) || len < 2 || pos + len > data.size()) return {};
  uint64_t raw = 0;
  for (size_t i = pos + 1; i < pos + len; ++i) raw = (raw << 8) | data[i];
  int64_t signedValue = static_cast<int64_t>(raw);
  if (type == 0x50) {
    const uint8_t bits = (len - 1) * 8;
    if (bits < 64 && (raw & (uint64_t(1) << (bits - 1)))) {
      signedValue =
          static_cast<int64_t>(raw | (~uint64_t(0) << bits));
    }
  }
  SmlNumber result;
  result.valid = true;
  result.value = type == 0x50 ? static_cast<double>(signedValue) : static_cast<double>(raw);
  result.next = pos + len;
  return result;
}

bool extractObisFrom(const std::vector<uint8_t> &data, const uint8_t obis[6], double &target) {
  for (size_t i = 0; i + 6 < data.size(); ++i) {
    if (memcmp(data.data() + i, obis, 6) != 0) continue;
    std::vector<SmlNumber> numbers;
    const size_t limit = std::min(data.size(), i + 52);
    for (size_t p = i + 6; p < limit;) {
      if (p > i + 8 && data[p] == 0x77) break;
      SmlNumber number = readSmlNumber(data, p);
      if (number.valid) {
        numbers.push_back(number);
        p = number.next;
      } else {
        ++p;
      }
    }
    if (numbers.empty()) continue;
    double value = numbers.back().value;
    if (numbers.size() >= 2) {
      const int scaler = static_cast<int>(numbers[numbers.size() - 2].value);
      if (scaler >= -9 && scaler <= 9) value *= pow(10.0, scaler);
    }
    target = value;
    return true;
  }
  return false;
}

bool extractObis(const uint8_t obis[6], double &target) {
  return extractObisFrom(telegram, obis, target);
}

uint16_t smlCrc16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xffff;
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
  }
  return crc ^ 0xffff;
}

void parseTelegram() {
  const uint8_t powerObis[] = {0x01, 0x00, 0x10, 0x07, 0x00, 0xff};
  const uint8_t importObis[] = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};
  const uint8_t exportObis[] = {0x01, 0x00, 0x02, 0x08, 0x00, 0xff};
  const uint8_t phasePowerObis[3][6] = {
      {0x01, 0x00, 0x24, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x38, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x4c, 0x07, 0x00, 0xff}};
  const uint8_t phaseVoltageObis[3][6] = {
      {0x01, 0x00, 0x20, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x34, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x48, 0x07, 0x00, 0xff}};
  const uint8_t phaseCurrentObis[3][6] = {
      {0x01, 0x00, 0x1f, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x33, 0x07, 0x00, 0xff},
      {0x01, 0x00, 0x47, 0x07, 0x00, 0xff}};
  const bool firstValidTelegram = meter.lastTelegramMs == 0;
  lastTelegram = telegram;
  ++meter.telegrams;
  meter.lastCrcValid = false;
  if (telegram.size() < 12) {
    ++meter.parseErrors;
    return;
  }
  const size_t crcLowIndex = telegram.size() - 2;
  const uint16_t expected =
      telegram[crcLowIndex] |
      (static_cast<uint16_t>(telegram[crcLowIndex + 1]) << 8);
  const uint16_t actual = smlCrc16(telegram.data(), crcLowIndex);
  meter.lastCrcValid = expected == actual;
  if (!meter.lastCrcValid) {
    // DE: Defekte Frames dürfen Livewerte/Historie nie ändern. | EN: Damaged frames must never alter live values/history.
    ++meter.crcErrors;
    return;
  }

  MeterValues candidate;
  bool found = false;
  found |= extractObis(powerObis, candidate.powerW);
  double importWh = NAN;
  double exportWh = NAN;
  if (extractObis(importObis, importWh)) {
    candidate.importKwh = importWh / 1000.0;
    found = true;
  }
  if (extractObis(exportObis, exportWh)) {
    candidate.exportKwh = exportWh / 1000.0;
    found = true;
  }
  for (uint8_t phase = 0; phase < 3; ++phase) {
    found |= extractObis(phasePowerObis[phase],
                         candidate.phasePowerW[phase]);
    found |= extractObis(phaseVoltageObis[phase],
                         candidate.phaseVoltageV[phase]);
    found |= extractObis(phaseCurrentObis[phase],
                         candidate.phaseCurrentA[phase]);
  }
  if (std::isfinite(candidate.phasePowerW[0]) &&
      std::isfinite(candidate.phasePowerW[1]) &&
      std::isfinite(candidate.phasePowerW[2])) {
    // DE: Die saldierte Summe folgt den vorzeichenbehafteten Phasenleistungen. | EN: The net total follows signed phase powers.
    candidate.powerW = candidate.phasePowerW[0] +
                       candidate.phasePowerW[1] +
                       candidate.phasePowerW[2];
  }
  const auto plausible = [](double value, double maximum) {
    return !std::isfinite(value) || std::abs(value) <= maximum;
  };
  bool plausibleValues =
      plausible(candidate.powerW, 100000.0) &&
      (!std::isfinite(candidate.importKwh) ||
       (candidate.importKwh >= 0.0 && candidate.importKwh <= 1.0e9)) &&
      (!std::isfinite(candidate.exportKwh) ||
       (candidate.exportKwh >= 0.0 && candidate.exportKwh <= 1.0e9));
  for (uint8_t phase = 0; phase < 3; ++phase) {
    plausibleValues &= plausible(candidate.phasePowerW[phase], 100000.0);
    plausibleValues &=
        !std::isfinite(candidate.phaseVoltageV[phase]) ||
        (candidate.phaseVoltageV[phase] >= 0.0 &&
         candidate.phaseVoltageV[phase] <= 500.0);
    plausibleValues &=
        !std::isfinite(candidate.phaseCurrentA[phase]) ||
        (candidate.phaseCurrentA[phase] >= 0.0 &&
         candidate.phaseCurrentA[phase] <= 200.0);
  }
  if (!found || !plausibleValues) {
    ++meter.parseErrors;
    return;
  }

  // DE: Atomar erst nach CRC, Parsing und Plausibilität übernehmen. | EN: Commit atomically only after CRC, parsing and plausibility succeed.
  meter.powerW = candidate.powerW;
  meter.importKwh = candidate.importKwh;
  meter.exportKwh = candidate.exportKwh;
  for (uint8_t phase = 0; phase < 3; ++phase) {
    meter.phasePowerW[phase] = candidate.phasePowerW[phase];
    meter.phaseVoltageV[phase] = candidate.phaseVoltageV[phase];
    meter.phaseCurrentA[phase] = candidate.phaseCurrentA[phase];
  }
  if (firstValidTelegram)
    eventLog.add("INFO", "METER_FIRST",
                 "Erstes gültiges Zählertelegramm empfangen");
  meter.lastTelegramMs = millis();
}

void consumeMeterByte(uint8_t value) {
  ++meter.bytes;
  if (!capturing) {
    if (value == kSmlStart[startMatched]) {
      ++startMatched;
      if (startMatched == sizeof(kSmlStart)) {
        telegram.assign(kSmlStart, kSmlStart + sizeof(kSmlStart));
        capturing = true;
        startMatched = 0;
      }
    } else {
      startMatched = value == kSmlStart[0] ? 1 : 0;
    }
    return;
  }
  telegram.push_back(value);
  if (smlTrailerRemaining) {
    if (--smlTrailerRemaining == 0) {
      parseTelegram();
      telegram.clear();
      capturing = false;
    }
    return;
  }
  if (telegram.size() > kTelegramMax) {
    telegram.clear();
    capturing = false;
    ++meter.parseErrors;
    return;
  }
  if (telegram.size() >= sizeof(kSmlEnd) &&
      matchAt(telegram, telegram.size() - sizeof(kSmlEnd), kSmlEnd, sizeof(kSmlEnd))) {
    // DE: SML-Trailer nach 1A: Füllbytes und CRC16 Little Endian. | EN: SML trailer after 1A: fill-byte count and CRC16 little-endian.
    smlTrailerRemaining = 3;
  }
}

void resetSmlCapture() {
  telegram.clear();
  startMatched = 0;
  capturing = false;
  smlTrailerRemaining = 0;
}

void restoreMeterSerialAfterScan() {
  meterSerial.end();
  resetSmlCapture();
  meterSerial.begin(config.baud, SERIAL_8N1, config.rxPin, config.txPin);
  if (config.ledPin >= 0) {
    pinMode(config.ledPin, OUTPUT);
    digitalWrite(config.ledPin, config.ledInverted);
  }
}

void finishGpioScan(bool found, const String &error = "") {
  gpioScan.active = false;
  gpioScan.complete = true;
  gpioScan.found = found;
  gpioScan.error = error;
  restoreMeterSerialAfterScan();
  eventLog.add(found ? "INFO" : "WARN", "GPIO_SCAN",
               found ? "IR-Eingang durch CRC-gueltiges SML-Telegramm bestaetigt"
                     : "GPIO-Suche ohne gueltiges SML-Telegramm beendet");
}

void beginGpioScanCandidate() {
  if (gpioScan.pinIndex >= gpioScan.pinCount) {
    finishGpioScan(false, "no_valid_sml_telegram");
    return;
  }
  gpioScan.currentPin = gpioScan.pins[gpioScan.pinIndex];
  gpioScan.currentBaud = gpioScan.bauds[gpioScan.baudIndex];
  meterSerial.end();
  resetSmlCapture();
  pinMode(gpioScan.currentPin, INPUT);
  meterSerial.begin(gpioScan.currentBaud, SERIAL_8N1,
                    gpioScan.currentPin, -1);
  gpioScan.baselineTelegrams = meter.telegrams;
  gpioScan.candidateStartedMs = millis();
}

void startGpioScan() {
  if (gpioScan.active) return;
  irPulse.active = false;
  apatorUnlock.active = false;
  gpioScan = GpioScanState{};
  gpioScan.active = true;

  // DE: Den aktuellen Wert zuerst wirklich pruefen, danach alle anderen
  // zulaessigen C3-Trackerpins. | EN: Really test the current value first,
  // followed by every other allowed C3 tracker pin.
  gpioScan.pins[gpioScan.pinCount++] = config.rxPin;
  for (uint8_t pin = 0; pin <= 10; ++pin)
    if (pin != config.rxPin) gpioScan.pins[gpioScan.pinCount++] = pin;

  gpioScan.bauds[gpioScan.baudCount++] = config.baud;
  const uint32_t commonBauds[] = {9600, 19200, 38400, 115200};
  for (uint32_t baud : commonBauds)
    if (baud != config.baud) gpioScan.bauds[gpioScan.baudCount++] = baud;
  gpioScan.total = gpioScan.pinCount * gpioScan.baudCount;
  requestCpuBoost("gpio_scan");
  beginGpioScanCandidate();
}

void updateGpioScan() {
  if (!gpioScan.active) return;
  if (meter.telegrams > gpioScan.baselineTelegrams &&
      meter.lastCrcValid &&
      meter.lastTelegramMs >= gpioScan.candidateStartedMs) {
    gpioScan.foundPin = gpioScan.currentPin;
    gpioScan.foundBaud = gpioScan.currentBaud;
    ++gpioScan.tested;
    finishGpioScan(true);
    return;
  }
  if (millis() - gpioScan.candidateStartedMs < kGpioScanWindowMs) return;
  ++gpioScan.tested;
  if (++gpioScan.baudIndex >= gpioScan.baudCount) {
    gpioScan.baudIndex = 0;
    ++gpioScan.pinIndex;
  }
  beginGpioScanCandidate();
}

String gpioScanJson() {
  String json = "{\"supported\":true,\"active\":";
  json += gpioScan.active ? "true" : "false";
  json += ",\"complete\":";
  json += gpioScan.complete ? "true" : "false";
  json += ",\"found\":";
  json += gpioScan.found ? "true" : "false";
  json += ",\"tested\":" + String(gpioScan.tested) +
          ",\"total\":" + String(gpioScan.total) +
          ",\"current_pin\":" + String(gpioScan.currentPin) +
          ",\"current_baud\":" + String(gpioScan.currentBaud) +
          ",\"found_pin\":" + String(gpioScan.foundPin) +
          ",\"found_baud\":" + String(gpioScan.foundBaud) +
          ",\"error\":\"" + jsonEscape(gpioScan.error) + "\"}";
  return json;
}

String statusJson() {
  const bool fresh = meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  const bool browserSessionValid = validBrowserSession();
  String json = "{";
  json += "\"firmware\":\"offline-" + String(kFirmwareVersion) + "\",";
  json += "\"installer_wifi_ota\":true,";
  json += "\"installer_gpio_tx_scan\":true,";
  json += "\"author\":\"" + String(kFirmwareAuthor) + "\",";
  json += "\"license\":\"" + String(kFirmwareLicense) + "\",";
  json += "\"transport_security\":\"http_local_trusted_network_only\",";
  json += "\"browser_session_valid\":" +
          String(browserSessionValid ? "true" : "false") + ",";
  json += "\"browser_session_state\":\"" +
          String(browserSessionState) + "\",";
  json += "\"mode\":\"" + String(accessPointMode ? "setup_ap" : "wifi") + "\",";
  json += "\"hostname\":\"" + jsonEscape(config.hostname) + "\",";
  json += "\"ip\":\"" + String(accessPointMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
  json += "\"wifi_rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
  json += "\"wifi_ssid\":\"" + jsonEscape(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "") + "\",";
  json += "\"setup_ap_active\":" +
          String(accessPointMode ? "true" : "false") + ",";
  json += "\"mdns_running\":" + String(mdnsRunning ? "true" : "false") + ",";
  json += "\"mdns_name\":\"" + jsonEscape(config.hostname) + ".local\",";
  json += "\"wifi_sta_only\":" +
          String(WiFi.getMode() == WIFI_STA ? "true" : "false") + ",";
  json += "\"wifi_min_modem_sleep\":" +
          String(wifiMinModemSleepActive ? "true" : "false") + ",";
  json += "\"adaptive_wifi_power\":" +
          String(config.adaptiveWifiPower ? "true" : "false") + ",";
  json += "\"wifi_tx_profile\":\"" + String(wifiTxProfileName()) + "\",";
  json += "\"wifi_tx_power_dbm\":" + String(wifiTxPowerDbm(), 1) + ",";
  json += "\"wifi_tx_power_changes\":" + String(wifiTxPowerChanges) + ",";
  json += "\"wifi_tx_power_errors\":" + String(wifiTxPowerErrors) + ",";
  json += "\"wifi_mode_errors\":" + String(wifiModeErrors) + ",";
  json += "\"wifi_tx_runtime_fault\":" +
          String(wifiTxPowerRuntimeFault ? "true" : "false") + ",";
  json += "\"mqtt_connected\":" + String(mqtt.connected() ? "true" : "false") + ",";
  json += "\"meter_fresh\":" + String(fresh ? "true" : "false") + ",";
  json += "\"power_w\":" + numberOrNull(meter.powerW) + ",";
  json += "\"import_kwh\":" + numberOrNull(meter.importKwh) + ",";
  json += "\"export_kwh\":" + numberOrNull(meter.exportKwh) + ",";
  json += "\"phases\":[";
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (phase) json += ",";
    json += "{\"phase\":\"L" + String(phase + 1) +
            "\",\"power_w\":" + numberOrNull(meter.phasePowerW[phase]) +
            ",\"voltage_v\":" + numberOrNull(meter.phaseVoltageV[phase]) +
            ",\"current_a\":" + numberOrNull(meter.phaseCurrentA[phase]) +
            "}";
  }
  json += "],";
  json += "\"telegrams\":" + String(meter.telegrams) + ",";
  json += "\"received_bytes\":" + String(meter.bytes) + ",";
  json += "\"parse_errors\":" + String(meter.parseErrors) + ",";
  json += "\"crc_errors\":" + String(meter.crcErrors) + ",";
  json += "\"last_crc_valid\":" + String(meter.lastCrcValid ? "true" : "false") + ",";
  json += "\"rx_gpio\":" + String(config.rxPin) + ",";
  json += "\"tx_gpio\":" + String(config.txPin) + ",";
  json += "\"ir_transmitting\":" + String(irPulse.active ? "true" : "false") + ",";
  json += "\"apator_unlock_active\":" +
          String(apatorUnlock.active ? "true" : "false") + ",";
  json += "\"apator_unlock_phase\":" + String(apatorUnlock.phase) + ",";
  json += "\"history_ready\":" + String(history.ready() ? "true" : "false") + ",";
  json += "\"live_history_minutes\":70,";
  json += "\"time_valid\":" + String(time(nullptr) >= 1700000000 ? "true" : "false") + ",";
  json += "\"event_count\":" + String(eventLog.count()) + ",";
  json += "\"event_log_persistent\":" +
          String(eventLog.persistent() ? "true" : "false") + ",";
  json += "\"eco_mode_enabled\":" +
          String(config.ecoMode ? "true" : "false") + ",";
  json += "\"eco_led_idle_off\":" +
          String(config.ecoLedOff ? "true" : "false") + ",";
  json += "\"led_fault_active\":" +
          String(trackerFaultActive() ? "true" : "false") + ",";
  json += "\"led_suppressed\":" +
          String(ecoLedSuppressed() ? "true" : "false") + ",";
  json += "\"eco_runtime_fault\":" +
          String(cpuEcoRuntimeFault ? "true" : "false") + ",";
  json += "\"cpu_frequency_mhz\":" + String(getCpuFrequencyMhz()) + ",";
  json += "\"cpu_boost_active\":" +
          String(cpuBoostActive() ? "true" : "false") + ",";
  json += "\"cpu_boost_remaining_s\":" +
          String(cpuBoostRemainingSeconds()) + ",";
  json += "\"cpu_boost_reason\":\"" + jsonEscape(cpuBoostReason) + "\",";
  json += "\"cpu_frequency_switches\":" + String(cpuFrequencySwitches) + ",";
  json += "\"cpu_frequency_errors\":" + String(cpuFrequencyErrors) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"heap_warning\":" +
          String(heapWarningActive ? "true" : "false") + ",";
  json += "\"restart_reason\":\"" + bootResetReason + "\",";
  json += "\"led_gpio\":" + String(config.ledPin) + ",";
  json += "\"baud\":" + String(config.baud) + ",";
  json += "\"github_update_check\":" +
          String(config.githubUpdateCheck ? "true" : "false") + ",";
  json += "\"github_auto_install\":" +
          String(config.githubAutoInstall ? "true" : "false") + ",";
  json += "\"github_update_available\":" +
          String(githubUpdate.available ? "true" : "false") + ",";
  json += "\"github_update_version\":\"" +
          jsonEscape(githubUpdate.version) + "\",";
  json += "\"uptime_s\":" + String(millis() / 1000);
  json += "}";
  return json;
}

String bytesToHex(const std::vector<uint8_t> &data) {
  static const char digits[] = "0123456789abcdef";
  String result;
  result.reserve(data.size() * 2);
  for (uint8_t value : data) {
    result += digits[value >> 4];
    result += digits[value & 0x0f];
  }
  return result;
}

String obisJson() {
  String json = "{\"values\":[";
  bool first = true;
  for (size_t i = 2; i + 7 < lastTelegram.size(); ++i) {
    if (lastTelegram[i - 2] != 0x77 || lastTelegram[i - 1] != 0x07) continue;
    const uint8_t *code = lastTelegram.data() + i;
    double value = NAN;
    if (!extractObisFrom(lastTelegram, code, value)) continue;
    if (!first) json += ",";
    first = false;
    char name[28];
    snprintf(name, sizeof(name), "%u-%u:%u.%u.%u*%u",
             code[0], code[1], code[2], code[3], code[4], code[5]);
    json += "{\"obis\":\"" + String(name) + "\",\"value\":" + numberOrNull(value, 6) + "}";
    i += 5;
  }
  json += "]}";
  return json;
}

String memoryJson() {
  String json = "{";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"minimum_free_heap\":" + String(ESP.getMinFreeHeap()) + ",";
  json += "\"heap_size\":" + String(ESP.getHeapSize()) + ",";
  json += "\"flash_size\":" + String(ESP.getFlashChipSize()) + ",";
  json += "\"firmware_size\":" + String(ESP.getSketchSize()) + ",";
  json += "\"free_firmware_space\":" + String(ESP.getFreeSketchSpace()) + ",";
  json += "\"history_used_bytes\":" + String(history.usedBytes()) + ",";
  json += "\"history_total_bytes\":" + String(history.totalBytes());
  json += ",\"eco_mode_enabled\":" +
          String(config.ecoMode ? "true" : "false");
  json += ",\"eco_runtime_fault\":" +
          String(cpuEcoRuntimeFault ? "true" : "false");
  json += ",\"cpu_frequency_mhz\":" + String(getCpuFrequencyMhz());
  json += ",\"cpu_boost_active\":" +
          String(cpuBoostActive() ? "true" : "false");
  json += ",\"cpu_boost_remaining_s\":" +
          String(cpuBoostRemainingSeconds());
  json += ",\"cpu_boost_reason\":\"" + jsonEscape(cpuBoostReason) + "\"";
  json += ",\"cpu_frequency_switches\":" + String(cpuFrequencySwitches);
  json += ",\"cpu_frequency_errors\":" + String(cpuFrequencyErrors);
  json += ",\"heap_warning\":" +
          String(heapWarningActive ? "true" : "false");
  json += ",\"restart_reason\":\"" + bootResetReason + "\"";
  json += "}";
  return json;
}

String metricsText() {
  String text = "# HELP irtracker_power_w Current grid power in watts\n"
                "# TYPE irtracker_power_w gauge\n";
  if (std::isfinite(meter.powerW)) text += "irtracker_power_w " + String(meter.powerW, 3) + "\n";
  text += "# HELP irtracker_import_kwh Imported grid energy\n# TYPE irtracker_import_kwh counter\n";
  if (std::isfinite(meter.importKwh)) text += "irtracker_import_kwh " + String(meter.importKwh, 6) + "\n";
  text += "# HELP irtracker_export_kwh Exported grid energy\n# TYPE irtracker_export_kwh counter\n";
  if (std::isfinite(meter.exportKwh)) text += "irtracker_export_kwh " + String(meter.exportKwh, 6) + "\n";
  text += "irtracker_telegrams_total " + String(meter.telegrams) + "\n";
  text += "irtracker_parse_errors_total " + String(meter.parseErrors) + "\n";
  text += "irtracker_crc_errors_total " + String(meter.crcErrors) + "\n";
  text += "irtracker_wifi_rssi_dbm " + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + "\n";
  text += "irtracker_mqtt_connected " + String(mqtt.connected() ? 1 : 0) + "\n";
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const String label = "{phase=\"L" + String(phase + 1) + "\"} ";
    if (std::isfinite(meter.phasePowerW[phase]))
      text += "irtracker_phase_power_w" + label +
              String(meter.phasePowerW[phase], 3) + "\n";
    if (std::isfinite(meter.phaseVoltageV[phase]))
      text += "irtracker_phase_voltage_v" + label +
              String(meter.phaseVoltageV[phase], 3) + "\n";
    if (std::isfinite(meter.phaseCurrentA[phase]))
      text += "irtracker_phase_current_a" + label +
              String(meter.phaseCurrentA[phase], 4) + "\n";
  }
  return text;
}

String influxLineProtocol() {
  String line = "irtracker,device=" + deviceId + ",hostname=" + config.hostname;
  bool hasField = false;
  String fields;
  auto addFloat = [&](const char *name, double value) {
    if (!std::isfinite(value)) return;
    if (hasField) fields += ",";
    fields += String(name) + "=" + String(value, 6);
    hasField = true;
  };
  addFloat("power_w", meter.powerW);
  addFloat("import_kwh", meter.importKwh);
  addFloat("export_kwh", meter.exportKwh);
  if (hasField) fields += ",";
  fields += "wifi_rssi=" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + "i";
  fields += ",telegrams=" + String(meter.telegrams) + "i";
  fields += ",parse_errors=" + String(meter.parseErrors) + "i";
  fields += ",crc_errors=" + String(meter.crcErrors) + "i";
  fields += ",meter_fresh=" + String(meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs ? "true" : "false");
  return line + " " + fields + "\n";
}

String csvValues() {
  String csv = "metric,value,unit\n";
  csv += "power_w," + numberOrNull(meter.powerW, 3) + ",W\n";
  csv += "import_kwh," + numberOrNull(meter.importKwh, 6) + ",kWh\n";
  csv += "export_kwh," + numberOrNull(meter.exportKwh, 6) + ",kWh\n";
  csv += "wifi_rssi," + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",dBm\n";
  csv += "telegrams," + String(meter.telegrams) + ",count\n";
  csv += "parse_errors," + String(meter.parseErrors) + ",count\n";
  csv += "crc_errors," + String(meter.crcErrors) + ",count\n";
  return csv;
}

String shellyGen1Status() {
  const bool fresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
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
                ",\"is_valid\":" + String(fresh ? "true" : "false") + "}]}";
  return json;
}

String shellyEmStatus() {
  const bool fresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
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
    json += names[phase];
    json += "_current\":" + numberOrNull(meter.phaseCurrentA[phase], 3);
  }
  json += ",\"errors\":" + String(fresh ? "[]" : "[\"meter_stale\"]") +
          "}";
  return json;
}

String selfTestJson() {
  const bool meterFresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  String json = "{\"tests\":[";
  auto add = [&](const char *id, const char *label, const char *state,
                 const String &detail, bool &first) {
    if (!first) json += ",";
    first = false;
    json += "{\"id\":\"" + String(id) + "\",\"label\":\"" +
            jsonEscape(label) + "\",\"state\":\"" + state +
            "\",\"detail\":\"" + jsonEscape(detail) + "\"}";
  };
  bool first = true;
  add("wifi", "WLAN", WiFi.status() == WL_CONNECTED ? "ok" : "warn",
      WiFi.status() == WL_CONNECTED
          ? WiFi.SSID() + " (" + String(WiFi.RSSI()) +
                " dBm), TX " + String(wifiTxPowerDbm(), 1) + " dBm"
          : "Nicht mit dem Heim-WLAN verbunden",
      first);
  add("wifi_eco", "WLAN-Energiesparen",
      wifiTxPowerRuntimeFault || wifiModeErrors ? "warn" : "ok",
      accessPointMode
          ? "Setup-Hotspot aktiv; volle Sendeleistung"
          : (String("STA, ") +
             (wifiMinModemSleepActive ? "MIN_MODEM" : "Modem-Sleep Fehler") +
             ", Profil " + wifiTxProfileName()),
      first);
  add("time", "Uhrzeit", time(nullptr) >= 1700000000 ? "ok" : "warn",
      time(nullptr) >= 1700000000
          ? String("Synchronisiert, TZ: ") + config.timezone
          : "Noch keine gültige Uhrzeit; Browser oder NTP erforderlich",
      first);
  add("meter", "Zählerempfang", meterFresh ? "ok" : "error",
      meterFresh ? String(meter.telegrams) + " Telegramme empfangen"
                 : "Kein aktuelles Zählertelegramm",
      first);
  add("values", "Messwerte",
      std::isfinite(meter.powerW) ? "ok" : "error",
      std::isfinite(meter.powerW)
          ? String(meter.powerW, 1) + " W"
          : "Leistungs-OBIS fehlt; PIN und Zählerfreigabe prüfen",
      first);
  uint8_t phaseCount = 0;
  for (uint8_t phase = 0; phase < 3; ++phase)
    if (std::isfinite(meter.phasePowerW[phase]) ||
        std::isfinite(meter.phaseVoltageV[phase]) ||
        std::isfinite(meter.phaseCurrentA[phase]))
      ++phaseCount;
  add("phases", "Phasenwerte", phaseCount ? "ok" : "off",
      phaseCount ? String(phaseCount) + " Phasen vom Zähler geliefert"
                 : "Zähler liefert keine einzelnen Phasenwerte",
      first);
  add("history", "Lokale Historie", history.ready() ? "ok" : "error",
      history.ready()
          ? String(history.usedBytes()) + " von " +
                String(history.totalBytes()) + " Bytes verwendet"
          : "Historien-Dateisystem nicht verfügbar",
      first);
  add("memory", "Arbeitsspeicher",
      ESP.getMinFreeHeap() >= 30000 ? "ok" : "warn",
      String(ESP.getFreeHeap()) + " Bytes frei, Minimum " +
          String(ESP.getMinFreeHeap()),
      first);
  add("cpu", "CPU-Energiesparmodus",
      cpuEcoRuntimeFault ? "warn" : (config.ecoMode ? "ok" : "off"),
      cpuEcoRuntimeFault
          ? "Eco-Laufzeitsicherung aktiv; Betrieb mit " +
                String(getCpuFrequencyMhz()) + " MHz"
          : (config.ecoMode
                 ? String(getCpuFrequencyMhz()) + " MHz, " +
                       (cpuBoostActive()
                            ? "Leistungsmodus noch " +
                                  String(cpuBoostRemainingSeconds()) + " s"
                            : "Eco-Betrieb")
                 : "Deaktiviert; dauerhaft 160 MHz"),
      first);
  add("eco_led", "Eco-Status-LED",
      config.ledPin < 0
          ? "off"
          : (trackerFaultActive() ? "warn" : "ok"),
      config.ledPin < 0
          ? "Kein LED-GPIO konfiguriert"
          : (trackerFaultActive()
                 ? "Fehleranzeige aktiv; Eco-Abschaltung wird ueberbrueckt"
                 : (ecoLedSuppressed()
                        ? "Im fehlerfreien Eco-Betrieb ausgeschaltet"
                        : "Normale Statusanzeige aktiv")),
      first);
  add("mqtt", "MQTT",
      !config.mqttHost.length() ? "off" : (mqtt.connected() ? "ok" : "warn"),
      !config.mqttHost.length()
          ? "Nicht konfiguriert"
          : (mqtt.connected() ? "Verbunden" : "Broker nicht erreichbar"),
      first);
  json += "]}";
  return json;
}

String meterReportJson() {
  const bool fresh =
      meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  String json = "{\"manufacturer\":\"unknown\",\"model\":\"SML electricity meter\",";
  json += "\"telegram_fresh\":" + String(fresh ? "true" : "false") + ",";
  json += "\"telegram_count\":" + String(meter.telegrams) + ",";
  json += "\"last_crc_valid\":" +
          String(meter.lastCrcValid ? "true" : "false") + ",";
  json += "\"received\":{\"1.8.0\":" +
          String(std::isfinite(meter.importKwh) ? "true" : "false") +
          ",\"2.8.0\":" +
          String(std::isfinite(meter.exportKwh) ? "true" : "false") +
          ",\"16.7.0\":" +
          String(std::isfinite(meter.powerW) ? "true" : "false") + "},";
  json += "\"phases\":[";
  const uint8_t powerCodes[3] = {36, 56, 76};
  const uint8_t voltageCodes[3] = {32, 52, 72};
  const uint8_t currentCodes[3] = {31, 51, 71};
  for (uint8_t phase = 0; phase < 3; ++phase) {
    if (phase) json += ",";
    json += "{\"phase\":\"L" + String(phase + 1) + "\",";
    json += "\"power_obis\":\"" + String(powerCodes[phase]) +
            ".7.0\",\"power_received\":" +
            String(std::isfinite(meter.phasePowerW[phase]) ? "true" : "false") +
            ",\"voltage_obis\":\"" + String(voltageCodes[phase]) +
            ".7.0\",\"voltage_received\":" +
            String(std::isfinite(meter.phaseVoltageV[phase]) ? "true"
                                                             : "false") +
            ",\"current_obis\":\"" + String(currentCodes[phase]) +
            ".7.0\",\"current_received\":" +
            String(std::isfinite(meter.phaseCurrentA[phase]) ? "true"
                                                             : "false") +
            "}";
  }
  json += "],\"ir_tx\":{\"gpio\":" + String(config.txPin) +
          ",\"inverted\":" + String(config.pinInverted ? "true" : "false") +
          ",\"last_sequence_result\":\"";
  json += std::isfinite(meter.powerW)
              ? "extended_dataset_received"
              : "no_extended_dataset_no_meter_ack_available";
  json += "\"},\"note\":\"Spannung und Strom werden nur live im RAM gehalten und nicht historisch gespeichert.\"}";
  return json;
}

void handleRoot() {
  if (!requireAdmin()) return;
  String body = F("<div class='grid'>"
    "<div class='card'><div class='muted'>Aktuelle Leistung</div><div class='value' id='powerValue'>–</div></div>"
    "<div class='card'><div class='muted'>Netzbezug</div><div class='value' id='importValue'>–</div></div>"
    "<div class='card'><div class='muted'>Einspeisung</div><div class='value' id='exportValue'>–</div></div>"
    "<div class='card'><div class='muted'>Zählerstatus</div><div class='value' id='meterState'>–</div></div></div>"
    "<div id='phaseSection'><h2>Phasenwerte live</h2><div class='grid'>"
    "<div class='card'><strong>L1</strong><div id='phase0'>–</div></div>"
    "<div class='card'><strong>L2</strong><div id='phase1'>–</div></div>"
    "<div class='card'><strong>L3</strong><div id='phase2'>–</div></div></div>"
    "<p class='muted'>Es werden ausschließlich tatsächlich vom Zähler übertragene Werte angezeigt.</p></div>"
    "<div class='section-head'><div><h2>Energieübersicht</h2>"
    "<div class='muted'>Tag, Vergleich und laufendes Kalenderjahr</div></div></div>"
    "<div class='grid'>"
    "<div class='card'><div class='muted'>Netzbezug heute</div><div class='value' id='todayImportSummary'>–</div></div>"
    "<div class='card'><div class='muted'>Einspeisung heute</div><div class='value' id='todayExportSummary'>–</div></div>"
    "<div class='card'><div class='muted'>Bezug zu gestern, gleiche Uhrzeit</div><div class='value' id='dayComparison'>–</div></div>"
    "<div class='card'><div class='muted'>Netzbezug im Jahr</div><div class='value' id='yearImport'>–</div></div>"
    "<div class='card'><div class='muted'>Einspeisung im Jahr</div><div class='value' id='yearExport'>–</div></div>"
    "<div class='card'><div class='muted'>Ø Netzbezug pro Tag im Jahr</div><div class='value' id='yearDailyImport'>–</div></div>"
    "<div class='card'><div class='muted'>Ø Einspeisung pro Tag im Jahr</div><div class='value' id='yearDailyExport'>–</div></div>"
    "</div><p class='muted' id='yearCoverage'>Jahreswerte werden aus der verfügbaren lokalen Historie berechnet.</p>");
  body += F(R"HTML(
    <div class='section-head'>
      <div><h2>Verlauf</h2><div class='muted'>Verbrauch und Einspeisung direkt auf dem Tracker</div></div>
      <a href='/history'>Erweiterte Auswertung öffnen</a>
    </div>
    <div class='card chart-card'>
      <div class='chart-controls'>
        <div><label for='dashRange'>Zeitraum</label><select id='dashRange'>
          <option value='hour'>1 Stunde</option><option value='day' selected>Kalendertag (00:00–24:00)</option>
          <option value='week'>Kalenderwoche (Mo–So)</option><option value='month'>Kalendermonat</option>
          <option value='year'>Kalenderjahr</option>
        </select></div><select id='dashSeries' hidden><option value='energy' selected>Bezug und Einspeisung</option></select>
      </div>
      <div id='dashDateNav' class='date-nav'>
        <label><span id='dashAnchorLabel'>Zeitpunkt</span><input id='dashDate' type='datetime-local' step='3600'></label>
        <button id='dashPrev' type='button' class='secondary'>← Vorheriger Zeitraum</button>
        <button id='dashToday' type='button'>Aktueller Zeitraum</button>
        <button id='dashNext' type='button' class='secondary'>Nächster Zeitraum →</button>
        <label class='date-slider'><span id='dashSliderLabel'>Schnell zurückspulen</span>
          <input id='dashDaysBack' type='range' min='0' max='730' value='0'>
          <output id='dashDaysLabel'>Aktuell</output>
        </label>
      </div>
      <div class='stats'>
        <div class='stat'><span id='averageLabel' class='muted'>Ø Leistung im Zeitraum</span><strong id='morningAverage'>–</strong><div id='averageYearCompare' class='metric-comparison'>–</div><small id='averageYearBaseline' class='metric-baseline'>Jahres-Ø: –</small></div>
        <div class='stat'><span id='periodImportLabel' class='muted'>Netzbezug im Zeitraum</span><strong id='periodImport'>–</strong><div id='importYearCompare' class='metric-comparison'>–</div><small id='importYearBaseline' class='metric-baseline'>Jahres-Ø pro Tag: –</small></div>
        <div class='stat'><span id='periodExportLabel' class='muted'>Einspeisung im Zeitraum</span><strong id='periodExport'>–</strong><div id='exportYearCompare' class='metric-comparison'>–</div><small id='exportYearBaseline' class='metric-baseline'>Jahres-Ø pro Tag: –</small></div>
      </div>
      <div id='dashLoading' class='loading'><span class='spinner'></span>Diagramm wird geladen …</div>
      <div id='dashEmpty' class='empty-state' hidden>Noch keine historischen Werte vorhanden.</div>
      <section class='chart-section'>
        <h2>Leistung</h2>
        <div id='powerChartWrap' class='dashboard-chart' hidden>
          <canvas id='powerChart' aria-label='Leistungsverlauf'></canvas>
          <div id='powerTip' class='tooltip'></div>
        </div>
        <div class='legend-row'><span class='legend-item'><i class='swatch' style='background:#63e68b'></i>Gesamtleistung</span>
        <span class='legend-item'><i class='swatch' style='background:var(--chart-gap)'></i>Datenlücke / Ausfall</span></div>
      </section>
      <section class='chart-section'>
        <h2>Netzbezug und Einspeisung</h2>
        <div id='dashChartWrap' class='dashboard-chart' hidden>
          <canvas id='dashChart' aria-label='Messwertverlauf'></canvas>
          <div id='dashTip' class='tooltip'></div>
        </div>
        <div id='dashLegend' class='legend-row'></div>
      </section>
      <p id='dashSummary' class='chart-note muted'></p>
      <p class='chart-note muted'>Messbalken: Maus oder Finger bewegen.
      Doppelklick oder Doppeltippen fixiert; ein einfacher Klick oder Tipp
      löst ihn wieder.</p>
    </div>
    <p style='margin-top:28px'><span class='status-pill'><i class='dot'></i> Lokal · ohne Cloud</span></p>)HTML");

  if (!sendPageStreamed("Dashboard", body, "/assets/dashboard.js?v=" + String(kFirmwareVersion))) {
    eventLog.add("ERROR", "DASHBOARD_PAGE_INCOMPLETE",
                 "Dashboard-Seite konnte nicht vollstaendig erzeugt werden");
    server.send(503, "text/plain; charset=utf-8",
                "Dashboard voruebergehend nicht verfuegbar. Bitte neu laden.");
  }
}

void handleHistoryPage() {
  if (!requireAdmin()) return;
  const String body = F(R"HTML(
    <div class='card'>
      <div class='toolbar'>
        <div><label>Zeitraum</label><select id='range'>
          <option value='hour'>1 Stunde</option><option value='day' selected>Kalendertag (00:00–24:00)</option>
          <option value='week'>Kalenderwoche (Mo–So)</option><option value='month'>Kalendermonat</option>
          <option value='year'>Kalenderjahr</option><option value='all'>Langzeit</option>
        </select></div>
        <div><label>Messwert</label><select id='series'>
          <option value='power'>Leistung</option><option value='combined'>Bezug und Einspeisung</option><option value='import'>Netzbezug</option>
          <option value='export'>Einspeisung</option>
        </select></div>
        <div id='modeBox'><label>Leistungsanzeige</label><select id='metric'>
          <option value='avg'>Durchschnitt</option><option value='minmax'>Durchschnitt mit Min/Max</option>
        </select></div>
        <button id='zoomIn' type='button' title='Hineinzoomen'>Zoom +</button>
        <button id='zoomOut' type='button' title='Herauszoomen'>Zoom −</button>
        <button id='reset' type='button'>Gesamt</button>
      </div>
      <div id='historyDateNav' class='date-nav' hidden>
        <label><span id='historyAnchorLabel'>Zeitpunkt</span><input id='historyDate' type='datetime-local' step='3600'></label>
        <button id='historyPrev' type='button' class='secondary'>← Vorheriger Zeitraum</button>
        <button id='historyToday' type='button'>Aktueller Zeitraum</button>
        <button id='historyNext' type='button' class='secondary'>Nächster Zeitraum →</button>
        <label class='date-slider'><span id='historySliderLabel'>Schnell zurückspulen</span>
          <input id='historyDaysBack' type='range' min='0' max='730' value='0'>
          <output id='historyDaysLabel'>Aktuell</output>
        </label>
      </div>
      <div class='stats'>
        <div class='stat'><span id='historyAverageLabel' class='muted'>Ø Leistung im Zeitraum</span><strong id='historyAverage'>–</strong></div>
        <div class='stat'><span id='historyImportLabel' class='muted'>Netzbezug im Zeitraum</span><strong id='todayImport'>–</strong></div>
        <div class='stat'><span id='historyExportLabel' class='muted'>Einspeisung im Zeitraum</span><strong id='todayExport'>–</strong></div>
      </div>
      <div id='legend' class='legend-row'></div>
      <div id='loading' class='loading'><span class='spinner'></span><span>Historie wird geladen …</span></div>
      <div id='error' class='error' hidden></div>
      <div id='chartWrap' class='chart-wrap' hidden>
        <canvas id='chart' aria-label='Historisches Messwertdiagramm'></canvas><div id='tooltip' class='tooltip'></div>
      </div>
      <p id='summary' class='muted'></p>
      <p class='muted'>Maus oder Finger: Messbalken verschieben. Doppelklick oder Doppeltippen fixiert ihn. Einfaches Klicken oder Tippen löst ihn wieder. Mausrad: zoomen. Umschalt+Ziehen: Zeitraum verschieben.</p>
      <p><a id='csv' href='/api/v1/history.csv?range=complete'>Vollständige Historie als CSV exportieren</a></p>
    </div>)HTML");

  if (!sendPageStreamed("Lokale Historie", body,
                        "/assets/history.js?v=" + String(kFirmwareVersion))) {
    eventLog.add("ERROR", "HISTORY_PAGE_INCOMPLETE",
                 "Historienseite konnte nicht vollstaendig erzeugt werden");
    server.send(503, "text/plain; charset=utf-8",
                "Historie voruebergehend nicht verfuegbar. Bitte neu laden.");
  }
}

struct HistoryQuery {
  HistoryStore::Tier tier;
  uint32_t since;
  uint32_t until;
};

uint32_t historyTierSeconds(HistoryStore::Tier tier) {
  switch (tier) {
    case HistoryStore::Tier::Minute: return 60;
    case HistoryStore::Tier::QuarterHour: return 900;
    case HistoryStore::Tier::Hour: return 3600;
    case HistoryStore::Tier::Day: return 86400;
  }
  return 60;
}

uint32_t requestedHistoryAnchor(uint32_t now) {
  const String value = server.arg("anchor");
  if (!value.length()) return now;
  if (value.length() != 10) return now;
  for (char c : value)
    if (!isDigit(c)) return now;
  const uint64_t parsed = strtoull(value.c_str(), nullptr, 10);
  return parsed >= 1577836800ULL && parsed <= now
             ? static_cast<uint32_t>(parsed)
             : now;
}

HistoryQuery calendarHistoryQuery(const String &range, uint32_t now) {
  time_t anchor = static_cast<time_t>(requestedHistoryAnchor(now));
  struct tm start {};
  localtime_r(&anchor, &start);
  start.tm_sec = 0;
  if (range == "hour") {
    start.tm_min = 0;
  } else {
    start.tm_hour = 0;
    start.tm_min = 0;
    if (range == "week")
      start.tm_mday -= (start.tm_wday + 6) % 7;  // DE: Montag | EN: Monday
    else if (range == "month")
      start.tm_mday = 1;
    else if (range == "year") {
      start.tm_mon = 0;
      start.tm_mday = 1;
    }
  }
  start.tm_isdst = -1;
  const time_t since = mktime(&start);
  struct tm end = start;
  if (range == "hour")
    end.tm_hour += 1;
  else if (range == "week")
    end.tm_mday += 7;
  else if (range == "month")
    end.tm_mon += 1;
  else if (range == "year")
    end.tm_year += 1;
  else
    end.tm_mday += 1;
  end.tm_isdst = -1;
  const time_t until = mktime(&end);
  const uint32_t age = now > static_cast<uint32_t>(until)
                           ? now - static_cast<uint32_t>(until)
                           : 0;
  HistoryStore::Tier tier;
  if (range == "year") {
    tier = HistoryStore::Tier::Day;
  } else if (range == "week" || range == "month") {
    tier = age < 179UL * 86400UL
               ? HistoryStore::Tier::QuarterHour
               : age < 729UL * 86400UL ? HistoryStore::Tier::Hour
                                       : HistoryStore::Tier::Day;
  } else {
    tier = age < 47UL * 3600UL
               ? HistoryStore::Tier::Minute
               : age < 179UL * 86400UL
                     ? HistoryStore::Tier::QuarterHour
                     : age < 729UL * 86400UL ? HistoryStore::Tier::Hour
                                             : HistoryStore::Tier::Day;
  }
  return {tier, static_cast<uint32_t>(since), static_cast<uint32_t>(until)};
}

HistoryQuery historyQuery() {
  const uint32_t now = time(nullptr);
  const String range = server.arg("range");
  if (range == "minute_all")
    return {HistoryStore::Tier::Minute, 0, now};
  if (range == "quarter_all")
    return {HistoryStore::Tier::QuarterHour, 0, now};
  if (range == "hour_all")
    return {HistoryStore::Tier::Hour, 0, now};
  if (range == "day_all")
    return {HistoryStore::Tier::Day, 0, now};
  if (range == "compare")
    return {HistoryStore::Tier::QuarterHour, now - 3 * 86400, now};
  if (range == "hour" || range == "day" || range == "week" ||
      range == "month" || range == "year")
    return calendarHistoryQuery(range, now);
  return {HistoryStore::Tier::Day, 0, now};
}

struct EnergyDelta {
  double importKwh = NAN;
  double exportKwh = NAN;
  uint32_t firstTimestamp = 0;
};

EnergyDelta storedEnergyDelta(HistoryStore::Tier tier, uint32_t since,
                              uint32_t until, double finalImport = NAN,
                              double finalExport = NAN) {
  double firstImport = NAN;
  double firstExport = NAN;
  double lastImport = NAN;
  double lastExport = NAN;
  EnergyDelta result;
  history.forEach(tier, since, until,
                  [&](const HistoryStore::Record &record) {
                    if (!result.firstTimestamp &&
                        (std::isfinite(record.importKwh) ||
                         std::isfinite(record.exportKwh)))
                      result.firstTimestamp = record.timestamp;
                    if (std::isfinite(record.importKwh)) {
                      if (!std::isfinite(firstImport))
                        firstImport = record.importKwh;
                      lastImport = record.importKwh;
                    }
                    if (std::isfinite(record.exportKwh)) {
                      if (!std::isfinite(firstExport))
                        firstExport = record.exportKwh;
                      lastExport = record.exportKwh;
                    }
                    return true;
                  });
  if (std::isfinite(finalImport)) lastImport = finalImport;
  if (std::isfinite(finalExport)) lastExport = finalExport;
  if (std::isfinite(firstImport) && std::isfinite(lastImport))
    result.importKwh = std::max(0.0, lastImport - firstImport);
  if (std::isfinite(firstExport) && std::isfinite(lastExport))
    result.exportKwh = std::max(0.0, lastExport - firstExport);
  return result;
}

void handleDashboardSummary() {
  if (!requireAdmin()) return;
  const time_t now = time(nullptr);
  if (now < 1700000000) {
    server.send(503, "application/json", "{\"error\":\"time_not_valid\"}");
    return;
  }
  struct tm localNow {};
  localtime_r(&now, &localNow);
  struct tm todayTm = localNow;
  todayTm.tm_hour = 0;
  todayTm.tm_min = 0;
  todayTm.tm_sec = 0;
  todayTm.tm_isdst = -1;
  const time_t todayStart = mktime(&todayTm);
  struct tm yesterdayTm = todayTm;
  yesterdayTm.tm_mday -= 1;
  yesterdayTm.tm_isdst = -1;
  const time_t yesterdayStart = mktime(&yesterdayTm);
  struct tm yesterdayCutoffTm = localNow;
  yesterdayCutoffTm.tm_mday -= 1;
  yesterdayCutoffTm.tm_isdst = -1;
  const time_t yesterdayCutoff = mktime(&yesterdayCutoffTm);
  struct tm yearTm = localNow;
  yearTm.tm_mon = 0;
  yearTm.tm_mday = 1;
  yearTm.tm_hour = 0;
  yearTm.tm_min = 0;
  yearTm.tm_sec = 0;
  yearTm.tm_isdst = -1;
  const time_t yearStart = mktime(&yearTm);
  const uint32_t todayBaseline =
      todayStart > 120 ? static_cast<uint32_t>(todayStart - 120) : 0;
  const uint32_t yesterdayBaseline =
      yesterdayStart > 120 ? static_cast<uint32_t>(yesterdayStart - 120) : 0;
  const uint32_t yearBaseline =
      yearStart > 2 * 86400 ? static_cast<uint32_t>(yearStart - 2 * 86400) : 0;
  const EnergyDelta today = storedEnergyDelta(
      HistoryStore::Tier::Minute, todayBaseline, static_cast<uint32_t>(now),
      meter.importKwh, meter.exportKwh);
  const EnergyDelta yesterday = storedEnergyDelta(
      HistoryStore::Tier::Minute, yesterdayBaseline,
      static_cast<uint32_t>(yesterdayCutoff));
  const EnergyDelta year = storedEnergyDelta(
      HistoryStore::Tier::Day, yearBaseline, static_cast<uint32_t>(now),
      meter.importKwh, meter.exportKwh);
  const uint32_t coverageStart =
      year.firstTimestamp
          ? std::max(static_cast<uint32_t>(yearStart), year.firstTimestamp)
          : 0;
  const double coverageDays =
      coverageStart && now > coverageStart
          ? std::max(1.0, (now - coverageStart) / 86400.0)
          : NAN;
  const double yearAveragePower =
      std::isfinite(coverageDays) && std::isfinite(year.importKwh) &&
              std::isfinite(year.exportKwh)
          ? (year.importKwh - year.exportKwh) * 1000.0 /
                (coverageDays * 24.0)
          : NAN;
  double changePercent = NAN;
  if (std::isfinite(today.importKwh) &&
      std::isfinite(yesterday.importKwh) && yesterday.importKwh > 0.000001)
    changePercent =
        (today.importKwh - yesterday.importKwh) / yesterday.importKwh * 100.0;
  String json;
  json.reserve(420);
  json = "{\"today_import_kwh\":" + numberOrNull(today.importKwh, 4) +
         ",\"today_export_kwh\":" + numberOrNull(today.exportKwh, 4) +
         ",\"yesterday_same_time_import_kwh\":" +
         numberOrNull(yesterday.importKwh, 4) +
         ",\"import_change_percent\":" +
         numberOrNull(changePercent, 1) +
         ",\"year_import_kwh\":" + numberOrNull(year.importKwh, 3) +
         ",\"year_export_kwh\":" + numberOrNull(year.exportKwh, 3) +
         ",\"year_average_power_w\":" +
         numberOrNull(yearAveragePower, 1) +
         ",\"year_daily_average_import_kwh\":" +
         numberOrNull(std::isfinite(year.importKwh) &&
                              std::isfinite(coverageDays)
                          ? year.importKwh / coverageDays
                          : NAN,
                      3) +
         ",\"year_daily_average_export_kwh\":" +
         numberOrNull(std::isfinite(year.exportKwh) &&
                              std::isfinite(coverageDays)
                          ? year.exportKwh / coverageDays
                          : NAN,
                      3) +
         ",\"year_coverage_days\":" + numberOrNull(coverageDays, 2) + "}";
  server.send(200, "application/json", json);
}

void handleHistoryJson() {
  if (!requireAdmin()) return;
  WiFiClient responseClient = server.client();
  const uint32_t now = time(nullptr);
  const String range = server.arg("range");
  const HistoryQuery query = historyQuery();
  const bool currentLiveHour =
      range == "hour" && query.since <= now && query.until >= now;
  if (currentLiveHour || range == "live") {
    const uint32_t since =
        range == "live" ? (now > 3600 ? now - 3600 : 0) : query.since;
    const uint32_t until = range == "live" ? now : query.until;
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    server.sendContent("{\"from\":" + String(since) + ",\"to\":" +
                       String(until) + ",\"step\":5,\"values\":[");
    String chunk;
    chunk.reserve(1000);
    const size_t first =
        liveCount < kLiveSamples ? 0 : liveWriteIndex;
    bool firstValue = true;
    for (size_t i = 0; i < liveCount; ++i) {
      if (!responseClient.connected()) break;
      const LiveSample &sample = liveSamples[(first + i) % kLiveSamples];
      if (sample.timestamp < since || sample.timestamp > now) continue;
      if (!firstValue) chunk += ',';
      firstValue = false;
      chunk += "{\"ts\":" + String(sample.timestamp) +
               ",\"avg\":" + numberOrNull(sample.powerW, 2) +
               ",\"min\":" + numberOrNull(sample.powerW, 2) +
               ",\"max\":" + numberOrNull(sample.powerW, 2) +
               ",\"import\":" + numberOrNull(sample.importKwh, 4) +
               ",\"export\":" + numberOrNull(sample.exportKwh, 4) + "}";
      if (chunk.length() > 900) {
        server.sendContent(chunk);
        chunk = "";
        if (!responseClient.connected()) break;
      }
    }
    if (responseClient.connected() && chunk.length()) server.sendContent(chunk);
    if (responseClient.connected()) server.sendContent("]}");
    return;
  }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"from\":" + String(query.since) + ",\"to\":" +
                     String(query.until) + ",\"step\":" +
                     String(historyTierSeconds(query.tier)) + ",\"values\":[");
  bool first = true;
  String chunk;
  chunk.reserve(1200);
  history.forEach(query.tier, query.since, query.until,
                   [&](const HistoryStore::Record &record) {
                     if (!responseClient.connected()) return false;
                     if (!first) chunk += ',';
                    first = false;
                    chunk += "{\"ts\":" + String(record.timestamp) +
                             ",\"avg\":" + String(record.averageW, 2) +
                             ",\"min\":" + String(record.minimumW, 2) +
                             ",\"max\":" + String(record.maximumW, 2) +
                             ",\"import\":" + numberOrNull(record.importKwh, 4) +
                             ",\"export\":" + numberOrNull(record.exportKwh, 4) + "}";
                     if (chunk.length() > 900) {
                       server.sendContent(chunk);
                       chunk = "";
                       if (!responseClient.connected()) return false;
                     }
                     return true;
                   });
  if (responseClient.connected() && chunk.length()) server.sendContent(chunk);
  if (responseClient.connected()) server.sendContent("]}");
}

void handleHistoryCsv() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_export");
  const uint32_t now = time(nullptr);
  const String range = server.arg("range");
  if (range == "complete") {
    server.sendHeader(
        "Content-Disposition",
        "attachment; filename=irtracker-complete-history.csv");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/csv; charset=utf-8", "");
    server.sendContent(
        "timestamp,average_w,minimum_w,maximum_w,import_kwh,export_kwh,"
        "resolution_seconds\n");
    String chunk;
    chunk.reserve(1200);
    const auto appendRecord =
        [&](const HistoryStore::Record &record, uint32_t resolution) {
          chunk += String(record.timestamp) + "," +
                   String(record.averageW, 2) + "," +
                   String(record.minimumW, 2) + "," +
                   String(record.maximumW, 2) + "," +
                   numberOrNull(record.importKwh, 4) + "," +
                   numberOrNull(record.exportKwh, 4) + "," +
                   String(resolution) + "\n";
          if (chunk.length() > 900) {
            server.sendContent(chunk);
            chunk = "";
          }
          return true;
        };
    const uint32_t minuteSince =
        now > 48UL * 3600UL ? now - 48UL * 3600UL : 0;
    const uint32_t quarterSince =
        now > 180UL * 86400UL ? now - 180UL * 86400UL : 0;
    const uint32_t hourSince =
        now > 730UL * 86400UL ? now - 730UL * 86400UL : 0;
    if (hourSince)
      history.forEach(HistoryStore::Tier::Day, 0, hourSince - 1,
                      [&](const HistoryStore::Record &record) {
                        return appendRecord(record, 86400);
                      });
    if (quarterSince > hourSince)
      history.forEach(HistoryStore::Tier::Hour, hourSince, quarterSince - 1,
                      [&](const HistoryStore::Record &record) {
                        return appendRecord(record, 3600);
                      });
    if (minuteSince > quarterSince)
      history.forEach(
          HistoryStore::Tier::QuarterHour, quarterSince, minuteSince - 1,
          [&](const HistoryStore::Record &record) {
            return appendRecord(record, 900);
          });
    history.forEach(HistoryStore::Tier::Minute, minuteSince, now,
                    [&](const HistoryStore::Record &record) {
                      return appendRecord(record, 60);
                    });
    const uint32_t currentMinute = now - now % 60;
    const size_t first = liveCount < kLiveSamples ? 0 : liveWriteIndex;
    for (size_t i = 0; i < liveCount; ++i) {
      const LiveSample &sample = liveSamples[(first + i) % kLiveSamples];
      if (sample.timestamp < currentMinute || sample.timestamp > now) continue;
      chunk += String(sample.timestamp) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.importKwh, 4) + "," +
               numberOrNull(sample.exportKwh, 4) + ",5\n";
      if (chunk.length() > 900) {
        server.sendContent(chunk);
        chunk = "";
      }
    }
    if (chunk.length()) server.sendContent(chunk);
    return;
  }
  const HistoryQuery query = historyQuery();
  const bool currentLiveHour =
      range == "hour" && query.since <= now && query.until >= now;
  if (currentLiveHour || range == "live") {
    const uint32_t since =
        range == "live" ? (now > 3600 ? now - 3600 : 0) : query.since;
    server.sendHeader("Content-Disposition",
                      "attachment; filename=irtracker-1hour.csv");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/csv; charset=utf-8", "");
    server.sendContent(
        "timestamp,average_w,minimum_w,maximum_w,import_kwh,export_kwh\n");
    String chunk;
    const size_t first =
        liveCount < kLiveSamples ? 0 : liveWriteIndex;
    for (size_t i = 0; i < liveCount; ++i) {
      const LiveSample &sample = liveSamples[(first + i) % kLiveSamples];
      if (sample.timestamp < since || sample.timestamp > now) continue;
      chunk += String(sample.timestamp) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.importKwh, 4) + "," +
               numberOrNull(sample.exportKwh, 4) + "\n";
      if (chunk.length() > 900) {
        server.sendContent(chunk);
        chunk = "";
      }
    }
    if (chunk.length()) server.sendContent(chunk);
    return;
  }
  server.sendHeader("Content-Disposition",
                    "attachment; filename=irtracker-history.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv; charset=utf-8", "");
  server.sendContent(
      "timestamp,average_w,minimum_w,maximum_w,import_kwh,export_kwh\n");
  String chunk;
  chunk.reserve(1200);
  history.forEach(query.tier, query.since, query.until,
                  [&](const HistoryStore::Record &record) {
                    chunk += String(record.timestamp) + "," +
                             String(record.averageW, 2) + "," +
                             String(record.minimumW, 2) + "," +
                             String(record.maximumW, 2) + "," +
                             String(record.importKwh, 4) + "," +
                             String(record.exportKwh, 4) + "\n";
                    if (chunk.length() > 900) {
                      server.sendContent(chunk);
                      chunk = "";
                    }
                    return true;
                  });
  if (chunk.length()) server.sendContent(chunk);
}

void handleSetTime() {
  if (!requireAdmin()) return;
  const time_t epoch = server.arg("epoch").toInt();
  if (epoch < 1700000000) {
    server.send(400, "application/json", "{\"error\":\"invalid_time\"}");
    return;
  }
  timeval tv = {epoch, 0};
  settimeofday(&tv, nullptr);
  server.send(200, "application/json", "{\"ok\":true}");
}

const char *driverName(EnergyManager::Driver driver) {
  switch (driver) {
    case EnergyManager::Driver::ModbusTcp:
      return "Modbus TCP / SunSpec";
    case EnergyManager::Driver::Mqtt:
      return "MQTT";
    case EnergyManager::Driver::Http:
      return "HTTP / REST";
    default:
      return "Deaktiviert";
  }
}

void handleInterfacesPage() {
  if (!requireAdmin()) return;
  const String host =
      accessPointMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  String body = F(
      "<div class='card'><span class='status-pill'><i class='dot'></i>Nur lesende Messwertausgabe</span>"
      "<h2>Der Tracker sendet keine Sollwerte</h2>"
      "<p>Null-Einspeisung, Ladegrenzen und Zeitpläne werden ausschließlich im Speicher oder Wechselrichter eingestellt. "
      "Der IR-Tracker stellt dafür nur die gemessene Netzleistung bereit.</p></div>"
      "<div class='grid'><div class='card'><h2>Shelly-kompatibel</h2>"
      "<p>Für Speicher, die einen Shelly EM oder Shelly Pro EM als externen Zähler unterstützen.</p><code>http://");
  body += host;
  body += F("/status</code><br><code>/emeter/0</code><br><code>/rpc/EM.GetStatus?id=0</code></div>"
            "<div class='card'><h2>Home Assistant / MQTT</h2>"
            "<p>MQTT und automatische Home-Assistant-Erkennung werden unter Einstellungen konfiguriert.</p>"
            "<a href='/setup'>MQTT konfigurieren</a></div>"
            "<div class='card'><h2>Monitoring und Export</h2>"
            "<code>/metrics</code><br><code>/openmetrics</code><br>"
            "<code>/api/v1/influx</code><br><code>/api/v1/values.csv</code></div></div>"
            "<div class='card'><h2>Sicherheitsprinzip</h2>"
            "<p>Alle hier aufgeführten Schnittstellen geben Messwerte aus. Es werden keine Register am Speicher beschrieben "
            "und keine Lade- oder Entladebefehle verschickt.</p></div>");
  server.send(200, "text/html; charset=utf-8",
              page("Schnittstellen", body));
}

void handleEnergyPage() {
  if (!requireAdmin()) return;
  const EnergyManager::Status &status = energyManager.status();
  String body = F(
      "<div class='card'><p><strong>Status:</strong> ");
  body += htmlEscape(status.message);
  body += " | letzter Sollwert: " + String(status.sentW) +
          " W | Fehler: " + String(status.failures) + F("</p>"
      "<p class='muted'>Treiber sind standardmäßig deaktiviert und beginnen im Trockenlauf. "
      "Vor echter Freigabe Herstellerdokumentation, Vorzeichen und Leistungsgrenzen prüfen.</p></div>"
      "<form method='post' action='/energy/save'><fieldset><legend>Betriebsart</legend>"
      "<label>Schnittstelle</label><select name='driver'>");
  for (uint8_t value = 0; value <= 3; ++value) {
    const auto driver = static_cast<EnergyManager::Driver>(value);
    body += "<option value='" + String(value) + "'" +
            (driver == energyConfig.driver ? " selected" : "") + ">" +
            driverName(driver) + "</option>";
  }
  body += F("</select><label><input style='width:auto' type='checkbox' name='enabled' value='1'");
  if (energyConfig.enabled) body += " checked";
  body += F("> Regler aktivieren</label>"
            "<label><input style='width:auto' type='checkbox' name='dry_run' value='1'");
  if (energyConfig.dryRun) body += " checked";
  body += F("> Trockenlauf: berechnen, aber nichts an den Speicher senden</label>"
            "<label><input style='width:auto' type='checkbox' name='invert' value='1'");
  if (energyConfig.inverted) body += " checked";
  body += F("> Sollwert-Vorzeichen invertieren</label>"
            "<label>LIVE-Bestätigung für echte Ausgabe</label>"
            "<input name='live_confirm' autocomplete='off' placeholder='LIVE nur zur bewussten Freigabe'>"
            "</fieldset><fieldset><legend>Netzwerk und Geräteprofil</legend>"
            "<div class='inline'><div><label>Host/IP</label><input name='em_host' value='");
  body += htmlEscape(energyConfig.host);
  body += F("' placeholder='192.168.178.50'></div><div><label>Port</label>"
            "<input type='number' name='em_port' min='1' max='65535' value='");
  body += String(energyConfig.port);
  body += F("'></div></div><div class='inline'><div><label>Modbus Unit-ID</label>"
            "<input type='number' name='em_unit' min='1' max='247' value='");
  body += String(energyConfig.unitId);
  body += F("'></div><div><label>Leistungsregister</label>"
            "<input type='number' name='em_reg' min='0' max='65535' value='");
  body += String(energyConfig.powerRegister);
  body += F("'></div></div><div class='inline'><div><label>Registerbreite</label>"
            "<select name='em_width'><option value='1'");
  if (energyConfig.registerWidth == 1) body += " selected";
  body += F(">16 Bit</option><option value='2'");
  if (energyConfig.registerWidth == 2) body += " selected";
  body += F(">32 Bit</option></select></div><div><label>Register-Skalierung W/Einheit</label>"
            "<input type='number' step='0.0001' name='em_scale' value='");
  body += String(energyConfig.registerScale, 4);
  body += F("'></div></div><label><input style='width:auto' type='checkbox' name='em_swap' value='1'");
  if (energyConfig.wordSwap) body += " checked";
  body += F("> 32-Bit-Wortreihenfolge tauschen</label>"
            "<label>MQTT-Sollwertthema</label><input name='em_topic' value='");
  body += htmlEscape(energyConfig.mqttTopic);
  body += F("' placeholder='opendtu/power/set'>"
            "<label>MQTT-Nutzlast ({power} wird ersetzt)</label>"
            "<input name='em_mqpay' value='");
  body += htmlEscape(energyConfig.mqttPayload);
  body += F("' placeholder='{power}'><label>HTTP-Pfad</label>"
            "<input name='em_path' value='");
  body += htmlEscape(energyConfig.httpPath);
  body += F("' placeholder='/api/power'><label>HTTP-Methode</label>"
            "<select name='em_method'><option value='POST'");
  if (energyConfig.httpMethod != "PUT") body += " selected";
  body += F(">POST</option><option value='PUT'");
  if (energyConfig.httpMethod == "PUT") body += " selected";
  body += F(">PUT</option></select><label>HTTP-JSON-Schablone ({power}, {grid})</label>"
            "<input name='em_httppay' value='");
  body += htmlEscape(energyConfig.httpPayload);
  body += F("'><label>HTTP Bearer-Token</label><input type='password' name='em_token' "
            "placeholder='");
  body += energyConfig.httpBearerToken.length() ? "gespeichert" : "optional";
  body += F("' autocomplete='new-password'>"
            "<label><input style='width:auto' type='checkbox' name='em_token_clear' value='1'>"
            " Gespeichertes Bearer-Token löschen</label></fieldset>"
            "<fieldset><legend>Sicherheitsgrenzen</legend>"
            "<div class='inline'><div><label>Ziel-Netzleistung (W)</label>"
            "<input type='number' name='em_target' min='-500' max='500' value='");
  body += String(energyConfig.targetGridW);
  body += F("'></div><div><label>Totband (W)</label>"
            "<input type='number' name='em_dead' min='0' max='500' value='");
  body += String(energyConfig.deadbandW);
  body += F("'></div></div><div class='inline'><div><label>Max. Laden (W)</label>"
            "<input type='number' name='em_charge' min='0' max='10000' value='");
  body += String(energyConfig.maxChargeW);
  body += F("'></div><div><label>Max. Entladen (W)</label>"
            "<input type='number' name='em_discharge' min='0' max='10000' value='");
  body += String(energyConfig.maxDischargeW);
  body += F("'></div></div><div class='inline'><div><label>Rampe (W/s)</label>"
            "<input type='number' name='em_ramp' min='10' max='5000' value='");
  body += String(energyConfig.rampWPerSecond);
  body += F("'></div><div><label>Regelintervall (ms)</label>"
            "<input type='number' name='em_interval' min='1000' max='30000' value='");
  body += String(energyConfig.intervalMs);
  body += F("'></div></div><label>Messwert-Timeout (ms)</label>"
            "<input type='number' name='em_stale' min='3000' max='60000' value='");
  body += String(energyConfig.staleMs);
  body += F("'></fieldset><button type='submit'>Konfiguration prüfen und speichern</button></form>"
            "<form method='post' action='/energy/stop'><button class='danger' type='submit'>"
            "Sofort Null-Sollwert senden und Regler stoppen</button></form>");
  server.send(200, "text/html; charset=utf-8",
              page("Lokale Nulleinspeisung", body));
}

void handleEnergySave() {
  if (!requireAdmin()) return;
  EnergyManager::Config next = energyConfig;
  next.driver = static_cast<EnergyManager::Driver>(
      constrain(server.arg("driver").toInt(), 0, 3));
  next.enabled = server.hasArg("enabled");
  next.dryRun = server.hasArg("dry_run");
  next.inverted = server.hasArg("invert");
  next.host = server.arg("em_host");
  next.host.trim();
  next.port = constrain(server.arg("em_port").toInt(), 1, 65535);
  next.unitId = constrain(server.arg("em_unit").toInt(), 1, 247);
  next.powerRegister = constrain(server.arg("em_reg").toInt(), 0, 65535);
  next.registerWidth = server.arg("em_width").toInt() == 2 ? 2 : 1;
  next.wordSwap = server.hasArg("em_swap");
  next.registerScale = server.arg("em_scale").toFloat();
  next.mqttTopic = server.arg("em_topic");
  next.mqttTopic.trim();
  next.mqttPayload = server.arg("em_mqpay");
  if (!next.mqttPayload.length()) next.mqttPayload = "{power}";
  next.httpPath = server.arg("em_path");
  next.httpPath.trim();
  next.httpMethod = server.arg("em_method") == "PUT" ? "PUT" : "POST";
  next.httpPayload = server.arg("em_httppay");
  if (!next.httpPayload.length())
    next.httpPayload = "{\"setpoint_w\":{power},\"grid_w\":{grid}}";
  const String newBearerToken = server.arg("em_token");
  if (server.hasArg("em_token_clear"))
    next.httpBearerToken = "";
  else if (newBearerToken.length())
    next.httpBearerToken = newBearerToken;
  next.targetGridW = constrain(server.arg("em_target").toInt(), -500, 500);
  next.deadbandW = constrain(server.arg("em_dead").toInt(), 0, 500);
  next.maxChargeW = constrain(server.arg("em_charge").toInt(), 0, 10000);
  next.maxDischargeW =
      constrain(server.arg("em_discharge").toInt(), 0, 10000);
  next.rampWPerSecond = constrain(server.arg("em_ramp").toInt(), 10, 5000);
  next.intervalMs =
      constrain(server.arg("em_interval").toInt(), 1000, 30000);
  next.staleMs = constrain(server.arg("em_stale").toInt(), 3000, 60000);
  if (!std::isfinite(next.registerScale) || next.registerScale == 0 ||
      (next.driver == EnergyManager::Driver::ModbusTcp &&
       !next.host.length()) ||
      (next.driver == EnergyManager::Driver::Mqtt &&
       !next.mqttTopic.length()) ||
      (next.driver == EnergyManager::Driver::Http &&
       (!next.host.length() || !next.httpPath.startsWith("/")))) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_energy_configuration\"}");
    return;
  }
  if (next.enabled && !next.dryRun &&
      server.arg("live_confirm") != "LIVE") {
    server.send(400, "application/json",
                "{\"error\":\"live_confirmation_required\"}");
    return;
  }
  if (energyConfig.enabled && !energyConfig.dryRun) {
    energyManager.stop([](const String &topic, const String &payload) {
      return mqtt.connected() &&
             mqtt.publish(topic.c_str(), payload.c_str(), false);
    });
  }
  energyConfig = next;
  energyManager.configure(energyConfig);
  saveConfig();
  eventLog.add("INFO", "ENERGY_CONFIG",
               energyConfig.enabled
                   ? (energyConfig.dryRun ? "Regler im Trockenlauf aktiviert"
                                          : "Regler LIVE aktiviert")
                   : "Regler deaktiviert");
  server.sendHeader("Location", "/energy", true);
  server.send(303, "text/plain", "");
}

void handleEnergyStop() {
  if (!requireAdmin()) return;
  energyManager.stop([](const String &topic, const String &payload) {
    return mqtt.connected() && mqtt.publish(topic.c_str(), payload.c_str(), false);
  });
  energyConfig.enabled = false;
  energyManager.configure(energyConfig);
  saveConfig();
  eventLog.add("WARN", "ENERGY_STOP", "Regler manuell gestoppt");
  server.sendHeader("Location", "/energy", true);
  server.send(303, "text/plain", "");
}

String settingsBackupJson() {
  DynamicJsonDocument document(8192);
  document["format"] = "irtracker-settings";
  document["version"] = 1;
  document["firmware"] = kFirmwareVersion;
  JsonArray wifi = document.createNestedArray("wifi");
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    JsonObject network = wifi.createNestedObject();
    network["ssid"] = config.ssid[i];
    network["password"] = config.password[i];
  }
  JsonObject device = document.createNestedObject("device");
  device["hostname"] = config.hostname;
  device["rx_pin"] = config.rxPin;
  device["tx_pin"] = config.txPin;
  device["led_pin"] = config.ledPin;
  device["led_inverted"] = config.ledInverted;
  device["baud"] = config.baud;
  device["api_access"] = config.apiAccess;
  device["sniffer"] = config.snifferEnabled;
  device["bridge"] = config.bridgeEnabled;
  device["timezone"] = config.timezone;
  device["setup_ap_minutes"] = config.setupApMinutes;
  device["persist_event_log"] = config.persistEventLog;
  device["eco_mode"] = config.ecoMode;
  device["eco_led_off"] = config.ecoLedOff;
  device["adaptive_wifi_power"] = config.adaptiveWifiPower;
  device["github_update_check"] = config.githubUpdateCheck;
  device["github_auto_install"] = config.githubAutoInstall;
  JsonObject mqttConfig = document.createNestedObject("mqtt");
  mqttConfig["host"] = config.mqttHost;
  mqttConfig["port"] = config.mqttPort;
  mqttConfig["user"] = config.mqttUser;
  mqttConfig["password"] = config.mqttPassword;
  mqttConfig["home_assistant_discovery"] = config.homeAssistantDiscovery;
  JsonObject pin = document.createNestedObject("meter_pin");
  pin["value"] = config.meterPin;
  pin["automatic"] = config.autoPin;
  pin["inverted"] = config.pinInverted;
  pin["pulse_ms"] = config.pinPulseMs;
  pin["digit_gap_ms"] = config.pinDigitGapMs;
  JsonObject energy = document.createNestedObject("energy");
  energy["driver"] = static_cast<uint8_t>(energyConfig.driver);
  energy["enabled"] = energyConfig.enabled;
  energy["dry_run"] = energyConfig.dryRun;
  energy["inverted"] = energyConfig.inverted;
  energy["host"] = energyConfig.host;
  energy["port"] = energyConfig.port;
  energy["unit_id"] = energyConfig.unitId;
  energy["power_register"] = energyConfig.powerRegister;
  energy["register_width"] = energyConfig.registerWidth;
  energy["word_swap"] = energyConfig.wordSwap;
  energy["register_scale"] = energyConfig.registerScale;
  energy["mqtt_topic"] = energyConfig.mqttTopic;
  energy["mqtt_payload"] = energyConfig.mqttPayload;
  energy["http_path"] = energyConfig.httpPath;
  energy["http_method"] = energyConfig.httpMethod;
  energy["http_payload"] = energyConfig.httpPayload;
  energy["http_bearer_token"] = energyConfig.httpBearerToken;
  energy["target_grid_w"] = energyConfig.targetGridW;
  energy["deadband_w"] = energyConfig.deadbandW;
  energy["max_charge_w"] = energyConfig.maxChargeW;
  energy["max_discharge_w"] = energyConfig.maxDischargeW;
  energy["ramp_w_per_second"] = energyConfig.rampWPerSecond;
  energy["interval_ms"] = energyConfig.intervalMs;
  energy["stale_ms"] = energyConfig.staleMs;
  String output;
  serializeJsonPretty(document, output);
  return output;
}

void handleSettingsBackup() {
  if (!requireAdmin()) return;
  server.sendHeader("Content-Disposition",
                    "attachment; filename=irtracker-settings.json");
  server.send(200, "application/json; charset=utf-8", settingsBackupJson());
}

void handleSettingsRestore() {
  if (!requireAdmin()) return;
  requestCpuBoost("settings_restore");
  if (server.arg("plain").length() > 16384) {
    server.send(413, "application/json",
                "{\"error\":\"settings_backup_too_large\"}");
    return;
  }
  DynamicJsonDocument document(8192);
  const DeserializationError error =
      deserializeJson(document, server.arg("plain"));
  if (error || document["format"] != "irtracker-settings" ||
      document["version"].as<int>() != 1) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_settings_backup\"}");
    return;
  }
  JsonArray wifi = document["wifi"].as<JsonArray>();
  if (wifi.size() != kWifiSlots) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_wifi_slots\"}");
    return;
  }
  JsonObject restoredDevice = document["device"];
  JsonObject restoredMqtt = document["mqtt"];
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    const String ssid = wifi[i]["ssid"] | "";
    const String password = wifi[i]["password"] | "";
    if (!safeSingleLine(ssid, 32) || !validWifiPassword(password)) {
      server.send(400, "application/json",
                  "{\"error\":\"invalid_wifi_credentials\"}");
      return;
    }
  }
  const String restoredHostname =
      String(restoredDevice["hostname"] | "ir-tracker");
  const String restoredTimezone = String(
      restoredDevice["timezone"] | "CET-1CEST,M3.5.0,M10.5.0/3");
  if (!validHostname(restoredHostname) ||
      !safeSingleLine(restoredTimezone, 80) ||
      !safeSingleLine(String(restoredMqtt["host"] | ""), 253) ||
      !safeSingleLine(String(restoredMqtt["user"] | ""), 128) ||
      !safeSingleLine(String(restoredMqtt["password"] | ""), 256)) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_settings_text\"}");
    return;
  }
  if (energyConfig.enabled && !energyConfig.dryRun) {
    energyManager.stop([](const String &topic, const String &payload) {
      return mqtt.connected() &&
             mqtt.publish(topic.c_str(), payload.c_str(), false);
    });
  }
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    config.ssid[i] = wifi[i]["ssid"] | "";
    config.password[i] = wifi[i]["password"] | "";
  }
  JsonObject device = document["device"];
  config.hostname = String(device["hostname"] | "ir-tracker");
  config.rxPin = constrain(device["rx_pin"] | 3, 0, 10);
  config.txPin = constrain(device["tx_pin"] | 6, -1, 10);
  config.ledPin = constrain(device["led_pin"] | 5, -1, 10);
  config.ledInverted = device["led_inverted"] | true;
  config.baud = constrain(device["baud"] | 9600, 300, 115200);
  config.apiAccess = constrain(device["api_access"] | 0, 0, 2);
  config.snifferEnabled = device["sniffer"] | false;
  config.bridgeEnabled = device["bridge"] | false;
  config.setupApMinutes =
      constrain(device["setup_ap_minutes"] | 15, 5, 60);
  config.persistEventLog = device["persist_event_log"] | false;
  config.ecoMode = device["eco_mode"] | true;
  config.ecoLedOff = device["eco_led_off"] | true;
  config.adaptiveWifiPower = device["adaptive_wifi_power"] | true;
  config.githubUpdateCheck = device["github_update_check"] | true;
  config.githubAutoInstall = device["github_auto_install"] | false;
  config.timezone = String(
      device["timezone"] | "CET-1CEST,M3.5.0,M10.5.0/3");
  JsonObject mqttConfig = document["mqtt"];
  config.mqttHost = String(mqttConfig["host"] | "");
  config.mqttPort = constrain(mqttConfig["port"] | 1883, 1, 65535);
  config.mqttUser = String(mqttConfig["user"] | "");
  config.mqttPassword = String(mqttConfig["password"] | "");
  config.homeAssistantDiscovery =
      mqttConfig["home_assistant_discovery"] | true;
  JsonObject pin = document["meter_pin"];
  config.meterPin = String(pin["value"] | "");
  if (config.meterPin.length() != 4) config.meterPin = "";
  config.autoPin = false;
  config.pinInverted = pin["inverted"] | false;
  config.pinPulseMs = constrain(pin["pulse_ms"] | 300, 50, 1000);
  config.pinDigitGapMs =
      constrain(pin["digit_gap_ms"] | 3000, 1000, 10000);
  JsonObject energy = document["energy"];
  energyConfig.driver = static_cast<EnergyManager::Driver>(
      constrain(energy["driver"] | 0, 0, 3));
  // DE: Wiederherstellung aktiviert nie echte Schreibzugriffe ohne neue LIVE-Bestätigung. | EN: Restore never enables real writes without fresh LIVE confirmation.
  energyConfig.enabled = false;
  energyConfig.dryRun = true;
  energyConfig.inverted = energy["inverted"] | false;
  energyConfig.host = String(energy["host"] | "");
  energyConfig.port = constrain(energy["port"] | 502, 1, 65535);
  energyConfig.unitId = constrain(energy["unit_id"] | 1, 1, 247);
  energyConfig.powerRegister =
      constrain(energy["power_register"] | 0, 0, 65535);
  energyConfig.registerWidth =
      energy["register_width"].as<int>() == 2 ? 2 : 1;
  energyConfig.wordSwap = energy["word_swap"] | false;
  energyConfig.registerScale = energy["register_scale"] | 1.0f;
  energyConfig.mqttTopic = String(energy["mqtt_topic"] | "");
  energyConfig.mqttPayload = String(energy["mqtt_payload"] | "{power}");
  energyConfig.httpPath = String(energy["http_path"] | "/api/power");
  energyConfig.httpMethod =
      String(energy["http_method"] | "POST") == "PUT" ? "PUT" : "POST";
  energyConfig.httpPayload = String(
      energy["http_payload"] |
      "{\"setpoint_w\":{power},\"grid_w\":{grid}}");
  energyConfig.httpBearerToken =
      String(energy["http_bearer_token"] | "");
  energyConfig.targetGridW =
      constrain(energy["target_grid_w"] | 0, -500, 500);
  energyConfig.deadbandW =
      constrain(energy["deadband_w"] | 30, 0, 500);
  energyConfig.maxChargeW =
      constrain(energy["max_charge_w"] | 800, 0, 10000);
  energyConfig.maxDischargeW =
      constrain(energy["max_discharge_w"] | 800, 0, 10000);
  energyConfig.rampWPerSecond =
      constrain(energy["ramp_w_per_second"] | 200, 10, 5000);
  energyConfig.intervalMs =
      constrain(energy["interval_ms"] | 2000, 1000, 30000);
  energyConfig.staleMs =
      constrain(energy["stale_ms"] | 10000, 3000, 60000);
  energyManager.configure(energyConfig);
  saveConfig();
  eventLog.add("INFO", "SETTINGS_RESTORE",
               "Einstellungen wiederhergestellt; Regler im Trockenlauf");
  server.send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
  delay(500);
  ESP.restart();
}

bool historyTierFromName(const String &name, HistoryStore::Tier &tier) {
  if (name == "minute")
    tier = HistoryStore::Tier::Minute;
  else if (name == "quarter")
    tier = HistoryStore::Tier::QuarterHour;
  else if (name == "hour")
    tier = HistoryStore::Tier::Hour;
  else if (name == "day")
    tier = HistoryStore::Tier::Day;
  else
    return false;
  return true;
}

void handleHistoryImportStart() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_import");
  if (server.arg("plain").length() > 256) {
    server.send(413, "application/json", "{\"error\":\"request_too_large\"}");
    return;
  }
  DynamicJsonDocument document(512);
  if (deserializeJson(document, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  HistoryStore::Tier tier;
  if (!historyTierFromName(String(document["tier"] | ""), tier) ||
      !history.clear(tier)) {
    server.send(400, "application/json", "{\"error\":\"invalid_tier\"}");
    return;
  }
  eventLog.add("WARN", "HISTORY_IMPORT",
               "Historienstufe für Wiederherstellung geleert");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleHistoryImportBatch() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_import");
  if (server.arg("plain").length() > 16384) {
    server.send(413, "application/json", "{\"error\":\"batch_too_large\"}");
    return;
  }
  DynamicJsonDocument document(12288);
  if (deserializeJson(document, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  HistoryStore::Tier tier;
  JsonArray values = document["values"].as<JsonArray>();
  if (!historyTierFromName(String(document["tier"] | ""), tier) ||
      values.isNull() || values.size() > 50) {
    server.send(400, "application/json", "{\"error\":\"invalid_batch\"}");
    return;
  }
  size_t imported = 0;
  for (JsonObject value : values) {
    HistoryStore::Record record = {
        value["ts"].as<uint32_t>(),
        value["avg"] | NAN,
        value["min"] | NAN,
        value["max"] | NAN,
        value["import"] | NAN,
        value["export"] | NAN};
    if (!history.importRecord(tier, record)) {
      server.send(400, "application/json",
                  "{\"error\":\"invalid_history_record\"}");
      return;
    }
    ++imported;
  }
  server.send(200, "application/json",
              "{\"ok\":true,\"imported\":" + String(imported) + "}");
}

void handleEventsJson() {
  if (!requireAdmin()) return;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"events\":[");
  bool first = true;
  String chunk;
  eventLog.forEach([&](const EventLog::Record &record) {
    if (!first) chunk += ',';
    first = false;
    chunk += "{\"ts\":" + String(record.timestamp) +
             ",\"uptime_s\":" + String(record.uptimeSeconds) +
             ",\"level\":\"" + jsonEscape(record.level) +
             "\",\"code\":\"" + jsonEscape(record.code) +
             "\",\"message\":\"" + jsonEscape(record.message) + "\"}";
    if (chunk.length() > 900) {
      server.sendContent(chunk);
      chunk = "";
    }
    return true;
  });
  if (chunk.length()) server.sendContent(chunk);
  server.sendContent("]}");
}

void handleEventsClear() {
  if (!requireAdmin()) return;
  const bool ok = eventLog.clear();
  if (ok) eventLog.add("INFO", "LOG_CLEAR", "Ereignisprotokoll gelöscht");
  server.send(ok ? 200 : 500, "application/json",
              ok ? "{\"ok\":true}" : "{\"error\":\"clear_failed\"}");
}

void handleHistoryClearAll() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_clear");
  if (server.arg("confirm") != "DELETE") {
    server.send(400, "application/json",
                "{\"error\":\"confirmation_required\"}");
    return;
  }
  bool ok = true;
  ok &= history.clear(HistoryStore::Tier::Minute);
  ok &= history.clear(HistoryStore::Tier::QuarterHour);
  ok &= history.clear(HistoryStore::Tier::Hour);
  ok &= history.clear(HistoryStore::Tier::Day);
  liveWriteIndex = 0;
  liveCount = 0;
  if (ok) eventLog.add("WARN", "HISTORY_CLEAR", "Gesamte Historie gelöscht");
  server.send(ok ? 200 : 500, "application/json",
              ok ? "{\"ok\":true}" : "{\"error\":\"history_clear_failed\"}");
}

bool otaRequestAuthorized() {
  return requireAdmin();
}

void resetSignedOta() {
  if (signedOta.updateStarted) Update.abort();
  mbedtls_sha256_free(&signedOta.sha);
  signedOta = SignedOtaState{};
  mbedtls_sha256_init(&signedOta.sha);
}

bool beginSignedOtaImage() {
  static const uint8_t magic[8] = {'I', 'R', 'F', 'W', '1', '0', '0', 0};
  if (memcmp(signedOta.header, magic, sizeof(magic)) != 0) {
    otaUploadError = "invalid_package_magic";
    return false;
  }
  signedOta.firmwareSize =
      static_cast<uint32_t>(signedOta.header[8]) |
      (static_cast<uint32_t>(signedOta.header[9]) << 8) |
      (static_cast<uint32_t>(signedOta.header[10]) << 16) |
      (static_cast<uint32_t>(signedOta.header[11]) << 24);
  signedOta.signatureSize =
      static_cast<uint16_t>(signedOta.header[12]) |
      (static_cast<uint16_t>(signedOta.header[13]) << 8);
  if (signedOta.firmwareSize < 1024 ||
      signedOta.firmwareSize > ESP.getFreeSketchSpace() ||
      signedOta.signatureSize < 64 ||
      signedOta.signatureSize > sizeof(signedOta.signature)) {
    otaUploadError = "invalid_package_sizes";
    return false;
  }
  if (!Update.begin(signedOta.firmwareSize, U_FLASH)) {
    otaUploadError = "update_partition_unavailable";
    return false;
  }
  signedOta.updateStarted = true;
  if (mbedtls_sha256_starts_ret(&signedOta.sha, 0) != 0) {
    otaUploadError = "sha256_initialization_failed";
    return false;
  }
  return true;
}

bool consumeSignedOta(const uint8_t *data, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    if (signedOta.headerRead < sizeof(signedOta.header)) {
      const size_t count =
          std::min(length - offset,
                   sizeof(signedOta.header) - signedOta.headerRead);
      memcpy(signedOta.header + signedOta.headerRead, data + offset, count);
      signedOta.headerRead += count;
      offset += count;
      if (signedOta.headerRead == sizeof(signedOta.header) &&
          !beginSignedOtaImage())
        return false;
      continue;
    }
    if (signedOta.signatureRead < signedOta.signatureSize) {
      const size_t count =
          std::min(length - offset,
                   static_cast<size_t>(signedOta.signatureSize) -
                       signedOta.signatureRead);
      memcpy(signedOta.signature + signedOta.signatureRead, data + offset,
             count);
      signedOta.signatureRead += count;
      offset += count;
      continue;
    }
    const size_t remaining =
        signedOta.firmwareSize - signedOta.firmwareWritten;
    if (!remaining) {
      otaUploadError = "package_has_trailing_data";
      return false;
    }
    const size_t count = std::min(length - offset, remaining);
    if (!signedOta.firstFirmwareByteChecked) {
      signedOta.firstFirmwareByteChecked = true;
      if (data[offset] != 0xE9) {
        otaUploadError = "not_an_esp32_application";
        return false;
      }
    }
    if (mbedtls_sha256_update_ret(&signedOta.sha, data + offset, count) != 0 ||
        Update.write(const_cast<uint8_t *>(data + offset), count) != count) {
      otaUploadError = "firmware_write_failed";
      return false;
    }
    signedOta.firmwareWritten += count;
    offset += count;
  }
  return true;
}

bool finishSignedOta() {
  if (!signedOta.updateStarted ||
      signedOta.firmwareWritten != signedOta.firmwareSize ||
      signedOta.signatureRead != signedOta.signatureSize) {
    otaUploadError = "incomplete_signed_package";
    return false;
  }
  uint8_t digest[32];
  if (mbedtls_sha256_finish_ret(&signedOta.sha, digest) != 0) {
    otaUploadError = "sha256_finalization_failed";
    return false;
  }
  mbedtls_pk_context publicKey;
  mbedtls_pk_init(&publicKey);
  const int parseResult = mbedtls_pk_parse_public_key(
      &publicKey,
      reinterpret_cast<const unsigned char *>(kFirmwareSigningPublicKey),
      strlen(kFirmwareSigningPublicKey) + 1);
  const int verifyResult =
      parseResult == 0
          ? mbedtls_pk_verify(&publicKey, MBEDTLS_MD_SHA256, digest,
                              sizeof(digest), signedOta.signature,
                              signedOta.signatureSize)
          : parseResult;
  mbedtls_pk_free(&publicKey);
  memset(digest, 0, sizeof(digest));
  if (verifyResult != 0) {
    otaUploadError = "firmware_signature_invalid";
    return false;
  }
  if (!history.flushPending(HistoryStore::Tier::Minute)) {
    otaUploadError = "history_flush_before_update_failed";
    eventLog.add("ERROR", "OTA_HISTORY_FLUSH",
                 "Update abgebrochen: Minutenpuffer nicht speicherbar");
    return false;
  }
  eventLog.add("INFO", "OTA_HISTORY_FLUSH",
               "Offenen Minutenblock vor Update gespeichert");
  if (!Update.end(true)) {
    otaUploadError = "firmware_image_validation_failed";
    return false;
  }
  return true;
}

uint64_t firmwareVersionNumber(const String &value) {
  String normalized = value;
  if (normalized.startsWith("v")) normalized.remove(0, 1);
  unsigned int major = 0, minor = 0, patch = 0, beta = 255;
  if (sscanf(normalized.c_str(), "%u.%u.%u-beta.%u", &major, &minor,
             &patch, &beta) < 3) {
    beta = 255;
    if (sscanf(normalized.c_str(), "%u.%u.%u", &major, &minor, &patch) != 3)
      return 0;
  }
  if (major > 65535 || minor > 65535 || patch > 65535 || beta > 255)
    return 0;
  return (static_cast<uint64_t>(major) << 40) |
         (static_cast<uint64_t>(minor) << 24) |
         (static_cast<uint64_t>(patch) << 8) | beta;
}

String githubUpdateJson() {
  String json = "{\"current_version\":\"" + String(kFirmwareVersion) +
                "\",\"automatic_checks\":" +
                String(config.githubUpdateCheck ? "true" : "false") +
                ",\"automatic_install\":" +
                String(config.githubAutoInstall ? "true" : "false") +
                ",\"checked\":" + String(githubUpdate.checked ? "true" : "false") +
                ",\"checking\":" + String(githubUpdate.checking ? "true" : "false") +
                ",\"installing\":" + String(githubUpdate.installing ? "true" : "false") +
                ",\"available\":" + String(githubUpdate.available ? "true" : "false") +
                ",\"latest_version\":\"" + jsonEscape(githubUpdate.version) +
                "\",\"asset_name\":\"" + jsonEscape(githubUpdate.assetName) +
                "\",\"asset_size\":" + String(githubUpdate.assetSize) +
                ",\"last_success\":" + String(static_cast<uint32_t>(githubUpdate.lastSuccess)) +
                ",\"error\":\"" + jsonEscape(githubUpdate.error) + "\"}";
  return json;
}

bool checkGithubFirmwareUpdate() {
  if (githubUpdate.checking || githubUpdate.installing) return false;
  githubUpdate.checking = true;
  githubUpdate.error = "";
  githubUpdate.lastAttemptMs = millis();
  githubUpdate.available = false;
  githubUpdate.version = "";
  githubUpdate.assetName = "";
  githubUpdate.assetUrl = "";
  githubUpdate.assetSize = 0;
  if (WiFi.status() != WL_CONNECTED) {
    githubUpdate.error = "wifi_not_connected";
    githubUpdate.checking = false;
    return false;
  }
  if (time(nullptr) < 1700000000) {
    githubUpdate.error = "system_time_not_synchronized";
    githubUpdate.checking = false;
    return false;
  }
  requestCpuBoost("github_update_check");
  WiFiClientSecure client;
  client.setCACert(kGithubRootCertificates);
  HTTPClient http;
  http.setConnectTimeout(7000);
  http.setTimeout(9000);
  if (!http.begin(client, kGithubReleasesApi)) {
    githubUpdate.error = "github_connection_initialization_failed";
    githubUpdate.checking = false;
    return false;
  }
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("X-GitHub-Api-Version", "2022-11-28");
  http.addHeader("User-Agent", "IR-Tracker-Offline/" + String(kFirmwareVersion));
  const int response = http.GET();
  if (response != HTTP_CODE_OK) {
    githubUpdate.error = "github_http_" + String(response);
    http.end();
    githubUpdate.checking = false;
    return false;
  }
  StaticJsonDocument<512> filter;
  filter[0]["draft"] = true;
  filter[0]["tag_name"] = true;
  filter[0]["assets"][0]["name"] = true;
  filter[0]["assets"][0]["browser_download_url"] = true;
  filter[0]["assets"][0]["size"] = true;
  DynamicJsonDocument releases(12288);
  const DeserializationError parseError = deserializeJson(
      releases, http.getStream(), DeserializationOption::Filter(filter));
  if (parseError) {
    githubUpdate.error = "github_json_invalid";
    http.end();
    githubUpdate.checking = false;
    return false;
  }
  const uint64_t current = firmwareVersionNumber(kFirmwareVersion);
  uint64_t best = current;
  for (JsonObject release : releases.as<JsonArray>()) {
    if (release["draft"] | true) continue;
    const String tag = release["tag_name"] | "";
    const uint64_t candidate = firmwareVersionNumber(tag);
    if (!candidate || candidate <= best) continue;
    for (JsonObject asset : release["assets"].as<JsonArray>()) {
      const String name = asset["name"] | "";
      const String url = asset["browser_download_url"] | "";
      const size_t size = asset["size"] | 0;
      if (!name.startsWith("ir-tracker-custom-") || !name.endsWith(".irfw") ||
          !url.startsWith(kGithubAssetPrefix) || size < 1024 ||
          size > kGithubMaximumPackageBytes)
        continue;
      best = candidate;
      githubUpdate.version = tag.startsWith("v") ? tag.substring(1) : tag;
      githubUpdate.assetName = name;
      githubUpdate.assetUrl = url;
      githubUpdate.assetSize = size;
      githubUpdate.available = true;
      break;
    }
  }
  http.end();
  githubUpdate.checked = true;
  githubUpdate.lastSuccess = time(nullptr);
  githubUpdate.checking = false;
  eventLog.add("INFO", "GITHUB_UPDATE_CHECK",
               githubUpdate.available
                   ? "Signiertes Firmwareupdate " + githubUpdate.version + " gefunden"
                   : "Keine neuere signierte Firmware verfuegbar");
  return true;
}

bool installGithubFirmwareUpdate() {
  if (!githubUpdate.available || githubUpdate.installing ||
      !githubUpdate.assetUrl.startsWith(kGithubAssetPrefix) ||
      githubUpdate.assetSize < 1024 ||
      githubUpdate.assetSize > kGithubMaximumPackageBytes) {
    githubUpdate.error = "no_valid_update_selected";
    return false;
  }
  githubUpdate.installing = true;
  githubUpdate.error = "";
  requestCpuBoost("github_update_install");
  WiFiClientSecure client;
  client.setCACert(kGithubRootCertificates);
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, githubUpdate.assetUrl)) {
    githubUpdate.error = "update_connection_initialization_failed";
    githubUpdate.installing = false;
    return false;
  }
  http.addHeader("Accept", "application/octet-stream");
  http.addHeader("User-Agent", "IR-Tracker-Offline/" + String(kFirmwareVersion));
  const int response = http.GET();
  if (response != HTTP_CODE_OK) {
    githubUpdate.error = "update_http_" + String(response);
    http.end();
    githubUpdate.installing = false;
    return false;
  }
  const int declaredLength = http.getSize();
  if (declaredLength > 0 &&
      static_cast<size_t>(declaredLength) != githubUpdate.assetSize) {
    githubUpdate.error = "update_size_mismatch";
    http.end();
    githubUpdate.installing = false;
    return false;
  }
  resetSignedOta();
  otaUploadError = "";
  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[1024];
  size_t received = 0;
  uint32_t lastProgress = millis();
  bool ok = true;
  while (received < githubUpdate.assetSize) {
    esp_task_wdt_reset();
    const int available = stream->available();
    if (available > 0) {
      const size_t wanted = std::min<size_t>(
          sizeof(buffer), std::min<size_t>(available,
                                           githubUpdate.assetSize - received));
      const int count = stream->readBytes(buffer, wanted);
      if (count <= 0 || !consumeSignedOta(buffer, count)) {
        ok = false;
        break;
      }
      received += count;
      lastProgress = millis();
    } else if (!http.connected() || millis() - lastProgress > 12000) {
      otaUploadError = "update_download_incomplete";
      ok = false;
      break;
    } else {
      delay(2);
    }
  }
  if (ok && received == githubUpdate.assetSize) ok = finishSignedOta();
  if (!ok) Update.abort();
  http.end();
  memset(buffer, 0, sizeof(buffer));
  githubUpdate.installing = false;
  if (!ok) {
    githubUpdate.error = otaUploadError.length() ? otaUploadError : "update_failed";
    eventLog.add("ERROR", "GITHUB_UPDATE_FAILED", githubUpdate.error);
    return false;
  }
  eventLog.add("WARN", "GITHUB_UPDATE_INSTALLED",
               "Signiertes GitHub-Update " + githubUpdate.version + " installiert");
  return true;
}

void manageGithubFirmwareUpdate() {
  if (!config.githubUpdateCheck || githubUpdate.checking ||
      githubUpdate.installing || gpioScan.active || irPulse.active ||
      WiFi.status() != WL_CONNECTED)
    return;
  const uint32_t interval = githubUpdate.checked ? kGithubCheckIntervalMs
                                                 : kGithubInitialCheckMs;
  if (millis() - githubUpdate.lastAttemptMs < interval) return;
  if (checkGithubFirmwareUpdate() && config.githubAutoInstall &&
      githubUpdate.available && installGithubFirmwareUpdate()) {
    delay(500);
    ESP.restart();
  }
}

void handleOtaUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    resetSignedOta();
    otaUploadError = "";
    otaUploadAuthorized = otaRequestAuthorized();
    if (otaUploadAuthorized) requestCpuBoost("firmware_update");
    otaUploadOk = otaUploadAuthorized &&
                  upload.filename.endsWith(".irfw");
    if (!otaUploadAuthorized) otaUploadError = "unauthorized";
    if (otaUploadAuthorized && !upload.filename.endsWith(".irfw"))
      otaUploadError = "signed_irfw_package_required";
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (otaUploadOk &&
        !consumeSignedOta(upload.buf, upload.currentSize)) {
      otaUploadOk = false;
      Update.abort();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (otaUploadOk) otaUploadOk = finishSignedOta();
    if (!otaUploadOk) Update.abort();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    otaUploadOk = false;
    otaUploadError = "upload_aborted";
    Update.abort();
  }
}

void handleOtaFinished() {
  if (!otaUploadAuthorized) {
    server.send(
        401, "text/html; charset=utf-8",
        page("Firmwareupdate nicht möglich",
             "<div class='error'><strong>Anmeldung oder Sicherheitsprüfung "
             "abgelaufen.</strong><p>Wartungsseite neu laden, erneut anmelden "
             "und das signierte IRFW-Paket noch einmal auswählen.</p></div>"
             "<p><a href='/maintenance#firmware-update'>Zurück zur Wartung</a></p>"));
    return;
  }
  if (!otaUploadOk) {
    const String technicalCode =
        otaUploadError.length() ? otaUploadError : "invalid_signed_firmware";
    server.send(
        400, "text/html; charset=utf-8",
        page("Firmwareupdate abgelehnt",
             "<div class='error'><strong>Das Firmwarepaket konnte nicht sicher "
             "installiert werden.</strong><p>Nur ein vollständiges, für diesen "
             "Tracker signiertes IRFW-Paket verwenden.</p><details><summary>"
             "Technischer Fehlercode</summary><code>" +
                 htmlEscape(technicalCode) +
                 "</code></details></div><p><a href='/maintenance#firmware-update'>"
                 "Zurück zur Wartung</a></p>"));
    return;
  }
  eventLog.add("WARN", "OTA_UPDATE",
               "Kryptografisch signierte Custom-Firmware installiert");
  server.send(200, "text/html; charset=utf-8",
              page("Update erfolgreich",
                   "<div class='card'><h2>Firmware geprüft und installiert</h2>"
                   "<p>Der Tracker startet jetzt mit dem neuen Custom-Slot.</p></div>"));
  delay(800);
  ESP.restart();
}

void handleSafeShutdown() {
  if (!requireAdmin()) return;
  if (server.arg("confirm") != "SHUTDOWN") {
    server.send(400, "application/json",
                "{\"error\":\"shutdown_confirmation_required\"}");
    return;
  }
  if (!history.flushPending(HistoryStore::Tier::Minute)) {
    eventLog.add("ERROR", "SHUTDOWN_ABORT",
                 "Herunterfahren wegen Speicherfehler abgebrochen");
    server.send(500, "application/json",
                "{\"error\":\"history_flush_failed\"}");
    return;
  }
  irPulse.active = false;
  apatorUnlock.active = false;
  eventLog.add("INFO", "SAFE_SHUTDOWN",
               "Minutenpuffer gespeichert, Tiefschlaf wird gestartet");
  server.send(200, "application/json",
              "{\"ok\":true,\"state\":\"deep_sleep\","
              "\"wake\":\"power_cycle_or_reset\"}");
  delay(600);
  if (mqtt.connected()) mqtt.disconnect();
  dns.stop();
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
  meterSerial.end();
  if (config.txPin >= 0) {
    pinMode(config.txPin, OUTPUT);
    digitalWrite(config.txPin, config.pinInverted);
  }
  if (config.ledPin >= 0)
    digitalWrite(config.ledPin, config.ledInverted ? HIGH : LOW);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_deep_sleep_start();
}

void handleMaintenancePage() {
  if (!requireAdmin()) return;
  String body = maintenanceTabs(false);
  body += F(
      "<div class='grid'><div class='card'><h2>Einstellungen</h2>"
      "<p>Enthält WLAN-, MQTT-, PIN- und Geräteprofil-Daten. Die Datei enthält Geheimnisse und muss sicher aufbewahrt werden.</p>"
      "<button id='fullBackup'>Vollständiges Backup erstellen</button>"
      "<button class='secondary' id='settingsExport'>Nur Einstellungen herunterladen</button>"
      "<label>Backup wiederherstellen</label><input id='settingsFile' type='file' accept='.json,application/json'>"
      "<button id='settingsImport'>Einstellungen prüfen und wiederherstellen</button></div>"
      "<div class='card'><h2>Historie</h2><p>Alle vier Ringpuffer werden als eine JSON-Datei im Browser zusammengeführt.</p>"
      "<button id='historyExport'>Historie herunterladen</button>"
      "<label>Historienbackup</label><input id='historyFile' type='file' accept='.json,application/json'>"
      "<button id='historyImport'>Historie gestaffelt wiederherstellen</button>"
      "<button class='danger' id='historyClear'>Gesamte Historie löschen</button></div></div>"
      "<div class='card'><h2>Custom-Firmware aktualisieren</h2>"
      "<p>Es werden ausschließlich kryptografisch signierte IRFW-Pakete von Michael Roßmann akzeptiert. "
      "Signatur und ESP32-Image werden vor der Aktivierung geprüft.</p>"
      "<form method='post' action='/system/update' enctype='multipart/form-data' "
      "onsubmit=\"return confirm('Firmware installieren und Tracker neu starten?')\">"
      "<label>Signiertes Firmwarepaket (.irfw)</label><input type='file' name='firmware' "
      "accept='.irfw,application/octet-stream' required>"
      "<button type='submit'>WLAN-Update installieren</button></form></div>"
      "<div class='card' id='firmware-update'><h2>GitHub-Firmwareupdate</h2>"
      "<p>Prüft das offizielle Projekt auf eine neuere Version. Installiert werden "
      "ausschließlich passend signierte IRFW-Pakete; ein Downgrade ist gesperrt.</p>"
      "<div class='grid'><div><span class='muted'>Installierte Version</span><br><strong id='updateCurrent'>–</strong></div>"
      "<div><span class='muted'>Verfügbare Version</span><br><strong id='updateAvailable'>–</strong></div>"
      "<div><span class='muted'>Letzte erfolgreiche Prüfung</span><br><strong id='updateLast'>Noch nicht geprüft</strong></div></div>"
      "<p id='updateState' class='muted'>Status wird geladen …</p>"
      "<details id='updateError' hidden><summary>Technischer Fehlercode</summary><code id='updateErrorCode'></code></details>"
      "<div class='actions'><form method='post' action='/api/v1/update/check'><button type='submit'>Jetzt prüfen</button></form>"
      "<form id='updateInstall' method='post' action='/api/v1/update/install' hidden "
      "onsubmit=\"return confirm('Signiertes GitHub-Update installieren und neu starten?')\">"
      "<button class='secondary' type='submit'>Gefundenes Update installieren</button></form></div></div>"
      "<div class='card'><h2>Tracker sicher ausschalten</h2>"
      "<p>Speichert den offenen Minutenblock und versetzt den ESP32 danach in Tiefschlaf. "
      "Zum Wiedereinschalten Strom kurz aus- und einschalten oder Reset betätigen.</p>"
      "<button class='danger' id='safeShutdown' type='button'>Sicher herunterfahren</button></div>"
      "<div class='card'><h2>Ereignis- und Fehlerprotokoll</h2>"
      "<button id='eventsReload'>Protokoll laden</button><button class='danger' id='eventsClear'>Protokoll löschen</button>"
      "<pre id='events' style='white-space:pre-wrap;max-height:420px;overflow:auto'></pre></div>"
      "<p id='maintenanceStatus' class='muted'></p>");
  String script = F(
      "const el=id=>document.getElementById(id),status=t=>el('maintenanceStatus').textContent=t;"
      "const updateText=e=>{if(e==='wifi_not_connected')return'Keine WLAN-Verbindung. Netzwerkverbindung prüfen.';if(e==='system_time_not_synchronized')return'Die Gerätezeit ist noch nicht synchronisiert.';if(e==='github_json_invalid')return'Die Antwort der Updatequelle konnte nicht verarbeitet werden.';if(e.startsWith('github_http_'))return'Die Updatequelle ist momentan nicht erreichbar.';return'Die Updateprüfung konnte nicht abgeschlossen werden.'};"
      "async function loadUpdate(){try{const r=await fetch('/api/v1/update/status'),u=await r.json();el('updateCurrent').textContent=u.current_version;el('updateAvailable').textContent=u.available?u.latest_version:'–';el('updateLast').textContent=u.last_success?new Date(u.last_success*1000).toLocaleString():'Noch nicht geprüft';el('updateInstall').hidden=!u.available;const s=el('updateState'),d=el('updateError');d.hidden=!u.error;el('updateErrorCode').textContent=u.error||'';s.className=u.error?'error':u.checked?'status-pill':'muted';s.textContent=u.error?'Updateprüfung fehlgeschlagen: '+updateText(u.error):u.available?'Eine neuere signierte Firmware ist verfügbar.':u.checked?'Die installierte Firmware ist aktuell.':'Es wurde in dieser Laufzeit noch keine manuelle Prüfung durchgeführt.'}catch(e){el('updateState').className='error';el('updateState').textContent='Update-Status konnte nicht geladen werden.'}}loadUpdate();"
      "const download=(name,data)=>{const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([data],{type:'application/json'}));a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000)};"
      "el('fullBackup').onclick=()=>{status('Vollständiges Backup wird erstellt …');el('settingsExport').click();setTimeout(()=>el('historyExport').click(),800)};"
      "el('settingsExport').onclick=async()=>{status('Einstellungen werden geladen ...');const r=await fetch('/api/v1/backup/settings');download('irtracker-settings.json',await r.text());status('Einstellungsbackup erstellt')};"
      "el('settingsImport').onclick=async()=>{const f=el('settingsFile').files[0];if(!f)return status('Bitte Einstellungsbackup auswählen');"
      "const text=await f.text();let j;try{j=JSON.parse(text)}catch(e){return status('Ungültige JSON-Datei')}if(j.format!=='irtracker-settings'||j.version!==1)return status('Falsches Backupformat');"
      "if(!confirm('Einstellungen ersetzen und Tracker neu starten?'))return;const r=await fetch('/api/v1/backup/settings/restore',{method:'POST',headers:{'Content-Type':'application/json'},body:text});status(r.ok?'Wiederhergestellt, Tracker startet neu':'Wiederherstellung fehlgeschlagen')};"
      "el('historyExport').onclick=async()=>{status('Historie wird gestreamt ...');const out={format:'irtracker-history',version:1,created:new Date().toISOString(),tiers:{}};"
      "for(const [name,range] of Object.entries({minute:'minute_all',quarter:'quarter_all',hour:'hour_all',day:'day_all'})){const r=await fetch('/api/v1/history?range='+range);if(!r.ok)return status('Fehler bei '+name);out.tiers[name]=(await r.json()).values}"
      "download('irtracker-history.json',JSON.stringify(out));status('Historienbackup erstellt')};"
      "el('historyImport').onclick=async()=>{const f=el('historyFile').files[0];if(!f)return status('Bitte Historienbackup auswählen');let b;try{b=JSON.parse(await f.text())}catch(e){return status('Ungültige JSON-Datei')}"
      "if(b.format!=='irtracker-history'||b.version!==1||!b.tiers)return status('Falsches Backupformat');for(const n of ['minute','quarter','hour','day']){if(!Array.isArray(b.tiers[n]))return status('Historienstufe fehlt: '+n);"
      "if(!b.tiers[n].every(v=>Number.isInteger(v.ts)&&v.ts>=1700000000&&Number.isFinite(v.avg)&&Number.isFinite(v.min)&&Number.isFinite(v.max)&&(v.import==null||Number.isFinite(v.import))&&(v.export==null||Number.isFinite(v.export))))return status('Ungültiger Datensatz in '+n)}"
      "if(!confirm('Vorhandene Historie durch dieses Backup ersetzen?'))return;for(const n of ['minute','quarter','hour','day']){status('Importiere '+n+' ...');let r=await fetch('/api/v1/history/import/start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tier:n})});if(!r.ok)return status('Start fehlgeschlagen: '+n);"
      "for(let i=0;i<b.tiers[n].length;i+=50){r=await fetch('/api/v1/history/import/batch',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tier:n,values:b.tiers[n].slice(i,i+50)})});if(!r.ok)return status('Import fehlgeschlagen: '+n+' '+i);status('Importiere '+n+': '+Math.min(i+50,b.tiers[n].length)+' / '+b.tiers[n].length)}}status('Historie vollständig wiederhergestellt')};"
      "el('historyClear').onclick=async()=>{if(!confirm('Wirklich ALLE lokalen Messwerte dauerhaft löschen? Vorher Backup erstellen!'))return;"
      "const r=await fetch('/api/v1/history/clear',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'confirm=DELETE'});status(r.ok?'Historie vollständig gelöscht':'Historie konnte nicht gelöscht werden')};"
      "async function loadEvents(){const r=await fetch('/api/v1/events');const j=await r.json();el('events').textContent=j.events.map(x=>`${x.ts>1700000000?new Date(x.ts*1000).toLocaleString('de-DE'):'Uptime '+x.uptime_s+'s'} [${x.level}] ${x.code}: ${x.message}`).join('\\n')||'Keine Ereignisse'}"
      "el('eventsReload').onclick=loadEvents;el('eventsClear').onclick=async()=>{if(confirm('Protokoll wirklich löschen?')){await fetch('/api/v1/events/clear',{method:'POST'});loadEvents()}};loadEvents();");
  script += F(
      "el('safeShutdown').onclick=async()=>{if(!confirm('Tracker wirklich sicher herunterfahren? Zum Starten ist danach Strom Aus/Ein oder Reset nötig.'))return;"
      "status('Minutenpuffer wird gespeichert ...');const r=await fetch('/system/shutdown',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'confirm=SHUTDOWN'});"
      "status(r.ok?'Tracker wurde sicher heruntergefahren. Zum Starten Strom Aus/Ein oder Reset.':'Herunterfahren fehlgeschlagen; Tracker bleibt aktiv.')};");
  server.send(200, "text/html; charset=utf-8",
              page("Backup und Wartung", body, script));
}

void handleSetup() {
  if (!requireAdmin()) return;
  String body = F("<form method='post' action='/setup/save'><fieldset><legend>WLAN-Verbindungen</legend>"
                  "<p class='muted'>Bis zu drei Netze. Der Tracker probiert sie der Reihe nach. Ist keines erreichbar, startet automatisch der Setup-Hotspot.</p>"
                  "<div class='error'>HTTP ist nicht transportverschlüsselt. Nur in einem vertrauenswürdigen Heim- oder getrennten IoT-Netz betreiben, keine Router-Portfreigabe einrichten und Fernzugriff ausschließlich per VPN verwenden.</div>");
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    body += "<div class='inline'><div><label>WLAN " + String(i + 1) + "</label><input name='ssid" +
            String(i) + "' value=\"" + htmlEscape(config.ssid[i]) +
            "\" maxlength='32' placeholder='Netzwerkname'></div><div><label>Passwort</label><input type='password' name='pass" +
            String(i) + "' placeholder='" + String(config.password[i].length() ? "gespeichert" : "offenes WLAN") +
            "' maxlength='64' autocomplete='off' data-lpignore='true'></div></div>";
  }
  body += F("<label>Hostname</label><input name='hostname' value='");
  body += htmlEscape(config.hostname);
  body += F("'><label>Zeitzone (POSIX-TZ)</label><input name='timezone' value='");
  body += htmlEscape(config.timezone);
  body += F("'><p class='muted'>Deutschland: CET-1CEST,M3.5.0,M10.5.0/3</p>"
            "<label>Setup-Hotspot Laufzeit (Minuten)</label><input type='number' name='ap_minutes' min='5' max='60' value='");
  body += String(config.setupApMinutes);
  body += F("'><p class='muted'>Nach Ablauf wird der Hotspot abgeschaltet. Ein Neustart öffnet ihn erneut.</p>"
            "<label>Neues Admin-Passwort</label><input type='password' name='admin_pass' "
            "minlength='4' maxlength='64' autocomplete='new-password' placeholder='unverändert lassen'>"
            "<label>Neues Admin-Passwort wiederholen</label><input type='password' name='admin_pass_confirm' "
            "minlength='4' maxlength='64' autocomplete='new-password' placeholder='unverändert lassen'>"
            "<p class='muted'>Erlaubt sind 4 bis 64 Zeichen; für gute Sicherheit werden mindestens 12 Zeichen empfohlen. "
            "Angemeldete Browser werden 60 Tage über ein signiertes "
            "HttpOnly-Cookie wiedererkannt. Eine Passwortänderung macht alte Sitzungen ungültig.</p>"
            "</fieldset><fieldset><legend>Stromzähler</legend>"
            "<label>IR-Eingang (GPIO)</label><select name='rx_pin'>");
  for (int pin = 0; pin <= 10; ++pin) {
    body += "<option value='" + String(pin) + "'" + (pin == config.rxPin ? " selected" : "") + ">" + String(pin) + "</option>";
  }
  body += F("</select><label>IR-Sendeausgang (GPIO, -1 = aus)</label><select name='tx_pin'>"
            "<option value='-1'>Aus</option>");
  for (int pin = 0; pin <= 10; ++pin) {
    body += "<option value='" + String(pin) + "'" + (pin == config.txPin ? " selected" : "") + ">" + String(pin) + "</option>";
  }
  body += "</select><label><input style='width:auto' type='checkbox' name='sniffer' value='1'" +
          String(config.snifferEnabled ? " checked" : "") +
          "> IR-Sniffer auf Port 81 aktivieren</label>"
          "<label><input style='width:auto' type='checkbox' name='bridge' value='1'" +
          String(config.bridgeEnabled ? " checked" : "") +
          "> Schreibende IR-Bridge aktivieren</label><p class='muted'>Aus Sicherheitsgründen standardmäßig deaktiviert.</p>";
  body += F("<label>Status-LED (GPIO, -1 = aus)</label><select name='led_pin'>"
            "<option value='-1'>Aus</option>");
  for (int pin = 0; pin <= 10; ++pin) {
    body += "<option value='" + String(pin) + "'" + (pin == config.ledPin ? " selected" : "") + ">" + String(pin) + "</option>";
  }
  body += "</select><label><input style='width:auto' type='checkbox' name='led_inv' value='1'" +
          String(config.ledInverted ? " checked" : "") + "> LED invertieren</label>";
  body += F("<label>Baudrate</label><select name='baud'>");
  const uint32_t rates[] = {9600, 19200, 38400, 115200};
  for (uint32_t rate : rates) {
    body += "<option value='" + String(rate) + "'" + (rate == config.baud ? " selected" : "") + ">" + String(rate) + "</option>";
  }
  body += F("</select></fieldset><fieldset><legend>Lokale API und Kompatibilität</legend>"
            "<label>Zugriffsmodus</label><select name='api_access'><option value='0'");
  if (config.apiAccess == 0) body += " selected";
  body += F(">Lokal offen (Integrationen ohne Anmeldung)</option><option value='1'");
  if (config.apiAccess == 1) body += " selected";
  body += F(">Admin-Anmeldung erforderlich</option><option value='2'");
  if (config.apiAccess == 2) body += " selected";
  body += F(">API und Shelly-Kompatibilität deaktiviert</option></select>"
            "<p class='muted'>Betrifft Messwert-API, Prometheus, Influx, CSV und Shelly-Endpunkte. Einstellungen und Wartung bleiben immer geschützt.</p>"
            "<details class='compact-details'><summary>JSON-API für Experten</summary>"
            "<p class='muted'>Für Home Assistant, ioBroker, Node-RED, openHAB und eigene lokale Auswertungen. "
            "Die API ist keine eigene Bedienseite und bleibt deshalb aus der Hauptnavigation ausgeblendet.</p>"
            "<code>/api/v1/status</code><br><code>/api/v1/obis</code><br>"
            "<code>/api/v1/history</code><br><code>/api/v1/values.csv</code>"
            "</details>"
            "<label><input style='width:auto' type='checkbox' name='event_flash' value='1'");
  if (config.persistEventLog) body += " checked";
  body += F("> Ereignis- und Fehlerprotokoll dauerhaft im Flash speichern</label>"
            "<p class='muted'>Standardmäßig aus: Bis zu 256 Einträge bleiben nur im RAM. "
            "Die ältesten werden automatisch überschrieben; ein Neustart leert das Protokoll. "
            "Aktivieren schreibt neue Einträge zusätzlich dauerhaft in den Flash.</p>"
            "</fieldset><fieldset><legend>Home Assistant / MQTT</legend>"
            "<p class='muted'>Optional. Mit MQTT Discovery erscheinen die Sensoren automatisch in Home Assistant.</p>"
            "<div class='inline'><div><label>MQTT-Server</label><input name='mqtt_host' value='");
  body += htmlEscape(config.mqttHost);
  body += F("' placeholder='192.168.178.10'></div><div><label>Port</label><input type='number' name='mqtt_port' value='");
  body += String(config.mqttPort);
  body += F("'></div></div><div class='inline'><div><label>Benutzer</label><input name='mqtt_user' value='");
  body += htmlEscape(config.mqttUser);
  body += F("'></div><div><label>Passwort</label><input type='password' name='mqtt_pass' placeholder='");
  body += config.mqttPassword.length() ? "gespeichert" : "optional";
  body += F("'></div></div><label><input style='width:auto' type='checkbox' name='ha_disc' value='1'");
  if (config.homeAssistantDiscovery) body += " checked";
  body += F("> Home-Assistant-Discovery aktivieren</label></fieldset>"
            "<fieldset><legend>Energiesparen</legend>"
            "<label><input style='width:auto' type='checkbox' name='eco_mode' value='1'");
  if (config.ecoMode) body += " checked";
  body += F("> Eco-Modus aktivieren</label>"
            "<p class='muted'>Standardmäßig aktiv: 80 MHz im Messbetrieb. "
            "Vollständiger Export, Import und Firmwareupdate schalten automatisch "
            "auf 160 MHz. Zwei Minuten nach der letzten rechenintensiven Aufgabe wird "
            "wieder auf 80 MHz reduziert.</p>"
            "<label><input style='width:auto' type='checkbox' name='eco_led_off' value='1'");
  if (config.ecoLedOff) body += " checked";
  body += F("> Status-LED bei fehlerfreiem Eco-Betrieb ausschalten</label>"
            "<p class='muted'>Wirkt nur bei aktivem Eco-Modus. Bei fehlendem oder "
            "veraltertem Zählerwert, WLAN-Ausfall, Speicherwarnung oder internem "
            "Eco-Fehler bleibt die LED-Warnanzeige automatisch aktiv.</p>"
            "<label><input style='width:auto' type='checkbox' name='wifi_power_auto' value='1'");
  if (config.adaptiveWifiPower) body += " checked";
  body += F("> WLAN-Sendeleistung im Eco-Modus automatisch anpassen</label>"
            "<p class='muted'>Start, Setup-Hotspot und Wiederverbindung verwenden immer "
            "19,5 dBm. Nach drei stabilen Minuten wird bei gutem Signal vorsichtig auf "
            "15 oder 11 dBm reduziert. Bei schwachem Signal oder Abbruch wird automatisch "
            "volle Leistung verwendet.</p></fieldset>"
            "<fieldset><legend>Firmwareupdates</legend>"
            "<label><input style='width:auto' type='checkbox' name='gh_check' value='1'");
  if (config.githubUpdateCheck) body += " checked";
  body += F("> Täglich auf signierte GitHub-Firmware prüfen</label>"
            "<label><input style='width:auto' type='checkbox' name='gh_auto' value='1'");
  if (config.githubAutoInstall) body += " checked";
  body += F("> Neue signierte Firmware automatisch installieren</label>"
            "<p class='muted'>Die automatische Installation ist standardmäßig aus. "
            "Akzeptiert werden nur neuere, von Michael Roßmann kryptografisch signierte "
            "IRFW-Pakete aus dem offiziellen GitHub-Release.</p></fieldset>"
            "<button type='submit'>Alle Einstellungen speichern</button></form>"
            "<form method='post' action='/auth/logout'><button class='secondary' type='submit'>"
            "Diesen Browser abmelden</button></form>");
  server.send(200, "text/html; charset=utf-8", page("Einstellungen", body));
}

void handleSetupSave() {
  if (!requireAdmin()) return;
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    if (!safeSingleLine(server.arg("ssid" + String(i)), 32) ||
        !validWifiPassword(server.arg("pass" + String(i)))) {
      server.send(400, "application/json",
                  "{\"error\":\"invalid_wifi_credentials\"}");
      return;
    }
  }
  String requestedHostname = server.arg("hostname");
  requestedHostname.trim();
  if (!validHostname(requestedHostname) ||
      !safeSingleLine(server.arg("timezone"), 80) ||
      !safeSingleLine(server.arg("mqtt_host"), 253) ||
      !safeSingleLine(server.arg("mqtt_user"), 128) ||
      !safeSingleLine(server.arg("mqtt_pass"), 256)) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_settings_text\"}");
    return;
  }
  const String requestedAdminPassword = server.arg("admin_pass");
  const String requestedAdminPasswordConfirm =
      server.arg("admin_pass_confirm");
  if (requestedAdminPassword.length() > 64) {
    server.send(400, "application/json",
                "{\"error\":\"admin_password_too_long\"}");
    return;
  }
  if (requestedAdminPassword != requestedAdminPasswordConfirm) {
    server.send(400, "application/json",
                "{\"error\":\"admin_password_confirmation_mismatch\"}");
    return;
  }
  for (uint8_t i = 0; i < kWifiSlots; ++i) {
    String newSsid = server.arg("ssid" + String(i));
    String newPassword = server.arg("pass" + String(i));
    newSsid.trim();
    if (newPassword.length() || newSsid != config.ssid[i]) config.password[i] = newPassword;
    config.ssid[i] = newSsid;
  }
  config.hostname = requestedHostname;
  String timezone = server.arg("timezone");
  timezone.trim();
  if (timezone.length() && timezone.length() <= 80)
    config.timezone = timezone;
  config.setupApMinutes =
      constrain(server.arg("ap_minutes").toInt(), 5, 60);
  const String newAdminPassword = requestedAdminPassword;
  if (newAdminPassword.length() && newAdminPassword.length() < 4) {
    server.send(400, "application/json",
                "{\"error\":\"admin_password_too_short\"}");
    return;
  }
  if (newAdminPassword.length() >= 4 && newAdminPassword.length() <= 64)
    config.adminPassword = newAdminPassword;
  config.rxPin = constrain(server.arg("rx_pin").toInt(), 0, 10);
  config.txPin = constrain(server.arg("tx_pin").toInt(), -1, 10);
  config.snifferEnabled = server.hasArg("sniffer");
  config.bridgeEnabled = server.hasArg("bridge");
  config.apiAccess = constrain(server.arg("api_access").toInt(), 0, 2);
  const bool previousEventPersistence = config.persistEventLog;
  config.persistEventLog = server.hasArg("event_flash");
  config.ecoMode = server.hasArg("eco_mode");
  config.ecoLedOff = server.hasArg("eco_led_off");
  config.adaptiveWifiPower = server.hasArg("wifi_power_auto");
  config.githubUpdateCheck = server.hasArg("gh_check");
  config.githubAutoInstall = server.hasArg("gh_auto");
  if (config.githubAutoInstall) config.githubUpdateCheck = true;
  config.ledPin = constrain(server.arg("led_pin").toInt(), -1, 10);
  config.ledInverted = server.hasArg("led_inv");
  config.baud = server.arg("baud").toInt();
  config.mqttHost = server.arg("mqtt_host");
  config.mqttHost.trim();
  config.mqttPort = constrain(server.arg("mqtt_port").toInt(), 1, 65535);
  config.mqttUser = server.arg("mqtt_user");
  String newMqttPassword = server.arg("mqtt_pass");
  if (newMqttPassword.length() || !config.mqttHost.length()) config.mqttPassword = newMqttPassword;
  config.homeAssistantDiscovery = server.hasArg("ha_disc");
  saveConfig();
  if (previousEventPersistence != config.persistEventLog &&
      !eventLog.setPersistence(config.persistEventLog)) {
    config.persistEventLog = previousEventPersistence;
    saveConfig();
    server.send(500, "application/json",
                "{\"error\":\"event_log_persistence_change_failed\"}");
    return;
  }
  eventLog.add("INFO", "SETTINGS_SAVE", "Einstellungen gespeichert");
  server.send(200, "text/html; charset=utf-8",
              page("Gespeichert", "<p>Der Tracker startet jetzt neu.</p>"));
  delay(750);
  ESP.restart();
}

void handleLogout() {
  if (!requireAdmin()) return;
  server.sendHeader(
      "Set-Cookie",
      "ir_session=deleted; Max-Age=0; Path=/; HttpOnly; SameSite=Strict",
      false);
  server.sendHeader("Clear-Site-Data", "\"cookies\"");
  server.send(
      200, "text/html; charset=utf-8",
      page("Abgemeldet",
           "<div class='card'><p>Die 60-Tage-Sitzung dieses Browsers wurde "
           "gelöscht.</p><p>Falls der Browser die HTTP-Basisanmeldung selbst "
           "gespeichert hat, kann sie zusätzlich in dessen Passwortverwaltung "
           "entfernt werden.</p><a href='/'>Erneut anmelden</a></div>"));
}

void handleDiagnostics() {
  if (!requireAdmin()) return;
  String body = maintenanceTabs(true);
  body += F("<div class='grid'><div class='card'><h2>Geführter Selbsttest</h2>"
                  "<button id='runSelftest' type='button'>Selbsttest ausführen</button>"
                  "<div id='selftest' class='stats'></div></div>"
                  "<div class='card'><h2>Zählerbericht</h2>"
                  "<p><a href='/api/v1/meter-report'>Detailliert anzeigen: empfangene und fehlende OBIS-Werte</a></p>"
                  "<p class='muted'>Spannung und Strom werden nur live im RAM gehalten, niemals in der Flash-Historie.</p>"
                  "<details class='compact-details'><summary>Technische Details</summary>"
                  "<p><a href='/api/v1/memory-info'>Speicherinformationen</a></p>"
                  "<p><a href='/api/v1/obis'>Alle erkannten OBIS-Werte</a></p>"
                  "<p><a href='/api/v1/raw'>Letztes SML-Telegramm (Hex)</a></p>"
                  "<p>IR-Sniffer WebSocket: <code>ws://GERAET:81/</code></p>"
                  "<p>IR-Bridge WebSocket: <code>ws://GERAET:82/</code></p>"
                  "<p class='muted'>Die Bridge ist nur aktiv, wenn ein TX-GPIO eingestellt wurde.</p>"
                  "</details></div></div>"
                  "<details class='card compact-details'><summary>Optionale IR-Freischaltung (experimentell)</summary>"
                  "<p>Nur für Zähler, die diese optische Impulsfolge unterstützen. Sendet automatisch Initialisierung, "
                  "gespeicherte PIN, Navigation zu Inf und den langen Impuls für Inf ON. "
                  "Danach prüft der Tracker bis zu 90 Sekunden auf Momentanleistung.</p>"
                  "<form method='post' action='/ir/meter-unlock' "
                  "onsubmit=\"return confirm('Optionale IR-Freischaltung jetzt starten? Der Tracker darf dabei nicht bewegt werden.')\">"
                  "<button type='submit'>Zähler mit gespeicherter PIN freischalten</button></form>"
                  "<p class='muted'>Vorher unten die PIN einmal lokal speichern. Dauer ungefähr eine Minute.</p></details>"
                  "<details class='card compact-details'><summary>Stromzähler-PIN über IR</summary>"
                  "<p>Die vierstellige PIN wird nur im Arbeitsspeicher verarbeitet und nicht gespeichert. "
                  "Am Zähler zuerst die PIN-Anzeige aktivieren. Eine Ziffer 0 wird als zehn Lichtimpulse gesendet.</p>"
                  "<form method='post' action='/ir/pin'>"
                  "<label>Vierstellige PIN</label><input name='pin' type='password' inputmode='numeric' "
                  "pattern='[0-9]{4}' minlength='4' maxlength='4' autocomplete='off' placeholder='");
  body += config.meterPin.length() ? "gespeichert - leer lassen zum Verwenden" : "vier Ziffern";
  body += F("'>"
                  "<div class='inline'><div><label>Impulsdauer (ms)</label>"
                  "<input name='pulse_ms' type='number' min='50' max='1000' value='");
  body += String(config.pinPulseMs);
  body += F("'></div>"
                  "<div><label>Ziffernpause (ms)</label>"
                  "<input name='digit_gap_ms' type='number' min='1000' max='10000' value='");
  body += String(config.pinDigitGapMs);
  body += F("'></div></div><label><input style='width:auto' type='checkbox' name='invert' value='1'");
  if (config.pinInverted) body += " checked";
  body += F("> IR-Ausgang invertieren</label>"
                  "<label><input style='width:auto' type='checkbox' name='save_pin' value='1'> PIN lokal speichern</label>"
                  "<p class='muted'>Unterstützt der Zähler keine optische PIN-Steuerung, erfolgt die Freischaltung mit Zählertaste oder Taschenlampe.</p>"
                  "<p class='muted'>Die automatische PIN-Eingabe wird nicht von jedem Zähler oder optischen Lesekopf unterstützt. "
                  "Bei fehlender Displayreaktion PIN und Inf-Freigabe mit Zählertaste oder Taschenlampe durchführen.<br><br>"
                  "Das Speichern ist optional. Die PIN liegt lokal im Gerätespeicher; "
                  "ohne aktivierte ESP32-Flash-Verschlüsselung ist sie nicht kryptografisch gegen "
                  "einen direkten Hardwarezugriff geschützt. Die Automatik sendet höchstens einmal "
                  "pro Neustart und bleibt aus, sobald der Leistungswert vorhanden ist.</p>"
                  "<button type='submit'>PIN-Impulsfolge starten</button></form>"
                  "<form method='post' action='/ir/pin/forget'><button class='danger' type='submit'>"
                  "Gespeicherte PIN und Automatik löschen</button></form>"
                  "<form method='post' action='/ir/pulse'><input type='hidden' name='count' value='1'>"
                  "<button type='submit'>Einzelnen Testimpuls senden</button></form>"
                  "<form method='post' action='/ir/stop'><button class='danger' type='submit'>IR-Sendung stoppen</button></form>"
                  "<p class='muted'>Die genaue Bedienfolge ist vom Zählermodell abhängig. Während der Impulsfolge "
                  "pausiert der SML-Empfang kurzzeitig.</p></details>");
  const String script = F(
      "const out=document.getElementById('selftest');"
      "async function test(){out.innerHTML='<div class=\"loading\"><span class=\"spinner\"></span>Prüfung läuft …</div>';"
      "try{const r=await fetch('/api/v1/selftest',{cache:'no-store'});if(!r.ok)throw Error(r.status);"
      "const j=await r.json(),c={ok:'#63e68b',warn:'#ffb454',error:'#ff8d69',off:'#9bb3a4'};"
      "out.innerHTML=j.tests.map(t=>`<div class='stat' style='border-color:${c[t.state]}'><span style='color:${c[t.state]};font-weight:700'>${t.state==='ok'?'OK':t.state==='warn'?'HINWEIS':t.state==='error'?'FEHLER':'OPTIONAL'}</span><strong>${t.label}</strong><small>${t.detail}</small></div>`).join('')}"
      "catch(e){out.innerHTML='<div class=\"error\">Selbsttest konnte nicht geladen werden. Verbindung zum Tracker prüfen.</div>'}}"
      "document.getElementById('runSelftest').onclick=test;test();");
  server.send(200, "text/html; charset=utf-8",
              page("Wartung – Diagnose", body, script));
}

void setIrPulseOutput(bool active) {
  if (irPulse.pin < 0) return;
  digitalWrite(irPulse.pin, active ^ irPulse.inverted);
  irPulse.outputActive = active;
}

void finishIrPulseJob() {
  const int8_t pulsePin = irPulse.pin;
  if (pulsePin >= 0) {
    setIrPulseOutput(false);
    delay(2);
    if (pulsePin != config.txPin) pinMode(pulsePin, INPUT);
  }
  irPulse.active = false;
  irPulse.outputActive = false;
  meterSerial.begin(config.baud, SERIAL_8N1, config.rxPin, config.txPin);
  if (config.ledPin >= 0) {
    pinMode(config.ledPin, OUTPUT);
    digitalWrite(config.ledPin, config.ledInverted);
  }
}

bool beginIrPulseJob(const uint8_t digits[4], uint16_t pulseMs,
                     uint16_t digitGapMs, bool inverted,
                     int8_t outputPin = -1) {
  const int8_t selectedPin = outputPin >= 0 ? outputPin : config.txPin;
  if (selectedPin < 0 || selectedPin > 10 || irPulse.active ||
      gpioScan.active || selectedPin == config.rxPin)
    return false;
  meterSerial.end();
  pinMode(selectedPin, OUTPUT);
  irPulse = {};
  irPulse.active = true;
  irPulse.pin = selectedPin;
  irPulse.inverted = inverted;
  irPulse.pulseMs = pulseMs;
  irPulse.pulseGapMs = pulseMs;
  irPulse.digitGapMs = digitGapMs;
  memcpy(irPulse.digits, digits, sizeof(irPulse.digits));
  irPulse.pulsesRemaining = irPulse.digits[0] ? irPulse.digits[0] : 10;
  setIrPulseOutput(false);
  irPulse.nextChangeMs = millis() + 250;
  return true;
}

bool beginIrPulseCount(uint8_t count, uint16_t pulseMs, bool inverted) {
  if (!count || config.txPin < 0 || irPulse.active) return false;
  meterSerial.end();
  pinMode(config.txPin, OUTPUT);
  irPulse = {};
  irPulse.active = true;
  irPulse.pin = config.txPin;
  irPulse.inverted = inverted;
  irPulse.pulseMs = pulseMs;
  irPulse.pulseGapMs = pulseMs >= 2000 ? 300 : pulseMs;
  irPulse.digitGapMs = 500;
  irPulse.digits[0] = count;
  irPulse.digits[1] = irPulse.digits[2] = irPulse.digits[3] = 0xff;
  irPulse.pulsesRemaining = count;
  setIrPulseOutput(false);
  irPulse.nextChangeMs = millis() + 250;
  return true;
}

void handleApatorUnlock() {
  if (!requireAdmin()) return;
  if (apatorUnlock.active || irPulse.active) {
    server.send(409, "application/json",
                "{\"error\":\"ir_sequence_already_running\"}");
    return;
  }
  if (config.meterPin.length() != 4 || config.txPin < 0) {
    server.send(400, "application/json",
                "{\"error\":\"saved_pin_or_ir_output_missing\"}");
    return;
  }
  apatorUnlock = {};
  apatorUnlock.active = true;
  apatorUnlock.phase = 1;
  apatorUnlock.nextMs = millis();
  eventLog.add("INFO", "METER_UNLOCK",
               "Optionale Zählerfreischaltung gestartet");
  server.send(202, "text/html; charset=utf-8",
              page("Zähler wird freigeschaltet",
                   "<div class='card'><h2>Automatische IR-Sequenz läuft</h2>"
                   "<p>Dauer ungefähr eine Minute. Tracker und Zähler währenddessen nicht bewegen.</p>"
                   "<p>Anschließend unter Messwerte prüfen, ob die aktuelle Leistung erscheint.</p>"
                   "<a href='/maintenance/diagnostics'>Status anzeigen</a></div>"));
}

void updateApatorUnlock() {
  if (!apatorUnlock.active || irPulse.active ||
      static_cast<int32_t>(millis() - apatorUnlock.nextMs) < 0)
    return;
  if (apatorUnlock.phase == 1) {
    // DE: Apator: Zwei kurze Impulse öffnen PIN-Menü/LCD-Test. | EN: Apator: two short flashes open the PIN menu/LCD test.
    if (beginIrPulseCount(2, 300, config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "1/4 Initialisierung: 2 kurze Impulse gesendet");
      apatorUnlock.phase = 2;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 2) {
    apatorUnlock.phase = 3;
    apatorUnlock.nextMs = millis() + 5000;
    return;
  }
  if (apatorUnlock.phase == 3) {
    uint8_t digits[4];
    for (uint8_t i = 0; i < 4; ++i) digits[i] = config.meterPin[i] - '0';
    if (beginIrPulseJob(digits, config.pinPulseMs, config.pinDigitGapMs,
                        config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "2/4 Gespeicherte PIN als Impulsfolge gesendet");
      apatorUnlock.phase = 4;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 4) {
    apatorUnlock.phase = 5;
    apatorUnlock.nextMs = millis() + 3500;
    return;
  }
  if (apatorUnlock.phase == 5) {
    // DE: Apator-Zweirichtungszähler: vom PIN-Ergebnis zu Inf OFF/ON. | EN: Two-way Apator meter: advance from PIN result to Inf OFF/ON.
    if (beginIrPulseCount(13, 300, config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "3/4 Navigation: 13 kurze Impulse gesendet");
      apatorUnlock.phase = 6;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 6) {
    apatorUnlock.phase = 7;
    apatorUnlock.nextMs = millis() + 800;
    return;
  }
  if (apatorUnlock.phase == 7) {
    // DE: Langer Impuls schaltet Inf von OFF auf ON. | EN: A long flash toggles Inf from OFF to ON.
    if (beginIrPulseCount(1, 5500, config.pinInverted)) {
      eventLog.add("INFO", "APATOR_STEP",
                   "4/4 Inf-Umschaltung: langer Impuls gesendet");
      apatorUnlock.phase = 8;
      apatorUnlock.nextMs = 0;
    }
    return;
  }
  if (apatorUnlock.phase == 8) {
    if (!apatorUnlock.verifyUntilMs) {
      apatorUnlock.verifyUntilMs = millis() + 90000;
      apatorUnlock.nextMs = millis() + 2000;
      return;
    }
    if (std::isfinite(meter.powerW)) {
      eventLog.add("INFO", "APATOR_UNLOCK_OK",
                   "Inf ON bestätigt: Momentanleistung empfangen");
      apatorUnlock.active = false;
    } else if (static_cast<int32_t>(millis() -
                                    apatorUnlock.verifyUntilMs) >= 0) {
      eventLog.add("ERROR", "APATOR_UNLOCK_FAIL",
                   "Kein 16.7.0: Zähler bestätigt Lichtimpulse nicht über SML; Display/IR-TX prüfen");
      apatorUnlock.active = false;
    } else {
      apatorUnlock.nextMs = millis() + 2000;
    }
  }
}

void handleIrPin() {
  if (!requireAdmin()) return;
  String pin = server.arg("pin");
  if (!pin.length()) pin = config.meterPin;
  if (pin.length() != 4) {
    server.send(400, "application/json", "{\"error\":\"pin_must_have_4_digits\"}");
    return;
  }
  uint8_t digits[4];
  for (uint8_t i = 0; i < 4; ++i) {
    if (!isDigit(pin[i])) {
      server.send(400, "application/json", "{\"error\":\"pin_must_be_numeric\"}");
      return;
    }
    digits[i] = pin[i] - '0';
  }
  const uint16_t pulseMs = constrain(server.arg("pulse_ms").toInt(), 50, 1000);
  const uint16_t digitGapMs = constrain(server.arg("digit_gap_ms").toInt(), 1000, 10000);
  const bool inverted = server.hasArg("invert");
  if (server.hasArg("save_pin")) config.meterPin = pin;
  config.autoPin = false;
  config.pinInverted = inverted;
  config.pinPulseMs = pulseMs;
  config.pinDigitGapMs = digitGapMs;
  saveConfig();
  eventLog.add("INFO", "PIN_SEND",
               server.hasArg("save_pin") ? "PIN gesendet und gespeichert"
                                         : "PIN gesendet");
  if (!beginIrPulseJob(digits, pulseMs, digitGapMs, inverted)) {
    server.send(409, "application/json", "{\"error\":\"ir_busy_or_disabled\"}");
    return;
  }
  server.send(202, "text/html; charset=utf-8",
              page("IR-PIN wird gesendet",
                   "<p>Die vier Ziffern werden jetzt nicht blockierend gesendet.</p>"
                   "<p><a href='/maintenance/diagnostics'>Zur Diagnose</a></p>"));
}

void handleForgetPin() {
  if (!requireAdmin()) return;
  config.meterPin = "";
  config.autoPin = false;
  saveConfig();
  eventLog.add("INFO", "PIN_FORGET", "Gespeicherte PIN gelöscht");
  server.sendHeader("Location", "/maintenance/diagnostics", true);
  server.send(303, "text/plain", "");
}

void handleIrPulse() {
  if (!requireAdmin()) return;
  uint8_t digits[4] = {1, 0, 0, 0};
  if (!beginIrPulseJob(digits, 300, 1000, server.hasArg("invert"))) {
    server.send(409, "application/json", "{\"error\":\"ir_busy_or_disabled\"}");
    return;
  }
  // DE: Einzelimpulstest endet nach der ersten Ziffer. | EN: A single-pulse test ends after its first digit.
  irPulse.digits[1] = irPulse.digits[2] = irPulse.digits[3] = 0xff;
  server.send(202, "application/json", "{\"accepted\":true,\"pulses\":1}");
}

void handleGpioOutputTest() {
  if (!requireAdmin()) return;
  const int pin = server.arg("pin").toInt();
  if (!server.hasArg("pin") || pin < 0 || pin > 10 || pin == config.rxPin) {
    server.send(400, "application/json",
                "{\"error\":\"invalid_or_rx_gpio\"}");
    return;
  }
  const uint8_t digits[4] = {1, 0xff, 0xff, 0xff};
  if (!beginIrPulseJob(digits, 350, 700, server.arg("inverted") == "1",
                       static_cast<int8_t>(pin))) {
    server.send(409, "application/json",
                "{\"error\":\"gpio_output_test_busy\"}");
    return;
  }
  eventLog.add("INFO", "GPIO_TX_TEST",
               "Kurzer Ausgangstest auf GPIO " + String(pin));
  server.send(202, "application/json",
              "{\"accepted\":true,\"pin\":" + String(pin) +
                  ",\"duration_ms\":350}");
}

struct DigitalSample {
  uint16_t highPermille = 0;
  uint16_t transitions = 0;
};

DigitalSample sampleDigitalPin(int8_t pin, uint32_t durationUs = 50000) {
  DigitalSample result;
  uint32_t high = 0;
  uint32_t total = 0;
  int previous = digitalRead(pin);
  const uint32_t started = micros();
  while (static_cast<int32_t>(micros() - started) <
         static_cast<int32_t>(durationUs)) {
    const int current = digitalRead(pin);
    high += current == HIGH;
    ++total;
    if (current != previous) {
      ++result.transitions;
      previous = current;
    }
    delayMicroseconds(80);
    if ((total & 63U) == 0) {
      esp_task_wdt_reset();
      yield();
    }
  }
  if (total) result.highPermille = high * 1000U / total;
  return result;
}

void handleGpioTxScan() {
  if (!requireAdmin()) return;
  const int rx = server.arg("rx").toInt();
  if (!server.hasArg("rx") || rx < 0 || rx > 10 || gpioScan.active ||
      irPulse.active) {
    server.send(400, "application/json", "{\"error\":\"invalid_rx_or_busy\"}");
    return;
  }
  requestCpuBoost("gpio_tx_scan");
  meterSerial.end();
  resetSmlCapture();
  pinMode(rx, INPUT);
  int8_t pins[10] = {};
  uint8_t pinCount = 0;
  if (config.txPin >= 0 && config.txPin <= 10 && config.txPin != rx)
    pins[pinCount++] = config.txPin;
  for (int8_t pin = 0; pin <= 10; ++pin)
    if (pin != rx && pin != config.txPin) pins[pinCount++] = pin;

  int8_t foundPin = -1;
  bool foundInverted = false;
  uint8_t confidence = 0;
  uint8_t tested = 0;
  uint16_t foundActiveTransitions = 0;
  uint16_t foundIdleTransitions = 0;
  for (uint8_t index = 0; index < pinCount; ++index) {
    const int8_t pin = pins[index];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(20);
    const DigitalSample low1 = sampleDigitalPin(rx);
    digitalWrite(pin, HIGH);
    delay(20);
    const DigitalSample high1 = sampleDigitalPin(rx);
    digitalWrite(pin, LOW);
    delay(20);
    const DigitalSample low2 = sampleDigitalPin(rx);
    digitalWrite(pin, HIGH);
    delay(20);
    const DigitalSample high2 = sampleDigitalPin(rx);
    digitalWrite(pin, LOW);
    pinMode(pin, INPUT);
    ++tested;

    const uint16_t lowTransitions =
        (low1.transitions + low2.transitions) / 2U;
    const uint16_t highTransitions =
        (high1.transitions + high2.transitions) / 2U;
    const uint16_t lowDuty = (low1.highPermille + low2.highPermille) / 2U;
    const uint16_t highDuty =
        (high1.highPermille + high2.highPermille) / 2U;
    const bool activeLow = lowTransitions < highTransitions;
    const uint16_t activeTransitions =
        activeLow ? lowTransitions : highTransitions;
    const uint16_t idleTransitions =
        activeLow ? highTransitions : lowTransitions;
    const uint16_t activeDuty = activeLow ? lowDuty : highDuty;
    const bool saturated = activeDuty <= 120U || activeDuty >= 880U;
    const bool correlated = idleTransitions >= 12U &&
                            idleTransitions >= activeTransitions * 4U + 8U;
    if (saturated && correlated) {
      foundPin = pin;
      foundInverted = activeLow;
      foundActiveTransitions = activeTransitions;
      foundIdleTransitions = idleTransitions;
      const uint16_t transitionScore = std::min<uint16_t>(
          70U, (idleTransitions - activeTransitions) * 2U);
      const uint16_t saturationDistance =
          std::min<uint16_t>(activeDuty, 1000U - activeDuty);
      confidence = std::min<uint16_t>(
          100U, 30U + transitionScore +
                    (saturationDistance <= 50U ? 10U : 0U));
      break;
    }
  }
  restoreMeterSerialAfterScan();
  if (foundPin < 0) {
    eventLog.add("WARN", "GPIO_TX_SCAN",
                 "Kein eindeutiger optischer TX-Rueckkanal erkannt");
    server.send(200, "application/json",
                "{\"complete\":true,\"found\":false,\"tested\":" +
                    String(tested) +
                    ",\"error\":\"no_optical_loopback\"}");
    return;
  }
  eventLog.add("INFO", "GPIO_TX_SCAN",
               "IR-Sender durch wiederholte optische RX-Korrelation auf GPIO " +
                   String(foundPin) + " erkannt");
  server.send(200, "application/json",
              "{\"complete\":true,\"found\":true,\"pin\":" +
                  String(foundPin) + ",\"inverted\":" +
                  String(foundInverted ? "true" : "false") +
                  ",\"confidence\":" + String(confidence) +
                  ",\"tested\":" + String(tested) +
                  ",\"active_transitions\":" +
                  String(foundActiveTransitions) +
                  ",\"idle_transitions\":" +
                  String(foundIdleTransitions) + "}");
}

void handleIrStop() {
  if (!requireAdmin()) return;
  if (irPulse.active) finishIrPulseJob();
  server.sendHeader("Location", "/maintenance/diagnostics", true);
  server.send(303, "text/plain", "");
}

void updateIrPulseJob() {
  if (!irPulse.active || static_cast<int32_t>(millis() - irPulse.nextChangeMs) < 0) return;
  if (!irPulse.outputActive && irPulse.pulsesRemaining) {
    setIrPulseOutput(true);
    --irPulse.pulsesRemaining;
    irPulse.nextChangeMs = millis() + irPulse.pulseMs;
    return;
  }
  if (irPulse.outputActive) {
    setIrPulseOutput(false);
    irPulse.nextChangeMs = millis() + irPulse.pulseGapMs;
    return;
  }
  ++irPulse.digitIndex;
  if (irPulse.digitIndex >= 4 || irPulse.digits[irPulse.digitIndex] == 0xff) {
    finishIrPulseJob();
    return;
  }
  irPulse.pulsesRemaining =
      irPulse.digits[irPulse.digitIndex] ? irPulse.digits[irPulse.digitIndex] : 10;
  irPulse.nextChangeMs = millis() + irPulse.digitGapMs;
}

void manageAutoPin() {
  if (autoPinAttempted || !config.autoPin || config.meterPin.length() != 4 ||
      irPulse.active || apatorUnlock.active || millis() < 60000 ||
      meter.telegrams < 2 ||
      std::isfinite(meter.powerW)) {
    return;
  }
  uint8_t digits[4];
  for (uint8_t i = 0; i < 4; ++i) digits[i] = config.meterPin[i] - '0';
  autoPinAttempted = true;
  beginIrPulseJob(digits, config.pinPulseMs, config.pinDigitGapMs,
                  config.pinInverted);
}

void startAccessPoint() {
  if (accessPointMode || !accessPointAllowed) return;
  forceFullWifiPower();
  wifiMinModemSleepActive = false;
  WiFi.persistent(false);
  if (!WiFi.mode(WIFI_AP_STA)) {
    ++wifiModeErrors;
    return;
  }
  const uint64_t chip = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned>(chip & 0xffff));
  String apName = "IR-Tracker-Setup-" + String(suffix);
  String apPassword = localAdminPassword();
  if (!WiFi.softAP(apName.c_str(), apPassword.c_str())) {
    ++wifiModeErrors;
    return;
  }
  accessPointMode = true;
  accessPointStartedMs = millis();
  Serial.printf("Setup AP started: %s\n", apName.c_str());
  dns.start(53, "*", WiFi.softAPIP());
}

void stopAccessPoint() {
  if (!accessPointMode) return;
  dns.stop();
  if (!WiFi.softAPdisconnect(true)) ++wifiModeErrors;
  if (!WiFi.mode(WIFI_STA)) ++wifiModeErrors;
  wifiMinModemSleepActive =
      esp_wifi_set_ps(WIFI_PS_MIN_MODEM) == ESP_OK;
  if (!wifiMinModemSleepActive) ++wifiModeErrors;
  accessPointMode = false;
  accessPointStartedMs = 0;
}

bool beginNextKnownWifi() {
  while (wifiTried < kWifiSlots) {
    const uint8_t slot = wifiCandidate;
    wifiCandidate = (wifiCandidate + 1) % kWifiSlots;
    ++wifiTried;
    if (!config.ssid[slot].length()) continue;
    WiFi.begin(config.ssid[slot].c_str(), config.password[slot].c_str());
    wifiCandidateStartedMs = millis();
    Serial.printf("Trying Wi-Fi slot %u: %s\n", slot + 1, config.ssid[slot].c_str());
    return true;
  }
  wifiCandidateStartedMs = 0;
  lastWifiAttemptMs = millis();
  return false;
}

void manageWifi() {
  static bool wasConnected = false;
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) {
    if (!wasConnected) {
      stopAccessPoint();
      wifiConnectedSinceMs = millis();
      lastWifiPowerEvaluateMs = millis();
      forceFullWifiPower();
      accessPointAllowed = true;
      if (!ntpConfigured) {
        configTzTime(config.timezone.c_str(), "fritz.box",
                     "pool.ntp.org", "time.cloudflare.com");
        ntpConfigured = true;
      }
      if (!mdnsRunning && MDNS.begin(config.hostname.c_str())) {
        MDNS.addService("http", "tcp", 80);
        mdnsRunning = true;
        eventLog.add("INFO", "MDNS_STARTED",
                     "Tracker erreichbar als " + config.hostname + ".local");
      } else if (!mdnsRunning) {
        eventLog.add("WARN", "MDNS_FAILED",
                     "mDNS konnte nicht gestartet werden");
      }
      Serial.printf("Wi-Fi connected: %s, %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      eventLog.add("INFO", "WIFI_CONNECTED",
                   WiFi.SSID() + " " + WiFi.localIP().toString());
    }
    wifiCandidateStartedMs = 0;
  } else {
    if (wasConnected) {
      forceFullWifiPower();
      wifiConnectedSinceMs = 0;
      lastWifiPowerEvaluateMs = 0;
      wifiMinModemSleepActive = false;
      eventLog.add("WARN", "WIFI_LOST", "WLAN-Verbindung verloren");
      if (mdnsRunning) {
        MDNS.end();
        mdnsRunning = false;
      }
      wifiTried = 0;
      wifiCandidate = 0;
      wifiCandidateStartedMs = 0;
    }
    startAccessPoint();
    if (accessPointMode && accessPointStartedMs &&
        millis() - accessPointStartedMs >=
            static_cast<uint32_t>(config.setupApMinutes) * 60000UL) {
      eventLog.add("INFO", "SETUP_AP_TIMEOUT",
                   "Setup-Hotspot nach Zeitlimit abgeschaltet");
      accessPointAllowed = false;
      stopAccessPoint();
    }
    if (wifiCandidateStartedMs && millis() - wifiCandidateStartedMs >= kWifiPerNetworkMs) {
      beginNextKnownWifi();
    } else if (!wifiCandidateStartedMs &&
               (!lastWifiAttemptMs || millis() - lastWifiAttemptMs >= kWifiRetryMs)) {
      wifiTried = 0;
      beginNextKnownWifi();
    }
  }
  wasConnected = connected;
}

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
  mqtt.publish((root + "/network/$properties").c_str(), "rssi,ssid,ip", true);
  mqtt.publish((root + "/network/rssi/$name").c_str(), "WLAN Signal", true);
  mqtt.publish((root + "/network/rssi/$datatype").c_str(), "integer", true);
  mqtt.publish((root + "/network/rssi/$unit").c_str(), "dBm", true);
  mqtt.publish((root + "/network/ssid/$datatype").c_str(), "string", true);
  mqtt.publish((root + "/network/ip/$datatype").c_str(), "string", true);
  mqtt.publish((root + "/$state").c_str(), "ready", true);
}

void publishMqttValues() {
  const String base = mqttBaseTopic();
  const String homie = "homie/" + deviceId;
  const bool fresh = meter.lastTelegramMs && millis() - meter.lastTelegramMs < kReadingStaleMs;
  mqtt.publish((base + "/state").c_str(), statusJson().c_str(), true);
  if (std::isfinite(meter.powerW)) {
    const String value = String(meter.powerW, 3);
    mqtt.publish((base + "/power_w").c_str(), value.c_str(), true);
    mqtt.publish((homie + "/meter/power").c_str(), value.c_str(), true);
  }
  if (std::isfinite(meter.importKwh)) {
    const String value = String(meter.importKwh, 6);
    mqtt.publish((base + "/import_kwh").c_str(), value.c_str(), true);
    mqtt.publish((homie + "/meter/import").c_str(), value.c_str(), true);
  }
  if (std::isfinite(meter.exportKwh)) {
    const String value = String(meter.exportKwh, 6);
    mqtt.publish((base + "/export_kwh").c_str(), value.c_str(), true);
    mqtt.publish((homie + "/meter/export").c_str(), value.c_str(), true);
  }
  for (uint8_t phase = 0; phase < 3; ++phase) {
    const String root = base + "/phase_l" + String(phase + 1);
    if (std::isfinite(meter.phasePowerW[phase]))
      mqtt.publish((root + "/power_w").c_str(),
                   String(meter.phasePowerW[phase], 3).c_str(), true);
    if (std::isfinite(meter.phaseVoltageV[phase]))
      mqtt.publish((root + "/voltage_v").c_str(),
                   String(meter.phaseVoltageV[phase], 3).c_str(), true);
    if (std::isfinite(meter.phaseCurrentA[phase]))
      mqtt.publish((root + "/current_a").c_str(),
                   String(meter.phaseCurrentA[phase], 4).c_str(), true);
  }
  mqtt.publish((homie + "/meter/fresh").c_str(), fresh ? "true" : "false", true);
  mqtt.publish((homie + "/meter/telegrams").c_str(), String(meter.telegrams).c_str(), true);
  mqtt.publish((homie + "/meter/crc-errors").c_str(), String(meter.crcErrors).c_str(), true);
  mqtt.publish((homie + "/network/rssi").c_str(), String(WiFi.RSSI()).c_str(), true);
  mqtt.publish((homie + "/network/ssid").c_str(), WiFi.SSID().c_str(), true);
  mqtt.publish((homie + "/network/ip").c_str(), WiFi.localIP().toString().c_str(), true);
}

void manageMqtt() {
  if (WiFi.status() != WL_CONNECTED || !config.mqttHost.length()) {
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

void setupRoutes() {
  const char *securityHeaders[] = {"Origin", "Referer", "Authorization",
                                   "X-CSRF-Token", "Cookie"};
  server.collectHeaders(securityHeaders, 5);
  server.on("/assets/common.css", HTTP_GET, [] {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("text/css; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kCommonCssGzip), kCommonCssGzipSize);
  });
  server.on("/assets/common.js", HTTP_GET, [] {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kCommonJsGzip), kCommonJsGzipSize);
  });
  server.on("/assets/i18n.js", HTTP_GET, [] {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kI18nJsGzip), kI18nJsGzipSize);
  });
  server.on("/assets/dashboard.js", HTTP_GET, [] {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kDashboardJsGzip),
                  kDashboardJsGzipSize);
  });
  server.on("/assets/history.js", HTTP_GET, [] {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, PSTR("application/javascript; charset=utf-8"),
                  reinterpret_cast<PGM_P>(kHistoryJsGzip), kHistoryJsGzipSize);
  });
  server.on("/", HTTP_GET, handleRoot);
  server.on("/history", HTTP_GET, handleHistoryPage);
  server.on("/interfaces", HTTP_GET, handleInterfacesPage);
  server.on("/maintenance", HTTP_GET, handleMaintenancePage);
  server.on("/maintenance/diagnostics", HTTP_GET, handleDiagnostics);
  server.on("/setup", HTTP_GET, handleSetup);
  server.on("/setup/save", HTTP_POST, handleSetupSave);
  server.on("/auth/logout", HTTP_POST, handleLogout);
  server.on("/diagnostics", HTTP_GET, [] {
    server.sendHeader("Location", "/maintenance/diagnostics", true);
    server.send(301, "text/plain", "");
  });
  server.on("/api/v1/status", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", statusJson());
  });
  server.on("/api/v1/admin-session", HTTP_GET, [] {
    if (!requireAdmin()) return;
    server.send(200, "application/json",
                "{\"csrf_token\":\"" + csrfToken + "\"}");
  });
  server.on("/api/v1/update/status", HTTP_GET, [] {
    if (!requireAdmin()) return;
    server.send(200, "application/json", githubUpdateJson());
  });
  server.on("/api/v1/update/check", HTTP_POST, [] {
    if (!requireAdmin()) return;
    checkGithubFirmwareUpdate();
    server.sendHeader("Location", "/maintenance#firmware-update", true);
    server.send(303, "text/plain", "");
  });
  server.on("/api/v1/update/install", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (!githubUpdate.available && !checkGithubFirmwareUpdate()) {
      server.sendHeader("Location", "/maintenance#firmware-update", true);
      server.send(303, "text/plain", "");
      return;
    }
    if (!installGithubFirmwareUpdate()) {
      server.sendHeader("Location", "/maintenance#firmware-update", true);
      server.send(303, "text/plain", "");
      return;
    }
    server.send(200, "text/html; charset=utf-8",
                page("Update erfolgreich",
                     "<div class='card'><h2>Signiertes GitHub-Update installiert</h2>"
                     "<p>Der Tracker startet jetzt neu.</p></div>"));
    delay(700);
    ESP.restart();
  });
  server.on("/api/v1/gpio-scan", HTTP_GET, [] {
    if (!requireAdmin()) return;
    server.send(200, "application/json", gpioScanJson());
  });
  server.on("/api/v1/gpio-scan/start", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (gpioScan.active) {
      server.send(409, "application/json", gpioScanJson());
      return;
    }
    startGpioScan();
    server.send(202, "application/json", gpioScanJson());
  });
  server.on("/api/v1/gpio-scan/cancel", HTTP_POST, [] {
    if (!requireAdmin()) return;
    if (gpioScan.active) finishGpioScan(false, "cancelled");
    server.send(200, "application/json", gpioScanJson());
  });
  server.on("/api/v1/gpio-output-test", HTTP_POST,
            handleGpioOutputTest);
  server.on("/api/v1/gpio-scan-tx", HTTP_POST, handleGpioTxScan);
  server.on("/api/v1/selftest", HTTP_GET, [] {
    if (requireAdmin())
      server.send(200, "application/json", selfTestJson());
  });
  server.on("/api/v1/meter-report", HTTP_GET, [] {
    if (requireAdmin())
      server.send(200, "application/json", meterReportJson());
  });
  // DE: Shelly-kompatible, nur lesende Zählerfassade für Speicher. | EN: Shelly-compatible read-only meter facade for storage systems.
  server.on("/status", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyGen1Status());
  });
  server.on("/emeter/0", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyEmStatus());
  });
  server.on("/rpc/EM.GetStatus", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyEmStatus());
  });
  server.on("/rpc/EMData.GetStatus", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", shellyEmStatus());
  });
  server.on("/rpc/Shelly.GetStatus", HTTP_GET, [] {
    if (!requireApiAccess()) return;
    server.send(200, "application/json",
                "{\"sys\":{\"uptime\":" + String(millis() / 1000) +
                    "},\"wifi\":{\"sta_ip\":\"" +
                    WiFi.localIP().toString() + "\",\"rssi\":" +
                    String(WiFi.RSSI()) + "},\"em:0\":" +
                    shellyEmStatus() + "}");
  });
  server.on("/api/v1/memory-info", HTTP_GET, [] {
    if (requireAdmin())
      server.send(200, "application/json", memoryJson());
  });
  server.on("/api/v1/obis", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/json", obisJson());
  });
  server.on("/api/v1/raw", HTTP_GET, [] {
    if (!requireApiAccess()) return;
    server.send(200, "application/json",
                "{\"encoding\":\"hex\",\"length\":" + String(lastTelegram.size()) +
                ",\"data\":\"" + bytesToHex(lastTelegram) + "\"}");
  });
  server.on("/metrics", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "text/plain; version=0.0.4", metricsText());
  });
  server.on("/openmetrics", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "application/openmetrics-text; version=1.0.0; charset=utf-8", metricsText() + "# EOF\n");
  });
  server.on("/api/v1/influx", HTTP_GET, [] {
    if (requireApiAccess())
      server.send(200, "text/plain; charset=utf-8", influxLineProtocol());
  });
  server.on("/api/v1/values.csv", HTTP_GET, [] {
    if (!requireApiAccess()) return;
    server.sendHeader("Content-Disposition", "inline; filename=irtracker-values.csv");
    server.send(200, "text/csv; charset=utf-8", csvValues());
  });
  server.on("/api/v1/history", HTTP_GET, handleHistoryJson);
  server.on("/api/v1/dashboard-summary", HTTP_GET, handleDashboardSummary);
  server.on("/api/v1/history.csv", HTTP_GET, handleHistoryCsv);
  server.on("/api/v1/backup/settings", HTTP_GET, handleSettingsBackup);
  server.on("/api/v1/backup/settings/restore", HTTP_POST,
            handleSettingsRestore);
  server.on("/api/v1/history/import/start", HTTP_POST,
            handleHistoryImportStart);
  server.on("/api/v1/history/import/batch", HTTP_POST,
            handleHistoryImportBatch);
  server.on("/api/v1/history/clear", HTTP_POST, handleHistoryClearAll);
  server.on("/api/v1/events", HTTP_GET, handleEventsJson);
  server.on("/api/v1/events/clear", HTTP_POST, handleEventsClear);
  server.on("/api/v1/time", HTTP_POST, handleSetTime);
  server.on("/ir/pin", HTTP_POST, handleIrPin);
  server.on("/ir/meter-unlock", HTTP_POST, handleApatorUnlock);
  server.on("/ir/apator", HTTP_POST, handleApatorUnlock);
  server.on("/ir/pin/forget", HTTP_POST, handleForgetPin);
  server.on("/ir/pulse", HTTP_POST, handleIrPulse);
  server.on("/ir/stop", HTTP_POST, handleIrStop);
  server.on("/system/update", HTTP_POST, handleOtaFinished,
            handleOtaUpload);
  server.on("/system/shutdown", HTTP_POST, handleSafeShutdown);
  server.onNotFound([] {
    if (accessPointMode) {
      server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/setup", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "application/json", "{\"error\":\"not_found\"}");
    }
  });
  server.begin();
}

void setupWebSockets() {
  if (config.snifferEnabled) snifferSocket.begin();
  if (config.bridgeEnabled) {
    bridgeSocket.begin();
    const String password = localAdminPassword();
    bridgeSocket.setAuthorization("admin", password.c_str());
    bridgeSocket.onEvent(
        [](uint8_t, WStype_t type, uint8_t *payload, size_t length) {
          if ((type == WStype_BIN || type == WStype_TEXT) &&
              config.bridgeEnabled && config.txPin >= 0 && length &&
              length <= 512) {
            meterSerial.write(payload, length);
          }
        });
  }
}

void updateLed() {
  static uint32_t lastToggle = 0;
  static bool state = false;
  if (gpioScan.active) return;
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
  loadConfig();
  const bool historyReady = history.begin();
  eventLog.begin(config.persistEventLog);
  eventLog.add(historyReady ? "INFO" : "ERROR", "BOOT",
               "Firmware " + String(kFirmwareVersion) +
                   (historyReady ? " gestartet" : " ohne Historie gestartet") +
                   ", Ursache: " + bootResetReason);
  char idBuffer[24];
  snprintf(idBuffer, sizeof(idBuffer), "irtracker_%08lx",
           static_cast<unsigned long>(ESP.getEfuseMac() & 0xffffffff));
  deviceId = idBuffer;
  meterSerial.setRxBufferSize(2048);
  meterSerial.begin(config.baud, SERIAL_8N1, config.rxPin, config.txPin);
  if (config.ledPin >= 0) {
    pinMode(config.ledPin, OUTPUT);
    digitalWrite(config.ledPin, config.ledInverted);
  }
  // DE: ESP-IDFs dauerhaften WLAN-Namensraum nicht nutzen; er gehört Solakon. | EN: Do not use ESP-IDF's persistent Wi-Fi namespace; it belongs to Solakon.
  WiFi.persistent(false);
  WiFi.setHostname(config.hostname.c_str());
  startAccessPoint();
  wifiTried = 0;
  beginNextKnownWifi();
  mqtt.setServer(config.mqttHost.c_str(), config.mqttPort);
  mqtt.setBufferSize(768);
  mqtt.setSocketTimeout(1);
  setupRoutes();
  setupWebSockets();
  startCpuPowerMode();
  Serial.printf("Offline firmware %s, partition=%s, RX=GPIO%u @ %lu baud\n",
                kFirmwareVersion, running ? running->label : "?", config.rxPin, config.baud);
  Serial.printf("Open http://%s/\n",
                accessPointMode ? WiFi.softAPIP().toString().c_str() : WiFi.localIP().toString().c_str());
}

void loop() {
  esp_task_wdt_reset();
  manageWifi();
  manageAdaptiveWifiPower();
  manageMqtt();
  manageGithubFirmwareUpdate();
  if (accessPointMode) dns.processNextRequest();
  server.handleClient();
  if (config.snifferEnabled) snifferSocket.loop();
  if (config.bridgeEnabled) bridgeSocket.loop();
  updateIrPulseJob();
  updateApatorUnlock();
  manageAutoPin();
  updateGpioScan();
  uint8_t incoming[128];
  size_t count = 0;
  while (!irPulse.active && meterSerial.available() && count < sizeof(incoming)) {
    incoming[count++] = meterSerial.read();
  }
  if (count) {
    if (config.snifferEnabled) snifferSocket.broadcastBIN(incoming, count);
    for (size_t i = 0; i < count; ++i) consumeMeterByte(incoming[i]);
  }
  updateGpioScan();
  const bool meterFresh =
      meter.lastTelegramMs &&
      millis() - meter.lastTelegramMs < kReadingStaleMs;
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
