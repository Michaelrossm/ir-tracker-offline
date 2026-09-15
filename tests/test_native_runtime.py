"""Run actual C++ paths on the host, using existing C++ build tools."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeRuntimeTests(unittest.TestCase):
    def test_history_minute_shrink(self):
        self._run_source("history_minute_shrink.cpp")

    def test_history_plausibility(self):
        self._run_source("history_plausibility.cpp")

    def test_history_five_minute(self):
        self._run_source("history_five_minute.cpp")

    def test_history_conflicts(self):
        self._run_source("history_conflicts.cpp")

    def test_history_shrink(self):
        self._run_source("history_shrink.cpp")

    def test_history_compact_writer(self):
        self._run_source("history_compact_writer.cpp")

    def test_history_retention(self):
        self._run_source("history_retention.cpp")

    def test_history_capacity(self):
        self._run_source("history_capacity.cpp")

    def test_history_compact_stage(self):
        self._run_source("history_compact_stage.cpp")

    def test_history_compact_reader(self):
        self._run_source("history_compact_reader.cpp")

    def test_history_block_codec(self):
        self._run_source("history_block_codec.cpp")

    def test_parser_and_modbus_runtime(self):
        self._run_source("runtime_regressions.cpp")

    @unittest.skipUnless(os.name == "nt", "rollback simulation uses Windows BCrypt SHA-256")
    def test_asset_rollback_power_cuts(self):
        self._run_source("asset_rollback.cpp")

    def _run_source(self, name):
        source = ROOT / "tests/native" / name
        includes = ROOT / "tests/native"
        with tempfile.TemporaryDirectory(prefix="irtracker-native-") as temp:
            executable = Path(temp) / "runtime.exe"
            # Compile selected production functions directly, avoiding unrelated
            # ESP32 HTTP/security dependencies in the host executable.
            security = (ROOT / "src/app/core/SecurityManager.cpp").read_text(encoding="utf-8")
            mqtt = (ROOT / "src/app/network/MqttManager.cpp").read_text(encoding="utf-8")
            selected = security[security.index("String jsonEscape("):security.index("String htmlEscape(")]
            selected += mqtt[mqtt.index("void publishMqttValues()"):mqtt.index("void manageMqtt()")]
            (Path(temp) / "runtime_selected.inc").write_text(selected, encoding="utf-8")
            if shutil.which("g++"):
                command = ["g++", "-std=c++17", "-O2", "-I", str(includes), "-I", temp,
                           str(source), "-o", str(executable)]
            elif os.name == "nt":
                vswhere = Path(os.environ.get("ProgramFiles(x86)", "")) / "Microsoft Visual Studio/Installer/vswhere.exe"
                if not vswhere.exists():
                    self.skipTest("C++ compiler unavailable")
                install = subprocess.check_output(
                    [str(vswhere), "-latest", "-products", "*", "-requires",
                     "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                     "-property", "installationPath"], text=True).strip()
                if not install:
                    self.skipTest("MSVC C++ tools unavailable")
                setup = Path(install) / "VC/Auxiliary/Build/vcvars64.bat"
                command = (f'cmd /d /s /c ""{setup}" >nul && '
                           f'cl /nologo /std:c++17 /EHsc /O2 /utf-8 '
                           f'/I"{includes}" /I"{temp}" "{source}" /Fe:"{executable}""')
            else:
                self.skipTest("C++ compiler unavailable")
            built = subprocess.run(command, cwd=temp, capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
            result = subprocess.run([str(executable)], cwd=temp,
                                    capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("PASS:", result.stdout)
