// This module is included by main.cpp inside its private namespace.
// It is excluded from standalone PlatformIO compilation to preserve the exact
// firmware behavior and memory layout while keeping responsibilities separate.
#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void handleRoot() {
  if (!requireAdmin()) return;
  String body = F("<div class='grid'>"
    "<div class='card'><div class='muted'>Aktuelle Leistung</div><div class='value' id='powerValue'>–</div></div>"
    "<div class='card'><div class='muted'>Netzbezug</div><div class='value' id='importValue'>–</div></div>"
    "<div class='card'><div class='muted'>Einspeisung</div><div class='value' id='exportValue'>–</div></div>"
    "<div class='card'><div class='muted'>Zählerstatus</div><div class='value' id='meterState'>–</div></div></div>"
    "<div id='phaseSection'><h2>Phasenwerte live</h2><div class='grid'>"
    "<div class='card'><strong>L1</strong><div id='phase0'>–</div></div>"
    "<div class='card'><strong>L2</strong><div id='phase1'>–</div></div>"
    "<div class='card'><strong>L3</strong><div id='phase2'>–</div></div></div>"
    "<p class='muted'>Es werden ausschließlich tatsächlich vom Zähler übertragene Werte angezeigt.</p></div>"
    "<div class='section-head'><div><h2>Energieübersicht</h2>"
    "<div class='muted'>Tag, Vergleich und laufendes Kalenderjahr</div></div></div>"
    "<div class='grid'>"
    "<div class='card'><div class='muted'>Netzbezug heute</div><div class='value' id='todayImportSummary'>–</div></div>"
    "<div class='card'><div class='muted'>Einspeisung heute</div><div class='value' id='todayExportSummary'>–</div></div>"
    "<div class='card'><div class='muted'>Bezug zu gestern, gleiche Uhrzeit</div><div class='value' id='dayComparison'>–</div></div>"
    "<div class='card'><div class='muted'>Netzbezug im Jahr</div><div class='value' id='yearImport'>–</div></div>"
    "<div class='card'><div class='muted'>Einspeisung im Jahr</div><div class='value' id='yearExport'>–</div></div>"
    "<div class='card'><div class='muted'>Ø Netzbezug pro Tag im Jahr</div><div class='value' id='yearDailyImport'>–</div></div>"
    "<div class='card'><div class='muted'>Ø Einspeisung pro Tag im Jahr</div><div class='value' id='yearDailyExport'>–</div></div>"
    "</div><p class='muted' id='yearCoverage'>Jahreswerte werden aus der verfügbaren lokalen Historie berechnet.</p>");
  body += F(R"HTML(
    <div class='section-head'>
      <div><h2>Verlauf</h2><div class='muted'>Verbrauch und Einspeisung direkt auf dem Tracker</div></div>
      <a href='/history'>Erweiterte Auswertung öffnen</a>
    </div>
    <div class='card chart-card'>
      <div class='chart-controls'>
        <div><label for='dashRange'>Zeitraum</label><select id='dashRange'>
          <option value='hour'>1 Stunde</option><option value='day' selected>Kalendertag (00:00–24:00)</option>
          <option value='week'>Kalenderwoche (Mo–So)</option><option value='month'>Kalendermonat</option>
          <option value='year'>Kalenderjahr</option>
        </select></div>
      </div>
      <div id='dashDateNav' class='date-nav'>
        <label><span id='dashAnchorLabel'>Zeitpunkt</span><input id='dashDate' type='datetime-local' step='3600'></label>
        <button id='dashPrev' type='button' class='secondary'>← Vorheriger Zeitraum</button>
        <button id='dashToday' type='button' class='secondary'>Aktueller Zeitraum</button>
        <button id='dashNext' type='button' class='secondary'>Nächster Zeitraum →</button>
        <label class='date-slider'><span id='dashSliderLabel'>Schnell zurückspulen</span>
          <input id='dashDaysBack' type='range' min='0' max='730' value='0'>
          <output id='dashDaysLabel'>Aktuell</output>
        </label>
      </div>
      <div class='stats'>
        <div class='stat'><span id='averageLabel' class='muted'>Ø Leistung im Zeitraum</span><strong id='morningAverage'>–</strong><div id='averageYearCompare' class='metric-comparison'>–</div><small id='averageYearBaseline' class='metric-baseline'>Jahres-Ø: –</small></div>
        <div class='stat'><span id='periodImportLabel' class='muted'>Netzbezug im Zeitraum</span><strong id='periodImport'>–</strong><div id='importYearCompare' class='metric-comparison'>–</div><small id='importYearBaseline' class='metric-baseline'>Jahres-Ø pro Tag: –</small></div>
        <div class='stat'><span id='periodExportLabel' class='muted'>Einspeisung im Zeitraum</span><strong id='periodExport'>–</strong><div id='exportYearCompare' class='metric-comparison'>–</div><small id='exportYearBaseline' class='metric-baseline'>Jahres-Ø pro Tag: –</small></div>
      </div>
      <div id='dashLoading' class='loading'><span class='spinner'></span>Diagramm wird geladen …</div>
      <div id='dashEmpty' class='empty-state' hidden>Noch keine historischen Werte vorhanden.</div>
      <section class='chart-section'>
        <h2>Leistung, Netzbezug und Einspeisung</h2>
        <div id='powerChartWrap' class='dashboard-chart' hidden>
          <canvas id='powerChart' aria-label='Leistungsverlauf'></canvas>
          <div id='powerTip' class='tooltip'></div>
        </div>
        <div id='powerLegend' class='legend-row'>
          <button type='button' class='legend-item legend-toggle' data-series='power' aria-pressed='true'><i class='swatch' style='background:var(--chart-power)'></i>Gesamtleistung</button>
          <button type='button' class='legend-item legend-toggle' data-series='import' aria-pressed='true'><i class='swatch' style='background:var(--chart-import)'></i>Netzbezug</button>
          <button type='button' class='legend-item legend-toggle' data-series='export' aria-pressed='true'><i class='swatch' style='background:var(--chart-export)'></i>Einspeisung</button>
          <button type='button' class='legend-item legend-toggle' data-series='gap' aria-pressed='true'><i class='swatch' style='background:var(--chart-gap)'></i>Datenlücke / Ausfall</button>
        </div>
      </section>
      <p id='dashSummary' class='chart-note muted'></p>
      <p class='chart-note muted'>Messbalken: Maus oder Finger bewegen.
      Doppelklick oder Doppeltippen fixiert; ein einfacher Klick oder Tipp
      löst ihn wieder.</p>
    </div>
    <p style='margin-top:28px'><span class='status-pill'><i class='dot'></i> Lokal · ohne Cloud</span></p>)HTML");

  if (!sendPageStreamed("Dashboard", body, "/assets/dashboard.js?v=" + String(kFirmwareVersion))) {
    eventLog.add("ERROR", "DASHBOARD_PAGE_INCOMPLETE",
                 "Dashboard-Seite konnte nicht vollstaendig erzeugt werden");
    server.send(503, "text/plain; charset=utf-8",
                "Dashboard voruebergehend nicht verfuegbar. Bitte neu laden.");
  }
}

