#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Display buffer size for SSD1306
#define SSD1306_BUFFER_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)

class DisplayManager {
public:
    DisplayManager();
    
    // Initialize the OLED display over I2C
    bool begin();
    
    // Clear the display buffer
    void clear();
    
    // Write buffer to display (call after all draw commands)
    void display();
    
    // Text drawing helpers
    void setTextSize(uint8_t size);
    void setTextColor(uint16_t color);
    void setCursor(int16_t x, int16_t y);
    void print(const char* text);
    void printf(const char* format, ...);
    void println(const char* text);
    
    // Drawing primitives (delegated to Adafruit_GFX)
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color);
    
    // Scrolling text helper (for probe sniffer display)
    void setScrollPosition(uint8_t line);
    void scrollUp();
    void scrollDown();
    
    // Boot splash screen
    void showBootScreen();
    
    // Display status info at bottom of screen (for menu)
    void drawStatusBar(const char* leftText, const char* rightText);
    
    // Screen dimensions
    int width() { return OLED_WIDTH; }
    int height() { return OLED_HEIGHT; }
    
    // Invert display temporarily (visual feedback)
    void flashDisplay(uint16_t durationMs);

private:
    Adafruit_SSD1306 _display;
    uint8_t _textSize;
    uint16_t _textColor;
    int16_t _cursorX;
    int16_t _cursorY;
    uint8_t _scrollLine;
};

#endif