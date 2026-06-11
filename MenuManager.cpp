#include "MenuManager.h"
#include <string.h>
#include "WiFiAttacks.h"
#include "BLESpam.h"
#include "ProbeSniffer.h"
#include "CaptivePortal.h"
#include "WebPanel.h"

// Global nesneler (bluejammerrtl82.ino icinde tanimli)
extern WebPanel webPanel;
extern WiFiManager wiFiManager;
extern WiFiAttacks wiFiAttacks;
extern BLESpam bleSpam;
extern ProbeSniffer probeSniffer;
extern CaptivePortal captivePortal;

// ============================================================
// GERCEK CALLBACK'LER
// ============================================================

// Hedef Tara - Wi-Fi taramasi baslat
static bool _cb_scan(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    bool started = wiFiManager.startScan();
    if (started) {
        mm->showMessage("Taranıyor...", 2000);
    } else {
        mm->showMessage("Tarama basarisiz!", 1500);
    }
    return false; // Scan doesn't keep "RUNNING" state
}

// Single Deauth - son taranan hedefe deauth gonder
static bool _cb_singleDeauth(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    NetworkInfo targets[1];
    if (wiFiManager.getScanResults(targets, 1) > 0) {
        bool started = wiFiAttacks.startSingleDeauth(targets[0].bssid, targets[0].channel);
        if (started) {
            mm->showMessage("Deauth basladi!", 1000);
            return true;
        }
    }
    mm->showMessage("Once hedef tara!", 1500);
    return false;
}

// Deauth All - tum aglara deauth baslat
static bool _cb_deauthAll(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    bool started = wiFiAttacks.startDeauthAll();
    if (started) {
        mm->showMessage("Deauth All basladi!", 1000);
        return true;
    }
    mm->showMessage("Deauth All basarisiz!", 1500);
    return false;
}

// Evil Twin - hedef agin klonunu olustur
static bool _cb_evilTwin(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    NetworkInfo targets[1];
    if (wiFiManager.getScanResults(targets, 1) > 0) {
        bool started = captivePortal.start(targets[0].ssid, targets[0].bssid, targets[0].channel);
        if (started) {
            mm->showMessage("Evil Twin aktif!", 1000);
            return true;
        }
    }
    mm->showMessage("Once hedef tara!", 1500);
    return false;
}

// DDoS Router - hedefe auth flood baslat
static bool _cb_ddos(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    NetworkInfo targets[1];
    if (wiFiManager.getScanResults(targets, 1) > 0) {
        bool started = wiFiAttacks.startAuthFlood(targets[0].bssid, targets[0].channel);
        if (started) {
            mm->showMessage("DDoS basladi!", 1000);
            return true;
        }
    }
    mm->showMessage("Once hedef tara!", 1500);
    return false;
}

// BLE Spam - Bluetooth reklam flood baslat
static bool _cb_bleSpam(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    bleSpam.setSpamType(3); // ALL - butun turleri dongule
    bool started = bleSpam.start();
    if (started) {
        mm->showMessage("BLE Spam basladi!", 1000);
        return true;
    }
    mm->showMessage("BLE Spam basarisiz!", 1500);
    return false;
}

// Probe Sniffer - prob paketlerini dinle
static bool _cb_probeSniff(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    bool started = probeSniffer.start();
    if (started) {
        mm->showMessage("Probe dinleniyor...", 1000);
        return true;
    }
    mm->showMessage("Probe basarisiz!", 1500);
    return false;
}

// Beacon Flood - sanal AP yayini
static bool _cb_beaconFlood(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    bool started = wiFiAttacks.startBeaconFlood();
    if (started) {
        mm->showMessage("Beacon Flood basladi!", 1000);
        return true;
    }
    mm->showMessage("Beacon basarisiz!", 1500);
    return false;
}

// Web Panel - tarayicidan kontrol paneli baslat
static bool _cb_webPanel(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    bool started = webPanel.start();
    if (started) {
        mm->showMessage("Web Panel:8080", 2000);
        return true;
    }
    mm->showMessage("Panel basarisiz!", 1500);
    return false;
}

