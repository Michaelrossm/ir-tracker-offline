#!/usr/bin/env python3
"""DE: Öffentliches ZIP bauen und private/proprietäre Daten abweisen.
EN: Build a public ZIP and reject private/proprietary material.
"""

from __future__ import annotations

import argparse
import hashlib
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


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
    ]
    documents = [
        ROOT / "README.md",
        ROOT / "LICENSE.md",
        ROOT / "THIRD_PARTY_NOTICES.md",
        ROOT / "TRADEMARKS.md",
        ROOT / "CHANGELOG.md",
        ROOT / "INSTALLATION.md",
        ROOT / "SECURITY.md",
        ROOT / "INTERFACES.md",
        ROOT / "USB_SWITCHING.md",
        ROOT / "SOAK_TEST.md",
        ROOT / "RIGHTS_REVIEW.md",
        ROOT / "RELEASE_CHECKLIST.md",
        ROOT / "HARDWARE_TEST.md",
        ROOT / "DEPENDENCIES.lock",
        ROOT / "signing" / "firmware-signing-public.pem",
        ROOT / "tools" / "flash-custom.ps1",
        ROOT / "tools" / "restore-original.ps1",
        ROOT / "tools" / "sign-firmware.py",
        ROOT / "tools" / "verify-firmware-package.py",
        ROOT / "tools" / "soak-test.py",
        ROOT / "tests" / "http_functional_test.py",
        ROOT / "tests" / "http_security_test.py",
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
    sums = "\n".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest().upper()}  {path.name}"
        for path in firmware_files
    ) + "\n"
    zip_path = release / f"ir-tracker-custom-{version}-public.zip"
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in files:
            if path.parent == ROOT / "licenses":
                target = f"licenses/{path.name}"
            else:
                target = path.name
            archive.write(path, target)
        archive.writestr("SHA256SUMS.txt", sums)
    print(zip_path)
    print(hashlib.sha256(zip_path.read_bytes()).hexdigest().upper())


if __name__ == "__main__":
    main()
