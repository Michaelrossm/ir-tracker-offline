// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String jsonEscape(const String &value) {
  String out;
  out.reserve(value.length() + 8);
  for (char c : value) {
    if (c == '"' || c == '\\') out += '\\';
    if (c == '\n') {
      out += "\\n";
    } else if (c != '\r') {
      out += c;
    }
  }
  return out;
}

String htmlEscape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}

bool tryServeDebugAsset(const char *relativePath, const char *mimeType) {
  String compressedPath = relativePath;
  compressedPath += ".gz";
  if (debugStorage.assetManifestReady()) {
    uint32_t rawOffset = 0;
    size_t rawSize = 0;
    if (debugStorage.rawVerifiedAsset(compressedPath.c_str(), rawOffset,
                                      rawSize)) {
      server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
      server.sendHeader("X-Content-Type-Options", "nosniff");
      server.sendHeader("Content-Encoding", "gzip");
      server.setContentLength(rawSize);
      server.send(200, mimeType, "");
      uint8_t buffer[256];
      for (size_t position = 0; position < rawSize;
           position += sizeof(buffer)) {
        const size_t count = std::min(sizeof(buffer), rawSize - position);
        if (!debugStorage.readRaw(rawOffset + position, buffer, count) ||
            server.client().write(buffer, count) != count)
          return true;
        delay(0);
      }
      debugStorage.noteAssetServed(compressedPath.c_str(), true);
      return true;
    }
    File file = debugStorage.openVerifiedAsset(compressedPath.c_str());
    if (file) {
      server.sendHeader("Cache-Control", "public, max-age=86400, immutable");
      server.sendHeader("X-Content-Type-Options", "nosniff");
      server.sendHeader("Content-Encoding", "gzip");
      server.streamFile(file, mimeType);
      file.close();
      debugStorage.noteAssetServed(compressedPath.c_str(), true);
      return true;
    }
  }
  debugStorage.noteAssetServed(compressedPath.c_str(), false);
  return false;
}

bool safeSingleLine(const String &value, size_t maximumLength) {
  if (value.length() > maximumLength) return false;
  // String::indexOf('\0') also sees the normal C-string terminator and would
  // reject every value. Inspect only the actual payload bytes instead.
  for (size_t index = 0; index < value.length(); ++index) {
    const char character = value[index];
    if (character == '\r' || character == '\n' || character == '\0')
      return false;
  }
  return true;
}

bool validWifiPassword(const String &value) {
  if (!safeSingleLine(value, 64)) return false;
  if (value.length() <= 63) return true;
  // DE: WPA2 erlaubt alternativ zu einer Passphrase exakt 64 Hex-Zeichen.
  // EN: WPA2 permits exactly 64 hexadecimal characters instead of a passphrase.
  for (const char character : value)
    if (!isxdigit(static_cast<unsigned char>(character))) return false;
  return true;
}

bool validHostname(const String &value) {
  if (!value.length() || value.length() > 32 || value[0] == '-' ||
      value[value.length() - 1] == '-')
    return false;
  for (char c : value)
    if (!isalnum(static_cast<unsigned char>(c)) && c != '-') return false;
  return true;
}

String localAdminPassword() {
  if (config.adminPassword.length() >= 4) return config.adminPassword;
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X",
           static_cast<unsigned>(ESP.getEfuseMac() & 0xffff));
  return "IRTracker-" + String(suffix);
}

String hexBytes(const uint8_t *data, size_t length) {
  static const char hex[] = "0123456789abcdef";
  String output;
  output.reserve(length * 2);
  for (size_t i = 0; i < length; ++i) {
    output += hex[data[i] >> 4];
    output += hex[data[i] & 0x0f];
  }
  return output;
}

