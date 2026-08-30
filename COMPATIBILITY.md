# IR Tracker Offline – Kompatibilitätsprüfung

Stand: 2026-08-30

Die Kompatibilitätsarbeit ist in drei getrennte Prüfmatrizen aufgeteilt:

1. [Stromzähler](COMPATIBILITY_METERS.md) – mindestens alle Modelle aus der aktuellen Solakon-PowerTracker-IR-Referenzliste; derzeit 76 Einträge.
2. [Speicher / Energiemanagement](COMPATIBILITY_STORAGE.md) – 100 priorisierte Systeme, mit Schwerpunkt Deutschland und anschließend EU.
3. [PV-Wechselrichter / Mikrowechselrichter](COMPATIBILITY_INVERTERS.md) – 100 priorisierte Systeme, mit Schwerpunkt Deutschland und anschließend EU.

## Grundsatz

Eine Aufnahme in eine Liste bedeutet **nicht**, dass das Gerät kompatibel ist. `✅ verifiziert` darf erst nach einem reproduzierbaren Test mit echter Hardware vergeben werden.

Bei Stromzählern muss die optische Kommunikation einschließlich Messwerten, Integrität, Aktualisierungsrate und Wiederanlauf geprüft werden. Bei Speichern und Wechselrichtern muss das Zielsystem den IR Tracker tatsächlich als Netzmessquelle verwenden und korrekt auf Bezug/Einspeisung reagieren.

## Priorität

Deutschland zuerst. Danach werden vor allem Österreich, Schweiz sowie die wichtigsten EU-PV-Märkte berücksichtigt. Die Listen sind Test-Backlogs und keine behaupteten Verkaufsranglisten.

## Ziel

Vor einer kommerziellen Kompatibilitätsaussage soll für jedes freigegebene Modell ein nachvollziehbarer Testnachweis mit Modell, Firmware, IR-Tracker-Version, verwendeter Schnittstelle und Ergebnis vorhanden sein.