// Hakkimda - bilgi ekrani
static bool _cb_about(void* ctx) {
    MenuManager* mm = (MenuManager*)ctx;
    mm->showMessage("BlueJammerRTL82 v1.0", 2000);
    return false;
}

// ============================================================
// MENU YAPISI TANIMLARI (forward declarations)
// ============================================================
extern MenuScreen _menuWifiAttacks;
extern MenuScreen _menuBluetooth;
extern MenuScreen _menuProbeSniffer;
extern MenuScreen _menuSettings;

extern MenuItem _itemsWifiAttacks[];
extern MenuItem _itemsBluetooth[];
extern MenuItem _itemsProbeSniffer[];
extern MenuItem _itemsSettings[];
extern MenuItem _itemsMain[];

// ============================================================
// MAIN MENU (Ana Menu)
// ============================================================
MenuItem _itemsMain[] = {
    { "Wi-Fi Saldirilari",  MENU_ITEM_SUBMENU, &_menuWifiAttacks, NULL, NULL, NULL },
    { "Bluetooth",          MENU_ITEM_SUBMENU, &_menuBluetooth,   NULL, NULL, NULL },
    { "Probe Sniffer",      MENU_ITEM_SUBMENU, &_menuProbeSniffer,NULL, NULL, NULL },
    { "Web Panel",          MENU_ITEM_ACTION,  NULL, _cb_webPanel, NULL, NULL },
    { "Ayarlar",            MENU_ITEM_SUBMENU, &_menuSettings,    NULL, NULL, NULL },
};
MenuScreen _menuMain = { "BlueJammerRTL82", _itemsMain, 5, NULL };

// ============================================================
// WI-FI ATTACKS SUBMENU
// ============================================================
MenuItem _itemsWifiAttacks[] = {
    { "Hedef Tara",         MENU_ITEM_ACTION,  NULL, _cb_scan,         NULL, NULL },
    { "Single Deauth",      MENU_ITEM_ACTION,  NULL, _cb_singleDeauth, NULL, NULL },
    { "Deauth All",         MENU_ITEM_ACTION,  NULL, _cb_deauthAll,    NULL, NULL },
    { "Evil Twin",          MENU_ITEM_ACTION,  NULL, _cb_evilTwin,     NULL, NULL },
    { "DDoS Router",        MENU_ITEM_ACTION,  NULL, _cb_ddos,         NULL, NULL },
    { "Beacon Flood",       MENU_ITEM_ACTION,  NULL, _cb_beaconFlood,  NULL, NULL },
    { "< Geri",             MENU_ITEM_BACK,    NULL, NULL,             NULL, NULL },
};
MenuScreen _menuWifiAttacks = { "Wi-Fi Saldirilari", _itemsWifiAttacks, 7, &_menuMain };

// ============================================================
// BLUETOOTH SUBMENU
// ============================================================
MenuItem _itemsBluetooth[] = {
    { "BLE Spam",           MENU_ITEM_ACTION,  NULL, _cb_bleSpam,   NULL, NULL },
    { "< Geri",             MENU_ITEM_BACK,    NULL, NULL,          NULL, NULL },
};
MenuScreen _menuBluetooth = { "Bluetooth", _itemsBluetooth, 2, &_menuMain };

// ============================================================
// PROBE SNIFFER SUBMENU
// ============================================================
MenuItem _itemsProbeSniffer[] = {
    { "Dinle",              MENU_ITEM_ACTION,  NULL, _cb_probeSniff, NULL, NULL },
    { "< Geri",             MENU_ITEM_BACK,    NULL, NULL,          NULL, NULL },
};
MenuScreen _menuProbeSniffer = { "Probe Sniffer", _itemsProbeSniffer, 2, &_menuMain };

