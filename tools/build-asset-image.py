#!/usr/bin/env python3
"""Build and verify the optional 64-kB LittleFS web-asset image."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

from asset_manifest import (
    ENTRY,
    HEADER,
    HEADER_SIZE,
    IMAGE_SIZE,
    MAGIC,
    validate_asset_image,
    validate_asset_tree,
)


ROOT = Path(__file__).resolve().parents[1]
def firmware_version() -> str:
    source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
    match = re.search(r'kFirmwareVersion\[\]\s*=\s*"([^"]+)"', source)
    if not match:
        raise SystemExit("kFirmwareVersion not found")
    return match.group(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--environment", default="solakon_tracker_offline")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    version = firmware_version()
    import subprocess
    subprocess.run(
        [sys.executable, "-m", "platformio", "run", "-e", args.environment],
        cwd=ROOT, check=True,
    )
    source = (
        ROOT / ".pio" / "build" / args.environment / "generated" /
        "asset-partition"
    )
    output = args.output or (
        ROOT / "release" / f"ir-tracker-assets-{version}.bin"
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    validation = validate_asset_tree(source, version)
    if not validation.valid:
        raise SystemExit(
            f"asset tree validation failed: {validation.error}"
        )
    manifest = json.loads((source / "assets" / "manifest.json").read_text(
        encoding="utf-8"
    ))
    version_bytes = version.encode("utf-8")
    if len(version_bytes) > 31:
        raise SystemExit("firmware version is too long for the asset header")
    files = sorted(manifest["files"].items())
    header = bytearray(HEADER_SIZE)
    HEADER.pack_into(
        header, 0, MAGIC, 1, HEADER_SIZE, IMAGE_SIZE,
        version_bytes + b"\0" * (32 - len(version_bytes)), len(files), 0,
    )
    image = bytearray(b"\xff" * IMAGE_SIZE)
    offset = HEADER_SIZE
    container_files = {}
    for index, (name, metadata) in enumerate(files):
        name_bytes = name.encode("utf-8")
        if len(name_bytes) > 31:
            raise SystemExit(f"asset name is too long: {name}")
        payload = (source / "assets" / name).read_bytes()
        aligned_offset = (offset + 255) & ~255
        padding = aligned_offset - offset
        offset = aligned_offset
        if offset + len(payload) > IMAGE_SIZE:
            raise SystemExit("asset image has no more free space")
        ENTRY.pack_into(
            header, HEADER.size + index * ENTRY.size,
            name_bytes + b"\0" * (32 - len(name_bytes)), offset, len(payload),
            hashlib.sha256(payload).digest(),
        )
        image[offset:offset + len(payload)] = payload
        container_files[name.removesuffix(".gz")] = {
            "offset": offset,
            "gzip": len(payload),
            "alignment_padding": padding,
            "container_contribution": len(payload) + padding,
        }
        offset += len(payload)
    image[:HEADER_SIZE] = header
    output.write_bytes(image)
    image_validation = validate_asset_image(output, version)
    if not image_validation.valid:
        raise SystemExit(
            f"asset image validation failed: {image_validation.error}"
        )
    source_report_path = (
        ROOT / ".pio" / "build" / args.environment / "generated" /
        "asset-build-report.json"
    )
    report = json.loads(source_report_path.read_text(encoding="utf-8"))
    for name, sizes in report["files"].items():
        sizes.update(container_files[name])
    report["container"] = {
        "header": HEADER_SIZE,
        "used": offset,
        "free": IMAGE_SIZE - offset,
        "image": IMAGE_SIZE,
    }
    report_path = args.report or output.with_suffix(".report.json")
    report_path.write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8", newline="\n",
    )
    print(f"Verified asset image: {output} ({IMAGE_SIZE} bytes)")
    print(f"Asset build report: {report_path} ({IMAGE_SIZE - offset} bytes free)")


if __name__ == "__main__":
    main()
