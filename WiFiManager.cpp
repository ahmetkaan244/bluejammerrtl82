#include "WiFiManager.h"
#include "debug.h"

// RTL8720DN low-level SDK for promiscuous mode, channel set, raw frame send
extern "C" {
    #include <wifi_conf.h>
}

WiFiManager::WiFiManager()
    : _currentMode(WIFI_MODE_STATION),
      _networkCount(0),
      _scanStartTime(0),
      _scanInProgress(false),
      _promiscuousActive(false),
      _apActive(false),
      _apChannel(1)
{
    _apSSID[0] = '\0';
}

void WiFiManager::begin() {
    // RTL8720DN: WiFi zaten SDK tarafindan baslatildi.
    // WiFi.begin("") cagirmak otomatik baglanti dener ve 20+ saniye bloklar.
    // Sadece varsa bekleyen baglantiyi kes, AP/promiscuous mod ayri fonksiyonlarla baslatilir.
    WiFi.disconnect();
    _currentMode = WIFI_MODE_STATION;
    DBG_PRINTF("[WiFiManager] Baslatildi.\n");
}

void WiFiManager::setMode(WiFiOpMode mode) {
    if (_currentMode == mode) return;

    if (_apActive) stopAP();
    if (_scanInProgress) _scanInProgress = false;

    switch (mode) {
        case WIFI_MODE_STATION:
            WiFi.disconnect();
            DBG_PRINTF("[WiFiManager] Mod: ISTASYON\n");
            break;
        case WIFI_MODE_AP:
            DBG_PRINTF("[WiFiManager] Mod: AP (startAP() ile baslat)\n");
            break;
        case WIFI_MODE_PROMISCUOUS:
            startPromiscuous();
            DBG_PRINTF("[WiFiManager] Mod: PROMISCUOUS\n");
            break;
    }
    _currentMode = mode;
}

WiFiOpMode WiFiManager::getMode() {
    return _currentMode;
}

// RTL8720DN scanNetworks() senkrondur — hemen sonuc doner
bool WiFiManager::startScan() {
    if (_scanInProgress) {
        DBG_PRINTF("[WiFiManager] Tarama zaten devam ediyor.\n");
        return false;
    }

    _networkCount = 0;
    _scanInProgress = true;
    _scanStartTime = millis();

    DBG_PRINTF("[WiFiManager] Tarama baslatildi...\n");

    // RTL8720DN: scanNetworks() tek seferde sonucu doner (bloklama)
    int16_t n = WiFi.scanNetworks();
    if (n < 0) {
        DBG_PRINTF("[WiFiManager] Tarama basarisiz!\n");
        _scanInProgress = false;
        return false;
    }

    _networkCount = 0;
    for (int i = 0; i < n && _networkCount < MAX_SCAN_RESULTS; i++) {
        String ssidStr = WiFi.SSID(i);
        strncpy(_networks[_networkCount].ssid, ssidStr.c_str(),
                sizeof(_networks[_networkCount].ssid) - 1);
        _networks[_networkCount].ssid[sizeof(_networks[_networkCount].ssid) - 1] = '\0';

        // RTL8720DN: BSSID(uint8_t* bssid) cagrisina buffer verilir
        uint8_t bssidBuf[6];
        memcpy(_networks[_networkCount].bssid, WiFi.BSSID(bssidBuf), 6);
        _networks[_networkCount].rssi = WiFi.RSSI(i);

        // RTL8720DN'de kanal bilgisi dogrudan alinamaz, varsayilan 1
        _networks[_networkCount].channel = 1;
        _networks[_networkCount].is5GHz = false;
        _networkCount++;
    }

    _scanInProgress = false;
    DBG_PRINTF("[WiFiManager] Tarama tamam: %d ag bulundu.\n", _networkCount);
    return true;
}

bool WiFiManager::isScanComplete() {
    return !_scanInProgress;
}

int WiFiManager::getScanResults(NetworkInfo* buffer, int maxCount) {
    int count = 0;
    for (int i = 0; i < _networkCount && i < maxCount; i++) {
        buffer[i] = _networks[i];
        count++;
    }
    return count;
}

