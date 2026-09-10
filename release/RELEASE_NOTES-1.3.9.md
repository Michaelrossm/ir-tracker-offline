# IR Tracker Offline 1.3.9

## Wichtig: Update aelterer Tracker

- Wird unter **Wartung** bereits **Vollstaendiges Update (.irup)** angezeigt,
  direkt `ir-tracker-update-1.3.9.irup` installieren.
- Aeltere Staende, die nur `.irfw` anbieten, sollten zuerst auf `1.3.8`
  wechseln und danach das vollstaendige `.irup`-Update auf `1.3.9` installieren.
- Sehr alte Geraete ohne signiertes WLAN-Update oder mit alter
  `coredump`-Partition sollten einmal mit dem aktuellen USB-Installer migriert
  werden. NVS, Einstellungen und History bleiben dabei erhalten.

## Aenderungen gegenueber 1.3.8

- Dashboard-Anzeigeintervall fuer ein und zwei Tage: 1, 5, 10 oder 15 Minuten,
  nur im Browser berechnet und ohne zusaetzliche Flash-Schreibvorgaenge.
- Datenluecken werden genauer abgegrenzt, solange minutengenaue History noch
  vorhanden ist. Kurze vorhandene Abschnitte bleiben sichtbar.
- Leistungs-, Netzbezugs- und Einspeisekurve bleiben in einem gemeinsamen
  Diagramm; Anzeigeintervalle werden an Datenluecken sauber getrennt.
- IRUP ist der einzige Updateweg in der Weboberflaeche und Recovery. App-only
  `.irfw`-Uploads und einzelne Webasset-Uploads wurden entfernt.
- IRUP-Updates benoetigen keinen automatischen Asset-Backup-Download mehr.
- Nach erfolgreichem Update wartet die Oberflaeche auf den Tracker-Neustart und
  laedt sich automatisch neu.
- mDNS wurde auf einen kleinen, eigenen IPv4-Responder fuer die benoetigten
  IR-Tracker-Dienste umgestellt.
- Technische Asset-Reports bleiben lokale Buildartefakte und werden nicht mehr
  im Release ausgeliefert.

## Dateien

- `ir-tracker-update-1.3.9.irup`: empfohlenes vollstaendiges WLAN-Update
- `ir-tracker-custom-1.3.9-usb.bin`: App-Image fuer den USB-Installer
- `ir-tracker-assets-1.3.9.bin`: separates Asset-Image fuer Diagnose/Recovery
- `partitions-1.3.9.bin`: Partitionstabelle ausschliesslich fuer USB-Erstflash

Die LAN-/PoE-Funktion wurde weiterhin noch nicht auf echter LAN-Hardware
getestet.

---

## English

Version 1.3.9 focuses on the complete IRUP update path and dashboard history
display. The web interface and recovery mode now use `.irup` bundles only,
manual asset-backup downloads are no longer required, and the page reloads
after the tracker comes back. The dashboard adds browser-only display
aggregation and improved data-gap rendering. Technical asset reports are kept
as local build artifacts and are not shipped with public releases.
