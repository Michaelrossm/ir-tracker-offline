// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String nav() {
  return F("<nav><a href='/'>Dashboard</a><a href='/setup'>Einstellungen</a>"
           "<a href='/history'>Historie</a><a href='/interfaces'>Schnittstellen</a>"
           "<a href='/maintenance'>Wartung</a>"
           "<button id='langToggle' class='theme-toggle' "
           "type='button' aria-label='Sprache wechseln'>English</button>"
           "<button id='themeToggle' class='theme-toggle' "
           "type='button' title='Farbschema wechseln'>Farben</button></nav>");
}

String maintenanceTabs(const bool diagnostics, const bool factory = false) {
  String tabs = String(F("<div class='subnav' aria-label='Wartungsbereiche'>"
                  "<a href='/maintenance'")) +
         (diagnostics || factory ? "" : " class='active'") +
         F(">Backup &amp; System</a><a href='/maintenance/diagnostics'") +
         (diagnostics && !factory ? " class='active'" : "") +
         F(">Diagnose &amp; Zähler</a>");
#if IR_TRACKER_ENABLE_FACTORY_TEST
  tabs += String(F("<a href='/maintenance/factory-test'")) +
          (factory ? " class='active'" : "") + F(">Werksprüfung</a>");
#endif
  tabs += "</div>";
  return tabs;
}

String page(const String &title, const String &body,
            const String &script = "", const String &assetPath = "") {
  String html;
  html.reserve(body.length() + script.length() + 3600);
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += "<title>" + title + "</title>";
  html += "<script>window.IR_TRACKER_CONFIG={csrfToken:'" + csrfToken +
          "',firmwareVersion:'" + String(kFirmwareVersion) + "'};</script>";
  html += F("<script src='/assets/common.js?v=");
  html += kFirmwareVersion;
  html += F("'></script><link rel='stylesheet' href='/assets/common.css?v=");
  html += kFirmwareVersion;
  html += F("'></head><body><main>");
  html += nav();
  html += "<h1>" + title + "</h1><!--IR_BODY-->" + body;
  if (script.length()) html += "<script>" + script + "</script>";
  html += F("<script src='/assets/i18n.js?v=");
  html += kFirmwareVersion;
  html += F("'></script>");
  if (assetPath.length())
    html += "<script src='" + htmlEscape(assetPath) + "'></script>";
  html += "<footer>Firmware von " + String(kFirmwareAuthor) +
          " · © 2026 Michael Roßmann · " + String(kFirmwareLicense) +
          " · nur nichtkommerzielle Nutzung<br>Unabhängiges Community-Projekt; "
          "nicht mit Solakon verbunden und nicht von Solakon unterstützt.</footer>";
  html += F("</main></body></html>");
  return html;
}

bool sendPageStreamed(const String &title, const String &body,
                      const String &assetPath) {
  // DE: Der gemeinsame Rahmen bleibt klein. Seitentext und komprimiertes
  // JavaScript werden getrennt übertragen. | EN: The common shell remains
  // small. Page markup and compressed JavaScript are transferred separately.
  String shell = page(title, "", "", assetPath);
  const String marker = "<!--IR_BODY-->";
  const int insertion = shell.indexOf(marker);
  if (insertion < 0 || shell.indexOf("</html>") < 0) return false;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent(shell.substring(0, insertion));
  server.sendContent(body);
  shell.remove(0, insertion + marker.length());
  server.sendContent(shell);
  server.sendContent("");
  return true;
}
