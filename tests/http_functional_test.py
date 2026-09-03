#!/usr/bin/env python3
"""DE: Nur lesende Funktionstests am geflashten Tracker.
EN: Read-only functional acceptance tests for a flashed tracker.
"""

from __future__ import annotations

import argparse
import base64
import json
import urllib.error
import urllib.request


def get(base: str, path: str, auth: str) -> tuple[int, bytes, str]:
    request = urllib.request.Request(base.rstrip("/") + path)
    request.add_header("Authorization", "Basic " + auth)
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            return response.status, response.read(), response.headers.get_content_type()
    except urllib.error.HTTPError as error:
        return error.code, error.read(), error.headers.get_content_type()


def post_json(base: str, path: str, auth: str, payload: dict,
              csrf: str = "") -> tuple[int, bytes]:
    request = urllib.request.Request(
        base.rstrip("/") + path,
        data=json.dumps(payload).encode(),
        headers={
            "Authorization": "Basic " + auth,
            "Content-Type": "application/json",
            "X-CSRF-Token": csrf,
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            return response.status, response.read()
    except urllib.error.HTTPError as error:
        return error.code, error.read()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", default="http://192.168.178.66")
    parser.add_argument("--user", default="admin")
    parser.add_argument("--password", required=True)
    args = parser.parse_args()
    auth = base64.b64encode(f"{args.user}:{args.password}".encode()).decode()

    status_code, body, _ = get(args.base, "/api/v1/status", auth)
    assert status_code == 200
    status = json.loads(body)
    for field in (
        "asset_partition_mounted", "asset_manifest_valid", "asset_version",
        "asset_source_maintenance_js", "asset_last_error",
    ):
        assert field in status, f"asset diagnostics field missing: {field}"
    assert status["asset_source_maintenance_js"] in (
        "partition", "embedded_fallback"
    )
    for field in (
        "firmware", "device_model", "device_serial", "device_mac",
        "author", "license", "power_w", "import_kwh",
        "export_kwh", "phases", "meter_fresh", "uptime_s", "restart_reason",
        "sml_crc_errors", "d0_bcc_errors",
    ):
        assert field in status, f"status field missing: {field}"
    assert len(status["phases"]) == 3
    assert status["device_model"] == "IRTRACKER-C3"
    assert status["device_serial"].startswith("IRT-")

    for path, expected_type in (
        ("/assets/common.css", "text/css"),
        ("/assets/common.js", "application/javascript"),
        ("/assets/i18n.js", "application/javascript"),
        ("/assets/dashboard.js", "application/javascript"),
        ("/assets/history.js", "application/javascript"),
        ("/assets/maintenance.js", "application/javascript"),
        ("/assets/diagnostics.js", "application/javascript"),
        ("/assets/setup.html", "text/html"),
        ("/assets/setup.js", "application/javascript"),
    ):
        code, payload, content_type = get(args.base, path, auth)
        assert code == 200 and payload, f"asset unavailable: {path}"
        assert content_type == expected_type, f"wrong MIME type: {path}"

    code, body, _ = get(args.base, "/api/v1/status", auth)
    served_status = json.loads(body)
    assert code == 200
    assert served_status["asset_source_maintenance_js"] == "partition"

    code, body, _ = get(args.base, "/api/v1/history?range=day", auth)
    assert code == 200
    history = json.loads(body)
    assert isinstance(history.get("values"), list)

    for path in ("/", "/history", "/setup", "/interfaces", "/maintenance",
                 "/maintenance/diagnostics"):
        code, body, content_type = get(args.base, path, auth)
        assert code == 200 and content_type == "text/html" and b"<main>" in body

    code, body, _ = get(args.base, "/metrics", auth)
    assert code == 200 and b"irtracker_power_w" in body
    assert b"irtracker_crc_errors_total" in body
    assert b"irtracker_sml_crc_errors_total" in body
    assert b"irtracker_d0_bcc_errors_total" in body

    code, body, _ = get(args.base, "/api/v1/influx", auth)
    assert code == 200 and b"crc_errors=" in body
    assert b"sml_crc_errors=" in body and b"d0_bcc_errors=" in body

    code, body, _ = get(args.base, "/api/v1/meter-report", auth)
    assert code == 200
    meter_report = json.loads(body)
    for field in ("crc_errors", "sml_crc_errors", "d0_bcc_errors"):
        assert field in meter_report, f"meter report field missing: {field}"

    code, body, _ = get(args.base, "/api/v1/support-report", auth)
    assert code == 200
    report_text = body.decode("utf-8")
    for label in (
        "Asset-Partition:", "Manifest:", "Asset-Version:",
        "maintenance.js Quelle:", "Letzter Asset-Fehler:",
    ):
        assert label in report_text, f"support report label missing: {label}"

    code, body, _ = get(args.base, "/v1/json", auth)
    assert code == 200
    eco_tracker = json.loads(body)
    for field in ("power", "powerAvg", "energyCounterIn", "agePower"):
        assert field in eco_tracker, f"EcoTracker field missing: {field}"
    assert eco_tracker["energyCounterInT1"] is None
    assert eco_tracker["energyCounterInT2"] is None
    assert eco_tracker["agePower"] is None or eco_tracker["agePower"] >= 0

    code, body, _ = get(args.base, "/api/v1/meter", auth)
    assert code == 200
    neutral_meter = json.loads(body)
    assert neutral_meter["schema"] == "irtracker.meter.v1"
    for field in ("timestamp", "fresh", "age_s", "power", "import",
                  "export", "phases", "units"):
        assert field in neutral_meter, f"neutral meter field missing: {field}"
    assert len(neutral_meter["phases"]) == 3

    code, body, _ = get(args.base, "/shelly", auth)
    assert code == 200
    device_info = json.loads(body)
    assert device_info["model"] == "IRTRACKER-C3-3EM"
    assert device_info["serial"].startswith("IRT-")
    assert device_info["mac"] == status["device_mac"]

    code, body, _ = get(args.base, "/api/v1/admin-session", auth)
    assert code == 200
    csrf = json.loads(body)["csrf_token"]

    code, body, _ = get(args.base, "/rpc/EM.GetStatus?id=0", auth)
    assert code == 200
    shelly = json.loads(body)
    assert "total_act_power" in shelly
    assert "a_current" in shelly and "errors" in shelly

    code, body, _ = get(args.base, "/rpc/EMData.GetStatus?id=0", auth)
    assert code == 200
    shelly_data = json.loads(body)
    assert "total_act" in shelly_data and "total_act_power" not in shelly_data

    code, body = post_json(
        args.base, "/rpc", auth,
        {"id": 7, "src": "acceptance-test", "method": "EM.GetStatus",
         "params": {"id": 0}}, csrf,
    )
    rpc = json.loads(body)
    assert code == 200 and rpc["id"] == 7 and "result" in rpc

    print("HTTP functional acceptance tests: PASS")


if __name__ == "__main__":
    main()
