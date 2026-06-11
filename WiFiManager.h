#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

#define MAX_SCAN_RESULTS 30

enum WiFiOpMode {
    WIFI_MODE_STATION = 0,
    WIFI_MODE_AP,
    WIFI_MODE_PROMISCUOUS
};

struct NetworkInfo {
    char ssid[33];
    uint8_t bssid[6];
    int32_t rssi;
    uint8_t channel;
    bool is5GHz;
};

class WiFiManager {
public:
    WiFiManager();
    void begin();
    void setMode(WiFiOpMode mode);
    WiFiOpMode getMode();

    // Senkron tarama (RTL8720DN scanNetworks() senkrondur)
    bool startScan();
    bool isScanComplete();
    int getScanResults(NetworkInfo* buffer, int maxCount);

    // AP mode (RTL8720DN: apbegin() kullanir)
    bool startAP(const char* ssid, const char* password = NULL, uint8_t channel = 1);
    void stopAP();
    IPAddress getAPIP();

    // Promiscuous (RTL8720DN'de dogrudan Arduino API'si yok)
    bool startPromiscuous();
    void stopPromiscuous();

    // Kanal (RTL8720DN'de dogrudan API yok, isaretleyici olarak)
    void setChannel(uint8_t channel);

    bool isConnected();
    void printStatus(Print& output);

private:
    WiFiOpMode _currentMode;
    NetworkInfo _networks[MAX_SCAN_RESULTS];
    int _networkCount;
    unsigned long _scanStartTime;
    bool _scanInProgress;
    bool _promiscuousActive;
    bool _apActive;
    char _apSSID[33];
    uint8_t _apChannel;
};

#endif