void handleHistoryPage() {
  if (!requireAdmin()) return;
  const String body = F(R"HTML(
    <div class='card'>
      <div class='toolbar'>
        <div><label>Zeitraum</label><select id='range'>
          <option value='hour'>1 Stunde</option><option value='day' selected>Kalendertag (00:00–24:00)</option>
          <option value='week'>Kalenderwoche (Mo–So)</option><option value='month'>Kalendermonat</option>
          <option value='year'>Kalenderjahr</option><option value='all'>Langzeit</option>
        </select></div>
        <div><label>Messwert</label><select id='series'>
          <option value='power'>Leistung</option><option value='combined'>Bezug und Einspeisung</option><option value='import'>Netzbezug</option>
          <option value='export'>Einspeisung</option>
        </select></div>
        <div id='modeBox'><label>Leistungsanzeige</label><select id='metric'>
          <option value='avg'>Durchschnitt</option><option value='minmax'>Durchschnitt mit Min/Max</option>
        </select></div>
        <button id='zoomIn' class='secondary' type='button' title='Hineinzoomen'>Zoom +</button>
        <button id='zoomOut' class='secondary' type='button' title='Herauszoomen'>Zoom −</button>
        <button id='reset' class='secondary' type='button'>Gesamt</button>
      </div>
      <div id='historyDateNav' class='date-nav' hidden>
        <label><span id='historyAnchorLabel'>Zeitpunkt</span><input id='historyDate' type='datetime-local' step='3600'></label>
        <button id='historyPrev' type='button' class='secondary'>← Vorheriger Zeitraum</button>
        <button id='historyToday' type='button' class='secondary'>Aktueller Zeitraum</button>
        <button id='historyNext' type='button' class='secondary'>Nächster Zeitraum →</button>
        <label class='date-slider'><span id='historySliderLabel'>Schnell zurückspulen</span>
          <input id='historyDaysBack' type='range' min='0' max='730' value='0'>
          <output id='historyDaysLabel'>Aktuell</output>
        </label>
      </div>
      <div class='stats'>
        <div class='stat'><span id='historyAverageLabel' class='muted'>Ø Leistung im Zeitraum</span><strong id='historyAverage'>–</strong></div>
        <div class='stat'><span id='historyImportLabel' class='muted'>Netzbezug im Zeitraum</span><strong id='todayImport'>–</strong></div>
        <div class='stat'><span id='historyExportLabel' class='muted'>Einspeisung im Zeitraum</span><strong id='todayExport'>–</strong></div>
      </div>
      <div id='legend' class='legend-row'></div>
      <div id='loading' class='loading'><span class='spinner'></span><span>Historie wird geladen …</span></div>
      <div id='error' class='error' hidden></div>
      <div id='chartWrap' class='chart-wrap' hidden>
        <canvas id='chart' aria-label='Historisches Messwertdiagramm'></canvas><div id='tooltip' class='tooltip'></div>
      </div>
      <p id='summary' class='muted'></p>
      <p class='muted'>Maus oder Finger: Messbalken verschieben. Doppelklick oder Doppeltippen fixiert ihn. Einfaches Klicken oder Tippen löst ihn wieder. Mausrad: zoomen. Umschalt+Ziehen: Zeitraum verschieben.</p>
      <p><a id='csv' href='/api/v1/history.csv?range=complete'>Vollständige Historie als CSV exportieren</a></p>
    </div>)HTML");

  if (!sendPageStreamed("Lokale Historie", body,
                        "/assets/history.js?v=" + String(kFirmwareVersion))) {
    eventLog.add("ERROR", "HISTORY_PAGE_INCOMPLETE",
                 "Historienseite konnte nicht vollstaendig erzeugt werden");
    server.send(503, "text/plain; charset=utf-8",
                "Historie voruebergehend nicht verfuegbar. Bitte neu laden.");
  }
}

