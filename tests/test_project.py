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
SOURCE = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
I18N_SOURCE = (ROOT / "web" / "i18n.js").read_text(encoding="utf-8")
COMMON_JS_SOURCE = (ROOT / "web" / "common.js").read_text(encoding="utf-8")
COMMON_CSS_SOURCE = (ROOT / "web" / "common.css").read_text(encoding="utf-8")
DASHBOARD_JS_SOURCE = (ROOT / "web" / "dashboard.js").read_text(encoding="utf-8")
HISTORY_JS_SOURCE = (ROOT / "web" / "history.js").read_text(encoding="utf-8")
WEB_RUNTIME_SOURCE = "\n".join(
    (SOURCE, COMMON_JS_SOURCE, COMMON_CSS_SOURCE, DASHBOARD_JS_SOURCE, HISTORY_JS_SOURCE)
)
HISTORY_SOURCE = (ROOT / "src" / "HistoryStore.cpp").read_text(encoding="utf-8")
HISTORY_HEADER = (ROOT / "src" / "HistoryStore.h").read_text(encoding="utf-8")
EVENT_SOURCE = (ROOT / "src" / "EventLog.cpp").read_text(encoding="utf-8")
EVENT_HEADER = (ROOT / "src" / "EventLog.h").read_text(encoding="utf-8")


