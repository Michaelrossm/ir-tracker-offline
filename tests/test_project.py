from __future__ import annotations

import hashlib
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
HISTORY_SOURCE = (ROOT / "src" / "HistoryStore.cpp").read_text(encoding="utf-8")
HISTORY_HEADER = (ROOT / "src" / "HistoryStore.h").read_text(encoding="utf-8")
EVENT_SOURCE = (ROOT / "src" / "EventLog.cpp").read_text(encoding="utf-8")
EVENT_HEADER = (ROOT / "src" / "EventLog.h").read_text(encoding="utf-8")


class ProjectSecurityTests(unittest.TestCase):
    def test_beta_version_and_bilingual_ui_are_embedded(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn('kFirmwareVersion[] = "1.0.0-beta.1"', source)
        self.assertIn("id='langToggle'", source)
        self.assertIn("irtracker-language-v1", source)
        self.assertIn("URLSearchParams(location.search).get('lang')", source)
        self.assertIn('"Einstellungen":"Settings"', source)
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
            "if (meterFresh)",
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
            "script.reserve(26000)",
            "DASHBOARD_SCRIPT_MEMORY",
            "HISTORY_SCRIPT_MEMORY",
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
            "credentialSafeFetch",
            ".chart-card,.dashboard-chart,.chart-wrap{min-width:0;max-width:100%}",
            ".legend-row{gap:10px 14px}",
            'server.on("/auth/logout"',
            '"Content-Security-Policy"',
            '"X-Frame-Options", "DENY"',
            '"X-Content-Type-Options", "nosniff"',
            "void handleDiagnostics() {\n  if (!requireAdmin()) return;",
            'server.on("/system/shutdown"',
            "esp_deep_sleep_start",
        ):
            self.assertIn(marker, SOURCE)
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
        start = SOURCE.index("const key='irtracker-theme-v1'")
        end = SOURCE.index("})();</script>)JS", start)
        theme_script = SOURCE[start:end]
        self.assertEqual(SOURCE.count("data-theme-var="), 2)
        self.assertIn("['--bg','--card'].forEach", SOURCE)
        self.assertIn("const defaults={'--bg':'#07100c','--card':'#10231a'}", theme_script)
        self.assertIn("localStorage.getItem", theme_script)
        self.assertIn("localStorage.setItem", theme_script)
        self.assertIn("localStorage.removeItem", theme_script)
        self.assertNotIn("fetch(", theme_script)
        self.assertNotIn("XMLHttpRequest", theme_script)

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
