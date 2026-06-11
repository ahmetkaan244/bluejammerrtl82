#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "ButtonHandler.h"
#include "DisplayManager.h"

// Menu item types
enum MenuItemType {
    MENU_ITEM_ACTION = 0,    // Execute an action (start attack, etc.)
    MENU_ITEM_SUBMENU,       // Navigate to submenu
    MENU_ITEM_TOGGLE,        // Toggle setting on/off
    MENU_ITEM_BACK           // Go back to parent menu
};

// Forward declaration
struct MenuItem;
struct MenuScreen;

// Callback for action items - return true if action was handled
typedef bool (*MenuActionCallback)(void* context);

struct MenuItem {
    const char* label;              // Display text (max 20 chars)
    MenuItemType type;              // What happens when selected
    MenuScreen* submenu;            // For MENU_ITEM_SUBMENU
    MenuActionCallback action;      // For MENU_ITEM_ACTION
    void* context;                  // Context for callback
    bool* toggleValue;              // For MENU_ITEM_TOGGLE
};

struct MenuScreen {
    const char* title;              // Screen title (shown at top)
    MenuItem* items;                // Array of menu items
    uint8_t itemCount;              // Number of items
    MenuScreen* parent;             // Parent screen (for BACK)
};

class MenuManager {
public:
    MenuManager();
    
    // Initialize with references to hardware modules
    void begin(ButtonHandler* btnHandler, DisplayManager* dispMgr);
    
    // Must be called every loop() - handles input and rendering
    void update();
    
    // Navigate to a specific screen
    void setScreen(MenuScreen* screen);
    
    // Get current screen
    MenuScreen* getCurrentScreen();
    
    // Get selected item index
    int8_t getSelectedIndex();
    
    // Check if an attack/action is currently running
    bool isActionRunning();
    void setActionRunning(bool running);
    
    // Show a temporary message on screen (for status updates)
    void showMessage(const char* msg, uint16_t durationMs);
    
    // Force redraw on next update
    void requestRedraw();

private:
    ButtonHandler* _btn;
    DisplayManager* _disp;
    
    MenuScreen* _currentScreen;
    int8_t _selectedIndex;
    int8_t _scrollOffset;       // For scrolling if items > visible area
    bool _actionRunning;
    bool _needsRedraw;
    
    // Message overlay state
    bool _showingMessage;
    char _message[64];
    unsigned long _messageStartMs;
    uint16_t _messageDurationMs;
    
    // Button repeat delay (hold to scroll fast)
    unsigned long _lastButtonRepeatMs;
    uint8_t _repeatButtonId;
    bool _buttonWasPressed;
    
    // Internal methods
    void _drawMenu();
    void _handleInput();
    void _executeSelected();
    void _navigateTo(MenuScreen* screen);
    void _navigateBack();
    void _scrollTo(int8_t index);
    
    // Built-in menu screens
    static MenuScreen _mainMenu;
    // Submenus will be defined externally or added via methods
    
    // Maximum visible items on screen
    static const uint8_t MAX_VISIBLE_ITEMS = 4;
};

#endif
