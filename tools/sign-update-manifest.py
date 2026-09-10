#!/usr/bin/env python3
"""Create one signed IRUP200 bundle: manifest, app image and web assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import Prehashed


ROOT = Path(__file__).resolve().parents[1]
MAGIC = b"IRUP200\0"
HEADER = struct.Struct("<8sIHH")
PRIVATE_KEY = ROOT / "signing" / "private" / "firmware-signing-key.pem"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--firmware", required=True, type=Path)
    parser.add_argument("--assets", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    firmware, assets = args.firmware.resolve(), args.assets.resolve()
    if (not firmware.is_file() or firmware.stat().st_size < 1024 or
            firmware.stat().st_size > 0x150000 or firmware.read_bytes()[:1] != b"\xE9"):
        raise SystemExit("Invalid ESP32 application image")
    if not assets.is_file() or assets.stat().st_size != 0x10000:
        raise SystemExit("Asset image must be exactly 65536 bytes")
    if not PRIVATE_KEY.is_file():
        raise SystemExit("Private signing key missing")
    app = firmware.read_bytes()
    firmware_size = len(app)
    manifest = json.dumps(
        {"schema": 2, "version": args.version,
         "firmware": {"size": firmware_size,
                      "sha256": hashlib.sha256(app).hexdigest()},
         "assets": {"size": assets.stat().st_size, "sha256": sha256(assets)}},
        separators=(",", ":"),
    ).encode("utf-8")
    key = serialization.load_pem_private_key(PRIVATE_KEY.read_bytes(), None)
    signature = key.sign(hashlib.sha256(manifest).digest(), ec.ECDSA(Prehashed(hashes.SHA256())))
    if not 64 <= len(signature) <= 80:
        raise SystemExit("Unexpected ECDSA signature length")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(
        HEADER.pack(MAGIC, len(manifest), len(signature), 0)
        + signature + manifest + app + assets.read_bytes()
    )
    print(args.output)


if __name__ == "__main__":
    main()
