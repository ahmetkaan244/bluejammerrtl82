#ifndef WEB_PANEL_H
#define WEB_PANEL_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "WiFiManager.h"
#include "WiFiAttacks.h"
#include "BLESpam.h"
#include "CaptivePortal.h"
#include "ProbeSniffer.h"

// WebPanel icin kullanilacak port (CaptivePortal'in 80'inden farkli)
#define WEB_PANEL_PORT 8080
#define WEB_PANEL_SSID "BlueJammer Panel"
#define WEB_PANEL_PSK  ""
#define WEB_PANEL_CHANNEL 6

class WebPanel {
public:
    WebPanel();
    void begin(WiFiManager* wifiMgr, WiFiAttacks* attacks, BLESpam* ble, CaptivePortal* portal, ProbeSniffer* probe);
    bool start();
    void stop();
    bool isRunning();
    void update();    // Her loop'ta cagrilir, istemcileri yonetir

private:
    WiFiManager*   _wifi;
    WiFiAttacks*   _attacks;
    BLESpam*       _ble;
    CaptivePortal* _portal;
    ProbeSniffer*  _probe;
    WiFiServer*    _server;
    bool           _running;
    unsigned long  _lastClientCheck;
    char           _ipAddrStr[16]; // cached IP string for JSON status

    void _handleClient(WiFiClient client);
    void _sendHtml(WiFiClient& c);
    void _sendJsonStatus(WiFiClient& c);
    void _sendJsonCaptured(WiFiClient& c);
    void _sendJsonProbes(WiFiClient& c);
    void _sendJson(WiFiClient& c, const char* json);
    void _sendHeader(WiFiClient& c, const char* contentType);

    // HTML sayfasi (PROGMEM'de)
    static const char* PAGE_HTML;
};

#endif