// ============================================================
// SETTINGS SUBMENU
// ============================================================
MenuItem _itemsSettings[] = {
    { "Hakkimda",           MENU_ITEM_ACTION,  NULL, _cb_about,   NULL, NULL },
    { "< Geri",             MENU_ITEM_BACK,    NULL, NULL,        NULL, NULL },
};
MenuScreen _menuSettings = { "Ayarlar", _itemsSettings, 2, &_menuMain };

// ============================================================
// CLASS MEMBER DEFINITIONS
// ============================================================
MenuScreen MenuManager::_mainMenu = _menuMain;

// ============================================================
// CONSTRUCTOR
// ============================================================
MenuManager::MenuManager()
    : _btn(NULL)
    , _disp(NULL)
    , _currentScreen(NULL)
    , _selectedIndex(0)
    , _scrollOffset(0)
    , _actionRunning(false)
    , _needsRedraw(true)
    , _showingMessage(false)
    , _messageStartMs(0)
    , _messageDurationMs(0)
    , _lastButtonRepeatMs(0)
    , _repeatButtonId(255)
    , _buttonWasPressed(false)
{
    _message[0] = '\0';
}

// ============================================================
// BEGIN
// ============================================================
void MenuManager::begin(ButtonHandler* btnHandler, DisplayManager* dispMgr) {
    _btn = btnHandler;
    _disp = dispMgr;
    _navigateTo(&_mainMenu);
}

// ============================================================
// UPDATE (cagri her loop()'ta yapilmalidir)
// ============================================================
void MenuManager::update() {
    if (!_btn || !_disp) return;

    // --- Mesaj zamanlayici kontrolu ---
    if (_showingMessage) {
        if (millis() - _messageStartMs >= _messageDurationMs) {
            _showingMessage = false;
            _needsRedraw = true;
        }
    }

    // --- Input isleme ---
    _handleInput();

    // --- Ekran guncelleme ---
    if (_needsRedraw) {
        _drawMenu();
        _needsRedraw = false;
    }
}

// ============================================================
// INPUT HANDLER
// ============================================================
void MenuManager::_handleInput() {
    unsigned long now = millis();

    // UP BUTTON
    ButtonEvent evUp = _btn->getEvent(BTN_UP_ID);
    if (evUp == BTN_EVENT_PRESSED || evUp == BTN_EVENT_LONG_PRESS) {
        if (!_actionRunning && _currentScreen && _currentScreen->itemCount > 0) {
            if (_selectedIndex > 0) {
                _selectedIndex--;
                _scrollTo(_selectedIndex);
            } else {
                // Wrap to bottom
                _selectedIndex = _currentScreen->itemCount - 1;
                _scrollTo(_selectedIndex);
            }
        }
        _buttonWasPressed = true;
        _repeatButtonId = BTN_UP_ID;
        _lastButtonRepeatMs = now;
    }
    if (evUp == BTN_EVENT_RELEASED) {
        if (_repeatButtonId == BTN_UP_ID) {
            _repeatButtonId = 255;
            _buttonWasPressed = false;
        }
    }

    // DOWN BUTTON
    ButtonEvent evDown = _btn->getEvent(BTN_DOWN_ID);
    if (evDown == BTN_EVENT_PRESSED || evDown == BTN_EVENT_LONG_PRESS) {
        if (!_actionRunning && _currentScreen && _currentScreen->itemCount > 0) {
            if (_selectedIndex < _currentScreen->itemCount - 1) {
                _selectedIndex++;
                _scrollTo(_selectedIndex);
            } else {
                // Wrap to top
                _selectedIndex = 0;
                _scrollTo(_selectedIndex);
            }
        }
        _buttonWasPressed = true;
        _repeatButtonId = BTN_DOWN_ID;
        _lastButtonRepeatMs = now;
    }
    if (evDown == BTN_EVENT_RELEASED) {
        if (_repeatButtonId == BTN_DOWN_ID) {
            _repeatButtonId = 255;
            _buttonWasPressed = false;
        }
    }

    // Auto-repeat: holding UP/DOWN
    if (_buttonWasPressed && (_repeatButtonId == BTN_UP_ID || _repeatButtonId == BTN_DOWN_ID)) {
        if (now - _lastButtonRepeatMs > 500) { // 500ms initial delay
            static unsigned long repeatInterval = 200; // 200ms repeat
            if (now - _lastButtonRepeatMs > 500 + repeatInterval) {
                _lastButtonRepeatMs = now - 500 + repeatInterval;
                if (_repeatButtonId == BTN_UP_ID) {
                    if (_selectedIndex > 0) { _selectedIndex--; _scrollTo(_selectedIndex); }
                } else {
                    if (_selectedIndex < _currentScreen->itemCount - 1) { _selectedIndex++; _scrollTo(_selectedIndex); }
                }
            }
        }
    }

    // SELECT BUTTON
    ButtonEvent evSel = _btn->getEvent(BTN_SELECT_ID);
    if (evSel == BTN_EVENT_PRESSED || evSel == BTN_EVENT_LONG_PRESS) {
        if (!_actionRunning) {
            _executeSelected();
        }
    }

    // BACK BUTTON - menu gecisi + saldiri durdurma
    ButtonEvent evBack = _btn->getEvent(BTN_BACK_ID);
    if (evBack == BTN_EVENT_PRESSED) {
        if (_actionRunning) {
            _actionRunning = false; // .ino loop saldirilari durduracak
            _needsRedraw = true;
        }
        _navigateBack();
    }
    if (evBack == BTN_EVENT_LONG_PRESS) {
        _actionRunning = false;
        _needsRedraw = true;
        while (_currentScreen && _currentScreen->parent) {
            _navigateBack();
        }
    }
}

