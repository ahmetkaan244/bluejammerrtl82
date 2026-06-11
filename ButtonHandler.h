#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "config.h"

// Button identifiers
enum ButtonID {
    BTN_UP_ID = 0,
    BTN_DOWN_ID,
    BTN_SELECT_ID,
    BTN_BACK_ID,
    BTN_COUNT  // Always keep last
};

// Button event types
enum ButtonEvent {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_PRESSED,    // Button just pressed (after debounce)
    BTN_EVENT_RELEASED,   // Button just released
    BTN_EVENT_LONG_PRESS  // Button held for LONG_PRESS_MS
};

class ButtonHandler {
public:
    ButtonHandler();
    
    // Initialize button pins (INPUT_PULLUP)
    void begin();
    
    // MUST be called every loop iteration - handles debounce and state machine
    void update();
    
    // Get the last event for a button (calling this clears the event)
    ButtonEvent getEvent(ButtonID btn);
    
    // Check if a button is currently physically pressed (after debounce)
    bool isPressed(ButtonID btn);
    
    // Get raw button state (before debounce - for debugging)
    bool getRawState(ButtonID btn);

private:
    struct ButtonState {
        int pin;                    // GPIO pin number
        uint8_t lastRawState;       // Last raw reading (HIGH/LOW)
        uint8_t debouncedState;     // Current debounced state
        unsigned long lastChangeMs;  // Last state change time
        unsigned long pressStartMs;  // When press started (for long press detection)
        ButtonEvent pendingEvent;    // Event waiting to be consumed
        bool longPressTriggered;     // Whether long press was already triggered
    };
    
    ButtonState _buttons[BTN_COUNT];
    
    // Debounce logic for a single button
    void _debounce(ButtonID btn);
    
    // Map ButtonID to pin number
    int _getPin(ButtonID btn);
};

#endif
