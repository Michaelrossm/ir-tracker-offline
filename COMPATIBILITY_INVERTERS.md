# Kompatibilität – PV-Wechselrichter / Mikrowechselrichter

Stand: 2026-08-30

Diese Liste bewertet **nicht nur, ob ein Wechselrichter irgendeine Schnittstelle besitzt**, sondern ob seine **Netzmessgeräte-Schnittstelle** mit den aktuell vom IR Tracker bereitgestellten Schnittstellen kompatibel ist.

## Bedeutung der Spalten

- **Kompatibel**: Der Wechselrichter bzw. sein EMS kann den IR Tracker mit der aktuellen Firmware grundsätzlich als Netzmessquelle verwenden.
- **Erwartete Zähler-Schnittstelle**: Schnittstelle bzw. Zählersystem, über das der Wechselrichter Bezug/Einspeisung für Regelungsfunktionen erhält.
- **Getestet**: Kombination wurde mit echter Hardware erfolgreich geprüft.

Status:
- 🟢 **Ja** – passende IR-Tracker-Schnittstelle bestätigt.
- 🟡 **Prüfung erforderlich** – technische Details/Varianten lassen keine sichere direkte Freigabe zu.
- ❌ **Nein, aktuell** – Wechselrichter erwartet eine andere Zähler-Schnittstelle als der IR Tracker derzeit bereitstellt.

> Der IR Tracker stellt aktuell HTTP/JSON, MQTT sowie Shelly-EM-/Shelly-Pro-EM-kompatible lokale Leseendpunkte bereit. Er emuliert derzeit **keinen SMA Energy Meter, KOSTAL Smart Energy Meter, Fronius Smart Meter, Huawei Smart Power Sensor, Sungrow/GoodWe/Fox/Deye-Zähler und keinen allgemeinen Modbus-RTU-/RS485-Zähler**. Eine vorhandene Modbus-Schnittstelle am Wechselrichter bedeutet deshalb nicht automatisch Kompatibilität.

## Wechselrichterliste

