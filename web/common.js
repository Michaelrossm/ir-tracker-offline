(function(){
const cfg=window.IR_TRACKER_CONFIG||{};
if(location.href.includes('@'))history.replaceState(null,'',location.pathname+location.search+location.hash);
try{const t=JSON.parse(localStorage.getItem('irtracker-theme-v1')||'{}');['--bg','--card'].forEach(k=>{const v=t[k];if(/^#[0-9a-f]{6}$/i.test(v||''))document.documentElement.style.setProperty(k,v)})}catch(e){}
const nativeFetch=window.fetch.bind(window);
window.fetch=function(input,init){init=init||{};const target=typeof input==='string'&&input.startsWith('/')?location.protocol+'//'+location.host+input:input,method=(init.method||'GET').toUpperCase();if(method==='POST'&&cfg.csrfToken){init.headers=new Headers(init.headers||{});init.headers.set('X-CSRF-Token',cfg.csrfToken)}return nativeFetch(target,init)};
function ready(){
if(cfg.csrfToken)document.querySelectorAll("form[method='post'],form[method='POST']").forEach(f=>{if(!f.querySelector("input[name='csrf_token']")){const i=document.createElement('input');i.type='hidden';i.name='csrf_token';i.value=cfg.csrfToken;f.insertBefore(i,f.firstChild)}});
const actionMessage=document.createElement('div');actionMessage.id='actionMessage';actionMessage.hidden=true;actionMessage.setAttribute('role','alert');const heading=document.querySelector('h1');if(heading)heading.insertAdjacentElement('afterend',actionMessage);
const actionErrors={
invalid_wifi_credentials:['WLAN-Daten ungültig. Netzwerkname und Passwort des markierten Eintrags prüfen.','Invalid Wi-Fi credentials. Check the network name and password of the highlighted entry.'],
invalid_settings_text:['Mindestens ein Textfeld enthält ungültige oder zu viele Zeichen.','At least one text field contains invalid or too many characters.'],
admin_password_too_long:['Das Admin-Passwort darf höchstens 64 Zeichen enthalten.','The administrator password must not exceed 64 characters.'],
admin_password_too_short:['Das Admin-Passwort muss mindestens 4 Zeichen enthalten.','The administrator password must contain at least 4 characters.'],
admin_password_confirmation_mismatch:['Die beiden Admin-Passwörter stimmen nicht überein.','The two administrator passwords do not match.'],
event_log_persistence_change_failed:['Die Einstellung für das dauerhafte Protokoll konnte nicht gespeichert werden.','The persistent log setting could not be saved.'],
csrf_token_invalid:['Die Seite ist nicht mehr aktuell. Bitte neu laden und die Aktion erneut ausführen.','This page is no longer current. Reload it and try the action again.'],
too_many_login_attempts:['Zu viele Anmeldeversuche. Bitte die angezeigte Sperrzeit abwarten.','Too many login attempts. Wait for the indicated lockout period.'],
invalid_energy_configuration:['Die Schnittstellenkonfiguration ist unvollständig oder ungültig.','The interface configuration is incomplete or invalid.'],
live_confirmation_required:['Für eine echte Ausgabe muss LIVE als Bestätigung eingegeben werden.','Enter LIVE to confirm real output.'],
saved_pin_or_ir_output_missing:['Gespeicherte Zähler-PIN oder IR-Sendeausgang fehlt.','The stored meter PIN or IR transmit output is missing.'],
ir_sequence_already_running:['Es läuft bereits eine IR-Sequenz. Bitte warten oder sie stoppen.','An IR sequence is already running. Wait or stop it first.'],
pin_must_have_4_digits:['Die Zähler-PIN muss genau vier Ziffern enthalten.','The meter PIN must contain exactly four digits.'],
pin_must_be_numeric:['Die Zähler-PIN darf nur Ziffern enthalten.','The meter PIN may contain digits only.'],
ir_busy_or_disabled:['Der IR-Ausgang ist deaktiviert oder momentan beschäftigt.','The IR output is disabled or currently busy.'],
unauthorized:['Die Anmeldung ist abgelaufen. Bitte die Seite neu laden und erneut anmelden.','Your login has expired. Reload the page and sign in again.'],
signed_irfw_package_required:['Bitte ein signiertes IRFW-Firmwarepaket auswählen.','Select a signed IRFW firmware package.'],
invalid_signed_firmware:['Das Firmwarepaket ist ungültig oder die Signatur konnte nicht bestätigt werden.','The firmware package is invalid or its signature could not be verified.']};
function actionText(code){const entry=actionErrors[code],english=document.documentElement.lang==='en';return entry?entry[english?1:0]:(english?'The action failed. Technical code: ':'Die Aktion ist fehlgeschlagen. Technischer Code: ')+(code||'unknown_error')}
function showAction(text,ok=false){actionMessage.className=ok?'status-pill':'error';actionMessage.textContent=text;actionMessage.hidden=false;actionMessage.scrollIntoView({behavior:'smooth',block:'nearest'})}
document.addEventListener('submit',async event=>{const form=event.target;if(event.defaultPrevented||!(form instanceof HTMLFormElement)||form.method.toLowerCase()!=='post'||form.enctype==='multipart/form-data'||form.dataset.nativeSubmit==='true')return;event.preventDefault();actionMessage.hidden=true;const buttons=[...form.querySelectorAll("button[type='submit'],input[type='submit']")];buttons.forEach(button=>button.disabled=true);try{const headers=new Headers({'Accept':'application/json, text/html;q=0.9'});if(cfg.csrfToken)headers.set('X-CSRF-Token',cfg.csrfToken);const response=await nativeFetch(form.action,{method:'POST',headers,body:new URLSearchParams(new FormData(form)),redirect:'follow'});if(response.redirected){location.assign(response.url);return}const type=response.headers.get('content-type')||'';if(type.includes('text/html')){const html=await response.text();document.open();document.write(html);document.close();return}let result={};try{result=await response.json()}catch(e){}if(!response.ok){showAction(actionText(result.error));return}showAction(document.documentElement.lang==='en'?'Action completed successfully.':'Aktion erfolgreich ausgeführt.',true)}catch(error){showAction(document.documentElement.lang==='en'?'The tracker could not be reached. Check the Wi-Fi connection.':'Der Tracker konnte nicht erreicht werden. WLAN-Verbindung prüfen.')}finally{buttons.forEach(button=>button.disabled=false)}},{capture:false});
const key='irtracker-theme-v1';
const defaults={'--bg':'#07100c','--card':'#10231a'};
const panel=document.getElementById('themePanel'),toggle=document.getElementById('themeToggle'),close=document.getElementById('themeClose'),reset=document.getElementById('themeReset'),inputs=[...document.querySelectorAll('[data-theme-var]')];
function stored(){try{const v=JSON.parse(localStorage.getItem(key)||'{}');return v&&typeof v==='object'?v:{}}catch(e){return {}}}
function color(name){const v=getComputedStyle(document.documentElement).getPropertyValue(name).trim();return /^#[0-9a-f]{6}$/i.test(v)?v:defaults[name]}
function sync(){inputs.forEach(i=>i.value=color(i.dataset.themeVar))}
function apply(values,save){const clean={};Object.entries(defaults).forEach(([name,fallback])=>{const value=values[name];clean[name]=/^#[0-9a-f]{6}$/i.test(value||'')?value:fallback;document.documentElement.style.setProperty(name,clean[name])});if(save){try{localStorage.setItem(key,JSON.stringify(clean))}catch(e){}}window.dispatchEvent(new CustomEvent('irtracker-theme-change'))}
function show(on){panel.hidden=!on;toggle.setAttribute('aria-expanded',String(on));if(on)sync()}
toggle.addEventListener('click',()=>show(panel.hidden));close.addEventListener('click',()=>show(false));
inputs.forEach(input=>input.addEventListener('input',()=>{const values=stored();values[input.dataset.themeVar]=input.value;apply(values,true)}));
reset.addEventListener('click',()=>{try{localStorage.removeItem(key)}catch(e){}apply(defaults,false);sync()});
document.addEventListener('keydown',e=>{if(e.key==='Escape'&&!panel.hidden)show(false)});sync();
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',ready,{once:true});else ready();
})();
