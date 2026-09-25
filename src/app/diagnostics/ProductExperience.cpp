// ProductExperience.cpp
// Thin UI/status adapter over existing diagnosis/update/history state.
// No duplicate diagnosis engine.

#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

const char *productPhaseName() {
  if (productSafeRecoveryActive()) return "safe_recovery";
  if (githubUpdate.installing) return "installing_update";
  if (githubUpdate.checking) return "checking_update";
  if (history.ready() && history.readOnly()) return "history_attention";
  if (!history.ready()) return "history_unavailable";
  if (meterCommissioning.active || !meter.lastTelegramMs) return "finding_meter";
  if (!valueFresh(meter.lastTelegramMs)) return "meter_waiting";
  return "ready";
}

String productPhaseMessage() {
  const char *phase = productPhaseName();

  if (!strcmp(phase, "safe_recovery"))
    return "Sicherer Wiederherstellungsmodus aktiv. Messung und Webzugang bleiben verfuegbar; automatische Updates und Historienmigration sind gesperrt.";
  if (!strcmp(phase, "checking_update"))
    return "Update wird geprueft.";
  if (!strcmp(phase, "installing_update"))
    return "Firmware und Weboberflaeche werden installiert. Nicht ausschalten.";
  if (!strcmp(phase, "history_attention"))
    return "Historie wird geprueft oder einmalig optimiert. Nicht ausschalten.";
  if (!strcmp(phase, "history_unavailable"))
    return "Lokale Historie ist momentan nicht verfuegbar.";
  if (!strcmp(phase, "finding_meter"))
    return "Stromzaehler wird automatisch gesucht.";
  if (!strcmp(phase, "meter_waiting"))
    return "Zaehler erkannt, aber aktuell kommen keine frischen Messwerte.";
  return "Bereit";
}

String guidedDiagnosisJson() {
  const MeterDiagnosis diagnosis = meterDiagnosis();
  const char *action = "Keine Aktion erforderlich.";

  switch (diagnosis.code) {
    case MeterDiagnosisCode::NoSignal:
      action = "Lesekopf neu ausrichten und optische Schnittstelle am Zaehler pruefen.";
      break;
    case MeterDiagnosisCode::NoTelegram:
      action = "IR-Daten sind vorhanden. Automatische Erkennung abwarten oder Zaehlerfreischaltung pruefen.";
      break;
    case MeterDiagnosisCode::Stale:
      action = "Lesekopfposition, Fremdlicht und Zaehleranzeige pruefen.";
      break;
    case MeterDiagnosisCode::PartialValues:
      action = "PIN-/INFO-Freischaltung bzw. erweiterten Datensatz am Zaehler pruefen.";
      break;
    case MeterDiagnosisCode::MissingEnergy:
      action = "Zaehlerfreischaltung pruefen; nicht jeder Zaehler liefert beide Energiezaehlerstaende.";
      break;
    case MeterDiagnosisCode::ParseUnstable:
    case MeterDiagnosisCode::IntegrityUnstable:
      action = "Lesekopfposition und Fremdlicht pruefen; bei Fortbestehen technischen Bericht erstellen.";
      break;
    default:
      break;
  }

  String json;
  json.reserve(680);
  json = "{\"state\":\"" + String(diagnosis.state) + "\"";
  json += ",\"code\":\"" + String(meterDiagnosisCodeName(diagnosis.code)) + "\"";
  json += ",\"summary\":\"" + jsonEscape(diagnosis.summary) + "\"";
  json += ",\"action\":\"" + jsonEscape(action) + "\"";
  json += ",\"commissioning\":" + meterCommissioningJson();
  json += "}";
  return json;
}

String productStateJson() {
  String json;
  json.reserve(850);
  json = "{\"phase\":\"" + String(productPhaseName()) + "\"";
  json += ",\"message\":\"" + jsonEscape(productPhaseMessage()) + "\"";
  json += ",\"safe\":" + productSafetyJson();
  json += ",\"diagnosis\":" + guidedDiagnosisJson();
  json += "}";
  return json;
}

void setupProductExperienceRoutes() {
  server.on("/api/v1/support-report-safe", HTTP_GET, [] {
    if (!requireAdmin()) return;
    sendIntegrationResponse("text/plain; charset=utf-8", supportReportText(true, true));
  });
  server.on("/api/v1/product-state", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", productStateJson());
  });

  server.on("/api/v1/guided-diagnosis", HTTP_GET, [] {
    if (!requireAdmin()) return;
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", guidedDiagnosisJson());
  });
}
