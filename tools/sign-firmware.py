#!/usr/bin/env python3
"""DE: IR-Tracker-Pakete erzeugen und signieren. Der private ECDSA-P-256-
Schlüssel liegt unter signing/private, bleibt außerhalb von Git und wird nur
offline gesichert. EN: Create and sign IR-Tracker packages. The private
ECDSA P-256 key lives under signing/private, stays out of Git and is backed up
offline only. Only the public verification key belongs in the repository.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import Prehashed


ROOT = Path(__file__).resolve().parents[1]
PRIVATE_KEY = ROOT / "signing" / "private" / "firmware-signing-key.pem"
PUBLIC_KEY = ROOT / "signing" / "firmware-signing-public.pem"
PUBLIC_HEADER = ROOT / "src" / "FirmwareSigningPublicKey.h"
MAGIC = b"IRFW100\0"


def initialise() -> None:
    if PRIVATE_KEY.exists():
        key = serialization.load_pem_private_key(
            PRIVATE_KEY.read_bytes(), password=None
        )
    else:
        PRIVATE_KEY.parent.mkdir(parents=True, exist_ok=True)
        key = ec.generate_private_key(ec.SECP256R1())
        PRIVATE_KEY.write_bytes(
            key.private_bytes(
                serialization.Encoding.PEM,
                serialization.PrivateFormat.PKCS8,
                serialization.NoEncryption(),
            )
        )

    public_pem = key.public_key().public_bytes(
        serialization.Encoding.PEM,
        serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    PUBLIC_KEY.parent.mkdir(parents=True, exist_ok=True)
    PUBLIC_KEY.write_bytes(public_pem)
    pem_text = public_pem.decode("ascii")
    lines = [
        "#pragma once",
        "",
        "// DE: Öffentlicher OTA-Prüfschlüssel; privater Schlüssel nie versionieren.",
        "// EN: Public OTA verification key; never commit the private key.",
        'constexpr char kFirmwareSigningPublicKey[] = R"PEM(',
        pem_text.rstrip(),
        ')PEM";',
        "",
    ]
    PUBLIC_HEADER.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"Public key:  {PUBLIC_KEY}")
    print(f"Public header: {PUBLIC_HEADER}")
    print(f"Private key / privater Schlüssel: {PRIVATE_KEY} (SECRET / GEHEIM; OFFLINE BACKUP)")


def sign(input_path: Path, output_path: Path) -> None:
    if not PRIVATE_KEY.exists():
        raise SystemExit("Privater Schlüssel fehlt. / Private key missing. Run with --init first.")
    firmware = input_path.read_bytes()
    if len(firmware) < 1024 or firmware[0] != 0xE9:
        raise SystemExit("Keine gültige ESP32-Anwendung. / Input does not look like an ESP32 application image.")
    digest = hashlib.sha256(firmware).digest()
    key = serialization.load_pem_private_key(PRIVATE_KEY.read_bytes(), None)
    signature = key.sign(digest, ec.ECDSA(Prehashed(hashes.SHA256())))
    if len(signature) > 80:
        raise SystemExit("Unerwartete ECDSA-Signaturlänge. / Unexpected ECDSA signature length.")
    header = struct.pack("<8sIHH", MAGIC, len(firmware), len(signature), 0)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(header + signature + firmware)
    print(f"Package: {output_path}")
    print(f"Firmware SHA-256: {digest.hex().upper()}")
    print(f"Package SHA-256:  {hashlib.sha256(output_path.read_bytes()).hexdigest().upper()}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--init", action="store_true", help="Schlüssel erzeugen/wiederverwenden / create or reuse signing key")
    parser.add_argument("--sign", type=Path, metavar="FIRMWARE_BIN")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.init:
        initialise()
    if args.sign:
        if not args.output:
            parser.error("--output is required with --sign")
        sign(args.sign.resolve(), args.output.resolve())
    if not args.init and not args.sign:
        parser.error("use --init and/or --sign")


if __name__ == "__main__":
    main()
