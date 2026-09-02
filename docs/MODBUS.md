# Herstellerneutrales Modbus TCP / Vendor-neutral Modbus TCP

## Deutsch

IR Tracker kann optional als **nur lesende Modbus-TCP-Messwertquelle** auf
Port 502 arbeiten. Die Funktion ist standardmäßig ausgeschaltet und wird in
den Einstellungen aktiviert. Es werden ausschließlich die Funktionscodes 03
und 04 angenommen; Schreibbefehle sind nicht implementiert. Verbindungen
werden nur aus privaten lokalen IPv4-Netzen akzeptiert.

Das Registerschema ist eine eigene, offen dokumentierte IR-Tracker-Schnittstelle
und bildet kein Registermodell eines Fremdherstellers nach. Alle 32-Bit-Werte
liegen als High-Word, Low-Word vor. Nicht verfügbare vorzeichenbehaftete Werte
verwenden `0x80000000`, nicht verfügbare Zähler und Zeitwerte `0xFFFFFFFF`.
Register 1 ist für die Verfügbarkeit maßgeblich.

| Register | Inhalt | Format / Skalierung |
|---:|---|---|
| 0 | Schemaversion | `1` |
| 1 | Verfügbarkeit | Bits 0 Leistung, 1 Bezug, 2 Einspeisung, 3–5 Phasenleistung, 6–8 Spannung, 9–11 Strom, 15 frisch |
| 2–3 | Gesamtleistung | `int32`, W × 100 |
| 4–5 | Netzbezug | `uint32`, Wh |
| 6–7 | Einspeisung | `uint32`, Wh |
| 8–9 | Zeitstempel | Unix-Sekunden |
| 10–15 | L1/L2/L3 Leistung | je `int32`, W × 100 |
| 16–21 | L1/L2/L3 Spannung | je `uint32`, V × 100 |
| 22–27 | L1/L2/L3 Strom | je `uint32`, A × 1000 |

Der Dienst verwendet das bereits zentral normalisierte `MeterData`-Modell.
REST, MQTT, Shelly-, EcoTracker- und Modbus-Ausgabe interpretieren Messwerte
daher nicht unabhängig voneinander.

## English

IR Tracker can optionally operate as a **read-only Modbus TCP meter source** on
port 502. It is disabled by default and can be enabled in Settings. Only
function codes 03 and 04 are accepted; write commands are not implemented.
Connections are accepted only from private local IPv4 networks.

The register map is IR Tracker's own openly documented interface and does not
imitate a third-party register model. Every 32-bit value uses high word first,
then low word. Unavailable signed values use `0x80000000`; unavailable counters
and timestamps use `0xFFFFFFFF`. Register 1 is authoritative for availability.

| Register | Meaning | Format / scale |
|---:|---|---|
| 0 | Schema version | `1` |
| 1 | Availability | bits 0 power, 1 import, 2 export, 3–5 phase power, 6–8 voltage, 9–11 current, 15 fresh |
| 2–3 | Total power | `int32`, W × 100 |
| 4–5 | Grid import | `uint32`, Wh |
| 6–7 | Grid export | `uint32`, Wh |
| 8–9 | Timestamp | Unix seconds |
| 10–15 | L1/L2/L3 power | `int32` each, W × 100 |
| 16–21 | L1/L2/L3 voltage | `uint32` each, V × 100 |
| 22–27 | L1/L2/L3 current | `uint32` each, A × 1000 |

The service consumes the existing normalized `MeterData` model. REST, MQTT,
Shelly-compatible, EcoTracker-compatible, and Modbus output therefore do not
maintain separate interpretations of a reading.
