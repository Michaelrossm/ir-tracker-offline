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