struct HistoryQuery {
  HistoryStore::Tier tier;
  uint32_t since;
  uint32_t until;
};

uint32_t historyTierSeconds(HistoryStore::Tier tier) {
  switch (tier) {
    case HistoryStore::Tier::Minute: return 60;
    case HistoryStore::Tier::QuarterHour: return 900;
    case HistoryStore::Tier::Hour: return 3600;
    case HistoryStore::Tier::Day: return 86400;
  }
  return 60;
}

uint32_t requestedHistoryAnchor(uint32_t now) {
  const String value = server.arg("anchor");
  if (!value.length()) return now;
  if (value.length() != 10) return now;
  for (char c : value)
    if (!isDigit(c)) return now;
  const uint64_t parsed = strtoull(value.c_str(), nullptr, 10);
  return parsed >= 1577836800ULL && parsed <= now
             ? static_cast<uint32_t>(parsed)
             : now;
}

HistoryQuery calendarHistoryQuery(const String &range, uint32_t now) {
  time_t anchor = static_cast<time_t>(requestedHistoryAnchor(now));
  struct tm start {};
  localtime_r(&anchor, &start);
  start.tm_sec = 0;
  if (range == "hour") {
    start.tm_min = 0;
  } else {
    start.tm_hour = 0;
    start.tm_min = 0;
    if (range == "week")
      start.tm_mday -= (start.tm_wday + 6) % 7;  // DE: Montag | EN: Monday
    else if (range == "month")
      start.tm_mday = 1;
    else if (range == "year") {
      start.tm_mon = 0;
      start.tm_mday = 1;
    }
  }
  start.tm_isdst = -1;
  const time_t since = mktime(&start);
  struct tm end = start;
  if (range == "hour")
    end.tm_hour += 1;
  else if (range == "week")
    end.tm_mday += 7;
  else if (range == "month")
    end.tm_mon += 1;
  else if (range == "year")
    end.tm_year += 1;
  else
    end.tm_mday += 1;
  end.tm_isdst = -1;
  const time_t until = mktime(&end);
  // DE: Die Stufe muss den gesamten angefragten Zeitraum abdecken. Eine
  // Auswahl anhand des Periodenendes kann bei einem alten Kalendertag eine
  // bereits teilweise ueberschriebene Minutenstufe liefern, obwohl die
  // vollstaendige 15-Minuten-Historie noch vorhanden ist.
  // EN: The tier must cover the complete requested period. Selecting by the
  // period end can return a partially overwritten minute tier for an older
  // calendar day even though complete quarter-hour history is still present.
  const uint32_t age = now > static_cast<uint32_t>(since)
                           ? now - static_cast<uint32_t>(since)
                           : 0;
  HistoryStore::Tier tier;
  if (range == "year") {
    tier = HistoryStore::Tier::Day;
  } else if (range == "week" || range == "month") {
    tier = age < 179UL * 86400UL
               ? HistoryStore::Tier::QuarterHour
               : age < 729UL * 86400UL ? HistoryStore::Tier::Hour
                                       : HistoryStore::Tier::Day;
  } else {
    tier = age < 47UL * 3600UL
               ? HistoryStore::Tier::Minute
               : age < 179UL * 86400UL
                     ? HistoryStore::Tier::QuarterHour
                     : age < 729UL * 86400UL ? HistoryStore::Tier::Hour
                                             : HistoryStore::Tier::Day;
  }
  return {tier, static_cast<uint32_t>(since), static_cast<uint32_t>(until)};
}

