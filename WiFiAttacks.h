#ifndef WIFI_ATTACKS_H
#define WIFI_ATTACKS_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "WiFiManager.h"

// Maksimum ham 802.11 çerçevesinin boyutu (beacon'lar icin genis)
#define MAX_FRAME_SIZE 256

// Beacon Flood icin SSID havuzu
#define BEACON_SSID_COUNT 30

class WiFiAttacks {
public:
    WiFiAttacks();

    // Başlatma (geri arama referanslarını ayarlar)
    void begin(WiFiManager* wifiMgr);

    // === TEKİL DEAUTH ===
    bool startSingleDeauth(uint8_t* targetBSSID, uint8_t channel);
    void stopSingleDeauth();
    bool isSingleDeauthRunning();

    // === TÜMÜNÜ DEAUTH ET ===
    bool startDeauthAll();
    void stopDeauthAll();
    bool isDeauthAllRunning();

    // === YÖNLENDİRİCİYE DDoS (AUTH FLOOD) ===
    bool startAuthFlood(uint8_t* targetBSSID, uint8_t channel);
    void stopAuthFlood();
    bool isAuthFloodRunning();

    // === BEACON FLOOD ===
    bool startBeaconFlood();
    void stopBeaconFlood();
    bool isBeaconFloodRunning();
    int getBeaconSSIDIndex(); // Su anki SSID indeksini dondurur (0-29)

    // === GENEL ===
    void stopAll();          // Tüm aktif saldırıları durdur
    bool isAnyRunning();     // Herhangi bir saldırının aktif olup olmadığını kontrol et
    void update();           // Her loop() çağrısında çalışır - paketleri zamanlamaya göre gönderir
    void setChannel(uint8_t channel);

private:
    WiFiManager* _wifi;
    
    // Saldırı durumu bayrakları
    bool _singleDeauthActive;
    bool _deauthAllActive;
    bool _authFloodActive;
    bool _beaconFloodActive;
    
    // Hedef bilgisi
    uint8_t _targetBSSID[6];
    uint8_t _targetChannel;
    
    // Zamanlama (engellemeyen paket zamanlaması)
    unsigned long _lastPacketMs;
    unsigned long _channelHopMs;  // Tüm deauth için - kanal atlama
    uint8_t _currentChannel;
    
    // Beacon flood durumu
    uint8_t _beaconSSIDIndex;    // Su anki SSID sirasi
    uint8_t _beaconMAC[3][6];    // 3 adet rastgele MAC (kaynak, BSSID, varsayilan)
    unsigned long _beaconSSIDChangeMs; // SSID degisim zamanlayicisi
    
    // Paket oluşturma yardımcıları
    uint16_t _craftDeauthFrame(uint8_t* buffer, const uint8_t* bssid, const uint8_t* stationMAC);
    uint16_t _craftAuthFrame(uint8_t* buffer, const uint8_t* bssid, const uint8_t* stationMAC);
    uint16_t _craftBeaconFrame(uint8_t* buffer, const char* ssid, uint8_t channel, const uint8_t* mac);
    void _sendFrame(const uint8_t* frame, uint16_t len);
    
    // İstatistikler
    uint32_t _packetsSent;
    uint16_t _sequenceNumber; // 802.11 çerçeveleri için sıra numarası
};

#endif