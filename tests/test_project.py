from __future__ import annotations

import hashlib
import gzip
import re
import struct
import unittest
from pathlib import Path

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import Prehashed


ROOT = Path(__file__).resolve().parents[1]
MAIN_SOURCE = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")


def expanded_firmware_source() -> str:
    """Mirror the ordered module includes used by the firmware build."""
    pattern = re.compile(r'^#include "(app/[^"]+\.cpp)"$', re.MULTILINE)
    return pattern.sub(
        lambda match: (ROOT / "src" / match.group(1)).read_text(encoding="utf-8"),
        MAIN_SOURCE,
    )


METER_DIR = ROOT / "src" / "app" / "meter"
STANDALONE_MODULE_SOURCE = "\n".join(
    path.read_text(encoding="utf-8")
    for path in (
        METER_DIR / "MeterData.h",
        METER_DIR / "MeterParser.h",
        METER_DIR / "SmlParser.cpp",
        METER_DIR / "D0Parser.cpp",
    )
)
SOURCE = expanded_firmware_source() + "\n" + STANDALONE_MODULE_SOURCE
I18N_SOURCE = (ROOT / "web" / "i18n.js").read_text(encoding="utf-8")
COMMON_JS_SOURCE = (ROOT / "web" / "common.js").read_text(encoding="utf-8")
COMMON_CSS_SOURCE = (ROOT / "web" / "common.css").read_text(encoding="utf-8")
DASHBOARD_JS_SOURCE = (ROOT / "web" / "dashboard.js").read_text(encoding="utf-8")
HISTORY_JS_SOURCE = (ROOT / "web" / "history.js").read_text(encoding="utf-8")
MAINTENANCE_JS_SOURCE = (ROOT / "web" / "maintenance.js").read_text(encoding="utf-8")
DIAGNOSTICS_JS_SOURCE = (ROOT / "web" / "diagnostics.js").read_text(encoding="utf-8")
SETUP_JS_SOURCE = (ROOT / "web" / "setup.js").read_text(encoding="utf-8")
SETUP_HTML_SOURCE = (ROOT / "web" / "setup.html").read_text(encoding="utf-8")
WEB_RUNTIME_SOURCE = "\n".join(
    (SOURCE, COMMON_JS_SOURCE, COMMON_CSS_SOURCE, DASHBOARD_JS_SOURCE,
    HISTORY_JS_SOURCE, MAINTENANCE_JS_SOURCE, DIAGNOSTICS_JS_SOURCE, SETUP_JS_SOURCE,
     SETUP_HTML_SOURCE)
)
HISTORY_SOURCE = (ROOT / "src/app/storage/HistoryStore.cpp").read_text(encoding="utf-8")
HISTORY_HEADER = (ROOT / "src/app/storage/HistoryStore.h").read_text(encoding="utf-8")
DEBUG_STORAGE_SOURCE = (ROOT / "src/app/storage/DebugStorage.cpp").read_text(encoding="utf-8")
DEBUG_STORAGE_HEADER = (ROOT / "src/app/storage/DebugStorage.h").read_text(encoding="utf-8")
EVENT_SOURCE = (ROOT / "src/app/core/EventLog.cpp").read_text(encoding="utf-8")
EVENT_HEADER = (ROOT / "src/app/core/EventLog.h").read_text(encoding="utf-8")
HARDWARE_PROFILE = (ROOT / "src/app/hardware/HardwareProfile.h").read_text(encoding="utf-8")
ETHERNET_SOURCE = (ROOT / "src/app/network/EthernetManager.cpp").read_text(encoding="utf-8")
LEGACY_METER_HEADER = (METER_DIR / "D0Parser.h").read_text(encoding="utf-8")
LEGACY_METER_SOURCE = (METER_DIR / "D0Parser.cpp").read_text(encoding="utf-8")
WEB_ASSET_SCRIPT = (ROOT / "tools/embed_web_assets.py").read_text(encoding="utf-8")
PLATFORMIO = (ROOT / "platformio.ini").read_text(encoding="utf-8")
PARTITIONS = (ROOT / "partitions.csv").read_text(encoding="utf-8")


