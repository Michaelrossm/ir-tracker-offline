#!/usr/bin/env python3
"""DE: Zerstörungsfreie HTTP-Sicherheits-Kurztests am Tracker.
EN: Non-destructive HTTP security smoke tests for a flashed tracker.
"""

from __future__ import annotations

import argparse
import base64
import json
import urllib.error
import urllib.request


def request(url: str, user: str | None = None, password: str | None = None,
            method: str = "GET", data: bytes | None = None,
            csrf: str | None = None) -> tuple[int, bytes]:
    req = urllib.request.Request(url, method=method, data=data)
    if user is not None:
        token = base64.b64encode(f"{user}:{password}".encode()).decode()
        req.add_header("Authorization", f"Basic {token}")
    if csrf:
        req.add_header("X-CSRF-Token", csrf)
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            return response.status, response.read()
    except urllib.error.HTTPError as error:
        return error.code, error.read()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", default="http://192.168.178.66")
    parser.add_argument("--user", default="admin")
    parser.add_argument("--password", required=True)
    args = parser.parse_args()
    base = args.base.rstrip("/")

    status, _ = request(base + "/setup")
    assert status == 401, f"/setup without auth returned {status}"

    status, _ = request(base + "/setup", args.user, args.password)
    assert status == 200, f"/setup with auth returned {status}"

    status, payload = request(
        base + "/api/v1/admin-session", args.user, args.password
    )
    assert status == 200, f"admin session returned {status}"
    csrf = json.loads(payload).get("csrf_token", "")
    assert len(csrf) == 64 and all(c in "0123456789abcdef" for c in csrf), \
        "random CSRF token not found"

    status, _ = request(
        base + "/api/v1/time", args.user, args.password, "POST", b"epoch=1"
    )
    assert status == 403, f"POST without CSRF returned {status}"

    status, payload = request(base + "/api/v1/memory-info", args.user, args.password)
    assert status == 200
    info = json.loads(payload)
    assert "restart_reason" in info and "heap_warning" in info
    print("HTTP security smoke tests: PASS")


if __name__ == "__main__":
    main()
