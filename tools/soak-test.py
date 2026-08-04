#!/usr/bin/env python3
"""DE: Langzeit-Hardwaremonitor; Passwort aus Umgebungsvariable lesen.
EN: Long-running hardware monitor; read password from an environment variable.
"""

from __future__ import annotations

import argparse
import base64
import csv
import getpass
import json
import os
import time
import urllib.request
from datetime import datetime, timezone
from pathlib import Path


def fetch(base: str, path: str, auth: str | None) -> dict:
    request = urllib.request.Request(base.rstrip("/") + path)
    if auth:
        request.add_header("Authorization", "Basic " + auth)
    with urllib.request.urlopen(request, timeout=10) as response:
        return json.load(response)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", default="http://192.168.178.66")
    parser.add_argument("--hours", type=float, default=72)
    parser.add_argument("--interval", type=float, default=10)
    parser.add_argument("--output", type=Path, default=Path("soak-test.csv"))
    parser.add_argument("--user", default="admin")
    parser.add_argument(
        "--public",
        action="store_true",
        help="nur öffentliche Status-API, kein Passwort / public status API only, no password",
    )
    args = parser.parse_args()
    password = None if args.public else os.environ.get("IRTRACKER_ADMIN_PASSWORD")
    if not args.public and not password:
        password = getpass.getpass("Admin password: ")
    auth = (
        base64.b64encode(f"{args.user}:{password}".encode()).decode()
        if password
        else None
    )
    deadline = time.monotonic() + args.hours * 3600
    previous_uptime = None
    failures = restarts = stale = 0
    minimum_heap = None
    first_crc_errors = first_parse_errors = None
    last_crc_errors = last_parse_errors = 0
    firmware = None
    new_file = not args.output.exists()
    with args.output.open("a", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        if new_file:
            writer.writerow(
                ["utc", "ok", "firmware", "uptime_s", "free_heap",
                 "min_free_heap", "meter_fresh", "telegrams", "crc_errors",
                 "parse_errors", "last_crc_valid", "wifi_rssi",
                 "response_ms", "error"]
            )
        while time.monotonic() < deadline:
            row_time = datetime.now(timezone.utc).isoformat()
            request_started = time.monotonic()
            try:
                status = fetch(args.base, "/api/v1/status", auth)
                memory = (
                    fetch(args.base, "/api/v1/memory-info", auth)
                    if auth
                    else {
                        "free_heap": status["free_heap"],
                        "minimum_free_heap": status["free_heap"],
                    }
                )
                uptime = int(status["uptime_s"])
                firmware = status.get("firmware")
                free_heap = int(memory["free_heap"])
                last_crc_errors = int(status.get("crc_errors", 0))
                last_parse_errors = int(status.get("parse_errors", 0))
                if first_crc_errors is None:
                    first_crc_errors = last_crc_errors
                    first_parse_errors = last_parse_errors
                minimum_heap = (
                    free_heap if minimum_heap is None else min(minimum_heap, free_heap)
                )
                if previous_uptime is not None and uptime < previous_uptime:
                    restarts += 1
                previous_uptime = uptime
                if not status.get("meter_fresh"):
                    stale += 1
                writer.writerow(
                    [row_time, 1, firmware, uptime, free_heap,
                     memory["minimum_free_heap"], status.get("meter_fresh"),
                     status.get("telegrams"), last_crc_errors,
                     last_parse_errors, status.get("last_crc_valid"),
                     status.get("wifi_rssi"),
                     round((time.monotonic() - request_started) * 1000, 1), ""]
                )
            except Exception as error:
                failures += 1
                writer.writerow(
                    [row_time, 0, "", "", "", "", "", "", "", "", "", "",
                     round((time.monotonic() - request_started) * 1000, 1),
                     str(error)]
                )
            stream.flush()
            time.sleep(args.interval)
    print(
        f"Completed {args.hours:g}h: failures={failures}, restarts={restarts}, "
        f"stale_samples={stale}, minimum_observed_heap={minimum_heap}, "
        f"crc_delta={last_crc_errors - (first_crc_errors or 0)}, "
        f"parse_delta={last_parse_errors - (first_parse_errors or 0)}, "
        f"firmware={firmware}"
    )
    if (failures or restarts or minimum_heap is None or minimum_heap < 36000
            or last_parse_errors > (first_parse_errors or 0)):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