class ProjectSecurityTests(unittest.TestCase):
    def test_source_tree_and_meter_model_are_consistent(self):
        expected = (
            "app/core/EventLog.cpp",
            "app/hardware/HardwareProfile.h",
            "app/meter/D0Parser.cpp",
            "app/meter/MeterData.h",
            "app/meter/MeterParser.h",
            "app/meter/SmlParser.cpp",
            "app/network/EthernetManager.cpp",
            "app/storage/HistoryStore.cpp",
            "app/storage/DebugStorage.cpp",
        )
        for relative in expected:
            self.assertTrue((ROOT / "src" / relative).is_file(), relative)
        self.assertEqual(
            [path.name for path in (ROOT / "src").iterdir() if path.is_file()],
            ["main.cpp"],
        )
        meter_header = (METER_DIR / "MeterData.h").read_text(encoding="utf-8")
        parser_header = (METER_DIR / "MeterParser.h").read_text(encoding="utf-8")
        sml_header = (METER_DIR / "SmlParser.h").read_text(encoding="utf-8")
        self.assertIn("struct MeterData", meter_header)
        self.assertIn("class MeterParser", parser_header)
        self.assertIn("public MeterParser", sml_header)
        self.assertIn("public MeterParser", LEGACY_METER_HEADER)
        self.assertNotIn("struct MeterValues", SOURCE)

    def test_main_is_split_into_ordered_responsibility_modules(self):
        required = (
            "core/EcoManager.cpp",
            "core/SecurityManager.cpp",
            "meter/MeterManager.cpp",
            "network/MqttManager.cpp",
            "network/NetworkStatus.cpp",
            "update/OtaManager.cpp",
            "web/WebApi.cpp",
            "web/IntegrationApi.cpp",
            "web/EcoTrackerEmulation.cpp",
            "web/ShellyEmulation.cpp",
            "diagnostics/FactoryTest.cpp",
        )
        self.assertLess(len(MAIN_SOURCE.encode("utf-8")), 30000)
        for relative in required:
            self.assertTrue((ROOT / "src" / "app" / relative).is_file())
            self.assertIn(f'#include "app/{relative}"', MAIN_SOURCE)
        for parser in ("app/meter/D0Parser.cpp", "app/meter/SmlParser.cpp"):
            self.assertIn(f'#include "{parser}"', MAIN_SOURCE)
        for compiled in (
            "app/core/EventLog.cpp", "app/network/EthernetManager.cpp",
            "app/storage/HistoryStore.cpp",
            "app/storage/DebugStorage.cpp",
        ):
            self.assertEqual(PLATFORMIO.count(f"+<{compiled}>"), 3)
        self.assertEqual(PLATFORMIO.count("-flto"), 3)
        self.assertEqual(PLATFORMIO.count("-fno-exceptions"), 3)
        self.assertEqual(PLATFORMIO.count("post:tools/enable_lto.py"), 3)

    def test_release_version_and_bilingual_ui_are_embedded(self):
        source = SOURCE
        self.assertIn('kFirmwareVersion[] = "1.3.5"', source)
        self.assertIn("id='langToggle'", source)
        self.assertIn("/assets/i18n.js", source)
        self.assertIn("irtracker-language-v1", I18N_SOURCE)
        self.assertIn("URLSearchParams(location.search).get('lang')", I18N_SOURCE)
        self.assertIn('"Einstellungen":"Settings"', I18N_SOURCE)
        self.assertIn("Michael Roßmann", source)

    def test_recurring_hotpaths_avoid_known_allocator_and_io_churn(self):
        self.assertIn("json.reserve(3000);", SOURCE)
        self.assertIn("char topic[96];", SOURCE)
        self.assertIn("char value[40];", SOURCE)
        self.assertIn("void serviceMeterInput()", SOURCE)
        loop_source = SOURCE[SOURCE.index("void loop()") :]
        self.assertLess(
            loop_source.index("serviceMeterInput();"),
            loop_source.index("manageMqtt();"),
        )
        self.assertIn("const auto readRange", HISTORY_SOURCE)
        self.assertIn("tier.capacity - first", HISTORY_SOURCE)

    def test_meter_diagnosis_reuses_one_evaluation_for_all_outputs(self):
        diagnostics = (ROOT / "src/app/diagnostics/DiagnosticsApi.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("constexpr MeterDiagnosisCode evaluateMeterDiagnosisCode(", diagnostics)
        self.assertGreaterEqual(diagnostics.count("meterDiagnosis();"), 3)
        for case in (
            "no bytes must report no IR signal",
            "bytes without telegrams must be distinguished",
            "power with missing energy values is a partial result",
            "one missing energy counter must not be a communication error",
            "complete stable readings must report OK",
            "CRC synchronization events must not imply lost telegrams",
            "rising CRC events with fresh valid data stay healthy",
            "stale valid data must remain a warning",
            "RX data without a valid telegram must be explicit",
            "parse failures plus missing power must warn",
        ):
            self.assertIn(case, diagnostics)
        self.assertIn("attempts >= 20U", diagnostics)
        self.assertIn("attempts) * 15U", diagnostics)
        self.assertNotIn(
            "static_cast<uint64_t>(telegrams) +\n                                         parseErrors + crcErrors",
            diagnostics,
        )

    def test_sml_crc_and_d0_bcc_events_are_separated_with_recoverable_auto_lock(self):
        meter_data = (METER_DIR / "MeterData.h").read_text(encoding="utf-8")
        manager = (METER_DIR / "MeterManager.cpp").read_text(encoding="utf-8")
        diagnostics = (ROOT / "src/app/diagnostics/DiagnosticsApi.cpp").read_text(
            encoding="utf-8"
        )
        telemetry = (ROOT / "src/app/web/TelemetryApi.cpp").read_text(
            encoding="utf-8"
        )
        status_api = (ROOT / "src/app/web/StatusApi.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("uint32_t smlCrcErrors", meter_data)
        self.assertNotIn("uint32_t d0BccErrors", meter_data)
        self.assertIn("d0Parser.checksumErrors()", manager)
        self.assertIn("++meter.smlCrcErrors", manager)
        self.assertIn("++meter.crcErrors", manager)
        self.assertIn("parserIntegrityIsActive(result.protocol)", manager)
        self.assertIn("meter.detectedProtocol != MeterProtocol::Auto", manager)
        self.assertIn("bool autoProtocolFresh(MeterProtocol protocol)", manager)
        self.assertIn("millis() - meter.lastTelegramMs < kReadingStaleMs", manager)
        self.assertIn("constexpr bool shouldFeedD0ParserFor(", manager)
        self.assertIn("constexpr bool shouldFeedSmlParserFor(", manager)
        self.assertIn('"stale Auto lock must release both parsers"', manager)
        self.assertIn('"fresh Auto/SML must pause only D0"', manager)
        self.assertIn('"fresh Auto/D0 must pause only SML"', manager)
        self.assertIn("if (shouldFeedD0Parser())", manager)
        self.assertIn("if (shouldFeedSmlParser())", manager)
        self.assertIn("smlParser.feed(&value, 1, result)", manager)
        self.assertIn("D0-BCC-Ereignisse stammen aus der parallelen Auto-Erkennung", diagnostics)
        for field in ("sml_crc_errors", "d0_bcc_errors"):
            self.assertIn(field, diagnostics)
            self.assertIn(field, telemetry)
            self.assertIn(field, status_api)
        self.assertIn("irtracker_sml_crc_errors_total", telemetry)
        self.assertIn("irtracker_d0_bcc_errors_total", telemetry)

    def test_support_report_is_on_demand_and_excludes_known_secrets(self):
        diagnostics = (ROOT / "src/app/diagnostics/DiagnosticsApi.cpp").read_text(
            encoding="utf-8"
        )
        report = diagnostics[
            diagnostics.index("String supportReportText(bool technical)") :
        ]
        for forbidden in (
            "config.ssid",
            "config.password",
            "config.adminPassword",
            "config.mqttUser",
            "config.mqttPassword",
            "config.meterPin",
            "WiFi.localIP",
            "WiFi.macAddress",
            "lastTelegram.data",
        ):
            self.assertNotIn(forbidden, report)
        self.assertIn('report.reserve(technical ? 3200 : 1800);', report)
        normal_report = report[: report.index("if (technical)")]
        for technical_only in (
            "SML-Parser-Modus", "Vergleichsabweichungen", "Kontrollvergleiche",
            "Largest Free Heap Block", "Stack Reserve (High Water Mark)",
        ):
            self.assertNotIn(technical_only, normal_report)
            self.assertIn(technical_only, report)
        self.assertIn('report += "Reset-Ursache: " + bootResetReason', report)
        self.assertIn('/api/v1/support-report', SOURCE)
        self.assertIn("copyReport(false)", WEB_RUNTIME_SOURCE)
        self.assertIn("copyReport(true)", WEB_RUNTIME_SOURCE)
        for heading in ("IR-Verbindung", "Protokoll", "Messwerte", "System"):
            self.assertIn(heading, SOURCE)

    def test_sensitive_raw_and_runtime_diagnostics_are_hardened(self):
        web_api = (ROOT / "src/app/web/WebApi.cpp").read_text(encoding="utf-8")
        raw_route = web_api[web_api.index('server.on("/api/v1/raw"') :]
        raw_route = raw_route[: raw_route.index("});") + 3]
        self.assertIn("requireAdmin()", raw_route)
        self.assertNotIn("requireApiAccess()", raw_route)

        telemetry = (ROOT / "src/app/web/TelemetryApi.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)", telemetry)
        self.assertIn("uxTaskGetStackHighWaterMark(nullptr)", telemetry)
        self.assertIn("largest_free_heap_block", telemetry)
        self.assertIn("stack_high_water_mark_bytes", telemetry)

        sml_header = (METER_DIR / "SmlParser.h").read_text(encoding="utf-8")
        sml_source = (METER_DIR / "SmlParser.cpp").read_text(encoding="utf-8")
        self.assertIn("kQualificationFrames = 32", sml_header)
        self.assertIn("kSentinelInterval = 512", sml_header)
        self.assertIn("Diagnostics diagnostics() const", sml_header)
        self.assertIn("++comparisonMismatches_", sml_source)
        self.assertIn("++sentinelComparisons_", sml_source)

    def test_debug_partition_is_used_for_optional_frontend_assets(self):
        source = SOURCE
        self.assertIn("debugStorage.begin(kFirmwareVersion)", source)
        self.assertIn("debugStorage.openVerifiedAsset(compressedPath.c_str())", source)
        self.assertIn("tryServeDebugAsset(path, contentType", source)
        self.assertIn('server.sendHeader("Content-Encoding", "gzip")', source)
        self.assertIn("assetManifestReady()", source)
        self.assertIn('assetManifestError_ = "version_mismatch"', DEBUG_STORAGE_SOURCE)
        self.assertIn("verifyAsset(path.c_str(), expectedSize, expectedSha256)",
                      DEBUG_STORAGE_SOURCE)
        for error in (
            "partition_missing", "mount_failed", "manifest_invalid",
            "version_mismatch", "size_mismatch", "sha256_mismatch",
            "file_missing",
        ):
            self.assertIn(f'"{error}"', DEBUG_STORAGE_SOURCE)
        for field in (
            "asset_partition_mounted", "asset_manifest_valid", "asset_version",
            "asset_source_maintenance_js", "asset_last_error",
        ):
            self.assertIn(field, source)
        self.assertIn("noteAssetServed(compressedPath.c_str(), true)", source)
        self.assertIn("noteAssetServed(compressedPath.c_str(), false)", source)
        self.assertIn('return maintenanceServedFromPartition_ ? "partition"',
                      DEBUG_STORAGE_HEADER)
        asset_builder = (ROOT / "tools/embed_web_assets.py").read_text(
            encoding="utf-8"
        )
        self.assertIn("PARTITION_ASSETS = set(ASSETS)", asset_builder)
        self.assertIn('"assets_version": firmware_version', asset_builder)
        self.assertIn('hashlib.sha256(compressed).hexdigest()', asset_builder)
        self.assertIn('servePartitionAsset("/assets/i18n.js"', source)
        self.assertIn("board_build.filesystem = littlefs", PLATFORMIO)
        self.assertIn("debugfs,    data, spiffs,  0x2B0000, 0x10000", PARTITIONS)
        self.assertNotIn("coredump,   data, spiffs", PARTITIONS)

    def test_asset_diagnostics_cover_partition_and_all_fallback_causes(self):
        report = (ROOT / "src/app/diagnostics/DiagnosticsApi.cpp").read_text(
            encoding="utf-8"
        )
        for label in (
            "Asset-Partition: ", "Manifest: ", "Asset-Version: ",
            "maintenance.js Quelle: ", "Letzter Asset-Fehler: ",
        ):
            self.assertIn(label, report)
        self.assertIn('"debugfs"', report)
        self.assertIn('"eingebetteter Fallback"', report)
        self.assertIn("if (!file) return AssetValidation::FileMissing",
                      DEBUG_STORAGE_SOURCE)
        self.assertIn("return AssetValidation::SizeMismatch",
                      DEBUG_STORAGE_SOURCE)
        self.assertIn("AssetValidation::Sha256Mismatch",
                      DEBUG_STORAGE_SOURCE)
        self.assertIn("assetManifestReady_ = true", DEBUG_STORAGE_SOURCE)
        self.assertIn("assetManifestError_ = \"\"", DEBUG_STORAGE_SOURCE)

    def test_debug_partition_compatibility_contract(self):
        # New labels win, old OTA layouts remain valid, and an absent optional
        # partition is represented by nullptr rather than causing a crash.
        self.assertIn(
            'return hasDebugfs ? "debugfs" : (hasCoredump ? "coredump" : nullptr);',
            DEBUG_STORAGE_HEADER,
        )
        self.assertIn('strcmp(observedTarget_->label, "debugfs")',
                      DEBUG_STORAGE_SOURCE)
        self.assertIn('strcmp(observedTarget_->label, "coredump")',
                      DEBUG_STORAGE_SOURCE)
        self.assertIn("loadRawAssetManifest(firmwareVersion)",
                      DEBUG_STORAGE_SOURCE)
        self.assertIn("if (!partitionLabel_)", DEBUG_STORAGE_SOURCE)
        self.assertIn("State::Missing", DEBUG_STORAGE_SOURCE)
        self.assertIn("debugfs must be preferred", DEBUG_STORAGE_HEADER)
        self.assertIn("legacy coredump must remain supported", DEBUG_STORAGE_HEADER)
        self.assertIn("missing debug storage must be safe", DEBUG_STORAGE_HEADER)
        self.assertIn("partitionIsBlank", DEBUG_STORAGE_SOURCE)
        self.assertIn("filesystem_.begin(false", DEBUG_STORAGE_SOURCE)
        self.assertIn("existing_partition_not_littlefs_preserved", DEBUG_STORAGE_SOURCE)
        self.assertIn('constexpr char kPreferredDirectory[] = "/debug"', DEBUG_STORAGE_SOURCE)
        self.assertIn('constexpr char kLegacyDirectory[] = "/coredump"', DEBUG_STORAGE_SOURCE)
        self.assertIn('LittleFS.begin(true, "/history", 10, "history")', HISTORY_SOURCE)
        for fixed_entry in (
            "ota_0,      app,  ota_0,   0x10000,  0x150000",
            "ota_1,      app,  ota_1,   0x160000, 0x150000",
            "history,    data, spiffs,  0x2C0000, 0x140000",
        ):
            self.assertIn(fixed_entry, PARTITIONS)

    def test_release_size_optimizations_remain_enabled(self):
        self.assertEqual(PLATFORMIO.count("-flto"), 3)
        self.assertEqual(PLATFORMIO.count("-fno-unwind-tables"), 3)
        self.assertEqual(PLATFORMIO.count("-fno-asynchronous-unwind-tables"), 3)
        self.assertEqual(PLATFORMIO.count("CORE_DEBUG_LEVEL=0"), 2)
        self.assertEqual(PLATFORMIO.count("CORE_DEBUG_LEVEL=1"), 1)
        self.assertIn("gzip.compress(minified, compresslevel=9, mtime=0)", WEB_ASSET_SCRIPT)
        self.assertIn("serializeJson(document, output)", SOURCE)
        self.assertNotIn("serializeJsonPretty(document, output)", SOURCE)

    def test_original_backups_are_ignored(self) -> None:
        ignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
        self.assertIn("original bin/", ignore)
        self.assertIn("signing/private/", ignore)

    def test_required_hardening_is_present(self) -> None:
        self.assertIn("safeSingleLine", SOURCE)
        self.assertNotIn("value.indexOf('\\0')", SOURCE)
        for marker in (
            "esp_fill_random",
            "guard.failures >= 5",
            "kLoginMaxLockMs",
            "esp_task_wdt_init",
            "kHeapWarningBytes",
            "config.autoPin = false",
            "firmware_signature_invalid",
            "constexpr size_t kLiveSamples = 840",
            "history.flushPending(HistoryStore::Tier::Minute)",
            "history_flush_before_update_failed",
            "Damaged frames must never alter live values/history",
            "Commit atomically only after CRC, parsing and plausibility",
            "settings_backup_too_large",
            "batch_too_large",
            "mbedtls_md_hmac",
            "kBrowserSessionSeconds",
            "HttpOnly; SameSite=Strict",
            "admin_password_confirmation_mismatch",
            "mqttNetwork.connect(config.mqttHost.c_str(), config.mqttPort, 250)",
            "mqttRetryMs * 2",
            "mqtt.setSocketTimeout(1)",
            "dBindCursor(pc)",
            "dCursorPinned=true",
            "cursorPinned=true",
            "if(e.detail===1){dCursorPinned=false",
            "if(e.detail===1){cursorPinned=false",
            "Doppelklick oder Doppeltippen fixiert ihn",
            "doubleTap=now-lastTap<420",
            "doubleTap=now-lastTouchTap<420",
            "e.pointerType==='touch'",
            'range == "hour"',
            "function dTimeAxis",
            "function hTimeAxis",
            "minute===30?8:4",
            "function scheduleLoad(delay=140)",
            "function dScheduleLoad(silent=false,delay=140)",
            "if(silent&&dLoadController)return",
            "new AbortController()",
            "request!==historyRequest",
            "generation!==dLoadGeneration",
            "function dStopLoads()",
            "function stopHistoryLoads()",
            "addEventListener('pagehide',dStopLoads)",
            "addEventListener('pagehide',stopHistoryLoads)",
            "if (!responseClient.connected()) return false",
            "localStorage.getItem(themeKey)",
            "localStorage.setItem(themeKey,n)",
            "const themes=[['#07100c','#10231a','#22c55e']",
            "target=dFrom+f*(dTo-dFrom)",
            "class='chart-section'",
            "width:min(100%,1800px)",
            "height:clamp(390px,55vh,680px)",
            "height:clamp(350px,42vh,560px)",
            "px.lineWidth=2",
            "gridImport=power.map",
            "gridExport=power.map",
            "Math.max(0,v)",
            "Math.min(0,v)",
            "dVisible={power:true,import:true,export:true,gap:true}",
            "cx.lineWidth=2",
            "calendarHistoryQuery",
            "now - static_cast<uint32_t>(since)",
            "window.irGapEdges",
            "current=to>=now-step",
            "Math.max(step*2.5,15)",
            "irGapCount(dd,dFrom,dTo,dStep())",
            "irGapCount(raw,rangeFrom,rangeTo,expectedStep())",
            "requestedHistoryAnchor",
            "historyTierSeconds",
            "id='dashDaysBack'",
            "id='historyDaysBack'",
            "Kalendertag (00:00–24:00)",
            "type:'datetime-local'",
            "type:'date'",
            "type:'week'",
            "type:'month'",
            "type:'number'",
            "label:'Kalenderwoche'",
            "label:'Kalendermonat'",
            "label:'Kalenderjahr'",
            "function minutePower(a)",
            "Math.floor(v.ts/60)*60",
            "rangeStep<60?60:rangeStep",
            "Vollständige Historie als CSV exportieren",
            'range=complete',
            "resolution_seconds",
            "handleDashboardSummary",
            "yesterday_same_time_import_kwh",
            "year_daily_average_import_kwh",
            "year_coverage_days",
            "Jahreswerte seit Aufzeichnungsbeginn",
            "year_average_power_w",
            "averageYearCompare",
            "importYearCompare",
            "exportYearCompare",
            "function dYearComparison",
            "% zum Jahres-Ø",
            "eventLog.begin(config.persistEventLog)",
            "event_log_persistent",
            'name="event_flash"',
            "kCpuBoostHoldMs = 2UL * 60UL * 1000UL",
            "kEcoCpuMhz = 80",
            "kPerformanceCpuMhz = 160",
            "prefs.getBool(\"eco_mode\", true)",
            'name="eco_mode"',
            "requestCpuBoost(\"history_export\")",
            "requestCpuBoost(\"firmware_update\")",
            "requestCpuBoost(\"wifi_connect\")",
            "requestCpuBoost(\"lan_fallback\")",
            "requestCpuBoost(\"factory_test\")",
            "manageCpuPowerMode()",
            "cpu_boost_remaining_s",
            "cpu_frequency_errors",
            "setInterval(refreshLive,4000)",
            "if(!document.hidden)updateValues()",
            "if(!document.hidden)updateDashboardSummary()",
            "prefs.getBool(\"eco_led_off\", true)",
            'name="eco_led_off"',
            "bool trackerFaultActive()",
            "ecoLedSuppressed()",
            "led_fault_active",
            "WiFi.softAPdisconnect(true)",
            "WiFi.mode(WIFI_STA)",
            "esp_wifi_set_ps(WIFI_PS_MIN_MODEM)",
            "#include <ESPmDNS.h>",
            "MDNS.begin(config.hostname.c_str())",
            'MDNS.addService("http", "tcp", 80)',
            "MDNS.end()",
            "mdns_running",
            "IR_TRACKER_ENABLE_MDNS",
            "trackerGpioAvailable(pin)",
            "w5500_gpio_reserved",
            "kWifiPowerStableMs = 3UL * 60UL * 1000UL",
            "kWifiPowerEvaluateMs = 60UL * 1000UL",
            "WIFI_POWER_19_5dBm",
            "WIFI_POWER_15dBm",
            "WIFI_POWER_11dBm",
            "prefs.getBool(\"wifi_power_auto\", true)",
            'name="wifi_power_auto"',
            "prefs.getBool(\"wifi_ps\", false)",
            'name="wifi_ps"',
            "manageAdaptiveWifiPower()",
            "wifi_tx_power_dbm",
            "wifi_min_modem_sleep",
            "'&anchor='+anchor",
            "age < 179UL * 86400UL",
            "Umschalt+Ziehen",
            "Minimum: ${fmtPower(v.min)}",
            "history.replaceState(null,'',location.pathname",
            "const nativeFetch=window.fetch.bind(window)",
            ".chart-card,.dashboard-chart,.chart-wrap{min-width:0;max-width:100%}",
            ".legend-row{gap:10px 14px}",
            'server.on("/auth/logout"',
            '"Content-Security-Policy"',
            '"X-Frame-Options", "DENY"',
            '"X-Content-Type-Options", "nosniff"',
            "void handleDiagnostics() {\n  if (!requireAdmin()) return;",
            'server.on("/system/shutdown"',
            "esp_deep_sleep_start",
            'server.on("/api/v1/gpio-scan/start"',
            'server.on("/api/v1/gpio-scan/cancel"',
            "meter.lastTelegramMs >= gpioScan.candidateStartedMs",
            "meter.lastCrcValid",
            "restoreMeterSerialAfterScan()",
            "if (meterFresh && !gpioScan.active)",
        ):
            self.assertIn(marker, WEB_RUNTIME_SOURCE)
        self.assertIn("lastWrittenBucket == bucket", HISTORY_SOURCE)
        self.assertIn("validRecord(record)", HISTORY_SOURCE)
        self.assertIn("kMaximumPlausiblePowerW", HISTORY_HEADER)
        self.assertIn("file.size() < requiredSize", HISTORY_SOURCE)
        self.assertNotIn("server.enableCORS(true)", SOURCE)
        self.assertNotIn("WiFi.softAPdisconnect(false)", SOURCE)
        self.assertNotIn('requestCpuBoost("history_query")', SOURCE)
        self.assertNotIn("now - static_cast<uint32_t>(until)", SOURCE)
        self.assertNotIn("kIdleCpuMhz", SOURCE)
        self.assertNotIn("setCpuFrequencyMhz(40", SOURCE)
        self.assertNotIn('requestCpuBoost("dashboard_summary")', SOURCE)
        self.assertIn("id='powerLegend'", WEB_RUNTIME_SOURCE)
        for series in ("power", "import", "export", "gap"):
            self.assertIn(f"data-series='{series}'", WEB_RUNTIME_SOURCE)
        self.assertIn("button.setAttribute('aria-pressed'", WEB_RUNTIME_SOURCE)
        self.assertNotIn("id='dashChart'", WEB_RUNTIME_SOURCE)
        self.assertNotIn("id='dashChartWrap'", WEB_RUNTIME_SOURCE)
        self.assertIn("Record records_[kCapacity]", EVENT_HEADER)
        self.assertIn("static constexpr uint32_t kCapacity = 256", EVENT_HEADER)
        self.assertIn("bool setPersistence(bool persistent)", EVENT_HEADER)
        self.assertIn("appendRam(record)", EVENT_SOURCE)
        self.assertIn("return !persistent_ || writePersistent(record)", EVENT_SOURCE)

    def test_theme_stays_browser_local(self) -> None:
        theme_script = COMMON_JS_SOURCE
        self.assertEqual(SOURCE.count("data-theme-var="), 0)
        self.assertIn("const themes=[['#07100c','#10231a','#22c55e']", theme_script)
        self.assertIn("r.setProperty('--accent',themes[n][2])", theme_script)
        self.assertIn("themeKey='irtracker-theme-v2'", theme_script)
        self.assertIn("localStorage.getItem(themeKey)", theme_script)
        self.assertIn("localStorage.setItem(themeKey,n)", theme_script)
        self.assertNotIn("fetch(", theme_script)
        self.assertNotIn("XMLHttpRequest", theme_script)

    def test_lan_profile_reserves_w5500_pins(self) -> None:
        self.assertIn("IR_TRACKER_LAN_PROFILE", HARDWARE_PROFILE)
        for pin_name in ("kW5500CsPin", "kW5500IntPin", "kW5500SckPin",
                         "kW5500MosiPin", "kW5500MisoPin"):
            self.assertIn(pin_name, HARDWARE_PROFILE)
        self.assertIn("trackerGpioAvailable(pin)", SOURCE)

    def test_universal_network_uses_one_lwip_stack(self) -> None:
        standard = PLATFORMIO.split("[env:solakon_tracker_developer]", 1)[0]
        self.assertIn("IR_TRACKER_LAN_PROFILE=1", standard)
        self.assertNotIn("arduino-libraries/Ethernet", PLATFORMIO)
        for marker in (
            "esp_eth_mac_new_w5500", "esp_eth_phy_new_w5500",
            "esp_eth_new_netif_glue", "esp_netif_attach",
            "route_prio = kEthernetRoutePriority", "probeW5500()",
            "transaction.rx_data[0] == 0x04",
        ):
            self.assertIn(marker, ETHERNET_SOURCE)
        self.assertIn("bool networkConnected()", SOURCE)
        self.assertIn("ethernet.hardwareDetected()", SOURCE)
        self.assertIn("ethernet.loop();", SOURCE)
        self.assertIn("if (!networkConnected() || !config.mqttHost.length())", SOURCE)
        self.assertIn('hardware_profile', SOURCE)
        self.assertIn('universal', SOURCE)

    def test_legacy_meter_protocol_is_read_only_and_shared(self) -> None:
        for marker in (
            "IEC-62056-21/D0-Parser", "kMaximumFrame", "bccPresent",
            "checksumErrors", "1.8.0", "1.8.1", "1.8.2",
            "2.8.0", "2.8.1", "2.8.2", "16.7.0",
            "36.7.0", "32.7.0", "31.7.0",
        ):
            self.assertIn(marker, LEGACY_METER_HEADER + LEGACY_METER_SOURCE)
        for marker in (
            '#include "app/meter/D0Parser.h"', "MeterProtocol::Iec62056",
            "commitMeterCandidate(candidate, result.protocol",
            "SERIAL_7E1", "meter_protocol", "no_valid_meter_telegram",
            "300, 600, 1200, 2400, 4800", "importPowerObis",
            "exportPowerObis", "importTariffObis", "exportTariffObis",
        ):
            self.assertIn(marker, SOURCE)
        for marker in ("Iec62056Active", "beginActiveD0Attempt",
                       "updateActiveD0", "acknowledgement",
                       "updateMeterRecovery", "METER_UART_RECOVERY"):
            self.assertIn(marker, SOURCE)

    def test_value_age_is_exposed_by_all_read_only_interfaces(self) -> None:
        for marker in (
            "power_age_s", "import_age_s", "export_age_s",
            "telegram_age_s", "voltage_age_s", "current_age_s",
            "irtracker_power_age_seconds", "irtracker_phase_power_age_seconds",
            "power_age_s=", "metric,value,unit,age_seconds",
            "irtracker_age_s",
        ):
            self.assertIn(marker, SOURCE)
        self.assertIn("valueFresh(meter.powerUpdatedMs)", SOURCE)

    def test_factory_test_is_isolated_and_fixture_based(self) -> None:
        for marker in (
            "[env:solakon_tracker_factory]",
            "IR_TRACKER_ENABLE_FACTORY_TEST=1",
            "IR_TRACKER_ENABLE_GITHUB_UPDATE=0",
        ):
            self.assertIn(marker, PLATFORMIO)
        for marker in (
            "kFactoryLoopbackPattern", "FCT_IR_LOOPBACK",
            "/maintenance/factory-test", "factoryAutomatedChecksPass",
            "ethernet.hardwareDetected()", "factoryTest.ledConfirmed",
        ):
            self.assertIn(marker, SOURCE)
        standard = PLATFORMIO.split("[env:solakon_tracker_developer]", 1)[0]
        self.assertIn("IR_TRACKER_ENABLE_FACTORY_TEST=0", standard)

    def test_legacy_phase_import_export_power_is_combined(self) -> None:
        for marker in ("21.7.0", "41.7.0", "61.7.0", "22.7.0",
                       "42.7.0", "62.7.0", "phaseImportPowerW",
                       "phaseExportPowerW"):
            self.assertIn(marker, LEGACY_METER_SOURCE)

    def test_production_is_read_only_and_developer_io_is_opt_in(self) -> None:
        self.assertFalse((ROOT / "src" / "EnergyManager.cpp").exists())
        self.assertFalse((ROOT / "src" / "EnergyManager.h").exists())
        for legacy_marker in (
            "energyConfig", "energyManager", "handleEnergyPage",
            "handleEnergySave", "handleEnergyStop", 'name="em_enable"',
        ):
            self.assertNotIn(legacy_marker, SOURCE)
        self.assertIn("IR_TRACKER_ENABLE_DEVELOPER_IO", HARDWARE_PROFILE)
        self.assertIn("[env:solakon_tracker_developer]", PLATFORMIO)
        standard = PLATFORMIO.split("[env:solakon_tracker_developer]", 1)[0]
        self.assertIn("IR_TRACKER_ENABLE_DEVELOPER_IO=0", standard)
        self.assertNotIn("links2004/WebSockets", standard)
        self.assertIn("IR_TRACKER_ENABLE_DEVELOPER_IO=1", PLATFORMIO)

    def test_read_only_integrations_remain_available(self) -> None:
        for marker in (
            "PubSubClient mqtt", "homeAssistantDiscovery", "/status",
            "/emeter/0", "/rpc/EM.GetStatus", "/api/v1/history",
            "/api/v1/values", "/metrics", "/openmetrics", "/v1/json",
        ):
            self.assertIn(marker, SOURCE)
        self.assertIn("normalizeHardwarePins();", SOURCE)

    def test_storage_compatibility_keeps_neutral_identity_and_secure_default(self):
        identity = (ROOT / "src/app/core/DeviceIdentity.h").read_text(
            encoding="utf-8")
        network = (ROOT / "src/app/network/NetworkManager.cpp").read_text(
            encoding="utf-8")
        security = (ROOT / "src/app/core/SecurityManager.cpp").read_text(
            encoding="utf-8")
        shelly = (ROOT / "src/app/web/ShellyEmulation.cpp").read_text(
            encoding="utf-8")
        eco = (ROOT / "src/app/web/EcoTrackerEmulation.cpp").read_text(
            encoding="utf-8")
        routes = (ROOT / "src/app/web/WebApi.cpp").read_text(encoding="utf-8")

        for own_marker in ("IRTRACKER-C3", "IRTRACKER-C3-3EM", "IRT-%s",
                           "irtracker-%s"):
            self.assertIn(own_marker, identity)
        for forbidden in ("SPEM-003CEBEU", "everhome_", "shellypro3em-"):
            self.assertNotIn(forbidden, SOURCE + identity)
        self.assertIn('bool storageCompatibilityMode = false', SOURCE)
        self.assertIn('prefs.getBool("storage_compat", false)', SOURCE)
        self.assertIn("localCompatibilityClient()", security)
        self.assertIn("requireStorageCompatibilityAccess()", routes)
        self.assertIn('MDNS.addService("shelly", "tcp", 80)', network)
        self.assertIn('MDNS.addService("everhome", "tcp", 80)', network)
        self.assertIn('MDNS.addService("irtracker", "tcp", 80)', network)
        self.assertIn('"product-id", "IRT1000"', network)
        self.assertIn('"serial-number"', network)
        self.assertIn("deviceIdentity.mac", shelly)
        compat_id = shelly[shelly.index("String shellyCompatId()"):
                           shelly.index("String shellyMac()")]
        self.assertIn("deviceIdentity.hostname", compat_id)
        self.assertNotIn("config.hostname", compat_id)
        self.assertIn('\\"energyCounterInT1\\":null', eco)
        self.assertIn('\\"energyCounterInT2\\":null', eco)
        self.assertIn("/ 1000U", eco)
        for protected in ('/system/update', '/api/v1/gpio-scan/start',
                          '/api/v1/history/clear',
                          '/api/v1/asset-partition/backup',
                          '/api/v1/asset-partition/update'):
            self.assertIn(protected, routes)

    def test_neutral_meter_interfaces_share_one_schema(self) -> None:
        integration = (ROOT / "src/app/web/IntegrationApi.cpp").read_text(
            encoding="utf-8")
        mqtt = (ROOT / "src/app/network/MqttManager.cpp").read_text(
            encoding="utf-8")
        routes = (ROOT / "src/app/web/WebApi.cpp").read_text(encoding="utf-8")
        modbus = (ROOT / "src/app/network/ModbusMeterServer.cpp").read_text(
            encoding="utf-8")
        interfaces = (ROOT / "src/app/web/SettingsApi.cpp").read_text(
            encoding="utf-8")
        self.assertIn('\\"schema\\":\\"irtracker.meter.v1\\"', integration)
        self.assertIn('server.on("/api/v1/meter"', routes)
        self.assertIn("neutralMeterJson()", mqtt)
        self.assertIn('publish(base, "/meter"', mqtt)
        self.assertIn("kModbusMeterPort = 502", modbus)
        self.assertIn("function != 0x03 && function != 0x04", modbus)
        self.assertIn("isPrivateLocalAddress", modbus)
        self.assertIn("<h2>Modbus TCP</h2>", interfaces)
        self.assertIn("docs/MODBUS.md", interfaces)
        for counter in ("modbusConnectionCount", "modbusValidRequestCount",
                        "modbusInvalidRequestCount", "modbusLastClient"):
            self.assertIn(counter, modbus)
        self.assertIn("bool modbusTcp = false", SOURCE)
        for forbidden in ("SPEM-003CEBEU", "shellypro3em-", "everhome_"):
            self.assertNotIn(forbidden, modbus)

    def test_integrations_share_hardened_read_only_response_path(self) -> None:
        integration = (ROOT / "src" / "app" / "web" /
                       "IntegrationApi.cpp").read_text(encoding="utf-8")
        security = (ROOT / "src" / "app" / "core" /
                    "SecurityManager.cpp").read_text(encoding="utf-8")
        eco = (ROOT / "src" / "app" / "web" /
               "EcoTrackerEmulation.cpp").read_text(encoding="utf-8")
        self.assertIn("X-IR-Tracker-Mode", integration)
        self.assertIn("X-IR-Tracker-Version", integration)
        api_access = security[security.index("bool requireApiAccess()") :]
        self.assertLess(api_access.index("config.apiAccess == 2"),
                        api_access.index('server.authenticate("admin"'))
        self.assertIn("kLiveSamples - 1U - i", eco)
        self.assertIn('requestCpuBoost("gpio_scan")', SOURCE)

    def test_navigation_and_maintenance_are_reduced(self) -> None:
        nav_start = SOURCE.index("String nav()")
        nav_end = SOURCE.index("String maintenanceTabs", nav_start)
        navigation = SOURCE[nav_start:nav_end]
        self.assertNotIn("/diagnostics", navigation)
        self.assertNotIn("JSON API", navigation)
        self.assertIn("/maintenance/diagnostics", SOURCE)
        self.assertIn("JSON-API f", SETUP_HTML_SOURCE)
        self.assertIn("Content-Encoding", SOURCE)
        self.assertIn("String recoveryPage()", SOURCE)
        self.assertIn("Asset-Image installieren", SOURCE)
        self.assertIn("servePartitionAsset", SOURCE)
        self.assertNotIn("kDashboardJsGzip", SOURCE)
        self.assertNotIn("kHistoryJsGzip", SOURCE)
        self.assertIn("async function loadDiagnosis()", DIAGNOSTICS_JS_SOURCE)
        self.assertNotIn("async function loadDiagnosis()", SOURCE)
        self.assertIn("window.IR_TRACKER_CONFIG={csrfToken:", SOURCE)
        self.assertNotIn("script.reserve(26000)", SOURCE)
        self.assertNotIn("DASHBOARD_SCRIPT_MEMORY", SOURCE)
        self.assertNotIn("HISTORY_SCRIPT_MEMORY", SOURCE)
        self.assertNotIn("Apator vollständig freischalten", SOURCE)
        self.assertNotIn("LEPUS-Spannung", SOURCE)

    def test_update_check_uses_maintenance_status_instead_of_raw_json(self) -> None:
        route_start = SOURCE.index('server.on("/api/v1/update/check"')
        route_end = SOURCE.index('server.on("/api/v1/update/install"', route_start)
        check_route = SOURCE[route_start:route_end]
        self.assertIn('server.sendHeader("Location", "/maintenance#firmware-update"', check_route)
        self.assertIn('server.send(303, "text/plain", "")', check_route)
        self.assertNotIn("githubUpdateJson()", check_route)
        self.assertNotIn("<pre>", check_route)
        self.assertIn("async function loadUpdate()", MAINTENANCE_JS_SOURCE)
        self.assertIn("Die installierte Firmware ist aktuell.", MAINTENANCE_JS_SOURCE)
        self.assertIn("Technischer Fehlercode", SOURCE)

    def test_first_access_password_is_prominent_in_readme(self) -> None:
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        self.assertIn("## Erster Zugang – Standardpasswort", readme)
        self.assertIn("### First access – default password", readme)
        self.assertIn("IR-Tracker-Setup-XXXX", readme)
        self.assertIn("IRTracker-XXXX", readme)
        self.assertIn("| Weboberfläche | `admin` |", readme)

    def test_browser_forms_show_friendly_errors_instead_of_raw_json(self) -> None:
        self.assertIn("document.addEventListener('submit',async event=>", COMMON_JS_SOURCE)
        self.assertIn("invalid_wifi_credentials", COMMON_JS_SOURCE)
        self.assertIn("csrf_token_invalid", COMMON_JS_SOURCE)
        self.assertIn("actionMessage", COMMON_JS_SOURCE)
        self.assertIn("new URLSearchParams(new FormData(form))", COMMON_JS_SOURCE)
        self.assertIn("validWifiPassword", SOURCE)
        self.assertIn('maxlength="32"', SETUP_JS_SOURCE)
        self.assertIn('maxlength="64" autocomplete="off"', SETUP_JS_SOURCE)
        ota_start = SOURCE.index("void handleOtaFinished()")
        ota_end = SOURCE.index("void handleSafeShutdown()", ota_start)
        ota_handler = SOURCE[ota_start:ota_end]
        self.assertIn("Firmwareupdate abgelehnt", ota_handler)
        self.assertNotIn('"application/json"', ota_handler)

    def test_external_asset_pipeline_is_reproducible_gzip(self) -> None:
        assets = {
            "common.css": COMMON_CSS_SOURCE,
            "common.js": COMMON_JS_SOURCE,
            "i18n.js": I18N_SOURCE,
            "dashboard.js": DASHBOARD_JS_SOURCE,
            "history.js": HISTORY_JS_SOURCE,
            "maintenance.js": MAINTENANCE_JS_SOURCE,
            "diagnostics.js": DIAGNOSTICS_JS_SOURCE,
            "setup.html": SETUP_HTML_SOURCE,
            "setup.js": SETUP_JS_SOURCE,
        }
        for filename, source in assets.items():
            lines = [line.strip() for line in source.replace("\r\n", "\n").split("\n")
                     if line.strip()]
            separator = "" if filename == "diagnostics.js" else "\n"
            expected = separator.join(lines).encode("utf-8")
            payload = gzip.compress(expected, compresslevel=9, mtime=0)
            self.assertEqual(gzip.decompress(payload), expected, filename)
            self.assertLess(len(payload), len(expected), filename)
            self.assertIn(f'"{filename}"', WEB_ASSET_SCRIPT)
        self.assertIn("PARTITION_ASSETS = set(ASSETS)", WEB_ASSET_SCRIPT)
        self.assertIn("asset-build-report.json", WEB_ASSET_SCRIPT)
        self.assertIn('Path(env.subst("$BUILD_DIR")) / "generated"', WEB_ASSET_SCRIPT)
        self.assertIn("env.Prepend(CPPPATH", WEB_ASSET_SCRIPT)
        self.assertFalse((ROOT / "src" / "WebAssets.h").exists())

    def test_large_pages_use_cached_external_assets(self) -> None:
        dashboard = SOURCE[SOURCE.index("void handleRoot()") : SOURCE.index("void handleHistoryPage()")]
        history = SOURCE[SOURCE.index("void handleHistoryPage()") : SOURCE.index("struct HistoryQuery")]
        self.assertIn("/assets/dashboard.js?v=", dashboard)
        self.assertIn("/assets/history.js?v=", history)
        self.assertIn("/assets/maintenance.js?v=", SOURCE)
        self.assertIn("/assets/setup.js?v=", SOURCE)
        self.assertNotIn("String script", dashboard)
        self.assertNotIn("String script", history)
        self.assertIn("/assets/common.css?v=", SOURCE)
        self.assertIn("/assets/common.js?v=", SOURCE)
        self.assertIn("window.IR_TRACKER_CONFIG={csrfToken:", SOURCE)
        self.assertIn("style-src 'self' 'unsafe-inline'", SOURCE)
        self.assertIn("script-src \"\n      \"'self' 'unsafe-inline'", SOURCE)
        self.assertEqual(COMMON_CSS_SOURCE.count("{"), COMMON_CSS_SOURCE.count("}"))

    def test_no_secret_is_printed_or_logged(self) -> None:
        forbidden = (
            r"Serial\.(?:print|printf|println)\([^\n]*(?:meterPin|mqttPassword|apPassword)",
            r"eventLog\.add\([^\n]*(?:mqttPassword|meterPin|adminPassword)",
        )
        for pattern in forbidden:
            self.assertIsNone(re.search(pattern, SOURCE, re.IGNORECASE))

    def test_author_and_license_are_embedded(self) -> None:
        self.assertIn('kFirmwareAuthor[] = "Michael Roßmann"', SOURCE)
        self.assertIn("PolyForm Noncommercial 1.0.0", SOURCE)
        self.assertTrue((ROOT / "LICENSE.md").exists())
        self.assertTrue(
            (ROOT / "docs/legal/THIRD_PARTY_NOTICES.md").exists()
        )
        notices = (ROOT / "docs/legal/THIRD_PARTY_NOTICES.md").read_text(
            encoding="utf-8"
        )
        self.assertIn("Arduino Core for ESP32 — LGPL-2.1-or-later", notices)
        self.assertTrue((ROOT / "licenses/Arduino-ESP32-LGPL-2.1.txt").exists())

    def test_committed_public_key_matches_header(self) -> None:
        pem = (ROOT / "signing" / "firmware-signing-public.pem").read_text()
        header = (ROOT / "src/app/update/FirmwareSigningPublicKey.h").read_text()
        self.assertIn(pem.strip(), header)
        key = serialization.load_pem_public_key(pem.encode())
        self.assertIsInstance(key, ec.EllipticCurvePublicKey)
        self.assertEqual(key.curve.name, "secp256r1")

    def test_signature_accepts_original_and_rejects_tamper(self) -> None:
        key = ec.generate_private_key(ec.SECP256R1())
        firmware = b"\xE9" + bytes(range(256)) * 8
        digest = hashlib.sha256(firmware).digest()
        signature = key.sign(digest, ec.ECDSA(Prehashed(hashes.SHA256())))
        package = (
            struct.pack("<8sIHH", b"IRFW100\0", len(firmware), len(signature), 0)
            + signature
            + firmware
        )
        _, size, sig_size, reserved = struct.unpack("<8sIHH", package[:16])
        self.assertEqual(size, len(firmware))
        self.assertEqual(reserved, 0)
        key.public_key().verify(
            package[16 : 16 + sig_size],
            hashlib.sha256(package[16 + sig_size :]).digest(),
            ec.ECDSA(Prehashed(hashes.SHA256())),
        )
        tampered = bytearray(package)
        tampered[-1] ^= 1
        with self.assertRaises(InvalidSignature):
            key.public_key().verify(
                tampered[16 : 16 + sig_size],
                hashlib.sha256(tampered[16 + sig_size :]).digest(),
                ec.ECDSA(Prehashed(hashes.SHA256())),
            )


if __name__ == "__main__":
    unittest.main()