| # | Hersteller | Modell/Familie | Kompatibel | Erwartete Zähler-Schnittstelle | Getestet |
|---:|---|---|---|---|---|
| 1 | SMA | Sunny Boy 3.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 2 | SMA | Sunny Boy 3.6 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 3 | SMA | Sunny Boy 4.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 4 | SMA | Sunny Boy 5.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 5 | SMA | Sunny Boy 6.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 6 | SMA | Sunny Boy Smart Energy 3.6 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 7 | SMA | Sunny Boy Smart Energy 4.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 8 | SMA | Sunny Boy Smart Energy 5.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 9 | SMA | Sunny Boy Smart Energy 6.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 10 | SMA | Sunny Tripower 5.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 11 | SMA | Sunny Tripower 6.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 12 | SMA | Sunny Tripower 8.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 13 | SMA | Sunny Tripower 10.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager, Speedwire | ☐ |
| 14 | SMA | Sunny Tripower Smart Energy 5.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 15 | SMA | Sunny Tripower Smart Energy 6.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 16 | SMA | Sunny Tripower Smart Energy 8.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 17 | SMA | Sunny Tripower Smart Energy 10.0 | ❌ Nein, aktuell | SMA Energy Meter / Sunny Home Manager | ☐ |
| 18 | Fronius | Primo GEN24 3.0 Plus | ❌ Nein, aktuell | Fronius Smart Meter, Modbus RTU/TCP; neuere Systeme zusätzlich MQTT-Smart-Meter-Profil | ☐ |
| 19 | Fronius | Primo GEN24 4.0 Plus | ❌ Nein, aktuell | Fronius Smart Meter, Modbus RTU/TCP / Fronius-MQTT-Meterprofil | ☐ |
| 20 | Fronius | Primo GEN24 5.0 Plus | ❌ Nein, aktuell | Fronius Smart Meter, Modbus RTU/TCP / Fronius-MQTT-Meterprofil | ☐ |
| 21 | Fronius | Primo GEN24 6.0 Plus | ❌ Nein, aktuell | Fronius Smart Meter, Modbus RTU/TCP / Fronius-MQTT-Meterprofil | ☐ |
| 22 | Fronius | Symo GEN24 6.0 Plus | ❌ Nein, aktuell | Fronius Smart Meter, Modbus RTU/TCP / Fronius-MQTT-Meterprofil | ☐ |
| 23 | Fronius | Symo GEN24 8.0 Plus | ❌ Nein, aktuell | Fronius Smart Meter, Modbus RTU/TCP / Fronius-MQTT-Meterprofil | ☐ |
| 24 | Fronius | Symo GEN24 10.0 Plus | ❌ Nein, aktuell | Fronius Smart Meter, Modbus RTU/TCP / Fronius-MQTT-Meterprofil | ☐ |
| 25 | Fronius | Symo 8.2-3-M | ❌ Nein, aktuell | Fronius Smart Meter / Modbus RTU | ☐ |
| 26 | Fronius | Symo 10.0-3-M | ❌ Nein, aktuell | Fronius Smart Meter / Modbus RTU | ☐ |
| 27 | KOSTAL | PLENTICORE plus G2 5.5 | ❌ Nein, aktuell | KOSTAL Smart Energy Meter (KSEM), Modbus RTU/TCP | ☐ |
| 28 | KOSTAL | PLENTICORE plus G2 7.0 | ❌ Nein, aktuell | KSEM, Modbus RTU/TCP | ☐ |
| 29 | KOSTAL | PLENTICORE plus G2 8.5 | ❌ Nein, aktuell | KSEM, Modbus RTU/TCP | ☐ |
| 30 | KOSTAL | PLENTICORE plus G2 10 | ❌ Nein, aktuell | KSEM, Modbus RTU/TCP | ☐ |
| 31 | KOSTAL | PLENTICORE G3 S | ❌ Nein, aktuell | KSEM / freigegebenes KOSTAL-Messkonzept | ☐ |
| 32 | KOSTAL | PLENTICORE G3 M | ❌ Nein, aktuell | KSEM / freigegebenes KOSTAL-Messkonzept | ☐ |
| 33 | KOSTAL | PLENTICORE G3 L | ❌ Nein, aktuell | KSEM / freigegebenes KOSTAL-Messkonzept | ☐ |
| 34 | KOSTAL | PIKO IQ 4.2 | ❌ Nein, aktuell | KSEM, Modbus | ☐ |
| 35 | KOSTAL | PIKO IQ 5.5 | ❌ Nein, aktuell | KSEM, Modbus | ☐ |
| 36 | KOSTAL | PIKO IQ 7.0 | ❌ Nein, aktuell | KSEM, Modbus | ☐ |
| 37 | KOSTAL | PIKO IQ 8.5 | ❌ Nein, aktuell | KSEM, Modbus | ☐ |
| 38 | KOSTAL | PIKO IQ 10 | ❌ Nein, aktuell | KSEM, Modbus | ☐ |
| 39 | Huawei | SUN2000-2KTL-L1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / kompatibler Meter über RS485 | ☐ |
| 40 | Huawei | SUN2000-3KTL-L1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 41 | Huawei | SUN2000-4KTL-L1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 42 | Huawei | SUN2000-5KTL-L1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 43 | Huawei | SUN2000-6KTL-L1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 44 | Huawei | SUN2000-3KTL-M1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 45 | Huawei | SUN2000-4KTL-M1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 46 | Huawei | SUN2000-5KTL-M1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 47 | Huawei | SUN2000-6KTL-M1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 48 | Huawei | SUN2000-8KTL-M1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 49 | Huawei | SUN2000-10KTL-M1 | ❌ Nein, aktuell | Huawei Smart Power Sensor / RS485 | ☐ |
| 50 | Sungrow | SH5.0RT | ❌ Nein, aktuell | Sungrow Smart Energy Meter / RS485 | ☐ |
| 51 | Sungrow | SH6.0RT | ❌ Nein, aktuell | Sungrow Smart Energy Meter / RS485 | ☐ |
| 52 | Sungrow | SH8.0RT | ❌ Nein, aktuell | Sungrow Smart Energy Meter / RS485 | ☐ |
| 53 | Sungrow | SH10RT | ❌ Nein, aktuell | Sungrow Smart Energy Meter / RS485 | ☐ |
| 54 | Sungrow | SH15T | ❌ Nein, aktuell | Sungrow-kompatibler Smart Meter / RS485 | ☐ |
| 55 | Sungrow | SH20T | ❌ Nein, aktuell | Sungrow-kompatibler Smart Meter / RS485 | ☐ |
| 56 | Sungrow | SH25T | ❌ Nein, aktuell | Sungrow-kompatibler Smart Meter / RS485 | ☐ |
| 57 | GoodWe | GW5KN-ET Plus+ | ❌ Nein, aktuell | GoodWe Smart Meter GM3000/GM3000C, RS485 | ☐ |
| 58 | GoodWe | GW6.5KN-ET Plus+ | ❌ Nein, aktuell | GoodWe Smart Meter / RS485 | ☐ |
| 59 | GoodWe | GW8KN-ET Plus+ | ❌ Nein, aktuell | GoodWe Smart Meter / RS485 | ☐ |
| 60 | GoodWe | GW10KN-ET Plus+ | ❌ Nein, aktuell | GoodWe Smart Meter / RS485 | ☐ |
| 61 | GoodWe | GW5K-ET G2 | ❌ Nein, aktuell | GoodWe Smart Meter / RS485 | ☐ |
| 62 | GoodWe | GW8K-ET G2 | ❌ Nein, aktuell | GoodWe Smart Meter / RS485 | ☐ |
| 63 | GoodWe | GW10K-ET G2 | ❌ Nein, aktuell | GoodWe Smart Meter / RS485 | ☐ |
| 64 | SolarEdge | SE5K-RWB Home Hub | ❌ Nein, aktuell | SolarEdge Energy Meter / Modbus RS485 | ☐ |
| 65 | SolarEdge | SE7K-RWB Home Hub | ❌ Nein, aktuell | SolarEdge Energy Meter / Modbus RS485 | ☐ |
| 66 | SolarEdge | SE8K-RWB Home Hub | ❌ Nein, aktuell | SolarEdge Energy Meter / Modbus RS485 | ☐ |
| 67 | SolarEdge | SE10K-RWB Home Hub | ❌ Nein, aktuell | SolarEdge Energy Meter / Modbus RS485 | ☐ |
| 68 | SolarEdge | SE5K | ❌ Nein, aktuell | SolarEdge Energy Meter / Modbus RS485 für Export-/Verbrauchsmessung | ☐ |
| 69 | SolarEdge | SE7K | ❌ Nein, aktuell | SolarEdge Energy Meter / Modbus RS485 | ☐ |
| 70 | SolarEdge | SE10K | ❌ Nein, aktuell | SolarEdge Energy Meter / Modbus RS485 | ☐ |
| 71 | RCT Power | Power Inverter 4.0 | ❌ Nein, aktuell | RCT Power Sensor bzw. freigegebener Modbus-Zähler | ☐ |
| 72 | RCT Power | Power Inverter 6.0 | ❌ Nein, aktuell | RCT Power Sensor / Modbus-Zähler | ☐ |
| 73 | RCT Power | Power Storage DC 4.0 | ❌ Nein, aktuell | RCT Power Sensor / Modbus-Zähler | ☐ |
| 74 | RCT Power | Power Storage DC 6.0 | ❌ Nein, aktuell | RCT Power Sensor / Modbus-Zähler | ☐ |
| 75 | RCT Power | Power Storage DC 8.0 | ❌ Nein, aktuell | RCT Power Sensor / Modbus-Zähler | ☐ |
| 76 | RCT Power | Power Storage DC 10.0 | ❌ Nein, aktuell | RCT Power Sensor / Modbus-Zähler | ☐ |
| 77 | Fox ESS | H3-5.0-E | ❌ Nein, aktuell | Fox/Chint Smart Meter (z. B. DTSU666) über RS485 | ☐ |
| 78 | Fox ESS | H3-8.0-E | ❌ Nein, aktuell | Fox/Chint Smart Meter / RS485 | ☐ |
| 79 | Fox ESS | H3-10.0-E | ❌ Nein, aktuell | Fox/Chint Smart Meter / RS485 | ☐ |
| 80 | Fox ESS | H3-12.0-E | ❌ Nein, aktuell | Fox/Chint Smart Meter / RS485 | ☐ |
| 81 | Growatt | MIN 3000TL-XH | ❌ Nein, aktuell | Growatt Smart Meter / CT, RS485 | ☐ |
| 82 | Growatt | MIN 4600TL-XH | ❌ Nein, aktuell | Growatt Smart Meter / CT, RS485 | ☐ |
| 83 | Growatt | MIN 6000TL-XH | ❌ Nein, aktuell | Growatt Smart Meter / CT, RS485 | ☐ |
| 84 | Growatt | MOD 5KTL3-XH | ❌ Nein, aktuell | Growatt Smart Meter / CT, RS485 | ☐ |
| 85 | Growatt | MOD 8KTL3-XH | ❌ Nein, aktuell | Growatt Smart Meter / CT, RS485 | ☐ |
| 86 | Growatt | MOD 10KTL3-XH | ❌ Nein, aktuell | Growatt Smart Meter / CT, RS485 | ☐ |
| 87 | Deye | SUN-5K-SG04LP3-EU | ❌ Nein, aktuell | CT bzw. unterstützter RS485-Smart-Meter | ☐ |
| 88 | Deye | SUN-8K-SG04LP3-EU | ❌ Nein, aktuell | CT bzw. unterstützter RS485-Smart-Meter | ☐ |
| 89 | Deye | SUN-10K-SG04LP3-EU | ❌ Nein, aktuell | CT bzw. unterstützter RS485-Smart-Meter | ☐ |
| 90 | Hoymiles | HMS-800W-2T | ❌ Nein, aktuell | Export Management über DTU-Pro/DTU-Pro-S und kompatiblen RS485-Zähler/CT, nicht Shelly-Endpunkt am Wechselrichter | ☐ |
| 91 | Hoymiles | HMS-800W-T2 | ❌ Nein, aktuell | DTU-Pro/Export-Management-System | ☐ |
| 92 | Hoymiles | HMT-2250-6T | ❌ Nein, aktuell | DTU-Pro/Export-Management-System | ☐ |
| 93 | APsystems | EZ1-M | ❌ Nein, aktuell | System-/Exportregelung nicht über IR-Tracker-Shelly-Meter-Endpunkt | ☐ |
| 94 | APsystems | DS3-S | ❌ Nein, aktuell | ECU-/Export-Control-System, nicht direkte Shelly-Meter-Schnittstelle | ☐ |
| 95 | APsystems | DS3-L | ❌ Nein, aktuell | ECU-/Export-Control-System, nicht direkte Shelly-Meter-Schnittstelle | ☐ |
| 96 | TSUN | TSOL-MS800 | 🟡 Prüfung erforderlich | externe Export-/Energiemanagement-Lösung modell-/Gatewayabhängig; keine sichere direkte IR-Tracker-Freigabe | ☐ |
| 97 | Enphase | IQ8MC | ❌ Nein, aktuell | IQ Gateway Metering mit Stromwandlern/CTs | ☐ |
| 98 | Enphase | IQ8AC | ❌ Nein, aktuell | IQ Gateway Metering mit Stromwandlern/CTs | ☐ |
| 99 | SolaX | X3-Hybrid G4 | ❌ Nein, aktuell | SolaX/kompatibler Smart Meter (z. B. DTSU/DDSU-Familie) über RS485 | ☐ |
| 100 | Solis | S6-EH3P10K-H-EU | ❌ Nein, aktuell | freigegebener Solis/Eastron/Acrel Smart Meter über RS485 | ☐ |

