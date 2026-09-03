#!/usr/bin/env python3
"""Build and verify the optional 64-kB LittleFS web-asset image."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IMAGE_SIZE = 0x10000


def firmware_version() -> str:
    source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
    match = re.search(r'kFirmwareVersion\[\]\s*=\s*"([^"]+)"', source)
    if not match:
        raise SystemExit("kFirmwareVersion not found")
    return match.group(1)


def find_mklittlefs() -> Path:
    package = Path.home() / ".platformio" / "packages" / "tool-mklittlefs"
    names = ("mklittlefs.exe", "mklittlefs")
    for name in names:
        candidates = list(package.rglob(name)) if package.is_dir() else []
        if candidates:
            return candidates[0]
    raise SystemExit("mklittlefs is unavailable; install the PlatformIO package first")


def run_checked(command: list[str]) -> str:
    result = subprocess.run(
        command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, check=False
    )
    print(result.stdout, end="")
    lowered = result.stdout.lower()
    if result.returncode or "no more free space" in lowered or "error adding" in lowered:
        raise SystemExit("asset image build failed")
    return result.stdout


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--environment", default="solakon_tracker_offline")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    version = firmware_version()
    run_checked([
        sys.executable, "-m", "platformio", "run", "-e", args.environment
    ])
    source = (
        ROOT / ".pio" / "build" / args.environment / "generated" /
        "asset-partition"
    )
    output = args.output or (
        ROOT / "release" / f"ir-tracker-assets-{version}.bin"
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    tool = find_mklittlefs()
    run_checked([
        str(tool), "-c", str(source), "-b", "4096", "-p", "256",
        "-s", str(IMAGE_SIZE), str(output)
    ])
    listing = run_checked([str(tool), "-l", str(output)])
    if output.stat().st_size != IMAGE_SIZE:
        raise SystemExit("asset image has an unexpected size")
    for required in ("/assets/manifest.json", "/assets/maintenance.js.gz"):
        if required not in listing:
            raise SystemExit(f"asset image is missing {required}")
    print(f"Verified asset image: {output} ({IMAGE_SIZE} bytes)")


if __name__ == "__main__":
    main()
