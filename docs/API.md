# IR Tracker HTTP API

This document is the developer-oriented HTTP API reference. The firmware is a read-only electricity-meter gateway for normal integrations; administrative and maintenance operations are explicitly separated below.

## Base URL and transport

`http://<tracker-ip>`

The current embedded server uses HTTP on the trusted local network. Do not expose it directly to the Internet. JSON responses use `application/json`; CSV and metrics endpoints use the content types shown below.

## Access classes

- **Integration API**: protected by the normal API-access policy (`requireApiAccess`).
- **Storage compatibility API**: available according to the storage-compatibility policy (`requireStorageCompatibilityAccess`). In compatibility mode the documented read-only compatibility endpoints can be consumed by private LAN clients.
- **Admin API**: requires an authenticated administrator session. State-changing browser/admin requests additionally use the firmware's CSRF protection where applicable.

Missing meter values are represented as `null` where the firmware has no fresh/valid value. L1/L2/L3 values exist only when supplied by the meter.

## Stable integration endpoints

### `GET /api/v1/meter`
Preferred vendor-neutral integration endpoint. Schema identifier: `irtracker.meter.v1`. Use this endpoint for new integrations rather than parsing the large diagnostic status object.

Typical fields represent current total power, import/export energy, per-phase measurements, freshness and meter/protocol information. Consumers must tolerate additional fields in future firmware versions.

### `GET /api/v1/status`
Detailed runtime/status object. Contains firmware/device identity, active transport, Wi-Fi/Ethernet state, meter freshness and protocol, power and energy values, phase values, parser/error counters, history/storage state and integration/runtime flags. This object is intended for status pages, support and diagnostics; it may grow between releases.

Important measurement fields include:

| Field | Type | Unit / meaning |
|---|---|---|
| `power_w` | number or null | instantaneous total power, W |
| `import_kwh` | number or null | imported energy, kWh |
| `export_kwh` | number or null | exported energy, kWh |
| `phases[]` | array | L1/L2/L3 values when available |
| `phases[].power_w` | number or null | phase power, W |
| `phases[].voltage_v` | number or null | phase voltage, V |
| `phases[].current_a` | number or null | phase current, A |
| `*_age_s` | number or null | age of the corresponding value, seconds |
| `meter_fresh` | boolean | current meter data considered fresh |
| `meter_protocol` | string | detected meter protocol |

### `GET /api/v1/obis`
Returns decoded OBIS information currently available from the meter.

### `GET /api/v1/history`
Returns local history. Query parameters and available resolutions are firmware-defined and should be discovered/validated against the running release before a client relies on a specific retention interval.

### `GET /api/v1/history.csv`
CSV export of local history.

### `GET /api/v1/values.csv`
Current values as CSV. Response header uses filename `irtracker-values.csv`.

### `GET /api/v1/dashboard-summary`
Compact data used by the local dashboard.

### `GET /metrics`
Prometheus exposition format (`text/plain; version=0.0.4`).

### `GET /openmetrics`
OpenMetrics 1.0 response (`application/openmetrics-text; version=1.0.0`) ending in `# EOF`.

### `GET /api/v1/influx`
Current values in Influx line protocol.

### `GET /api/v1/events`
Returns the local event log according to the API-access policy.

## Storage compatibility endpoints

These endpoints are **read-only meter facades**. They do not control a battery or inverter.

| Method | Path | Purpose |
|---|---|---|
| GET | `/v1/json` | EcoTracker-compatible local meter response |
| GET | `/shelly` | Shelly-compatible device information |
| GET | `/status` | Shelly Gen1-compatible status |
| GET | `/emeter/0` | Shelly Gen1 EM-compatible meter values |
| GET | `/rpc/EM.GetStatus?id=0` | Shelly Pro 3EM-compatible meter status |
| GET | `/rpc/EMData.GetStatus?id=0` | Shelly Pro 3EM-compatible energy data |
| GET | `/rpc/Shelly.GetDeviceInfo` | Shelly-compatible device-information method |
| GET | `/rpc/Shelly.ListMethods` | supported RPC methods |
| GET | `/rpc/Shelly.GetStatus` | Shelly-compatible RPC status |
| POST | `/rpc` | supported read-only Shelly JSON-RPC facade |

