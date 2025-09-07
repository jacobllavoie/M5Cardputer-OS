#include "globals.h"
#include "ui.h"
#include "input.h"
#include "sd_card.h"
#include "wifi.h"
#include "serial.h" 
#include "web_server.h"
#include "ota.h"

// Timer for battery status refresh
unsigned long last_battery_update = 0;
const int battery_update_interval = 2000;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true); 
    Serial.begin(115200);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setRotation(1);

    displayMessage("Mounting SD Card...", "", 1000);
    mountSD();
    
    wifiAutoConnect();
    drawScreen();
}

void loop() {
    M5Cardputer.update();

    if (currentState == STATE_WEB_SERVER_ACTIVE) {
        handleWebServerClient();
    } else if (currentState == STATE_OTA_MODE) {
        handleOTA();
    }

    handleInput();
    handleSerial();

    if (currentState == STATE_MAIN_MENU) {
        if (millis() - last_battery_update > battery_update_interval) {
            drawBatteryStatus();
            last_battery_update = millis();
        }
    }
}