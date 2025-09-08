#ifndef GLOBALS_H
#define GLOBALS_H

#include "M5Cardputer.h"
#include <SPI.h>
#include <Preferences.h>
#include <vector>

// --- Conditional Includes based on platformio.ini ---
#ifdef ENABLE_SD_CARD
#include "SdFat.h"
#endif

#ifdef ENABLE_WIFI
#include <WiFi.h>
#endif

#ifdef ENABLE_OTA
#include "Update.h"
#include "esp_ota_ops.h"
#endif

#ifdef ENABLE_SETTINGS_PERSISTENCE
#include <Preferences.h>
#endif

// --- Objects ---
#ifdef ENABLE_SD_CARD
extern SdFs sd;
#endif
#ifdef ENABLE_SETTINGS_PERSISTENCE
extern Preferences preferences;
#endif

// --- UI Configuration ---
extern float menuTextSize;

// --- Application State ---
enum AppState {
    STATE_MAIN_MENU,
    STATE_APPS_MENU,
    STATE_SETTINGS_MENU,
    STATE_DISPLAY_SETTINGS_MENU,
    STATE_FACTORY_RESET_CONFIRM,
    STATE_KEYBOARD_TEST,
    
    #ifdef ENABLE_SD_CARD
    STATE_SDCARD_SETTINGS_MENU,
    #endif

    #ifdef ENABLE_WIFI
    STATE_WIFI_SETTINGS_MENU,
    STATE_WIFI_SCAN_RESULTS,
    STATE_WIFI_PASSWORD_INPUT,
    #endif

    #ifdef ENABLE_WEB_SERVER
    STATE_WEB_SERVER_ACTIVE,
    #endif

    #ifdef ENABLE_OTA
    STATE_OTA_MODE,
    #endif
};
extern AppState currentState;

// --- SD Card Status ---
#ifdef ENABLE_SD_CARD
extern bool isSdCardMounted;
#endif

// --- Keyboard Test Globals ---
extern String lastKeyPressed;

// --- WiFi Globals ---
#ifdef ENABLE_WIFI
extern std::vector<String> scanned_networks;
extern int selected_network_index;
extern String selected_ssid;
extern String password_buffer;
#endif

// --- Menu Definitions & Selections ---
extern const char* mainMenuItems[];
extern const int numMainMenuItems;
extern int currentMainMenuSelection;

extern std::vector<String> app_list;
extern int currentAppSelection;

extern std::vector<const char*> settingsMenuItems;
extern int numSettingsMenuItems;
extern int currentSettingsSelection;

extern const char* displayMenuItems[];
extern const int numDisplayMenuItems;
extern int currentDisplaySelection;

#ifdef ENABLE_SD_CARD
extern const char* sdCardMenuItems[];
extern const int numSdCardMenuItems;
extern int currentSdCardSelection;
#endif

#ifdef ENABLE_WIFI
extern const char* wifiMenuItems[];
extern const int numWifiMenuItems;
extern int currentWifiSelection;
#endif

// --- Color Scheme ---
#define BACKGROUND_COLOR BLACK
#define TEXT_COLOR GREEN
#define HIGHLIGHT_COLOR WHITE
#define HIGHLIGHT_TEXT_COLOR BLACK
#define WARNING_COLOR RED

#endif