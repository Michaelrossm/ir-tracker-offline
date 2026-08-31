#pragma once

// DE: Erzeugter öffentlicher OTA-Prüfschlüssel; der private Schlüssel wird nie versioniert.
// EN: Generated public OTA verification key; the private key is never committed.
constexpr char kFirmwareSigningPublicKey[] = R"PEM(
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE0HJXX0memQGC1a3zQ/CJAEdiHH0+
QItptkSb+Kz0peBun1NWDpgFtmCX6aw5XqyWzdl/IMfJkowiXacory7d6Q==
-----END PUBLIC KEY-----
)PEM";