HistoryQuery historyQuery() {
  const uint32_t now = time(nullptr);
  const String range = server.arg("range");
  if (range == "minute_all")
    return {HistoryStore::Tier::Minute, 0, now};
  if (range == "quarter_all")
    return {HistoryStore::Tier::QuarterHour, 0, now};
  if (range == "hour_all")
    return {HistoryStore::Tier::Hour, 0, now};
  if (range == "day_all")
    return {HistoryStore::Tier::Day, 0, now};
  if (range == "compare")
    return {HistoryStore::Tier::QuarterHour, now - 3 * 86400, now};
  if (range == "hour" || range == "day" || range == "week" ||
      range == "month" || range == "year")
    return calendarHistoryQuery(range, now);
  return {HistoryStore::Tier::Day, 0, now};
}

struct EnergyDelta {
  double importKwh = NAN;
  double exportKwh = NAN;
  uint32_t firstTimestamp = 0;
};

EnergyDelta storedEnergyDelta(HistoryStore::Tier tier, uint32_t since,
                              uint32_t until, double finalImport = NAN,
                              double finalExport = NAN) {
  double firstImport = NAN;
  double firstExport = NAN;
  double lastImport = NAN;
  double lastExport = NAN;
  EnergyDelta result;
  history.forEach(tier, since, until,
                  [&](const HistoryStore::Record &record) {
                    if (!result.firstTimestamp &&
                        (std::isfinite(record.importKwh) ||
                         std::isfinite(record.exportKwh)))
                      result.firstTimestamp = record.timestamp;
                    if (std::isfinite(record.importKwh)) {
                      if (!std::isfinite(firstImport))
                        firstImport = record.importKwh;
                      lastImport = record.importKwh;
                    }
                    if (std::isfinite(record.exportKwh)) {
                      if (!std::isfinite(firstExport))
                        firstExport = record.exportKwh;
                      lastExport = record.exportKwh;
                    }
                    return true;
                  });
  if (std::isfinite(finalImport)) lastImport = finalImport;
  if (std::isfinite(finalExport)) lastExport = finalExport;
  if (std::isfinite(firstImport) && std::isfinite(lastImport))
    result.importKwh = std::max(0.0, lastImport - firstImport);
  if (std::isfinite(firstExport) && std::isfinite(lastExport))
    result.exportKwh = std::max(0.0, lastExport - firstExport);
  return result;
}

