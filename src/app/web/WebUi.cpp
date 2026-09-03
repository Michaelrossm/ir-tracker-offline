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

String recoveryPage() {
  debugStorage.noteAssetServed("/assets/maintenance.js.gz", false);
  String html;
  html.reserve(6200);
  html = F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<title>IR Tracker Recovery</title><style>"
           "body{margin:0;background:#06140d;color:#eef8f1;font:16px system-ui}"
           "main{max-width:900px;margin:auto;padding:24px}section{border:1px solid #28734b;"
           "border-radius:12px;padding:18px;margin:14px 0;background:#0b2418}"
           "h1,h2{margin-top:0}label{display:block;margin:10px 0 5px}"
           "input,button,a{box-sizing:border-box;font:inherit}input{width:100%;padding:10px;"
           "background:#06140d;color:#fff;border:1px solid #3b8b60;border-radius:7px}"
           "button,.button{display:inline-block;margin:10px 6px 0 0;padding:10px 14px;"
           "border:0;border-radius:7px;background:#20bd67;color:#04150b;font-weight:700;"
           "text-decoration:none;cursor:pointer}code,pre{white-space:pre-wrap;word-break:break-word}"
           ".error{color:#ffb0a9}.muted{color:#a9c9b7}</style></head><body><main>"
           "<h1>IR Tracker – Recovery / Wiederherstellung</h1>"
           "<p>Die normalen Webassets sind nicht verfügbar. Der Tracker läuft weiter; "
           "installieren Sie ein passendes Asset-Image oder eine signierte Firmware.</p>"
           "<p class='muted'>Normal web assets are unavailable. The tracker remains active; "
           "install a matching asset image or signed firmware.</p><section><h2>Status</h2><pre>");
  html += "Firmware: ";
  html += kFirmwareVersion;
  html += "\nNetzwerk / network: ";
  html += primaryTransportName();
  html += "\nIP: ";
  html += htmlEscape(primaryNetworkIp());
  html += "\nAsset: ";
  html += htmlEscape(debugStorage.assetManifestError());
  html += F("</pre><a class='button' href='/api/v1/status'>Status-API</a>"
            "<a class='button' href='/api/v1/support-report'>Diagnosebericht</a></section>"
            "<section><h2>Firmware aktualisieren / Update firmware</h2>"
            "<form id='fw'><label>Signiertes IRFW-Paket</label>"
            "<input name='firmware' type='file' accept='.irfw' required>"
            "<button>Firmware installieren</button></form></section>"
            "<section><h2>Webassets wiederherstellen / Restore web assets</h2>"
            "<a class='button' href='/api/v1/asset-partition/backup'>64-kB-Sicherung laden</a>"
            "<form id='assets'><label>64-kB-Asset-Image</label>"
            "<input name='asset' type='file' accept='.bin' required>"
            "<label>SHA-256 des Images</label><input id='sha' pattern='[0-9A-Fa-f]{64}' "
            "maxlength='64' required><button>Asset-Image installieren</button></form></section>"
            "<section><h2>System</h2><button id='restart' type='button'>"
            "Tracker neu starten</button></section><pre id='result' class='muted'></pre><script>");
  html += "const csrf='" + csrfToken + "',out=document.getElementById('result');";
  html += F("async function send(url,body,headers={}){headers['X-CSRF-Token']=csrf;"
            "const r=await fetch(url,{method:'POST',body,headers});const t=await r.text();"
            "out.textContent='HTTP '+r.status+'\\n'+t;if(!r.ok)throw Error(t);return t}"
            "document.getElementById('fw').onsubmit=async e=>{e.preventDefault();"
            "try{await send('/system/update',new FormData(e.target));}catch(_){}};"
            "document.getElementById('assets').onsubmit=async e=>{e.preventDefault();"
            "const f=e.target.asset.files[0],sha=document.getElementById('sha').value;"
            "if(!f||f.size!==65536){out.textContent='Asset-Image muss exakt 65536 Byte haben.';return;}"
            "try{await send('/api/v1/asset-partition/update',new FormData(e.target),"
            "{'X-Asset-SHA256':sha,'X-Asset-Confirm':'BACKUP-VERIFIED-0x2B0000-0x10000'});"
            "location.reload()}catch(_){}};document.getElementById('restart').onclick=async()=>{"
            "try{await send('/system/restart','confirm=RESTART',"
            "{'Content-Type':'application/x-www-form-urlencoded'});}catch(_){}};"
            "</script></main></body></html>");
  return html;
}

String page(const String &title, const String &body,
            const String &script = "", const String &assetPath = "") {
  if (!debugStorage.assetManifestReady()) return recoveryPage();
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
  if (!debugStorage.assetManifestReady()) {
    server.send(200, "text/html; charset=utf-8", recoveryPage());
    return true;
  }
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
