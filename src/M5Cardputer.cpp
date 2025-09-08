#include "globals.h"
#include "ui.h"
#include "input.h"
#include "serial.h" 

// --- Conditional Includes ---
#ifdef ENABLE_SD_CARD
#include "sd_card.h"
#endif
#ifdef ENABLE_WIFI
#include "wifi.h"
#endif
#ifdef ENABLE_WEB_SERVER
#include "web_server.h"
#endif
#ifdef ENABLE_OTA
#include "ota.h"
#endif

// Timer for battery status refresh
unsigned long last_battery_update = 0;
const int battery_update_interval = 2000;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setRotation(1);

    initializeMenus();

    #ifdef ENABLE_SETTINGS_PERSISTENCE
    // Open preferences and check if it was successful
    if (preferences.begin("disp-settings", true)) {
        menuTextSize = preferences.getFloat("fontSize", 0.8f);
        preferences.end();
        displayMessage("Loaded Font Size:", String(menuTextSize, 2), 1500);
    } else {
        displayMessage("Error:", "Failed to load settings!", 2000);
    }
    #endif

    #ifdef ENABLE_SD_CARD
    mountSD();
    #endif
    
    #ifdef ENABLE_WIFI
    wifiAutoConnect();
    #endif

    drawScreen();
}

void loop() {
    M5Cardputer.update();

    #ifdef ENABLE_WEB_SERVER
    if (currentState == STATE_WEB_SERVER_ACTIVE) {
        handleWebServerClient();
    } 
    #endif
    
    #ifdef ENABLE_OTA
    if (currentState == STATE_OTA_MODE) {
        handleOTA();
    }
    #endif

    handleInput();
    handleSerial();

    if (currentState == STATE_MAIN_MENU) {
        if (millis() - last_battery_update > battery_update_interval) {
            drawBatteryStatus();
            last_battery_update = millis();
        }
    }
}