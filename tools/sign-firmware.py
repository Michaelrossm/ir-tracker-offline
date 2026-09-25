#!/usr/bin/env python3
"""Create/sign IR Tracker firmware packages and keep the embedded OTA public
key synchronized with signing/private/firmware-signing-key.pem.

The private ECDSA P-256 key remains outside Git.  The generated public key is
written to both signing/firmware-signing-public.pem and the exact header used
by the firmware: src/app/update/FirmwareSigningPublicKey.h.
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
PUBLIC_HEADER = ROOT / "src" / "app" / "update" / "FirmwareSigningPublicKey.h"
MAGIC = b"IRFW100\0"


def write_public_material(key) -> None:
    public_pem = key.public_key().public_bytes(
        serialization.Encoding.PEM,
        serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    PUBLIC_KEY.parent.mkdir(parents=True, exist_ok=True)
    PUBLIC_KEY.write_bytes(public_pem)

    pem_text = public_pem.decode("ascii").rstrip()
    lines = [
        "#pragma once",
        "",
        "// DE: Erzeugter öffentlicher OTA-Prüfschlüssel; der private Schlüssel wird nie versioniert.",
        "// EN: Generated public OTA verification key; the private key is never committed.",
        'constexpr char kFirmwareSigningPublicKey[] = R"PEM(',
        pem_text,
        ')PEM";',
        "",
    ]
    PUBLIC_HEADER.parent.mkdir(parents=True, exist_ok=True)
    PUBLIC_HEADER.write_text(
        "\n".join(lines), encoding="utf-8", newline="\n"
    )


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

    write_public_material(key)
    print(f"Public key:    {PUBLIC_KEY}")
    print(f"Public header: {PUBLIC_HEADER}")
    print(
        f"Private key:   {PRIVATE_KEY} "
        "(SECRET / GEHEIM; OFFLINE BACKUP)"
    )


def verify_key_material() -> None:
    if not PRIVATE_KEY.is_file():
        raise SystemExit(
            "Privater Schlüssel fehlt. / Private key missing: "
            f"{PRIVATE_KEY}"
        )

    key = serialization.load_pem_private_key(PRIVATE_KEY.read_bytes(), None)
    expected_der = key.public_key().public_bytes(
        serialization.Encoding.DER,
        serialization.PublicFormat.SubjectPublicKeyInfo,
    )

    if not PUBLIC_KEY.is_file():
        raise SystemExit(f"Öffentlicher Schlüssel fehlt: {PUBLIC_KEY}")
    public = serialization.load_pem_public_key(PUBLIC_KEY.read_bytes())
    actual_der = public.public_bytes(
        serialization.Encoding.DER,
        serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    if actual_der != expected_der:
        raise SystemExit(
            "FEHLER: Private Key und signing/firmware-signing-public.pem "
            "gehören nicht zusammen."
        )

    if not PUBLIC_HEADER.is_file():
        raise SystemExit(f"Firmware-Public-Key-Header fehlt: {PUBLIC_HEADER}")
    header = PUBLIC_HEADER.read_text(encoding="utf-8")
    begin = header.find("-----BEGIN PUBLIC KEY-----")
    end = header.find("-----END PUBLIC KEY-----")
    if begin < 0 or end < begin:
        raise SystemExit("FEHLER: Public Key im Firmware-Header nicht gefunden.")
    end += len("-----END PUBLIC KEY-----")
    embedded = serialization.load_pem_public_key(
        (header[begin:end] + "\n").encode("ascii")
    )
    embedded_der = embedded.public_bytes(
        serialization.Encoding.DER,
        serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    if embedded_der != expected_der:
        raise SystemExit(
            "FEHLER: Der in der Firmware eingebettete Public Key passt "
            "nicht zum privaten Signierschlüssel."
        )

    print("Signing keys: OK (private/public/firmware header match)")


def sign(input_path: Path, output_path: Path) -> None:
    verify_key_material()
    firmware = input_path.read_bytes()
    if len(firmware) < 1024 or firmware[0] != 0xE9:
        raise SystemExit(
            "Keine gültige ESP32-Anwendung. / "
            "Input does not look like an ESP32 application image."
        )

    digest = hashlib.sha256(firmware).digest()
    key = serialization.load_pem_private_key(PRIVATE_KEY.read_bytes(), None)
    signature = key.sign(
        digest, ec.ECDSA(Prehashed(hashes.SHA256()))
    )
    if len(signature) > 80:
        raise SystemExit(
            "Unerwartete ECDSA-Signaturlänge. / "
            "Unexpected ECDSA signature length."
        )

    header = struct.pack("<8sIHH", MAGIC, len(firmware), len(signature), 0)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(header + signature + firmware)
    print(f"Package: {output_path}")
    print(f"Firmware SHA-256: {digest.hex().upper()}")
    print(
        "Package SHA-256:  "
        + hashlib.sha256(output_path.read_bytes()).hexdigest().upper()
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--init",
        action="store_true",
        help="Schlüssel erzeugen/wiederverwenden und Public-Key-Dateien synchronisieren",
    )
    parser.add_argument(
        "--sync",
        action="store_true",
        help="Public-Key-Dateien aus vorhandenem Private Key synchronisieren",
    )
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--sign", type=Path, metavar="FIRMWARE_BIN")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    if args.init:
        initialise()

    if args.sync:
        if not PRIVATE_KEY.is_file():
            raise SystemExit(f"Privater Schlüssel fehlt: {PRIVATE_KEY}")
        key = serialization.load_pem_private_key(
            PRIVATE_KEY.read_bytes(), password=None
        )
        write_public_material(key)
        print("Public key + firmware header synchronized.")

    if args.verify:
        verify_key_material()

    if args.sign:
        if not args.output:
            parser.error("--output is required with --sign")
        sign(args.sign.resolve(), args.output.resolve())

    if not (args.init or args.sync or args.verify or args.sign):
        parser.error("use --init, --sync, --verify and/or --sign")


if __name__ == "__main__":
    main()
