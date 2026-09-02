(async()=>{
const root=document.getElementById('setupRoot'),config=window.IR_TRACKER_SETUP;
const response=await fetch('/assets/setup.html?v='+window.IR_TRACKER_CONFIG.firmwareVersion);
if(!response.ok){root.textContent='Einstellungen konnten nicht geladen werden.';return}
root.innerHTML=await response.text();
const form=root.querySelector('form'),field=name=>form.elements[name],set=(name,value)=>{field(name).value=value??''},check=(name,value)=>{field(name).checked=!!value};
config.ssids.forEach((ssid,index)=>{const row=document.createElement('div');row.className='inline';row.innerHTML=`<div><label>WLAN ${index+1}</label><input name="ssid${index}" maxlength="32" placeholder="Netzwerkname"></div><div><label>Passwort</label><input type="password" name="pass${index}" maxlength="64" autocomplete="off" data-lpignore="true" placeholder="${ssid?'gespeichert':'offenes WLAN'}"></div>`;row.querySelector(`[name=ssid${index}]`).value=ssid;document.getElementById('wifiSlots').appendChild(row)});
const addPins=(name,selected,off)=>{const select=field(name);if(off)select.add(new Option('Aus','-1'));config.gpios.forEach(pin=>select.add(new Option(pin,pin)));select.value=String(selected)};
addPins('rx_pin',config.rx_pin,false);addPins('tx_pin',config.tx_pin,true);addPins('led_pin',config.led_pin,true);
[300,600,1200,2400,4800,9600,19200,38400,115200].forEach(rate=>field('baud').add(new Option(rate,rate)));
set('hostname',config.hostname);set('timezone',config.timezone);set('ap_minutes',config.ap_minutes);set('meter_protocol',config.meter_protocol);set('baud',config.baud);set('api_access',config.api_access);set('mqtt_host',config.mqtt_host);set('mqtt_port',config.mqtt_port);set('mqtt_user',config.mqtt_user);field('mqtt_pass').placeholder=config.mqtt_password_saved?'gespeichert':'optional';
['led_inv','storage_compat','modbus_tcp','event_flash','ha_disc','eco_mode','eco_led_off','wifi_power_auto','wifi_ps','gh_check','gh_auto'].forEach(name=>check(name,config[name]));
if(config.developer_io){document.getElementById('developerIo').hidden=false;check('sniffer',config.sniffer);check('bridge',config.bridge)}
})();