Compatibility means protocol/schema interoperability for the implemented subset. The tracker keeps its own neutral identity; it does not clone third-party serial numbers, OUIs or cloud identity.

## Modbus TCP

Optional read-only server on TCP port `502`; disabled by default. See [`MODBUS.md`](MODBUS.md) for the register map. It exposes meter data only and is not a writable inverter/battery-control interface.

## MQTT

MQTT provides meter telemetry and Home Assistant Discovery when configured. Topic names and broker credentials are runtime configuration. MQTT is independent from the HTTP compatibility facade.

## Administrative and maintenance endpoints

These are not part of the stable third-party meter API. Clients should not depend on them for normal energy integrations.

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/v1/admin-session` | authenticated session/CSRF information |
| GET | `/api/v1/memory-info` | memory diagnostics |
| GET | `/api/v1/meter-report` | meter support report |
| GET | `/api/v1/support-report` | support report |
| GET | `/api/v1/raw` | last raw telegram as hex |
| GET | `/api/v1/selftest` | self-test |
| GET | `/api/v1/gpio-scan` | GPIO scan status |
| POST | `/api/v1/gpio-scan/start` | start volatile GPIO/baud scan |
| POST | `/api/v1/gpio-scan/cancel` | cancel GPIO scan |
| POST | `/api/v1/gpio-output-test` | protected output test |
| POST | `/api/v1/gpio-scan-tx` | protected TX scan |
| GET | `/api/v1/update/status` | signed-update status |
| POST | `/api/v1/update/check` | check for signed update |
| POST | `/api/v1/update/install` | install verified update |
| POST | `/api/v1/update/bundle` | upload combined update bundle |
| GET | `/api/v1/backup/settings` | settings backup |
| POST | `/api/v1/backup/settings/restore` | settings restore |
| POST | `/api/v1/history/import/start` | begin history import |
| POST | `/api/v1/history/import/batch` | history import batch |
| POST | `/api/v1/history/clear` | clear history |
| POST | `/api/v1/events/clear` | clear event log |
| POST | `/api/v1/time` | set time |
| GET | `/api/v1/asset-partition` | asset-partition status |
| GET | `/api/v1/asset-partition/backup` | asset backup |
| POST | `/system/restart` | restart tracker |
| POST | `/system/shutdown` | safe shutdown |

IR unlock/PIN/pulse operations (`/ir/...`) are local administrative meter-maintenance functions and are not part of the read-only integration contract.

## Developer-only interfaces

The normal universal production build does not include writable battery/inverter control and does not include the WebSocket library. The optional `solakon_tracker_developer` PlatformIO profile adds the raw IR sniffer (port 81) and authenticated writable IR bridge (port 82) for laboratory/protocol work only.

## API compatibility rules

1. New integrations should prefer `/api/v1/meter`.
2. Existing fields in the `irtracker.meter.v1` contract should retain their meaning and unit within v1.
3. Clients must ignore unknown JSON fields so additive firmware updates remain compatible.
4. `null` means unavailable/not valid; it must not be interpreted as zero.
5. Diagnostic/admin objects may gain fields without an API-major change.
6. Third-party compatibility facades implement only the documented read-only subset.
7. No endpoint documented as a meter interface implies permission to control a battery, inverter, export limit or charging schedule.

## Errors

The embedded server uses standard HTTP status codes. Common cases include `401/403` for missing/invalid access, `404` for unknown routes, `409` when an operation conflicts with current state, `202` for accepted asynchronous operations, and `503` when required web assets need recovery. Error responses are generally JSON for API routes; administrative browser actions may redirect to the maintenance UI.

## Discovery

When enabled, mDNS advertises the neutral IR Tracker service. Storage compatibility mode may additionally advertise compatibility service types. Discovery always retains the IR Tracker's own identity.

---

This file documents the firmware routes implemented by the current source tree. `INTERFACES.md` is the short integration overview; this file is the developer reference. Administrative implementation details are intentionally separated from the stable meter-data contract.
