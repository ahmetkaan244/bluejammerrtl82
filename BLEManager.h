#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

// Forward declaration for BLE library types
// RTL8720DN BLE library specifics will be wrapped here
// For Realtek Ameba Arduino SDK, we typically include specific BLE headers.
// We'll use a generic approach here and assume the necessary BLE objects
// will be defined or included by the main sketch or a platform-specific header.
// Example: #include <BLEDevice.h> // This would be specific to a library

// Advertisement types for BLE Spam
enum BLESpamType {
    BLE_SPAM_APPLE = 0,    // AirPods, AirPods Pro, AirPods Max
    BLE_SPAM_WINDOWS,       // Swift Pair
    BLE_SPAM_ANDROID,       // Fast Pair
    BLE_SPAM_ALL            // Cycle through all types
};

class BLEManager {
public:
    BLEManager();
    
    // Initialize BLE hardware
    bool begin();
    
    // Start/stop advertising (general)
    bool startAdvertising();
    bool stopAdvertising();
    
    // Configure advertisement for specific spam type
    bool setAppleAdvertisement();      // Apple AirPods-like advertisement
    bool setWindowsAdvertisement();     // Windows Swift Pair advertisement
    bool setAndroidAdvertisement();     // Android Fast Pair advertisement
    
    // Set advertisement interval
    void setAdvertisementInterval(uint16_t intervalMs);
    
    // Update advertisement data (for cycling spam types)
    // This function is for raw data updates, useful for dynamic payloads.
    bool updateAdvertisementData(const uint8_t* data, size_t length);
    
    // Status
    bool isAdvertising();
    bool isInitialized();
    
    // Debug
    void printStatus(Print& output);

private:
    bool _initialized;
    bool _advertising;
    uint16_t _advertisementInterval; // Reklam aralığı (milisaniye cinsinden)
    
    // BLE library objects (wrapped for compilation)
    // Actual RTL8720DN BLE objects will be stored here.
    // We'll use placeholder types and comments for the Realtek Ameba Arduino SDK.
    // Example:
    // BLEAdvertising* _pAdvertising;
    // BLECharacteristic* _pCharacteristic;
    // BLEService* _pService;

    // Internal buffer for advertisement data
    static const size_t MAX_ADV_DATA_LENGTH = 31; // BLE reklam verisi maksimum uzunluğu
    uint8_t _advertisementData[MAX_ADV_DATA_LENGTH];
    size_t _advertisementDataLength;
    
    // Internal helper to construct advertisement data payloads
    void _constructApplePayload(uint8_t* buffer, size_t* length);
    void _constructWindowsPayload(uint8_t* buffer, size_t* length);
    void _constructAndroidPayload(uint8_t* buffer, size_t* length);

    // Helper to set common advertisement parameters
    void _setCommonAdvertisementData(const char* deviceName, uint16_t companyId, const uint8_t* manufacturerData, size_t manufacturerDataLen);
};

#endif
