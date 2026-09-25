// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

String nav() {
  return F("<nav><a href='/'>Dashboard</a><a href='/setup'>Einstellungen</a>"
           "<a href='/interfaces'>Schnittstellen</a>"
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
  html.reserve(9000);
  html = F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<title>IR Tracker Recovery</title><style>"
           "body{margin:0;background:#06140d;color:#eef8f1;font:16px system-ui}"
           "main{max-width:900px;margin:auto;padding:24px}section{border:1px solid #28734b;"
           "border-radius:12px;padding:18px;margin:14px 0;background:#0b2418}"
           "h1,h2{margin-top:0}label{display:block;margin:10px 0 5px}"
           "input,button,a{box-sizing:border-box;font:inherit}input{width:100%;padding:10px;"
           "background:#06140d;color:#fff;border:1px solid #3b8b60;border-radius:7px}"
           "input[type=checkbox]{width:auto}"
           "button,.button{display:inline-block;margin:10px 6px 0 0;padding:10px 14px;"
           "border:0;border-radius:7px;background:#20bd67;color:#04150b;font-weight:700;"
           "text-decoration:none;cursor:pointer}button:disabled{opacity:.55;cursor:not-allowed}"
           "code,pre{white-space:pre-wrap;word-break:break-word}"
           ".error{color:#ffb0a9}.ok{color:#72e6a5}.muted{color:#a9c9b7}"
           ".update-status{margin-top:14px;padding:12px;border:1px solid #28734b;border-radius:8px;"
           "background:#071b11}.update-status strong{display:block;margin-bottom:7px}"
           "progress{width:100%;height:18px;accent-color:#20bd67}"
           ".small{font-size:13px;color:#a9c9b7}</style></head><body><main>"
           "<h1>IR Tracker – Recovery / Wiederherstellung</h1>"
           "<p>Die normalen Webassets sind nicht verfügbar. Der Tracker läuft weiter; "
           "installieren Sie ein vollständiges, signiertes IRUP-Update.</p>"
           "<p class='muted'>Normal web assets are unavailable. The tracker remains active; "
           "install a complete signed IRUP update.</p><section><h2>Status</h2><pre>");
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
            "<section><h2>Vollständiges Update / Complete update</h2>"
            "<p>Das IRUP-Paket aktualisiert Firmware und Weboberfläche gemeinsam. History und Einstellungen bleiben erhalten.</p>"
            "<form id='bundle'><label>Signiertes Gesamtupdate (.irup)</label>"
            "<input id='bundleFile' name='update' type='file' accept='.irup' required>"
            "<button id='bundleButton'>Vollständiges Update installieren</button></form>"
            "<div id='updateBox' class='update-status'>"
            "<strong id='updateStage'>Bereit – noch kein Update gestartet.</strong>"
            "<progress id='updateProgress' max='100' value='0'></progress>"
            "<div id='updateDetail' class='small'>Wählen Sie eine IRUP-Datei aus.</div>"
            "<pre id='updateLog' class='muted'></pre></div></section>"
            "<section><h2>System</h2><button id='restart' type='button'>"
            "Tracker neu starten</button></section><pre id='result' class='muted'></pre><script>");
  html += "const csrf='" + csrfToken + "';";
  html += F(
      "const out=document.getElementById('result'),stage=document.getElementById('updateStage'),"
      "detail=document.getElementById('updateDetail'),bar=document.getElementById('updateProgress'),"
      "log=document.getElementById('updateLog'),fileInput=document.getElementById('bundleFile');"
      "let started=0,timer=null;"
      "const stamp=()=>new Date().toLocaleTimeString();"
      "const addLog=t=>{log.textContent+=(log.textContent?'\\n':'')+'['+stamp()+'] '+t};"
      "const setStage=(t,p,d,cls='')=>{stage.textContent=t;stage.className=cls;"
      "if(Number.isFinite(p))bar.value=Math.max(0,Math.min(100,p));if(d!==undefined)detail.textContent=d};"
      "const elapsed=()=>started?Math.round((Date.now()-started)/1000):0;"
      "const errorText=code=>({"
      "unauthorized:'Anmeldung/CSRF-Prüfung fehlgeschlagen.',"
      "signed_irup_bundle_required:'Die ausgewählte Datei ist kein gültiges .irup-Paket.',"
      "update_bundle_header_invalid:'IRUP-Header ist ungültig oder beschädigt.',"
      "update_bundle_signature_invalid:'Signatur ungültig – Public/Private-Key passen vermutlich nicht zusammen.',"
      "update_bundle_json_invalid:'Das signierte IRUP-Manifest ist ungültig.',"
      "update_bundle_content_invalid:'IRUP-Inhalt, Größen oder Prüfsummenangaben sind ungültig.',"
      "fixed_layout_mismatch:'Flash-Partitionslayout passt nicht zur Firmware.',"
      "asset_label_invalid:'Asset-Partition hat einen unerwarteten Typ oder Namen.',"
      "asset_geometry_mismatch:'Asset-Partition hat falsche Größe oder Adresse.',"
      "asset_rollback_pending:'Ein vorheriger Asset/OTA-Vorgang ist noch nicht sauber abgeschlossen.',"
      "history_flush_before_update_failed:'Historie konnte vor dem Update nicht sicher gespeichert werden.',"
      "firmware_overlaps_rollback_reserve:'Firmware ist für den sicheren OTA-Bereich zu groß.',"
      "update_partition_unavailable:'Kein geeigneter inaktiver OTA-Slot verfügbar.',"
      "sha256_initialization_failed:'SHA-256-Prüfung konnte nicht gestartet werden.',"
      "not_an_esp32_application:'Firmwareteil ist kein gültiges ESP32-App-Image.',"
      "firmware_write_failed:'Firmware konnte nicht in den OTA-Slot geschrieben werden.',"
      "firmware_sha256_mismatch:'Firmware-Prüfsumme stimmt nicht mit dem Manifest überein.',"
      "asset_backup_or_journal_failed:'Asset-Backup/Rollback-Journal konnte nicht vorbereitet werden.',"
      "asset_erase_failed:'Webasset-Partition konnte nicht gelöscht/vorbereitet werden.',"
      "asset_write_failed:'Webassets konnten nicht vollständig geschrieben werden.',"
      "asset_size_mismatch:'Webasset-Image hat nicht exakt 64 KiB.',"
      "asset_sha256_mismatch:'Prüfsumme des Webasset-Images stimmt nicht.',"
      "manifest_invalid:'Webasset-Manifest ist nach dem Schreiben ungültig.',"
      "file_missing:'Mindestens ein benötigtes Webasset fehlt.',"
      "sha256_mismatch:'Mindestens ein Webasset hat eine falsche Prüfsumme.',"
      "asset_flash_verification_failed:'Geschriebene Webassets konnten nicht verifiziert werden.',"
      "firmware_image_validation_failed:'ESP32 hat das fertige Firmware-Image abgewiesen.',"
      "update_bundle_upload_aborted:'Upload wurde abgebrochen.',"
      "update_bundle_invalid:'IRUP-Verarbeitung ist fehlgeschlagen.'"
      "}[code]||code||'Unbekannter Updatefehler');"
      "fileInput.onchange=()=>{const f=fileInput.files[0];if(!f)return;"
      "setStage('Datei ausgewählt',0,f.name+' · '+(f.size/1024/1024).toFixed(2)+' MiB');"
      "log.textContent='';addLog('IRUP ausgewählt: '+f.name+' ('+f.size+' Byte)')};"
      "function uploadBundle(form,button){return new Promise((resolve,reject)=>{"
      "const xhr=new XMLHttpRequest();xhr.open('POST','/api/v1/update/bundle');"
      "xhr.setRequestHeader('X-CSRF-Token',csrf);"
      "xhr.timeout=900000;"
      "xhr.upload.onloadstart=()=>{setStage('1/4 Upload gestartet',1,'IRUP wird zum Tracker übertragen …');addLog('Upload gestartet')};"
      "xhr.upload.onprogress=e=>{if(!e.lengthComputable)return;const p=Math.max(1,Math.min(95,Math.round(e.loaded/e.total*95)));"
      "setStage('1/4 IRUP wird übertragen',p,e.loaded+' / '+e.total+' Byte · '+Math.round(e.loaded/e.total*100)+' % · '+elapsed()+' s')};"
      "xhr.upload.onload=()=>{setStage('2/4 Upload vollständig – Prüfung läuft',96,"
      "'Der Tracker prüft jetzt Header, Signatur, Manifest, Firmware-Hash und Webasset-Hash. Bitte nicht ausschalten.');"
      "addLog('Upload vollständig; serverseitige Prüfung/Flash läuft')};"
      "xhr.onload=()=>{let code='',message=xhr.responseText||'';try{const j=JSON.parse(message);code=j.error||''}catch(_){}"
      "if(xhr.status>=200&&xhr.status<300){setStage('3/4 Update geschrieben und verifiziert',99,"
      "'Firmware und Webassets wurden angenommen. Neustart wird erwartet.','ok');"
      "addLog('Server meldet HTTP '+xhr.status+': Update angenommen');resolve(message);return}"
      "const readable=errorText(code);setStage('Update fehlgeschlagen',bar.value,"
      "'HTTP '+xhr.status+' · '+readable,'error');addLog('FEHLER HTTP '+xhr.status+': '+(code||message));"
      "reject(Error((code?code+': ':'')+readable))};"
      "xhr.onerror=()=>{setStage('Verbindungsfehler',bar.value,'Verbindung zum Tracker während des Updates abgebrochen.','error');"
      "addLog('Netzwerkfehler während des Uploads');reject(Error('network_error'))};"
      "xhr.ontimeout=()=>{setStage('Zeitüberschreitung',bar.value,'Der Tracker hat innerhalb von 15 Minuten nicht geantwortet.','error');"
      "addLog('Timeout nach 15 Minuten');reject(Error('timeout'))};"
      "xhr.onabort=()=>{setStage('Upload abgebrochen',bar.value,'Der Upload wurde abgebrochen.','error');"
      "addLog('Upload abgebrochen');reject(Error('aborted'))};"
      "xhr.send(new FormData(form))})}"
      "document.getElementById('bundle').onsubmit=async e=>{e.preventDefault();"
      "const button=document.getElementById('bundleButton'),file=fileInput.files[0];"
      "if(button.disabled||!file)return;button.disabled=true;started=Date.now();"
      "if(timer)clearInterval(timer);timer=setInterval(()=>{if(started&&bar.value>=96&&bar.value<100)"
      "detail.textContent='Serverseitige Prüfung/Flash läuft · '+elapsed()+' s seit Start';},1000);"
      "try{await uploadBundle(e.target,button);setStage('4/4 Neustart wird erwartet',100,"
      "'Update erfolgreich angenommen. Der Tracker startet neu; Verbindung wird automatisch geprüft.','ok');"
      "addLog('Warte auf Neustart');let attempts=0;"
      "const poll=async()=>{try{const c=new AbortController(),to=setTimeout(()=>c.abort(),2500);"
      "const r=await fetch('/api/v1/status',{cache:'no-store',signal:c.signal});clearTimeout(to);"
      "if(r.ok){const s=await r.json();if(s.asset_manifest_valid){addLog('Tracker wieder erreichbar; Asset-Manifest gültig');"
      "setStage('Update abgeschlossen',100,'Tracker läuft wieder mit gültigen Webassets. Seite wird neu geladen.','ok');"
      "if(timer)clearInterval(timer);setTimeout(()=>location.replace('/?updated='+Date.now()),800);return}}}catch(_){}"
      "attempts++;detail.textContent='Warte auf Neustart/Assets … Versuch '+attempts+' / 60 · '+elapsed()+' s';"
      "if(attempts<60)setTimeout(poll,2000);else{setStage('Update übertragen – Tracker noch nicht bereit',100,"
      "'Bitte Diagnose prüfen oder Seite später neu laden.','error');addLog('Neustartprüfung nach 60 Versuchen beendet');"
      "button.disabled=false;if(timer)clearInterval(timer)}};setTimeout(poll,4000)"
      "}catch(err){out.textContent='Update fehlgeschlagen: '+err.message;button.disabled=false;if(timer)clearInterval(timer)}};"
      "document.getElementById('restart').onclick=async()=>{try{const r=await fetch('/system/restart',{method:'POST',"
      "headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':csrf},body:'confirm=RESTART'});"
      "out.textContent='HTTP '+r.status+'\\n'+await r.text()}catch(err){out.textContent=err.message}};"
      "</script></main></body></html>");
  return html;
}

String page(const String &title, const String &body,
            const String &script = "", const String &assetPath = "") {
  if (assetRollbackBlocked || !debugStorage.assetManifestReady()) return recoveryPage();
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
  if (assetRollbackBlocked || !debugStorage.assetManifestReady()) {
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