void handleDashboardSummary() {
  if (!requireAdmin()) return;
  const time_t now = time(nullptr);
  if (now < 1700000000) {
    server.send(503, "application/json", "{\"error\":\"time_not_valid\"}");
    return;
  }
  struct tm localNow {};
  localtime_r(&now, &localNow);
  struct tm todayTm = localNow;
  todayTm.tm_hour = 0;
  todayTm.tm_min = 0;
  todayTm.tm_sec = 0;
  todayTm.tm_isdst = -1;
  const time_t todayStart = mktime(&todayTm);
  struct tm yesterdayTm = todayTm;
  yesterdayTm.tm_mday -= 1;
  yesterdayTm.tm_isdst = -1;
  const time_t yesterdayStart = mktime(&yesterdayTm);
  struct tm yesterdayCutoffTm = localNow;
  yesterdayCutoffTm.tm_mday -= 1;
  yesterdayCutoffTm.tm_isdst = -1;
  const time_t yesterdayCutoff = mktime(&yesterdayCutoffTm);
  struct tm yearTm = localNow;
  yearTm.tm_mon = 0;
  yearTm.tm_mday = 1;
  yearTm.tm_hour = 0;
  yearTm.tm_min = 0;
  yearTm.tm_sec = 0;
  yearTm.tm_isdst = -1;
  const time_t yearStart = mktime(&yearTm);
  const uint32_t todayBaseline =
      todayStart > 120 ? static_cast<uint32_t>(todayStart - 120) : 0;
  const uint32_t yesterdayBaseline =
      yesterdayStart > 120 ? static_cast<uint32_t>(yesterdayStart - 120) : 0;
  const uint32_t yearBaseline =
      yearStart > 2 * 86400 ? static_cast<uint32_t>(yearStart - 2 * 86400) : 0;
  const EnergyDelta today = storedEnergyDelta(
      HistoryStore::Tier::Minute, todayBaseline, static_cast<uint32_t>(now),
      meter.importKwh, meter.exportKwh);
  const EnergyDelta yesterday = storedEnergyDelta(
      HistoryStore::Tier::Minute, yesterdayBaseline,
      static_cast<uint32_t>(yesterdayCutoff));
  const EnergyDelta year = storedEnergyDelta(
      HistoryStore::Tier::Day, yearBaseline, static_cast<uint32_t>(now),
      meter.importKwh, meter.exportKwh);
  const uint32_t coverageStart =
      year.firstTimestamp
          ? std::max(static_cast<uint32_t>(yearStart), year.firstTimestamp)
          : 0;
  const double coverageDays =
      coverageStart && now > coverageStart
          ? std::max(1.0, (now - coverageStart) / 86400.0)
          : NAN;
  const double yearAveragePower =
      std::isfinite(coverageDays) && std::isfinite(year.importKwh) &&
              std::isfinite(year.exportKwh)
          ? (year.importKwh - year.exportKwh) * 1000.0 /
                (coverageDays * 24.0)
          : NAN;
  double changePercent = NAN;
  if (std::isfinite(today.importKwh) &&
      std::isfinite(yesterday.importKwh) && yesterday.importKwh > 0.000001)
    changePercent =
        (today.importKwh - yesterday.importKwh) / yesterday.importKwh * 100.0;
  String json;
  json.reserve(420);
  json = "{\"today_import_kwh\":" + numberOrNull(today.importKwh, 4) +
         ",\"today_export_kwh\":" + numberOrNull(today.exportKwh, 4) +
         ",\"yesterday_same_time_import_kwh\":" +
         numberOrNull(yesterday.importKwh, 4) +
         ",\"import_change_percent\":" +
         numberOrNull(changePercent, 1) +
         ",\"year_import_kwh\":" + numberOrNull(year.importKwh, 3) +
         ",\"year_export_kwh\":" + numberOrNull(year.exportKwh, 3) +
         ",\"year_average_power_w\":" +
         numberOrNull(yearAveragePower, 1) +
         ",\"year_daily_average_import_kwh\":" +
         numberOrNull(std::isfinite(year.importKwh) &&
                              std::isfinite(coverageDays)
                          ? year.importKwh / coverageDays
                          : NAN,
                      3) +
         ",\"year_daily_average_export_kwh\":" +
         numberOrNull(std::isfinite(year.exportKwh) &&
                              std::isfinite(coverageDays)
                          ? year.exportKwh / coverageDays
                          : NAN,
                      3) +
         ",\"year_coverage_days\":" + numberOrNull(coverageDays, 2) + "}";
  server.send(200, "application/json", json);
}

