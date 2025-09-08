#include "globals.h"
#include <esp_system.h>
#include "ui.h"
#include "input.h"

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
    // ...existing code...
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setRotation(1);

    #ifdef ENABLE_VERBOSE_BOOT
    String serialStatus = "initializing";
    String sdStatus = "mounting";
    String wifiStatus = "idle";
    String ip = "...";

    // Show initial boot list
    drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
    delay(800);

    initializeMenus();

    // Update serial status only in boot list
    #ifdef ENABLE_SETTINGS_PERSISTENCE
    if (preferences.begin("disp-settings", true)) {
        menuTextSize = preferences.getFloat("fontSize", 0.8f);
        preferences.end();
        serialStatus = "initialized";
    } else {
        serialStatus = "error";
    }
    drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
    delay(700);
    #endif

    // Update SD card status only in boot list
    #ifdef ENABLE_SD_CARD
    mountSD();
    if (isSdCardMounted) {
        sdStatus = "mounted";
        if (!SD.exists("/apps")) {
            debugMessage("DEBUG:", "Creating /apps dir on SD card");
            SD.mkdir("/apps");
        }
    } else {
        sdStatus = "error";
    }
    drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
    delay(900);
    #endif

    // Update WiFi status only in boot list
    #ifdef ENABLE_WIFI
    preferences.begin("wifi", true);
    String ssid = preferences.getString("ssid", "");
    preferences.end();
    if (ssid.length() > 0) {
        wifiStatus = "connecting";
        drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
        wifiAutoConnect(false);
        if (WiFi.status() == WL_CONNECTED) {
            wifiStatus = "connected";
            ip = WiFi.localIP().toString();
        } else {
            wifiStatus = "error";
        }
        drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
        delay(900);
    }
    else {
        drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
        delay(600);
    }
    #endif

    drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
    delay(1200);

    // Show WELCOME screen
    M5Cardputer.Display.fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
    M5Cardputer.Display.setTextSize(1.5f);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.setTextColor(COLOR_CYAN, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("Welcome", M5Cardputer.Display.width()/2, M5Cardputer.Display.height()/2);
    delay(1200);
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

    if (currentState == STATE_MAIN_MENU) {
        if (millis() - last_battery_update > battery_update_interval) {
            drawBatteryStatus();
            last_battery_update = millis();
        }
    }
}