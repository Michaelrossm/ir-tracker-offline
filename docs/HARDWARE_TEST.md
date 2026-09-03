# Hardwaretest / Hardware test

## Deutsch

Vor Freigabe auf echter Hardware prüfen:

1. Start, WLAN-Verbindung und automatischer Setup-Hotspot.
2. SML-Empfang, CRC-Prüfung und plausible Gesamtleistung.
3. L1/L2/L3, Spannung und Strom nur soweit vom Zähler geliefert.
4. Stunde/Tag/Woche/Monat/Jahr sowie Touch- und Mausdiagramme.
5. Backup, Wiederherstellung und Löschen der Historie.
6. Signiertes OTA; manipuliertes Paket muss abgewiesen werden.
7. Admin-Sperre, CSRF, API-Modi und deaktivierte optionale Schnittstellen.
8. Eco-Modus: 80 MHz im Leerlauf; 160 MHz bei WLAN-Aufbau, LAN-Rückfall,
   Firmwareupdate, GPIO-Suche und Werksprüfung; Rückkehr nach zwei Minuten.
9. WLAN-Wiederverbindung, LAN-Rückfall und Fehler-LED.
10. IEC-62056-21/D0 mit einem unterstützten älteren Zähler prüfen.
11. Mindestens 72 Stunden Dauerlauf ohne Heapwarnung, Neustart oder Messlücke durch die Firmware.

### Ausgeführter Beta-2-Assettest (03.09.2026)

- ESP32-C3 über signiertes WLAN-OTA aktualisiert; Partitionstabelle nicht geschrieben.
- Tatsächliches Altgerätelayout bestätigt: `coredump`, `0x2B0000`, `0x10000`;
  History weiterhin bei `0x2C0000`.
- Alle neun Assets aus dem vollständigen Container geladen;
  `maintenance.js Quelle: debugfs` bestätigt.
- Ungültigen 64-kB-Container installiert: Recovery-Seite erschien, kein
  Bootloop oder Crash, Messwerte, History und lokale APIs blieben erreichbar.
- Gültigen Container wiederhergestellt und durch vollständiges 64-kB-Readback
  mit SHA-256 `1490DE1B9096E6AE00DE592AC2D2511E521E941E31C83D690FF60616F845366D`
  bytegenau bestätigt.
- 64 gemischte WLAN-Abrufe: 0 Fehler. SML über mehrere Minuten: 0 Parserfehler,
  CRC-Zähler unverändert, Heap stabil. Dies ersetzt nicht den 72-Stunden-Test.

## English

Before release, verify on real hardware:

1. Boot, Wi-Fi connection and automatic setup hotspot.
2. SML reception, CRC validation and plausible total power.
3. L1/L2/L3, voltage and current only where supplied by the meter.
4. Hour/day/week/month/year plus touch and mouse charts.
5. History backup, restore and deletion.
6. Signed OTA; a tampered package must be rejected.
7. Admin lockout, CSRF, API modes and disabled optional interfaces.
8. Eco mode: 80 MHz while idle; 160 MHz during Wi-Fi association, Ethernet
   fallback, firmware updates, GPIO scans and factory testing; return after two minutes.
9. Wi-Fi reconnection, Ethernet fallback and the fault LED.
10. Verify IEC 62056-21/D0 using a supported legacy meter.
11. At least 72 hours continuous operation without heap warning, reboot or firmware-caused measurement gap.

### Completed beta-2 asset test (2026-09-03)

- Updated the ESP32-C3 through signed Wi-Fi OTA without writing the partition table.
- Confirmed the actual legacy layout: `coredump`, `0x2B0000`, `0x10000`, with
  history still at `0x2C0000`.
- Loaded all nine assets from the complete container and confirmed
  `maintenance.js Quelle: debugfs`.
- Installed an invalid 64-kB container: recovery appeared without a bootloop or
  crash, while measurements, history and local APIs remained reachable.
- Restored the valid container and confirmed an exact 64-kB readback with
  SHA-256 `1490DE1B9096E6AE00DE592AC2D2511E521E941E31C83D690FF60616F845366D`.
- 64 mixed Wi-Fi requests completed with zero failures. SML remained stable for
  several minutes with zero parser errors, unchanged CRC count and stable heap.
  This does not replace the 72-hour soak test.
