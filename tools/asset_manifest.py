#!/usr/bin/env python3
"""Independent validator for generated IR Tracker asset trees."""

from __future__ import annotations

import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path


IMAGE_SIZE = 0x10000
HEADER_SIZE = 1024
MAGIC = b"IRASSET1"
HEADER = struct.Struct("<8sHHI32sHH")
ENTRY = struct.Struct("<32sII32s")
REQUIRED_ASSETS = {
    "common.css.gz", "common.js.gz", "i18n.js.gz", "dashboard.js.gz",
    "history.js.gz", "maintenance.js.gz", "diagnostics.js.gz",
    "setup.html.gz", "setup.js.gz",
}


@dataclass(frozen=True)
class ValidationResult:
    valid: bool
    version: str
    error: str

    @property
    def maintenance_source(self) -> str:
        return "partition" if self.valid else "embedded_fallback"


def validate_asset_tree(root: Path, firmware_version: str,
                        *, partition_available: bool = True) -> ValidationResult:
    """Validate the on-disk tree with the same externally visible rules as firmware."""
    if not partition_available:
        return ValidationResult(False, "", "partition_missing")
    manifest_path = root / "assets" / "manifest.json"
    try:
        raw = manifest_path.read_bytes()
        if not raw or len(raw) > 4096:
            raise ValueError("manifest size")
        manifest = json.loads(raw)
    except (OSError, ValueError, TypeError):
        return ValidationResult(False, "", "manifest_invalid")

    if manifest.get("schema") != 1 or not isinstance(manifest.get("files"), dict):
        return ValidationResult(False, "", "manifest_invalid")
    files = manifest["files"]
    if set(files) != REQUIRED_ASSETS:
        return ValidationResult(False, "", "manifest_invalid")

    version = manifest.get("assets_version")
    if not isinstance(version, str):
        return ValidationResult(False, "", "manifest_invalid")
    if version != firmware_version:
        return ValidationResult(False, version, "version_mismatch")

    for name, metadata in files.items():
        if not isinstance(name, str) or not isinstance(metadata, dict):
            return ValidationResult(False, version, "manifest_invalid")
        expected_size = metadata.get("size")
        expected_sha256 = metadata.get("sha256")
        if (not isinstance(expected_size, int) or expected_size <= 0 or
                not isinstance(expected_sha256, str) or
                len(expected_sha256) != 64):
            return ValidationResult(False, version, "manifest_invalid")
        asset = root / "assets" / name
        try:
            payload = asset.read_bytes()
        except OSError:
            return ValidationResult(False, version, "file_missing")
        if len(payload) != expected_size:
            return ValidationResult(False, version, "size_mismatch")
        if hashlib.sha256(payload).hexdigest() != expected_sha256:
            return ValidationResult(False, version, "sha256_mismatch")

    return ValidationResult(True, version, "")


def validate_asset_image(path: Path, firmware_version: str) -> ValidationResult:
    """Validate the compact raw image consumed directly by the ESP partition API."""
    try:
        image = path.read_bytes()
    except OSError:
        return ValidationResult(False, "", "partition_missing")
    if len(image) != IMAGE_SIZE:
        return ValidationResult(False, "", "size_mismatch")
    try:
        magic, schema, header_size, image_size, raw_version, count, _ = (
            HEADER.unpack_from(image)
        )
    except struct.error:
        return ValidationResult(False, "", "manifest_invalid")
    if (magic != MAGIC or schema != 1 or header_size != HEADER_SIZE or
            image_size != IMAGE_SIZE or count != len(REQUIRED_ASSETS)):
        return ValidationResult(False, "", "manifest_invalid")
    version = raw_version.split(b"\0", 1)[0].decode("utf-8", errors="replace")
    if version != firmware_version:
        return ValidationResult(False, version, "version_mismatch")
    found = set()
    for index in range(count):
        try:
            raw_name, offset, size, expected_sha = ENTRY.unpack_from(
                image, HEADER.size + index * ENTRY.size
            )
        except struct.error:
            return ValidationResult(False, version, "manifest_invalid")
        name = raw_name.split(b"\0", 1)[0]
        try:
            decoded_name = name.decode("utf-8")
        except UnicodeDecodeError:
            return ValidationResult(False, version, "manifest_invalid")
        if (decoded_name not in REQUIRED_ASSETS or decoded_name in found or
                offset < HEADER_SIZE or size <= 0 or offset > IMAGE_SIZE or (
                size > IMAGE_SIZE - offset)):
            return ValidationResult(False, version, "manifest_invalid")
        found.add(decoded_name)
        payload = image[offset:offset + size]
        if hashlib.sha256(payload).digest() != expected_sha:
            return ValidationResult(False, version, "sha256_mismatch")
    if found != REQUIRED_ASSETS:
        return ValidationResult(False, version, "file_missing")
    return ValidationResult(True, version, "")