void handleHistoryJson() {
  if (!requireAdmin()) return;
  WiFiClient responseClient = server.client();
  const uint32_t now = time(nullptr);
  const String range = server.arg("range");
  const HistoryQuery query = historyQuery();
  const bool currentLiveHour =
      range == "hour" && query.since <= now && query.until >= now;
  if (currentLiveHour || range == "live") {
    const uint32_t since =
        range == "live" ? (now > 3600 ? now - 3600 : 0) : query.since;
    const uint32_t until = range == "live" ? now : query.until;
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    server.sendContent("{\"from\":" + String(since) + ",\"to\":" +
                       String(until) + ",\"step\":5,\"values\":[");
    String chunk;
    chunk.reserve(1000);
    const size_t first =
        liveCount < kLiveSamples ? 0 : liveWriteIndex;
    bool firstValue = true;
    for (size_t i = 0; i < liveCount; ++i) {
      if (!responseClient.connected()) break;
      const LiveSample &sample = liveSamples[(first + i) % kLiveSamples];
      if (sample.timestamp < since || sample.timestamp > now) continue;
      if (!firstValue) chunk += ',';
      firstValue = false;
      chunk += "{\"ts\":" + String(sample.timestamp) +
               ",\"avg\":" + numberOrNull(sample.powerW, 2) +
               ",\"min\":" + numberOrNull(sample.powerW, 2) +
               ",\"max\":" + numberOrNull(sample.powerW, 2) +
               ",\"import\":" + numberOrNull(sample.importKwh, 4) +
               ",\"export\":" + numberOrNull(sample.exportKwh, 4) + "}";
      if (chunk.length() > 900) {
        server.sendContent(chunk);
        chunk = "";
        if (!responseClient.connected()) break;
      }
    }
    if (responseClient.connected() && chunk.length()) server.sendContent(chunk);
    if (responseClient.connected()) server.sendContent("]}");
    return;
  }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"from\":" + String(query.since) + ",\"to\":" +
                     String(query.until) + ",\"step\":" +
                     String(historyTierSeconds(query.tier)) + ",\"values\":[");
  bool first = true;
  String chunk;
  chunk.reserve(1200);
  history.forEach(query.tier, query.since, query.until,
                   [&](const HistoryStore::Record &record) {
                     if (!responseClient.connected()) return false;
                     if (!first) chunk += ',';
                    first = false;
                    chunk += "{\"ts\":" + String(record.timestamp) +
                             ",\"avg\":" + String(record.averageW, 2) +
                             ",\"min\":" + String(record.minimumW, 2) +
                             ",\"max\":" + String(record.maximumW, 2) +
                             ",\"import\":" + numberOrNull(record.importKwh, 4) +
                             ",\"export\":" + numberOrNull(record.exportKwh, 4) + "}";
                     if (chunk.length() > 900) {
                       server.sendContent(chunk);
                       chunk = "";
                       if (!responseClient.connected()) return false;
                     }
                     return true;
                   });
  if (responseClient.connected() && chunk.length()) server.sendContent(chunk);
  if (responseClient.connected()) server.sendContent("]}");
}

