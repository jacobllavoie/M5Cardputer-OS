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

    preferences.begin("display-settings", true);
    menuTextSize = preferences.getFloat("fontSize", 0.8f);
    preferences.end();
    #ifdef ENABLE_SD_CARD
    displayMessage("Mounting SD Card...", "", 1000);
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