// ============================================================
// EXECUTE SELECTED MENU ITEM
// ============================================================
void MenuManager::_executeSelected() {
    if (!_currentScreen || _selectedIndex >= _currentScreen->itemCount) return;

    MenuItem* item = &_currentScreen->items[_selectedIndex];

    switch (item->type) {
        case MENU_ITEM_SUBMENU:
            if (item->submenu) _navigateTo(item->submenu);
            break;

        case MENU_ITEM_ACTION:
            if (item->action) {
                bool started = item->action(this);
                if (started) setActionRunning(true);
            }
            break;

        case MENU_ITEM_TOGGLE:
            if (item->toggleValue) {
                *(item->toggleValue) = !*(item->toggleValue);
                _needsRedraw = true;
            }
            break;

        case MENU_ITEM_BACK:
            _navigateBack();
            break;
    }
}

// ============================================================
// DRAW MENU ON OLED
// ============================================================
void MenuManager::_drawMenu() {
    if (!_disp) return;

    _disp->clear();

    // --- Mesaj overlay gosterimi ---
    if (_showingMessage) {
        _disp->setTextSize(1);
        _disp->setTextColor(SSD1306_WHITE);
        int16_t x = (OLED_WIDTH - strlen(_message) * 6) / 2;
        _disp->setCursor(max((int16_t)0, x), OLED_HEIGHT / 2 - 4);
        _disp->print(_message);
        _disp->display();
        return;
    }

    // --- Menu basligi ---
    if (_currentScreen) {
        _disp->fillRect(0, 0, OLED_WIDTH, 10, SSD1306_WHITE);
        _disp->setTextSize(1);
        _disp->setTextColor(SSD1306_BLACK);
        _disp->setCursor(1, 1);
        _disp->print(_currentScreen->title);
    }

    // --- Menu ogeleri ---
    if (!_currentScreen || _currentScreen->itemCount == 0) {
        _disp->setTextSize(1);
        _disp->setTextColor(SSD1306_WHITE);
        _disp->setCursor(10, OLED_HEIGHT / 2);
        _disp->print("Menu bos");
        _disp->display();
        return;
    }

    int yStart = 12; // Basligin altindan basla
    int yPos = yStart;

    for (int i = _scrollOffset; i < _currentScreen->itemCount && i < _scrollOffset + MAX_VISIBLE_ITEMS; i++) {
        MenuItem* item = &_currentScreen->items[i];

        // Secili ogeyi vurgula
        if (i == _selectedIndex) {
            _disp->fillRect(0, yPos, OLED_WIDTH, MENU_ITEM_HEIGHT, SSD1306_WHITE);
            _disp->setTextColor(SSD1306_BLACK);
        } else {
            _disp->setTextColor(SSD1306_WHITE);
        }

        char displayBuf[22];
        int bufPos = 0;

        // Oge tipine gore on-ek
        if (item->type == MENU_ITEM_SUBMENU) {
            bufPos += snprintf(displayBuf, sizeof(displayBuf), "> %s", item->label);
        } else if (item->type == MENU_ITEM_BACK) {
            bufPos += snprintf(displayBuf, sizeof(displayBuf), "%s", item->label);
        } else if (item->type == MENU_ITEM_ACTION) {
            bufPos += snprintf(displayBuf, sizeof(displayBuf), ">> %s", item->label);
        } else {
            bufPos += snprintf(displayBuf, sizeof(displayBuf), "  %s", item->label);
        }

        _disp->setTextSize(1);
        _disp->setCursor(2, yPos + 1);
        _disp->print(displayBuf);

        yPos += MENU_ITEM_HEIGHT;
    }

    // --- Kaydirma oklari ---
    if (_scrollOffset > 0) {
        _disp->setTextSize(1);
        _disp->setTextColor(SSD1306_WHITE);
        _disp->setCursor(OLED_WIDTH - 8, yStart + 1);
        _disp->print("^");
    }
    if (_scrollOffset + MAX_VISIBLE_ITEMS < _currentScreen->itemCount) {
        _disp->setTextSize(1);
        _disp->setTextColor(SSD1306_WHITE);
        _disp->setCursor(OLED_WIDTH - 8, OLED_HEIGHT - 10);
        _disp->print("v");
    }

    // --- Calisan saldiri gostergesi ---
    if (_actionRunning) {
        _disp->setTextSize(1);
        _disp->setTextColor(SSD1306_WHITE);
        _disp->setCursor(OLED_WIDTH - 40, 1);
        _disp->print("CALISIYOR");
    }

    _disp->display();
}

