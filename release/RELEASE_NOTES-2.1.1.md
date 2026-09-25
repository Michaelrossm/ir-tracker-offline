# IR Tracker Offline 2.1.1 — Release Notes / Versionshinweise

**Deutsch** | [English](#english)

## Deutsch

IR Tracker Offline 2.1.1 bündelt die seit 2.0.0 hinzugekommenen Produkt-, Diagnose-, Netzwerk- und Updateverbesserungen. Die bestehende lokale Messwertverarbeitung, Compact-History und die signierten IRUP-Gesamtupdates bleiben die Grundlage.

### Zählererkennung und Inbetriebnahme

- Neue automatische Zähler-Inbetriebnahme nutzt die vorhandenen SML- und IEC-62056-21/D0-Parser und probiert konservativ mehrere serielle Kandidaten.
- Geprüft werden 9600 Baud 8N1, 9600 Baud 7E1, 300 Baud 7E1, 2400 Baud 7E1 und 115200 Baud 8N1.
- Ein Kandidat gilt erst nach mehreren gültigen Telegrammen als erkannt; bei keinem eindeutigen Ergebnis wird keine Konfiguration blind übernommen.
- Während der automatischen Erkennung sind konkurrierende GPIO-, Factory- und IR-Sendeabläufe gesperrt, damit nicht mehrere Funktionen gleichzeitig die Zähler-UART umkonfigurieren.
- Diagnose und Produktstatus zeigen den Zustand der automatischen Zählererkennung an.

### Sicherer Produktbetrieb

- Bootloop-Schutz erkennt wiederholte frühe Panic-, Watchdog- oder Brownout-Starts.
- Nach drei frühen Fehlerstarts wird für diese Laufzeit ein Safe-Recovery-Modus aktiviert.
- Im Safe-Recovery-Modus bleiben Messung und Webzugang verfügbar, während automatische Firmwareupdates und History-Migration gesperrt werden.
- Nach 60 Sekunden stabilem Betrieb wird der frühe Fehlerzähler zurückgesetzt.
- Absichtliche Software-/OTA-Neustarts werden nicht als Crashloop gezählt.

### OTA und laufende Messung

- Der OTA-Messmodus bedient die Zähler-UART nun auch während des eigentlichen HTTP-Uploadcallbacks.
- IR-Impulse, Zählererfassung und die laufenden Mess-/History-Servicepfade werden während längerer Update-Schreibvorgänge regelmäßig weiter bedient.
- Der OTA-Modus wird bei Fehlern und Abbrüchen kontrolliert zurückgesetzt.
- Vor dem finalen OTA-Commit wird der offene History-Puffer gespeichert; bei einem Fehler wird das Update nicht blind bestätigt.
- Das signierte IRUP-Gesamtformat bleibt der normale Updateweg für Firmware und Webassets.

### WLAN, LAN und Anmeldung

- Der Setup-Hotspot unterstützt die üblichen Captive-Portal-Erkennungsendpunkte von Android, iOS/macOS und Windows.
- Der Captive-Portal-Einstieg zeigt den lokalen Setup-Zugang, ohne den kleinen Systembrowser unnötig in eine HTTP-Basic-Auth-Schleife zu schicken.
- LAN und WLAN verwenden dasselbe Admin-Konto.
- Das aktuell wirksame Admin-Passwort wird in den geschützten Einstellungen 1:1 angezeigt.
- Neu gesetzte Admin-Passwörter müssen mindestens 6 und höchstens 64 Zeichen lang sein.
- Für den WPA2-Setup-Hotspot gilt weiterhin die technisch notwendige Mindestlänge von 8 Zeichen. Bei einem 6- oder 7-stelligen Admin-Passwort verwendet der Setup-Hotspot deshalb weiterhin das ursprüngliche gerätespezifische Passwort im Format `IRTracker-XXXX`.
- WLAN-Passphrasen werden entsprechend WPA/WPA2 als leer/offen, 8–63 Zeichen oder exakt 64 hexadezimale Zeichen validiert.

### Ethernet / W5500

- Der W5500-SPI-Takt ist auf 30 MHz festgelegt.
- 30 MHz ist als Kompromiss zwischen Update-/Backup-Durchsatz und zusätzlicher Signalintegritätsreserve gegenüber höheren SPI-Takten gewählt.
- Bei nicht erkanntem W5500 wird der SPI-Bus wieder freigegeben, damit die vorhandenen GPIOs im WLAN-Betrieb verfügbar bleiben.

### Diagnose und Support

- Neuer Produktstatus unter `/api/v1/product-state` fasst Betriebsphase, Safe-Recovery und geführte Diagnose zusammen.
- Neue geführte Diagnose unter `/api/v1/guided-diagnosis` verbindet den vorhandenen Diagnosecode mit konkreten nächsten Prüfschritten.
- Neuer datensparsamer Supportbericht unter `/api/v1/support-report-safe` lässt Geräte-Hostname/Serienkennung und zuletzt bekannte Client-Adresse weg.
- Der normale Diagnosebericht zeigt zusätzlich Safe-Recovery und Status der automatischen Zählererkennung.
- Diagnosepfade unterscheiden weiterhin fehlendes Signal, vorhandene Rohdaten ohne gültiges Telegramm, veraltete Messwerte, Teilwerte sowie Integritäts-/Parserprobleme.

### Factory-Test

- Der Factory-Test enthält einen isolierten NVS-Schreib-/Lesetest mit anschließendem Aufräumen des Testwertes.
- Der automatische PASS-Zustand berücksichtigt diesen NVS-Test zusätzlich zu Chip, Flash, Heap, History, Netzwerk und vorhandenen Hardwareprüfungen.
- Factory-Test, GPIO-Suche und IR-Ausgabe respektieren die neue exklusive Zähler-Inbetriebnahme und greifen nicht gleichzeitig auf dieselbe UART zu.

### Code-Struktur und Laufzeit

- Produktlogik wurde in kleine, klar getrennte Module für Product Runtime, Product Safety, Product Experience, Runtime Health und Meter Auto Commissioning aufgeteilt.
- Die bestehende Messwert-, Parser- und History-Logik wird wiederverwendet; es wurde kein zweiter paralleler SML-/D0-Parser eingeführt.
- LED-/Heap-Laufzeitüberwachung wurde aus `main.cpp` in ein eigenes Runtime-Health-Modul verschoben, ohne die grundlegende Funktion zu ändern.
- Native Tests und Policy-Tests wurden für die neuen Produktzustände und Sicherheitsregeln erweitert.

### Kompatibilität und Update

- 2.1.1 verwendet weiterhin dieselbe 4-MiB-Grundpartitionierung mit zwei OTA-App-Slots, Asset-Bereich und History-Bereich.
- Ein normales IRUP-WLAN-Update ändert die Partitionstabelle nicht.
- Beim Update von einem bereits erfolgreich auf 2.0.0 migrierten Gerät reicht normalerweise ein vollständiges 2.1.1-IRUP-Update.
- Beim direkten Übergang von einer älteren, noch nicht auf Compact-History migrierten Version gelten weiterhin die Schutzregeln der 2.0.0-Migration: beide validierten App-Slots müssen einen kompatiblen Reader enthalten, bevor die Migration starten darf.
- Firmware und Webasset-Version müssen zusammenpassen. Ein Mischstand wie Firmware 2.1.1 mit Asset-Version 2.0.0 ist kein vollständiger 2.1.1-Releasezustand und soll durch ein vollständiges IRUP korrigiert werden.

### Bekannte Grenzen

- Automatische Zählererkennung kann eine fehlerhafte optische Hardware, falsche Polung, schlechte Signalpegel oder eine nicht aktivierte Zählerschnittstelle nicht softwareseitig reparieren.
- Captive-Portal-Verhalten hängt zusätzlich vom Betriebssystem und dessen Mini-Browser ab; `http://192.168.4.1/setup` bleibt der direkte lokale Zugang zum Setup-Hotspot.
- Die Weboberfläche verwendet HTTP und ist ausschließlich für ein vertrauenswürdiges Heim- oder IoT-Netz vorgesehen. Keine direkte Portfreigabe ins Internet.

---

<a id="english"></a>

## English

IR Tracker Offline 2.1.1 consolidates the product, diagnostics, networking and update improvements added after 2.0.0. Local meter processing, Compact History and signed complete IRUP updates remain the foundation.

### Meter discovery and commissioning

- New automatic meter commissioning reuses the existing SML and IEC 62056-21/D0 parsers and conservatively tries several serial candidates.
- Candidates include 9600 baud 8N1, 9600 baud 7E1, 300 baud 7E1, 2400 baud 7E1 and 115200 baud 8N1.
- A candidate is accepted only after multiple valid telegrams; ambiguous or unsuccessful scans do not blindly overwrite the configuration.
- GPIO scan, factory test and IR transmit operations are blocked while commissioning owns the meter UART.
- Diagnostics and product state expose the commissioning state.

### Safe product operation

- Boot-loop protection detects repeated early panic, watchdog or brownout resets.
- After three early crash-like boots, Safe Recovery is enabled for the current runtime.
- Measurement and web access remain available in Safe Recovery, while automatic firmware updates and history migration are blocked.
- After 60 seconds of stable runtime, the early-crash counter is cleared.
- Intentional software/OTA restarts are not counted as crash loops.

### OTA while measuring

- OTA measurement servicing now also runs inside the HTTP upload callback itself.
- Meter UART servicing, IR jobs and measurement/history service paths are called regularly during longer update writes.
- OTA measurement mode is cleared on failures and aborted uploads.
- The open history buffer is flushed before the final OTA commit; a failed flush prevents blind update confirmation.
- Signed complete IRUP bundles remain the normal update path for firmware and web assets.

### Wi-Fi, LAN and login

- The setup AP now supports common captive-portal detection endpoints used by Android, iOS/macOS and Windows.
- The captive landing page provides local setup guidance without forcing the OS mini-browser into an unnecessary HTTP Basic Auth loop.
- LAN and Wi-Fi use the same administrator account.
- The currently effective administrator password is displayed exactly in the protected Settings page.
- Newly configured administrator passwords must contain 6–64 characters.
- WPA2 still technically requires at least 8 characters for the setup AP. If the administrator password contains only 6 or 7 characters, the setup AP therefore keeps using the original per-device `IRTracker-XXXX` password.
- Wi-Fi passphrases are validated as open/empty, 8–63 characters, or exactly 64 hexadecimal characters.

### Ethernet / W5500

- W5500 SPI is fixed at 30 MHz.
- 30 MHz is used as a compromise between update/backup throughput and additional signal-integrity margin compared with higher SPI clocks.
- If no W5500 is detected, the SPI bus is released so the GPIOs remain available for Wi-Fi-only hardware.

### Diagnostics and support

- `/api/v1/product-state` provides the current product phase, Safe Recovery state and guided diagnosis.
- `/api/v1/guided-diagnosis` combines the existing diagnosis engine with concrete next-step guidance.
- `/api/v1/support-report-safe` provides a privacy-reduced support report that omits device hostname/serial details and the last known client address.
- The regular support report additionally shows Safe Recovery and automatic meter commissioning status.
- Diagnostics continue to distinguish missing signal, raw data without a valid telegram, stale values, partial values and parser/integrity problems.

### Factory test

- Factory testing now includes an isolated NVS write/read probe and removes the scratch value afterwards.
- Automatic PASS evaluation includes this NVS result in addition to chip, flash, heap, history, network and existing hardware checks.
- Factory test, GPIO scanning and IR output respect exclusive meter commissioning and no longer compete for the same UART.

### Code structure and runtime

- Product behavior is split into small modules for Product Runtime, Product Safety, Product Experience, Runtime Health and Meter Auto Commissioning.
- Existing meter, parser and history implementations are reused; no parallel second SML/D0 parser was introduced.
- LED/heap runtime monitoring was moved out of `main.cpp` into a dedicated Runtime Health module without changing its basic behavior.
- Native and policy tests were extended for the new product states and safety rules.

### Compatibility and update

- 2.1.1 keeps the same 4 MiB base partition layout with two OTA application slots, asset area and history area.
- A normal IRUP Wi-Fi update does not replace the partition table.
- Devices already successfully migrated to 2.0.0 Compact History normally need one complete 2.1.1 IRUP update.
- When upgrading directly from an older pre-migration version, the 2.0.0 migration safety rule still applies: both validated application slots must contain a compatible reader before migration may start.
- Firmware and web-asset versions must match. A mixed state such as firmware 2.1.1 with asset version 2.0.0 is not a complete 2.1.1 release state and should be corrected with a complete IRUP bundle.

### Known limitations

- Automatic meter commissioning cannot software-fix faulty optical hardware, wrong polarity, marginal signal levels or a meter interface that has not been enabled.
- Captive-portal behavior also depends on the operating system and its mini-browser; `http://192.168.4.1/setup` remains the direct local setup address.
- The web UI uses HTTP and is intended only for a trusted home or isolated IoT network. Do not expose it directly to the Internet.