String sessionSignature(const String &expiry) {
  String key = localAdminPassword() + ":";
  key += String(static_cast<uint32_t>(ESP.getEfuseMac() >> 32), HEX);
  key += String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
  key += ":irtracker-session-v1";
  uint8_t digest[32] = {};
  const mbedtls_md_info_t *info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info ||
      mbedtls_md_hmac(
          info, reinterpret_cast<const unsigned char *>(key.c_str()),
          key.length(),
          reinterpret_cast<const unsigned char *>(expiry.c_str()),
          expiry.length(), digest) != 0) {
    key = "";
    return "";
  }
  key = "";
  const String signature = hexBytes(digest, sizeof(digest));
  memset(digest, 0, sizeof(digest));
  return signature;
}

bool constantTimeEqual(const String &left, const String &right) {
  if (left.length() != right.length()) return false;
  uint8_t difference = 0;
  for (size_t i = 0; i < left.length(); ++i)
    difference |= static_cast<uint8_t>(left[i] ^ right[i]);
  return difference == 0;
}

bool validBrowserSession() {
  const time_t now = time(nullptr);
  if (now < 1700000000) {
    browserSessionState = "time_invalid";
    return false;
  }
  const String cookieHeader = server.header("Cookie");
  const String marker = "ir_session=";
  int start = cookieHeader.indexOf(marker);
  if (start < 0) {
    browserSessionState = "missing";
    return false;
  }
  start += marker.length();
  int end = cookieHeader.indexOf(';', start);
  if (end < 0) end = cookieHeader.length();
  const String token = cookieHeader.substring(start, end);
  const int separator = token.indexOf('.');
  if (separator <= 0 || separator >= static_cast<int>(token.length() - 1)) {
    browserSessionState = "malformed";
    return false;
  }
  const String expiry = token.substring(0, separator);
  if (expiry.length() < 10 || expiry.length() > 11) {
    browserSessionState = "malformed";
    return false;
  }
  for (char c : expiry)
    if (!isDigit(c)) {
      browserSessionState = "malformed";
      return false;
    }
  const uint64_t expiresAt = strtoull(expiry.c_str(), nullptr, 10);
  if (expiresAt <= static_cast<uint64_t>(now) ||
      expiresAt > static_cast<uint64_t>(now) + kBrowserSessionSeconds + 300) {
    browserSessionState = "expired";
    return false;
  }
  const bool valid =
      constantTimeEqual(token.substring(separator + 1),
                        sessionSignature(expiry));
  browserSessionState = valid ? "valid" : "signature_invalid";
  return valid;
}

void issueBrowserSession() {
  const time_t now = time(nullptr);
  if (now < 1700000000) return;
  const String expiry =
      String(static_cast<uint64_t>(now) + kBrowserSessionSeconds);
  const String signature = sessionSignature(expiry);
  if (!signature.length()) return;
  server.sendHeader(
      "Set-Cookie",
      "ir_session=" + expiry + "." + signature +
          "; Max-Age=" + String(kBrowserSessionSeconds) +
          "; Path=/; HttpOnly; SameSite=Strict",
      false);
}

bool timePending(uint32_t deadline) {
  return deadline && static_cast<int32_t>(millis() - deadline) < 0;
}

LoginGuard &loginGuardFor(const IPAddress &ip) {
  LoginGuard *oldest = &loginGuards[0];
  for (auto &guard : loginGuards) {
    if (guard.ip == ip) return guard;
    if (guard.lastSeenMs < oldest->lastSeenMs) oldest = &guard;
  }
  *oldest = LoginGuard{};
  oldest->ip = ip;
  return *oldest;
}

bool validCsrfRequest() {
  if (server.method() != HTTP_POST) return true;
  const String supplied = server.header("X-CSRF-Token").length()
                              ? server.header("X-CSRF-Token")
                              : server.arg("csrf_token");
  if (csrfToken.length() != 64 || supplied != csrfToken) {
    server.send(403, "application/json", "{\"error\":\"csrf_token_invalid\"}");
    return false;
  }
  return true;
}