// ============================================================
// NAVIGATION
// ============================================================
void MenuManager::_navigateTo(MenuScreen* screen) {
    if (!screen) return;
    _currentScreen = screen;
    _selectedIndex = 0;
    _scrollOffset = 0;
    _needsRedraw = true;
}

void MenuManager::_navigateBack() {
    if (_currentScreen && _currentScreen->parent) {
        _navigateTo(_currentScreen->parent);
    }
}

void MenuManager::_scrollTo(int8_t index) {
    if (index < _scrollOffset) {
        _scrollOffset = index;
    } else if (index >= _scrollOffset + MAX_VISIBLE_ITEMS) {
        _scrollOffset = index - MAX_VISIBLE_ITEMS + 1;
    }
    _needsRedraw = true;
}

// ============================================================
// PUBLIC API
// ============================================================
void MenuManager::setScreen(MenuScreen* screen) { _navigateTo(screen); }
MenuScreen* MenuManager::getCurrentScreen() { return _currentScreen; }
int8_t MenuManager::getSelectedIndex() { return _selectedIndex; }
bool MenuManager::isActionRunning() { return _actionRunning; }

void MenuManager::setActionRunning(bool running) {
    _actionRunning = running;
    _needsRedraw = true;
}

void MenuManager::showMessage(const char* msg, uint16_t durationMs) {
    strncpy(_message, msg, sizeof(_message) - 1);
    _message[sizeof(_message) - 1] = '\0';
    _messageStartMs = millis();
    _messageDurationMs = durationMs;
    _showingMessage = true;
    _needsRedraw = true;
}

void MenuManager::requestRedraw() { _needsRedraw = true; }
