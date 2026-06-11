#ifndef PROBE_SNIFFER_H
#define PROBE_SNIFFER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Maximum length of an SSID
#define MAX_SSID_LEN 32

// Structure to hold captured probe request data
struct ProbeRecord {
    uint8_t mac[6];              // Source MAC of probing device
    char ssid[MAX_SSID_LEN + 1]; // SSID being probed (empty if broadcast probe)
    int8_t rssi;                 // Signal strength
    unsigned long timestamp;     // When captured (millis)
    uint8_t channel;             // Channel it was captured on
};

class ProbeSniffer {
public:
    ProbeSniffer();

    // Initialize
    void begin();
    
    // Start/stop sniffer
    bool start();
    void stop();
    bool isRunning();
    
    // Must be called every loop()
    void update();
    
    // Get captured records
    int getProbeCount();
    const ProbeRecord* getProbe(int index);
    
    // Clear all records
    void clearProbes();
    
    // Channel hopping for better coverage
    void setChannelHopInterval(uint16_t ms);
    
    // Get total packets captured since start
    uint32_t getTotalCaptured();

private:
    bool _running;
    ProbeRecord _probes[PROBE_LIST_MAX];
    int _probeCount;
    int _probeIndex;       // For circular buffer: next position to write
    uint32_t _totalCaptured;
    
    // Channel hopping
    uint8_t _currentChannel;
    unsigned long _lastChannelHopMs;
    uint16_t _channelHopInterval;
    
    // Packet reception callback (RTL8720DN SDK callback imzasi)
    static void _promiscuousCallback(unsigned char* buf, unsigned int len, void* userdata);
    
    // Parse a probe request from raw packet data
    bool _parseProbeRequest(const uint8_t* frame, uint16_t len, int8_t rssi, uint8_t channel);
    
    // Store a probe record (circular buffer)
    void _storeProbe(const uint8_t* mac, const char* ssid, int8_t rssi, uint8_t channel);
    
    // Check if MAC already has a recent entry
    bool _isDuplicate(const uint8_t* mac);
    
    // Channel list for hopping
    static const uint8_t CHANNELS[];
    static const uint8_t CHANNEL_COUNT;

    // Static instance pointer for the callback
    static ProbeSniffer* _instance;
};

#endif
