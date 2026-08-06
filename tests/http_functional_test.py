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

    code, body, _ = get(args.base, "/rpc/EM.GetStatus?id=0", auth)
    assert code == 200
    shelly = json.loads(body)
    assert "total_act_power" in shelly

    print("HTTP functional acceptance tests: PASS")


if __name__ == "__main__":
    main()
