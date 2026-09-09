# IR Tracker Offline 1.3.8

## Wichtig: Update älterer Tracker

- Wird unter **Wartung** bereits **Vollständiges Update (.irup)** angezeigt,
  direkt `ir-tracker-update-1.3.8.irup` installieren.
- Wird nur **Custom-Firmware aktualisieren (.irfw)** angezeigt, zuerst
  `ir-tracker-custom-1.3.8.irfw` installieren. Nach dem Neustart erneut
  **Wartung** öffnen und anschließend `ir-tracker-update-1.3.8.irup`
  installieren, damit auch die passende Weboberfläche aktualisiert wird.
- Sehr alte Geräte ohne signiertes WLAN-Update oder mit unbekannter
  Partitionstabelle müssen zuerst mit dem aktuellen USB-Installer migriert
  werden. Der Installer erhält NVS, Einstellungen und History und ändert nur
  das vorhandene 64-kB-Gebiet von `coredump` zu `debugfs`.

## Änderungen

- Ein einziges signiertes IRUP200-Paket aktualisiert Firmware und Webassets.
- Signatur, Manifest und SHA-256 beider Inhalte werden geprüft; der OTA-Slot
  wird erst nach vollständiger Prüfung aktiviert.
- Die normale signierte `.irfw` bleibt für ältere Geräte und App-only-Updates
  enthalten.
- Zählerdiagnose erkennt fehlende Telegramme, gültige Telegramme ohne aktuelle
  Leistung sowie den vollständig fehlerfreien Zustand.
- Datenlücken erscheinen als dezente Hintergrundflächen; eine vorübergehende
  falsche Lücke am aktuellen Diagrammrand wurde verhindert.
- Tagesabfragen der Historie und statische Webassets wurden optimiert.
- MQTT, Home Assistant, Shelly-/EcoTracker-Kompatibilität, Modbus TCP, REST,
  CSV, Prometheus, Influx, History, LAN/WLAN und Eco-Modus bleiben enthalten.

## Dateien

- `ir-tracker-update-1.3.8.irup`: empfohlenes vollständiges WLAN-Update
- `ir-tracker-custom-1.3.8.irfw`: signiertes Firmware-/Übergangsupdate
- `ir-tracker-custom-1.3.8-usb.bin`: App-Image für den USB-Installer
- `ir-tracker-assets-1.3.8.bin`: separates Asset-Image für Diagnose/Recovery
- `partitions-1.3.8.bin`: Partitionstabelle ausschließlich für USB-Erstflash

Die LAN-/PoE-Funktion wurde weiterhin noch nicht auf echter LAN-Hardware
getestet.

---

## English

If **Complete update (.irup)** is already available, install
`ir-tracker-update-1.3.8.irup` directly. On older firmware offering only
`.irfw`, install `ir-tracker-custom-1.3.8.irfw` first, wait for the restart,
then install the `.irup` bundle to add the matching web assets. Very old
devices require the safe USB installer migration first.

Version 1.3.8 introduces the signed IRUP200 bundle for atomic activation of a
verified application and matching web assets, improves meter diagnostics and
history visualization, and keeps the normal signed IRFW available for older
devices.
