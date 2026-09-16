# Schnittstellen / Interfaces

## Deutsch

Der Tracker arbeitet als **nur lesender Stromzähler**. Null-Einspeisung, Ladegrenzen und Zeitpläne werden im Speicher, Wechselrichter oder Automatisierungssystem konfiguriert.

| Schnittstelle | Zweck |
|---|---|
| `/api/v1/meter` | bevorzugtes stabiles, herstellerneutrales Schema `irtracker.meter.v1` |
| `/api/v1/status` | ausführlicher Laufzeit-/Diagnosestatus einschließlich Messwerten |
| `/api/v1/obis` | aktuell dekodierte OBIS-Daten |
| `/api/v1/history`, `/api/v1/history.csv` | lokale Historie / CSV-Export |
| `/api/v1/values.csv` | aktuelle CSV-Werte |
| `/metrics`, `/openmetrics` | Prometheus/OpenMetrics |
| `/api/v1/influx` | Influx Line Protocol |
| `/v1/json` | EcoTracker-kompatible lokale Messwertabfrage (nur lesend) |
| `/shelly`, `/status`, `/emeter/0` | Shelly-EM-kompatible Erkennung und Abfrage |
| `/rpc`, `/rpc/EM.GetStatus?id=0`, `/rpc/EMData.GetStatus?id=0` | nur lesende Shelly-Pro-3EM-kompatible RPC-Abfrage |
| MQTT Discovery | automatische Home-Assistant-Sensoren |
| Modbus TCP, Port 502 | optionales, nur lesendes IR-Tracker-Registerschema; standardmäßig aus |

Für neue eigene Integrationen sollte `/api/v1/meter` verwendet werden. `/api/v1/status` ist bewusst umfangreicher und für Oberfläche, Support und Diagnose gedacht. L1/L2/L3 werden nur ausgegeben, wenn der Stromzähler die entsprechenden Werte liefert. Weitere Details einschließlich Admin-/Wartungsrouten stehen in [API.md](API.md); das Modbus-Registerschema steht in [MODBUS.md](MODBUS.md).

### Speicher-Kompatibilitätsmodus

Der Modus ist standardmäßig ausgeschaltet. Wird er unter **Schnittstellen** aktiviert, stellt der Tracker die implementierten EcoTracker-/Shelly-kompatiblen **Leseendpunkte** nach der lokalen Kompatibilitäts-Zugriffsregel bereit. OTA, Einstellungen, GPIO-Diagnose, History-Änderungen und sonstige Schreibzugriffe gehören nicht zu dieser Kompatibilitätsschnittstelle. Der Tracker behält dabei seine neutrale IR-Tracker-Identität; fremde Seriennummern, OUI-, Cloud- oder Produktidentitäten werden nicht nachgebildet.

### Geschützte Diagnose

GPIO-/Baudraten-Suche, Selbsttest, Rohtelegramme, Supportberichte und Updatefunktionen sind Wartungs-/Adminfunktionen und keine stabile Integrations-API. Die tatsächlich implementierten Methoden und Pfade sind in [API.md](API.md) getrennt dokumentiert.

## English

The tracker operates as a **read-only electricity meter**. Zero-export control, charge limits, and schedules are configured in the battery, inverter, or automation system.

| Interface | Purpose |
|---|---|
| `/api/v1/meter` | preferred stable vendor-neutral `irtracker.meter.v1` schema |
| `/api/v1/status` | detailed runtime/diagnostic status including readings |
| `/api/v1/obis` | currently decoded OBIS data |
| `/api/v1/history`, `/api/v1/history.csv` | local history / CSV export |
| `/api/v1/values.csv` | current CSV values |
| `/metrics`, `/openmetrics` | Prometheus/OpenMetrics |
| `/api/v1/influx` | Influx Line Protocol |
| `/v1/json` | EcoTracker-compatible local meter request (read-only) |
| `/shelly`, `/status`, `/emeter/0` | Shelly EM-compatible discovery and request |
| `/rpc`, `/rpc/EM.GetStatus?id=0`, `/rpc/EMData.GetStatus?id=0` | read-only Shelly Pro 3EM-compatible RPC request |
| MQTT Discovery | automatic Home Assistant sensors |
| Modbus TCP, port 502 | optional read-only IR Tracker register map; disabled by default |

New custom integrations should use `/api/v1/meter`. `/api/v1/status` is deliberately broader and intended for the UI, support, and diagnostics. L1/L2/L3 are exposed only when supplied by the meter. See [API.md](API.md) for the developer HTTP reference including administrative routes and [MODBUS.md](MODBUS.md) for the register map.

### Storage compatibility mode

This mode is disabled by default. When enabled under **Interfaces**, the tracker exposes the implemented EcoTracker-/Shelly-compatible **read-only endpoints** according to the local compatibility access policy. OTA, settings, GPIO diagnostics, history mutation, and other write operations are not part of this compatibility interface. The tracker retains its neutral IR Tracker identity and does not imitate third-party serial numbers, OUIs, cloud identities, or product identities.

### Protected diagnostics

GPIO/baud-rate scanning, self-test, raw telegrams, support reports, and update functions are maintenance/admin features rather than a stable integration API. Their actually implemented methods and paths are documented separately in [API.md](API.md).
