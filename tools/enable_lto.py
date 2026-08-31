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
