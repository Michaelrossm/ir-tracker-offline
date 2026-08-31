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


def post_json(base: str, path: str, auth: str, payload: dict) -> tuple[int, bytes]:
    request = urllib.request.Request(
        base.rstrip("/") + path,
        data=json.dumps(payload).encode(),
        headers={
            "Authorization": "Basic " + auth,
            "Content-Type": "application/json",
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
        "firmware", "author", "license", "power_w", "import_kwh",
        "export_kwh", "phases", "meter_fresh", "uptime_s", "restart_reason",
    ):
        assert field in status, f"status field missing: {field}"
    assert len(status["phases"]) == 3

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

    code, body, _ = get(args.base, "/v1/json", auth)
    assert code == 200
    eco_tracker = json.loads(body)
    for field in ("power", "powerAvg", "energyCounterIn", "agePower"):
        assert field in eco_tracker, f"EcoTracker field missing: {field}"
    assert "energyCounterInT1" not in eco_tracker
    assert "energyCounterInT2" not in eco_tracker

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
         "params": {"id": 0}},
    )
    rpc = json.loads(body)
    assert code == 200 and rpc["id"] == 7 and "result" in rpc

    print("HTTP functional acceptance tests: PASS")


if __name__ == "__main__":
    main()
