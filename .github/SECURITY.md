# Sicherheit / Security

## Deutsch

- Nur in einem vertrauenswürdigen Heimnetz oder getrennten IoT-VLAN betreiben.
- Keine Portfreigaben für HTTP 80 oder WebSockets 81/82 einrichten.
- Fernzugriff ausschließlich über Router-VPN oder einen korrekt abgesicherten Reverse Proxy.
- Admin-Passwort mit mindestens 12 Zeichen empfohlen; erlaubt sind 4–64 Zeichen.
- Nach fünf Fehlversuchen wird die Quell-IP im RAM bis zu einer Stunde gesperrt.
- Schreibende IR-Bridge und Sniffer deaktiviert lassen, wenn sie nicht benötigt werden.
- API auf **Admin-Anmeldung erforderlich** stellen, sofern Integrationen keinen offenen lokalen Zugriff benötigen.
- Ereignisprotokoll enthält keine Passwörter; Flash-Speicherung ist standardmäßig aus.
- OTA akzeptiert nur Pakete mit gültiger ECDSA-P-256-Signatur von Michael Roßmann.

HTTP verschlüsselt Zugangsdaten und Messwerte nicht. HTTPS ist wegen lokaler Shelly-Kompatibilität, Zertifikatsverwaltung und ESP32-Ressourcen nicht erzwungen. VPN ist die bevorzugte Lösung.

Sicherheitsprobleme bitte nicht mit Gerätedumps, WLAN-Passwörtern, Zähler-PINs oder privaten Schlüsseln öffentlich melden.

## English

- Operate only in a trusted home network or isolated IoT VLAN.
- Never forward HTTP port 80 or WebSocket ports 81/82 from the internet.
- Use only a router VPN or properly secured reverse proxy for remote access.
- An admin password of at least 12 characters is recommended; 4–64 are accepted.
- After five failed attempts, the source IP is blocked in RAM for up to one hour.
- Keep the writable IR bridge and sniffer disabled unless required.
- Set the API to **Admin login required** unless integrations need open local access.
- Event logs never contain passwords; flash persistence is disabled by default.
- OTA accepts only packages with a valid ECDSA P-256 signature from Michael Roßmann.

HTTP does not encrypt credentials or readings. HTTPS is not mandatory because of local Shelly compatibility, certificate management and ESP32 resource limits. A VPN is preferred.

Do not publicly report security issues together with device dumps, Wi-Fi passwords, meter PINs or private keys.
