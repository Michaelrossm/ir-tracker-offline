"""Use the firmware's rollback reservation as the single source of truth."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
_header = (ROOT / "src/app/update/AssetRollback.h").read_text(encoding="utf-8")
APP_MAX_BYTES = int(re.search(r"kAppLimit\s*=\s*(0x[0-9A-Fa-f]+)", _header)[1], 16)

def check_app_size(path):
    size = Path(path).stat().st_size
    if (size + 4095) // 4096 * 4096 > APP_MAX_BYTES:
        raise ValueError("Firmware overlaps the reserved asset rollback journal")
    return APP_MAX_BYTES - size