class ProjectSecurityTests(unittest.TestCase):
    def test_beta_version_and_bilingual_ui_are_embedded(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn('kFirmwareVersion[] = "1.0.2-beta.1"', source)
        self.assertIn("id='langToggle'", source)
        self.assertIn("/assets/i18n.js", source)
        self.assertIn("irtracker-language-v1", I18N_SOURCE)
        self.assertIn("URLSearchParams(location.search).get('lang')", I18N_SOURCE)
        self.assertIn('"Einstellungen":"Settings"', I18N_SOURCE)
        self.assertIn("Michael Roßmann", source)

    def test_original_backups_are_ignored(self) -> None:
        ignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
        self.assertIn("original bin/", ignore)
        self.assertIn("signing/private/", ignore)

    def test_required_hardening_is_present(self) -> None:
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
            "dBindCursor(pc);dBindCursor(dc)",
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
            "new AbortController()",
            "request!==historyRequest",
            "generation!==dLoadGeneration",
            "function dStopLoads()",
            "function stopHistoryLoads()",
            "addEventListener('pagehide',dStopLoads)",
            "addEventListener('pagehide',stopHistoryLoads)",
            "if (!responseClient.connected()) return false",
            "localStorage.getItem('irtracker-theme-v1')",
            "localStorage.setItem(key,JSON.stringify(clean))",
            "data-theme-var='--card'",
            "target=dFrom+f*(dTo-dFrom)",
            "class='chart-section'",
            "width:min(100%,1800px)",
            "height:clamp(390px,55vh,680px)",
            "height:clamp(350px,42vh,560px)",
            "px.lineWidth=2",
            "dx.lineWidth=2",
            "cx.lineWidth=2",
            "calendarHistoryQuery",
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
            "name='event_flash'",
            "kCpuBoostHoldMs = 2UL * 60UL * 1000UL",
            "kEcoCpuMhz = 80",
            "kPerformanceCpuMhz = 160",
            "prefs.getBool(\"eco_mode\", true)",
            "name='eco_mode'",
            "requestCpuBoost(\"history_export\")",
            "requestCpuBoost(\"firmware_update\")",
            "manageCpuPowerMode()",
            "cpu_boost_remaining_s",
            "cpu_frequency_errors",
            "setInterval(refreshLive,4000)",
            "if(!document.hidden)updateValues()",
            "if(!document.hidden)updateDashboardSummary()",
            "prefs.getBool(\"eco_led_off\", true)",
            "name='eco_led_off'",
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
            "kWifiPowerStableMs = 3UL * 60UL * 1000UL",
            "kWifiPowerEvaluateMs = 60UL * 1000UL",
            "WIFI_POWER_19_5dBm",
            "WIFI_POWER_15dBm",
            "WIFI_POWER_11dBm",
            "prefs.getBool(\"wifi_power_auto\", true)",
            "name='wifi_power_auto'",
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
        self.assertNotIn('requestCpuBoost("dashboard_summary")', SOURCE)
        self.assertIn("Record records_[kCapacity]", EVENT_HEADER)
        self.assertIn("static constexpr uint32_t kCapacity = 256", EVENT_HEADER)
        self.assertIn("bool setPersistence(bool persistent)", EVENT_HEADER)
        self.assertIn("appendRam(record)", EVENT_SOURCE)
        self.assertIn("return !persistent_ || writePersistent(record)", EVENT_SOURCE)

    def test_theme_stays_browser_local(self) -> None:
        theme_script = COMMON_JS_SOURCE
        self.assertEqual(SOURCE.count("data-theme-var="), 2)
        self.assertIn("['--bg','--card'].forEach", theme_script)
        self.assertIn("const defaults={'--bg':'#07100c','--card':'#10231a'}", theme_script)
        self.assertIn("localStorage.getItem", theme_script)
        self.assertIn("localStorage.setItem", theme_script)
        self.assertIn("localStorage.removeItem", theme_script)
        self.assertNotIn("fetch(", theme_script)
        self.assertNotIn("XMLHttpRequest", theme_script)

    def test_navigation_and_maintenance_are_reduced(self) -> None:
        nav_start = SOURCE.index("String nav()")
        nav_end = SOURCE.index("String maintenanceTabs", nav_start)
        navigation = SOURCE[nav_start:nav_end]
        self.assertNotIn("/diagnostics", navigation)
        self.assertNotIn("JSON API", navigation)
        self.assertIn("/maintenance/diagnostics", SOURCE)
        self.assertIn("JSON-API f", SOURCE)
        self.assertIn("Content-Encoding", SOURCE)
        self.assertIn("kI18nJsGzip", SOURCE)
        self.assertIn("kCommonCssGzip", SOURCE)
        self.assertIn("kCommonJsGzip", SOURCE)
        self.assertIn("kDashboardJsGzip", SOURCE)
        self.assertIn("kHistoryJsGzip", SOURCE)
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
        self.assertIn("async function loadUpdate()", SOURCE)
        self.assertIn("Die installierte Firmware ist aktuell.", SOURCE)
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
        self.assertIn("maxlength='32'", SOURCE)
        self.assertIn("maxlength='64' autocomplete='off'", SOURCE)
        ota_start = SOURCE.index("void handleOtaFinished()")
        ota_end = SOURCE.index("void handleSafeShutdown()", ota_start)
        ota_handler = SOURCE[ota_start:ota_end]
        self.assertIn("Firmwareupdate abgelehnt", ota_handler)
        self.assertNotIn('"application/json"', ota_handler)

    def test_embedded_translation_asset_is_valid_gzip(self) -> None:
        header = (ROOT / "src" / "WebAssets.h").read_text(encoding="utf-8")
        assets = {
            "kCommonCssGzip": COMMON_CSS_SOURCE,
            "kCommonJsGzip": COMMON_JS_SOURCE,
            "kI18nJsGzip": I18N_SOURCE,
            "kDashboardJsGzip": DASHBOARD_JS_SOURCE,
            "kHistoryJsGzip": HISTORY_JS_SOURCE,
        }
        for symbol, source in assets.items():
            match = re.search(
                rf"{symbol}\[\] PROGMEM = \{{(.*?)\n\}};", header, re.DOTALL
            )
            self.assertIsNotNone(match, symbol)
            payload = bytes(
                int(value, 16) for value in re.findall(r"0x([0-9a-f]{2})", match.group(1))
            )
            expected = source.replace("\r\n", "\n").encode("utf-8")
            self.assertEqual(gzip.decompress(payload), expected, symbol)
            self.assertLess(len(payload), len(expected), symbol)

    def test_large_pages_use_cached_external_assets(self) -> None:
        dashboard = SOURCE[SOURCE.index("void handleRoot()") : SOURCE.index("void handleHistoryPage()")]
        history = SOURCE[SOURCE.index("void handleHistoryPage()") : SOURCE.index("struct HistoryQuery")]
        self.assertIn("/assets/dashboard.js?v=", dashboard)
        self.assertIn("/assets/history.js?v=", history)
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
        self.assertTrue((ROOT / "THIRD_PARTY_NOTICES.md").exists())

    def test_committed_public_key_matches_header(self) -> None:
        pem = (ROOT / "signing" / "firmware-signing-public.pem").read_text()
        header = (ROOT / "src" / "FirmwareSigningPublicKey.h").read_text()
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
