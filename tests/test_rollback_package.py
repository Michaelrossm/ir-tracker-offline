import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import Prehashed

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from update_limits import APP_MAX_BYTES, check_app_size
spec = importlib.util.spec_from_file_location("package_verify", ROOT / "tools/verify-firmware-package.py")
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)

class RollbackPackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "update.irup"
        self.public = Path(self.temp.name) / "public.pem"
        self.key = ec.generate_private_key(ec.SECP256R1())
        self.public.write_bytes(self.key.public_key().public_bytes(
            serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo))

    def bundle(self, size=1024, schema=2):
        app = b"\xe9" + bytes(size - 1)
        assets = bytes(65536)
        manifest = json.dumps({"schema": schema, "version": "test",
            "firmware": {"size": size, "sha256": hashlib.sha256(app).hexdigest()},
            "assets": {"size": 65536, "sha256": hashlib.sha256(assets).hexdigest()}}).encode()
        signature = self.key.sign(hashlib.sha256(manifest).digest(), ec.ECDSA(Prehashed(hashes.SHA256())))
        self.path.write_bytes(struct.pack("<8sIHH", b"IRUP200\0", len(manifest), len(signature), 0) +
                              signature + manifest + app + assets)

    def test_signed_bundle(self):
        self.bundle()
        self.assertEqual(verifier.verify(self.path, self.public)[0], 1024)

    def test_tampering_and_truncation(self):
        self.bundle()
        original = self.path.read_bytes()
        for offset in [16, len(original)-1, len(original)-65536-1]:
            corrupted = bytearray(original)
            corrupted[offset] ^= 1
            self.path.write_bytes(corrupted)
            with self.assertRaises(ValueError): verifier.verify(self.path, self.public)
        self.path.write_bytes(original[:-1])
        with self.assertRaises(ValueError): verifier.verify(self.path, self.public)

    def test_signed_oversize(self):
        self.bundle(APP_MAX_BYTES+1)
        with self.assertRaises(ValueError): verifier.verify(self.path, self.public)

    def test_bad_schema(self):
        self.bundle(schema=3)
        with self.assertRaises(ValueError): verifier.verify(self.path, self.public)

    def test_shared_limits(self):
        self.assertEqual(APP_MAX_BYTES, 0x13f000)
        self.path.write_bytes(bytes(APP_MAX_BYTES))
        self.assertEqual(check_app_size(self.path), 0)
        self.path.write_bytes(bytes(APP_MAX_BYTES+1))
        with self.assertRaises(ValueError): check_app_size(self.path)

    def test_boot_confirmation_and_geometry_guards(self):
        source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
        boot = source[source.index("void setup() {"):]
        self.assertLess(boot.index("recoverAssetTransaction()"), boot.index("debugStorage.begin"))
        self.assertLess(boot.index("esp_ota_mark_app_valid_cancel_rollback()"), boot.index("assetRollback.confirm"))
        storage = (ROOT / "src/app/storage/DebugStorage.cpp").read_text(encoding="utf-8")
        self.assertIn("observedTarget_->subtype != ESP_PARTITION_SUBTYPE_DATA_SPIFFS", storage)
        self.assertIn("0x2C0000U, 0x140000U", storage)

    def test_updater_erase_model_stays_before_journal(self):
        # Model the installed Updater.cpp 64-KiB block / 4-KiB tail strategy.
        for base in (0x10000, 0x160000):
            for size in (1024, APP_MAX_BYTES-4095, APP_MAX_BYTES-1, APP_MAX_BYTES):
                for progress in range(0, size, 4096):
                    offset = base + progress
                    block = size - progress >= 65536 and offset % 65536 == 0
                    tail = offset >= (base + size) // 65536 * 65536
                    if block or tail:
                        end = progress + (65536 if block else 4096)
                        self.assertLessEqual(end, APP_MAX_BYTES)
