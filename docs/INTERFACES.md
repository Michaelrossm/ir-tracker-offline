# Schnittstellen / Interfaces

## Deutsch

Der Tracker arbeitet als **nur lesender Stromzähler**. Null-Einspeisung, Ladegrenzen und Zeitpläne werden im Speicher oder Wechselrichter konfiguriert.

| Schnittstelle | Zweck |
|---|---|
| `/api/v1/status` | aktuelle JSON-Messwerte |
| `/api/v1/meter` | stabiles herstellerneutrales Schema `irtracker.meter.v1` |
| `/api/v1/history` | lokale Historie nach Zeitraum |
| `/api/v1/values.csv` | aktuelle CSV-Werte |
| `/metrics`, `/openmetrics` | Prometheus/OpenMetrics |
| `/api/v1/influx` | Influx Line Protocol |
| `/v1/json` | EcoTracker-kompatible lokale Messwertabfrage (nur lesend) |
| `/shelly`, `/status`, `/emeter/0` | Shelly-EM-kompatible Erkennung und Abfrage |
| `/rpc`, `/rpc/EM.GetStatus?id=0`, `/rpc/EMData.GetStatus?id=0` | Nur lesende Shelly-Pro-3EM-kompatible RPC-Abfrage |
| MQTT Discovery | automatische Home-Assistant-Sensoren |
| Modbus TCP, Port 502 | optionales, nur lesendes IR-Tracker-Registerschema; standardmäßig aus |

Weitere lokale Systeme: ioBroker, Node-RED, openHAB und jede Anwendung mit HTTP/JSON oder MQTT. L1/L2/L3 werden nur ausgegeben, wenn der Stromzähler diese OBIS-Werte sendet.

### Speicher-Kompatibilitätsmodus und Erkennung

Der Modus ist standardmäßig ausgeschaltet. Wird er in den Einstellungen
aktiviert, sind ausschließlich `/v1/json`, `/shelly`, `/status`, `/emeter/0`
und die oben genannten nur lesenden RPC-Methoden aus privaten lokalen Netzen
ohne Anmeldung erreichbar. OTA, Einstellungen, GPIO, Diagnose, Historienänderung
und alle sonstigen Schreibzugriffe bleiben geschützt. mDNS kündigt
`_irtracker._tcp`, `_shelly._tcp` und `_everhome._tcp` mit der neutralen IR-Tracker-Identität und
der aktiven WLAN- oder LAN-IP an. Fremde Modell-, Serien-, OUI- oder
Produktkennungen werden nicht nachgebildet. Proprietäre UDP- oder Cloud-Bindung
ist ohne offen dokumentiertes, neutral implementierbares Protokoll absichtlich
nicht enthalten.

### Geschützte GPIO-Diagnose

`POST /api/v1/gpio-scan/start` startet eine flüchtige Suche über die im aktiven
Universalprofil verfügbaren GPIOs und gängige Baudraten. Wird ein W5500 über
sein Versionsregister erkannt, werden dessen GPIOs 0, 1, 4, 7 und 10 niemals
umgeschaltet. Ohne W5500 bleiben diese Pins für bestehende WLAN-Hardware
verfügbar. `GET /api/v1/gpio-scan` liefert Fortschritt und Ergebnis,
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
| `/api/v1/meter` | stable vendor-neutral `irtracker.meter.v1` schema |
| `/api/v1/history` | local history by period |
| `/api/v1/values.csv` | current CSV values |
| `/metrics`, `/openmetrics` | Prometheus/OpenMetrics |
| `/api/v1/influx` | Influx Line Protocol |
| `/v1/json` | EcoTracker-compatible local meter request (read-only) |
| `/shelly`, `/status`, `/emeter/0` | Shelly EM compatible discovery and request |
| `/rpc`, `/rpc/EM.GetStatus?id=0`, `/rpc/EMData.GetStatus?id=0` | Read-only Shelly Pro 3EM compatible RPC request |
| MQTT Discovery | automatic Home Assistant sensors |
| Modbus TCP, port 502 | optional read-only IR Tracker register map; disabled by default |

Other local systems include ioBroker, Node-RED, openHAB and any HTTP/JSON or MQTT application. L1/L2/L3 are exposed only when the electricity meter transmits those OBIS values.

### Storage compatibility mode and discovery

This mode is disabled by default. When enabled in Settings, only `/v1/json`,
`/shelly`, `/status`, `/emeter/0`, and the read-only RPC methods listed above
are available without authentication from private local networks. OTA,
settings, GPIO, diagnostics, history mutation, and every other write operation
remain protected. mDNS advertises `_irtracker._tcp`, `_shelly._tcp`, and `_everhome._tcp` using the
neutral IR Tracker identity and the active Wi-Fi or Ethernet IP. No third-party
model, serial, OUI, or product identity is imitated. Proprietary UDP or cloud
binding is deliberately omitted unless it can be implemented from an openly
documented protocol without identity spoofing.

### Protected GPIO diagnostics

`POST /api/v1/gpio-scan/start` starts a volatile scan across the GPIOs available
in the universal hardware profile and common baud rates. Once a W5500 is
confirmed through its version register, GPIOs 0, 1, 4, 7 and 10 are never
toggled. Without a W5500 those pins remain available to existing Wi-Fi-only
hardware. `GET /api/v1/gpio-scan` returns progress and results, while
`POST /api/v1/gpio-scan/cancel` aborts it. A pin is accepted only after a fresh,
CRC-valid SML telegram. The scan stores no GPIO setting and restores the normal
UART afterwards. All three endpoints require admin authentication; POST calls
also require the CSRF token obtained from `GET /api/v1/admin-session`.

## Entwickler-Build / Developer build

DE: Der universelle Produktions-Build enthält keine schreibende
Speicher-/Wechselrichtersteuerung und keine WebSocket-Bibliothek. Der rohe
IR-Sniffer (Port 81) und die authentifizierte, schreibende IR-Bridge (Port 82)
werden ausschließlich mit dem nicht standardmäßig gebauten PlatformIO-Profil
`solakon_tracker_developer` eingebunden. Dieses Profil ist für Labor- und
Protokolltests gedacht, nicht für normale Tracker-Installationen.

EN: The universal production build contains neither writable
battery/inverter control nor the WebSocket library. The raw IR sniffer (port
81) and authenticated writable IR bridge (port 82) are included exclusively by
the non-default PlatformIO profile `solakon_tracker_developer`. This profile is
intended for lab and protocol testing, not normal tracker installations.
