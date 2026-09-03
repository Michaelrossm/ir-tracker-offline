#!/usr/bin/env python3
"""DE: Öffentliches ZIP bauen und private/proprietäre Daten abweisen.
EN: Build a public ZIP and reject private/proprietary material.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IRFW_MAGIC = b"IRFW100\0"
APP_PARTITION_BYTES = 0x150000


def validate_firmware_pair(package_path: Path, usb_path: Path) -> None:
    package = package_path.read_bytes()
    if len(package) < 16:
        raise SystemExit("IRFW-Paket ist zu kurz. / IRFW package is too short.")
    magic, firmware_size, signature_size, reserved = struct.unpack(
        "<8sIHH", package[:16]
    )
    expected_size = 16 + signature_size + firmware_size
    if (
        magic != IRFW_MAGIC
        or reserved != 0
        or not 64 <= signature_size <= 80
        or len(package) != expected_size
    ):
        raise SystemExit("IRFW-Struktur ist ungültig. / Invalid IRFW structure.")
    firmware = package[16 + signature_size :]
    usb_firmware = usb_path.read_bytes()
    if firmware != usb_firmware:
        raise SystemExit(
            "IRFW und USB-BIN stimmen nicht überein. / IRFW and USB BIN differ."
        )
    if firmware_size > APP_PARTITION_BYTES:
        raise SystemExit(
            f"Firmware ist {firmware_size - APP_PARTITION_BYTES} Byte zu groß. / "
            f"Firmware exceeds the app partition by {firmware_size - APP_PARTITION_BYTES} bytes."
        )
    print(
        f"App reserve / App-Reserve: {APP_PARTITION_BYTES - firmware_size} bytes"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    args = parser.parse_args()
    version = args.version
    release = ROOT / "release"
    firmware_files = [
        release / f"ir-tracker-custom-{version}.irfw",
        release / f"ir-tracker-custom-{version}-usb.bin",
        release / f"partitions-{version}.bin",
        release / f"ir-tracker-assets-{version}.bin",
    ]
    documents = [
        ROOT / "README.md",
        ROOT / "LICENSE.md",
        ROOT / "docs" / "legal" / "THIRD_PARTY_NOTICES.md",
        ROOT / "docs" / "legal" / "TRADEMARKS.md",
        ROOT / "CHANGELOG.md",
        ROOT / "docs" / "INSTALLATION.md",
        ROOT / ".github" / "SECURITY.md",
        ROOT / "docs" / "INTERFACES.md",
        ROOT / "docs" / "MODBUS.md",
        ROOT / "docs" / "USB_SWITCHING.md",
        ROOT / "docs" / "SOAK_TEST.md",
        ROOT / "docs" / "legal" / "RIGHTS_REVIEW.md",
        ROOT / "docs" / "RELEASE_CHECKLIST.md",
        ROOT / "docs" / "HARDWARE_TEST.md",
        ROOT / "docs" / "ARCHITECTURE.md",
        ROOT / "docs" / "ASSET_PARTITION.md",
        ROOT / "docs" / "reports" / f"FLASH_REPORT_{version}.md",
        ROOT / "docs" / "reports" / f"FLASH_SYMBOLS_BEFORE_{version}.txt",
        ROOT / "docs" / "reports" / f"FLASH_SYMBOLS_AFTER_{version}.txt",
        ROOT / "DEPENDENCIES.lock",
        ROOT / "signing" / "firmware-signing-public.pem",
        ROOT / "tools" / "flash-custom.ps1",
        ROOT / "tools" / "restore-original.ps1",
        ROOT / "tools" / "sign-firmware.py",
        ROOT / "tools" / "verify-firmware-package.py",
        ROOT / "tools" / "build-asset-image.py",
        ROOT / "tools" / "asset_manifest.py",
        ROOT / "tools" / "soak-test.py",
        ROOT / "tests" / "http_functional_test.py",
        ROOT / "tests" / "http_security_test.py",
        ROOT / "tests" / "test_asset_partition.py",
        release / f"RELEASE_NOTES-{version}.md",
        release / f"ir-tracker-assets-{version}.report.json",
    ]
    license_files = sorted((ROOT / "licenses").glob("*"))
    files = firmware_files + documents + license_files
    missing = [str(path) for path in files if not path.is_file()]
    if missing:
        raise SystemExit("Release-Dateien fehlen / Missing release files:\n" + "\n".join(missing))
    for path in files:
        lowered = str(path).lower()
        if "original bin" in lowered or "signing/private" in lowered:
            raise SystemExit(f"Private/proprietäre Datei abgewiesen / Private/proprietary file rejected: {path}")
    validate_firmware_pair(firmware_files[0], firmware_files[1])
    if firmware_files[2].stat().st_size != 3072:
        raise SystemExit(
            "Unerwartete Partitionstabellengröße. / Unexpected partition-table size."
        )
    if firmware_files[3].stat().st_size != 0x10000:
        raise SystemExit(
            "Unerwartete Asset-Partitionsgröße. / Unexpected asset-partition size."
        )
    sums = "\n".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest().upper()}  {path.name}"
        for path in firmware_files
    ) + "\n"
    zip_path = release / f"ir-tracker-custom-{version}-public.zip"
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in files:
            target = (path.name if path.parent == release
                      else path.relative_to(ROOT).as_posix())
            archive.write(path, target)
        archive.writestr("SHA256SUMS.txt", sums)
    print(zip_path)
    print(hashlib.sha256(zip_path.read_bytes()).hexdigest().upper())


if __name__ == "__main__":
    main()
