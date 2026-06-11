#include "DisplayManager.h"
#include "debug.h"
#include <stdarg.h>
#include <string.h>

// 64x32 1-bit BMP - "BJRTL" logotype
static const uint8_t _bootLogoBitmap[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7E, 0x00, 0x7E, 0x00, 0xFE, 0x00, 0x7C, 0x00,
    0x42, 0x00, 0x42, 0x00, 0x82, 0x00, 0x82, 0x00,
    0x42, 0x00, 0x42, 0x00, 0x82, 0x00, 0x82, 0x00,
    0x42, 0x00, 0x42, 0x00, 0x82, 0x00, 0x82, 0x00,
    0x7E, 0x00, 0x3C, 0x00, 0xFE, 0x00, 0x7C, 0x00,
    0x42, 0x00, 0x00, 0x00, 0x82, 0x00, 0x44, 0x00,
    0x42, 0x00, 0x00, 0x00, 0x82, 0x00, 0x28, 0x00,
    0x42, 0x00, 0x00, 0x00, 0x82, 0x00, 0x10, 0x00,
    0x42, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// --- Yapıcı Metot (Constructor) ---
DisplayManager::DisplayManager()
    : _display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1),
      _textSize(1),
      _textColor(SSD1306_WHITE),
      _cursorX(0),
      _cursorY(0) {
    // Wire (I2C) başlatma pinleri daha sonra begin()'de yapılacak
}

// --- Başlatma (Initialization) ---
bool DisplayManager::begin() {
    // I2C başlatma: OLED_SDA ve OLED_SCL pinlerini kullan
    Wire.begin(); // RTL8720DN: Wire zaten dogru pinlerde (I2C_SDA, I2C_SCL)

    // SSD1306 ekranı başlat
    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {   // periphBegin=false: Wire.begin() zaten cagrildi, tekrar cagirma
        DBG_PRINTF("HATA: OLED ekran bulunamadi! I2C adresini kontrol edin.\n");
        return false;
    }

    _display.clearDisplay();
    _display.setTextSize(_textSize);
    _display.setTextColor(_textColor);
    _display.setCursor(0, 0);
    _display.display();

    DBG_PRINTF("OLED ekran baslatildi.\n");
    return true;
}

// --- Temel Çizim İşlevleri (Delegated to Adafruit) ---

void DisplayManager::clear() {
    _display.clearDisplay();
}

void DisplayManager::display() {
    _display.display();
}

void DisplayManager::setTextSize(uint8_t size) {
    _textSize = size;
    _display.setTextSize(size);
}

void DisplayManager::setTextColor(uint16_t color) {
    _textColor = color;
    _display.setTextColor(color);
}

void DisplayManager::setCursor(int16_t x, int16_t y) {
    _cursorX = x;
    _cursorY = y;
    _display.setCursor(x, y);
}

void DisplayManager::print(const char* text) {
    _display.print(text);
}

void DisplayManager::println(const char* text) {
    _display.println(text);
}

void DisplayManager::printf(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    _display.print(buffer);
}

// --- Çizim Temelleri (Drawing Primitives) ---

void DisplayManager::drawPixel(int16_t x, int16_t y, uint16_t color) {
    _display.drawPixel(x, y, color);
}

void DisplayManager::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    _display.drawLine(x0, y0, x1, y1, color);
}

void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _display.drawRect(x, y, w, h, color);
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _display.fillRect(x, y, w, h, color);
}

void DisplayManager::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color) {
    _display.drawBitmap(x, y, bitmap, w, h, color);
}

void DisplayManager::setScrollPosition(uint8_t line) { _scrollLine = line; }
void DisplayManager::scrollUp() { if (_scrollLine < 255) _scrollLine++; }
void DisplayManager::scrollDown() { if (_scrollLine > 0) _scrollLine--; }

// --- Başlangıç Ekranı (Boot Splash) ---
void DisplayManager::showBootScreen() {
    _display.clearDisplay();

    // Logo bitmap'ini ortala
    int logoX = (OLED_WIDTH - 64) / 2;
    int logoY = -5; // Üst kısma yakın
    _display.drawBitmap(logoX, logoY, _bootLogoBitmap, 64, 32, SSD1306_WHITE);

    // "BlueJammer" yazısı
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor((OLED_WIDTH - (9 * 6)) / 2, 30); // 9 karakter * 6px
    _display.print(F("BlueJammer"));

    // "RTL82" yazısı
    _display.setTextSize(2);
    _display.setCursor((OLED_WIDTH - (5 * 12)) / 2, 42); // 5 karakter * 12px
    _display.print(F("RTL82"));

    // Versiyon
    _display.setTextSize(1);
    _display.setCursor(OLED_WIDTH - 30, OLED_HEIGHT - 10);
    _display.print(F("v1.0"));

    _display.display();
}

// --- Durum Çubuğu (Status Bar) ---
void DisplayManager::drawStatusBar(const char* leftText, const char* rightText) {
    // Alt kısımda bir ayraç çizgisi
    _display.drawLine(0, OLED_HEIGHT - 9, OLED_WIDTH - 1, OLED_HEIGHT - 9, SSD1306_WHITE);

    // Sol metin
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, OLED_HEIGHT - 8);
    _display.print(leftText);

    // Sağ metin (hizalamak için sağdan sola)
    int rightLen = strlen(rightText);
    _display.setCursor(OLED_WIDTH - (rightLen * 6), OLED_HEIGHT - 8);
    _display.print(rightText);
}

void DisplayManager::flashDisplay(uint16_t durationMs) {
    _display.invertDisplay(true);
    unsigned long startMs = millis();
    while (millis() - startMs < durationMs) { }
    _display.invertDisplay(false);
}

