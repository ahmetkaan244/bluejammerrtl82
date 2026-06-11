#ifndef CONFIG_H
#define CONFIG_H

// === DISPLAY (OLED) ===
#define OLED_ADDR    0x3C
// NOTE: On RTL8720DN (BW16), PA_26 = Arduino pin 8, PA_25 = Arduino pin 7
// Chip GPIO numbers (PA_26/PA_25) are NOT valid Arduino pin numbers — use 8 and 7 instead.
#define OLED_SDA     8   // PA_26 (chip GPIO) = Arduino D8 on BW16
#define OLED_SCL     7   // PA_25 (chip GPIO) = Arduino D7 on BW16
#define OLED_WIDTH   128
#define OLED_HEIGHT  64

// === BUTTONS ===
#define BTN_UP       PA14
#define BTN_DOWN     PA30
#define BTN_SELECT   PA12
#define BTN_BACK     PA7

#define DEBOUNCE_DELAY_MS  50   // Button debounce interval in ms
#define LONG_PRESS_MS      1000 // Long press threshold in ms

// === MENU ===
#define MENU_MAX_ITEMS      10
#define MENU_ITEM_HEIGHT    12  // Pixels per menu item line

// === WIFI ===
#define WIFI_CHANNEL_MAX    13  // 2.4GHz channels
#define WIFI_SCAN_INTERVAL  3000 // ms between scans

// === BLE ===
#define BLE_SPAM_INTERVAL   20  // ms between BLE advertisement bursts

// === DEAUTH ===
#define DEAUTH_PACKET_RATE  100 // packets per second (single target)
#define DEAUTH_ALL_RATE     50  // packets per second per network (all)

// === BEACON FLOOD ===
#define BEACON_FLOOD_RATE   10   // beacons per second per channel

// === PROBE SNIFFER ===
#define PROBE_LIST_MAX      20  // Max entries in probe display list

// === EVIL TWIN ===
#define CAPTIVE_PORTAL_PORT 80

// === DDoS ===
#define AUTH_FLOOD_RATE     200 // packets per second

// === TIMING ===
#define BOOT_SPLASH_DURATION 3000 // ms to show boot logo

#endif // CONFIG_H