void handleHistoryCsv() {
  if (!requireAdmin()) return;
  requestCpuBoost("history_export");
  const uint32_t now = time(nullptr);
  const String range = server.arg("range");
  if (range == "complete") {
    server.sendHeader(
        "Content-Disposition",
        "attachment; filename=irtracker-complete-history.csv");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/csv; charset=utf-8", "");
    server.sendContent(
        "timestamp,average_w,minimum_w,maximum_w,import_kwh,export_kwh,"
        "resolution_seconds\n");
    String chunk;
    chunk.reserve(1200);
    const auto appendRecord =
        [&](const HistoryStore::Record &record, uint32_t resolution) {
          chunk += String(record.timestamp) + "," +
                   String(record.averageW, 2) + "," +
                   String(record.minimumW, 2) + "," +
                   String(record.maximumW, 2) + "," +
                   numberOrNull(record.importKwh, 4) + "," +
                   numberOrNull(record.exportKwh, 4) + "," +
                   String(resolution) + "\n";
          if (chunk.length() > 900) {
            server.sendContent(chunk);
            chunk = "";
          }
          return true;
        };
    const uint32_t minuteSince =
        now > 48UL * 3600UL ? now - 48UL * 3600UL : 0;
    const uint32_t quarterSince =
        now > 180UL * 86400UL ? now - 180UL * 86400UL : 0;
    const uint32_t hourSince =
        now > 730UL * 86400UL ? now - 730UL * 86400UL : 0;
    if (hourSince)
      history.forEach(HistoryStore::Tier::Day, 0, hourSince - 1,
                      [&](const HistoryStore::Record &record) {
                        return appendRecord(record, 86400);
                      });
    if (quarterSince > hourSince)
      history.forEach(HistoryStore::Tier::Hour, hourSince, quarterSince - 1,
                      [&](const HistoryStore::Record &record) {
                        return appendRecord(record, 3600);
                      });
    if (minuteSince > quarterSince)
      history.forEach(
          HistoryStore::Tier::QuarterHour, quarterSince, minuteSince - 1,
          [&](const HistoryStore::Record &record) {
            return appendRecord(record, 900);
          });
    history.forEach(HistoryStore::Tier::Minute, minuteSince, now,
                    [&](const HistoryStore::Record &record) {
                      return appendRecord(record, 60);
                    });
    const uint32_t currentMinute = now - now % 60;
    const size_t first = liveCount < kLiveSamples ? 0 : liveWriteIndex;
    for (size_t i = 0; i < liveCount; ++i) {
      const LiveSample &sample = liveSamples[(first + i) % kLiveSamples];
      if (sample.timestamp < currentMinute || sample.timestamp > now) continue;
      chunk += String(sample.timestamp) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.importKwh, 4) + "," +
               numberOrNull(sample.exportKwh, 4) + ",5\n";
      if (chunk.length() > 900) {
        server.sendContent(chunk);
        chunk = "";
      }
    }
    if (chunk.length()) server.sendContent(chunk);
    return;
  }
  const HistoryQuery query = historyQuery();
  const bool currentLiveHour =
      range == "hour" && query.since <= now && query.until >= now;
  if (currentLiveHour || range == "live") {
    const uint32_t since =
        range == "live" ? (now > 3600 ? now - 3600 : 0) : query.since;
    server.sendHeader("Content-Disposition",
                      "attachment; filename=irtracker-1hour.csv");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/csv; charset=utf-8", "");
    server.sendContent(
        "timestamp,average_w,minimum_w,maximum_w,import_kwh,export_kwh\n");
    String chunk;
    const size_t first =
        liveCount < kLiveSamples ? 0 : liveWriteIndex;
    for (size_t i = 0; i < liveCount; ++i) {
      const LiveSample &sample = liveSamples[(first + i) % kLiveSamples];
      if (sample.timestamp < since || sample.timestamp > now) continue;
      chunk += String(sample.timestamp) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.powerW, 2) + "," +
               numberOrNull(sample.importKwh, 4) + "," +
               numberOrNull(sample.exportKwh, 4) + "\n";
      if (chunk.length() > 900) {
        server.sendContent(chunk);
        chunk = "";
      }
    }
    if (chunk.length()) server.sendContent(chunk);
    return;
  }
  server.sendHeader("Content-Disposition",
                    "attachment; filename=irtracker-history.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv; charset=utf-8", "");
  server.sendContent(
      "timestamp,average_w,minimum_w,maximum_w,import_kwh,export_kwh\n");
  String chunk;
  chunk.reserve(1200);
  history.forEach(query.tier, query.since, query.until,
                  [&](const HistoryStore::Record &record) {
                    chunk += String(record.timestamp) + "," +
                             String(record.averageW, 2) + "," +
                             String(record.minimumW, 2) + "," +
                             String(record.maximumW, 2) + "," +
                             String(record.importKwh, 4) + "," +
                             String(record.exportKwh, 4) + "\n";
                    if (chunk.length() > 900) {
                      server.sendContent(chunk);
                      chunk = "";
                    }
                    return true;
                  });
  if (chunk.length()) server.sendContent(chunk);
}
