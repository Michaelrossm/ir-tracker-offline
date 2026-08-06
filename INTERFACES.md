# Schnittstellen / Interfaces

## Deutsch

Der Tracker arbeitet als **nur lesender Stromzähler**. Null-Einspeisung, Ladegrenzen und Zeitpläne werden im Speicher oder Wechselrichter konfiguriert.

| Schnittstelle | Zweck |
|---|---|
| `/api/v1/status` | aktuelle JSON-Messwerte |
| `/api/v1/history` | lokale Historie nach Zeitraum |
| `/api/v1/values.csv` | aktuelle CSV-Werte |
| `/metrics`, `/openmetrics` | Prometheus/OpenMetrics |
| `/api/v1/influx` | Influx Line Protocol |
| `/status`, `/emeter/0` | Shelly-EM-kompatible Abfrage |
| `/rpc/EM.GetStatus?id=0` | Shelly-Pro-EM-kompatible Abfrage |
| MQTT Discovery | automatische Home-Assistant-Sensoren |

Weitere lokale Systeme: ioBroker, Node-RED, openHAB und jede Anwendung mit HTTP/JSON oder MQTT. L1/L2/L3 werden nur ausgegeben, wenn der Stromzähler diese OBIS-Werte sendet.

### Geschützte GPIO-Diagnose

`POST /api/v1/gpio-scan/start` startet eine flüchtige Suche über GPIO 0–10 und
gängige Baudraten. `GET /api/v1/gpio-scan` liefert Fortschritt und Ergebnis,
`POST /api/v1/gpio-scan/cancel` bricht ab. Ein Pin wird ausschließlich nach
einem frischen, CRC-gültigen SML-Telegramm bestätigt. Die Suche speichert keine
GPIO-Einstellung und stellt den normalen UART anschließend wieder her. Alle drei
Endpunkte benötigen Admin-Anmeldung; POST-Aufrufe zusätzlich das CSRF-Token aus
`GET /api/v1/admin-session`.

## English

The tracker operates as a **read-only electricity meter**. Zero export, charge limits and schedules are configured in the battery or inverter.

| Interface | Purpose |
|---|---|
| `/api/v1/status` | current JSON readings |
| `/api/v1/history` | local history by period |
| `/api/v1/values.csv` | current CSV values |
| `/metrics`, `/openmetrics` | Prometheus/OpenMetrics |
| `/api/v1/influx` | Influx Line Protocol |
| `/status`, `/emeter/0` | Shelly EM compatible request |
| `/rpc/EM.GetStatus?id=0` | Shelly Pro EM compatible request |
| MQTT Discovery | automatic Home Assistant sensors |

Other local systems include ioBroker, Node-RED, openHAB and any HTTP/JSON or MQTT application. L1/L2/L3 are exposed only when the electricity meter transmits those OBIS values.

### Protected GPIO diagnostics

`POST /api/v1/gpio-scan/start` starts a volatile scan across GPIO 0–10 and
common baud rates. `GET /api/v1/gpio-scan` returns progress and results, while
`POST /api/v1/gpio-scan/cancel` aborts it. A pin is accepted only after a fresh,
CRC-valid SML telegram. The scan stores no GPIO setting and restores the normal
UART afterwards. All three endpoints require admin authentication; POST calls
also require the CSRF token obtained from `GET /api/v1/admin-session`.
