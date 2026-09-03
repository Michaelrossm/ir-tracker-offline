(()=>{
const el=id=>document.getElementById(id),out=el('selftest');
const shown=(v,d=3,u='')=>v==null?'nicht verfügbar':Number(v).toLocaleString('de-DE',{maximumFractionDigits:d})+(u?' '+u:'');
async function loadDiagnosis(){try{const r=await fetch('/api/v1/meter-report',{cache:'no-store'});if(!r.ok)throw Error(r.status);const m=await r.json(),d=m.diagnosis,v=m.values,s=m.system;
const labels={ok:'OK',warn:'HINWEIS',error:'FEHLER'},colors={ok:'#63e68b',warn:'#ffb454',error:'#ff8d69'},badge=el('diagnosisState');badge.textContent=labels[d.state]||'HINWEIS';badge.style.borderColor=colors[d.state];badge.style.color=colors[d.state];
el('diagnosisSummary').textContent=d.summary;el('diagnosisHints').replaceChildren(...d.hints.map(h=>{const li=document.createElement('li');li.textContent=h;return li}));
const integrity=m.integrity_present?(m.last_crc_valid?'gültig':'fehlerhaft'):'nicht geliefert';
el('diagnosisIr').textContent=`RX-Bytes: ${m.rx_bytes} · Telegramme: ${m.telegram_count} · Letztes: ${m.telegram_age_s==null?'nicht verfügbar':m.telegram_age_s+' s'} · CRC/Integrität: ${integrity} · Parsefehler: ${m.parse_errors} · CRC-Fehler: ${m.crc_errors}`;
el('diagnosisProtocol').textContent=`Konfiguriert: ${m.configured_protocol} · Erkannt: ${m.protocol}`;
const phases=m.phases.map(p=>`${p.phase}: ${shown(p.power_w,1,'W')} / ${shown(p.voltage_v,1,'V')} / ${shown(p.current_a,3,'A')}`).join(' · ');
el('diagnosisValues').textContent=`Leistung: ${shown(v.power_w,1,'W')} · Bezug: ${shown(v.import_kwh,6,'kWh')} · Einspeisung: ${shown(v.export_kwh,6,'kWh')} · ${phases}`;
const mqtt={connected:'verbunden',disconnected:'nicht verbunden',not_configured:'nicht konfiguriert'}[s.mqtt_state]||s.mqtt_state;
const modbus=s.modbus_enabled?` · Modbus: ${s.modbus_running?'aktiv':'inaktiv'}, Verbindungen ${s.modbus_connections}, gültig ${s.modbus_valid_requests}, ungültig ${s.modbus_invalid_requests}, letzter Client ${s.modbus_last_client??'–'}`:'';
el('diagnosisSystem').textContent=`Verbindung: ${s.transport} · WLAN-Signal: ${s.wifi_rssi==null?'nicht aktiv':s.wifi_rssi+' dBm'} · Heap: ${s.free_heap} Byte · Minimum: ${s.minimum_free_heap} Byte · Historie: ${s.history_ready?'bereit':'Fehler'} · MQTT: ${mqtt}${modbus}`}
catch(e){el('diagnosisState').textContent='FEHLER';el('diagnosisSummary').textContent='Diagnosedaten konnten nicht geladen werden. Verbindung zum Tracker prüfen.'}}
async function copyReport(technical){const status=el('copyStatus');try{const r=await fetch('/api/v1/support-report'+(technical?'?technical=1':''),{cache:'no-store'});if(!r.ok)throw Error(r.status);const text=await r.text();
if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText(text);else{const area=document.createElement('textarea');area.value=text;area.style.position='fixed';area.style.opacity='0';document.body.appendChild(area);area.select();if(!document.execCommand('copy'))throw Error('copy');area.remove()}status.textContent=technical?'Technische Diagnose kopiert.':'Diagnose für Support kopiert.'}
catch(e){status.textContent='Kopieren nicht möglich. Bitte Browserberechtigung prüfen.'}}
el('copySupport').onclick=()=>copyReport(false);el('copyTechnical').onclick=()=>copyReport(true);
async function test(){out.innerHTML='<div class="loading"><span class="spinner"></span>Prüfung läuft …</div>';
try{const r=await fetch('/api/v1/selftest',{cache:'no-store'});if(!r.ok)throw Error(r.status);const j=await r.json(),c={ok:'#63e68b',warn:'#ffb454',error:'#ff8d69',off:'#9bb3a4'};
out.innerHTML=j.tests.map(t=>`<div class='stat' style='border-color:${c[t.state]}'><span style='color:${c[t.state]};font-weight:700'>${t.state==='ok'?'OK':t.state==='warn'?'HINWEIS':t.state==='error'?'FEHLER':'OPTIONAL'}</span><strong>${t.label}</strong><small>${t.detail}</small></div>`).join('')}
catch(e){out.innerHTML='<div class="error">Selbsttest konnte nicht geladen werden. Verbindung zum Tracker prüfen.</div>'}}
el('runSelftest').onclick=test;loadDiagnosis();test();
})();
