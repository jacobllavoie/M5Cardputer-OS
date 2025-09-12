// M5Cardputer-OS main application
// Copyright (c) 2025 Jacob Lavoie
// Licensed under the MIT license. See LICENSE file in the project root for details.
#include <M5CardputerOS_core.h>
#include <esp_system.h>
#include <ui.h>
#include <input.h>
#include <settings_manager.h>

// --- Conditional Includes ---
#ifdef ENABLE_SD_CARD
#include <sd_card.h>
#endif
#ifdef ENABLE_WIFI
#include <wifi.h>
#endif
#ifdef ENABLE_WEB_SERVER
#include <web_server.h>
#endif
#ifdef ENABLE_OTA
#include <ota.h>
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
    settings_init();
    menuTextSize = (float)settings_get_font_size() / 10.0f;
    String savedFontName = settings_get_font_name();
    for (int i = 0; i < numAvailableFonts; ++i) {
        if (savedFontName == availableFonts[i].name) {
            currentFontSelection = i;
            break;
        }
    }
    serialStatus = "initialized";
    drawStartupScreen(serialStatus, sdStatus, wifiStatus, ip, false);
    delay(700);
    #endif

    // Update SD card status only in boot list
    #ifdef ENABLE_SD_CARD
    if (mountSD()) {
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
    String ssid = settings_get_wifi_ssid();
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
    M5Cardputer.Display.setFont(availableFonts[currentFontSelection].font);
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