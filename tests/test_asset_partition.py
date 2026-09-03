import hashlib
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from asset_manifest import (  # noqa: E402
    ENTRY, HEADER, HEADER_SIZE, IMAGE_SIZE, MAGIC,
    REQUIRED_ASSETS, validate_asset_image, validate_asset_tree,
)


VERSION = "1.3.5-beta.3"
PAYLOAD = b"verified maintenance javascript"
PAYLOADS = {
    name: (PAYLOAD if name == "maintenance.js.gz" else
           ("verified " + name).encode())
    for name in REQUIRED_ASSETS
}


class AssetPartitionScenarioTests(unittest.TestCase):
    def setUp(self):
        self.temp = Path(tempfile.mkdtemp(prefix="ir-asset-test-"))
        self.assets = self.temp / "assets"
        self.assets.mkdir()
        self.asset = self.assets / "maintenance.js.gz"
        for name, payload in PAYLOADS.items():
            (self.assets / name).write_bytes(payload)
        self.manifest = self.assets / "manifest.json"
        self.write_manifest()

    def tearDown(self):
        shutil.rmtree(self.temp)

    def write_manifest(self, *, version=VERSION, size=len(PAYLOAD), sha=None):
        document = {
            "schema": 1,
            "assets_version": version,
            "files": {
                name: {
                    "size": size if name == "maintenance.js.gz" else len(payload),
                    "sha256": (sha if name == "maintenance.js.gz" and sha else
                               hashlib.sha256(payload).hexdigest()),
                }
                for name, payload in PAYLOADS.items()
            },
        }
        self.manifest.write_text(json.dumps(document), encoding="utf-8")

    def write_raw_image(self, *, version=VERSION, payload=PAYLOAD,
                        expected_sha=None):
        image = bytearray(b"\xff" * IMAGE_SIZE)
        header = bytearray(HEADER_SIZE)
        version_bytes = version.encode()
        HEADER.pack_into(
            header, 0, MAGIC, 1, HEADER_SIZE, IMAGE_SIZE,
            version_bytes + b"\0" * (32 - len(version_bytes)),
            len(PAYLOADS), 0,
        )
        offset = HEADER_SIZE
        for index, (asset_name, default_payload) in enumerate(sorted(PAYLOADS.items())):
            current_payload = payload if asset_name == "maintenance.js.gz" else default_payload
            name = asset_name.encode()
            sha = (expected_sha if asset_name == "maintenance.js.gz" and expected_sha
                   else hashlib.sha256(current_payload).digest())
            ENTRY.pack_into(
                header, HEADER.size + index * ENTRY.size,
                name + b"\0" * (32 - len(name)), offset,
                len(current_payload), sha,
            )
            image[offset:offset + len(current_payload)] = current_payload
            offset += len(current_payload)
        image[:HEADER_SIZE] = header
        path = self.temp / "asset.bin"
        path.write_bytes(image)
        return path

    def assert_fallback(self, expected_error):
        result = validate_asset_tree(self.temp, VERSION)
        self.assertFalse(result.valid)
        self.assertEqual(result.maintenance_source, "embedded_fallback")
        self.assertEqual(result.error, expected_error)

    def test_valid_manifest_uses_partition(self):
        result = validate_asset_tree(self.temp, VERSION)
        self.assertTrue(result.valid)
        self.assertEqual(result.version, VERSION)
        self.assertEqual(result.maintenance_source, "partition")
        self.assertEqual(result.error, "")

    def test_missing_partition_uses_fallback(self):
        result = validate_asset_tree(
            self.temp, VERSION, partition_available=False
        )
        self.assertFalse(result.valid)
        self.assertEqual(result.maintenance_source, "embedded_fallback")
        self.assertEqual(result.error, "partition_missing")

    def test_invalid_manifest_uses_fallback(self):
        self.manifest.write_text("{invalid", encoding="utf-8")
        self.assert_fallback("manifest_invalid")

    def test_wrong_version_uses_fallback(self):
        self.write_manifest(version="0.0.0-test")
        self.assert_fallback("version_mismatch")

    def test_wrong_size_uses_fallback(self):
        self.write_manifest(size=len(PAYLOAD) + 1)
        self.assert_fallback("size_mismatch")

    def test_wrong_sha256_uses_fallback(self):
        self.write_manifest(sha="0" * 64)
        self.assert_fallback("sha256_mismatch")

    def test_missing_file_uses_fallback(self):
        self.asset.unlink()
        self.assert_fallback("file_missing")

    def test_web_route_uses_compact_recovery_fallback(self):
        security = (ROOT / "src/app/core/SecurityManager.cpp").read_text(
            encoding="utf-8"
        )
        web_ui = (ROOT / "src/app/web/WebUi.cpp").read_text(encoding="utf-8")
        self.assertIn("debugStorage.noteAssetServed(compressedPath.c_str(), false)", security)
        self.assertIn("String recoveryPage()", web_ui)
        self.assertIn("Asset-Image installieren", web_ui)

    def test_raw_image_is_exactly_64_kb_and_validated(self):
        image = self.write_raw_image()
        result = validate_asset_image(image, VERSION)
        self.assertEqual(image.stat().st_size, 65536)
        self.assertTrue(result.valid)
        self.assertEqual(result.maintenance_source, "partition")

    def test_corrupt_raw_image_uses_fallback(self):
        image = self.write_raw_image(expected_sha=b"\0" * 32)
        result = validate_asset_image(image, VERSION)
        self.assertFalse(result.valid)
        self.assertEqual(result.error, "sha256_mismatch")
        self.assertEqual(result.maintenance_source, "embedded_fallback")

    def test_wifi_writer_is_locked_to_existing_fixed_layout(self):
        storage = (ROOT / "src/app/storage/DebugStorage.cpp").read_text(
            encoding="utf-8"
        )
        updater = (ROOT / "src/app/update/OtaManager.cpp").read_text(
            encoding="utf-8"
        )
        routes = (ROOT / "src/app/web/WebApi.cpp").read_text(encoding="utf-8")
        for fixed in (
            "0x2B0000U", "0x10000U", "0x2C0000U", "0x140000U",
            "0x10000U, 0x150000U", "0x160000U, 0x150000U",
        ):
            self.assertIn(fixed, storage)
        self.assertIn("if (!fixedLayoutValid_ || !assetPartition_) return false",
                      storage)
        self.assertIn("esp_partition_erase_range(assetPartition_", storage)
        self.assertNotIn("0x8000", updater)
        self.assertIn("BACKUP-VERIFIED-0x2B0000-0x10000", updater)
        self.assertIn("assetRawUpload.written == 0x10000U", updater)
        self.assertIn("X-Asset-SHA256", routes)
        self.assertIn("X-CSRF-Token", routes)


if __name__ == "__main__":
    unittest.main()
