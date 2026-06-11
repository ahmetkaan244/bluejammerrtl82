#include "ProbeSniffer.h"
#include "debug.h"

extern "C" {
    #include <wifi_conf.h>
    #include <wifi_util.h>
}

ProbeSniffer* ProbeSniffer::_instance = NULL;

const uint8_t ProbeSniffer::CHANNELS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const uint8_t ProbeSniffer::CHANNEL_COUNT = 13;

ProbeSniffer::ProbeSniffer()
    : _running(false)
    , _probeCount(0)
    , _probeIndex(0)
    , _totalCaptured(0)
    , _currentChannel(1)
    , _lastChannelHopMs(0)
    , _channelHopInterval(500)
{
    memset(_probes, 0, sizeof(_probes));
}

void ProbeSniffer::begin() {
    _instance = this;
    DBG_PRINTF("[ProbeSniffer] Baslatildi.\n");
}

bool ProbeSniffer::start() {
    if (_running) return true;

    _currentChannel = 1;
    _lastChannelHopMs = millis();
    _probeCount = 0;
    _probeIndex = 0;
    _totalCaptured = 0;

    wifi_set_promisc(RTW_PROMISC_ENABLE_2, _promiscuousCallback, 1);
    wifi_set_channel(_currentChannel);

    _running = true;
    DBG_PRINTF("[ProbeSniffer] Dinleme basladi, kanal %d.\n", _currentChannel);
    return true;
}

void ProbeSniffer::stop() {
    if (!_running) return;
    _running = false;
    wifi_set_promisc(RTW_PROMISC_DISABLE, NULL, 0);
    DBG_PRINTF("[ProbeSniffer] Dinleme durduruldu.\n");
}

bool ProbeSniffer::isRunning() {
    return _running;
}

void ProbeSniffer::update() {
    if (!_running) return;

    unsigned long now = millis();
    if (now - _lastChannelHopMs >= _channelHopInterval) {
        _currentChannel++;
        if (_currentChannel > CHANNEL_COUNT) {
            _currentChannel = 1;
        }
        wifi_set_channel(CHANNELS[_currentChannel - 1]);
        _lastChannelHopMs = now;
    }
}

int ProbeSniffer::getProbeCount() {
    return _probeCount;
}

const ProbeRecord* ProbeSniffer::getProbe(int index) {
    if (index < 0 || index >= _probeCount) return NULL;
    return &_probes[index];
}

void ProbeSniffer::clearProbes() {
    _probeCount = 0;
    _probeIndex = 0;
    memset(_probes, 0, sizeof(_probes));
}

void ProbeSniffer::setChannelHopInterval(uint16_t ms) {
    _channelHopInterval = ms;
}

uint32_t ProbeSniffer::getTotalCaptured() {
    return _totalCaptured;
}

void ProbeSniffer::_promiscuousCallback(unsigned char* buf, unsigned int len, void* userdata) {
    if (!_instance) return;
    if (!buf || len < 24) return;

    uint8_t frameControl0 = buf[0];
    uint8_t frameControl1 = buf[1];

    uint8_t type = frameControl1 & 0x0F;
    uint8_t subtype = (frameControl0 >> 4) & 0x0F;

    // Probe Request: type=0 (management), subtype=4
    if (type == 0 && subtype == 4) {
        uint8_t* srcMac = buf + 10;
        int8_t rssi = 0;
        uint8_t channel = _instance->_currentChannel;

        _instance->_parseProbeRequest(buf, len, rssi, channel);
    }
}

bool ProbeSniffer::_parseProbeRequest(const uint8_t* frame, uint16_t len, int8_t rssi, uint8_t channel) {
    if (!frame || len < 24) return false;

    uint8_t* srcMac = (uint8_t*)frame + 10;
    uint16_t pos = 24;

    char ssidBuf[MAX_SSID_LEN + 1] = {0};
    bool hasSSID = false;

    while (pos + 2 <= len) {
        uint8_t tag = frame[pos];
        uint8_t tagLen = frame[pos + 1];

        if (pos + 2 + tagLen > len) break;

        if (tag == 0x00 && tagLen > 0 && tagLen <= MAX_SSID_LEN) {
            memcpy(ssidBuf, frame + pos + 2, tagLen);
            ssidBuf[tagLen] = '\0';
            hasSSID = true;
            break;
        }

        pos += 2 + tagLen;
    }

    if (_isDuplicate(srcMac)) return false;

    _storeProbe(srcMac, hasSSID ? ssidBuf : "", rssi, channel);
    return true;
}

void ProbeSniffer::_storeProbe(const uint8_t* mac, const char* ssid, int8_t rssi, uint8_t channel) {
    int idx = _probeIndex;

    memcpy(_probes[idx].mac, mac, 6);
    strncpy(_probes[idx].ssid, ssid, MAX_SSID_LEN);
    _probes[idx].ssid[MAX_SSID_LEN] = '\0';
    _probes[idx].rssi = rssi;
    _probes[idx].timestamp = millis();
    _probes[idx].channel = channel;

    _probeIndex = (_probeIndex + 1) % PROBE_LIST_MAX;
    if (_probeCount < PROBE_LIST_MAX) {
        _probeCount++;
    }
    _totalCaptured++;
}

bool ProbeSniffer::_isDuplicate(const uint8_t* mac) {
    unsigned long now = millis();
    for (int i = 0; i < _probeCount; i++) {
        if (memcmp(_probes[i].mac, mac, 6) == 0) {
            if (now - _probes[i].timestamp < 10000) {
                return true;
            }
        }
    }
    return false;
}
