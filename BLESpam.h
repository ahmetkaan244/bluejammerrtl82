#ifndef BLE_SPAM_H
#define BLE_SPAM_H

#include <Arduino.h>
#include "config.h"

// Variants within each platform type
enum AppleVariant {
    APPLE_AIRPODS_1 = 0,
    APPLE_AIRPODS_2,
    APPLE_AIRPODS_PRO,
    APPLE_AIRPODS_MAX,
    APPLE_AIRPODS_COUNT
};

enum WindowsVariant {
    WINDOWS_SURFACE_EARBUDS = 0,
    WINDOWS_SURFACE_HEADPHONES,
    WINDOWS_COUNT
};

enum AndroidVariant {
    ANDROID_PIXEL_BUDS = 0,
    ANDROID_PIXEL_BUDS_PRO,
    ANDROID_COUNT
};

class BLESpam {
public:
    BLESpam();

    // Initialize (set up BLE hardware via BLEManager or direct)
    void begin();

    // Start/stop spam
    bool start();
    void stop();
    bool isRunning();

    // Set spam type (APPLE, WINDOWS, ANDROID, or ALL to cycle)
    void setSpamType(uint8_t type); // 0=Apple, 1=Windows, 2=Android, 3=ALL

    // Must be called every loop()
    void update();

    // Set packet rate (interval in ms between advertisements)
    void setInterval(uint16_t intervalMs);

    // Statistics
    uint32_t getPacketsSent();
    uint16_t getPacketsPerSecond();

private:
    bool _running;
    uint8_t _spamType;      // Current spam mode
    uint16_t _intervalMs;   // ms between advertisement bursts (default from config.h)
    
    // Timing
    unsigned long _lastAdvertMs;
    
    // Cycling state
    uint8_t _currentVariant;
    unsigned long _variantChangeMs;
    uint16_t _variantDurationMs;  // How long to stay on one variant (3 seconds)
    
    // Statistics
    uint32_t _totalPackets;
    uint16_t _currentPPS;
    unsigned long _ppsCountStartMs;
    uint32_t _ppsPacketCount;
    
    // Advertisement payload construction
    void _buildApplePayload(uint8_t* buffer, uint8_t* length, AppleVariant variant);
    void _buildWindowsPayload(uint8_t* buffer, uint8_t* length, WindowsVariant variant);
    void _buildAndroidPayload(uint8_t* buffer, uint8_t* length, AndroidVariant variant);
    
    // Generic BLE AD structure helpers
    void _addAD(uint8_t* buffer, uint8_t* offset, uint8_t adType, const uint8_t* data, uint8_t dataLen);
    void _addFlagsAD(uint8_t* buffer, uint8_t* offset);
    void _addNameAD(uint8_t* buffer, uint8_t* offset, const char* name);
    void _addManufacturerAD(uint8_t* buffer, uint8_t* offset, uint16_t companyId, const uint8_t* data, uint8_t dataLen);
    
    // Send a raw BLE advertisement
    void _sendAdvertisement(const uint8_t* data, uint8_t length);
    
    // Variant names
    const char* _getVariantName();
};

#endif