bool requireAdmin() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("X-Frame-Options", "DENY");
  server.sendHeader("Referrer-Policy", "no-referrer");
  server.sendHeader("Cross-Origin-Resource-Policy", "same-origin");
  server.sendHeader(
      "Content-Security-Policy",
      "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src "
      "'self' 'unsafe-inline'; connect-src 'self' ws:; img-src 'self' data:; "
      "object-src 'none'; base-uri 'none'; form-action 'self'; "
      "frame-ancestors 'none'");
  server.sendHeader("Permissions-Policy",
                    "camera=(), microphone=(), geolocation=()");
  if (validBrowserSession()) return validCsrfRequest();
  const IPAddress remote = server.client().remoteIP();
  LoginGuard &guard = loginGuardFor(remote);
  guard.lastSeenMs = millis();
  if (timePending(guard.lockUntilMs)) {
    server.sendHeader("Retry-After",
                      String((guard.lockUntilMs - millis()) / 1000 + 1));
    server.send(429, "application/json",
                "{\"error\":\"too_many_login_attempts\"}");
    return false;
  }
  if (guard.lockUntilMs) {
    guard.failures = 0;
    guard.lockUntilMs = 0;
  }
  const String password = localAdminPassword();
  if (server.authenticate("admin", password.c_str())) {
    guard.failures = 0;
    guard.firstFailureMs = 0;
    issueBrowserSession();
    return validCsrfRequest();
  }
  // DE: Eine Anfrage ohne Zugangsdaten öffnet nur den Browserdialog und zählt
  // nicht als Fehlversuch. | EN: A credential-free request only opens the
  // browser login dialog and does not count as a failed attempt.
  if (server.header("Authorization").length()) {
    if (!guard.firstFailureMs ||
        millis() - guard.firstFailureMs > kLoginWindowMs) {
      guard.failures = 0;
      guard.firstFailureMs = millis();
    }
    if (++guard.failures >= 5) {
      guard.failures = 0;
      guard.lockLevel = std::min<uint8_t>(guard.lockLevel + 1, 4);
      const uint32_t duration =
          std::min<uint32_t>(5UL * 60UL * 1000UL
                                 << (guard.lockLevel - 1),
                             kLoginMaxLockMs);
      guard.lockUntilMs = millis() + duration;
      server.sendHeader("Retry-After", String(duration / 1000));
      server.send(429, "application/json",
                  "{\"error\":\"too_many_login_attempts\"}");
      return false;
    }
  }
  server.requestAuthentication(BASIC_AUTH, "IR-Tracker Einstellungen",
                               "Anmeldung erforderlich");
  return false;
}

bool requireApiAccess() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("Referrer-Policy", "no-referrer");
  server.sendHeader("X-Frame-Options", "DENY");
  if (config.apiAccess == 0) return true;
  // Disabled means disabled for every caller, including administrators. The
  // maintenance UI remains available through requireAdmin().
  if (config.apiAccess == 2) {
    server.send(404, "application/json", "{\"error\":\"api_disabled\"}");
    return false;
  }
  const String password = localAdminPassword();
  if (server.authenticate("admin", password.c_str())) return true;
  return requireAdmin();
}

bool isPrivateLocalAddress(const IPAddress &address) {
  return address[0] == 10 || address[0] == 127 ||
         (address[0] == 172 && address[1] >= 16 && address[1] <= 31) ||
         (address[0] == 192 && address[1] == 168) ||
         (address[0] == 169 && address[1] == 254);
}

bool localCompatibilityClient() {
  return isPrivateLocalAddress(server.client().remoteIP());
}

bool requireStorageCompatibilityAccess() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("Referrer-Policy", "no-referrer");
  server.sendHeader("X-Frame-Options", "DENY");
  if (config.apiAccess == 2) {
    server.send(404, "application/json", "{\"error\":\"api_disabled\"}");
    return false;
  }
  if (config.storageCompatibilityMode && localCompatibilityClient()) return true;
  return requireAdmin();
}