## Ergebnis der Schnittstellenprüfung

Die bisherige pauschale Annahme, ein verbreiteter Wechselrichter sei deshalb ein Kandidat für die aktuelle IR-Tracker-Firmware, war zu weit gefasst. Die meisten klassischen String-/Hybridwechselrichter verwenden am Netzanschlusspunkt **herstellerspezifische Zähler, Modbus RTU/RS485, Speedwire oder CT-Systeme**. Die aktuelle Shelly-HTTP-Emulation des IR Trackers kann diese Geräte nicht direkt ersetzen.

Das bedeutet nicht, dass diese Wechselrichter dauerhaft ausgeschlossen sind. Viele könnten durch zusätzliche Softwareprofile erschlossen werden, z. B. Fronius-MQTT-Smart-Meter, Modbus-TCP-Zähleremulation oder – mit zusätzlicher Hardware – Modbus RTU/RS485. Solche Profile dürfen erst nach Implementierung und Prüfung auf `🟢 Ja` gesetzt werden.

## Feldtest

Ein Haken in **Getestet** wird nur gesetzt, wenn der konkrete Wechselrichter bzw. sein EMS den IR Tracker tatsächlich als Netzmessquelle akzeptiert und die benötigten Werte einschließlich Vorzeichen, Bezug/Einspeisung, Laständerung, Verbindungsverlust und Wiederanlauf korrekt verarbeitet.
