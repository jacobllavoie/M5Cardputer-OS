// M5Cardputer-OS main application
// Copyright (c) 2025 Jacob Lavoie
// Licensed under the MIT license. See LICENSE file in the project root for details.
#include <M5CardputerOS_core.h>
#include <esp_system.h>
#include <ui.h>
#include <input.h>
#include <settings_manager.h>
#include <Adafruit_NeoPixel.h>

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

// --- Heartbeat LED Settings ---
// #define NEOPIXEL_PIN 21
// Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int heartbeat_speed = 10; // Lower is faster, higher is slower
unsigned long last_heartbeat_update = 0;
int current_color = 0; // 0=Red, 1=Green, 2=Blue
int brightness = 0;
bool increasing = true;

void handleHeartbeat() {
    if (millis() - last_heartbeat_update > heartbeat_speed) {
        last_heartbeat_update = millis();

        if (increasing) {
            brightness++;
            if (brightness >= 255) {
                brightness = 255;
                increasing = false;
            }
        } else {
            brightness--;
            if (brightness <= 0) {
                brightness = 0;
                increasing = true;
                current_color = (current_color + 1) % 3; // Cycle to the next color
            }
        }

        switch (current_color) {
            case 0: // Red
                pixel.setPixelColor(0, pixel.Color(brightness, 0, 0));
                break;
            case 1: // Green
                pixel.setPixelColor(0, pixel.Color(0, brightness, 0));
                break;
            case 2: // Blue
                pixel.setPixelColor(0, pixel.Color(0, 0, brightness));
                break;
        }
        pixel.show();
    }
}

void setup() {
    // ...existing code...
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    pixel.begin();
    pixel.setBrightness(255); // Set initial brightness
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
            #ifdef ENABLE_WEB_SERVER
            startWebServer();
            #endif
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
    M5Cardputer.Display.setTextSize(1.75);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.setTextColor(WHITE, BACKGROUND_COLOR);
    M5Cardputer.Display.drawString("Welcome", M5Cardputer.Display.width()/2, M5Cardputer.Display.height()/2);
    delay(1200);
    #endif

    drawScreen();
}

void loop() {
    M5Cardputer.update();
    handleHeartbeat();

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