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
