#ifndef GLOBALS_H
#define GLOBALS_H

#include "M5Cardputer.h"
#include <SPI.h>
#include "SdFat.h"
#include <WiFi.h>
#include <Preferences.h>
#include <vector>
#include "Update.h"
#include "esp_ota_ops.h"

// --- Objects ---
extern SdFs sd;
extern Preferences preferences;

// --- UI Configuration ---
extern float menuTextSize;

// --- Application State ---
enum AppState {
    STATE_MAIN_MENU,
    STATE_APPS_MENU,
    STATE_SETTINGS_MENU,
    STATE_DISPLAY_SETTINGS_MENU,
    STATE_SDCARD_SETTINGS_MENU,
    STATE_WIFI_SETTINGS_MENU,
    STATE_WIFI_SCAN_RESULTS,
    STATE_WIFI_PASSWORD_INPUT,
    STATE_FACTORY_RESET_CONFIRM,
    STATE_WEB_SERVER_ACTIVE,
    STATE_OTA_MODE,
    STATE_KEYBOARD_TEST
};
extern AppState currentState;

// --- SD Card Status ---
extern bool isSdCardMounted;

// --- Keyboard Test Globals ---
extern String lastKeyPressed;

// --- WiFi Globals ---
extern std::vector<String> scanned_networks;
extern int selected_network_index;
extern String selected_ssid;
extern String password_buffer;

// --- Menu Definitions & Selections ---
extern const char* mainMenuItems[];
extern const int numMainMenuItems;
extern int currentMainMenuSelection;

extern std::vector<String> app_list;
extern int currentAppSelection;

extern const char* settingsMenuItems[];
extern const int numSettingsMenuItems;
extern int currentSettingsSelection;

extern const char* displayMenuItems[];
extern const int numDisplayMenuItems;
extern int currentDisplaySelection;

extern const char* sdCardMenuItems[];
extern const int numSdCardMenuItems;
extern int currentSdCardSelection;

extern const char* wifiMenuItems[];
extern const int numWifiMenuItems;
extern int currentWifiSelection;

// --- Color Scheme ---
#define BACKGROUND_COLOR BLACK
#define TEXT_COLOR GREEN
#define HIGHLIGHT_COLOR WHITE
#define HIGHLIGHT_TEXT_COLOR BLACK
#define WARNING_COLOR RED

#endif