bool WiFiManager::startAP(const char* ssid, const char* password, uint8_t channel) {
    if (_currentMode != WIFI_MODE_AP) setMode(WIFI_MODE_AP);

    strncpy(_apSSID, ssid, sizeof(_apSSID) - 1);
    _apSSID[sizeof(_apSSID) - 1] = '\0';
    _apChannel = channel;

    char chStr[4];
    snprintf(chStr, sizeof(chStr), "%d", channel);

    bool success = false;
    if (password && strlen(password) >= 8) {
        success = (WiFi.apbegin((char*)ssid, (char*)password, chStr) == 0);
    } else {
        success = (WiFi.apbegin((char*)ssid, chStr) == 0);
    }

    if (success) {
        _apActive = true;
        DBG_PRINTF("[WiFiManager] AP baslatildi: SSID=%s\n", ssid);
    } else {
        DBG_PRINTF("[WiFiManager] AP baslatilamadi!\n");
    }
    return success;
}

void WiFiManager::stopAP() {
    if (_apActive) {
        // RTL8720DN: AP'yi durdurmak icin dogrudan API yok
        // WiFi.end() tumunu kapatir, apbegin() sonlandirma ayri olmayabilir
        WiFi.disconnect();
        _apActive = false;
        DBG_PRINTF("[WiFiManager] AP durduruldu.\n");
    }
}

IPAddress WiFiManager::getAPIP() {
    // RTL8720DN: AP IP'si icin localIP(1) dene
    return WiFi.localIP(1);
}

bool WiFiManager::startPromiscuous() {
    if (_promiscuousActive) return true;
    // RTL8720DN SDK: wifi_set_promisc(RTW_PROMISC_ENABLE_2, callback, len_used)
    // RTW_PROMISC_ENABLE_2 = tum 802.11 paketlerini al
    // callback=NULL ise paketleri almaz, sadece promiscuous modu acar
    // (WiFiAttacks kendi callback'ini ayarlayacak)
    int ret = wifi_set_promisc(RTW_PROMISC_ENABLE_2, NULL, 1);
    if (ret != 0) {
        DBG_PRINTF("[WiFiManager] Promiscuous mod baslatilamadi!\n");
        return false;
    }
    _promiscuousActive = true;
    DBG_PRINTF("[WiFiManager] Promiscuous mod etkin.\n");
    return true;
}

void WiFiManager::stopPromiscuous() {
    if (_promiscuousActive) {
        wifi_set_promisc(RTW_PROMISC_DISABLE, NULL, 0);
        _promiscuousActive = false;
        DBG_PRINTF("[WiFiManager] Promiscuous mod kapali.\n");
    }
}

void WiFiManager::setChannel(uint8_t channel) {
    // RTL8720DN SDK: wifi_set_channel()
    wifi_set_channel(channel);
    DBG_PRINTF("[WiFiManager] Kanal %d\n", channel);
}

bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void WiFiManager::printStatus(Print& output) {
    DBG_PRINTF_PRINT(output, "--- WiFiManager Durumu ---\n");
    DBG_PRINTF_PRINT(output, "Mod: %s\n",
        _currentMode == WIFI_MODE_STATION ? "ISTASYON" :
        _currentMode == WIFI_MODE_AP ? "AP" : "PROMISCUOUS");
    DBG_PRINTF_PRINT(output, "Baglantili: %s\n", isConnected() ? "Evet" : "Hayir");
    if (isConnected()) {
        IPAddress staIP = WiFi.localIP();
        char ipBuf[16];
        snprintf(ipBuf, sizeof(ipBuf), "%d.%d.%d.%d", staIP[0], staIP[1], staIP[2], staIP[3]);
        DBG_PRINTF_PRINT(output, "IP: %s\n", ipBuf);
    }
    DBG_PRINTF_PRINT(output, "AP Aktif: %s\n", _apActive ? "Evet" : "Hayir");
    DBG_PRINTF_PRINT(output, "Promiscuous: %s\n", _promiscuousActive ? "Evet" : "Hayir");
    DBG_PRINTF_PRINT(output, "--------------------------\n");
}
