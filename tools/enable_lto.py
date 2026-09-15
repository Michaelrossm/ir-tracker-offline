"""DE: GCC-Link-Time-Optimierung für das kleine OTA-Abbild aktivieren.
EN: Enable GCC link-time optimization for the size-constrained OTA image.
"""

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.

# DE: PlatformIO übernimmt -flto für die Kompilierung; die ältere ESP32-
# Toolchain benötigt es zusätzlich beim abschließenden Linken.
# EN: PlatformIO forwards -flto to compilation, but this older ESP32 toolchain
# also needs it explicitly on the final compiler-driver link.
env.Replace(
    LINKFLAGS=[flag for flag in env.get("LINKFLAGS", []) if flag != "-fno-lto"]
    + ["-flto"]
)

# Enforce the smaller usable app area without modifying partitions.csv.
import sys
from pathlib import Path
sys.path.insert(0, str(Path(env["PROJECT_DIR"]) / "tools"))
from update_limits import check_app_size

def check_rollback_reserve(source, target, env):
    reserve = check_app_size(str(target[0]))
    print(f"Asset rollback reserve protected; usable app headroom: {reserve} bytes")

env.AddPostAction("$BUILD_DIR/firmware.bin", check_rollback_reserve)
