#!/usr/bin/env python3
"""DE: Signierte IRFW-Pakete offline prüfen.
EN: Verify signed IRFW packages offline.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import Prehashed


MAGIC = b"IRFW100\0"
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PUBLIC_KEY = ROOT / "signing" / "firmware-signing-public.pem"


def verify(package: Path, public_key_path: Path) -> tuple[int, str]:
    data = package.read_bytes()
    if len(data) < 16:
        raise ValueError("package too short")
    magic, firmware_size, signature_size, reserved = struct.unpack(
        "<8sIHH", data[:16]
    )
    if magic != MAGIC or reserved != 0:
        raise ValueError("invalid package header")
    expected = 16 + signature_size + firmware_size
    if signature_size < 64 or signature_size > 80 or len(data) != expected:
        raise ValueError("invalid package sizes")
    signature = data[16 : 16 + signature_size]
    firmware = data[16 + signature_size :]
    if not firmware or firmware[0] != 0xE9:
        raise ValueError("not an ESP32 application image")
    digest = hashlib.sha256(firmware).digest()
    key = serialization.load_pem_public_key(public_key_path.read_bytes())
    try:
        key.verify(signature, digest, ec.ECDSA(Prehashed(hashes.SHA256())))
    except InvalidSignature as exc:
        raise ValueError("invalid firmware signature") from exc
    return firmware_size, digest.hex().upper()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("package", type=Path)
    parser.add_argument("--public-key", type=Path, default=DEFAULT_PUBLIC_KEY)
    args = parser.parse_args()
    try:
        size, digest = verify(args.package.resolve(), args.public_key.resolve())
    except (OSError, ValueError) as error:
        print(f"Signature: INVALID ({error})", file=sys.stderr)
        raise SystemExit(1)
    print("Signature: VALID")
    print(f"Firmware bytes: {size}")
    print(f"Firmware SHA-256: {digest}")


if __name__ == "__main__":
    main()
