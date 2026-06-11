#include "ButtonHandler.h"

// ButtonHandler sınıfının yapıcı metodu
// Tüm düğme durumlarını varsayılan değerlerle başlatır.
ButtonHandler::ButtonHandler() {
    for (int i = 0; i < BTN_COUNT; i++) {
        _buttons[i].pin = -1; // Pin numarası henüz atanmadı
        _buttons[i].lastRawState = HIGH; // Son okunan ham durum (çekme direnci ile yüksek)
        _buttons[i].debouncedState = HIGH; // Debounce sonrası durum (çekme direnci ile yüksek)
        _buttons[i].lastChangeMs = 0; // Son durum değişikliği zamanı
        _buttons[i].pressStartMs = 0; // Basılma başlangıç zamanı
        _buttons[i].pendingEvent = BTN_EVENT_NONE; // Bekleyen olay yok
        _buttons[i].longPressTriggered = false; // Uzun basılma tetiklenmedi
    }
}

// Düğme pinlerini başlatır ve ilk durumlarını okur.
void ButtonHandler::begin() {
    _buttons[BTN_UP_ID].pin = _getPin(BTN_UP_ID);
    _buttons[BTN_DOWN_ID].pin = _getPin(BTN_DOWN_ID);
    _buttons[BTN_SELECT_ID].pin = _getPin(BTN_SELECT_ID);
    _buttons[BTN_BACK_ID].pin = _getPin(BTN_BACK_ID);

    for (int i = 0; i < BTN_COUNT; i++) {
        if (_buttons[i].pin != -1) {
            pinMode(_buttons[i].pin, INPUT_PULLUP); // Pinleri dahili çekme direnci ile giriş olarak ayarla
            _buttons[i].lastRawState = digitalRead(_buttons[i].pin); // İlk ham durumu oku
            _buttons[i].debouncedState = _buttons[i].lastRawState; // Debounce sonrası durumu da aynı yap
        }
    }
}

// Her loop iterasyonunda çağrılmalı. Düğme durumlarını günceller ve debounce işlemini yapar.
void ButtonHandler::update() {
    for (int i = 0; i < BTN_COUNT; i++) {
        _debounce((ButtonID)i);
    }
}

// Belirli bir düğme için debounce ve olay algılama mantığı.
void ButtonHandler::_debounce(ButtonID btn) {
    unsigned long currentMs = millis(); // Mevcut zamanı al
    uint8_t currentRawState = digitalRead(_buttons[btn].pin); // Düğmenin anlık ham durumunu oku

    // Ham durum değiştiyse, son değişiklik zamanını güncelle
    if (currentRawState != _buttons[btn].lastRawState) {
        _buttons[btn].lastChangeMs = currentMs;
        _buttons[btn].lastRawState = currentRawState;
    }

    // Debounce süresi geçtiyse ve durum hala stabilse, debouncedState'i güncelle
    if ((currentMs - _buttons[btn].lastChangeMs) > DEBOUNCE_DELAY_MS) {
        if (currentRawState != _buttons[btn].debouncedState) {
            _buttons[btn].debouncedState = currentRawState;

            // Düğme basıldı (HIGH'dan LOW'a geçiş)
            if (_buttons[btn].debouncedState == LOW) {
                _buttons[btn].pendingEvent = BTN_EVENT_PRESSED; // Basılma olayını beklemede olarak ayarla
                _buttons[btn].pressStartMs = currentMs; // Basılma başlangıç zamanını kaydet
                _buttons[btn].longPressTriggered = false; // Uzun basılma bayrağını sıfırla
            } 
            // Düğme bırakıldı (LOW'dan HIGH'a geçiş)
            else {
                _buttons[btn].pendingEvent = BTN_EVENT_RELEASED; // Bırakılma olayını beklemede olarak ayarla
                _buttons[btn].longPressTriggered = false; // Uzun basılma bayrağını sıfırla
            }
        }
    }

    // Uzun basılma algılama
    // Düğme basılıysa (LOW), uzun basılma süresi geçtiyse ve daha önce tetiklenmediyse
    if (_buttons[btn].debouncedState == LOW && 
        (currentMs - _buttons[btn].pressStartMs) > LONG_PRESS_MS && 
        !_buttons[btn].longPressTriggered) {
        
        _buttons[btn].pendingEvent = BTN_EVENT_LONG_PRESS; // Uzun basılma olayını beklemede olarak ayarla
        _buttons[btn].longPressTriggered = true; // Uzun basılma tetiklendi olarak işaretle
    }
}

// Belirli bir düğme için bekleyen olayı döndürür ve olayı temizler.
ButtonEvent ButtonHandler::getEvent(ButtonID btn) {
    ButtonEvent event = _buttons[btn].pendingEvent;
    _buttons[btn].pendingEvent = BTN_EVENT_NONE; // Olayı temizle
    return event;
}

// Belirli bir düğmenin şu anda basılı olup olmadığını kontrol eder (debounce sonrası).
bool ButtonHandler::isPressed(ButtonID btn) {
    return _buttons[btn].debouncedState == LOW;
}

// Belirli bir düğmenin ham durumunu döndürür (debounce öncesi - hata ayıklama için).
bool ButtonHandler::getRawState(ButtonID btn) {
    return _buttons[btn].lastRawState;
}

// ButtonID'yi gerçek pin numarasına eşler.
int ButtonHandler::_getPin(ButtonID btn) {
    switch (btn) {
        case BTN_UP_ID:
            return BTN_UP;
        case BTN_DOWN_ID:
            return BTN_DOWN;
        case BTN_SELECT_ID:
            return BTN_SELECT;
        case BTN_BACK_ID:
            return BTN_BACK;
        default:
            return -1; // Geçersiz ButtonID
    }
}
